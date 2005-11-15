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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Utils.h"
#include "AudioCapturePluginCommon.h"


AudioChunkCallBackFunction g_audioChunkCallBack;
CaptureEventCallBackFunction g_captureEventCallBack;
OrkLogManager* g_logManager;

void __CDECL__ RegisterCallBacks(AudioChunkCallBackFunction audioCallBack, CaptureEventCallBackFunction captureEventCallBack, OrkLogManager* logManager)
{
	g_audioChunkCallBack = audioCallBack;
	g_captureEventCallBack = captureEventCallBack;
	g_logManager = logManager;
}
