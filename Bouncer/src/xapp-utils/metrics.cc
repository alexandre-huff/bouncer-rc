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
            .Help("E2SM-RC REPORT Style 4 RSRP")
            .Labels({{"HOSTNAME", hostname}})
            .Register(*registry);
    gauge_rsrq = &BuildGauge()
            .Name("e2sm_rc_report_style4_rsrq")
            .Help("E2SM-RC REPORT Style 4 RSRQ")
            .Labels({{"HOSTNAME", hostname}})
            .Register(*registry);
    gauge_sinr = &BuildGauge()
            .Name("e2sm_rc_report_style4_sinr")
            .Help("E2SM-RC REPORT Style 4 SINR")
            .Labels({{"HOSTNAME", hostname}})
            .Register(*registry);

    exposer = std::make_shared<Exposer>("0.0.0.0:9090", 1);
    exposer->RegisterCollectable(registry);
}

PrometheusMetrics::~PrometheusMetrics() {
    for (auto it : ues) {
        prometheus_ue_info_t &ue_data = it.second;
        for (auto &it_data : ue_data.metrics) {
            std::shared_ptr<prometheus_ue_metrics_t> &data = it_data.second;
            gauge_rrc_state->Remove(data->rrc_state);
            gauge_rrc_state->Remove(data->rsrp);
            gauge_rrc_state->Remove(data->rsrq);
            gauge_rrc_state->Remove(data->sinr);
        }
    }
}

std::shared_ptr<prometheus_ue_metrics_t> PrometheusMetrics::get_ue_metrics_instance(std::string imsi, uint64_t nci) {
    const auto &it_map = ues.find(imsi);    // fetch ue info
    if (it_map != ues.end()) {
        // fetch metrics by cell
        const auto &it_metrics = it_map->second.metrics.find(nci);
        if (it_metrics != it_map->second.metrics.end()) {
            return it_metrics->second;
        }
    }
    return std::shared_ptr<prometheus_ue_metrics_t>();
}

/*
    Deletes all metrics from a given UE
*/
void PrometheusMetrics::delete_ue_metrics_instance(std::string imsi) {
    const auto &it = ues.find(imsi);
    if (it != ues.end()) {
        std::vector<uint64_t> nci_list;

        prometheus_ue_info_t &ue_info = it->second;

        for (auto &ue_metrics : ue_info.metrics) {
            auto &data = ue_metrics.second;

            gauge_rrc_state->Remove(data->rrc_state);
            gauge_rsrp->Remove(data->rsrp);
            gauge_rsrq->Remove(data->rsrq);
            gauge_sinr->Remove(data->sinr);

            nci_list.emplace_back(ue_metrics.first);
        }

        for (auto &nci : nci_list) {
            ue_info.metrics.erase(nci);
        }

        ues.erase(imsi);
    }
}

std::shared_ptr<prometheus_ue_metrics_t> PrometheusMetrics::create_ue_metrics_instance(
    std::string imsi, std::string gnb_mcc, std::string gnb_mnc, uint32_t gnb_id, uint16_t pci, uint64_t nci) {

    std::shared_ptr<prometheus_ue_metrics_t> data = std::make_shared<prometheus_ue_metrics_t>();
    std::string gnbid = std::to_string(gnb_id);
    std::string cellid = std::to_string(pci);

    // TODO IMPORTANT: commented out to decrease the overhead of unused data scrapped by Prometheus
    // All RRC State, RSRP, and RSRQ are already working
    // data->rrc_state = &gauge_rrc_state->Add({
    //     {"imsi", imsi},
    //     {"mcc", gnb_mcc},
    //     {"mnc", gnb_mnc},
    //     {"gnbid", gnbid},
    //     {"pci", cellid}
    // }, 0.0);

    // data->rsrp = &gauge_rsrp->Add({
    //     {"imsi", imsi},
    //     {"mcc", gnb_mcc},
    //     {"mnc", gnb_mnc},
    //     {"gnbid", gnbid},
    //     {"pci", cellid}
    // }, 0.0);

    // data->rsrq = &gauge_rsrq->Add({
    //     {"imsi", imsi},
    //     {"mcc", gnb_mcc},
    //     {"mnc", gnb_mnc},
    //     {"gnbid", gnbid},
    //     {"pci", cellid}
    // }, 0.0);

    data->sinr = &gauge_sinr->Add({
        {"imsi", imsi},
        {"mcc", gnb_mcc},
        {"mnc", gnb_mnc},
        {"gnbid", gnbid},
        {"pci", cellid}
    }, 0.0);

    const auto &it_map = ues.find(imsi);    // fetch ue info
    if (it_map != ues.end()) {              // create a new metric per cell id
        it_map->second.metrics.emplace(nci, data);

    } else {                                // if not present we add a new entry of UE info
        prometheus_ue_info_t ue_info;
        ue_info.metrics.emplace(nci, data);
        ues.emplace(imsi, std::move(ue_info));
    }

    return data;
}
