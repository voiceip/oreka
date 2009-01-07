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
#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "EventStreaming.h"
#include "ConfigManager.h"

//==========================================================

EventStreamingSession::EventStreamingSession()
{
}

void EventStreamingSession::AddTapeMessage(MessageRef& message)
{
	if(m_messages.size() > 10000)
	{
		m_messages.pop_front();
	}
	m_messages.push_back(message);
	m_semaphore.release();
}

void EventStreamingSession::GetTapeMessage(MessageRef& message)
{
	MutexSentinel mutexSentinel(m_mutex);

	if(m_messages.size() > 0)
	{
		message = m_messages.front();
		m_messages.pop_front();
	}
}

int EventStreamingSession::GetNumMessages()
{
	MutexSentinel mutexSentinel(m_mutex);

	return m_messages.size();
}

void EventStreamingSession::WaitForMessages()
{
	m_semaphore.acquire();
}

//==============================================

EventStreaming::EventStreaming()
{
}

void EventStreaming::GetLiveSessions(std::list<EventStreamingSessionRef>& sessions)
{
	MutexSentinel sentinel(m_mutex);

	for(std::list<EventStreamingSessionRef>::iterator it = m_sessions.begin(); it != m_sessions.end(); it++)
	{
		EventStreamingSessionRef session = *it;

		sessions.push_back(session);
	}
}

void EventStreaming::AddSession(EventStreamingSessionRef& session)
{
	MutexSentinel sentinel(m_mutex);
	m_sessions.push_back(session);
}

void EventStreaming::RemoveSession(EventStreamingSessionRef& session)
{
	MutexSentinel sentinel(m_mutex);
	m_sessions.remove(session);
}

int EventStreaming::GetNumSessions()
{
	MutexSentinel sentinel(m_mutex);
	return m_sessions.size();
}

CStdString EventStreaming::GetNewSessionId()
{
	return m_alphaCounter.GetNext();
}

void EventStreaming::AddTapeMessage(MessageRef& message)
{
	MutexSentinel sentinel(m_mutex);

	for(std::list<EventStreamingSessionRef>::iterator it = m_sessions.begin(); it != m_sessions.end(); it++)
	{
		(*it)->AddTapeMessage(message);
	}
}

