/*
# ==================================================================================
# Copyright (c) 2020 HCL Technologies Limited.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==================================================================================
*/

#include <cstdio>
#include <string>
#include <rapidjson/filereadstream.h>
#include <rapidjson/document.h>
#include <rapidjson/pointer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/error.h>

#include "xapp_config.hpp"
#include "BuildRunName.h"

extern "C" {
	#include "ProtocolIE-Field.h"
	#include "PLMN-Identity.h"
	#include "GlobalE2node-ID.h"
	#include "GlobalE2node-gNB-ID.h"
	#include "GlobalgNB-ID.h"
	#include "GNB-ID-Choice.h"
	#include "OCTET_STRING.h"
	#include "BIT_STRING.h"
}

using namespace rapidjson;

string& XappSettings::operator[](const SettingName& theName){
    return theSettings[theName];
}

void XappSettings::usage(char *command){
	std::cout << "Usage : " << command << " " << std::endl;
	std::cout << " --xappname[-n] xapp instance name" << std::endl;
	std::cout << " --xappid[-x] xapp instance id" << std::endl;
    std::cout << " --port[-p] port to listen on e.g 4561" << std::endl;
    std::cout << " --threads[-t] number of listener threads" << std::endl;
	std::cout << " --ves-interval[-i] VES collector interval" << std::endl;
	std::cout << " --gnodeb[-g] gNodeB name to subscribe" << std::endl;
}

void XappSettings::loadCmdlineSettings(int argc, char **argv){

	   // Parse command line options to override
	  static struct option long_options[] =
	    {
	    		{"xappname", required_argument, 0, 'n'},
				{"xappid", required_argument, 0, 'x'},
				{"port", required_argument, 0, 'p'},
				{"threads", required_argument,    0, 't'},
				{"ves-interval", required_argument, 0, 'i'},
				{"gnodeb", required_argument, 0, 'g'}

	    };


	   while(1) {

		int option_index = 0;
		char c = getopt_long(argc, argv, "n:p:t:s:g:a:v:u:i:c:x:", long_options, &option_index);

	        if(c == -1){
		    break;
	         }

		switch(c)
		  {

		  case 'n':
		    theSettings[XAPP_NAME].assign(optarg);
		    break;

		  case 'p':
		    theSettings[BOUNCER_PORT].assign(optarg);
		    break;

		  case 't':
			theSettings[THREADS].assign(optarg);
		    mdclog_write(MDCLOG_INFO, "Number of threads set to %s from command line e\n", theSettings[THREADS].c_str());
		    break;

		  case 'g':
			{
				uint32_t gnb_id = strtoumax(optarg, NULL, 10);
				string meid = buildGlobalGNodeBId((uint8_t *)"747", gnb_id);
				theSettings[G_NODE_B].assign(meid);
			}
		    mdclog_write(MDCLOG_INFO, "gNodeB set to %s from command line\n", theSettings[G_NODE_B].c_str());
		    break;

		  case 'x':
		    theSettings[XAPP_ID].assign(optarg);
		    mdclog_write(MDCLOG_INFO, "XAPP ID set to  %s from command line ", theSettings[XAPP_ID].c_str());
		    break;

		  case 'h':
		    usage(argv[0]);
		    exit(0);

		  default:
		    usage(argv[0]);
		    exit(1);
		  }
	   };

}

void XappSettings::loadDefaultSettings(){

	if(theSettings[XAPP_NAME].empty()){
		theSettings[XAPP_NAME] = DEFAULT_XAPP_NAME;
	}
	if(theSettings[XAPP_ID].empty()){
		theSettings[XAPP_ID] = DEFAULT_XAPP_NAME; //for now xapp_id is same as xapp_name since single xapp instance.
	}
	if(theSettings[LOG_LEVEL].empty()){
		theSettings[LOG_LEVEL] = DEFAULT_LOG_LEVEL;
	}
	if(theSettings[BOUNCER_PORT].empty()){
		theSettings[BOUNCER_PORT] = DEFAULT_RMR_PORT;
	}
	if(theSettings[HTTP_PORT].empty()){
		theSettings[HTTP_PORT] = DEFAULT_HTTP_PORT;
	}
	if(theSettings[MSG_MAX_BUFFER].empty()){
		theSettings[MSG_MAX_BUFFER] = DEFAULT_MSG_MAX_BUFFER;
	}
	if(theSettings[THREADS].empty()){
		theSettings[THREADS] = DEFAULT_THREADS;
	}
	if(theSettings[CONFIG_FILE].empty()){
		theSettings[CONFIG_FILE] = DEFAULT_CONFIG_FILE;
	}

}

