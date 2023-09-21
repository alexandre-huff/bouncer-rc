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


#pragma once

#ifndef SRC_XAPP_HPP_
#define SRC_XAPP_HPP_

#include <iostream>
#include <string>
#include <memory>
#include <csignal>
#include <stdio.h>
#include <pthread.h>
#include <unordered_map>
#include <thread>
#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>
#include <nlohmann/json.hpp>
#include "xapp_rmr.hpp"
#include "xapp_sdl.hpp"
#include "rapidjson/writer.h"
#include "rapidjson/document.h"
#include "rapidjson/error/error.h"
#include "msgs_proc.hpp"
#include "subs_mgmt.hpp"
#include "xapp_config.hpp"
extern "C" {
#include "rnib/rnibreader.h"
}

using namespace std;
using namespace std::placeholders;
using namespace rapidjson;
using namespace web::http;
using namespace web::http::experimental::listener;

using jsonn = nlohmann::json;


class Xapp{
public:

  Xapp(XappSettings &, XappRmr &);

  ~Xapp(void);

  void stop(void);

  void startup(SubscriptionHandler &);
  void shutdown(void);

  void start_xapp_receiver(XappMsgHandler &, int);

  Xapp(Xapp const &)=delete;
  Xapp& operator=(Xapp const &) = delete;

  void register_handler(XappMsgHandler &fn){
	  _callbacks.emplace_back(fn);
  }

  //getters/setters.
  void set_rnib_gnblist(void);
  std::vector<std::string> get_rnib_gnblist(){ return rnib_gnblist; }

  void fetch_connected_nodeb_list();

private:
  void startup_subscribe_kpm_requests(void);
  void startup_subscribe_requests();
  void shutdown_delete_subscriptions(void);
  void startup_get_policies(void );
  void startup_registration_request();
  void shutdown_deregistration_request();
  inline void subscribe_request(string, jsonn);
  inline void subscribe_delete_request(string);
  jsonn build_rc_subscription_request(string);
  jsonn build_kpm_subscription_request(string);
  void startup_http_listener();
  void shutdown_http_listener();
  void handle_request(http_request request);
  void handle_error(pplx::task<void>& t, const utility::string_t msg);


  XappRmr * rmr_ref;
  XappSettings * config_ref;
  SubscriptionHandler *subhandler_ref;
  std::unique_ptr<http_listener> listener;

  std::mutex *xapp_mutex;
  std::vector<std::thread> xapp_rcv_thread;
  std::vector<std::string> rnib_gnblist;
  std::vector<XappMsgHandler> _callbacks;
  std::unordered_map<std::string, std::string> subscription_map;
  std::unordered_map<std::string, web::json::value> e2node_map;
};


#endif /* SRC_XAPP_HPP_ */
