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
#include <bitset>
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
	std::cout << "\nUsage : " << command << " [options]" << std::endl << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout << "  -x  --xappname  xApp instance name" << std::endl;
	std::cout << "  -i  --xappid    xApp instance id" << std::endl;
	std::cout << "  -p  --port      Port to listen on (e.g. 4560)" << std::endl;
	std::cout << "  -t  --threads   Number of listener threads" << std::endl;
	std::cout << "  -b  --nodebid   E2 NodeB ID to subscribe (e.g. 12, 0xC)" << std::endl;
	std::cout << "  -c  --mcc       Mobile Country Code of NodeB to subscribe" << std::endl;
	std::cout << "  -n  --mnc       Mobile Network Code of NodeB to subscribe" << std::endl;
	std::cout << "  -h  --help      Shows this information and quit" << std::endl << std::endl;
}

void XappSettings::loadCmdlineSettings(int argc, char **argv)
{

	// Parse command line options to override
	static struct option long_options[] =
		{
			{"xappname", required_argument, 0, 'x'},
			{"xappid", required_argument, 0, 'i'},
			{"port", required_argument, 0, 'p'},
			{"threads", required_argument, 0, 't'},
			{"nodebid", required_argument, 0, 'b'},
			{"mcc", required_argument, 0, 'c'},
			{"mnc", required_argument, 0, 'n'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};

	while (1) {
		int option_index = 0;
		char c = getopt_long(argc, argv, "x:i:p:t:b:c:n:h", long_options, &option_index);

		if (c == -1) {
			break;
		}

		switch (c) {
			case 'x':
				theSettings[XAPP_NAME].assign(optarg);
				mdclog_write(MDCLOG_INFO, "xApp Name set to %s from command line\n", theSettings[XAPP_NAME].c_str());
				break;

			case 'i':
				theSettings[XAPP_ID].assign(optarg);
				mdclog_write(MDCLOG_INFO, "xApp ID set to %s from command line\n", theSettings[XAPP_ID].c_str());
				break;

			case 'p':
				theSettings[BOUNCER_PORT].assign(optarg);
				mdclog_write(MDCLOG_INFO, "xApp Port set to %s from command line\n", theSettings[BOUNCER_PORT].c_str());
				break;

			case 't':
				theSettings[THREADS].assign(optarg);
				mdclog_write(MDCLOG_INFO, "Number of threads set to %s from command line\n", theSettings[THREADS].c_str());
				break;

			case 'b':
			{
				unsigned long nodebid_num = 0;
				try {
					if ((strlen(optarg) > 2) && (optarg[0] == '0') && (optarg[1] == 'x' || optarg[1] == 'X')) {	// check if hex value
						nodebid_num = std::stoul(optarg, nullptr, 16);
					} else {	// we assume it is decimal
						nodebid_num = std::stoul(optarg, nullptr, 10);
					}

				} catch (std::exception &e) {
					mdclog_write(MDCLOG_ERR, "unable to parse NodeB ID to binary value");
					usage(argv[0]);
					exit(1);
				}

				theSettings[NODEB_ID].assign( bitset<32>(nodebid_num).to_string() );
				mdclog_write(MDCLOG_INFO, "E2NodeB ID set to %s (%s) from command line\n", optarg, theSettings[NODEB_ID].c_str());
			}
				break;

			case 'c':
				if (strlen(optarg) != 3) {
					mdclog_write(MDCLOG_ERR, "MCC requires 3 digits\n");
					exit(1);
				}
				theSettings[MCC].assign(optarg);
				mdclog_write(MDCLOG_INFO, "MCC set to %s from command line\n", theSettings[MCC].c_str());
				break;

			case 'n':
			{
				size_t len = strlen(optarg);
				if (len != 2 && len != 3) {
					mdclog_write(MDCLOG_ERR, "MNC requires 2 or 3 digits\n");
					exit(1);
				}
			}
				theSettings[MNC].assign(optarg);
				mdclog_write(MDCLOG_INFO, "MNC set to %s from command line\n", theSettings[MNC].c_str());
				break;

			case 'h':
				usage(argv[0]);
				exit(0);

			default:
				usage(argv[0]);
				exit(1);
			}
	}
}

void XappSettings::loadDefaultSettings(){

	if(theSettings[XAPP_NAME].empty()){
		theSettings[XAPP_NAME] = DEFAULT_XAPP_NAME;
	}
	if(theSettings[XAPP_ID].empty()){
		theSettings[XAPP_ID] = DEFAULT_XAPP_NAME; // xapp_id and xapp_name are the same by default
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
	if(theSettings[MCC].empty()){
		theSettings[MCC] = DEFAULT_MCC;
	}
	if(theSettings[MNC].empty()){
		theSettings[MNC] = DEFAULT_MNC;
	}

}

void XappSettings::loadEnvVarSettings(){

	if (const char *env_xname = std::getenv("XAPP_NAME")){
		theSettings[XAPP_NAME].assign(env_xname);
		mdclog_write(MDCLOG_INFO,"Xapp Name set to %s from environment variable", theSettings[XAPP_NAME].c_str());
	}
	if (const char *env_xid = std::getenv("XAPP_ID")){
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
		mdclog_write(MDCLOG_ERR, "unable to build E2 gNodeB name");
	}

	// ASN_STRUCT_RESET(asn_DEF_E2setupRequestIEs, &ie) won't work here since we don't fill all fields in the E2setupRequestIEs_t
	free(gnb_id_str->buf);
	free(plmn->buf);
	free(gNB);

	mdclog_write(MDCLOG_DEBUG, "Global gNodeB ID has been built to %s", gnb_str.c_str());

	return gnb_str;
}

string XappSettings::buildGlobalENodeBId(uint8_t *plmn_id, uint32_t enb_id) {
	string enb_str;

	mdclog_write(MDCLOG_DEBUG, "in %s function", __func__);

	size_t len = strlen((char *)plmn_id); // maximum plmn_id size must be 3 octet string bytes
	if (len > 3) {
		throw invalid_argument("maximum plmn_id size is 3");
	}

	if (enb_id >= 1<<20) {
		throw invalid_argument("maximum macro enb_id value is 2^20-1");
	}

	E2setupRequestIEs_t ie;
	ie.value.present = E2setupRequestIEs__value_PR_GlobalE2node_ID;
	ie.value.choice.GlobalE2node_ID.present = GlobalE2node_ID_PR_eNB;
	GlobalE2node_eNB_ID_t *eNB = (GlobalE2node_eNB_ID_t *) calloc(1, sizeof(GlobalE2node_eNB_ID_t));
	ie.value.choice.GlobalE2node_ID.choice.eNB = eNB;

	// encoding PLMN identity
	PLMNIdentity_t *plmn = &ie.value.choice.GlobalE2node_ID.choice.eNB->global_eNB_ID.pLMNIdentity;
	plmn->buf = (uint8_t *) calloc(len, sizeof(uint8_t));
	plmn->size = len;
	memcpy(plmn->buf, plmn_id, len);

	// encoding eNodeB Choice
	ie.value.choice.GlobalE2node_ID.choice.eNB->global_eNB_ID.eNB_ID.present = ENB_ID_PR_macro_eNB_ID;
	BIT_STRING_t *enb_id_str = &ie.value.choice.GlobalE2node_ID.choice.eNB->global_eNB_ID.eNB_ID.choice.macro_eNB_ID;

	// encoding macro eNodeB identity
	enb_id_str->buf = (uint8_t *) calloc(1, 3); // maximum size is 24 bits
	enb_id_str->size = 3;
	enb_id_str->bits_unused = 4; // we are using the leftmost 20 bits of the E-UTRAN Cell Identifier for the macro enb_id
	enb_id = ((enb_id & 0X000FFFFF) << 12);	// 20 + 12 bits equals 32 bits (uint32_t)
	enb_id_str->buf[0] = ((enb_id & 0XFF000000) >> 24);
	enb_id_str->buf[1] = ((enb_id & 0X00FF0000) >> 16);
	enb_id_str->buf[2] = ((enb_id & 0X0000FF00) >> 8);

	char buf[256] = {0, };
	int ret = buildRanName(buf, &ie);
	if (ret == 0) {
		enb_str.assign(buf);
	} else {
		mdclog_write(MDCLOG_ERR, "unable to build E2 eNodeB name");
	}

	// ASN_STRUCT_RESET(asn_DEF_E2setupRequestIEs, &ie) won't work here since we don't fill all fields in the E2setupRequestIEs_t
	free(enb_id_str->buf);
	free(plmn->buf);
	free(eNB);

	mdclog_write(MDCLOG_DEBUG, "Global Macro eNodeB ID has been built to %s", enb_str.c_str());

    return enb_str;
}

/*
	Builds the PLMN ID based on MCC and MNC

	410 32 becomes 14 F0 23
	or
	410 532 becomes 14 50 23
*/
string XappSettings::buildPlmnId() {
	const char *mcc = theSettings[MCC].c_str();
	const char *mnc = theSettings[MNC].c_str();
	string plmnid = "";
	plmnid.reserve(6);
	plmnid += mcc[1];
	plmnid += mcc[0];
	if (strlen(mnc) == 3) {
		plmnid += mnc[0];
		plmnid += mcc[2];
		plmnid += mnc[2];
		plmnid += mnc[1];
	} else {
		plmnid += 'F';
		plmnid += mcc[2];
		plmnid += mnc[1];
		plmnid += mnc[0];
	}

    return plmnid;
}
