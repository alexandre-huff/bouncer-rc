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

#include <mdclog/mdclog.h>

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
