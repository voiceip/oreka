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


RtpSession::RtpSession(CStdString& trackingId)
{
	m_trackingId = trackingId;
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
	m_rtpTimestampCorrectiveOffset = 0;
}

void RtpSession::Stop()
{
	if(m_started)
	{
		LOG4CXX_INFO(m_log, m_trackingId + ": " + m_capturePort + " Session stop");
		CaptureEventRef stopEvent(new CaptureEvent);
		stopEvent->m_type = CaptureEvent::EtStop;
		stopEvent->m_timestamp = time(NULL);
		g_captureEventCallBack(stopEvent, m_capturePort);
	}
}

void RtpSession::Start()
{
	m_started = true;
	LOG4CXX_INFO(m_log,  m_trackingId + ": " + m_capturePort + " " + ProtocolToString(m_protocol) + " Session start");
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
	char szInviteeIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_inviteeIp, szInviteeIp, sizeof(szInviteeIp));
	m_capturePort.Format("%s,%d", szInviteeIp, m_inviteeTcpPort);
}

void RtpSession::ProcessMetadataSipOutgoing()
{
	m_remoteParty = m_invite->m_to;
	m_localParty = m_invite->m_from;
	m_direction = CaptureEvent::DirOut;
	char szInvitorIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_invitorIp, szInvitorIp, sizeof(szInvitorIp));
	m_capturePort.Format("%s,%d", szInvitorIp, m_invitorTcpPort);
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

	char szSourceIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_sourceIp, szSourceIp, sizeof(szSourceIp));
	char szDestIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_destIp, szDestIp, sizeof(szDestIp));

	if(sourceIsLocal)
	{
		m_localParty = szSourceIp;
		m_remoteParty = szDestIp;
		m_capturePort.Format("%s,%d", szSourceIp, rtpPacket->m_sourcePort);
	}
	else
	{
		m_localParty = szDestIp;
		m_remoteParty = szSourceIp;
		m_capturePort.Format("%s,%d", szDestIp, rtpPacket->m_destPort);
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
		LOG4CXX_ERROR(m_log,  m_trackingId + ": " + m_ipAndPort + " alien RTP packet");
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
	// In skinny, we know that ipAndPort are always those of the remote party (other internal phone or gateway).
	// However, what we want as a capture port are IP+Port of the local phone
	char szSourceIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_sourceIp, szSourceIp, sizeof(szSourceIp));
	m_capturePort.Format("%s,%u", szSourceIp, rtpPacket->m_sourcePort);
	if(m_capturePort.Equals(m_ipAndPort))
	{
		char szDestIp[16];
		ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_destIp, szDestIp, sizeof(szDestIp));
		m_capturePort.Format("%s,%u", szDestIp, rtpPacket->m_destPort);
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
	CStdString logMsg;

	if(m_lastRtpPacket.get() == NULL)
	{
		// Until now, we knew the remote IP and port for RTP (at best, as given by SIP invite or Skinny StartMediaTransmission)
		// With the first RTP packet, we can extract local and remote IP and ports for RTP
		// And from that, we can figure out a bit of metadata
		if(m_protocol == ProtRawRtp)
		{
			ProcessMetadataRawRtp(rtpPacket);
		}
		else if(m_protocol == ProtSip)
		{
			ProcessMetadataSip(rtpPacket);
		}
		else if(m_protocol == ProtSkinny)
		{
			ProcessMetadataSkinny(rtpPacket);
		}
	}
	m_lastRtpPacket = rtpPacket;
	m_numRtpPackets++;
	if(m_lastRtpPacketSide1.get() == NULL)
	{
		// First RTP packet for side 1
		m_lastRtpPacketSide1 = rtpPacket;
		if(m_log->isInfoEnabled())
		{
			rtpPacket->ToString(logMsg);
			logMsg =  m_trackingId + ": " + "1st packet s1: " + logMsg;
			LOG4CXX_INFO(m_log, logMsg);
		}
	}
	else
	{
		if(rtpPacket->m_sourceIp.s_addr == m_lastRtpPacketSide1->m_sourceIp.s_addr)
		{
			m_lastRtpPacketSide1 = rtpPacket;
		}
		else
		{
			if(m_lastRtpPacketSide2.get() == NULL)
			{
				// First RTP packet for side 2
				if(m_log->isInfoEnabled())
				{
					rtpPacket->ToString(logMsg);
					logMsg =  m_trackingId + ": " + "1st packet s2: " + logMsg;
					LOG4CXX_INFO(m_log, logMsg);
				}
			}
			m_lastRtpPacketSide2 = rtpPacket;
		}
	}

	// Compute the corrective offset (only if the two streams have greatly different timestamp, eg for Cisco CallManager)
	if(m_rtpTimestampCorrectiveOffset == 0 && m_lastRtpPacketSide2.get() != NULL)
	{
		if (m_lastRtpPacketSide2->m_arrivalTimestamp == m_lastRtpPacketSide1->m_arrivalTimestamp)
		{
			int timestampOffset = m_lastRtpPacketSide2->m_timestamp - m_lastRtpPacketSide1->m_timestamp;
			if(timestampOffset > 8000 || timestampOffset < -8000)	// 1s @ 8KHz
			{
				m_rtpTimestampCorrectiveOffset = timestampOffset;
				if(m_log->isDebugEnabled())
				{
					CStdString timestampOffsetString = IntToString(timestampOffset);
					LOG4CXX_DEBUG(m_log,  m_trackingId + ": " + m_capturePort + ": " + "Applying timestamp corrective offset:" + timestampOffsetString);
				}
			}
		}
	}
	// apply the corrective offset
	if(m_lastRtpPacketSide2.get() != NULL)
	{
		m_lastRtpPacketSide2->m_timestamp = m_lastRtpPacketSide2->m_timestamp - m_rtpTimestampCorrectiveOffset;
	}

	if(m_log->isDebugEnabled())
	{
		CStdString debug;
		debug.Format("%s: %s: Add RTP packet ts:%u arrival:%u", m_trackingId, m_capturePort, rtpPacket->m_timestamp, rtpPacket->m_arrivalTimestamp);
		LOG4CXX_DEBUG(m_log, debug);
	}

	if(		(m_protocol == ProtRawRtp && m_numRtpPackets == 50)	||
			(m_protocol == ProtSkinny && m_numRtpPackets == 2)	||
			(m_protocol == ProtSip && m_numRtpPackets == 2)			)
	{
		// We've got enough packets to start the session.
		// For Raw RTP, the high number is to make sure we have a "real" raw RTP session, not a leftover from a SIP/Skinny session
		Start();
		ReportMetadata();
	}

	if(m_started)
	{
		m_rtpRingBuffer.AddRtpPacket(rtpPacket);
		m_lastUpdated = rtpPacket->m_arrivalTimestamp;
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
	char szFromIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&invite->m_fromIp, szFromIp, sizeof(szFromIp));

	CStdString ipAndPort = CStdString(szFromIp) + "," + invite->m_fromRtpPort;
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
	CStdString trackingId = alphaCounter.GetNext();
	RtpSessionRef session(new RtpSession(trackingId));
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
		// A session exists on the same CallId, stop old session
		RtpSessionRef session = pair->second;
		Stop(session);
	}

	// create new session and insert into the callid map
	CStdString trackingId = alphaCounter.GetNext();
	RtpSessionRef session(new RtpSession(trackingId));
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
			char szRemoteIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&startMedia->remoteIpAddr, szRemoteIp, sizeof(szRemoteIp));
			ipAndPort.Format("%s,%u", szRemoteIp, startMedia->remoteTcpPort);
			
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
	else
	{
		// Discard because we have not seen any CallInfo Message before
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
	int numSessionsFound = 0;
	RtpSessionRef session1;
	RtpSessionRef session2;

	// Add RTP packet to session with matching source or dest IP+Port. 
	// On CallManager there might be two sessions with two different CallIDs for one 
	// phone call, so this RTP packet can potentially be reported to two sessions.

	// Does a session exist with this source Ip+Port
	CStdString port = IntToString(rtpPacket->m_sourcePort);
	char szSourceIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_sourceIp, szSourceIp, sizeof(szSourceIp));
	CStdString ipAndPort = CStdString(szSourceIp) + "," + port;
	std::map<CStdString, RtpSessionRef>::iterator pair;

	pair = m_byIpAndPort.find(ipAndPort);
	if (pair != m_byIpAndPort.end())
	{
		session1 = pair->second;
		if (!session1.get() == NULL)
		{
			// Found a session give it the RTP packet info
			session1->AddRtpPacket(rtpPacket);
			numSessionsFound++;;
		}
	}

	// Does a session exist with this destination Ip+Port
	port = IntToString(rtpPacket->m_destPort);
	char szDestIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_destIp, szDestIp, sizeof(szDestIp));
	ipAndPort = CStdString(szDestIp) + "," + port;

	pair = m_byIpAndPort.find(ipAndPort);
	if (pair != m_byIpAndPort.end())
	{
		session2 = pair->second;
		if (!session2.get() == NULL)
		{
			// Found a session give it the RTP packet info
			session2->AddRtpPacket(rtpPacket);
			numSessionsFound++;
		}
	}

	if(numSessionsFound == 2)
	{
		// Need to "merge" the two sessions (ie discard one of them)
		// usually happens on CallManager with internal calls, so we keep the one that's outgoing

		RtpSessionRef mergerSession;
		RtpSessionRef mergeeSession;

		if(session1->m_direction == CaptureEvent::DirOut)
		{
			mergerSession = session1;
			mergeeSession = session2;
		}
		else
		{
			mergerSession = session2;
			mergeeSession = session1;		
		}
		if(m_log->isInfoEnabled())
		{
			CStdString debug;
			debug.Format("Merging session %s %s with callid:%s into session %s %s with callid:%s",
				mergeeSession->m_trackingId, mergeeSession->m_ipAndPort, mergeeSession->m_callId,
				mergerSession->m_trackingId, mergerSession->m_ipAndPort, mergerSession->m_callId);
			LOG4CXX_INFO(m_log, debug);
		}
		Stop(mergeeSession);
	}

	if(numSessionsFound == 0)
	{
		// create new Raw RTP session and insert into IP+Port map
		CStdString trackingId = alphaCounter.GetNext();
		RtpSessionRef session(new RtpSession(trackingId));
		session->m_protocol = RtpSession::ProtRawRtp;
		session->m_ipAndPort = ipAndPort;
		m_byIpAndPort.insert(std::make_pair(ipAndPort, session));
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
		LOG4CXX_INFO(m_log,  session->m_trackingId + ": " + session->m_ipAndPort + " Expired");
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
		LOG4CXX_INFO(m_log,  session->m_trackingId + ": " + session->m_ipAndPort + " Expired");
		Stop(session);
	}
}

//==========================================================
void SipInviteInfo::ToString(CStdString& string)
{
	char fromIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_fromIp, fromIp, sizeof(fromIp));

	string.Format("from:%s %s,%s to:%s callid:%s", m_from, fromIp, m_fromRtpPort, m_to, m_callId);
}


