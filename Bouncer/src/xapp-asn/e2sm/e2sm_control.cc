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


/* Classes to handle E2 service model based on e2sm-Bouncer-v001.asn */
#include "e2sm_control.hpp"
#include "E2SM-RC-ControlHeader-Format1.h"
#include "E2SM-RC-ControlMessage-Format1.h"
#include "E2SM-RC-ControlMessage-Format1-Item.h"
#include "RANParameter-ValueType-Choice-Structure.h"
#include "RANParameter-ValueType-Choice-ElementFalse.h"
#include "RANParameter-STRUCTURE.h"
#include "RANParameter-STRUCTURE-Item.h"
#include "RANParameter-Value.h"
#include "NR-CGI.h"
#include "UEID-GNB.h"

 //initialize
 e2sm_control::e2sm_control(void){

	memset(&head_fmt1, 0, sizeof(E2SM_Bouncer_ControlHeader_Format1_t));
	memset(&msg_fmt1, 0, sizeof(E2SM_Bouncer_ControlMessage_Format1_t));

  control_head = 0;
  control_head = ( E2SM_Bouncer_ControlHeader_t *)calloc(1, sizeof( E2SM_Bouncer_ControlHeader_t));
  assert(control_head != 0);

  control_msg = 0;
  control_msg = (E2SM_Bouncer_ControlMessage_t*)calloc(1, sizeof(E2SM_Bouncer_ControlMessage_t));
  assert(control_msg !=0);

  rc_control_header = (E2SM_RC_ControlHeader_t *) calloc(1, sizeof(E2SM_RC_ControlHeader_t));
  assert(rc_control_header != NULL);
  rc_control_msg = (E2SM_RC_ControlMessage_t *) calloc(1, sizeof(E2SM_RC_ControlMessage_t));
  assert(rc_control_msg != NULL);
  rc_call_proc_id = (E2SM_RC_CallProcessID_t *) calloc(1, sizeof(E2SM_RC_CallProcessID_t));
  assert(rc_call_proc_id != NULL);

  errbuf_len = 128;
  };

 e2sm_control::~e2sm_control(void){

  mdclog_write(MDCLOG_DEBUG, "Freeing event trigger object memory");

  control_head->choice.controlHeader_Format1 = 0;

  control_msg->choice.controlMessage_Format1 = 0;

  ASN_STRUCT_FREE(asn_DEF_E2SM_Bouncer_ControlHeader, control_head);
  ASN_STRUCT_FREE(asn_DEF_E2SM_Bouncer_ControlMessage, control_msg);

  ASN_STRUCT_FREE(asn_DEF_E2SM_RC_ControlHeader, rc_control_header);
  ASN_STRUCT_FREE(asn_DEF_E2SM_RC_ControlMessage, rc_control_msg);
  ASN_STRUCT_FREE(asn_DEF_E2SM_RC_CallProcessID, rc_call_proc_id);

};

bool e2sm_control::encode_control_header(unsigned char *buf, size_t *size, e2sm_control_helper &helper){

  ASN_STRUCT_RESET(asn_DEF_E2SM_Bouncer_ControlHeader, control_head);

  bool res;
  res = set_fields(control_head, helper);
  if (!res){

    return false;
  }

  int ret_constr = asn_check_constraints(&asn_DEF_E2SM_Bouncer_ControlHeader, control_head, errbuf, &errbuf_len);
  if(ret_constr){
    error_string.assign(&errbuf[0], errbuf_len);
    return false;
  }

  if (mdclog_level_get() > MDCLOG_INFO) {
    xer_fprint(stdout, &asn_DEF_E2SM_Bouncer_ControlHeader, control_head);
  }

  asn_enc_rval_t retval = asn_encode_to_buffer(0, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2SM_Bouncer_ControlHeader, control_head, buf, *size);

  if(retval.encoded == -1){
    error_string.assign(strerror(errno));
    return false;
  }
  else if (retval.encoded > *size){
    std::stringstream ss;
    ss  <<"Error encoding event trigger definition. Reason =  encoded pdu size " << retval.encoded << " exceeds buffer size " << *size << std::endl;
    error_string = ss.str();
    return false;
  }
  else{
    *size = retval.encoded;
  }

  return true;
}

