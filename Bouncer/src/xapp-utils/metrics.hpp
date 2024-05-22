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

#ifndef METRICS_HPP
#define METRICS_HPP

#include <string>
#include <unordered_map>
#include <memory>

#include <prometheus/family.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/gauge.h>

using namespace prometheus;

// helper for prometheus metrics
typedef struct {
    // std::string imsi;
    // std::string gnbid;
    Gauge *rrc_state;
    Gauge *rsrp;
    Gauge *rsrq;
    Gauge *sinr;
} prometheus_ue_metrics_t;

typedef struct {
    // std::string imsi;
    std::unordered_map<std::string, prometheus_ue_metrics_t> metrics;  // key is gnbid
} prometheus_ue_info_t;

typedef struct {
    bool primary_cell;
    long rsrp;
    long rsrq;
    long sinr;
    std::string nr_cgi;
} raw_ue_metrics_t;

/*
    Builds the prometheus configuration and exposes its metrics on port 9090
*/
class PrometheusMetrics {
public:
    PrometheusMetrics();
    ~PrometheusMetrics();

    prometheus_ue_metrics_t &get_ue_metrics_instance(std::string imsi, std::string gnbid);
    void delete_ue_metrics_instance(std::string imsi);

private:
    prometheus_ue_metrics_t create_ue_metrics_instance(std::string imsi, std::string gnbid);

    std::shared_ptr<Registry> registry;
    std::shared_ptr<Exposer> exposer;
    Family<Gauge> *gauge_rrc_state;
    Family<Gauge> *gauge_rsrp;
    Family<Gauge> *gauge_rsrq;
    Family<Gauge> *gauge_sinr;
    std::unordered_map<std::string, prometheus_ue_info_t> ues;  // key is imsi
};



#endif
