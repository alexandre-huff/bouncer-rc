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

#include "msgs_proc.hpp"
#include "e2sm_indication.hpp"
#include "e2sm_helpers.hpp"
#include "metrics.hpp"
#include "utils.hpp"

#include "E2SM-RC-IndicationHeader-Format1.h"
#include "E2SM-RC-IndicationMessage-Format2.h"
#include "E2SM-RC-IndicationMessage-Format2-Item.h"
#include "E2SM-RC-IndicationMessage-Format2-RANParameter-Item.h"
#include "UEID-GNB.h"
#include "INTEGER.h"
#include "RANParameter-ID.h"
#include "RANParameter-ValueType.h"
#include "RANParameter-ValueType-Choice-ElementFalse.h"
#include "RANParameter-Value.h"
#include "RANParameter-STRUCTURE.h"
#include "RANParameter-STRUCTURE-Item.h"
#include "RANParameter-LIST.h"
#include "RRC-State.h"
#include "NR-CGI.h"


bool XappMsgHandler::encode_subscription_delete_request(unsigned char* buffer, ssize_t *buf_len){

	subscription_helper sub_helper;
	sub_helper.set_request({0, 0, 0}); // requirement of subscription manager ... ?
	sub_helper.set_function_id(0);

	subscription_delete e2ap_sub_req_del;

	  // generate the delete request pdu

	  bool res = e2ap_sub_req_del.encode_e2ap_subscription(&buffer[0], buf_len, sub_helper);
	  if(! res){
	    mdclog_write(MDCLOG_ERR, "%s, %d: Error encoding subscription delete request pdu. Reason = %s", __FILE__, __LINE__, e2ap_sub_req_del.get_error().c_str());
	    return false;
	  }

	return true;

}

bool XappMsgHandler::decode_subscription_response(unsigned char* data_buf, size_t data_size){

	subscription_helper subhelper;
	subscription_response subresponse;
	bool res = true;
	E2AP_PDU_t *e2pdu = 0;

	asn_dec_rval_t rval;

	ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, e2pdu);

	rval = asn_decode(0,ATS_ALIGNED_BASIC_PER, &asn_DEF_E2AP_PDU, (void**)&e2pdu, data_buf, data_size);
	switch(rval.code)
	{
		case RC_OK:
			   //Put in Subscription Response Object.
			   //asn_fprint(stdout, &asn_DEF_E2AP_PDU, e2pdu);
			   break;
		case RC_WMORE:
				mdclog_write(MDCLOG_ERR, "RC_WMORE");
				res = false;
				break;
		case RC_FAIL:
				mdclog_write(MDCLOG_ERR, "RC_FAIL");
				res = false;
				break;
		default:
				break;
	 }
	ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, e2pdu);
	return res;

}

bool XappMsgHandler::a1_policy_handler(char *message, int *message_len, a1_policy_helper &helper){
    if (!_ref_a1_handler->parse_a1_policy(message, helper)) {
        mdclog_write(MDCLOG_ERR, "Unable to process A1 policy request. Reason: %s", _ref_a1_handler->error_string.c_str());
        return false;
    }

    if (helper.policy_type_id == to_string(BOUNCER_POLICY_ID)) {
        if (helper.operation == "CREATE") {
            if (!_ref_a1_handler->parse_a1_payload(helper)) {
                mdclog_write(MDCLOG_ERR, "Unable to process A1 policy request. Reason: %s", _ref_a1_handler->error_string.c_str());
                return false;
            }

            mdclog_write(MDCLOG_INFO, "\n\nA1 policy request: handler_id=%s, operation=%s, policy_type_id=%s, policy_instance_id=%s, threshold=%d\n\n",
                         helper.handler_id.c_str(),
                         helper.operation.c_str(),
                         helper.policy_type_id.c_str(),
                         helper.policy_instance_id.c_str(),
                         helper.threshold);

            // TODO implement the logic for policy enforcement here


            helper.status = "OK";

        } else {
            mdclog_write(MDCLOG_WARN, "A1 operation \"%s\" not implemented", helper.operation.c_str());
            return false;
        }

        // Preparing response message to A1 Mediator
        if (!_ref_a1_handler->serialize_a1_response(message, message_len, helper)) {
            mdclog_write(MDCLOG_ERR, "Unable to serialize A1 reponse. Reason: %s", _ref_a1_handler->error_string.c_str());
            return false;
        }

    } else {
        mdclog_write(MDCLOG_ERR, "A1 policy type id %s not supported", helper.policy_type_id.c_str());
        return false;
    }

    return true;
}

