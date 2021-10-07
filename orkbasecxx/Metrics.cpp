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

statsd Metrics::Statsd() {
    return m_StatsdClient;
}

Timer* Metrics::timer(std::string name){
    return new Timer(name);
}

Metrics::Metrics(std::string hostname, int port){
    m_StatsdClient.open(hostname, port, SOCK_DGRAM);
    m_StatsdClient.setPrefix("orkaudio.");
}

Timer::Timer(std::string name): metricName(name), start(std::chrono::steady_clock::now()) {

};

Timer::~Timer(){
    auto end = std::chrono::steady_clock::now();
    int duration =  std::chrono::duration_cast<std::chrono::milliseconds> (start - end).count();
    Metrics::Instance()->Statsd().timing(metricName, duration);
};

