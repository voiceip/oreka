/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */

#include "OrkTrack.h"

std::vector<OrkTrack> OrkTrack::s_trackers;

void OrkTrack::Initialize(const std::list<CStdString>& hostnameList, const CStdString defaultServicename, const int defaultTcpPort, const int defaultTlsPort)
{
	s_trackers.clear();

	for(std::list<CStdString>::const_iterator it = hostnameList.begin(); it != hostnameList.end(); it++) {
		CStdString token = *it;
		OrkTrack tracker;
		size_t pos = std::string::npos;
		bool is_https = false;

		// check to see if https: hostname will start with https://
		if (token.rfind("https://",0) != std::string::npos)
		{
			token.erase(0,8); //strip off the "https://"
			is_https = true;
		}
		else if(token.rfind("http://",0) != std::string::npos)
		{
			token.erase(0,7); //strip off the "http://"
		}
		// chop the servicename, if any
		pos = token.find("/");
		tracker.m_servicename = (pos != std::string::npos) ? token.substr(pos+1) : CStdString(defaultServicename);
		token = token.substr(0,pos);
		
		// chop the port, if any
		pos = token.find(":");
		if (is_https)
			tracker.m_port = (pos != std::string::npos) ? atoi(token.substr(pos+1).c_str()) : defaultTlsPort;
		else
			tracker.m_port = (pos != std::string::npos) ? atoi(token.substr(pos+1).c_str()) : defaultTcpPort;

		token = token.substr(0,pos);

		// remaining bit is the hostname
		tracker.m_hostname = token;
		tracker.m_https = is_https;

		s_trackers.push_back(tracker);
	}
}

