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
#include "RRC-State.h"


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
 * Stores de
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

			// if (mdclog_level_get() > MDCLOG_INFO) / FIXME uncomment this
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
				long msin;
				std::string mcc;
				std::string mnc;
				asn_INTEGER2long(&ueid->amf_UE_NGAP_ID, &msin);

				if (!decode_plmnid(&ueid->guami.pLMNIdentity, mcc, mnc)) {
					mdclog_write(MDCLOG_ERR, "Unable to decode PLMN_ID from %s", asn_DEF_E2SM_RC_IndicationMessage_Format2_Item.name);
					break;
				}
				std::string imsi = mcc + mnc + std::to_string(msin);
				std::string gnbid;
				long rrc_state_changed_to;
				long rsrp;
				long rsrq;
				long sinr;

				// ##### RAN Parameters #####
				for (int j = 0; j < item->ranP_List.list.count; j++) {
					E2SM_RC_IndicationMessage_Format2_RANParameter_Item *ranp_item = item->ranP_List.list.array[j];
					switch (ranp_item->ranParameter_ID) {
						case 202:
							rrc_state_changed_to = ranp_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueInt;
							break;

						case 12501:
							rsrp = ranp_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueInt;
							break;

						case 12502:
							rsrq = ranp_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueInt;
							break;

						case 12503:
							sinr = ranp_item->ranParameter_valueType.choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueInt;
							break;

						case 17011:
						{
							RANParameter_ValueType_Choice_Structure_t *struct17011 = ranp_item->ranParameter_valueType.choice.ranP_Choice_Structure;
							if (!processRanParameter17011(gnbid, struct17011)) {
								gnbid = "unknown";
								mdclog_write(MDCLOG_WARN, "Unable to decode RAN Parameter 17011 (Global gNB ID)");
							}

							break;
						}

						default:
							mdclog_write(MDCLOG_WARN, "RAN Parameter ID %lu not supported", ranp_item->ranParameter_ID);
					}
				}

				// Prometheus metrics
				ue_metrics_t &metrics = prometheusMetrics->get_ue_metrics_instance(imsi, gnbid);

				int state;
				if (rrc_state_changed_to == RRC_State_rrc_connected) {
					state = 1;
				} else if (rrc_state_changed_to == RRC_State_rrc_inactive) {
					state = 0;
				} else {
					mdclog_write(MDCLOG_WARN, "Unexpexted RRC State %ld", rrc_state_changed_to);
					state = -1;
				}
				metrics.rrc_state->Set(state);
				metrics.rsrp->Set(rsrp);
				metrics.rsrq->Set(rsrq);
				metrics.sinr->Set(sinr);

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


