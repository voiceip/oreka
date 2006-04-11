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

#define RTP_SESSION_TIMEOUT 10

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
	LOG4CXX_INFO(m_log, m_trackingId + ": " + m_capturePort + " Session stop");

	if(m_started)
	{
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
	if((unsigned int)rtpPacket->m_sourceIp.s_addr == (unsigned int)m_invitorIp.s_addr)
	{
		m_inviteeIp = rtpPacket->m_destIp;
		m_inviteeTcpPort = rtpPacket->m_destPort;
		m_invitorTcpPort = rtpPacket->m_sourcePort;
	}
	else if((unsigned int)rtpPacket->m_destIp.s_addr == (unsigned int)m_invitorIp.s_addr)
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
	unsigned char channel = 0;

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
		channel = 1;
		if(m_log->isInfoEnabled())
		{
			rtpPacket->ToString(logMsg);
			logMsg =  m_trackingId + ": " + "1st packet s1: " + logMsg;
			LOG4CXX_INFO(m_log, logMsg);
		}
	}
	else
	{
		if((unsigned int)rtpPacket->m_sourceIp.s_addr == (unsigned int)m_lastRtpPacketSide1->m_sourceIp.s_addr)
		{
			m_lastRtpPacketSide1 = rtpPacket;
			channel = 1;
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
			channel = 2;
		}
	}

	// Compute the corrective offset (only if the two streams have greatly different timestamp, eg for Cisco CallManager)
	if(m_rtpTimestampCorrectiveOffset == 0 && m_lastRtpPacketSide2.get() != NULL)
	{
		//if (m_lastRtpPacketSide2->m_arrivalTimestamp == m_lastRtpPacketSide1->m_arrivalTimestamp)
		//{
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
		//}
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
		AudioChunkDetails details;
		details.m_arrivalTimestamp = rtpPacket->m_arrivalTimestamp;
		details.m_numBytes = rtpPacket->m_payloadSize;
		details.m_timestamp = rtpPacket->m_timestamp;
		details.m_rtpPayloadType = rtpPacket->m_payloadType;
		details.m_sequenceNumber = rtpPacket->m_seqNum;
		details.m_channel = channel;
		details.m_encoding = AlawAudio;
		AudioChunkRef chunk(new AudioChunk());
		chunk->SetBuffer(rtpPacket->m_payload, rtpPacket->m_payloadSize, details);
		g_audioChunkCallBack(chunk, m_capturePort);

		m_lastUpdated = rtpPacket->m_arrivalTimestamp;
	}
}


