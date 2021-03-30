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

#include "AudioCapture.h"
#include "AudioCapturePlugin.h"
#include "Utils.h"

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
	void ProcessMetadataMsg(SyncMessage* msg);

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
	ProcessMetadataMsgFunction m_ProcessMetadataMsgFunction;
	apr_dso_handle_t *m_dsoHandle;
	bool m_loaded;
};

#endif

