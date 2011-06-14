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

class DLL_IMPORT_EXPORT_ORKBASE CapturePluginProxy
{
public:
	static bool Initialize();
	static CapturePluginProxy* Singleton();

	void Run();
	void Shutdown();
	void StartCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid, CStdString& side);
	void PauseCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid);
	void StopCapture(CStdString& party, CStdString& orkUid, CStdString& nativecallid, CStdString& qos);
	void SetOnHold(CStdString& party, CStdString& orkuid);
	void SetOffHold(CStdString& party, CStdString& orkuid);
	void GetConnectionStatus(CStdString& msg);

	static void __CDECL__  AudioChunkCallBack(AudioChunkRef chunkRef, CStdString& capturePort);
	static void __CDECL__ CaptureEventCallBack(CaptureEventRef eventRef, CStdString& capturePort);
private:
	CapturePluginProxy();
	static CapturePluginProxy* m_singleton;	
	bool Init();

	ConfigureFunction m_configureFunction;
	RegisterCallBacksFunction m_registerCallBacksFunction;
	InitializeFunction m_initializeFunction;
	RunFunction m_runFunction;
	StartCaptureFunction m_startCaptureFunction;
	StopCaptureFunction m_stopCaptureFunction;
	PauseCaptureFunction m_pauseCaptureFunction;
	SetOnHoldFunction m_setOnHoldFunction;
	SetOffHoldFunction m_setOffHoldFunction;
	GetConnectionStatusFunction m_GetConnectionStatusFunction;

	ACE_DLL m_dll;
	bool m_loaded;
};

//typedef ACE_Singleton<CapturePluginProxy, ACE_Thread_Mutex> CapturePluginProxySingleton;

#endif