void RtpSession::ReportSipInvite(SipInviteInfoRef& invite)
{
	m_invite = invite;
	m_invitorIp = invite->m_fromRtpIp;
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
	char szFromRtpIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&invite->m_fromRtpIp, szFromRtpIp, sizeof(szFromRtpIp));

	CStdString ipAndPort = CStdString(szFromRtpIp) + "," + invite->m_fromRtpPort;
	std::map<CStdString, RtpSessionRef>::iterator pair;
	
	pair = m_byIpAndPort.find(ipAndPort);
	if (pair != m_byIpAndPort.end())
	{
		// The session already exists, do nothing
		return;
	}
	pair = m_byCallId.find(invite->m_callId);
	if (pair != m_byCallId.end())
	{
		// The session already exists
		RtpSessionRef session = pair->second;
		if(!session->m_ipAndPort.Equals(ipAndPort))
		{
			// The session RTP connection address has changed
			// Remove session from IP and Port map
			m_byIpAndPort.erase(session->m_ipAndPort);
			// ... update
			session->m_ipAndPort = ipAndPort;
			session->ReportSipInvite(invite);
			// ... and reinsert
			m_byIpAndPort.insert(std::make_pair(session->m_ipAndPort, session));

			LOG4CXX_INFO(m_log, session->m_trackingId + ": updated with new INVITE data");
		}
		return;
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

	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

	LOG4CXX_INFO(m_log, trackingId + ": created by SIP INVITE");
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

void RtpSessions::ReportSkinnyCallInfo(SkCallInfoStruct* callInfo, IpHeaderStruct* ipHeader)
{
	CStdString callId = IntToString(callInfo->callId);
	std::map<CStdString, RtpSessionRef>::iterator pair;
	pair = m_byCallId.find(callId);

	if (pair != m_byCallId.end())
	{
		return; // CM can resend the same message more than once in a session, so do nothing in this case
	}

	// create new session and insert into the callid map
	CStdString trackingId = alphaCounter.GetNext();
	RtpSessionRef session(new RtpSession(trackingId));
	session->m_callId = callId;
	session->m_endPointIp = ipHeader->ip_dest;	// CallInfo message always goes from CM to endpoint 
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

	if(m_log->isInfoEnabled())
	{
		CStdString logMsg;
		CStdString dir = CaptureEvent::DirectionToString(session->m_direction);
		char szEndPointIp[16];
		ACE_OS::inet_ntop(AF_INET, (void*)&session->m_endPointIp, szEndPointIp, sizeof(szEndPointIp));

		logMsg.Format("%s: Skinny CallInfo callId %s local:%s remote:%s dir:%s endpoint:%s", session->m_trackingId,
			session->m_callId, session->m_localParty, session->m_remoteParty, dir, szEndPointIp);
		LOG4CXX_INFO(m_log, logMsg);
	}

	m_byCallId.insert(std::make_pair(session->m_callId, session));

	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

}

void RtpSessions::ReportSkinnyStartMediaTransmission(SkStartMediaTransmissionStruct* startMedia, IpHeaderStruct* ipHeader)
{
	// Lookup by callId
	RtpSessionRef session;
	//CStdString callId = IntToString(startMedia->conferenceId);
	std::map<CStdString, RtpSessionRef>::iterator pair;

	//if(callId.Equals("0"))
	//{
		// Ok this seems to be CallManager 3.3 or older, Conference ID field is not populated
		// We need to find the CallInfo with the same endpoint IP address
		for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
		{
			RtpSessionRef tmpSession = pair->second;

			if((unsigned int)tmpSession->m_endPointIp.s_addr == (unsigned int)ipHeader->ip_dest.s_addr)
			{
				if(tmpSession->m_ipAndPort.size() == 0)
				{
					session = tmpSession;
					// The "Call ID" between StartMediaTransmission and StopMediaTransmission 
					// is represented by either PassThruPartyId or conferenceId field.
					// So let's move the session within the callId map.
					// 1. remove it from the map
					m_byCallId.erase(session->m_callId);
					// 2. add it back with new CallId
					CStdString newCallId;
					if(startMedia->passThruPartyId)
					{
						newCallId = IntToString(startMedia->passThruPartyId);
					}
					else
					{
						newCallId = IntToString(startMedia->conferenceId);
					}
					CStdString oldCallId = session->m_callId;
					session->m_callId = newCallId;
					m_byCallId.insert(std::make_pair(newCallId, session));

					if(m_log->isInfoEnabled())
					{
						CStdString logMsg;
						logMsg.Format("%s: Skinny StartMedia: callId %s becomes %s", session->m_trackingId, oldCallId, newCallId);
						LOG4CXX_INFO(m_log, logMsg);
					}
					break;
				}
				else
				{
					// The session has already had a StartMediaTransmission message.
				}
			}
		}
	//}
	//else
	//{
		// CallManager 4 or newer
	//	pair = m_byCallId.find(callId);
	//	if (pair != m_byCallId.end())
	//	{
	//		session = pair->second;
	//	}
	//}

	if (session.get() != NULL)
	{
		// Session found
		if(session->m_ipAndPort.size() == 0)
		{
			CStdString ipAndPort;
			char szRemoteIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&startMedia->remoteIpAddr, szRemoteIp, sizeof(szRemoteIp));
			ipAndPort.Format("%s,%u", szRemoteIp, startMedia->remoteTcpPort);
			
			pair = m_byIpAndPort.find(ipAndPort);
			if (pair != m_byIpAndPort.end())
			{
				// A session exists on the same IP+port, stop old session
				RtpSessionRef session = pair->second;
				Stop(session);
			}

			if(m_log->isInfoEnabled())
			{
				CStdString logMsg;
				logMsg.Format("%s: Skinny StartMedia: callId %s is on %s", session->m_trackingId, session->m_callId, ipAndPort);
				LOG4CXX_INFO(m_log, logMsg);
			}

			session->m_ipAndPort = ipAndPort;
			m_byIpAndPort.insert(std::make_pair(session->m_ipAndPort, session));

			CStdString numSessions = IntToString(m_byIpAndPort.size());
			LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);
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
	CStdString callId;
	if(stopMedia->conferenceId == 0)
	{
		// CallManager 3.3 or older, see explanation in ReportSkinnyStartMediaTransmission
		callId = IntToString(stopMedia->passThruPartyId);
	}
	else
	{
		// CallManager 4 or later
		callId = IntToString(stopMedia->conferenceId);
	}
	std::map<CStdString, RtpSessionRef>::iterator pair;
	pair = m_byCallId.find(callId);

	if (pair != m_byCallId.end())
	{
		// Session found: stop it
		RtpSessionRef session = pair->second;

		if(m_log->isInfoEnabled())
		{
			CStdString logMsg;
			logMsg.Format("%s: Skinny StopMedia", session->m_trackingId);
			LOG4CXX_INFO(m_log, logMsg);
		}

		Stop(session);
	}
}

void RtpSessions::Stop(RtpSessionRef& session)
{
	session->Stop();
	if(session->m_ipAndPort.size() > 0)
	{
		m_byIpAndPort.erase(session->m_ipAndPort);

		CStdString numSessions = IntToString(m_byIpAndPort.size());
		LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);
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
		if (session1.get() != NULL)
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
		if (session2.get() != NULL)
		{
			// Found a session give it the RTP packet info
			session2->AddRtpPacket(rtpPacket);
			numSessionsFound++;
		}
	}

	if(numSessionsFound == 2)
	{
		// Need to "merge" the two sessions (ie discard one of them)
		// Can happen when RTP stream detected before skinny StartMediaTransmission
		// Can also happens on CallManager with internal calls, so we keep the one that's outgoing
		RtpSessionRef mergerSession;
		RtpSessionRef mergeeSession;

		if(session1->m_protocol == RtpSession::ProtRawRtp)
		{
			mergerSession = session2;
			mergeeSession = session1;
		}
		else if(session2->m_protocol == RtpSession::ProtRawRtp)
		{
			mergerSession = session1;
			mergeeSession = session2;
		}
		else
		{
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
		session->AddRtpPacket(rtpPacket);
		m_byIpAndPort.insert(std::make_pair(ipAndPort, session));

		CStdString numSessions = IntToString(m_byIpAndPort.size());
		LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

		LOG4CXX_INFO(m_log, trackingId + ": created by RTP packet");
	}
}

void RtpSessions::StopAll()
{
	time_t forceExpiryTime = time(NULL) + 2*RTP_SESSION_TIMEOUT;
	Hoover(forceExpiryTime);
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
		if((now - session->m_lastUpdated) > RTP_SESSION_TIMEOUT)
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
		if((now - session->m_lastUpdated) > RTP_SESSION_TIMEOUT)
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
SipInviteInfo::SipInviteInfo()
{
	m_fromRtpIp.s_addr = 0;
}

void SipInviteInfo::ToString(CStdString& string)
{
	char fromRtpIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_fromRtpIp, fromRtpIp, sizeof(fromRtpIp));

	char senderIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));

	char receiverIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	string.Format("sender:%s from:%s RTP:%s,%s to:%s rcvr:%s callid:%s", senderIp, m_from, fromRtpIp, m_fromRtpPort, m_to, receiverIp, m_callId);
}


