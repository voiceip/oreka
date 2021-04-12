#pragma once

#ifndef LIVESTREAMSERVER_H
#define LIVESTREAMSERVER_H

#include <httplib.h>

class LiveStreamServer
{

public:
	int serverPort;
	LiveStreamServer(int port)
	{
		serverPort = port;
	}

	~LiveStreamServer()
	{
		std::cout << "Shutting Down LiveStream HTTP Service" << std::endl;
	}

	void Start();

	void Stop(httplib::Server *svr);
};

#endif
