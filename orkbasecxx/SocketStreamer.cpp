/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 * Please refer to http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "SocketStreamer.h"

static LoggerPtr s_log;

void SocketStreamer::Initialize()
{
	std::list<CStdString>::iterator it;
	CStdString logMsg;

	s_log = Logger::getLogger("socketstreamer");

	for(it = CONFIG.m_socketStreamerTargets.begin(); it != CONFIG.m_socketStreamerTargets.end(); it++)
	{
		CStdString tcpAddress = *it;
		CStdString host;
		int port = 0;
		struct in_addr hostAddr;

		memset(&hostAddr, 0, sizeof(hostAddr));
		host = GetHostFromAddressPair(tcpAddress);
		port = GetPortFromAddressPair(tcpAddress);

		if(!host.size() || port == 0)
		{
			logMsg.Format("Invalid host:%s or port:%d -- check SocketStreamerTargets in config.xml", host, port);
			LOG4CXX_WARN(s_log, logMsg);
			continue;
		}

		if(!ACE_OS::inet_aton((PCSTR)host, &hostAddr))
		{
			logMsg.Format("Invalid host:%s -- check SocketStreamerTargets in config.xml", host);
			LOG4CXX_WARN(s_log, logMsg);
			continue;
		}

		TcpAddress* addr = new TcpAddress;

		addr->port = port;
		memcpy(&addr->ip, &hostAddr, sizeof(struct in_addr));

		if(!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(ThreadHandler), (void*)addr))
		{
			delete addr;
			logMsg.Format("Failed to start thread on %s,%d", host, port);
			LOG4CXX_WARN(s_log, logMsg);
			continue;
		}
	}
}


void SocketStreamer::ThreadHandler(void *args)
{
	TcpAddress* tcpAddress = (TcpAddress*)args;
	char szIp[16];
	char buf[1024];
	CStdString ipPort;
	ACE_INET_Addr srvr;
	ACE_SOCK_Connector connector;
	ACE_SOCK_Stream peer;
	bool connected = false;
	struct timespec ts;
	time_t nextReportConnFail = 0;
	time_t nextReportBytesRead = 0;
	int bytesRead = 0;
	unsigned long int bytesSoFar = 0;

	memset(&szIp, 0, sizeof(szIp));
	memset(buf, 0, sizeof(buf));

	ACE_OS::inet_ntop(AF_INET, (void*)&tcpAddress->ip, szIp, sizeof(szIp));
	ipPort.Format("%s,%u", szIp, tcpAddress->port);

	do {
		if(!connected)
		{
			if(SocketStreamer::Connect(tcpAddress, srvr, connector, peer) == 0)
			{
				LOG4CXX_INFO(s_log, "Connected to:" + ipPort);
				connected = true;
				nextReportConnFail = 0;
			}
			else
			{
				if(time(NULL) >= nextReportConnFail)
				{
					LOG4CXX_WARN(s_log, "Couldn't connect to:" + ipPort + ": " + CStdString(strerror(errno)));
					nextReportConnFail = time(NULL) + 60;
				}

				ts.tv_sec = 2;
				ts.tv_nsec = 0;
				ACE_OS::nanosleep(&ts, NULL);

				continue;
			}
		}

		bytesRead = peer.recv(buf, sizeof(buf));
		if(bytesRead < 0)
		{
			LOG4CXX_WARN(s_log, "Connection to:" + ipPort + " closed: " + CStdString(strerror(errno)));
			peer.close();
			connected = false;
			continue;
		}

		if(bytesRead == 0)
		{
			LOG4CXX_WARN(s_log, "Connection to:" + ipPort + " closed: Remote host closed connection");
			peer.close();
			connected = false;
			continue;
		}

		bytesSoFar += bytesRead;
		if(time(NULL) > nextReportBytesRead)
		{
			LOG4CXX_INFO(s_log, "Read " + FormatDataSize(bytesSoFar) + " from " + ipPort + " so far");
			nextReportBytesRead = time(NULL) + 60;
		}

		if(bytesSoFar >= 4000000000UL)
		{
			bytesSoFar = 0;
		}

		ts.tv_sec = 0;
		ts.tv_nsec = 1;
		ACE_OS::nanosleep(&ts, NULL);
	} while(1);
}

int SocketStreamer::Connect(TcpAddress* tcpAddress, ACE_INET_Addr& srvr, ACE_SOCK_Connector& connector, ACE_SOCK_Stream& peer)
{
	char szIp[16];

	memset(&szIp, 0, sizeof(szIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&tcpAddress->ip, szIp, sizeof(szIp));

	srvr.set(tcpAddress->port, szIp);
	if(connector.connect(peer, srvr) == -1)
	{
		return -1;
	}

	return 0;
}
