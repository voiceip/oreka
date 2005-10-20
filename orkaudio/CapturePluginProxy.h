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

#ifndef __CAPTUREPLUGINPROXY_H__
#define __CAPTUREPLUGINPROXY_H__

#include "ace/Singleton.h"
#include "ace/Thread_Mutex.h"
#include "ace/DLL.h"
#include "AudioCapture.h"
#include "AudioCapturePlugin.h"

class CapturePluginProxy
{
public:
	CapturePluginProxy();
	bool Initialize();
	void Run();
	void StartCapture(CStdString& capturePort);
	void StopCapture(CStdString& capturePort);

	static void __CDECL__  AudioChunkCallBack(AudioChunkRef chunkRef, CStdString& capturePort, bool remote = false);
	static void __CDECL__ CaptureEventCallBack(CaptureEventRef eventRef, CStdString& capturePort);
private:
	ConfigureFunction m_configureFunction;
	RegisterCallBacksFunction m_registerCallBacksFunction;
	InitializeFunction m_initializeFunction;
	RunFunction m_runFunction;
	StartCaptureFunction m_startCaptureFunction;
	StopCaptureFunction m_stopCaptureFunction;

	ACE_DLL m_dll;
	bool m_loaded;
};

typedef ACE_Singleton<CapturePluginProxy, ACE_Thread_Mutex> CapturePluginProxySingleton;

#endif