void XappSettings::loadEnvVarSettings(){

	if (const char *env_xname = std::getenv("XAPP_NAME")){
		theSettings[XAPP_NAME].assign(env_xname);
		mdclog_write(MDCLOG_INFO,"Xapp Name set to %s from environment variable", theSettings[XAPP_NAME].c_str());
	}
	if (const char *env_xid = std::getenv("XAPP_NAME")){
		theSettings[XAPP_ID].assign(env_xid);
		mdclog_write(MDCLOG_INFO,"Xapp ID set to %s from environment variable", theSettings[XAPP_ID].c_str());
	}
	if (const char *env_ports = std::getenv("BOUNCER_PORT")){
		theSettings[BOUNCER_PORT].assign(env_ports);
		mdclog_write(MDCLOG_INFO,"Ports set to %s from environment variable", theSettings[BOUNCER_PORT].c_str());
	}
	if (const char *env_buf = std::getenv("MSG_MAX_BUFFER")){
		theSettings[MSG_MAX_BUFFER].assign(env_buf);
		mdclog_write(MDCLOG_INFO,"Msg max buffer set to %s from environment variable", theSettings[MSG_MAX_BUFFER].c_str());
	}
	if (const char *env_threads = std::getenv("THREADS")){
		theSettings[THREADS].assign(env_threads);
		mdclog_write(MDCLOG_INFO,"Threads set to %s from environment variable", theSettings[THREADS].c_str());
	}
	if (const char *env_config_file = std::getenv("CONFIG_FILE")){
		theSettings[CONFIG_FILE].assign(env_config_file);
		mdclog_write(MDCLOG_INFO,"Config file set to %s from environment variable", theSettings[CONFIG_FILE].c_str());
	}
	if (char *env = getenv("RMR_SRC_ID")) {
		theSettings[RMR_SRC_ID].assign(env);
		mdclog_write(MDCLOG_INFO,"RMR_SRC_ID set to %s from environment variable", theSettings[RMR_SRC_ID].c_str());
	} else {
		mdclog_write(MDCLOG_ERR, "RMR_SRC_ID env var is not defined");
	}

}

void XappSettings::loadXappDescriptorSettings() {
	mdclog_write(MDCLOG_INFO, "Loading xApp descriptor file");

	FILE *fp = fopen(theSettings[CONFIG_FILE].c_str(), "r");
	if (fp == NULL) {
		mdclog_write(MDCLOG_ERR, "unable to open config file %s, reason = %s",
					theSettings[CONFIG_FILE].c_str(), strerror(errno));
		return;
	}
	char buffer[4096];
	FileReadStream is(fp, buffer, sizeof(buffer));
	Document doc;
	doc.ParseStream(is);

	if (Value *value = Pointer("/version").Get(doc)) {
		theSettings[VERSION].assign(value->GetString());
	} else {
		mdclog_write(MDCLOG_WARN, "unable to get version from config file");
	}
	if (Value *value = Pointer("/messaging/ports").Get(doc)) {
		auto array = value->GetArray();
		for (auto &el : array) {
			if (el.HasMember("name") && el.HasMember("port")) {
				string name = el["name"].GetString();

				if (name.compare("rmr-data") == 0) {
					theSettings[BOUNCER_PORT].assign(to_string(el["port"].GetInt()));

				} else if (name.compare("http") == 0) {
					theSettings[HTTP_PORT].assign(to_string(el["port"].GetInt()));
					theSettings[HTTP_SRC_ID].assign(buildHttpAddress());	// we only set if http port is defined
				}
			}
		}
	} else {
		mdclog_write(MDCLOG_WARN, "unable to get ports from config file");
	}

	StringBuffer outbuf;
	outbuf.Clear();
	Writer<StringBuffer> writer(outbuf);
	doc.Accept(writer);
	theSettings[CONFIG_STR].assign(outbuf.GetString());

	fclose(fp);
}

