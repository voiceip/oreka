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

#ifndef __AUDIOCAPTUREPLUGINCOMMON_H__
#define __AUDIOCAPTUREPLUGINCOMMON_H__

#include "AudioCapturePlugin.h"

#ifdef WIN32
	#define DLL_EXPORT  __declspec( dllexport )
#else
	#define DLL_EXPORT
#endif

// Shared library exports
extern "C"		// to avoid function name decoration, makes them easier to lookup
{
DLL_EXPORT void __CDECL__  RegisterCallBacks(AudioChunkCallBackFunction, CaptureEventCallBackFunction, OrkLogManager*);
DLL_EXPORT void __CDECL__  Run();
DLL_EXPORT void __CDECL__  Initialize();
DLL_EXPORT void __CDECL__  Shutdown();
DLL_EXPORT void __CDECL__  Configure(DOMNode*);
DLL_EXPORT void __CDECL__  StartCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid, CStdString& side);
DLL_EXPORT void __CDECL__  StopCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid, CStdString& qos);
DLL_EXPORT void __CDECL__  PauseCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid);
DLL_EXPORT void __CDECL__  SetOnHold(CStdString& port, CStdString& orkuid);
DLL_EXPORT void __CDECL__  SetOffHold(CStdString& port, CStdString& orkuid);
DLL_EXPORT void __CDECL__  GetConnectionStatus(CStdString& msg);
}

#endif
