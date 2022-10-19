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

#include "xapp.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <cpprest/http_client.h>
#include <cpprest/filestream.h>
#include <cpprest/uri.h>
#include <cpprest/json.h>

using namespace utility;
using namespace web;
using namespace web::http;
using namespace web::http::client;
using namespace concurrency::streams;
using jsonn = nlohmann::json;

#define BUFFER_SIZE 1024

Xapp::Xapp(XappSettings &config, XappRmr &rmr){

	  rmr_ref = &rmr;
	  config_ref = &config;
	  xapp_mutex = NULL;
	  subhandler_ref = NULL;
	  return;
  }

Xapp::~Xapp(void){
	if(xapp_mutex!=NULL){
		xapp_mutex->~mutex();
		delete xapp_mutex;
	}
}

//Stop the xapp. Note- To be run only from unit test scripts.
void Xapp::stop(void){
  // Get the mutex lock
	std::lock_guard<std::mutex> guard(*xapp_mutex);
	rmr_ref->set_listen(false);
	rmr_ref->~XappRmr();

	//Detaching the threads....not sure if this is the right way to stop the receiver threads.
	//Hence function should be called only in Unit Tests
	int threadcnt = xapp_rcv_thread.size();
	for(int i=0; i<threadcnt; i++){
		xapp_rcv_thread[i].detach();
	}
	sleep(10);
}

void Xapp::startup(SubscriptionHandler &sub_ref) {

	subhandler_ref = &sub_ref;
	set_rnib_gnblist();

	startup_registration_request(); // throws std::exception

	startup_http_listener();	// throws std::exception

	//send subscriptions.
	// startup_subscribe_kpm_requests();
	startup_subscribe_rc_requests(); // throws std::exception

	//read A1 policies
	//startup_get_policies();
	return;
}

void Xapp::start_xapp_receiver(XappMsgHandler& mp_handler, int threads){
	if(threads < 1) {
		threads = 1;
	}
	rmr_ref->set_listen(true);
	if(xapp_mutex == NULL){
		xapp_mutex = new std::mutex();
	}

	for(int i = 0; i < threads; i++) {
		mdclog_write(MDCLOG_INFO,"Receiver Thread %d, file= %s, line=%d", i, __FILE__, __LINE__);
		{
			std::lock_guard<std::mutex> guard(*xapp_mutex);
			std::thread th_recv([&](){ rmr_ref->xapp_rmr_receive(std::move(mp_handler), rmr_ref);});
			xapp_rcv_thread.push_back(std::move(th_recv));
		}
	}
	return;
}

void Xapp::shutdown(){
	mdclog_write(MDCLOG_INFO, "Shutting down xapp %s", config_ref->operator[](XappSettings::SettingName::XAPP_ID).c_str());

	//send subscriptions delete.
	shutdown_subscribe_deletes();
	// send deregistration request
	shutdown_deregistration_request();

	sleep(2);
	rmr_ref->set_listen(false);

	shutdown_http_listener();

	//Joining the threads
	int threadcnt = xapp_rcv_thread.size();
	for(int i=0; i<threadcnt; i++){
		if(xapp_rcv_thread[i].joinable())
			xapp_rcv_thread[i].join();
	}
	xapp_rcv_thread.clear();

	return;
}

inline void Xapp::subscribe_delete_request(string meid) {
	auto subs = subscription_map.find(meid);

	if (subs != subscription_map.end()) {

		auto delJson = pplx::create_task([subs, this]() {
			utility::string_t port = U("8088");
			utility::string_t address = U("http://service-ricplt-submgr-http.ricplt.svc.cluster.local:");
			address.append(port);
			address.append(U("/ric/v1/subscriptions/"));
			address.append( utility::string_t(subs->second));
			uri_builder uri(address);
			auto addr = uri.to_uri().to_string();
			http_client client(addr);
			ucout << utility::string_t(U("making requests at: ")) << addr <<std::endl;
			return client.request(methods::DEL);
		})

		// Get the response.
		.then([subs, this](http_response response) {
			// Check the status code.
			if (response.status_code() != 204) {
				throw std::runtime_error("Returned " + std::to_string(response.status_code()));
			}

			// Convert the response body to JSON object.
			std::wcout << "Deleted: " << std::boolalpha << (response.status_code() == 204) << std::endl;

			subscription_map.erase(subs->first);	// TODO also remove it from gnblist
		});

		// serialize the user details.

		try {
			delJson.wait();
		}
		catch (const std::exception& e) {
			printf("Error exception:%s\n", e.what());
		}

	}
	else{
		mdclog_write(MDCLOG_WARN,"Subscription delete cannot send in file=%s, line=%d for MEID %s as not subscribed",__FILE__,__LINE__, meid.c_str());
	}
}

