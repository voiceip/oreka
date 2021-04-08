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

#ifndef __LIVESTREAMSESSION_H__
#define __LIVESTREAMSESSION_H__

#include <log4cxx/logger.h>
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "Utils.h"
#include "VoIpSession.h"

using namespace log4cxx;

class LiveStreamSessions : public OrkSingleton<LiveStreamSessions>
{
public:
	LiveStreamSessions();
	bool StartStreamNativeCallId(CStdString &nativecallid);
	bool EndStreamNativeCallId(CStdString &nativecallid);
	std::set<std::string> GetStreamNativeCallId();

private:
	VoIpSessions *voIpSessions;
};

#define LiveStreamSessionsSingleton LiveStreamSessions
#endif