/**
 * Process the RAN parameter 17011 (Global gNB ID)
*/
bool XappMsgHandler::processRanParameter17011(std::string &gnodeb_id, RANParameter_ValueType_Choice_Structure_t *p17011) {
	std::string gnbid;
	std::string mcc;
	std::string mnc;

	for (int i = 0; i < p17011->ranParameter_Structure->sequence_of_ranParameters->list.count; i++ ) {
		RANParameter_STRUCTURE_Item_t *p17011_item =
			p17011->ranParameter_Structure->sequence_of_ranParameters->list.array[i];
		switch (p17011_item->ranParameter_ID) {
			case 17012: // PLMN Identity
			{
				OCTET_STRING_t *plmnid = &p17011_item->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueOctS;
				if (!decode_plmnid(plmnid, mcc, mnc)) {
					mdclog_write(MDCLOG_ERR, "Unable to decode PLMN ID of RAN Parameter 17011 mcc=%s mnc=%s", mcc.c_str(), mnc.c_str());
					return false;
				}
				break;
			}

			case 17013:
			{
				RANParameter_STRUCTURE_t *p17013 = p17011_item->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure;
				for (int j = 0; j < p17013->sequence_of_ranParameters->list.count; j++) {
					RANParameter_STRUCTURE_Item_t *item = p17013->sequence_of_ranParameters->list.array[j];
					if (item->ranParameter_ID == 17014) { // gNB ID Structure
						RANParameter_STRUCTURE_t *p17014 = item->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure;
						if (p17014->sequence_of_ranParameters->list.count == 1) {
							RANParameter_STRUCTURE_Item_t *gnb = p17014->sequence_of_ranParameters->list.array[0];
							if (gnb->ranParameter_ID == 17015) { // gNB ID Element
								BIT_STRING_t *gnb_bs = &gnb->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueBitS;
								if (!translateBitString(gnbid, gnb_bs)) {
									mdclog_write(MDCLOG_ERR, "Unable to translate gNodeB ID to string");
									return false;
								}
							}

						} else {
							mdclog_write(MDCLOG_ERR, "Expected 1 element in RAN Parameter %lu", item->ranParameter_ID);
							return false;
						}

					} else {
						mdclog_write(MDCLOG_ERR, "RAN Parameter %lu not supported", item->ranParameter_ID);
						return false;
					}
				}

				break;
			}

			default:
				mdclog_write(MDCLOG_WARN, "RAN parameter %lu not supported", p17011_item->ranParameter_ID);
				return false;
		}
	}

	if (mnc.length() == 2) {
		gnodeb_id = mcc + mnc + "_" + gnbid;
	} else {
		gnodeb_id = mcc + mnc + gnbid;
	}

    return true;
}

bool XappMsgHandler::processRanParameter21503(RANParameter_ValueType_Choice_Structure_t *p21503, std::vector<raw_ue_metrics_t> &ue_metrics) {
	raw_ue_metrics_t metrics;

	RANParameter_STRUCTURE_Item_t *p21504 =
		get_ran_parameter_structure_item(p21503->ranParameter_Structure, 21504, RANParameter_ValueType_PR_ranP_Choice_Structure);
	if (p21504) {
		std::string mcc;
		std::string mnc;
		std::string cellid_hex;
		if (process_NR_Cell_data(p21504, mcc, mnc, cellid_hex, metrics.rsrp, metrics.rsrq, metrics.sinr)) {
			if (mnc.length() == 3) {
				metrics.nr_cgi = mcc + mnc + cellid_hex;
			} else {	// two digits
				metrics.nr_cgi = mcc + mnc + "_" + cellid_hex;
			}

			metrics.primary_cell = true;

			ue_metrics.push_back(std::move(metrics));

			return true;
		} else {
			mdclog_write(MDCLOG_ERR, "Unable to process NR Cell metrics in param tree: 21503, 21504");
			return false;
		}
	}

	mdclog_write(MDCLOG_ERR, "Unable to find the last RAN Param ID in param tree: 21503, 21504");

	return false;
}

