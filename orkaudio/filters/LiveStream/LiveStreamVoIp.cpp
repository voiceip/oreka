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
#pragma warning(disable : 4786) // disables truncated symbols in browse-info warning
#define _WINSOCKAPI_			// prevents the inclusion of winsock.h

#ifndef WIN32
#include "sys/socket.h"
#include <unistd.h>
#endif

#ifdef WIN32
#define snprintf _snprintf
#endif

#include <list>
#include <stdio.h>
#include <string.h>
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "Utils.h"
#include "LiveStreamSession.h"
#include <set>

static std::mutex s_mutex;

class LiveStreamVoIp : public OrkSingleton<LiveStreamVoIp>
{
public:
	bool StartStream(CStdString &nativecallid);
	bool EndStream(CStdString &nativecallid);
	std::set<std::string> GetStream();
};

#define LiveStreamVoIpSingleton LiveStreamVoIp

bool LiveStreamVoIp::StartStream(CStdString &nativecallid)
{
	MutexSentinel mutexSentinel(s_mutex);

	if (nativecallid.size())
	{
		return LiveStreamSessionsSingleton::instance()->StartStreamNativeCallId(nativecallid);
	}
	return false;
}

bool LiveStreamVoIp::EndStream(CStdString &nativecallid)
{
	MutexSentinel mutexSentinel(s_mutex);

	if (nativecallid.size())
	{
		return LiveStreamSessionsSingleton::instance()->EndStreamNativeCallId(nativecallid);
	}
	return false;
}

std::set<std::string> LiveStreamVoIp::GetStream()
{
	MutexSentinel mutexSentinel(s_mutex);

	return LiveStreamSessionsSingleton::instance()->GetStreamNativeCallId();
}

//================================================================================

bool __CDECL__ StartStream(CStdString &nativecallid)
{
	return LiveStreamVoIpSingleton::instance()->StartStream(nativecallid);
}

bool __CDECL__ EndStream(CStdString &nativecallid)
{
	return LiveStreamVoIpSingleton::instance()->EndStream(nativecallid);
}

std::set<std::string> __CDECL__ GetStream()
{
	return LiveStreamVoIpSingleton::instance()->GetStream();
}