void Xapp::shutdown_subscribe_deletes(void )
{
	std::string xapp_id = config_ref->operator [](XappSettings::SettingName::XAPP_ID);
	std::string gnb_id = config_ref->operator[](XappSettings::SettingName::G_NODE_B);

	mdclog_write(MDCLOG_INFO,"Preparing to send subscription Delete in file=%s, line=%d",__FILE__,__LINE__);

	auto gnblist = get_rnib_gnblist();

	size_t sz = gnblist.size();
	mdclog_write(MDCLOG_INFO,"GNBList size : %lu", sz);

	if(sz > 0) {
		string gnb_id = config_ref->operator[](XappSettings::SettingName::G_NODE_B);

		if (gnb_id.empty()) {	// we iterate over all gNodeBs if not specified
			for(size_t i = 0; i<sz; i++)
			{
				sleep(5);
				mdclog_write(MDCLOG_INFO,"sending subscription delete request %lu out of %lu", i+1, sz);
				mdclog_write(MDCLOG_INFO,"sending subscription delete to meid = %s", gnblist[i].c_str());

				subscribe_delete_request(gnblist[i]);
			}

		} else {
			sleep(5);
			subscribe_delete_request(gnb_id);
		}

	} else {
		mdclog_write(MDCLOG_INFO,"Subscriptions Delete cannot be sent as GNBList in RNIB is NULL");
	}

		/*


	 	subscription_helper  din;
	 	subscription_helper  dout;

         	subscription_delete sub_del;
         	subscription_delete sub_recv;


		unsigned char buf[BUFFER_SIZE];
	 	size_t buf_size = BUFFER_SIZE;
	 	bool res;


	 	//Random Data  for request
	 	int request_id = 1;
	 	int function_id = 1;

	 	din.set_request(request_id);
	 	din.set_function_id(function_id);

         	res = sub_del.encode_e2ap_subscription(&buf[0], &buf_size, din);

	 	mdclog_write(MDCLOG_INFO,"Sending subscription delete  in file= %s, line=%d for MEID %s",__FILE__,__LINE__, meid);

	 	xapp_rmr_header rmr_header;
 	 	rmr_header.message_type = RIC_SUB_DEL_REQ;
 	 	rmr_header.payload_length = buf_size; //data_size

	 	strcpy((char*)rmr_header.meid,gnblist[i].c_str());
	 	auto transmitter = std::bind(&XappRmr::xapp_rmr_send,rmr_ref, &rmr_header, (void*)buf); //(void*)data)
		if (subhandler_ref)
		{
			mdclog_write(MDCLOG_INFO,"subhandler_ref is valid pointer");
		}
		else
		{
		 	mdclog_write(MDCLOG_INFO,"subhandler_ref is invalid pointer");
		}
         	int result = subhandler_ref->manage_subscription_delete_request(gnblist[i], transmitter);

       	 	if(result==SUBSCR_SUCCESS)
		{

     	      		mdclog_write(MDCLOG_INFO,"Subscription Delete SUCCESSFUL in file= %s, line=%d for MEID %s",__FILE__,__LINE__, meid);
          	}
          	else
		{
		 	mdclog_write(MDCLOG_ERR,"Subscription Delete FAILED in file= %s, line=%d for MEID %s",__FILE__,__LINE__, meid);
              	}
		*/
}

