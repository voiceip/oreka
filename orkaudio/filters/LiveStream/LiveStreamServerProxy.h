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

#ifndef __LIVESTREAMSERVERPROXY_H__
#define __LIVESTREAMSERVERPROXY_H__

#include "AudioCapture.h"
#include "AudioCapturePlugin.h"
#include "Utils.h"
#include <set>

class DLL_IMPORT_EXPORT_ORKBASE LiveStreamServerProxy
{
public:
	static bool Initialize();
	static LiveStreamServerProxy *Singleton();

	void Run();
	void Shutdown();

	bool StartStream(CStdString &nativecallid);
	bool EndStream(CStdString &nativecallid);
	std::set<std::string> GetStream();

private:
	LiveStreamServerProxy();
	static LiveStreamServerProxy *m_singleton;
	bool Init();

	InitializeFunction m_initializeFunction;
	RunFunction m_runFunction;
	StartStreamFunction m_startStreamFunction;
	EndStreamFunction m_endStreamFunction;
	GetStreamFunction m_getStreamFunction;
	apr_dso_handle_t *m_dsoHandle;
	bool m_loaded;
};

#endif
