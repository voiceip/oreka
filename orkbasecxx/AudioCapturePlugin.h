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

#ifndef __AUDIOCAPTUREPLUGIN_H__
#define __AUDIOCAPTUREPLUGIN_H__

#include "dll.h"
#include "OrkBase.h"
#include "AudioCapture.h"
#include "LogManager.h"
#include "xercesc/dom/DOMNode.hpp"

using namespace XERCES_CPP_NAMESPACE;

//#ifdef WIN32
//#define __CDECL__ __cdecl
//#else
//#define __CDECL__
//#endif

#define AUDIO_CAPTURE_PLUGIN_INTERFACE_VERSION 1

// Callback definitions
typedef void (__CDECL__*AudioChunkCallBackFunction)(AudioChunkRef, CStdString& capturePort);
typedef void (__CDECL__*CaptureEventCallBackFunction)(CaptureEventRef, CStdString& capturePort);


// Exported functions definitions
typedef void (__CDECL__* RegisterCallBacksFunction)(AudioChunkCallBackFunction, CaptureEventCallBackFunction, OrkLogManager*);
typedef void (__CDECL__* InitializeFunction)();
typedef void (__CDECL__* RunFunction)();
typedef void (__CDECL__* ShutdownFunction)();
typedef void (__CDECL__* ConfigureFunction)(DOMNode*);
typedef void (__CDECL__* StartCaptureFunction)(CStdString& port, CStdString& orkUid, CStdString& nativecallid, CStdString& side);
typedef void (__CDECL__* StopCaptureFunction)(CStdString& port, CStdString& orkUid, CStdString& nativecallid, CStdString& qos);
typedef void (__CDECL__* PauseCaptureFunction)(CStdString& port, CStdString& orkUid, CStdString& nativecallid);
typedef void (__CDECL__* SetOnHoldFunction)(CStdString& port, CStdString& orkUid);
typedef void (__CDECL__* SetOffHoldFunction)(CStdString& port, CStdString& orkUid);
typedef void (__CDECL__* GetConnectionStatusFunction)(CStdString& msg);


#endif

