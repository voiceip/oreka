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
#include "LogManager.h"
#include "Filter.h"
#include <math.h>
#include "Utils.h"
#include <queue>
#include <list>
#include "shared_ptr.h"
#include "ace/Singleton.h"
#include "ace/Thread_Mutex.h"
#include "ace/Thread_Semaphore.h"
#include "AudioCapture.h"
#include "ConfigManager.h"
#include "CapturePluginProxy.h"
#include "AudioTape.h"
#include "dll.h"
#include "messages/AddTagMsg.h"

//==========================================================

class DLL_IMPORT_EXPORT_ORKBASE EventStreamingSession
{
public:
	EventStreamingSession();

	void AddMessage(MessageRef message);
	void GetTapeMessage(MessageRef& message);
	int GetNumMessages();
	void WaitForMessages();
private:
	std::list<MessageRef> m_messages;
	ACE_Thread_Mutex m_mutex;
	ACE_Thread_Semaphore m_semaphore;
};
typedef oreka::shared_ptr<EventStreamingSession> EventStreamingSessionRef;

//==========================================================

class DLL_IMPORT_EXPORT_ORKBASE EventStreaming
{
public:
	EventStreaming();

	void GetLiveSessions(std::list<EventStreamingSessionRef>& sessions);
	void AddSession(EventStreamingSessionRef& session);
	void RemoveSession(EventStreamingSessionRef& session);
	int GetNumSessions();
	CStdString GetNewSessionId();
	void AddMessage(MessageRef message);

private:
	AlphaCounter m_alphaCounter;
	ACE_Thread_Mutex m_mutex;
	std::list<EventStreamingSessionRef> m_sessions;
};
typedef ACE_Singleton<EventStreaming, ACE_Thread_Mutex> EventStreamingSingleton;

//==========================================================

