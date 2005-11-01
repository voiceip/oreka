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
#include "RtpSession.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include <list>
#include "VoIpConfig.h"
#include "ace/OS_NS_arpa_inet.h"

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;

extern VoIpConfigTopObjectRef g_VoIpConfigTopObjectRef;
#define DLLCONFIG g_VoIpConfigTopObjectRef.get()->m_config


RtpSession::RtpSession()
{
	m_lastUpdated = time(NULL);
	m_log = Logger::getLogger("rtpsession");
	m_invitorIp.s_addr = 0;
	m_invitorTcpPort = 0;
	m_inviteeIp.s_addr = 0;
	m_inviteeTcpPort = 0;
	m_direction = CaptureEvent::DirUnkn;
	m_protocol = ProtUnkn;
	m_numRtpPackets = 0;
	m_started = false;
}

void RtpSession::Stop()
{
	if(m_started)
	{
		LOG4CXX_DEBUG(m_log, m_capturePort + " Session stop");
		CaptureEventRef stopEvent(new CaptureEvent);
		stopEvent->m_type = CaptureEvent::EtStop;
		stopEvent->m_timestamp = time(NULL);
		g_captureEventCallBack(stopEvent, m_capturePort);
	}
}

void RtpSession::Start()
{
	m_started = true;
	LOG4CXX_DEBUG(m_log, m_capturePort + " " + ProtocolToString(m_protocol) + " Session start");
	m_rtpRingBuffer.SetCapturePort(m_capturePort);
	CaptureEventRef startEvent(new CaptureEvent);
	startEvent->m_type = CaptureEvent::EtStart;
	startEvent->m_timestamp = time(NULL);
	g_captureEventCallBack(startEvent, m_capturePort);
}

void RtpSession::ProcessMetadataSipIncoming()
{
	m_remoteParty = m_invite->m_from;
	m_localParty = m_invite->m_to;
	m_direction = CaptureEvent::DirIn;
	m_capturePort.Format("%s,%d", ACE_OS::inet_ntoa(m_inviteeIp), m_inviteeTcpPort);
}

void RtpSession::ProcessMetadataSipOutgoing()
{
	m_remoteParty = m_invite->m_to;
	m_localParty = m_invite->m_from;
	m_direction = CaptureEvent::DirOut;
	m_capturePort.Format("%s,%d", ACE_OS::inet_ntoa(m_invitorIp), m_invitorTcpPort);
}

void RtpSession::ProcessMetadataRawRtp(RtpPacketInfoRef& rtpPacket)
{
	bool sourceIsLocal = true;

	if(DLLCONFIG.IsMediaGateway(rtpPacket->m_sourceIp))
	{
		if(DLLCONFIG.IsMediaGateway(rtpPacket->m_destIp))
		{
			// media gateway to media gateway
			sourceIsLocal = false;
		}
		else if (DLLCONFIG.IsPartOfLan(rtpPacket->m_destIp))
		{
			// Media gateway to internal
			sourceIsLocal = false;
		}
		else
		{
			// Media gateway to external
			sourceIsLocal = true;
		}
	}
	else if (DLLCONFIG.IsPartOfLan(rtpPacket->m_sourceIp))
	{
		// source address is internal
		sourceIsLocal = true;
	}
	else
	{
		// source address is external
		sourceIsLocal = false;
	}

	if(sourceIsLocal)
	{
		m_localParty.Format("%s", ACE_OS::inet_ntoa(rtpPacket->m_sourceIp));
		m_remoteParty.Format("%s", ACE_OS::inet_ntoa(rtpPacket->m_destIp));
		m_capturePort.Format("%s,%d", ACE_OS::inet_ntoa(rtpPacket->m_sourceIp), rtpPacket->m_sourcePort);
	}
	else
	{
		m_localParty.Format("%s", ACE_OS::inet_ntoa(rtpPacket->m_destIp));
		m_remoteParty.Format("%s", ACE_OS::inet_ntoa(rtpPacket->m_sourceIp));
		m_capturePort.Format("%s,%d", ACE_OS::inet_ntoa(rtpPacket->m_destIp), rtpPacket->m_destPort);
	}
}