string XappSettings::buildHttpAddress() {
	string http_addr;
	if (char *env = getenv("RMR_SRC_ID")) {
		http_addr = env;
		size_t pos = http_addr.find("-rmr.");
		if (pos != http_addr.npos ) {
			http_addr = http_addr.replace(http_addr.find("-rmr."), 5, "-http." );
		}
	} else {
		mdclog_write(MDCLOG_ERR, "RMR_SRC_ID env var is not defined");
	}

	return http_addr;
}

string XappSettings::buildGlobalGNodeBId(uint8_t *plmn_id, uint32_t gnb_id) {
	string gnb_str;

	mdclog_write(MDCLOG_DEBUG, "in %s function", __func__);

	size_t len = strlen((char *)plmn_id); // maximum plmn_id size must be 3 octet string bytes
	if (len > 3) {
		throw invalid_argument("maximum plmn_id size is 3");
	}

	if (gnb_id >= 1<<29) {
		throw invalid_argument("maximum gnb_id value is 2^29-1");
	}

	E2setupRequestIEs_t ie;
	ie.value.present = E2setupRequestIEs__value_PR_GlobalE2node_ID;
	ie.value.choice.GlobalE2node_ID.present = GlobalE2node_ID_PR_gNB;
	GlobalE2node_gNB_ID_t *gNB = (GlobalE2node_gNB_ID_t *) calloc(1, sizeof(GlobalE2node_gNB_ID_t));
	ie.value.choice.GlobalE2node_ID.choice.gNB = gNB;

	// encoding PLMN identity
	PLMN_Identity_t *plmn = &ie.value.choice.GlobalE2node_ID.choice.gNB->global_gNB_ID.plmn_id;
	plmn->buf = (uint8_t *) calloc(len, sizeof(uint8_t));
	plmn->size = len;
	memcpy(plmn->buf, plmn_id, len);

	// encoding gNodeB Choice
	ie.value.choice.GlobalE2node_ID.choice.gNB->global_gNB_ID.gnb_id.present = GNB_ID_Choice_PR_gnb_ID;
	BIT_STRING_t *gnb_id_str = &ie.value.choice.GlobalE2node_ID.choice.gNB->global_gNB_ID.gnb_id.choice.gnb_ID;

	// encoding gNodeB identity
	gnb_id_str->buf = (uint8_t *) calloc(1, 4); // maximum size is 32 bits
	gnb_id_str->size = 4;
	gnb_id_str->bits_unused = 3; // we are using 29 bits for gnb_id so that 7 bits (3+4) is left for the NR Cell Identity
	gnb_id = ((gnb_id & 0X1FFFFFFF) << 3);
	gnb_id_str->buf[0] = ((gnb_id & 0XFF000000) >> 24);
	gnb_id_str->buf[1] = ((gnb_id & 0X00FF0000) >> 16);
	gnb_id_str->buf[2] = ((gnb_id & 0X0000FF00) >> 8);
	gnb_id_str->buf[3] = (gnb_id & 0X000000FF);

	char buf[256] = {0, };
	int ret = buildRanName(buf, &ie);
	if (ret == 0) {
		gnb_str.assign(buf);
	} else {
		mdclog_write(MDCLOG_ERR, "unable to build E2Node name");
	}

	// ASN_STRUCT_RESET(asn_DEF_E2setupRequestIEs, &ie) won't work here since we don't fill all fields in the E2setupRequestIEs_t
	free(gnb_id_str->buf);
	free(plmn->buf);
	free(gNB);

	mdclog_write(MDCLOG_DEBUG, "Global gNodeB ID has been built to %s", gnb_str.c_str());

	return gnb_str;
}