bool e2sm_control::encode_control_message(unsigned char *buf, size_t *size, e2sm_control_helper &helper){

  bool res;
  res = set_fields(control_msg, helper);
  if (!res){
    return false;
  }


  int ret_constr = asn_check_constraints(&asn_DEF_E2SM_Bouncer_ControlMessage, control_msg, errbuf, &errbuf_len);
  if(ret_constr){
    error_string.assign(&errbuf[0], errbuf_len);
    return false;
  }

  if (mdclog_level_get() > MDCLOG_INFO) {
    xer_fprint(stdout, &asn_DEF_E2SM_Bouncer_ControlMessage, control_msg);
  }

  asn_enc_rval_t retval = asn_encode_to_buffer(0, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2SM_Bouncer_ControlMessage, control_msg, buf, *size);

  if(retval.encoded == -1){
    error_string.assign(strerror(errno));
    return false;
  }
  else if (retval.encoded > *size){
    std::stringstream ss;
    ss  <<"Error encoding action definition. Reason =  encoded pdu size " << retval.encoded << " exceeds buffer size " << *size << std::endl;
    error_string = ss.str();
    return false;
  }
  else{
    *size = retval.encoded;
  }

  return true;
}

bool e2sm_control::set_fields(E2SM_Bouncer_ControlHeader_t * ref_control_head, e2sm_control_helper & helper){

 if(ref_control_head == 0){
    error_string = "Invalid reference for Event Trigger Definition set fields";
    return false;
  }

  ref_control_head->present = E2SM_Bouncer_ControlHeader_PR_controlHeader_Format1;

  head_fmt1.controlHeaderParam = helper.header;

  ref_control_head->choice.controlHeader_Format1 = &head_fmt1;

  return true;
};

bool e2sm_control::set_fields(E2SM_Bouncer_ControlMessage_t * ref_control_msg, e2sm_control_helper & helper){

 if(ref_control_msg == 0){
    error_string = "Invalid reference for Event Action Definition set fields";
    return false;
  }
  ref_control_msg->present = E2SM_Bouncer_ControlMessage_PR_controlMessage_Format1;

  msg_fmt1.controlMsgParam.buf = helper.message;
  msg_fmt1.controlMsgParam.size = helper.message_len;


  ref_control_msg->choice.controlMessage_Format1 = &msg_fmt1;


  return true;
};

bool e2sm_control::get_fields(E2SM_Bouncer_ControlHeader_t * ref_indictaion_header, e2sm_control_helper & helper){

	if (ref_indictaion_header == 0){
	    error_string = "Invalid reference for Control Header get fields";
	    return false;
	  }

	helper.header = ref_indictaion_header->choice.controlHeader_Format1->controlHeaderParam;
	return true;
}

bool e2sm_control::get_fields(E2SM_Bouncer_ControlMessage_t * ref_control_message, e2sm_control_helper & helper){

	  if (ref_control_message == 0){
	  	    error_string = "Invalid reference for Control Message get fields";
	  	    return false;
	  	  }
	  helper.message = ref_control_message->choice.controlMessage_Format1->controlMsgParam.buf;
	  helper.message_len = ref_control_message->choice.controlMessage_Format1->controlMsgParam.size;

	  return true;
  }

bool e2sm_control::encode_rc_control_header(unsigned char *buf, size_t *size, UEID_t *ueid) {
  rc_control_header = (E2SM_RC_ControlHeader_t *) calloc(1, sizeof(E2SM_RC_ControlHeader_t));

  bool res = set_fields(rc_control_header, ueid);
  if (!res){
    return false;
  }

  int ret_constr = asn_check_constraints(&asn_DEF_E2SM_RC_ControlHeader, rc_control_header, errbuf, &errbuf_len);
  if(ret_constr){
    error_string.assign(&errbuf[0], errbuf_len);
    return false;
  }

  if (mdclog_level_get() > MDCLOG_INFO) {
    xer_fprint(stdout, &asn_DEF_E2SM_RC_ControlHeader, rc_control_header);
  }

  asn_enc_rval_t retval = asn_encode_to_buffer(0, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2SM_RC_ControlHeader, rc_control_header, buf, *size);

  if(retval.encoded == -1){
    error_string.assign(strerror(errno));
    return false;
  }
  else if (retval.encoded > *size){
    std::stringstream ss;
    ss  <<"Error encoding E2SM_RC_ControlHeader. Reason =  encoded pdu size " << retval.encoded << " exceeds buffer size " << *size << std::endl;
    error_string = ss.str();
    return false;
  }
  else{
    *size = retval.encoded;
  }

  return true;
}