void RtpSession::ProcessMetadataSip(RtpPacketInfoRef& rtpPacket)
{
	bool done = false;

	// work out invitee IP address
	if(rtpPacket->m_sourceIp.s_addr == m_invitorIp.s_addr)
	{
		m_inviteeIp = rtpPacket->m_destIp;
		m_inviteeTcpPort = rtpPacket->m_destPort;
		m_invitorTcpPort = rtpPacket->m_sourcePort;
	}
	else if(rtpPacket->m_destIp.s_addr == m_invitorIp.s_addr)
	{
		m_inviteeIp = rtpPacket->m_sourceIp;
		m_inviteeTcpPort = rtpPacket->m_sourcePort;
		m_invitorTcpPort = rtpPacket->m_destPort;
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
			ProcessMetadataSipIncoming();
		}	
	}
	else if (DLLCONFIG.IsPartOfLan(m_invitorIp))
	{
		ProcessMetadataSipOutgoing();
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
			ProcessMetadataSipIncoming();
		}
		else
		{
			// SIP invitee IP address is an outside IP address
			ProcessMetadataSipOutgoing();
		}
	}
}

void RtpSession::ProcessMetadataSkinny(RtpPacketInfoRef& rtpPacket)
{
	// In skinny, we know that ipAndPort are those from the CallManager.
	// However, what we want as a capture port are IP+Port of the phone
	m_capturePort.Format("%s,%u", ACE_OS::inet_ntoa(rtpPacket->m_sourceIp), rtpPacket->m_sourcePort);
	if(m_capturePort.Equals(m_ipAndPort))
	{
		m_capturePort.Format("%s,%u", ACE_OS::inet_ntoa(rtpPacket->m_destIp), rtpPacket->m_destPort);
	}
}


void RtpSession::ReportMetadata()
{
	// report Local party
	CaptureEventRef event(new CaptureEvent());
	event->m_type = CaptureEvent::EtLocalParty;
	event->m_value = m_localParty;
	g_captureEventCallBack(event, m_capturePort);

	// Report remote party
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtRemoteParty;
	event->m_value = m_remoteParty;
	g_captureEventCallBack(event, m_capturePort);

	// report direction
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtDirection;
	event->m_value = CaptureEvent::DirectionToString(m_direction);
	g_captureEventCallBack(event, m_capturePort);
}


void RtpSession::AddRtpPacket(RtpPacketInfoRef& rtpPacket)
{
	// if first RTP packet, start session
	if(m_lastRtpPacket.get() == NULL)
	{
		if(m_protocol == ProtRawRtp)
		{
			ProcessMetadataRawRtp(rtpPacket);
		}
		else if(m_protocol == ProtSip)
		{
			ProcessMetadataSip(rtpPacket);
			Start();
			ReportMetadata();
		}
		else if(m_protocol == ProtSkinny)
		{
			ProcessMetadataSkinny(rtpPacket);
			Start();
			ReportMetadata();
		}
	}
	m_lastRtpPacket = rtpPacket;
	m_numRtpPackets++;

	if(m_protocol == ProtRawRtp && m_numRtpPackets == 50)
	{
		// We've got 50 RTP packets, this should be a "real" raw RTP session, not a leftover from a SIP session
		Start();
		ReportMetadata();
	}

	// send audio buffer
	//AudioChunkRef chunkRef(new AudioChunk);
	//chunkRef->SetBuffer(rtpPacket->m_payload, rtpPacket->m_payloadSize, AudioChunk::AlawAudio);
	//g_audioChunkCallBack(chunkRef, m_ipAndPort);
	
	if(m_started)
	{
		m_rtpRingBuffer.AddRtpPacket(rtpPacket);
		m_lastUpdated = time(NULL);
	}
}


void RtpSession::ReportSipInvite(SipInviteInfoRef& invite)
{
	m_invite = invite;
	m_invitorIp = invite->m_fromIp;
}

