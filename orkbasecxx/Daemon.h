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

#ifndef __DAEMON_H__
#define __DAEMON_H__

#include "StdString.h"
#include "ace/Singleton.h"
#include "ace/Thread_Mutex.h"

typedef void (*DaemonHandler)(void);

class DLL_IMPORT_EXPORT_ORKBASE Daemon
{
public:
	static void Initialize(CStdString serviceName, DaemonHandler runHandler, DaemonHandler stopHandler);
	static Daemon* Singleton();
	void Start();
	void Stop();
	void Install();
	void Uninstall();
	bool IsStopping();

	void SetShortLived();
	bool GetShortLived();

private:
	Daemon();
	static Daemon* m_singleton;

#ifdef WIN32
	static void WINAPI Run( DWORD /*argc*/, TCHAR* /*argv*/[] );
#else
	static void Run();
#endif

	DaemonHandler m_runHandler;
	DaemonHandler m_stopHandler;
	CStdString m_serviceName;

	bool m_stopping;
	bool m_shortLived;
};

//typedef ACE_Singleton<Daemon, ACE_Thread_Mutex> DaemonSingleton;

#endif