bool XappMsgHandler::processRanParameter21528(RANParameter_ValueType_Choice_List_t *p21528, std::vector<raw_ue_metrics_t> &ue_metrics) {
	std::stringstream ss;

	std::vector<RANParameter_STRUCTURE_t *> list_items = get_ran_parameter_list_items(p21528->ranParameter_List);

	ss << "21528";
	for (auto cell_item : list_items) {
		ss << ", 21529";
		RANParameter_STRUCTURE_Item_t *p21529 = get_ran_parameter_structure_item(cell_item, 21529, RANParameter_ValueType_PR_ranP_Choice_Structure);
		if (!p21529) {
			break;
		}

		ss << ", 21530";
		RANParameter_STRUCTURE_Item_t *p21530 =
			get_ran_parameter_structure_item(p21529->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure,
											21530, RANParameter_ValueType_PR_ranP_Choice_Structure);
		if (!p21530) {
			break;
		}

		ss << ", 21531";
		RANParameter_STRUCTURE_Item_t *p21531 =
			get_ran_parameter_structure_item(p21530->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure,
											21531, RANParameter_ValueType_PR_ranP_Choice_Structure);
		if (!p21531) {
			break;
		}

		raw_ue_metrics_t metrics;
		std::string mcc;
		std::string mnc;
		std::string cellid_hex;
		if (process_NR_Cell_data(p21531, mcc, mnc, cellid_hex, metrics.rsrp, metrics.rsrq, metrics.sinr)) {
			if (mnc.length() == 3) {
				metrics.nr_cgi = mcc + mnc + cellid_hex;
			} else {	// two digits
				metrics.nr_cgi = mcc + mnc + "_" + cellid_hex;
			}

			metrics.primary_cell = false;

			ue_metrics.push_back(std::move(metrics));

			return true;
		} else {
			mdclog_write(MDCLOG_ERR, "Unable to process NR Cell metrics in param tree: %s", ss.str().c_str());
			return false;
		}

	}

	mdclog_write(MDCLOG_ERR, "Unable to find the last RAN Param ID in param tree: %s", ss.str().c_str());

	return false;
}

