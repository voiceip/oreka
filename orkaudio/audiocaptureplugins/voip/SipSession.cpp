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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Utils.h"
#include "SipSession.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include <list>
#include "VoIpConfig.h"
#include "ace/OS_NS_arpa_inet.h"

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;

extern VoIpConfigTopObjectRef g_VoIpConfigTopObjectRef;
#define DLLCONFIG g_VoIpConfigTopObjectRef.get()->m_config


SipSession::SipSession()
{
	m_lastUpdated = time(NULL);
	m_log = Logger::getLogger("sipsession");
	m_invitorIp.s_addr = 0;
	m_inviteeIp.s_addr = 0;
	m_direction = CaptureEvent::DirUnkn;
}

void SipSession::Stop()
{
	LOG4CXX_DEBUG(m_log, m_ipAndPort + " SIP Session stop");
	CaptureEventRef stopEvent(new CaptureEvent);
	stopEvent->m_type = CaptureEvent::EtStop;
	stopEvent->m_timestamp = time(NULL);
	g_captureEventCallBack(stopEvent, m_ipAndPort);
}

void SipSession::Start()
{
	LOG4CXX_DEBUG(m_log, m_ipAndPort + " SIP Session start");
	m_rtpRingBuffer.SetCapturePort(m_ipAndPort);
	CaptureEventRef startEvent(new CaptureEvent);
	startEvent->m_type = CaptureEvent::EtStart;
	startEvent->m_timestamp = time(NULL);
	g_captureEventCallBack(startEvent, m_ipAndPort);
}

void SipSession::ProcessMetadataIncoming()
{
	m_remoteParty = m_invite->m_from;
	m_localParty = m_invite->m_to;
	m_direction = CaptureEvent::DirIn;
}

void SipSession::ProcessMetadataOutgoing()
{
	m_remoteParty = m_invite->m_to;
	m_localParty = m_invite->m_from;
	m_direction = CaptureEvent::DirOut;
}

void SipSession::ProcessMetadata(RtpPacketInfoRef& rtpPacket)
{
	bool done = false;

	// work out invitee IP address
	if(rtpPacket->m_sourceIp.s_addr == m_invitorIp.s_addr)
	{
		m_inviteeIp = rtpPacket->m_destIp;
	}
	else if(rtpPacket->m_destIp.s_addr == m_invitorIp.s_addr)
	{
		m_inviteeIp = rtpPacket->m_sourceIp;
	}
	else
	{
		LOG4CXX_DEBUG(m_log, m_ipAndPort + " alien RTP packet");
	}

	// work out capture port and direction
	if(DLLCONFIG.IsMediaGateway(m_invitorIp))
	{
		if(DLLCONFIG.IsMediaGateway(m_inviteeIp))
		{
			// dismiss
		}
		if(DLLCONFIG.IsPartOfLan(m_inviteeIp))
		{
			ProcessMetadataIncoming();
		}	
	}
	else if (DLLCONFIG.IsPartOfLan(m_invitorIp))
	{
		ProcessMetadataOutgoing();
	}
	else
	{
		// SIP invitor IP address is an outside IP address
		if(DLLCONFIG.IsMediaGateway(m_inviteeIp))
		{
			// dismiss
		}
		else if(DLLCONFIG.IsPartOfLan(m_inviteeIp))
		{
			ProcessMetadataIncoming();
		}
		else
		{
			// SIP invitee IP address is an outside IP address
			ProcessMetadataOutgoing();
		}
	}
	ReportMetadata();
}

void SipSession::ReportMetadata()
{
	// report Local party
	CaptureEventRef event(new CaptureEvent());
	event->m_type = CaptureEvent::EtLocalParty;
	event->m_value = m_localParty;
	g_captureEventCallBack(event, m_ipAndPort);

	// Report remote party
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtRemoteParty;
	event->m_value = m_remoteParty;
	g_captureEventCallBack(event, m_ipAndPort);

	// report direction
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtDirection;
	event->m_value = CaptureEvent::DirectionToString(m_direction);
	g_captureEventCallBack(event, m_ipAndPort);
}


