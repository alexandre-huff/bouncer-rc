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

#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <mdclog/mdclog.h>

#include "PLMN-Identity.h"
#include "BIT_STRING.h"
#include "RANParameter-ID.h"
#include "RANParameter-STRUCTURE.h"
#include "RANParameter-STRUCTURE-Item.h"
#include "RANParameter-Value.h"
#include "RANParameter-ValueType.h"
#include "NR-CGI.h"
#include "RANParameter-LIST.h"

bool decode_plmnid(PLMN_Identity_t *plmnid, std::string &mcc, std::string &mnc);
bool translateBitString(std::string &dest, const BIT_STRING_t *data);

RANParameter_STRUCTURE_Item_t *get_ran_parameter_structure_item(const RANParameter_STRUCTURE_t *ranp_s,
                                                                const RANParameter_ID_t ranp_id,
                                                                const RANParameter_ValueType_PR ranp_type);

std::vector<RANParameter_STRUCTURE_t *> get_ran_parameter_list_items(const RANParameter_LIST_t *ranp_list);

bool decode_NR_CGI(const NR_CGI_t *nr_cgi, std::string &mcc, std::string &mnc, std::string &nr_cellid_hex);


/// @brief Retrieves the data of a given RAN Parameter Value
/// @tparam T Return type of the data
/// @param v The ASN1 RAN Parameter Value
/// @param vtype The type that the ASN1 RAN Parameter Value must match
/// @return A pointer to the address of @p T in @p v , or @p nullptr on error:
///         - RAN Parameter Value type mismatching @p vtype
///         - Invalid @p vtype enum value
template<typename T>
T *get_ran_parameter_value_data(const RANParameter_Value_t *v, RANParameter_Value_PR vtype) {
    /*
        Please check why this implementation is required to be in the header file
        Source:
        https://isocpp.org/wiki/faq/templates#templates-defn-vs-decl
        https://stackoverflow.com/questions/495021/why-can-templates-only-be-implemented-in-the-header-file/495056#495056
    */

    T *value;

    if (!v) {
        mdclog_write(MDCLOG_ERR, "RAN Parameter Value is required. nil?");
        return nullptr;
    }

    if (v->present != vtype) {
        mdclog_write(MDCLOG_ERR, "RAN Parameter Value Type does not match with type %d", vtype);
        return nullptr;
    }

    switch (v->present) {
        case RANParameter_Value_PR_valueBoolean:
            value = (T *)&v->choice.valueBoolean;
            break;
        case RANParameter_Value_PR_valueInt:
            value = (T *)&v->choice.valueInt;
            break;
        case RANParameter_Value_PR_valueReal:
            value = (T *)&v->choice.valueReal;
            break;
        case RANParameter_Value_PR_valueBitS:
            value = (T *)&v->choice.valueBitS;
            break;
        case RANParameter_Value_PR_valueOctS:
            value = (T *)&v->choice.valueOctS;
            break;
        case RANParameter_Value_PR_valuePrintableString:
            value = (T *)&v->choice.valuePrintableString;
            break;
        case RANParameter_Value_PR_NOTHING:
        default:
            mdclog_write(MDCLOG_ERR, "Invalid RANParameter value PR %d in %s", v->present, asn_DEF_RANParameter_Value.name);
            value = nullptr;
    }

    return value;
}


#endif
