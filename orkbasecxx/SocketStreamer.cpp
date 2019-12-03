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
#define _WINSOCKAPI_            // prevents the inclusion of winsock.h

#include "SocketStreamer.h"

SocketStreamer* SocketStreamerFactory::Create()
{
	return new SocketStreamer(Logger::getLogger("socketstreamer"),"orka:sockstream");
}

SocketStreamer::SocketStreamer(LoggerPtr log, CStdString threadName) :
	m_log(log),
	m_threadName(threadName)
{
	OrkAprSingleton* orkAprsingle = OrkAprSingleton::GetInstance();
	apr_pool_create(&m_peer, orkAprsingle->GetAprMp());
}

void SocketStreamer::ThreadHandler(void *args)
{
	SocketStreamer* ssc = (SocketStreamer*) args;
	SetThreadName(ssc->m_threadName);

	CStdString logMsg;

	CStdString params = ssc->m_logMsg;

	CStdString ipPort = params;
	bool connected = false;
	time_t lastLogTime = 0;
	int bytesRead = 0;
	unsigned long int bytesSoFar = 0;

	while(1) {
		if (!connected) {
			lastLogTime = 0;
			while (!ssc->Connect()) {
				if (time(NULL) - lastLogTime > 60 ) {
					FLOG_WARN(ssc->m_log, "Couldn't connect to: %s error: %s", ipPort, CStdString(strerror(errno)));
					lastLogTime = time(NULL);
				}
				OrkSleepSec(2);
			}
			connected=true;
			lastLogTime = 0;
		}

		bytesRead = ssc->Recv();
		if(bytesRead == -1)
		{
			FLOG_WARN(ssc->m_log, "Connection to: %s closed", ipPort);
			ssc->Close();
			connected = false;
			continue;
		}
		else if(bytesRead == 0)
        {
            continue;
        }

		bytesSoFar += bytesRead;
		if (time(NULL) - lastLogTime > 9 ) {
			FLOG_INFO(ssc->m_log,"Read %s from %s so far", FormatDataSize(bytesSoFar), ipPort);
			lastLogTime = time(NULL);
		}
		OrkSleepMs(2);
	}
}

bool SocketStreamer::Parse(CStdString target) 
{
	CStdString ip;
	ChopToken(ip,":",target);

	if (!inet_pton4((PCSTR)ip, &m_ip)) {
		m_logMsg.Format("Invalid host:%s", ip);
		return false;
	}
	m_logMsg += CStdString(" host:") + ip;

	m_port = strtol(target,NULL,0);
	if (m_port == 0) {
		m_logMsg.Format("Invalid port:%s", target);
		return false;
	}
	m_logMsg += CStdString(" port:") + target;

	return true;
}

bool SocketStreamer::Connect()
{
	char szIp[16];
	memset(m_buf, 0, sizeof(m_buf));
	memset(&szIp, 0, sizeof(szIp));
	inet_ntopV4(AF_INET, (void*)&m_ip, szIp, sizeof(szIp));

	apr_status_t ret;
	apr_sockaddr_t* serverAddr;
	apr_sockaddr_info_get(&serverAddr, szIp, APR_INET, m_port, 0, m_peer);
	apr_socket_create(&m_socket, serverAddr->family, SOCK_STREAM, APR_PROTO_TCP, m_peer);
	apr_socket_opt_set(m_socket, APR_SO_NONBLOCK, 0);
	ret = apr_socket_connect(m_socket, serverAddr);
	if(ret != APR_SUCCESS){
		return false;
	}
    apr_interval_time_t to = 1000*1000;
    apr_socket_timeout_set(m_socket, to);
	return Handshake();

}

void SocketStreamer::Close() {
	apr_socket_close(m_socket);
	apr_pool_clear(m_peer);
}

size_t SocketStreamer::Recv() {
	CStdString logMsg;
	m_bytesRead = sizeof(m_buf);
	apr_status_t ret = apr_socket_recv(m_socket, (char*)m_buf, &m_bytesRead);
	if(ret == APR_EOF){
		return -1;
	}

	if (m_bytesRead>0) {
		if(ProcessData() == false){
            return -1;
        }
	}
	return m_bytesRead;
}

bool SocketStreamer::ProcessData() {
	// default do nothing
    return true;
}

bool SocketStreamer::Handshake() {
	return true; // default no handshake
}

void SocketStreamer::Initialize(std::list<CStdString>& targetList, SocketStreamerFactory *factory)
{
	CStdString logMsg;
	SocketStreamerFactory ssf;

	if (factory == NULL) {
		factory = &ssf;
	}

	for (std::list<CStdString>::iterator it = targetList.begin(); it != targetList.end(); it++)
	{
		CStdString target=*it;

		CStdString protocol;
		ChopToken(protocol,"://",target);
		protocol.ToLower();

		if (factory->Accepts(protocol)) {

			SocketStreamer *ss = factory->Create();
			ss->m_logMsg.Format("protocol:%s",protocol);

			if (!ss->Parse(target) || !ss->Spawn()) {
				FLOG_ERROR(ss->m_log,"Target:%s - %s", *it, ss->m_logMsg);
				delete ss;
			}
		}
	}
}

bool SocketStreamer::Spawn() 
{
	CStdString logMsg;

#ifndef UNIT_TESTING //disable if unit testing
	try{
		std::thread httpHandler(ThreadHandler, this);
		httpHandler.detach();
	} catch(const std::exception &ex){
		m_logMsg.Format("Failed to start RunHttpServer thread reason:%s",  ex.what());
		return false;
	}
#endif
	FLOG_INFO(m_log, "Successfully created thread (%s)", m_logMsg);
	return true;
}