void Xapp::startup_subscribe_kpm_requests(void ){
	mdclog_write(MDCLOG_INFO,"Preparing to send subscription in file=%s, line=%d",__FILE__,__LINE__);

	auto gnblist = get_rnib_gnblist();

	size_t sz = gnblist.size();
	mdclog_write(MDCLOG_INFO,"GNBList size : %lu", sz);
	if(sz <= 0)
		mdclog_write(MDCLOG_INFO,"Subscriptions cannot be sent as GNBList in RNIB is NULL");

	for(size_t i = 0; i<sz; i++)
	{
		sleep(5);
		auto meid = gnblist[i];

		mdclog_write(MDCLOG_INFO,"sending subscription request %lu out of %lu", i+1, sz);
		mdclog_write(MDCLOG_INFO,"sending subscription to meid = %s", meid.c_str());

		auto postJson = pplx::create_task([meid, this]() {

	        jsonn jsonObject;
            jsonObject =
				{
					{"SubscriptionId",""},
					{"ClientEndpoint",{
						{"Host","service-ricxapp-bouncer-xapp-http.ricxapp"},
						{"HTTPPort",8080},
						{"RMRPort",4560}}
					},
					{"Meid",meid},
					{"RANFunctionID",0},
					{"SubscriptionDetails",
						{
							{
								{"XappEventInstanceId",12345},
								{"EventTriggers",{0}},
								{"ActionToBeSetupList",
									{
										{
											{"ActionID",1},
											{"ActionType","report"},
											{"ActionDefinition",{0}},
											{"SubsequentAction",{
												{"SubsequentActionType","continue"},
												{"TimeToWait","zero"}}
											}
										}
									}
								}
							}
						}
					}

				};

			std::cout <<jsonObject.dump(4) << "\n";
			utility::stringstream_t s;
			s << jsonObject.dump().c_str();
			web::json::value ret = json::value::parse(s);
			// std::wcout << ret.serialize().c_str() << std::endl;
			utility::string_t port = U("8088");
			utility::string_t address = U("http://service-ricplt-submgr-http.ricplt.svc.cluster.local:");
			address.append(port);
			address.append(U("/ric/v1/subscriptions"));
			uri_builder uri(address);
			auto addr = uri.to_uri().to_string();
			http_client client(addr);
			//std::cout<<uri::validate(addr)<<" validation \n";
			ucout << utility::string_t(U("making requests at: ")) << addr << "\n";
			return client.request(methods::POST,U("/"),ret.serialize(),U("application/json"));
		})

		// Get the response.
		.then([meid, this](http_response response) {
			// Check the status code.
			if (response.status_code() != 201) {
					throw std::runtime_error("Returned " + std::to_string(response.status_code()));
			}

			// Convert the response body to JSON object.
			return response.extract_json();
		})

		// serialize the user details.
		.then([meid, this](json::value jsonObject) {
			std::cout<<"\nRecieved REST subscription response\n";
			std::wcout << jsonObject.serialize().c_str() << "\n";
			std::string tmp;
			tmp=jsonObject[U("SubscriptionId")].as_string();
			subscription_map.emplace(std::make_pair(meid, tmp));

		});

		try {
				postJson.wait();
		}
		catch (const std::exception& e) {
				printf("Error exception:%s\n", e.what());
		}


	/*
	 //give the message to subscription handler, along with the transmitter.
	 strcpy((char*)meid,gnblist[i].c_str());

         mdclog_write(MDCLOG_INFO,"GNBList size : %d", sz);
	mdclog_write(MDCLOG_INFO,"sending %d subscription request out of : %d",i+1, sz);
	 subscription_helper  din;
	 subscription_helper  dout;

	 subscription_request sub_req;
	 subscription_request sub_recv;

	 unsigned char buf[BUFFER_SIZE];
	 size_t buf_size = BUFFER_SIZE;
	 bool res;


	 //Random Data  for request
	 int request_id = 1;
	 int function_id = 0;
	 std::string event_def = "01";

	 din.set_request(request_id);
	 din.set_function_id(function_id);
	 din.set_event_def(event_def.c_str(), event_def.length());

	 std::string act_def = "01";

	 din.add_action(1,0,(void*)act_def.c_str(), act_def.length(), 0);

	 res = sub_req.encode_e2ap_subscription(&buf[0], &buf_size, din);

	 //mdclog_write(MDCLOG_INFO,"GNBList = %s and ith val = %d", gnblist[i], i);

	 mdclog_write(MDCLOG_INFO,"Sending subscription in file= %s, line=%d for MEID %s",__FILE__,__LINE__, meid);

	 xapp_rmr_header rmr_header;
 	 rmr_header.message_type = RIC_SUB_REQ;
 	 rmr_header.payload_length = buf_size; //data_size

	 strcpy((char*)rmr_header.meid,gnblist[i].c_str());

	 auto transmitter = std::bind(&XappRmr::xapp_rmr_send,rmr_ref, &rmr_header, (void*)buf); //(void*)data);

         int result = subhandler_ref->manage_subscription_request(gnblist[i], transmitter);

       	 if(result==SUBSCR_SUCCESS){

     	      mdclog_write(MDCLOG_INFO,"Subscription SUCCESSFUL in file= %s, line=%d for MEID %s",__FILE__,__LINE__, meid);
          }
          else {
		 mdclog_write(MDCLOG_ERR,"Subscription FAILED in file= %s, line=%d for MEID %s",__FILE__,__LINE__, meid);
              }
	    */
	}
   std::cout << "\nSubscription map size = " << subscription_map.size() << "\n";
}

