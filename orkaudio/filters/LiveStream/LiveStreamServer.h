/*
 * Oreka -- A media capture and retrieval platform
 *
 * LiveStreamFilter Plugin
 * Author Shushant Sharan
 *
 */
#ifndef LIVESTREAMSERVER_H
#define LIVESTREAMSERVER_H
#define CPPHTTPLIB_USE_POLL

#include <httplib.h>
#include <nlohmann/json.hpp>
#include "ConfigManager.h"
#include "LiveStreamSession.h"
#include "Config.h"

class LiveStreamServer {
    public:
        LiveStreamServer(int port);
        void Run();
        void Start();
        void Stop();

    private:
        int m_port;
        httplib::Server svr;
};

#endif
