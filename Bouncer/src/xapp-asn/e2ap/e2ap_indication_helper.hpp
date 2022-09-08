/*
==================================================================================

        Copyright (c) 2019-2020 AT&T Intellectual Property.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
==================================================================================
*/
/*
 * ric_indication.h
 *
 *  Created on: Jul 11, 2019
 *      Author: sjana, Ashwin Sridharan
 */


#ifndef E2AP_INDICATION_HELPER_
#define E2AP_INDICATION_HELPER_

#include "UEID.h"
#include "E2SM-RC-IndicationMessage.h"
#include "E2SM-RC-IndicationMessage-Format5.h"
#include "E2SM-RC-IndicationHeader.h"
#include "E2SM-RC-IndicationHeader-Format2.h"

struct ric_indication_helper{

  ric_indication_helper() {
    ASN_STRUCT_RESET(asn_DEF_RICrequestID, &request_id);
    func_id = 0;
    action_id = 0;
    indication_type = 0;
    indication_sn = 0;
    ASN_STRUCT_RESET(asn_DEF_RICcallProcessID, &call_process_id);
    ASN_STRUCT_RESET(asn_DEF_RICindicationHeader, &indication_header);
    ASN_STRUCT_RESET(asn_DEF_RICindicationMessage, &indication_msg);
  }

  RICrequestID_t request_id;
  RANfunctionID_t func_id;
  RICactionID_t action_id;
  RICindicationType_t indication_type;
  RICindicationSN_t indication_sn;
  RICcallProcessID_t call_process_id;
  RICindicationHeader_t indication_header;
  RICindicationMessage_t indication_msg;

  UEID_t *get_ui_id() {
    E2SM_RC_IndicationHeader *header = NULL; // FIXME needs free it afterwards
    asn_transfer_syntax syntax = ATS_ALIGNED_BASIC_PER;
    asn_dec_rval_t rval = asn_decode(NULL, syntax, &asn_DEF_UEID, (void **)&header, indication_header.buf, indication_header.size);
    if (rval.code != RC_OK) {
      fprintf(stderr, "ERROR %s:%d unable to decode UEID from indication header\n", __FILE__, __LINE__);
      return nullptr;
    }
    return &header->ric_indicationHeader_formats.choice.indicationHeader_Format2->ueID;
  }

  E2SM_RC_IndicationMessage_Format5_t *get_indication_msg_fmt5() {
    // FIXME needs free it afterwards
    E2SM_RC_IndicationMessage_t *msg = NULL;

    asn_transfer_syntax syntax = ATS_ALIGNED_BASIC_PER;
    asn_dec_rval_t rval = asn_decode(NULL, syntax, &asn_DEF_E2SM_RC_IndicationMessage, (void **)&msg, indication_msg.buf, indication_msg.size);
    // asn_dec_rval_t rval = aper_decode_complete(NULL, &asn_DEF_E2SM_RC_IndicationMessage, (void **)&msg, indication_msg.buf, indication_msg.size);
    if (rval.code != RC_OK) {
      fprintf(stderr, "ERROR %s:%d - unable to decode E2SM RC IndicationMessage\n", __FILE__, __LINE__);
      return nullptr;
    }

    return msg->ric_indicationMessage_formats.choice.indicationMessage_Format5;
  }

};

#endif