bool e2sm_control::encode_rc_control_message(unsigned char *buf, size_t *size) {
  rc_control_msg = (E2SM_RC_ControlMessage_t *) calloc(1, sizeof(E2SM_RC_ControlMessage_t));

  bool res;
  res = set_fields(rc_control_msg);
  if (!res){
    return false;
  }

  // int ret_constr = asn_check_constraints(&asn_DEF_E2SM_RC_ControlMessage, rc_control_msg, errbuf, &errbuf_len);
  // if(ret_constr){
  //   error_string.assign(&errbuf[0], errbuf_len);
  //   return false;
  // }

  asn_enc_rval_t retval = asn_encode_to_buffer(0, ATS_ALIGNED_BASIC_PER, &asn_DEF_E2SM_RC_ControlMessage, rc_control_msg, buf, *size);

  if(mdclog_level_get() > MDCLOG_INFO) {
    xer_fprint(stdout, &asn_DEF_E2SM_RC_ControlMessage, rc_control_msg);
  }

  if(retval.encoded == -1){
    error_string.assign(strerror(errno));
    return false;
  }
  else if (retval.encoded > *size){
    std::stringstream ss;
    ss  <<"Error encoding E2SM_RC_ControlMessage. Reason =  encoded pdu size " << retval.encoded << " exceeds buffer size " << *size << std::endl;
    error_string = ss.str();
    return false;
  }
  else{
    *size = retval.encoded;
  }

  return true;
}

bool e2sm_control::set_fields(E2SM_RC_ControlHeader_t *control_header, UEID_t *ueid) {
  if(control_header == 0){
    error_string = "Invalid reference for E2SM_RC_ControlHeader set fields";
    return false;
  }

  ASN_STRUCT_RESET(asn_DEF_E2SM_RC_ControlHeader, control_header);

  E2SM_RC_ControlHeader_Format1_t *ctrlhead_fmt1 = generate_e2sm_rc_control_header_format1(ueid);
  if(ctrlhead_fmt1 == NULL) {
    return false; // error string is set on called function
  }

  control_header->ric_controlHeader_formats.present = E2SM_RC_ControlHeader__ric_controlHeader_formats_PR_controlHeader_Format1;
  control_header->ric_controlHeader_formats.choice.controlHeader_Format1 = ctrlhead_fmt1;

  return true;
}

bool e2sm_control::set_fields(E2SM_RC_ControlMessage_t *control_msg) {
  if(control_msg == 0){
    error_string = "Invalid reference for E2SM_RC_ControlMessage set fields";
    return false;
  }

  ASN_STRUCT_RESET(asn_DEF_E2SM_RC_ControlMessage, rc_control_msg);

  E2SM_RC_ControlMessage_Format1_t *ctrlmsg_fmt1 = generate_e2sm_rc_control_msg_format1();
  if(ctrlmsg_fmt1 == NULL) {
    return false; // error string is set on called function
  }

  control_msg->ric_controlMessage_formats.present = E2SM_RC_ControlMessage__ric_controlMessage_formats_PR_controlMessage_Format1;
  control_msg->ric_controlMessage_formats.choice.controlMessage_Format1 = ctrlmsg_fmt1;

  return true;
}

E2SM_RC_ControlHeader_Format1_t *e2sm_control::generate_e2sm_rc_control_header_format1(UEID_t *ueid) {
  // TODO we should populate this using the corresponding values from indication request
  E2SM_RC_ControlHeader_Format1_t *ctrlhead_fmt1 = (E2SM_RC_ControlHeader_Format1_t *) calloc(1, sizeof(E2SM_RC_ControlHeader_Format1_t));
  if(ctrlhead_fmt1 == NULL) {
    error_string = "unable to alloc E2SM_RC_ControlHeader_Format1 for generating control header";
    return NULL;
  }
  generate_e2sm_rc_ueid(&ctrlhead_fmt1->ueID);  // TODO this should be the corresponding UE from insert message
  ctrlhead_fmt1->ric_Style_Type = 4; // Radio access control
  ctrlhead_fmt1->ric_ControlAction_ID = 1; // UE Admission Control
  ctrlhead_fmt1->ric_ControlDecision = (long *) calloc(1, sizeof(long));
  if(ctrlhead_fmt1->ric_ControlDecision == NULL) {
    error_string = "unable to alloc Ric Control Decision for set fields";
    ASN_STRUCT_FREE(asn_DEF_E2SM_RC_ControlHeader_Format1, ctrlhead_fmt1);
    return NULL;
  }
  *ctrlhead_fmt1->ric_ControlDecision = E2SM_RC_ControlHeader_Format1__ric_ControlDecision_accept;  // for now we accept all insert requests

  return ctrlhead_fmt1;
}

