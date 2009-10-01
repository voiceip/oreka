/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */

#include "OrkTrack.h"
#include "LogManager.h"
#include "ace/Thread_Manager.h"
#include "messages/InitMsg.h"
#include "ConfigManager.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

void OrkTrack::Initialize()
{
	CStdString logMsg;

	for(std::list<CStdString>::iterator it = CONFIG.m_trackerHostname.begin(); it != CONFIG.m_trackerHostname.end(); it++)
	{
		CStdString trackerHostname = *it;
		OrkTrackHost *oth = (OrkTrackHost *)malloc(sizeof(OrkTrackHost));

		memset(oth, 0, sizeof(OrkTrackHost));
		snprintf(oth->m_serverHostname, sizeof(oth->m_serverHostname), "%s", trackerHostname.c_str());
		oth->m_serverPort = CONFIG.m_trackerTcpPort;

		if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(OrkTrack::Run), (void*)oth, THR_DETACHED))
		{
			logMsg.Format("OrkTrack::Initialize(): Failed to start thread for %s,%d", oth->m_serverHostname, oth->m_serverPort);
			LOG4CXX_WARN(LOG.rootLog, logMsg);
			free(oth);
		}
	}
}

void OrkTrack::Run(void* args)
{
	OrkTrackHostRef hostRef;
	OrkTrackHost *pHost = (OrkTrackHost *)args;
	InitMsgRef msgRef(new InitMsg());
	char host[255];
	time_t reportErrorLastTime = 0;
	CStdString serverHostname;
	CStdString logMsg;

	ACE_OS::hostname(host, sizeof(host));

	hostRef.reset(pHost);
	serverHostname = hostRef->m_serverHostname;
	msgRef->m_name.Format("orkaudio-%s", host);
	msgRef->m_hostname = host;
	msgRef->m_type = "A";
	msgRef->m_tcpPort = 59140;
	msgRef->m_contextPath = "/audio";
	msgRef->m_absolutePath = CONFIG.m_audioOutputPath;

	OrkHttpSingleLineClient c;
	SimpleResponseMsg response;
	CStdString msgAsSingleLineString = msgRef->SerializeSingleLine();
	bool success = false;

	while (!success)
	{
		if (c.Execute((SyncMessage&)(*msgRef.get()), (AsyncMessage&)response, serverHostname, hostRef->m_serverPort, CONFIG.m_trackerServicename, CONFIG.m_clientTimeout))
		{
			success = true;
			logMsg.Format("OrkTrack::Run(): [%s,%d] success:%s comment:%s", hostRef->m_serverHostname, hostRef->m_serverPort, (response.m_success == true ? "true" : "false"), response.m_comment);
			LOG4CXX_INFO(LOG.rootLog, logMsg);
		}
		else
		{
			if(((time(NULL) - reportErrorLastTime) > 60))
			{
				reportErrorLastTime = time(NULL);
				logMsg.Format("OrkTrack::Run(): [%s,%d] Could not contact orktrack", hostRef->m_serverHostname, hostRef->m_serverPort);
				LOG4CXX_WARN(LOG.rootLog, logMsg);
			}

			ACE_OS::sleep(CONFIG.m_clientTimeout + 10);
		}
	}
}