void SipSession::AddRtpPacket(RtpPacketInfoRef& rtpPacket)
{
	// if first RTP packet, start session
	if(m_lastRtpPacket.get() == NULL)
	{
		Start();
		ProcessMetadata(rtpPacket);
	}
	m_lastRtpPacket = rtpPacket;

	// send audio buffer
	//AudioChunkRef chunkRef(new AudioChunk);
	//chunkRef->SetBuffer(rtpPacket->m_payload, rtpPacket->m_payloadSize, AudioChunk::AlawAudio);
	//g_audioChunkCallBack(chunkRef, m_ipAndPort);

	m_rtpRingBuffer.AddRtpPacket(rtpPacket);

	m_lastUpdated = time(NULL);
}


void SipSession::ReportSipInvite(SipInviteInfoRef& invite)
{
	m_invite = invite;
	m_invitorIp = invite->m_fromIp;
}

//=====================================================================
SipSessions::SipSessions()
{
	m_log = Logger::getLogger("sipsessions");
}


void SipSessions::ReportSipInvite(SipInviteInfoRef& invite)
{
	CStdString key = CStdString(ACE_OS::inet_ntoa(invite->m_fromIp)) + "," + invite->m_fromRtpPort;
	std::map<CStdString, SipSessionRef>::iterator pair;
	
	pair = m_byIpAndPort.find(key);

	if (pair != m_byIpAndPort.end())
	{
		// A session exists ont the same IP+port, stop old session
		SipSessionRef session = pair->second;
		Stop(session);
	}
	// create new session and insert into both maps
	SipSessionRef session(new SipSession());
	session->m_ipAndPort = key;
	session->ReportSipInvite(invite);
	m_byCallId.insert(std::make_pair(invite->m_callId, session));
	m_byIpAndPort.insert(std::make_pair(key, session));
}

void SipSessions::ReportSipBye(SipByeInfo bye)
{
	std::map<CStdString, SipSessionRef>::iterator pair;
	pair = m_byCallId.find(bye.m_callId);

	if (pair != m_byCallId.end())
	{
		// Session found: stop it
		SipSessionRef session = pair->second;
		Stop(session);
	}
}

void SipSessions::Stop(SipSessionRef& session)
{
	session->Stop();
	m_byIpAndPort.erase(session->m_ipAndPort);
	m_byCallId.erase(session->m_invite->m_callId);
}


void SipSessions::ReportRtpPacket(RtpPacketInfoRef& rtpPacket)
{
	// Does a session exist with this source Ip+Port
	SipSessionRef session;
	CStdString port = IntToString(rtpPacket->m_sourcePort);
	CStdString ipAndPort = CStdString(ACE_OS::inet_ntoa(rtpPacket->m_sourceIp)) + "," + port;
	std::map<CStdString, SipSessionRef>::iterator pair;

	pair = m_byIpAndPort.find(ipAndPort);
	if (pair != m_byIpAndPort.end())
	{
		session = pair->second;
	}	
	else
	{
		// Does a session exist with this destination Ip+Port
		port = IntToString(rtpPacket->m_destPort);
		ipAndPort = CStdString(ACE_OS::inet_ntoa(rtpPacket->m_destIp)) + "," + port;
		pair = m_byIpAndPort.find(ipAndPort);
		if (pair != m_byIpAndPort.end())
		{
			session = pair->second;
		}
	}
	if (!session.get() == NULL)
	{
		// Found a session give it the RTP packet info
		session->AddRtpPacket(rtpPacket);
	}
}

void SipSessions::Hoover(time_t now)
{
	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, "Hoover - check " + numSessions + " sessions time:" + IntToString(now));

	// Go round the sessions and find inactive ones
	std::map<CStdString, SipSessionRef>::iterator pair;
	std::list<SipSessionRef> toDismiss;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end(); pair++)
	{
		SipSessionRef session = pair->second;
		if((now - session->m_lastUpdated) > 10)
		{
			toDismiss.push_back(session);
		}
	}

	// discard inactive sessions
	for (std::list<SipSessionRef>::iterator it = toDismiss.begin(); it != toDismiss.end() ; it++)
	{
		SipSessionRef session = *it;
		LOG4CXX_DEBUG(m_log, session->m_ipAndPort + " Expired");
		Stop(session);
	}
}

