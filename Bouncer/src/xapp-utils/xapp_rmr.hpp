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

#ifndef XAPP_RMR_XAPP_RMR_H_
#define XAPP_RMR_XAPP_RMR_H_


#ifdef __GNUC__
#define likely(x)  __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <error.h>
#include <assert.h>
#include <sstream>
#include <thread>
#include <functional>
#include <map>
#include <mutex>
#include <sys/epoll.h>
#include <rmr/rmr.h>
#include <rmr/RIC_message_types.h>
#include <mdclog/mdclog.h>
#include <vector>
#include <ctime>
#include <chrono>
#include <sys/time.h>

#include "a1_helper.hpp"
#include "e2ap_control.hpp"
#include "e2ap_control_response.hpp"
#include "e2ap_indication.hpp"
#include "subscription_delete_request.hpp"
#include "subscription_delete_response.hpp"
#include "subscription_helper.hpp"
#include "subscription_request.hpp"
#include "subscription_response.hpp"
#include "e2sm_subscription.hpp"
#include "subs_mgmt.hpp"

typedef struct{
	struct timespec ts;
	int32_t message_type;
	int32_t state;
	int32_t payload_length;

	unsigned char sid[RMR_MAX_SID]; //Subscription ID.
	unsigned char src[RMR_MAX_SRC]; //Xapp Name
	unsigned char meid[RMR_MAX_MEID]={};

}  xapp_rmr_header;


class XappRmr{
private:
	std::string _proto_port;
	int _nattempts;
	bool _rmr_is_ready;
    bool _listen;
	void* _xapp_rmr_ctx;
	rmr_mbuf_t*		_xapp_send_buff;					// send buffer


public:

	XappRmr(std::string, int rmrattempts=10);
	~XappRmr(void);
	void xapp_rmr_init(bool);

	template <class MessageProcessor>
	void xapp_rmr_receive(MessageProcessor&&, XappRmr *parent);

	bool xapp_rmr_send(xapp_rmr_header*, void*);

	bool rmr_header(xapp_rmr_header*);
	void set_listen(bool);
	bool get_listen(void);
	int get_is_ready(void);
	bool get_isRunning(void);
	void* get_rmr_context(void);

};

static inline long elapsed_microseconds(struct timespec ts_start, struct timespec ts_end) {
    unsigned long start;
    unsigned long end;
    long latency;

    start = ts_start.tv_sec * 1000000000 + ts_start.tv_nsec;
    end = ts_end.tv_sec * 1000000000 + ts_end.tv_nsec;

    latency = (end - start) / 1000000;     // converting to microseconds

    return latency;
}

// main workhorse thread which does the listen->process->respond loop
template <class MsgHandler>
void XappRmr::xapp_rmr_receive(MsgHandler&& msgproc, XappRmr *parent){
	rmr_mbuf_t *mbuf = NULL;

	bool* resend = new bool(false);
	// Get the thread id
	std::thread::id my_id = std::this_thread::get_id();
	std::stringstream thread_id;
	std::stringstream ss;
	std::fstream io_file;

	thread_id << my_id;

	// Get the rmr context from parent (all threads and parent use same rmr context. rmr context is expected to be thread safe)
	if(!parent->get_is_ready()){
			mdclog_write( MDCLOG_ERR, "RMR Shows Not Ready in RECEIVER, file= %s, line=%d ",__FILE__,__LINE__);
			return;
	}
	void *rmr_context = parent->get_rmr_context();
	assert(rmr_context != NULL);

	mdclog_write(MDCLOG_INFO, "Starting receiver thread %s",  thread_id.str().c_str());
	io_file.open("/tmp/timestamp.txt", std::ios::in|std::ios::out|std::ios::app);
	std::time_t sentMsg_time;
	std::time_t recvMsg_time;
	// struct timeval ts_recv;
	// struct timeval ts_sent;
	struct timespec ts_recv;
	struct timespec ts_sent;
	int num = 0;

	while(parent->get_listen()) {
		mdclog_write(MDCLOG_DEBUG, "Listening at Thread: %s",  thread_id.str().c_str());

		mbuf = rmr_torcv_msg( rmr_context, mbuf, 2000 ); // come up every 2 sec to check for get_listen()

		if (mbuf == NULL || mbuf->state == RMR_ERR_TIMEOUT) {
			continue;
		}

		if (io_file) {
			if (mdclog_level_get() > MDCLOG_INFO) {
				clock_gettime(CLOCK_REALTIME, &ts_recv);
				io_file << "Received Msg with msgType: " << mbuf->mtype << " at time: " <<  (ts_recv.tv_sec * 1000) + (ts_recv.tv_nsec/1000000) << std::endl;
			}
		}

		if( mbuf->mtype < 0 || mbuf->state != RMR_OK ) {
			mdclog_write(MDCLOG_ERR, "bad msg:  state=%d  errno=%d, file= %s, line=%d", mbuf->state, errno, __FILE__,__LINE__ );
			return;
		}
		else
		{
			mdclog_write(MDCLOG_INFO,"RMR Received Message of Type: %d",mbuf->mtype);
			mdclog_write(MDCLOG_DEBUG,"RMR Received Message: %s",(char*)mbuf->payload);

		    //in case message handler returns true, need to resend the message.
			msgproc(mbuf, resend);

			//start of code to check decoding indication payload

			num++;
			mdclog_write(MDCLOG_DEBUG, "Total Indications received : %d", num);

			if(*resend){
				mdclog_write(MDCLOG_INFO,"RMR Return to Sender Message of Type: %d",mbuf->mtype);
				mdclog_write(MDCLOG_DEBUG,"RMR Return to Sender Message: %s",(char*)mbuf->payload);

				if (io_file) {
					if (mdclog_level_get() > MDCLOG_INFO) {
						clock_gettime(CLOCK_REALTIME, &ts_sent);
						io_file << "Send Msg with msgType: " << mbuf->mtype << " at time: " << (ts_sent.tv_sec * 1000) + (ts_sent.tv_nsec/1000000) << std::endl;

						// io_file << "Time diff: " << ((ts_sent.tv_sec - ts_recv.tv_sec)*1000 + (ts_sent.tv_usec - ts_recv.tv_usec)/1000) << std::endl;
						io_file << "Time diff: " << elapsed_microseconds(ts_recv, ts_sent) << std::endl;
					}
				}

				rmr_rts_msg(rmr_context, mbuf );
				//sleep(1);

				*resend = false;
			}

		}

	}

	if (io_file) {
		io_file.close();
	}

	// Clean up
	try{
		delete resend;
		rmr_free_msg(mbuf);
	}
	catch(std::runtime_error &e){
		std::string identifier = __FILE__ +  std::string(", Line: ") + std::to_string(__LINE__) ;
		std::string error_string = identifier = " Error freeing RMR message ";
		mdclog_write(MDCLOG_ERR, error_string.c_str(), "");
	}

	mdclog_write(MDCLOG_INFO, "Cleaned up receiver thread %s",  thread_id.str().c_str());

	return;
}

#endif /* XAPP_RMR_XAPP_RMR_H_ */
