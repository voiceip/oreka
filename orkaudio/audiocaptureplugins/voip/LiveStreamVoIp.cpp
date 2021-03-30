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
#include "VoIpConfig.h"
#include "VoIpSession.h"
#include <set>

static std::mutex s_mutex;

class LiveStreamVoIp : public OrkSingleton<LiveStreamVoIp>
{
public:
	void StartStream(CStdString &port, CStdString &orkuid, CStdString &nativecallid);
	void EndStream(CStdString &port, CStdString &orkuid, CStdString &nativecallid);
	std::set<std::string> GetStream();
};

#define LiveStreamVoIpSingleton LiveStreamVoIp

void LiveStreamVoIp::StartStream(CStdString &party, CStdString &orkuid, CStdString &nativecallid)
{
	MutexSentinel mutexSentinel(s_mutex);

	if (nativecallid.size())
	{
		orkuid = VoIpSessionsSingleton::instance()->StartStreamNativeCallId(nativecallid);
	}
}

void LiveStreamVoIp::EndStream(CStdString &party, CStdString &orkuid, CStdString &nativecallid)
{
	MutexSentinel mutexSentinel(s_mutex);

	if (nativecallid.size())
	{
		orkuid = VoIpSessionsSingleton::instance()->EndStreamNativeCallId(nativecallid);
	}
}

std::set<std::string> LiveStreamVoIp::GetStream()
{
	MutexSentinel mutexSentinel(s_mutex);

	return VoIpSessionsSingleton::instance()->GetStreamNativeCallId();
}

//================================================================================

void __CDECL__ StartStream(CStdString &party, CStdString &orkuid, CStdString &nativecallid)
{
	LiveStreamVoIpSingleton::instance()->StartStream(party, orkuid, nativecallid);
}

void __CDECL__ EndStream(CStdString &party, CStdString &orkuid, CStdString &nativecallid)
{
	LiveStreamVoIpSingleton::instance()->EndStream(party, orkuid, nativecallid);
}

std::set<std::string> __CDECL__ GetStream()
{
	return LiveStreamVoIpSingleton::instance()->GetStream();
}
