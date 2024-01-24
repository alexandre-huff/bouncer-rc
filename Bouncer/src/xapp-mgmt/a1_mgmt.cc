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
    a1_mgmt.cc

    Created on: Oct 2023
    Updated on: Jan 2024
    Author: Alexandre Huff
*/

#include <fstream>
#include <sstream>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/schema.h>

#include "a1_mgmt.hpp"

// /*
// 	throws std::runtime_error
// */
// A1Handler::A1Handler(XappSettings &config) {
// 	std::string a1_schema_file = config.operator[](XappSettings::SettingName::A1_POLICY_SCHEMA_FILE);
// 	std::string a1_payload_schema_file = config.operator[](XappSettings::SettingName::A1_PAYLOAD_SCHEMA_FILE);

// 	try {
// 		a1_schema = parse_schema_file(a1_schema_file.c_str());
// 		a1_payload_schema = parse_schema_file(a1_payload_schema_file.c_str());

// 	} catch(const std::exception& e) {
// 		throw std::runtime_error(e.what());
// 	}

// }

// A1Handler::~A1Handler() {
// 	// Nothing to do here
// }

// /*
// 	throws std::runtime_error
// */
// std::shared_ptr<SchemaDocument> A1Handler::parse_schema_file(const char *file) {
// 	std::stringstream ss;

// 	// reading the schema file
// 	std::ifstream f(file);
// 	std::string schema_str;
// 	if (!f) {
// 		ss << "Unable to open the schema file " << file;
// 		throw std::runtime_error(ss.str());
// 	}
// 	std::ostringstream os;
// 	os << f.rdbuf();
// 	schema_str = os.str();

// 	rapidjson::Document sd;
// 	if (sd.Parse(schema_str.c_str()).HasParseError()) {
// 		ss << "The schema is not a valid JSON: " << schema_str;
// 		throw std::runtime_error(ss.str());
// 	}

// 	std::shared_ptr<SchemaDocument> schema = std::make_shared<SchemaDocument>(sd);

//     return schema;
// }

// bool A1Handler::validate_json(Document &doc, SchemaDocument &schema) {
// 	// Validating json message based on the schema file
// 	rapidjson::SchemaValidator validator(schema);
// 	if (!doc.Accept(validator)) {
// 		// Input JSON is invalid according to the schema
// 		// Output diagnostic information
// 		StringBuffer sb;
// 		validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);

// 		std::stringstream ss;
// 		ss << "Invalid property: " << sb.GetString() << ", ";
// 		ss << "Invalid keyword: " << validator.GetInvalidSchemaKeyword() << ", ";
// 		sb.Clear();
// 		validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
// 		ss << "Invalid document: " << sb.GetString();

// 		error_string = ss.str();

// 		return false;
// 	}

// 	return true;
// }

bool A1Handler::parse_a1_policy(char *message, a1_policy_helper &helper) {
	rapidjson::Document doc;
	if (doc.Parse<kParseStopWhenDoneFlag>(message).HasParseError()){
		error_string = "Unable to parse A1 JSON message";
		return false;
	}

	// if (! validate_json(doc, *a1_schema.get())) {
	// 	std::stringstream ss;
	// 	ss << "Policy does not conform to schema file. " << error_string;
	// 	error_string = ss.str();
	// 	return false;
	// }

	//Extract Operation
	rapidjson::Pointer temp1("/operation");
	rapidjson::Value * ref1 = temp1.Get(doc);
	if (ref1 == NULL){
		error_string = "Unable to extract operation from A1 message";
		return false;
	}
	helper.operation = ref1->GetString();

	// Extract policy id type
	rapidjson::Pointer temp2("/policy_type_id");
	rapidjson::Value * ref2 = temp2.Get(doc);
	if (ref2 == NULL){
		error_string = "Unable to extract policy type id from A1 message";
		return false;
	}
	helper.policy_type_id = ref2->GetString();

	// Extract policy instance id
	rapidjson::Pointer temp("/policy_instance_id");
	rapidjson::Value * ref = temp.Get(doc);
	if (ref == NULL){
		error_string = "Unable to extract policy instance id from A1 message";
		return false;
	}
	helper.policy_instance_id = ref->GetString();

	// Extract payload
	rapidjson::Pointer payload_ptr("/payload");
	rapidjson::Value * payload_val = payload_ptr.Get(doc);
	if (ref == NULL){
		error_string = "Unable to extract payload from A1 message";
		return false;
	}
	helper.payload = payload_val->GetString();

	return true;
}

bool A1Handler::parse_a1_payload(a1_policy_helper &helper) {
	rapidjson::Document doc;
	if (doc.Parse<kParseStopWhenDoneFlag>(helper.payload.c_str()).HasParseError()){
		error_string = "Unable to parse payload in A1 JSON message";
		return false;
	}

	// if (! validate_json(doc, *a1_payload_schema.get())) {
	// 	std::stringstream ss;
	// 	ss << "Policy payload does not conform to schema file. " << error_string;
	// 	error_string = ss.str();
	// 	return false;
	// }

	// Extract list of ues
	std::stringstream ss;

	rapidjson::Pointer th_ptr("/threshold");
	rapidjson::Value *th_value = th_ptr.Get(doc);
	if (th_value == NULL) {
		ss << "Unable to extract threshold from: " << doc.GetString();
        error_string = ss.str();
        return false;
	}

    helper.threshold = th_value->GetInt();

	return true;
}

bool A1Handler::serialize_a1_response(char *buffer, int *buf_len, a1_policy_helper &helper) {
	Document doc;
	doc.SetObject();

	Document::AllocatorType& alloc = doc.GetAllocator();

    Value handler_id;
    handler_id.SetString(helper.handler_id.c_str(), helper.handler_id.length(), alloc);

    Value status;
    status.SetString(helper.status.c_str(), helper.status.length(), alloc);

    Value policy_id;
    policy_id.SetInt(stoi(helper.policy_type_id));

	Value policy_instance_id;
	policy_instance_id.SetString(helper.policy_instance_id.c_str(), helper.policy_instance_id.length(), alloc);

    doc.AddMember("policy_type_id", policy_id, alloc);
	doc.AddMember("policy_instance_id", policy_instance_id, alloc);
    doc.AddMember("handler_id", handler_id, alloc);
    doc.AddMember("status", status, alloc);

	StringBuffer sbuf;
    Writer<StringBuffer> writer(sbuf);
    doc.Accept(writer);

	int sbuflen = sbuf.GetLength();
    if (*buf_len > sbuflen) {	// might result in buffer overflow so we test it before copying
		strncpy(buffer, sbuf.GetString(), sbuflen + 1);  // we want a C-like null-terminated string
		*buf_len = sbuflen;

    } else {
		error_string = "The A1 response buffer is too small to encode the A1 response message";
		return false;
    }

    return true;
}
