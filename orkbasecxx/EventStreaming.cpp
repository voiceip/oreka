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

	TapeMsg* tapeMsg = (TapeMsg*)message.get();
	//For unknown reason, singlelineserialization the message can cause crash since the pointer somehow got modified somewhere else
	//This would lead to some pointed memory address become inaccessible
	//To avoid this, we create a cloned local pointer
	TapeMsgRef cloneTape (new TapeMsg());
	cloneTape->m_recId = tapeMsg->m_recId;
	cloneTape->m_stage = tapeMsg->m_stage;
	cloneTape->m_timestamp = tapeMsg->m_timestamp;
	cloneTape->m_fileName = tapeMsg->m_fileName;
	cloneTape->m_capturePort = tapeMsg->m_capturePort;
	cloneTape->m_localParty = tapeMsg->m_localParty;
	cloneTape->m_localEntryPoint = tapeMsg->m_localEntryPoint;
	cloneTape->m_remoteParty = tapeMsg->m_remoteParty;
	cloneTape->m_direction = tapeMsg->m_direction;
	cloneTape->m_audioKeepDirection = tapeMsg->m_audioKeepDirection;
	cloneTape->m_serviceName = tapeMsg->m_serviceName;
	cloneTape->m_localIp = tapeMsg->m_localIp;
	cloneTape->m_remoteIp = tapeMsg->m_remoteIp;
	cloneTape->m_localMac = tapeMsg->m_localMac;
	cloneTape->m_remoteMac = tapeMsg->m_remoteMac;
	cloneTape->m_nativeCallId = tapeMsg->m_nativeCallId;
	cloneTape->m_duration = tapeMsg->m_duration;
	cloneTape->m_onDemand = tapeMsg->m_onDemand;
	std::map<CStdString, CStdString>::iterator it;
	for(it = tapeMsg->m_tags.begin(); it != tapeMsg->m_tags.end(); it++)
	{
		cloneTape->m_tags.insert(std::make_pair(it->first, it->second));
	}

	m_messages.push_back(cloneTape);
	m_semaphore.release();
}

void EventStreamingSession::AddAddTagMessage(MessageRef& message)
{
	if(m_messages.size() > 10000)
	{
		m_messages.pop_front();
	}
	AddTagMsg* tagMsg = (AddTagMsg*)message.get();
	AddTagMsgRef cloneTag (new AddTagMsg());
	cloneTag->m_party = tagMsg->m_party;
	cloneTag->m_orkuid = tagMsg->m_orkuid;
	cloneTag->m_tagType = tagMsg->m_tagType;
	cloneTag->m_tagText = tagMsg->m_tagText;
	cloneTag->m_dtmfOffsetMs = tagMsg->m_dtmfOffsetMs;
	cloneTag->m_success = tagMsg->m_success;
	m_messages.push_back(cloneTag);
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

void EventStreaming::AddAddTagMessage(MessageRef& message)
{
	MutexSentinel sentinel(m_mutex);

	for(std::list<EventStreamingSessionRef>::iterator it = m_sessions.begin(); it != m_sessions.end(); it++)
	{
		(*it)->AddAddTagMessage(message);
	}
}