int RtpSession::ProtocolToEnum(CStdString& protocol)
{
	int protocolEnum = ProtUnkn;
	if(protocol.CompareNoCase(PROT_RAW_RTP) == 0)
	{
		protocolEnum = ProtRawRtp;
	}
	else if (protocol.CompareNoCase(PROT_SIP) == 0)
	{
		protocolEnum = ProtSip;
	}
	else if (protocol.CompareNoCase(PROT_SKINNY) == 0)
	{
		protocolEnum = ProtSkinny;
	}
	return protocolEnum;
}

CStdString RtpSession::ProtocolToString(int protocolEnum)
{
	CStdString protocolString;
	switch (protocolEnum)
	{
	case ProtRawRtp:
		protocolString = PROT_RAW_RTP;
		break;
	case ProtSip:
		protocolString = PROT_SIP;
		break;
	case ProtSkinny:
		protocolString = PROT_SKINNY;
		break;
	default:
		protocolString = PROT_UNKN;
	}
	return protocolString;
}

//=====================================================================
RtpSessions::RtpSessions()
{
	m_log = Logger::getLogger("rtpsessions");
}


void RtpSessions::ReportSipInvite(SipInviteInfoRef& invite)
{
	CStdString ipAndPort = CStdString(ACE_OS::inet_ntoa(invite->m_fromIp)) + "," + invite->m_fromRtpPort;
	std::map<CStdString, RtpSessionRef>::iterator pair;
	
	pair = m_byIpAndPort.find(ipAndPort);
	if (pair != m_byIpAndPort.end())
	{
		// A session exists ont the same IP+port, stop old session
		RtpSessionRef session = pair->second;
		Stop(session);
	}
	pair = m_byCallId.find(invite->m_callId);
	if (pair != m_byCallId.end())
	{
		// A session exists ont the same CallId, stop old session
		RtpSessionRef session = pair->second;
		Stop(session);
	}

	// create new session and insert into both maps
	RtpSessionRef session(new RtpSession());
	session->m_ipAndPort = ipAndPort;
	session->m_callId = invite->m_callId;
	session->m_protocol = RtpSession::ProtSip;
	session->ReportSipInvite(invite);
	m_byIpAndPort.insert(std::make_pair(session->m_ipAndPort, session));
	m_byCallId.insert(std::make_pair(session->m_callId, session));
}

void RtpSessions::ReportSipBye(SipByeInfo bye)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	pair = m_byCallId.find(bye.m_callId);

	if (pair != m_byCallId.end())
	{
		// Session found: stop it
		RtpSessionRef session = pair->second;
		Stop(session);
	}
}

void RtpSessions::ReportSkinnyCallInfo(SkCallInfoStruct* callInfo)
{
	CStdString callId = IntToString(callInfo->callId);
	std::map<CStdString, RtpSessionRef>::iterator pair;
	pair = m_byCallId.find(callId);
	if (pair != m_byCallId.end())
	{
		// A session exists ont the same CallId, stop old session
		RtpSessionRef session = pair->second;
		Stop(session);
	}

	// create new session and insert into both maps
	RtpSessionRef session(new RtpSession());
	session->m_callId = callId;
	session->m_protocol = RtpSession::ProtSkinny;
	switch(callInfo->callType)
	{
	case SKINNY_CALL_TYPE_INBOUND:
		session->m_localParty = callInfo->calledParty;
		session->m_remoteParty = callInfo->callingParty;
		session->m_direction = CaptureEvent::DirIn;
		break;
	case SKINNY_CALL_TYPE_OUTBOUND:
		session->m_localParty = callInfo->callingParty;
		session->m_remoteParty = callInfo->calledParty;
		session->m_direction = CaptureEvent::DirOut;
		break;
	}
	m_byCallId.insert(std::make_pair(session->m_callId, session));
}

