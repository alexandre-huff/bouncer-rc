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


#ifndef SRC_XAPP_CONFIG_XAPP_CONFIG_HPP_
#define SRC_XAPP_CONFIG_XAPP_CONFIG_HPP_

#include <getopt.h>
#include <map>
#include <iostream>
#include <cstdlib>
#include <mdclog/mdclog.h>

#define DEFAULT_XAPP_NAME "bouncer-xapp"
#define DEFAULT_RMR_PORT "4560"
#define DEFAULT_HTTP_PORT "8080"
#define DEFAULT_MSG_MAX_BUFFER "2072"
#define DEFAULT_THREADS "1"

#define DEFAULT_LOG_LEVEL	MDCLOG_WARN
#define DEFAULT_CONFIG_FILE "/opt/ric/config/config-file.json"
#define DEFAULT_MCC "001"
#define DEFAULT_MNC "01"

#define ASN_BUFF_MAX_SIZE		4096
#define MAX_SUBSCRIPTION_ATTEMPTS	10
#define BOUNCER_POLICY_ID 2

using namespace std;

struct XappSettings{

public:
	typedef enum{
		  XAPP_ID,
		  XAPP_NAME,
		  VERSION,
		  BOUNCER_PORT,
		  MSG_MAX_BUFFER,
		  THREADS,
		  LOG_LEVEL,
		  CONFIG_FILE,
		  CONFIG_STR,
		  HTTP_PORT,
		  RMR_SRC_ID,
		  HTTP_SRC_ID,
		  NODEB_ID,	// stored using bit values
		  MCC,
		  MNC
	} SettingName;

	void loadDefaultSettings();
	void loadCmdlineSettings(int, char **);
	void loadEnvVarSettings();
	void loadXappDescriptorSettings();
	void usage(char*);
	string& operator[](const SettingName& theName);

	string buildPlmnId();
private:
	typedef map<SettingName, std::string> SettingCollection;
	SettingCollection theSettings;

	string buildHttpAddress();
	string buildGlobalGNodeBId(uint8_t *plmn_id, uint32_t gnb_id);
	string buildGlobalENodeBId(uint8_t *plmn_id, uint32_t enb_id);

};



#endif /* SRC_XAPP_CONFIG_XAPP_CONFIG_HPP_ */
