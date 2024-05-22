/*****************************************************************************
#                                                                            *
# Copyright 2024 Alexandre Huff                                              *
#                                                                            *
# Licensed under the Apache License, Version 2.0 (the "License");            *
# you may not use this file except in compliance with the License.           *
# You may obtain a copy of the License at                                    *
#                                                                            *
#      http://www.apache.org/licenses/LICENSE-2.0                            *
#                                                                            *
# Unless required by applicable law or agreed to in writing, software        *
# distributed under the License is distributed on an "AS IS" BASIS,          *
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
# See the License for the specific language governing permissions and        *
# limitations under the License.                                             *
#                                                                            *
******************************************************************************/

#include <sstream>
#include <ios>

#include "utils.hpp"

/*
	Decodes PLMN ID to MCC and MNC

	14 F0 23 becomes 410 32
	or
	14 50 23 becomes 410 532
*/
bool decode_plmnid_old(PLMN_Identity_t *plmnid, std::string &mcc, std::string &mnc) {
	mcc.clear();
	mnc.clear();

	if (!plmnid || plmnid->size != 3) {
		return false;
	}

	std::string buf((const char *)plmnid->buf, plmnid->size);
	mcc = buf[1];
	mcc += buf[0];
	mcc += buf[3];

	if (std::toupper(buf[2]) == 'F') {
		mnc = buf[5];
		mnc += buf[4];
	} else {
		mnc = buf[2];
		mnc += buf[5];
		mnc += buf[4];
	}

	// mcc = std::to_string(atoi(mcc.c_str()));
	// mnc = std::to_string(atoi(mnc.c_str()));

	return true;
}

/*
	Decodes PLMN ID to MCC and MNC

	14 F0 23 becomes 410 32
	or
	14 50 23 becomes 410 532
*/
bool decode_plmnid(PLMN_Identity_t *plmnid, std::string &mcc, std::string &mnc) {
	mcc.clear();
	mnc.clear();

	if (!plmnid || plmnid->size != 3) {
		return false;
	}
	mcc = std::to_string((int)((plmnid->buf[0] & 0x0F)));
	mcc += std::to_string((int)((plmnid->buf[0] >> 4) & 0x0F));
	mcc += std::to_string((int)(plmnid->buf[1] & 0x0F));

	if (((plmnid->buf[1] >> 4) & 0x0F) == 15) {	// means F
		mnc = std::to_string((int)(plmnid->buf[2] & 0x0F));
		mnc += std::to_string((int)((plmnid->buf[2] >> 4) & 0x0F));
	} else {
		mnc = std::to_string((int)((plmnid->buf[1] >> 4) & 0x0F));
		mnc += std::to_string((int)(plmnid->buf[2] & 0x0F));
		mnc += std::to_string((int)((plmnid->buf[2] >> 4) & 0x0F));
	}

	return true;
}

bool translateBitString(std::string &dest, const BIT_STRING_t *data) {
    std::stringstream ss;
    unsigned b1 = 0;
    unsigned b2 = 0;
    unsigned tmp_digit=0;

    if (!data) {
        mdclog_write(MDCLOG_ERR, "Invalid arguments, data is nil?");
        return false;
    }

    for (int i = 0; i < (int)data->size; i++) {
        //
        // we need to shift trailing zeros in the bit string (from asn1c) to leading zeros
        //
        tmp_digit = (0==i ? (uint8_t) 0: (uint8_t) data->buf[i-1])<<(8-data->bits_unused);
        tmp_digit = tmp_digit | ((unsigned) data->buf[i] >> data->bits_unused);

	    b1 = tmp_digit & (unsigned)0xF0;
        b1 = b1 >> (unsigned)4;
        ss << std::uppercase << std::hex << b1;
        b2 = tmp_digit & (unsigned)0x0F;
        ss << std::uppercase << std::hex << b2;
    }

    dest = ss.str();

    return true;
}