bool e2sm_control::generate_e2sm_rc_ueid(UEID_t *ueid) {
  UEID_GNB_t *ueid_gnb = (UEID_GNB_t *) calloc(1, sizeof(UEID_GNB_t));

  // an integer between 0..2^40-1, but we only alloc 1 byte to store values between 0..255
  ueid_gnb->amf_UE_NGAP_ID.buf = (uint8_t *) calloc(1, sizeof(uint8_t));
  ueid_gnb->amf_UE_NGAP_ID.buf[0] = (uint8_t) 1;
  ueid_gnb->amf_UE_NGAP_ID.size = sizeof(uint8_t);

  uint8_t *plmnid = (uint8_t *) "747";   // TODO it seems that we should get this from e2sim base class
  OCTET_STRING_fromBuf(&ueid_gnb->guami.pLMNIdentity, (char *) plmnid, strlen((char *) plmnid));

  ueid_gnb->guami.aMFRegionID.buf = (uint8_t *) calloc(1, sizeof(uint8_t)); // (8 bits)
  ueid_gnb->guami.aMFRegionID.buf[0] = (uint8_t) 128; // this is a dummy value
  ueid_gnb->guami.aMFRegionID.size = 1;
  ueid_gnb->guami.aMFRegionID.bits_unused = 0;

  ueid_gnb->guami.aMFSetID.buf = (uint8_t *) calloc(2, sizeof(uint8_t)); // (10 bits)
  uint16_t v = (uint16_t) 4; // this is a dummy vale (uint16_t is required to have room for 10 bits)
  v = v << 6; // we are only interested in 10 bits, so rotate them to the correct place
  ueid_gnb->guami.aMFSetID.buf[0] = (v >> 8); // only interested in the most significant bits (& 0x00ff only required for signed)
  ueid_gnb->guami.aMFSetID.buf[1] = v & 0x00ff; // we are only interested in the least significant bits
  ueid_gnb->guami.aMFSetID.size = 2;
  ueid_gnb->guami.aMFSetID.bits_unused = 6;

  ueid_gnb->guami.aMFPointer.buf = (uint8_t *) calloc(1, sizeof(uint8_t)); // (6 bits)
  ueid_gnb->guami.aMFPointer.buf[0] = (uint8_t) 1 << 2; // this is a dummy value
  ueid_gnb->guami.aMFPointer.size = 1;
  ueid_gnb->guami.aMFPointer.bits_unused = 2;

  ueid->choice.gNB_UEID = ueid_gnb;
  ueid->present = UEID_PR_gNB_UEID;
}

