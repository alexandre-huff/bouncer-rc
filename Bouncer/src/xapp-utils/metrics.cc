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

#include "metrics.hpp"


PrometheusMetrics::PrometheusMetrics() {

    std::string hostname = "unknown-bouncer-rc-hostname";
    char *pod_env = std::getenv("HOSTNAME");
    if(pod_env != NULL) {
        hostname = pod_env;
    }

    registry = std::make_shared<Registry>();
    gauge_rrc_state = &BuildGauge()
            .Name("e2sm_rc_report_style4_rrc_state")
            .Help("E2SM-RC REPORT Style 4 rrc state (connected = 1, disconnected = 0)")
            .Labels({{"HOSTNAME", hostname}})
            .Register(*registry);
    gauge_rsrp = &BuildGauge()
            .Name("e2sm_rc_report_style4_rsrp")
            .Help("E2SM-RC REPORT Style 4 RSRP (Integer range 0..127)")
            .Labels({{"HOSTNAME", hostname}})
            .Register(*registry);
    gauge_rsrq = &BuildGauge()
            .Name("e2sm_rc_report_style4_rsrq")
            .Help("E2SM-RC REPORT Style 4 RSRQ (Integer range 0..127)")
            .Labels({{"HOSTNAME", hostname}})
            .Register(*registry);
    gauge_sinr = &BuildGauge()
            .Name("e2sm_rc_report_style4_sinr")
            .Help("E2SM-RC REPORT Style 4 SINR (Integer range 0..127)")
            .Labels({{"HOSTNAME", hostname}})
            .Register(*registry);

    exposer = std::make_shared<Exposer>("0.0.0.0:9090", 1);
    exposer->RegisterCollectable(registry);
}

PrometheusMetrics::~PrometheusMetrics() {
    for (auto it : metrics) {
        ue_metrics_t &data = it.second;
        gauge_rrc_state->Remove(data.rrc_state);
        gauge_rrc_state->Remove(data.rsrp);
        gauge_rrc_state->Remove(data.rsrq);
        gauge_rrc_state->Remove(data.sinr);
    }
}

ue_metrics_t& PrometheusMetrics::get_ue_metrics_instance(std::string imsi, std::string gnbid) {
    const auto &it = metrics.find(imsi);
    if (it == metrics.end()) {  // if not present we add a new entry
        ue_metrics_t data = create_ue_metrics_instance(imsi, gnbid);
        metrics.emplace(imsi, std::move(data));
        return metrics.at(imsi);
    }

    if (it->second.gnbid.compare(gnbid) != 0) { // if ue has changed its gNodeB connection
        ue_metrics_t new_data = create_ue_metrics_instance(imsi, gnbid);
        metrics.erase(it);
        metrics.emplace(imsi, std::move(new_data));
        return metrics.at(imsi);
    }

    return it->second;
}

ue_metrics_t PrometheusMetrics::create_ue_metrics_instance(std::string imsi, std::string gnbid) {
    ue_metrics_t data;
    data.imsi = imsi;
    data.gnbid = gnbid;

    data.rrc_state = &gauge_rrc_state->Add({
        {"imsi", imsi},
        {"gnbid", gnbid}
    }, 0.0);

    data.rsrp = &gauge_rsrp->Add({
        {"imsi", imsi},
        {"gnbid", gnbid}
    }, 0.0);

    data.rsrq = &gauge_rsrq->Add({
        {"imsi", imsi},
        {"gnbid", gnbid}
    }, 0.0);

    data.sinr = &gauge_sinr->Add({
        {"imsi", imsi},
        {"gnbid", gnbid}
    }, 0.0);

    return data;
}

void PrometheusMetrics::update_ue_metrics_gnb_label(ue_metrics_t& data, std::string gnbid) {
    data.gnbid = gnbid;

    data.rrc_state = &gauge_rrc_state->Add({
        {"imsi", data.imsi},
        {"gnbid", gnbid}
    }, 0.0);

    data.rsrp = &gauge_rsrp->Add({
        {"imsi", data.imsi},
        {"gnbid", gnbid}
    }, 0.0);

    data.rsrq = &gauge_rsrq->Add({
        {"imsi", data.imsi},
        {"gnbid", gnbid}
    }, 0.0);

    data.sinr = &gauge_sinr->Add({
        {"imsi", data.imsi},
        {"gnbid", gnbid}
    }, 0.0);
}