RANParameter_STRUCTURE_Item_t *get_ran_parameter_structure_item(const RANParameter_STRUCTURE_t *ranp_s,
																const RANParameter_ID_t ranp_id,
																const RANParameter_ValueType_PR ranp_type) {
	if (!ranp_s) {
        mdclog_write(MDCLOG_ERR, "Unable to retrieve RAN Parameter ID %ld. Does RAN Parameter Structure is nil?", ranp_id);
        return nullptr;
    }

    RANParameter_STRUCTURE_Item_t *param = nullptr;

    int count = ranp_s->sequence_of_ranParameters->list.count;
    RANParameter_STRUCTURE_Item_t **params = ranp_s->sequence_of_ranParameters->list.array;
    for (int i = 0; i < count; i++) {
        if (params[i]->ranParameter_ID == ranp_id) {
            if (params[i]->ranParameter_valueType->present == ranp_type) {
                param = params[i];
            } else {
                mdclog_write(MDCLOG_ERR, "RAN Parameter ID %ld is not of type %d", ranp_id, ranp_type);
            }
            break;
        }
    }

	if (!param) {
		mdclog_write(MDCLOG_WARN, "RAN Parameter ID %ld not found in RAN Parameter Structure", ranp_id);
	}

    return param;
}

std::vector<RANParameter_STRUCTURE_t *> get_ran_parameter_list_items(const RANParameter_LIST_t *ranp_list) {
	if (!ranp_list) {
        mdclog_write(MDCLOG_ERR, "Unable to retrieve RAN Parameter LIST items. Does RAN Parameter List is nil?");
        return std::vector<RANParameter_STRUCTURE_t *>();
    }

	std::vector<RANParameter_STRUCTURE_t *> list;

    int count = ranp_list->list_of_ranParameter.list.count;
    RANParameter_STRUCTURE_t **params = ranp_list->list_of_ranParameter.list.array;

	for (int i = 0; i < count; i++) {
		list.emplace_back(params[i]);
    }

    return list;
}

bool decode_NR_CGI(const NR_CGI_t *nr_cgi, std::string &mcc, std::string &mnc, std::string &nr_cellid_hex) {
    if (!nr_cgi) {
        mdclog_write(MDCLOG_ERR, "Unable to decode NR CGI, nil?");
        return false;
    }

    // ASN1 has different names for PLMN Identity types in E2AP and E2SM, so we have to typecast
    if (!decode_plmnid((PLMN_Identity_t *)&nr_cgi->pLMNIdentity, mcc, mnc)) {
        mdclog_write(MDCLOG_ERR, "Unable to decode PLMN Identity from NR CGI");
        return false;
    }

	// we do not consider cell here, so the cell value is 0. Thus, NCI = gnbId * 2^(36-29) + cellid
    // we leave 7 bits for cellid
    uint64_t nodeb_id;
    nodeb_id = (uint64_t)nr_cgi->nRCellIdentity.buf[0] << 32;
    nodeb_id |= (uint64_t)nr_cgi->nRCellIdentity.buf[1] << 24;
    nodeb_id |= (uint64_t)nr_cgi->nRCellIdentity.buf[2] << 16;
    nodeb_id |= (uint64_t)nr_cgi->nRCellIdentity.buf[3] << 8;
    nodeb_id |= (uint64_t)nr_cgi->nRCellIdentity.buf[4];

    nodeb_id = nodeb_id >> nr_cgi->nRCellIdentity.bits_unused;
    nodeb_id = nodeb_id / (1 << 7); // we did not consider cellid for now
    // TODO check https://nrcalculator.firebaseapp.com/nrgnbidcalc.html
    // TODO check https://www.telecomhall.net/t/what-is-the-formula-for-cell-id-nci-in-5g-nr-networks/12623/2

    if (nodeb_id > ((1 << 29) - 1)) { // we use 29 bits to identify an E2 Node (double check)
        mdclog_write(MDCLOG_ERR, "Unable to decode Cell ID from NR CGI, value cannot be higher than 29 bits");
        return false;
    }

	std::string padding;
	std::stringstream ss;
	ss << std::uppercase << std::hex << nodeb_id;

	// we need to pad extra characters to complete 8 digits (required for 29 bits in hex)
	size_t len = 8 - ss.str().length();
	padding = padding.insert(0, len, '0');  // we pad with zeroes to reach 8 digits in the final cell idendity

	nr_cellid_hex = padding + ss.str();

    mdclog_write(MDCLOG_DEBUG, "NR CGI decoded to MCC=%s, MNC=%s, Cell_ID=%s", mcc.c_str(), mnc.c_str(), nr_cellid_hex.c_str());

    return true;
}