bool XappMsgHandler::process_NR_Cell_data(RANParameter_STRUCTURE_Item_t *nrcell, std::string &mcc, std::string &mnc,
											std::string &cellid_hex, long &rsrp, long &rsrq, long &sinr) {
	RANParameter_STRUCTURE_Item_t *p10001 =
			get_ran_parameter_structure_item(nrcell->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure,
											10001, RANParameter_ValueType_PR_ranP_Choice_ElementFalse);
	if (p10001) {
		OCTET_STRING_t *nrcgi_data = get_ran_parameter_value_data<OCTET_STRING_t>(
			p10001->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value, RANParameter_Value_PR_valueOctS);
		if (!nrcgi_data) {
			mdclog_write(MDCLOG_ERR, "Unable to get OCTET STRING data from RAN Param ID 10001");
			return false;
		}

		NR_CGI_t *nrcgi = NULL;
		asn_dec_rval_t rval = asn_decode(NULL, ATS_ALIGNED_BASIC_PER, &asn_DEF_NR_CGI, (void **)&nrcgi, nrcgi_data->buf, nrcgi_data->size);
		if (rval.code != RC_OK) {
			mdclog_write(MDCLOG_ERR, "Unable to decode NR_CGI from RAN Parameter ID 10001");
			return false;
		}

		if (!decode_NR_CGI(nrcgi, mcc, mnc, cellid_hex)) {
			mdclog_write(MDCLOG_ERR, "Unable to decode NR_CGI to MCC, MNC, and Cell Identity from RAN Parameter ID 10001");
			ASN_STRUCT_FREE(asn_DEF_NR_CGI, nrcgi);
			return false;
		}
		ASN_STRUCT_FREE(asn_DEF_NR_CGI, nrcgi);
	}

	RANParameter_STRUCTURE_Item_t *p10101 =
			get_ran_parameter_structure_item(nrcell->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure,
											10101, RANParameter_ValueType_PR_ranP_Choice_Structure);
	if (!p10101) {
		mdclog_write(MDCLOG_ERR, "Unable to get RAN Param ID 10101 from NR Cell");
		return false;
	}

	RANParameter_STRUCTURE_Item_t *p10102 =
			get_ran_parameter_structure_item(p10101->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure,
											10102, RANParameter_ValueType_PR_ranP_Choice_Structure);
	if (!p10102) {
		mdclog_write(MDCLOG_ERR, "Unable to get RAN Param ID 10102 from NR Cell");
		return false;
	}

	RANParameter_STRUCTURE_Item_t *p10106 =
			get_ran_parameter_structure_item(p10102->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure,
											10106, RANParameter_ValueType_PR_ranP_Choice_Structure);
	if (!p10106) {
		mdclog_write(MDCLOG_ERR, "Unable to get RAN Param ID 10106 from NR Cell");
		return false;
	}

	// RRC Signal Measurements (RSRP)
	RANParameter_STRUCTURE_Item_t *p12501 =
			get_ran_parameter_structure_item(p10106->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure,
											12501, RANParameter_ValueType_PR_ranP_Choice_ElementFalse);
	if (!p12501) {
		mdclog_write(MDCLOG_ERR, "Unable to get RAN Param ID 12501 from RRC Signal Measurements");
		return false;
	}
	long *rsrp_ptr = get_ran_parameter_value_data<long>(
		p12501->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value, RANParameter_Value_PR_valueInt);
	if (!rsrp_ptr) {
		mdclog_write(MDCLOG_ERR, "Unable to get RSRP data from RAN Param ID 12501");
		return false;
	}
	rsrp = *rsrp_ptr;

	// RRC Signal Measurements (RSRQ)
	RANParameter_STRUCTURE_Item_t *p12502 =
			get_ran_parameter_structure_item(p10106->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure,
											12502, RANParameter_ValueType_PR_ranP_Choice_ElementFalse);
	if (!p12502) {
		mdclog_write(MDCLOG_ERR, "Unable to get RAN Param ID 12502 from RRC Signal Measurements");
		return false;
	}
	long *rsrq_ptr = get_ran_parameter_value_data<long>(
		p12502->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value, RANParameter_Value_PR_valueInt);
	if (!rsrq_ptr) {
		mdclog_write(MDCLOG_ERR, "Unable to get RSRQ data from RAN Param ID 12502");
		return false;
	}
	rsrq = *rsrq_ptr;

	// RRC Signal Measurements (SINR)
	RANParameter_STRUCTURE_Item_t *p12503 =
			get_ran_parameter_structure_item(p10106->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure,
											12503, RANParameter_ValueType_PR_ranP_Choice_ElementFalse);
	if (!p12503) {
		mdclog_write(MDCLOG_ERR, "Unable to get RAN Param ID 12503 from RRC Signal Measurements");
		return false;
	}
	long *sinr_ptr = get_ran_parameter_value_data<long>(
		p12503->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value, RANParameter_Value_PR_valueInt);
	if (!sinr_ptr) {
		mdclog_write(MDCLOG_ERR, "Unable to get SINR data from RAN Param ID 12503");
		return false;
	}
	sinr = *sinr_ptr;

    return true;
}

