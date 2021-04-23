/*
 * Oreka -- A media capture and retrieval platform
 *
 * LiveStreamFilter Plugin
 * Author Shushant Sharan
 *
 */
#include "LiveStreamServer.h"

using json = nlohmann::json;
using namespace std;
using namespace httplib;

static log4cxx::LoggerPtr s_log = log4cxx::Logger::getLogger("interface.liveStreamServer");

LiveStreamServer::LiveStreamServer(int port) {
    m_port = port;
}

void LiveStreamServer::Run() {
    try {
        std::thread liveStreamHandler( & LiveStreamServer::Start, this);
        liveStreamHandler.detach();
        LOG4CXX_INFO(s_log, CStdString("Started LiveStreamServer on port:") + IntToString(m_port));
    } catch (const std::exception & ex) {
        LOG4CXX_ERROR(s_log, CStdString("Failed to start liveStreamHandler thread reason:") + ex.what());
    }
}

void LiveStreamServer::Start() {
    SetThreadName("oreka:liveStreamSvr");

    svr.Get("/hi", [](const Request & req, Response & res) {
        json response = {{"message", "Hi!" }}; 
        res.set_content(response.dump(), "application/json");
    });

    svr.Get("/livestream/livecalls", [](const Request & req, Response & res) {
        json response = {{"liveCalls", json::array() }};
        try {
            if (LiveStreamSessionsSingleton::instance() != NULL) {
                for (auto callId: LiveStreamSessionsSingleton::instance()->GetLiveCallList()) {
                    if (callId.length() > 0)
                        response["liveCalls"].push_back(callId);
                }
                res.status = 200;
            }
        } catch (const std::exception & e) {
            std::string response_msg = "System Error: " + string(e.what());
            response = {{"message", response_msg }};
            res.status = 500;
        }
        res.set_content(response.dump(), "application/json");
    });

    svr.Get("/livestream/streamcalls", [](const Request & req, Response & res) {
        json response = {{"streamCalls", json::array() }};
        try {
            for (auto callId: LiveStreamSessionsSingleton::instance()->GetStreamCallList()) {
                if (callId.length() > 0)
                    response["streamCalls"].push_back(callId);
            }
            res.status = 200;
        } catch (const std::exception & e) {
            std::string response_msg = "System Error: " + string(e.what());
            response = {{"message", response_msg }};
            res.status = 500;
        }
        res.set_content(response.dump(), "application/json");
    });

    svr.Post("/livestream/start", [](const Request & req, Response & res) {
        CStdString url;
        auto requestBody = json::parse(req.body);
        json response;
        CStdString m_nativecallid;
        try {
            m_nativecallid = requestBody["nativeCallId"].get<std::string>();
            try {
                if (m_nativecallid.size() > 0 && LiveStreamSessionsSingleton::instance() -> StartStreamNativeCallId(m_nativecallid)) {
                    url = "rtmp://" + CONFIG.m_rtmpServerEndpoint + ":" + CONFIG.m_rtmpServerPort + "/live/" + m_nativecallid;
                    res.status = 200;
                    response = {{"url", url }};
                } else {
                    response = {{"message", "Not Found!"}};
                    res.status = 500;
                }
            } catch (const std::exception & e) {
                std::string response_msg = "System Error: " + string(e.what());
                response = {{"message", response_msg }};
                res.status = 500;
            }
        } catch (...) {
            response = {{"message", "Bad Request"}};
            res.status = 400;
        }

        res.set_content(response.dump(), "application/json");
    });

    svr.Post("/livestream/stop", [](const Request & req, Response & res) {
        CStdString url;
        auto requestBody = json::parse(req.body);
        json response;
        CStdString m_nativecallid;
        try {
            m_nativecallid = requestBody["nativeCallId"].get<std::string>();

            try {
                if (m_nativecallid.size() > 0 && LiveStreamSessionsSingleton::instance() -> StopStreamNativeCallId(m_nativecallid)) {
                    url = "rtmp://" + CONFIG.m_rtmpServerEndpoint + ":" + CONFIG.m_rtmpServerPort + "/live/" + m_nativecallid;
                    res.status = 200;
                    response = {{"url", url }};
                } else {
                    response = {{"message", "Not Found!"}};
                    res.status = 500;
                }
            } catch (const std::exception & e) {
                std::string response_msg = "System Error: " + string(e.what());
                response = {{"message", response_msg }};
                res.status = 500;
            }
        } catch (...) {
            response = {{"message", "Bad Request"}};
            res.status = 400;
        }

        res.set_content(response.dump(), "application/json");
    });

    svr.listen("0.0.0.0", m_port);

    Stop();

    LOG4CXX_INFO(s_log, CStdString("LiveStreamServer::Shutdown Complete"));
}

void LiveStreamServer::Stop() {
    LOG4CXX_INFO(s_log, CStdString("LiveStreamServer::Stoping..."));
    svr.stop();
}