void RtpSessions::ReportSkinnyStartMediaTransmission(SkStartMediaTransmissionStruct* startMedia)
{
	// Lookup by callId
	CStdString callId = IntToString(startMedia->conferenceId);
	std::map<CStdString, RtpSessionRef>::iterator pair;
	pair = m_byCallId.find(callId);

	if (pair != m_byCallId.end())
	{
		// Session found
		RtpSessionRef session = pair->second;

		if(session->m_ipAndPort.size() == 0)
		{
			CStdString ipAndPort;
			ipAndPort.Format("%s,%u", ACE_OS::inet_ntoa(startMedia->remoteIpAddr), startMedia->remoteTcpPort);
			
			pair = m_byIpAndPort.find(ipAndPort);
			if (pair != m_byIpAndPort.end())
			{
				// A session exists ont the same IP+port, stop old session
				RtpSessionRef session = pair->second;
				Stop(session);
			}
			session->m_ipAndPort = ipAndPort;
			m_byIpAndPort.insert(std::make_pair(session->m_ipAndPort, session));
		}
		else
		{
			// The session has already had a StartMediaTransmission message.
		}
	}
}

void RtpSessions::ReportSkinnyStopMediaTransmission(SkStopMediaTransmissionStruct* stopMedia)
{
	CStdString callId = IntToString(stopMedia->conferenceId);

	std::map<CStdString, RtpSessionRef>::iterator pair;
	pair = m_byCallId.find(callId);

	if (pair != m_byCallId.end())
	{
		// Session found: stop it
		RtpSessionRef session = pair->second;
		Stop(session);
	}
}

void RtpSessions::Stop(RtpSessionRef& session)
{
	session->Stop();
	if(session->m_ipAndPort.size() > 0)
	{
		m_byIpAndPort.erase(session->m_ipAndPort);
	}
	if(session->m_callId.size() > 0)
	{
		m_byCallId.erase(session->m_callId);
	}
}


void RtpSessions::ReportRtpPacket(RtpPacketInfoRef& rtpPacket)
{
	// Does a session exist with this source Ip+Port
	RtpSessionRef session;
	CStdString port = IntToString(rtpPacket->m_sourcePort);
	CStdString ipAndPort = CStdString(ACE_OS::inet_ntoa(rtpPacket->m_sourceIp)) + "," + port;
	std::map<CStdString, RtpSessionRef>::iterator pair;

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
		else
		{
			// create new Raw RTP session and insert into IP+Port map
			RtpSessionRef session(new RtpSession());
			session->m_protocol = RtpSession::ProtRawRtp;
			session->m_ipAndPort = ipAndPort;
			m_byIpAndPort.insert(std::make_pair(ipAndPort, session));
		}
	}
	if (!session.get() == NULL)
	{
		// Found a session give it the RTP packet info
		session->AddRtpPacket(rtpPacket);
	}
}

void RtpSessions::Hoover(time_t now)
{
	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, "Hoover - check " + numSessions + " sessions time:" + IntToString(now));

	// Go round the ipAndPort session index and find inactive sessions
	std::map<CStdString, RtpSessionRef>::iterator pair;
	std::list<RtpSessionRef> toDismiss;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end(); pair++)
	{
		RtpSessionRef session = pair->second;
		if((now - session->m_lastUpdated) > 10)
		{
			toDismiss.push_back(session);
		}
	}

	// discard inactive sessions
	for (std::list<RtpSessionRef>::iterator it = toDismiss.begin(); it != toDismiss.end() ; it++)
	{
		RtpSessionRef session = *it;
		LOG4CXX_DEBUG(m_log, session->m_ipAndPort + " Expired");
		Stop(session);
	}

	// Go round the callId session index and find inactive sessions
	toDismiss.clear();
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		RtpSessionRef session = pair->second;
		if((now - session->m_lastUpdated) > 10)
		{
			toDismiss.push_back(session);
		}
	}

	// discard inactive sessions
	for (std::list<RtpSessionRef>::iterator it2 = toDismiss.begin(); it2 != toDismiss.end() ; it2++)
	{
		RtpSessionRef session = *it2;
		LOG4CXX_DEBUG(m_log, session->m_ipAndPort + " Expired");
		Stop(session);
	}
}