inline void Xapp::subscribe_request(string meid) {
	std::string http_addr = config_ref->operator[](XappSettings::SettingName::HTTP_SRC_ID);
	std::string port = config_ref->operator[](XappSettings::SettingName::HTTP_PORT);
	int http_port = stoi(port);
	port = config_ref->operator[](XappSettings::SettingName::BOUNCER_PORT);
	int rmr_port = stoi(port);

	mdclog_write(MDCLOG_INFO, "sending subscription to meid = %s", meid.c_str());

	auto postJson = pplx::create_task([meid, http_addr, http_port, rmr_port, this]() {
		jsonn jsonObject;
		jsonObject =
			{
				{"SubscriptionId",""},
				{"ClientEndpoint",{{"Host",http_addr},{"HTTPPort",http_port},{"RMRPort",rmr_port}}},
				{"Meid",meid},
				{"RANFunctionID",1},
				{"SubscriptionDetails",
					{
						{
							{"XappEventInstanceId",12345},{"EventTriggers",{2}},
							{"ActionToBeSetupList",
								{
									{
										{"ActionID",1},
										{"ActionType","insert"},
										{"ActionDefinition",{3}},
										{"SubsequentAction",
											{
												{"SubsequentActionType","continue"},
												{"TimeToWait","w10ms"}
											}
										}
									}
								}
							}
						}
					}
				}
			};
		std::cout << jsonObject.dump(4) << "\n";
		utility::stringstream_t s;
		s << jsonObject.dump().c_str();
		web::json::value ret = json::value::parse(s);
		// std::wcout << ret.serialize().c_str() << std::endl;
		utility::string_t port = U("8088");
		utility::string_t address = U("http://service-ricplt-submgr-http.ricplt.svc.cluster.local:");
		address.append(port);
		address.append(U("/ric/v1/subscriptions"));
		uri_builder uri(address);
		auto addr = uri.to_uri().to_string();
		http_client client(addr);
		//std::cout<<uri::validate(addr)<<" validation \n";

		ucout << utility::string_t(U("making requests at: ")) << addr << "\n";

		return client.request(methods::POST,U("/"),ret.serialize(),U("application/json"));
	})

	// Get the response.
	.then([meid, this](http_response response)
		{
		// Check the status code.
		if (response.status_code() != 201) {
				throw std::runtime_error("Returned " + std::to_string(response.status_code()));
		}

		// Convert the response body to JSON object.
		return response.extract_json();
	})

	// serialize the user details.
	.then([meid, this](json::value jsonObject)
		{
			std::cout << "\nReceived REST subscription response: " << jsonObject.serialize().c_str() << "\n\n";

			std::string tmp;
			tmp = jsonObject[U("SubscriptionId")].as_string();
			subscription_map.emplace(std::make_pair(meid, tmp));
	});

	try
	{
		postJson.wait();
	}
	catch (const std::exception &e)
	{
		mdclog_write(MDCLOG_ERR, "xapp subscription exception: %s\n", e.what());
		throw;
	}
}

