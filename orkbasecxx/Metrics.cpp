/*
 * Metrics.cpp : Provides implementation for metrics
 * 
 * Copyright (C) 2021, Kinshuk Bairagi
 *
 * This program is free software, distributed under the terms of
 * the GNU v3 General Public License.
 *
 */

#include "Metrics.h"
#include <statsd.hpp>

Metrics Metrics::metricsInstance("localhost", 8125);

Metrics* Metrics::Instance()
{
	return &metricsInstance;
}

// statsd Metrics::Statsd() {
//     return m_StatsdClient;
// }

Metering::Timer* Metrics::timer(std::string name){
    return new Metering::Timer(name);
}

Metrics::Metrics(const std::string hostname, int16_t port){
    statsd::open(hostname, port, SOCK_DGRAM);
    statsd::setPrefix("orkaudio.");
}

Metering::Timer::Timer(std::string name): metricName(name), start(std::chrono::steady_clock::now()) {
    std::cout << "Starting Timer " << metricName <<  std::endl;
};

Metering::Timer::~Timer(){
    auto end = std::chrono::steady_clock::now();
    int duration =  std::chrono::duration_cast<std::chrono::milliseconds> (start - end).count();
    statsd::timing(metricName, duration);
    std::cout << "Time taken by " << metricName <<  " is " << duration << "ms" << std::endl;
};