XappMsgHandler::XappMsgHandler(std::string xid, SubscriptionHandler &subhandler, A1Handler &a1handler) {
	xapp_id=xid;
	_ref_sub_handler=&subhandler;
	_ref_a1_handler=&a1handler;
	prometheusMetrics = std::make_shared<PrometheusMetrics>();
}

// For processing received messages.XappMsgHandler should mention if resend is required or not.
void XappMsgHandler::operator()(rmr_mbuf_t *message, bool *resend)
{

	if (message->len > MAX_RMR_RECV_SIZE)
	{
		mdclog_write(MDCLOG_ERR, "Error : %s, %d, RMR message larger than %d. Ignoring ...", __FILE__, __LINE__, MAX_RMR_RECV_SIZE);
		return;
	}
	//a1_policy_helper helper;
	// bool res=false;
	E2AP_PDU_t* e2pdu = (E2AP_PDU_t*)calloc(1, sizeof(E2AP_PDU));
	// int num = 0;

	switch (message->mtype)
	{
		// need to fix the health check.
		case (RIC_HEALTH_CHECK_REQ):
			message->mtype = RIC_HEALTH_CHECK_RESP; // if we're here we are running and all is ok
			message->sub_id = -1;
			strncpy((char *)message->payload, "Bouncer OK\n", rmr_payload_size(message));
			*resend = true;
			break;

		case (RIC_SUB_RESP):
			mdclog_write(MDCLOG_INFO, "Received subscription message of type = %d", message->mtype);
			unsigned char *me_id;
			if ((me_id = (unsigned char *)malloc(sizeof(unsigned char) * RMR_MAX_MEID)) == NULL)
			{
				mdclog_write(MDCLOG_ERR, "Error :  %s, %d : malloc failed for me_id", __FILE__, __LINE__);
				me_id = rmr_get_meid(message, NULL);
			}
			else
			{
				rmr_get_meid(message, me_id);
			}
			if (me_id == NULL)
			{
				mdclog_write(MDCLOG_ERR, " Error :: %s, %d : rmr_get_meid failed me_id is NULL", __FILE__, __LINE__);
				break;
			}
			mdclog_write(MDCLOG_INFO, "RMR Received MEID: %s", me_id);
			if (_ref_sub_handler != NULL)
			{
				_ref_sub_handler->manage_subscription_response(message->mtype, reinterpret_cast<char const *>(me_id));
			}
			else
			{
				mdclog_write(MDCLOG_ERR, " Error :: %s, %d : Subscription handler not assigned in message processor !", __FILE__, __LINE__);
			}
			*resend = false;
			if (me_id != NULL)
			{
				mdclog_write(MDCLOG_INFO, "Free RMR Received MEID memory: %s(0x%p)", me_id, me_id);
				free(me_id);
			}
			break;

		case (RIC_SUB_DEL_RESP):
			mdclog_write(MDCLOG_INFO, "Received subscription delete message of type = %d", message->mtype);
			// unsigned char *me_id;
			if ((me_id = (unsigned char *)malloc(sizeof(unsigned char) * RMR_MAX_MEID)) == NULL)
			{
				mdclog_write(MDCLOG_ERR, "Error :  %s, %d : malloc failed for me_id", __FILE__, __LINE__);
				me_id = rmr_get_meid(message, NULL);
			}
			else

			{
				rmr_get_meid(message, me_id);
			}
			if (me_id == NULL)
			{
				mdclog_write(MDCLOG_ERR, " Error :: %s, %d : rmr_get_meid failed me_id is NULL", __FILE__, __LINE__);
				break;
			}
			mdclog_write(MDCLOG_INFO, "RMR Received MEID: %s", me_id);
			if (_ref_sub_handler != NULL)
			{
				_ref_sub_handler->manage_subscription_response(message->mtype, reinterpret_cast<char const *>(me_id));
			}
			else
			{
				mdclog_write(MDCLOG_ERR, " Error :: %s, %d : Subscription handler not assigned in message processor !", __FILE__, __LINE__);
			}
			*resend = false;
			if (me_id != NULL)
			{
				mdclog_write(MDCLOG_INFO, "Free RMR Received MEID memory: %s(0x%p)", me_id, me_id);
				free(me_id);
			}
			break;

		case (RIC_INDICATION):
		{
			mdclog_write(MDCLOG_DEBUG, "Decoding indication for msg = %d", message->mtype);

			*resend = false;

			ASN_STRUCT_RESET(asn_DEF_E2AP_PDU, e2pdu);
			asn_transfer_syntax syntax;
			syntax = ATS_ALIGNED_BASIC_PER;

			mdclog_write(MDCLOG_DEBUG, "Data_size = %d", message->len);

			auto rval = asn_decode(nullptr, syntax, &asn_DEF_E2AP_PDU, (void **)&e2pdu, message->payload, message->len);

			if (rval.code == RC_OK) {
				mdclog_write(MDCLOG_DEBUG, "rval.code = %d ", rval.code);

			} else {
				mdclog_write(MDCLOG_ERR, " rval.code = %d ", rval.code);
				break;
			}

			if (mdclog_level_get() > MDCLOG_INFO)
				asn_fprint(stderr, &asn_DEF_E2AP_PDU, e2pdu);

			ric_indication e2ap_indication;
			ric_indication_helper ind_helper;
			string error_msg;
			e2ap_indication.get_fields(e2pdu->choice.initiatingMessage, ind_helper);

			e2sm_indication indication;
			E2SM_RC_IndicationHeader_t *header = indication.decode_e2sm_rc_indication_header(&ind_helper.indication_header);
			if (!header) {
				mdclog_write(MDCLOG_ERR, "Unable to decode E2SM_RC_IndicationHeader");
				break;
			}
			E2SM_RC_IndicationMessage_t *msg = indication.decode_e2sm_rc_indication_message(&ind_helper.indication_msg);
			if (!msg) {
				mdclog_write(MDCLOG_ERR, "Unable to decode E2SM_RC_IndicationMessage");
				break;
			}

			if (header->ric_indicationHeader_formats.present != E2SM_RC_IndicationHeader__ric_indicationHeader_formats_PR_indicationHeader_Format1) {
				mdclog_write(MDCLOG_ERR, "Only E2SM RC Indication Header Format 1 is supported");
				break;
			}
			if (msg->ric_indicationMessage_formats.present != E2SM_RC_IndicationMessage__ric_indicationMessage_formats_PR_indicationMessage_Format2) {
				mdclog_write(MDCLOG_ERR, "Only E2SM RC Indication Message Format 2 is supported");
				break;
			}

			E2SM_RC_IndicationHeader_Format1_t *fmt1 = header->ric_indicationHeader_formats.choice.indicationHeader_Format1;
			if (fmt1)
				mdclog_write(MDCLOG_DEBUG, "Event Trigger Condition ID %ld", *fmt1->ric_eventTriggerCondition_ID);

			E2SM_RC_IndicationMessage_Format2_t *fmt2 = msg->ric_indicationMessage_formats.choice.indicationMessage_Format2;
			for (int i = 0; i < fmt2->ueParameter_List.list.count; i++) {
				E2SM_RC_IndicationMessage_Format2_Item *item = fmt2->ueParameter_List.list.array[i];

				// ###### IMSI ######
				UEID_GNB_t *ueid = item->ueID.choice.gNB_UEID;
				long msin_number;
				std::string mcc;
				std::string mnc;
				asn_INTEGER2long(&ueid->amf_UE_NGAP_ID, &msin_number);

				if (!decode_plmnid(&ueid->guami.pLMNIdentity, mcc, mnc)) {
					mdclog_write(MDCLOG_ERR, "Unable to decode PLMN_ID from %s", asn_DEF_E2SM_RC_IndicationMessage_Format2_Item.name);
					break;
				}
				std::string msin = std::to_string(msin_number);

				// we need to pad extra characters between mnc and msin to have a total of 15 digits in the final imsi
				int padding = 15 - 3 - mnc.length() - msin.length(); // MCC always has 3 digits
				msin = msin.insert(0, padding, '0');  // we pad msin with zeroes to reach 15 digits in the final imsi

				std::string imsi = mcc + mnc + msin;
				long rrc_state_changed_to = -1;	// set to ERROR to force that rrc state will be CONNECTED or DISCONNECTED
				std::vector<raw_ue_metrics_t> raw_metrics;
				bool report_ok = true;

				// ##### RAN Parameters #####
				for (int j = 0; j < item->ranP_List.list.count; j++) {
					E2SM_RC_IndicationMessage_Format2_RANParameter_Item *ranp_item = item->ranP_List.list.array[j];
					switch (ranp_item->ranParameter_ID) {
						case 202:
							rrc_state_changed_to = ranp_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueInt;
							break;

						case 21503:
							if (!processRanParameter21503(ranp_item->ranParameter_valueType.choice.ranP_Choice_Structure, raw_metrics)) {
								mdclog_write(MDCLOG_ERR, "Unable to process CHOICE Primary Cell of MCG for UE %s", imsi.c_str());
								report_ok = false;
							}
							break;

						case 21528:
							if (!processRanParameter21528(ranp_item->ranParameter_valueType.choice.ranP_Choice_List, raw_metrics)) {
								mdclog_write(MDCLOG_ERR, "Unable to process CHOICE Primary Cell of MCG for UE %s", imsi.c_str());
								report_ok = false;
							}
							break;

						// case 17011:	// actually, the correct parameter tree should start with 21501 to process gNB ID data from UE Context
						// {
						// 	RANParameter_ValueType_Choice_Structure_t *struct17011 = ranp_item->ranParameter_valueType.choice.ranP_Choice_Structure;
						// 	if (!processRanParameter17011(gnbid, struct17011)) {
						// 		gnbid = "unknown";
						// 		mdclog_write(MDCLOG_WARN, "Unable to decode RAN Parameter 17011 (Global gNB ID)");
						// 	}

						// 	break;
						// }

						default:
							mdclog_write(MDCLOG_WARN, "RAN Parameter ID %lu not supported", ranp_item->ranParameter_ID);
							report_ok = false;
					}
				}

				if (report_ok) {

					if (rrc_state_changed_to == RRC_State_rrc_connected) {	// CONNECTED

						for (auto &data : raw_metrics) {
							// Prometheus metrics
							prometheus_ue_metrics_t &metrics = prometheusMetrics->get_ue_metrics_instance(imsi, data.nr_cgi);
							if (data.primary_cell) {
								metrics.rrc_state->Set(1);
							} else {
								metrics.rrc_state->Set(0);
							}
							metrics.rsrp->Set(data.rsrp);
							metrics.rsrq->Set(data.rsrq);
							metrics.sinr->Set(data.sinr);
						}

					} else if (rrc_state_changed_to == RRC_State_rrc_inactive) {	// DISCONNECTED
						prometheusMetrics->delete_ue_metrics_instance(imsi);

					} else {
						mdclog_write(MDCLOG_ERR, "Unexpected RRC State %ld, nothing done", rrc_state_changed_to);
					}

				}

			}

			ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationHeader, header);
			ASN_STRUCT_FREE(asn_DEF_E2SM_RC_IndicationMessage, msg);

			break;
		}

		case A1_POLICY_REQ:
		{
			mdclog_write(MDCLOG_INFO, "Received A1_POLICY_REQ");
			a1_policy_helper helper;
			helper.handler_id = xapp_id;

			bool res = a1_policy_handler((char*)message->payload, &message->len, helper);
			if(res) {
				message->mtype = A1_POLICY_RESP;        // if we're here we are running and all is ok
				message->sub_id = -1;
				*resend = true;
			}
			break;
		}

		default:
			mdclog_write(MDCLOG_ERR, "Error :: Unknown message type %d received from RMR", message->mtype);
			*resend = false;
	}

	ASN_STRUCT_FREE(asn_DEF_E2AP_PDU, e2pdu);
}


