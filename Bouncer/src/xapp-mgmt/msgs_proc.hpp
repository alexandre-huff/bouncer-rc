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


#pragma once

#ifndef XAPP_MSG_XAPP_MSG_HPP_
#define XAPP_MSG_XAPP_MSG_HPP_

#include <iostream>
#include <rmr/rmr.h>
#include <rmr/RIC_message_types.h>
#include <mdclog/mdclog.h>

#include "a1_helper.hpp"
#include "e2ap_control.hpp"
#include "e2ap_control_response.hpp"
#include "e2ap_indication.hpp"
#include "subscription_delete_request.hpp"
#include "subscription_delete_response.hpp"
#include "subscription_helper.hpp"
#include "subscription_request.hpp"
#include "subscription_response.hpp"
#include "e2sm_subscription.hpp"
#include "subs_mgmt.hpp"
#include "e2sm_control.hpp"
#include "metrics.hpp"
#include "a1_mgmt.hpp"

#include "E2SM-RC-ControlMessage-Format1-Item.h"
#include "E2SM-RC-IndicationMessage-Format5-Item.h"
#include "RANParameter-ValueType-Choice-Structure.h"
#include "RANParameter-ValueType-Choice-List.h"
#include "RANParameter-STRUCTURE-Item.h"

#define MAX_RMR_RECV_SIZE 2<<15

class XappMsgHandler{

private:
	std::string xapp_id;
	SubscriptionHandler *_ref_sub_handler;
	A1Handler *_ref_a1_handler;
	shared_ptr<PrometheusMetrics> prometheusMetrics;

	bool processRanParameter17011(std::string &gnodeb_id, RANParameter_ValueType_Choice_Structure_t *p17011);
	bool processRanParameter21503(RANParameter_ValueType_Choice_Structure_t *p21503, std::vector<raw_ue_metrics_t> &ue_metrics);
	bool processRanParameter21528(RANParameter_ValueType_Choice_List_t *p21528, std::vector<raw_ue_metrics_t> &ue_metrics);
	bool process_NR_Cell_data(RANParameter_STRUCTURE_Item_t *nrcell, std::string &mcc, std::string &mnc,
								std::string &cellid_hex, long &rsrp, long &rsrq, long &sinr);

public:
	//constructor for xapp_id.
	 XappMsgHandler(std::string xid){xapp_id=xid; _ref_sub_handler=NULL;};
	 XappMsgHandler(std::string xid, SubscriptionHandler &subhandler, A1Handler &a1handler);

	 void operator() (rmr_mbuf_t *, bool*);

	 void register_handler();
	 bool encode_subscription_delete_request(unsigned char*, ssize_t* );

	 bool decode_subscription_response(unsigned char*, size_t );

	 bool a1_policy_handler(char *, int* , a1_policy_helper &);

	 void testfunction() {std::cout << "<<<<<<<<<<<<<<<<<<IN TEST FUNCTION<<<<<<<<<<<<<<<" << std::endl;}
};


#endif /* XAPP_MSG_XAPP_MSG_HPP_ */