void Xapp::startup_subscribe_rc_requests(){
	mdclog_write(MDCLOG_INFO, "Preparing to send subscription in file=%s, line=%d", __FILE__, __LINE__);

	auto gnblist = get_rnib_gnblist();

	size_t sz = gnblist.size();
	mdclog_write(MDCLOG_INFO, "GNBList size : %lu", sz);

	if (sz > 0) {
		string gnb_id = config_ref->operator[](XappSettings::SettingName::G_NODE_B);

		if (gnb_id.empty()) {	// we iterate over all gNodeBs if not specified
			for (size_t i = 0; i < sz; i++)
			{
				sleep(5);
				mdclog_write(MDCLOG_INFO, "sending subscription request %lu out of %lu", i + 1, sz);

				// mdclog_write(MDCLOG_INFO,"GNBList,gnblist[i] = %s and ith val = %d", gnblist[i], i);
				subscribe_request(gnblist[i]);
			}

		} else {
			sleep(5);	// FIXME wait for registration to complete
			subscribe_request(gnb_id); // FIXME this should be called only after the xApp has been registered
		}

	} else {
		throw std::runtime_error("Subscriptions cannot be sent as GNBList in RNIB is NULL");
	}

	// std::cout << "\nSubscription map size = " << subscription_map.size() << "\n\n";
}

void Xapp::startup_get_policies(void){

    int policy_id = BOUNCER_POLICY_ID;

    std::string policy_query = "{\"policy_type_id\":" + std::to_string(policy_id) + "}";
    unsigned char * message = (unsigned char *)calloc(policy_query.length(), sizeof(unsigned char));
    memcpy(message, policy_query.c_str(),  policy_query.length());
    xapp_rmr_header header;
    header.payload_length = policy_query.length();
    header.message_type = A1_POLICY_QUERY;
    mdclog_write(MDCLOG_INFO, "Sending request for policy id %d\n", policy_id);
    rmr_ref->xapp_rmr_send(&header, (void *)message);
    free(message);

}

void Xapp::set_rnib_gnblist(void) {

	   openSdl();

	   void *result = getListGnbIds();
	   if(strlen((char*)result) < 1){
		    mdclog_write(MDCLOG_ERR, "ERROR: no data from getListGnbIds\n");
	        return;
	    }

	    mdclog_write(MDCLOG_INFO, "GNB List in R-NIB %s\n", (char*)result);


	    Document doc;
	    ParseResult parseJson = doc.Parse<kParseStopWhenDoneFlag>((char*)result);
	    if (!parseJson) {
			mdclog_write(MDCLOG_ERR, "JSON parse error: %s", (char *)GetParseErrorFunc(parseJson.Code()));
	    	return;
	    }

	    if(!doc.HasMember("gnb_list")){
	        mdclog_write(MDCLOG_ERR, "JSON Has No GNB List Object");
	    	return;
	    }
	    assert(doc.HasMember("gnb_list"));

	    const Value& gnblist = doc["gnb_list"];
	    if (gnblist.IsNull())
	      return;

	    if(!gnblist.IsArray()){
	        mdclog_write(MDCLOG_ERR, "GNB List is not an array");
	    	return;
	    }


	   	assert(gnblist.IsArray());
	    for (SizeType i = 0; i < gnblist.Size(); i++) // Uses SizeType instead of size_t
	    {
	    	assert(gnblist[i].IsObject());
	    	const Value& gnbobj = gnblist[i];
	    	assert(gnbobj.HasMember("inventory_name"));
	    	assert(gnbobj["inventory_name"].IsString());
	    	std::string name = gnbobj["inventory_name"].GetString();
	    	rnib_gnblist.push_back(name);

	    }
	    closeSdl();
	    return;

}

