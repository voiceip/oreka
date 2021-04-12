#include "LiveStreamServer.h"
#include "json.h"
#include "ConfigManager.h"
#include "LiveStreamServerProxy.h"

using json = nlohmann::json;
using namespace std;
using namespace httplib;

void LiveStreamServer::Start()
{

	std::cout << "LiveSreamServer::Start....." << std::endl;
	httplib::Server svr;

	svr.Get("/hi", [](const Request &req, Response &res) {
		json response = {
			{"message", "Hi!"}};
		res.set_content(response.dump(), "application/json");
	});

	svr.Get("/livestream", [](const Request &req, Response &res) {
		json responseBody;
		json response;
		try
		{
			for (auto callId : LiveStreamServerProxy::Singleton()->GetStream())
			{
				if (callId.length() > 0)
					responseBody["liveCalls"].push_back(callId);
			}
			res.status = 200;
		}
		catch (const std::exception &e)
		{
			std::string response_msg = "System Error: " + string(e.what());
			response = {
				{"message", response_msg}};
			res.status = 500;
		}
		if (responseBody.size() != 0)
		{
			response.push_back(responseBody);
		}
		else
		{
			response = {
				{"liveCalls", json::array()}};
		}

		res.set_content(response.dump(), "application/json");
	});

	svr.Post("/livestream/start", [](const Request &req, Response &res) {
		CStdString url;
		auto requestBody = json::parse(req.body);
		json response;
		CStdString m_nativecallid;
		try
		{
			m_nativecallid = requestBody["nativeCallId"].get<std::string>();

			try
			{
				if (LiveStreamServerProxy::Singleton()->StartStream(m_nativecallid))
				{
					url = "rtmp://" + CONFIG.m_rtmpServerEndpoint + ":" + CONFIG.m_rtmpServerPort + "/live/" + m_nativecallid;
					res.status = 200;
				}
				response = {
					{"url", url}};
			}
			catch (const std::exception &e)
			{
				std::string response_msg = "System Error: " + string(e.what());
				response = {
					{"message", response_msg}};
				res.status = 500;
			}
		}
		catch (...)
		{
			response = {
				{"message", "Bad Request"}};
			res.status = 400;
		}

		res.set_content(response.dump(), "application/json");
	});

	svr.Post("/livestream/stop", [](const Request &req, Response &res) {
		CStdString url;
		auto requestBody = json::parse(req.body);
		json response;
		CStdString m_nativecallid;
		try
		{
			m_nativecallid = requestBody["nativeCallId"].get<std::string>();

			try
			{
				if (LiveStreamServerProxy::Singleton()->EndStream(m_nativecallid))
				{
					url = "rtmp://" + CONFIG.m_rtmpServerEndpoint + ":" + CONFIG.m_rtmpServerPort + "/live/" + m_nativecallid;
					res.status = 200;
				}
				response = {
					{"url", url}};
			}
			catch (const std::exception &e)
			{
				std::string response_msg = "System Error: " + string(e.what());
				response = {
					{"message", response_msg}};
				res.status = 500;
			}
		}
		catch (...)
		{
			response = {
				{"message", "Bad Request"}};
			res.status = 400;
		}

		res.set_content(response.dump(), "application/json");
	});

	svr.listen("0.0.0.0", serverPort);

	Stop(&svr);

	std::cout << "Shutdown Complete.." << std::endl;
}

void LiveStreamServer::Stop(httplib::Server *svr)
{
	std::cout << "LiveSreamServer::Stop....." << std::endl;
	svr->stop();
}