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

void OrkTrack::Initialize(const std::list<CStdString>& hostnameList, const CStdString defaultServicename, const int defaultPort)
{
	s_trackers.clear();

	for(std::list<CStdString>::const_iterator it = hostnameList.begin(); it != hostnameList.end(); it++) {
		CStdString token = *it;
		OrkTrack tracker;
		size_t pos = std::string::npos;

		// chop the servicename, if any
		pos = token.find("/");
		tracker.m_servicename = (pos != std::string::npos) ? token.substr(pos+1) : CStdString(defaultServicename);
		token = token.substr(0,pos);
		
		// chop the port, if any
		pos = token.find(":");
		tracker.m_port = (pos != std::string::npos) ? atoi(token.substr(pos+1).c_str()) : defaultPort;
		token = token.substr(0,pos);

		// remaining bit is the hostname
		tracker.m_hostname = token;

		s_trackers.push_back(tracker);
	}
}