void Xapp::startup_registration_request() {
	mdclog_write(MDCLOG_INFO, "Preparing registration request");

	string xapp_name = config_ref->operator[](XappSettings::SettingName::XAPP_NAME);
	string xapp_id = config_ref->operator[](XappSettings::SettingName::XAPP_ID);
	string config_path = config_ref->operator[](XappSettings::SettingName::CONFIG_FILE);
	string config_str = config_ref->operator[](XappSettings::SettingName::CONFIG_STR);
	string version = config_ref->operator[](XappSettings::SettingName::VERSION);
	string rmr_addr = config_ref->operator[](XappSettings::SettingName::RMR_SRC_ID);
	string http_addr = config_ref->operator[](XappSettings::SettingName::HTTP_SRC_ID);
	string rmr_port = config_ref->operator[](XappSettings::SettingName::BOUNCER_PORT);
	string http_port = config_ref->operator[](XappSettings::SettingName::HTTP_PORT);

	rmr_addr.append(":" + rmr_port);

	if (!http_addr.empty() && !http_port.empty()) {
		http_addr.append(":" + http_port);
	}

	pplx::create_task([xapp_name, version, config_path, xapp_id, http_addr, rmr_addr, config_str]()
		{
			jsonn jObj;
			jObj = {
				{"appName", xapp_name},
				{"appVersion", version},
				{"configPath", config_path},
				{"appInstanceName", xapp_id},
				{"httpEndpoint", http_addr},
				{"rmrEndpoint", rmr_addr},
				{"config", config_str}
			};

			if (mdclog_level_get() > MDCLOG_INFO) {
				cerr << "registration body is\n" << jObj.dump(4) << "\n";
			}
			utility::stringstream_t s;
			s << jObj.dump().c_str();
			web::json::value ret = json::value::parse(s);

			utility::string_t port = U("8080");
			utility::string_t address = U("http://service-ricplt-appmgr-http.ricplt.svc.cluster.local:");
			address.append(port);
			address.append(U("/ric/v1/register"));
			uri_builder uri(address);
			auto addr = uri.to_uri().to_string();
			http_client client(addr);

			mdclog_write(MDCLOG_INFO, "sending registration request at: %s", addr.c_str());

			return client.request(methods::POST,U("/"),ret.serialize(),U("application/json"));
		})

		// Get the response.
		.then([xapp_id](http_response response)
		{
			// Check the status code
			if (response.status_code() == 201) {
				mdclog_write(MDCLOG_INFO, "xapp %s has been registered", xapp_id.c_str());
			} else {
				mdclog_write(MDCLOG_ERR, "registration returned http status code %s - %s",
							std::to_string(response.status_code()).c_str(), response.reason_phrase().c_str());
			}
		})

		// catch any exception
		.then([](pplx::task<void> previousTask)
		{
			try {
				previousTask.wait();
			} catch (exception& e) {
				mdclog_write(MDCLOG_ERR, "xapp registration exception: %s", e.what());
				throw;
			}
		}).get();	// get allows rethrowing exceptions from task
}