E2SM_RC_ControlMessage_Format1_t *e2sm_control::generate_e2sm_rc_control_msg_format1() {
  E2SM_RC_ControlMessage_Format1_t *ctrlmsg_fmt1 = (E2SM_RC_ControlMessage_Format1_t *) calloc(1, sizeof(E2SM_RC_ControlMessage_Format1_t));
  if(ctrlmsg_fmt1 == NULL) {
    error_string = "unable to alloc E2SM_RC_ControlMessage_Format1 for generating control message";
    return NULL;
  }

  // TODO need to check alloc errors
  E2SM_RC_ControlMessage_Format1_Item_t *format_item = (E2SM_RC_ControlMessage_Format1_Item_t *) calloc(1, sizeof(E2SM_RC_ControlMessage_Format1_Item_t));
  format_item->ranParameter_ID = 1; // Primary Cell ID as in E2SM-RC v01.02 section 8.4.5.1
  format_item->ranParameter_valueType.present = RANParameter_ValueType_PR_ranP_Choice_Structure;
  format_item->ranParameter_valueType.choice.ranP_Choice_Structure =
      (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));
  ASN_SEQUENCE_ADD(&ctrlmsg_fmt1->ranP_List.list, format_item);

  RANParameter_STRUCTURE_t *ranp_struct_item1 =
      (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
  format_item->ranParameter_valueType.choice.ranP_Choice_Structure->ranParameter_Structure = ranp_struct_item1;

  ranp_struct_item1->sequence_of_ranParameters = (struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *)
                                  calloc(1, sizeof(struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

  RANParameter_STRUCTURE_Item_t *ranp_struct_item2 = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
  ASN_SEQUENCE_ADD(&ranp_struct_item1->sequence_of_ranParameters->list, ranp_struct_item2);

  ranp_struct_item2->ranParameter_ID = 2; // CHOICE Primary Cell as in E2SM-RC v01.02 section 8.4.5.1
  ranp_struct_item2->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
  ranp_struct_item2->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_Structure;
  ranp_struct_item2->ranParameter_valueType->choice.ranP_Choice_Structure =
      (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));

  RANParameter_STRUCTURE_t *ranp_struct2 = (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
  ranp_struct_item2->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure = ranp_struct2;

  ranp_struct2->sequence_of_ranParameters = (struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *)
                              calloc(1, sizeof(struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

  RANParameter_STRUCTURE_Item *ranp_struct_item3 =
      (RANParameter_STRUCTURE_Item *) calloc(1, sizeof(RANParameter_STRUCTURE_Item));
  ASN_SEQUENCE_ADD(&ranp_struct2->sequence_of_ranParameters->list, ranp_struct_item3);

  ranp_struct_item3->ranParameter_ID = 3; // NR Cell as in E2SM-RC v01.02 section 8.4.5.1
  ranp_struct_item3->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
  ranp_struct_item3->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_Structure;
  ranp_struct_item3->ranParameter_valueType->choice.ranP_Choice_Structure =
      (RANParameter_ValueType_Choice_Structure_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_Structure_t));

  RANParameter_STRUCTURE_t *ranp_struct3 = (RANParameter_STRUCTURE_t *) calloc(1, sizeof(RANParameter_STRUCTURE_t));
  ranp_struct_item3->ranParameter_valueType->choice.ranP_Choice_Structure->ranParameter_Structure = ranp_struct3;

  ranp_struct3->sequence_of_ranParameters =
      (struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters *) calloc(1, sizeof(struct RANParameter_STRUCTURE::RANParameter_STRUCTURE__sequence_of_ranParameters));

  RANParameter_STRUCTURE_Item_t *ranp_struct_item4 = (RANParameter_STRUCTURE_Item_t *) calloc(1, sizeof(RANParameter_STRUCTURE_Item_t));
  ASN_SEQUENCE_ADD(&ranp_struct3->sequence_of_ranParameters->list, ranp_struct_item4);

  ranp_struct_item4->ranParameter_ID = 4; // NR CGI as in E2SM-RC v01.02 section 8.4.5.1
  ranp_struct_item4->ranParameter_valueType = (RANParameter_ValueType_t *) calloc(1, sizeof(RANParameter_ValueType_t));
  ranp_struct_item4->ranParameter_valueType->choice.ranP_Choice_ElementFalse =
      (RANParameter_ValueType_Choice_ElementFalse_t *) calloc(1, sizeof(RANParameter_ValueType_Choice_ElementFalse_t));

  ranp_struct_item4->ranParameter_valueType->present = RANParameter_ValueType_PR_ranP_Choice_ElementFalse;

  ranp_struct_item4->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value =
      (RANParameter_Value_t *) calloc(1, sizeof(RANParameter_Value_t));

  ranp_struct_item4->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->present = RANParameter_Value_PR_valueOctS;
  OCTET_STRING_t *nr_cgi = &ranp_struct_item4->ranParameter_valueType->choice.ranP_Choice_ElementFalse->ranParameter_value->choice.valueOctS;
  // we should get this information from indication request
  *nr_cgi = *(generate_and_encode_nr_cgi("747", 89));  // for now we use dummy values
  if(!nr_cgi) {
    ASN_STRUCT_FREE(asn_DEF_E2SM_RC_ControlMessage_Format1, ctrlmsg_fmt1);
    return NULL;  // error string is set on called function
  }

  return ctrlmsg_fmt1;
}

OCTET_STRING_t *e2sm_control::generate_and_encode_nr_cgi(const char *plmnid, unsigned long cell_id) {
  if (!plmnid) {
    error_string = "Invalid reference for plmnid in generate and encode NR CGI";
    return NULL;
  }
  if (strlen(plmnid) > 3) {
    error_string = "Invalid length of plmnid (max length is 3)";
    return NULL;
  }

  NR_CGI_t *nr_cgi = (NR_CGI_t *) calloc(1, sizeof(NR_CGI_t));
  if (nr_cgi == NULL) {
    error_string = "unable to alloc NR CGI in generate and enconde NR CGI";
    return NULL;
  }

  OCTET_STRING_fromBuf(&nr_cgi->pLMNIdentity, plmnid, strlen((char *) plmnid));
  BIT_STRING_t *nr_cell_id = (BIT_STRING_t*) calloc(1, sizeof(BIT_STRING_t));
  if (nr_cell_id == NULL) {
    error_string = "unable to alloc NR Cell ID bit string in generate and enconde NR CGI";
    ASN_STRUCT_FREE(asn_DEF_NR_CGI, nr_cgi);
    return NULL;
  }
  if(nr_cell_id) {
      nr_cell_id->buf = (uint8_t*)calloc(1,5);
      if(nr_cell_id->buf) {
          nr_cell_id->size = 5;
          nr_cell_id->buf[0] = ((cell_id & 0X0FF0000000) >> 28);
          nr_cell_id->buf[1] = ((cell_id & 0X000FF00000) >> 20);
          nr_cell_id->buf[2] = ((cell_id & 0X00000FF000) >> 12);
          nr_cell_id->buf[3] = ((cell_id & 0X0000000FF0) >> 4);
          nr_cell_id->buf[4] = (cell_id & 0X000000000F) << 4;
          nr_cell_id->bits_unused = 4;

          nr_cgi->nRCellIdentity = *nr_cell_id;
      }
  }

  // fprintf(stderr, "INFO %s:%d - about to check constraints of NR_CGI\n", __FILE__, __LINE__);
  int ret = asn_check_constraints(&asn_DEF_NR_CGI, nr_cgi, errbuf, &errbuf_len);
  if(ret){
    error_string.assign(&errbuf[0], errbuf_len);
    ASN_STRUCT_FREE(asn_DEF_NR_CGI, nr_cgi);
    ASN_STRUCT_FREE(asn_DEF_BIT_STRING, nr_cell_id);
    return NULL;
  }
  // fprintf(stderr, "NR_CGI set up\n");

  uint8_t nr_cgi_buffer[8192] = {0, };
  size_t nr_cgi_buffer_size = 8192;

  asn_enc_rval_t retval = asn_encode_to_buffer(NULL, ATS_ALIGNED_BASIC_PER,
                          &asn_DEF_NR_CGI, nr_cgi, nr_cgi_buffer, nr_cgi_buffer_size);

  if(retval.encoded == -1){
    error_string.assign(strerror(errno));
    ASN_STRUCT_FREE(asn_DEF_NR_CGI, nr_cgi);
    ASN_STRUCT_FREE(asn_DEF_BIT_STRING, nr_cell_id);
    return NULL;
  }
  else if (retval.encoded > nr_cgi_buffer_size){
    std::stringstream ss;
    ss  <<"Error encoding E2SM_RC_ControlHeader. Reason =  encoded pdu size " << retval.encoded << " exceeds buffer size " << nr_cgi_buffer_size << std::endl;
    error_string = ss.str();
    ASN_STRUCT_FREE(asn_DEF_NR_CGI, nr_cgi);
    ASN_STRUCT_FREE(asn_DEF_BIT_STRING, nr_cell_id);
    return NULL;
  }

  OCTET_STRING_t *ostr = OCTET_STRING_new_fromBuf(&asn_DEF_NR_CGI, (char *) nr_cgi_buffer, retval.encoded);
  if (!ostr) {
    error_string = "unable to encode OCTET_STRING from buffer for NR CGI";
    ASN_STRUCT_FREE(asn_DEF_NR_CGI, nr_cgi);
    ASN_STRUCT_FREE(asn_DEF_BIT_STRING, nr_cell_id);
    return NULL;
  }

  ASN_STRUCT_FREE(asn_DEF_NR_CGI, nr_cgi);

  return ostr;
}
