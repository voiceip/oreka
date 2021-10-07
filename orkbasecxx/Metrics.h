/*
 * Metrics.h : Provides support for metrics
 * 
 * Copyright (C) 2021, Kinshuk Bairagi
 *
 * This program is free software, distributed under the terms of
 * the GNU v3 General Public License.
 *
 */

#ifndef __METRICS_H__
#define __METRICS_H__

#include "OrkBase.h"
#include <statsd.hpp>
#include <chrono>
#include <string.h>

class DLL_IMPORT_EXPORT_ORKBASE Timer {
    const std::string metricName;
    const std::chrono::steady_clock::time_point start;

    public:
    Timer(std::string name);

    ~Timer();
};

class DLL_IMPORT_EXPORT_ORKBASE Metrics {
public:
    static Metrics* Instance();
    statsd Statsd();
    Timer* timer(std::string name);
    Metrics(std::string hostname, int port);

private:
    statsd m_StatsdClient;
    static Metrics metricsInstance;
};

#define METRICS (*Metrics::Instance())
#define STATSD_METRICS (Metrics::Instance()->Statsd())


#endif


