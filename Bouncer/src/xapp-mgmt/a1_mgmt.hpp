/*
# ==================================================================================
# Copyright 2023-2024 Alexandre Huff.
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

/*
    a1_mgmt.hpp

    Created on: Oct 2023
    Updated on: Jan 2024
    Author: Alexandre Huff
*/

#ifndef XAPP_A1_MGMT_HPP_
#define XAPP_A1_MGMT_HPP_

#include <string>

#include "a1_helper.hpp"
#include "xapp_config.hpp"

class A1Handler {
public:
    // A1Handler(XappSettings &config);
    A1Handler() { };
    ~A1Handler() { };

    bool parse_a1_policy(char *message, a1_policy_helper &helper);
    bool parse_a1_payload(a1_policy_helper &helper);
    bool serialize_a1_response(char *buffer, int *buf_len, a1_policy_helper &helper);

    std::string error_string;

private:
    // std::shared_ptr<SchemaDocument> parse_schema_file(const char *file);
    // bool validate_json(Document &doc, SchemaDocument &schema);

    // std::shared_ptr<SchemaDocument> a1_schema;          // overall A1 message schema
    // std::shared_ptr<SchemaDocument> a1_payload_schema;  // policy specific shema

};

#endif