void Xapp::shutdown_deregistration_request() {
	mdclog_write(MDCLOG_INFO, "Preparing deregistration request");

	string xapp_name = config_ref->operator[](XappSettings::SettingName::XAPP_NAME);
	string xapp_id = config_ref->operator[](XappSettings::SettingName::XAPP_ID);

	pplx::create_task([xapp_name, xapp_id]()
		{
			jsonn jObj;
			jObj = {
				{"appName", xapp_name},
				{"appInstanceName", xapp_id}
			};

			if (mdclog_level_get() > MDCLOG_INFO) {
				cerr << "deregistration body is\n" << jObj.dump(4) << "\n";
			}
			utility::stringstream_t s;
			s << jObj.dump().c_str();
			web::json::value ret = json::value::parse(s);

			utility::string_t port = U("8080");
			utility::string_t address = U("http://service-ricplt-appmgr-http.ricplt.svc.cluster.local:");
			address.append(port);
			address.append(U("/ric/v1/deregister"));
			uri_builder uri(address);
			auto addr = uri.to_uri().to_string();
			http_client client(addr);

			mdclog_write(MDCLOG_INFO, "sending deregistration request at: %s", addr.c_str());

			return client.request(methods::POST,U("/"),ret.serialize(),U("application/json"));
		})

		// Get the response.
		.then([xapp_id](http_response response)
		{
			// Check the status code
			if (response.status_code() == 204) {
				mdclog_write(MDCLOG_INFO, "xapp %s has been deregistered", xapp_id.c_str());
			} else {
				mdclog_write(MDCLOG_ERR, "deregistration returned http status code %s - %s",
							std::to_string(response.status_code()).c_str(), response.reason_phrase().c_str());
			}
		})

		// catch any exception
		.then([](pplx::task<void> previousTask)
		{
			try {
				previousTask.wait();
			} catch (exception& e) {
				mdclog_write(MDCLOG_ERR, "deregistration exception: %s", e.what());
			}
		});
}

void Xapp::handle_error(pplx::task<void>& t, const utility::string_t msg) {
	try {
		t.get();
	} catch (std::exception& e) {
		mdclog_write(MDCLOG_ERR, "%s : Reason = %s", msg.c_str(), e.what());
	}
}

/*
	Handles JSON in http requests.

	Currenty we do nothing but logging the request.
*/
void Xapp::handle_request(http_request request) {

	if (mdclog_level_get() > MDCLOG_INFO) {
		cerr << "\n===== Handling HTTP request =====\n" << request.to_string() << "\n=================================\n\n";
	}

	auto answer = json::value::object();
	request
		.extract_json()
		.then([&answer, request, this](pplx::task<json::value> task) {
			try {
				answer = task.get();
				mdclog_write(MDCLOG_INFO, "Received REST notification %s", answer.serialize().c_str());

				request.reply(status_codes::OK, answer)
					.then([this](pplx::task<void> t)
					{
						handle_error(t, "http reply exception");
					});

			} catch (http_exception const &e) {
				mdclog_write(MDCLOG_ERR, "unable to process JSON payload from http request. Reason = %s", e.what());

				request.reply(status_codes::InternalError)
					.then([this](pplx::task<void> t)
					{
						handle_error(t, "http reply exception");
					});
			}

		}).wait();

}

void Xapp::startup_http_listener() {
	mdclog_write(MDCLOG_INFO, "Starting up HTTP Listener");

	utility::string_t port = U(config_ref->operator[](XappSettings::SettingName::HTTP_PORT));
	utility::string_t address = U("http://0.0.0.0:");
	address.append(port);
	address.append(U("/ric/v1/subscriptions/response"));
	uri_builder uri(address);

	auto addr = uri.to_uri().to_string();
	if (!uri::validate(addr)) {
		throw std::runtime_error("unable starting up the http listener due to invalid URI: " + addr);
	}

	listener = make_unique<http_listener>(addr);
	mdclog_write(MDCLOG_INFO, "Listening for REST Notification at: %s", addr.c_str());

	listener->support(methods::POST,[this](http_request request) { handle_request(request); });
	listener->support(methods::PUT,[this](http_request request){ handle_request(request); });
	try {
		listener
			->open()
			.wait();	// non-blocking operation

	} catch (exception const &e) {
		mdclog_write(MDCLOG_ERR, "startup http listener exception: %s", e.what());
		throw;
	}
}

void Xapp::shutdown_http_listener() {
	mdclog_write(MDCLOG_INFO, "Shutting down HTTP Listener");

	try {
		listener->close().wait();

	} catch (exception const &e) {
		mdclog_write(MDCLOG_ERR, "shutdown http listener exception: %s", e.what());
	}
}
