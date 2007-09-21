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

#include "Utils.h"
#include "AudioCapture.h"
#include "RtpSession.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include <list>
#include "ConfigManager.h"
#include "VoIpConfig.h"
#include "ace/OS_NS_arpa_inet.h"
#include "MemUtils.h"

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;

extern VoIpConfigTopObjectRef g_VoIpConfigTopObjectRef;
#define DLLCONFIG g_VoIpConfigTopObjectRef.get()->m_config

RtpSession::RtpSession(CStdString& trackingId)
{
	m_trackingId = trackingId;
	m_lastUpdated = time(NULL);
	m_creationDate = ACE_OS::gettimeofday();
	m_skinnyLastCallInfoTime = m_creationDate;
	m_log = Logger::getLogger("rtpsession");
	m_invitorIp.s_addr = 0;
	m_invitorTcpPort = 0;
	m_inviteeIp.s_addr = 0;
	m_inviteeTcpPort = 0;
	m_direction = CaptureEvent::DirUnkn;
	m_protocol = ProtUnkn;
	m_numRtpPackets = 0;
	m_started = false;
	m_stopped = false;
	m_onHold = false;
	m_keep = false;
	m_nonLookBackSessionStarted = false;
	m_beginDate = 0;
	m_hasDuplicateRtp = false;
	m_highestRtpSeqNumDelta = 0;
	m_minRtpSeqDelta = (double)DLLCONFIG.m_rtpDiscontinuityMinSeqDelta;
	m_minRtpTimestampDelta = (double)DLLCONFIG.m_rtpDiscontinuityMinSeqDelta * 160;		// arbitrarily based on 160 samples per packet (does not need to be precise)
	m_skinnyPassThruPartyId = 0;
	memset(m_localMac, 0, sizeof(m_localMac));
	memset(m_remoteMac, 0, sizeof(m_remoteMac));
	m_currentRtpEvent = 65535;
	m_lastEventEndSeqNo = 0;
	m_currentDtmfDuration = 0;
	m_currentRtpEventTs = 0;
	m_currentDtmfVolume = 0;
}

void RtpSession::Stop()
{
	CStdString logMsg;
	logMsg.Format("[%s] %s Session stop, numRtpPkts:%d dupl:%d seqDelta:%d lastUpdated:%u", m_trackingId, m_capturePort, m_numRtpPackets, m_hasDuplicateRtp, m_highestRtpSeqNumDelta, m_lastUpdated);
	LOG4CXX_INFO(m_log, logMsg);

	if(m_started && !m_stopped)
	{
		CaptureEventRef stopEvent(new CaptureEvent);
		stopEvent->m_type = CaptureEvent::EtStop;
		stopEvent->m_timestamp = m_lastUpdated;
		g_captureEventCallBack(stopEvent, m_capturePort);
		m_stopped = true;
	}
}

void RtpSession::Start()
{
	m_started = true;
	m_beginDate = time(NULL);
	GenerateOrkUid();
	CaptureEventRef startEvent(new CaptureEvent);
	startEvent->m_type = CaptureEvent::EtStart;
	startEvent->m_timestamp = m_beginDate;
	startEvent->m_value = m_trackingId;
	CStdString timestamp = IntToString(startEvent->m_timestamp);
	LOG4CXX_INFO(m_log,  "[" + m_trackingId + "] " + m_capturePort + " " + ProtocolToString(m_protocol) + " Session start, timestamp:" + timestamp);
	g_captureEventCallBack(startEvent, m_capturePort);
}

void RtpSession::GenerateOrkUid()
{
	struct tm date = {0};
	ACE_OS::localtime_r(&m_beginDate, &date);
	int month = date.tm_mon + 1;				// january=0, decembre=11
	int year = date.tm_year + 1900;
	m_orkUid.Format("%.4d%.2d%.2d_%.2d%.2d%.2d_%s", year, month, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec, m_trackingId);
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

	if(DLLCONFIG.m_sangomaEnable)
	{
		m_capturePort = IntToString(rtpPacket->m_sourcePort % 1000);
	}
	else
	{
		m_capturePort = m_ipAndPort;
	}

	if(sourceIsLocal)
	{
                /* With Raw RTP, the local party is not obtained through any intelligent
                 * signalling so we should probably do this check here? */
                if(DLLCONFIG.m_useMacIfNoLocalParty)
                {
                        MemMacToHumanReadable((unsigned char*)rtpPacket->m_sourceMac, m_localParty);
                }
                else
                {
                        m_localParty = szSourceIp;
                }

		m_remoteParty = szDestIp;
		//m_capturePort.Format("%s,%d", szSourceIp, rtpPacket->m_sourcePort);
		m_localIp = rtpPacket->m_sourceIp;
		m_remoteIp = rtpPacket->m_destIp;
		memcpy(m_localMac, rtpPacket->m_sourceMac, sizeof(m_localMac));
		memcpy(m_remoteMac, rtpPacket->m_destMac, sizeof(m_remoteMac));
	}
	else
	{
                /* With Raw RTP, the local party is not obtained through any intelligent
                 * signalling so we should probably do this check here? */
		if(DLLCONFIG.m_useMacIfNoLocalParty)
                {
                        MemMacToHumanReadable((unsigned char*)rtpPacket->m_destMac, m_localParty);
		}
		else
		{
			m_localParty = szDestIp;
		}

		m_remoteParty = szSourceIp;
		//m_capturePort.Format("%s,%d", szDestIp, rtpPacket->m_destPort);
		m_localIp = rtpPacket->m_destIp;
		m_remoteIp = rtpPacket->m_sourceIp;
                memcpy(m_localMac, rtpPacket->m_destMac, sizeof(m_localMac));
                memcpy(m_remoteMac, rtpPacket->m_sourceMac, sizeof(m_remoteMac));
	}
}

void RtpSession::ProcessMetadataSipIncoming()
{
	m_remoteParty = m_invite->m_from;
	m_localParty = m_invite->m_to;
	m_direction = CaptureEvent::DirIn;
	char szInviteeIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_inviteeIp, szInviteeIp, sizeof(szInviteeIp));
	m_capturePort.Format("%s,%d", szInviteeIp, m_inviteeTcpPort);
	m_localIp = m_inviteeIp;
	m_remoteIp = m_invitorIp;
	memcpy(m_localMac, m_inviteeMac, sizeof(m_localMac));
	memcpy(m_remoteMac, m_invitorMac, sizeof(m_remoteMac));
}

void RtpSession::ProcessMetadataSipOutgoing()
{
	m_remoteParty = m_invite->m_to;
	m_localParty = m_invite->m_from;
	m_direction = CaptureEvent::DirOut;
	char szInvitorIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_invitorIp, szInvitorIp, sizeof(szInvitorIp));
	m_capturePort.Format("%s,%d", szInvitorIp, m_invitorTcpPort);
	m_localIp = m_invitorIp;
	m_remoteIp = m_inviteeIp;
        memcpy(m_localMac, m_invitorMac, sizeof(m_localMac));
	memcpy(m_remoteMac, m_inviteeMac, sizeof(m_remoteMac));
}

void RtpSession::UpdateMetadataSip(RtpPacketInfoRef& rtpPacket, bool sourceRtpAddressIsNew)
{
	// Find out if the new RTP packet could match one of the SIP invites associated with the session
	SipInviteInfoRef invite;

	std::list<SipInviteInfoRef>::iterator it;
	for(it = m_invites.begin(); it != m_invites.end(); it++)
	{
		SipInviteInfoRef tmpInvite = *it;
		if(tmpInvite->m_validated)
		{
			break;
		}
		if(sourceRtpAddressIsNew)
		{
			if((unsigned int)(rtpPacket->m_sourceIp.s_addr) == (unsigned int)(tmpInvite->m_receiverIp.s_addr))
			{
				invite = tmpInvite;
			}
		}
		else
		{
			if((unsigned int)(rtpPacket->m_destIp.s_addr) == (unsigned int)(tmpInvite->m_receiverIp.s_addr))
			{
				invite = tmpInvite;
			}
		}
	}
	if(invite.get())
	{
		// The INVITE has generated an RTP stream
		invite->m_validated = true;

		// Update session metadata with INVITE info
		m_remoteParty = invite->m_from;
		m_localParty = invite->m_to;
		m_localIp = invite->m_receiverIp;
		memcpy(m_localMac, invite->m_receiverMac, sizeof(m_localMac));

		// Do some logging
		CStdString inviteString;
		invite->ToString(inviteString);
		CStdString rtpString;
		rtpPacket->ToString(rtpString);
		CStdString logMsg;
		logMsg.Format("[%s] metadata update: local:%s remote:%s RTP:%s INVITE:%s", m_trackingId, m_localParty, m_remoteParty, rtpString, inviteString);
		LOG4CXX_INFO(m_log,  logMsg);

		// Report Local party
		CaptureEventRef event(new CaptureEvent());
		event->m_type = CaptureEvent::EtLocalParty;
		event->m_value = m_localParty;
		g_captureEventCallBack(event, m_capturePort);

		// Report remote party
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtRemoteParty;
		event->m_value = m_remoteParty;
		g_captureEventCallBack(event, m_capturePort);

		// Report Local IP
		char szLocalIp[16];
		ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtLocalIp;
		event->m_value = szLocalIp;
		g_captureEventCallBack(event, m_capturePort);

		// Trigger metadata update
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtUpdate;
		g_captureEventCallBack(event, m_capturePort);
	}
}

void RtpSession::ProcessMetadataSip(RtpPacketInfoRef& rtpPacket)
{
	// work out invitee media IP address
	if((unsigned int)rtpPacket->m_sourceIp.s_addr == (unsigned int)m_invitorIp.s_addr)
	{
		m_inviteeIp = rtpPacket->m_destIp;
		m_inviteeTcpPort = rtpPacket->m_destPort;
		m_invitorTcpPort = rtpPacket->m_sourcePort;
		memcpy(m_inviteeMac, rtpPacket->m_destMac, sizeof(m_inviteeMac));
	}
	else if((unsigned int)rtpPacket->m_destIp.s_addr == (unsigned int)m_invitorIp.s_addr)
	{
		m_inviteeIp = rtpPacket->m_sourceIp;
		m_inviteeTcpPort = rtpPacket->m_sourcePort;
		m_invitorTcpPort = rtpPacket->m_destPort;
		memcpy(m_inviteeMac, rtpPacket->m_sourceMac, sizeof(m_inviteeMac));
	}
	else
	{
		m_inviteeIp = rtpPacket->m_sourceIp;
		m_inviteeTcpPort = rtpPacket->m_sourcePort;
		m_invitorTcpPort = rtpPacket->m_destPort;
		memcpy(m_inviteeMac, rtpPacket->m_sourceMac, sizeof(m_inviteeMac));
		LOG4CXX_WARN(m_log,  "[" + m_trackingId + "] " + m_ipAndPort + " alien RTP packet");
	}

	// work out capture port and direction
	if(DLLCONFIG.IsMediaGateway(m_invitorIp))
	{
		if(DLLCONFIG.IsMediaGateway(m_inviteeIp))
		{
			// Media gateway talking to media gateway, this is probably incoming
			ProcessMetadataSipIncoming();
		}
		else if(DLLCONFIG.IsPartOfLan(m_inviteeIp))
		{
			// Gateway to LAN, this is pobably incoming
			ProcessMetadataSipIncoming();
		}
		else
		{
			// Gateway to outside address, probably outgoing but treat as incoming for now because
			// It can be due to misconfigured LAN Mask, odds are it's still incoming.
			ProcessMetadataSipIncoming();
		}
	}
	else if (DLLCONFIG.IsPartOfLan(m_invitorIp))
	{
		ProcessMetadataSipOutgoing();
	}
	else
	{
		// SIP invitor media IP address is an outside IP address
		if(DLLCONFIG.IsMediaGateway(m_inviteeIp))
		{
			ProcessMetadataSipIncoming();
		}
		else if(DLLCONFIG.IsPartOfLan(m_inviteeIp))
		{
			ProcessMetadataSipIncoming();
		}
		else
		{
			// SIP invitee media address is an outside IP address
			ProcessMetadataSipOutgoing();
		}
	}
}

void RtpSession::ProcessMetadataSkinny(RtpPacketInfoRef& rtpPacket)
{
	//  In Skinny we always want the endpoint (phone) to be used as a capture port and as a local IP address
	char szEndpointIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_endPointIp, szEndpointIp, sizeof(szEndpointIp));

	if( ( (unsigned int)m_endPointIp.s_addr) == ((unsigned int)rtpPacket->m_destIp.s_addr) )
	{
		m_capturePort.Format("%s,%u", szEndpointIp, rtpPacket->m_destPort);

		m_localIp = rtpPacket->m_destIp;
		m_remoteIp = rtpPacket->m_sourceIp;
                memcpy(m_localMac, rtpPacket->m_destMac, sizeof(m_localMac));
                memcpy(m_remoteMac, rtpPacket->m_sourceMac, sizeof(m_remoteMac));
	}
	else
	{
		m_capturePort.Format("%s,%u", szEndpointIp, rtpPacket->m_sourcePort);

		m_localIp = rtpPacket->m_sourceIp;
		m_remoteIp = rtpPacket->m_destIp;
                memcpy(m_localMac, rtpPacket->m_sourceMac, sizeof(m_localMac));
                memcpy(m_remoteMac, rtpPacket->m_destMac, sizeof(m_remoteMac));
	}
}


void RtpSession::ReportMetadata()
{
	char szLocalIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
	char szRemoteIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_remoteIp, szRemoteIp, sizeof(szRemoteIp));

	// Check if we don't have the local party based on the endpoint IP address
	if(m_localParty.IsEmpty())
	{
		if(m_protocol == ProtSkinny)
		{
			EndpointInfoRef endpointInfo = RtpSessionsSingleton::instance()->GetEndpointInfo(m_endPointIp);
			if(endpointInfo.get())
			{
				m_localParty = endpointInfo->m_extension;
			}
		}
	}

	// Make sure Local Party is always reported
	if(m_localParty.IsEmpty())
	{
		if(DLLCONFIG.m_useMacIfNoLocalParty)
		{
			MemMacToHumanReadable((unsigned char*)m_localMac, m_localParty);
		}
		else
		{
			m_localParty = szLocalIp;
		}
	}

	// Report Local party
	CaptureEventRef event(new CaptureEvent());
	event->m_type = CaptureEvent::EtLocalParty;
	event->m_value = m_localParty;
	g_captureEventCallBack(event, m_capturePort);

	// Report remote party
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtRemoteParty;
	event->m_value = m_remoteParty;
	g_captureEventCallBack(event, m_capturePort);

	// Report direction
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtDirection;
	event->m_value = CaptureEvent::DirectionToString(m_direction);
	g_captureEventCallBack(event, m_capturePort);

	// Report Local IP address
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtLocalIp;
	event->m_value = szLocalIp;
	g_captureEventCallBack(event, m_capturePort);

	// Report Remote IP address
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtRemoteIp;
	event->m_value = szRemoteIp;
	g_captureEventCallBack(event, m_capturePort);

	// Report OrkUid
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtOrkUid;
	event->m_value = m_orkUid;
	g_captureEventCallBack(event, m_capturePort);

	// Report native Call ID
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtCallId;
	event->m_value = m_callId;
	g_captureEventCallBack(event, m_capturePort);

	// Report extracted fields
	for(std::map<CStdString, CStdString>::iterator pair = m_tags.begin(); pair != m_tags.end(); pair++)
	{
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key = pair->first;
		event->m_value = pair->second;
		g_captureEventCallBack(event, m_capturePort);
	}

	// Report end of metadata
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtEndMetadata;
	g_captureEventCallBack(event, m_capturePort);
}

void RtpSession::RecordRtpEvent()
{
	CaptureEventRef event(new CaptureEvent());
	CStdString dtmfEventString, dtmfEventKey;

	dtmfEventString.Format("event:%d timestamp:%d duration:%d volume:%d seqno:%d", m_currentRtpEvent, m_currentRtpEventTs,
 m_currentDtmfDuration, m_currentDtmfVolume, m_currentSeqNo);
	dtmfEventKey.Format("%d_RtpDtmfEvent", m_currentRtpEventTs);
	event->m_type = CaptureEvent::EtKeyValue;
	event->m_key = dtmfEventKey;
	event->m_value = dtmfEventString;
	g_captureEventCallBack(event, m_capturePort);

	//LOG4CXX_INFO(m_log, "[" + m_trackingId + "] Recording RTP event [ " + dtmfEventString + " ]");
}

void RtpSession::HandleRtpEvent(RtpPacketInfoRef& rtpPacket)
{
	CStdString logMsg;

	if(rtpPacket->m_payloadSize < sizeof(RtpEventPayloadFormat))
	{
		LOG4CXX_WARN(m_log, "[" + m_trackingId + "] Payload size for event packet too small");
		return;
	}

	RtpEventPayloadFormat *payloadFormat = (RtpEventPayloadFormat *)rtpPacket->m_payload;
	RtpEventInfoRef rtpEventInfo(new RtpEventInfo());

	rtpEventInfo->m_event = (unsigned short)payloadFormat->event;
	rtpEventInfo->m_e = (payloadFormat->er_volume & 0x80) ? 1 : 0;
	rtpEventInfo->m_r = (payloadFormat->er_volume & 0x40) ? 1 : 0;
	rtpEventInfo->m_volume = (unsigned short)(payloadFormat->er_volume & 0x3F);
	rtpEventInfo->m_duration = ntohs(payloadFormat->duration);
	rtpEventInfo->m_startTimestamp = rtpPacket->m_timestamp;

	if((m_currentRtpEvent != 65535) && (m_currentRtpEvent != rtpEventInfo->m_event))
	{
		RecordRtpEvent();
	}
	else if(rtpEventInfo->m_e)
	{
		if((m_currentRtpEvent != 65535))
		{
			m_currentDtmfDuration = rtpEventInfo->m_duration;
			m_currentDtmfVolume = rtpEventInfo->m_volume;
			m_currentRtpEventTs = rtpEventInfo->m_startTimestamp;
			m_currentSeqNo = rtpPacket->m_seqNum;

			if(m_lastEventEndSeqNo != rtpPacket->m_seqNum)
			{
				RecordRtpEvent();
				m_lastEventEndSeqNo = rtpPacket->m_seqNum;
			}

			m_currentRtpEvent = 65535;
		}

		rtpEventInfo->m_event = 65535;
		rtpEventInfo->m_duration = 0;
	}
	else if((m_currentRtpEvent != 65535) && m_currentDtmfDuration && (rtpEventInfo->m_duration < m_currentDtmfDuration))
	{
		RecordRtpEvent();
	}

	if(!rtpEventInfo->m_e)
	{
		m_currentRtpEvent = rtpEventInfo->m_event;
	}

	m_currentDtmfDuration = rtpEventInfo->m_duration;
	m_currentDtmfVolume = rtpEventInfo->m_volume;
	m_currentRtpEventTs = rtpEventInfo->m_startTimestamp;
	m_currentSeqNo = rtpPacket->m_seqNum;

	return;
}

// Returns false if the packet does not belong to the session (RTP timestamp discontinuity)
bool RtpSession::AddRtpPacket(RtpPacketInfoRef& rtpPacket)
{
	CStdString logMsg;
	unsigned char channel = 0;

	if((CONFIG.m_lookBackRecording == false) && (m_numRtpPackets > 0))
	{
		if(m_numRtpPackets == 1 && !m_nonLookBackSessionStarted)
		{
			Start();
			ReportMetadata();
			m_nonLookBackSessionStarted = true;
		}

		if(!m_keep)
		{
			m_lastUpdated = time(NULL);
			return true;
		}
	}

	// Dismiss packets that should not be part of a Skinny session
	if(m_protocol == ProtSkinny)
	{
		if(	(unsigned int)rtpPacket->m_sourceIp.s_addr != (unsigned int)m_endPointIp.s_addr &&
			(unsigned int)rtpPacket->m_destIp.s_addr != (unsigned int)m_endPointIp.s_addr     )
		{
			return true;	// dismiss packet that has neither source or destination matching the endpoint.
		}
	}

	if(m_protocol == ProtSip)
	{
		/* Check if this is a telephone-event */
		if(rtpPacket->m_payloadType == StringToInt(m_telephoneEventPayloadType))
		{
			// This is a telephone-event
			HandleRtpEvent(rtpPacket);
			return true;
		}
	}

	// If we are on hold, unmark this
	if(m_onHold)
	{
		logMsg =  "[" + m_trackingId + "] Session going off hold due to RTP activity";
		m_onHold = false;
	}

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

	if(m_lastRtpPacketSide1.get() == NULL)
	{
		// First RTP packet for side 1
		m_lastRtpPacketSide1 = rtpPacket;
		channel = 1;
		if(m_log->isInfoEnabled())
		{
			rtpPacket->ToString(logMsg);
			logMsg =  "[" + m_trackingId + "] 1st packet s1: " + logMsg;
			LOG4CXX_INFO(m_log, logMsg);
		}
	}
	else
	{
		// Comparing destination IP address to find out if side1, see (1)
		if((unsigned int)rtpPacket->m_destIp.s_addr == (unsigned int)m_lastRtpPacketSide1->m_destIp.s_addr)
		{
			if(rtpPacket->m_timestamp == m_lastRtpPacketSide1->m_timestamp)
			{
				m_hasDuplicateRtp = true;
				return true;	// dismiss duplicate RTP packet
			}
			else
			{
				double seqNumDelta = (double)rtpPacket->m_seqNum - (double)m_lastRtpPacketSide1->m_seqNum;
				if(DLLCONFIG.m_rtpDiscontinuityDetect)
				{
					double timestampDelta = (double)rtpPacket->m_timestamp - (double)m_lastRtpPacketSide1->m_timestamp;
					if(	abs(seqNumDelta) > m_minRtpSeqDelta  &&
						abs(timestampDelta) > m_minRtpTimestampDelta)	
					{
						logMsg.Format("[%s] RTP discontinuity s1: before: seq:%u ts:%u after: seq:%u ts:%u", 
							m_trackingId, m_lastRtpPacketSide1->m_seqNum, m_lastRtpPacketSide1->m_timestamp, 
							rtpPacket->m_seqNum, rtpPacket->m_timestamp);
						LOG4CXX_INFO(m_log, logMsg);
						return false;
					}
				}
				if(seqNumDelta > (double)m_highestRtpSeqNumDelta)
				{
					m_highestRtpSeqNumDelta = (unsigned int)seqNumDelta;
				}
			}
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
					logMsg =  "[" + m_trackingId + "] 1st packet s2: " + logMsg;
					LOG4CXX_INFO(m_log, logMsg);
				}
			}
			else
			{
				if(rtpPacket->m_timestamp == m_lastRtpPacketSide2->m_timestamp)
				{
					m_hasDuplicateRtp = true;
					return true;	// dismiss duplicate RTP packet
				}
				else
				{
					double seqNumDelta = (double)rtpPacket->m_seqNum - (double)m_lastRtpPacketSide2->m_seqNum;
					if(DLLCONFIG.m_rtpDiscontinuityDetect)
					{
						double timestampDelta = (double)rtpPacket->m_timestamp - (double)m_lastRtpPacketSide2->m_timestamp;
						if(	abs(seqNumDelta) > m_minRtpSeqDelta  &&
							abs(timestampDelta) > m_minRtpTimestampDelta)	
						{
							logMsg.Format("[%s] RTP discontinuity s2: before: seq:%u ts:%u after: seq:%u ts:%u", 
								m_trackingId, m_lastRtpPacketSide2->m_seqNum, m_lastRtpPacketSide2->m_timestamp, 
								rtpPacket->m_seqNum, rtpPacket->m_timestamp);
							LOG4CXX_INFO(m_log, logMsg);
							return false;
						}
					}
					if(seqNumDelta > (double)m_highestRtpSeqNumDelta)
					{
						m_highestRtpSeqNumDelta = (unsigned int)seqNumDelta;
					}
				}
			}
			m_lastRtpPacketSide2 = rtpPacket;
			channel = 2;
		}
	}

	m_numRtpPackets++;

	// Detect RTP stream change
	bool hasSourceAddress = m_rtpAddressList.HasAddressOrAdd(rtpPacket->m_sourceIp, rtpPacket->m_sourcePort);
	bool hasDestAddress = m_rtpAddressList.HasAddressOrAdd(rtpPacket->m_destIp, rtpPacket->m_destPort);
	if(	hasSourceAddress == false || hasDestAddress == false )
	{
		rtpPacket->ToString(logMsg);
		logMsg.Format("[%s] new RTP stream s%d: %s", 
							m_trackingId, channel, logMsg);
		LOG4CXX_INFO(m_log, logMsg);

		if(m_protocol == ProtSip && m_started)	// make sure this only happens if ReportMetadata() already been called for the session
		{
			UpdateMetadataSip(rtpPacket, hasDestAddress);
		}
	}

	if(m_log->isDebugEnabled())
	{
		CStdString debug;
		debug.Format("[%s] %s: Add RTP packet srcPort:%u dstPort:%u seq:%u ts:%u  arrival:%u ch:%d", m_trackingId, m_capturePort, rtpPacket->m_sourcePort, rtpPacket->m_destPort, rtpPacket->m_seqNum, rtpPacket->m_timestamp, rtpPacket->m_arrivalTimestamp, channel);
		LOG4CXX_DEBUG(m_log, debug);
	}

	if(		(m_protocol == ProtRawRtp && m_numRtpPackets == 50)	||
			(m_protocol == ProtSkinny && m_numRtpPackets == 2)	||
			(m_protocol == ProtSip && m_numRtpPackets == 2)			)
	{
		// We've got enough packets to start the session.
		// For Raw RTP, the high number is to make sure we have a "real" raw RTP session, not a leftover from a SIP/Skinny session
		if(CONFIG.m_lookBackRecording == true) {
			Start();
			ReportMetadata();
		}
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
		details.m_numBytes = rtpPacket->m_payloadSize;
		AudioChunkRef chunk(new AudioChunk());
		chunk->SetBuffer(rtpPacket->m_payload, details);
		g_audioChunkCallBack(chunk, m_capturePort);

		m_lastUpdated = rtpPacket->m_arrivalTimestamp;
	}
	return true;
}


void RtpSession::ReportSipInvite(SipInviteInfoRef& invite)
{
	if(m_invite.get() == NULL)
	{
		m_invite = invite;
		m_invitorIp = invite->m_fromRtpIp;
		memcpy(m_invitorMac, invite->m_senderMac, sizeof(m_invitorMac));
	}
	else
	{
		CStdString inviteString;
		invite->ToString(inviteString);
		CStdString logMsg;
		logMsg.Format("[%s] associating INVITE:%s", m_trackingId, inviteString);
		m_fromRtpIp = invite->m_fromRtpIp;
		LOG4CXX_INFO(m_log, logMsg);
	}
	m_invites.push_front(invite);
	m_telephoneEventPayloadType = invite->m_telephoneEventPayloadType;

	// Gather extracted fields
	std::copy(invite->m_extractedFields.begin(), invite->m_extractedFields.end(), std::inserter(m_tags, m_tags.begin()));
}

void RtpSession::ReportSipErrorPacket(SipFailureMessageInfoRef& info)
{
	CaptureEventRef event(new CaptureEvent());
	event->m_type = CaptureEvent::EtKeyValue;
	event->m_key = CStdString("failed");
	event->m_value = CStdString("true");
	g_captureEventCallBack(event, m_capturePort);
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

bool RtpSession::OrkUidMatches(CStdString &oUid)
{
	if(m_orkUid.CompareNoCase(oUid) == 0)
	{
		return true;
	}

	return false;
}

bool RtpSession::PartyMatches(CStdString &party)
{
	if(party.size() > 0)
	{
		if(m_localParty.CompareNoCase(party) == 0 || m_remoteParty.CompareNoCase(party) == 0)
		{
			return true;
		}
	}
	return false;
}

//=====================================================================
RtpSessions::RtpSessions()
{
	m_log = Logger::getLogger("rtpsessions");
	if(CONFIG.m_debug)
	{
		m_alphaCounter.Reset();
	}
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
		// The session already exists, report the new INVITE

		/*
		 * If the sendonly attribute is present then our call is
		 * going on hold.
		 */
		RtpSessionRef session = pair->second;

		if(invite->m_attrSendonly)
		{
			session->m_onHold = true;
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going on hold");
			return;
		}
		else
		{
			if(session->m_onHold)
			{
				session->m_onHold = false;
				LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going off hold");
				return;
			}
		}

		session->ReportSipInvite(invite);
		return;
	}
	pair = m_byCallId.find(invite->m_callId);
	if (pair != m_byCallId.end())
	{
		// The session already exists
		RtpSessionRef session = pair->second;

		/*
		 * If the sendonly attribute is present then our call is
		 * going on hold.
		 */
		if(invite->m_attrSendonly)
		{
			session->m_onHold = true;
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going on hold");
			return;
		}
		else
		{
			/* If we're already on hold and sendonly is not present
			 * then we go off hold */
			if(session->m_onHold)
			{
				session->m_onHold = false;
				LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going off hold");
				return;
			}
		}

		// For now, do not report new INVITEs that have the same SIP call ID but a different media address
		// those INVITEs are ignored altogether.
		if(!session->m_ipAndPort.Equals(ipAndPort))
		{
			//===== The following is disabled because it disrupts valid sessions	====
			//===== We need to make sure that at least one RTP packet has been		====
			//===== seen that validates any new INVITE associated with the session	====
			//// The session RTP connection address has changed
			//// Remove session from IP and Port map
			//m_byIpAndPort.erase(session->m_ipAndPort);
			//// ... update
			//session->m_ipAndPort = ipAndPort;
			//session->ReportSipInvite(invite);
			//// ... and reinsert
			//m_byIpAndPort.insert(std::make_pair(session->m_ipAndPort, session));
			//
			//LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] updated with new INVITE data");
		}
		return;
	}

	// create new session and insert into both maps
	CStdString trackingId = m_alphaCounter.GetNext();
	RtpSessionRef session(new RtpSession(trackingId));
	session->m_ipAndPort = ipAndPort;
	session->m_callId = invite->m_callId;
	session->m_protocol = RtpSession::ProtSip;
	session->ReportSipInvite(invite);
	m_byIpAndPort.insert(std::make_pair(session->m_ipAndPort, session));
	m_byCallId.insert(std::make_pair(session->m_callId, session));

	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

	CStdString inviteString;
	invite->ToString(inviteString);
	LOG4CXX_INFO(m_log, "[" + trackingId + "] created by INVITE:" + inviteString);
}

void RtpSessions::ReportSipErrorPacket(SipFailureMessageInfoRef& info)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;

	pair = m_byCallId.find(info->m_callId);
	if (pair != m_byCallId.end())
	{
		RtpSessionRef session = pair->second;

		if(info->m_errorCode == "407")
		{
			// authentication needed
		}
		else
		{
			// Other error, stop the session
			session->ReportSipErrorPacket(info);

			CStdString InviteInfoString;
			session->m_invite->ToString(InviteInfoString);
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] stopped by SIP \"" + info->m_errorCode + " " + info->m_errorString + "\" " + InviteInfoString);
			Stop(session);
		}
	}
	else
	{
		CStdString errorString;
		info->ToString(errorString);
		LOG4CXX_INFO(m_log, "Could not associate SIP error packet [" + errorString + "] with any RTP session");
	}
	return;
}

void RtpSessions::ReportSip200Ok(Sip200OkInfoRef info)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;

	pair = m_byCallId.find(info->m_callId);
	if (pair != m_byCallId.end())
	{
		RtpSessionRef session = pair->second;

		//if(info->m_hasSdp && DLLCONFIG.IsPartOfLan(session->m_fromRtpIp) && !DLLCONFIG.IsPartOfLan(info->m_fromRtpIp) && !session->m_numRtpPackets)
		if(info->m_hasSdp && DLLCONFIG.m_sipUse200OkMediaAddress && !session->m_numRtpPackets) 
		{
			unsigned short mediaPort = ACE_OS::atoi(info->m_mediaPort);
			SetMediaAddress(session, info->m_mediaIp, mediaPort);
		}
		//else
		//{
		//	CStdString logString;
		//	logString.Format("hasSDP:%d use200:%d numRtpPkts:%d callId:%s", info->m_hasSdp, DLLCONFIG.m_sipUse200OkMediaAddress, session->m_numRtpPackets, info->m_callId);
		//	LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] 200Ok RTP address not updated: " + session->m_ipAndPort + " " + logString );
		//}
	}
	//else
	//{
	//	LOG4CXX_INFO(m_log, "200OK Did not find " + info->m_callId);
	//}
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

void RtpSessions::UpdateSessionWithCallInfo(SkCallInfoStruct* callInfo, RtpSessionRef& session)
{
	switch(callInfo->callType)
	{
	case SKINNY_CALL_TYPE_INBOUND:
	case SKINNY_CALL_TYPE_FORWARD:
		session->m_localParty = callInfo->calledParty;
		session->m_remoteParty = callInfo->callingParty;
		session->m_direction = CaptureEvent::DirIn;
		break;
	case SKINNY_CALL_TYPE_OUTBOUND:
		session->m_localParty = callInfo->callingParty;
		session->m_remoteParty = callInfo->calledParty;
		session->m_direction = CaptureEvent::DirOut;
		break;
	default:
		session->m_localParty = callInfo->calledParty;
		session->m_remoteParty = callInfo->callingParty;		
	}
}


void RtpSessions::ReportSkinnyCallInfo(SkCallInfoStruct* callInfo, IpHeaderStruct* ipHeader)
{
	CStdString callId = GenerateSkinnyCallId(ipHeader->ip_dest, callInfo->callId);
	std::map<CStdString, RtpSessionRef>::iterator pair;
	pair = m_byCallId.find(callId);

	if (pair != m_byCallId.end())
	{
		// CM can resend the same message more than once in a session, 
		// just update timestamp
		RtpSessionRef existingSession = pair->second;
		existingSession->m_skinnyLastCallInfoTime = ACE_OS::gettimeofday();
		if(DLLCONFIG.m_skinnyAllowCallInfoUpdate)
		{
			UpdateSessionWithCallInfo(callInfo, existingSession);
		}
		return;
	}

	// create new session and insert into the callid map
	CStdString trackingId = m_alphaCounter.GetNext();
	RtpSessionRef session(new RtpSession(trackingId));
	session->m_callId = callId;
	session->m_endPointIp = ipHeader->ip_dest;	// CallInfo message always goes from CM to endpoint 
	session->m_protocol = RtpSession::ProtSkinny;
	UpdateSessionWithCallInfo(callInfo, session);

	if(m_log->isInfoEnabled())
	{
		CStdString logMsg;
		CStdString dir = CaptureEvent::DirectionToString(session->m_direction);
		char szEndPointIp[16];
		ACE_OS::inet_ntop(AF_INET, (void*)&session->m_endPointIp, szEndPointIp, sizeof(szEndPointIp));

		logMsg.Format("[%s] Skinny CallInfo callId %s local:%s remote:%s dir:%s endpoint:%s", session->m_trackingId,
			session->m_callId, session->m_localParty, session->m_remoteParty, dir, szEndPointIp);
		LOG4CXX_INFO(m_log, logMsg);
	}

	m_byCallId.insert(std::make_pair(session->m_callId, session));

	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

}

//RtpSessionRef RtpSessions::findByEndpointIpUsingIpAndPort(struct in_addr endpointIpAddr)
//{
//	RtpSessionRef session;
//	std::map<CStdString, RtpSessionRef>::iterator pair;
//
//        // Scan all sessions and try to find a session on the same IP endpoint
//	// This function uses the m_byIpAndPort mapping unlike findByEndpointIp()
//
//        for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end(); pair++)
//        {
//                RtpSessionRef tmpSession = pair->second;
//
//		if((unsigned int)tmpSession->m_endPointIp.s_addr == (unsigned int)endpointIpAddr.s_addr)
//                {
//                        session = tmpSession;
//                        break;
//                }
//	}
//
//	return session;
//}

// Find a session by Skinny endpoint IP address. 
// If a passThruPartyId is supplied, only returns session matching both criteria
RtpSessionRef RtpSessions::findByEndpointIp(struct in_addr endpointIpAddr, int passThruPartyId)
{
	RtpSessionRef session;
	std::map<CStdString, RtpSessionRef>::iterator pair;

	// Scan all sessions and try to find a session on the same IP endpoint
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		RtpSessionRef tmpSession = pair->second;

		if((unsigned int)tmpSession->m_endPointIp.s_addr == (unsigned int)endpointIpAddr.s_addr)
		{
			if(passThruPartyId == 0 || tmpSession->m_skinnyPassThruPartyId == passThruPartyId)
			{
				session = tmpSession;
				break;
			}
		}
	}

	return session;
}

RtpSessionRef RtpSessions::findNewestByEndpointIp(struct in_addr endpointIpAddr)
{
	RtpSessionRef session;
	std::map<CStdString, RtpSessionRef>::iterator pair;

	// Scan all sessions and try to find the most recently signalled session on the IP endpoint
	// This always scans the entire session list, might be good to index sessions by endpoint at some point
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		RtpSessionRef tmpSession = pair->second;

		if((unsigned int)tmpSession->m_endPointIp.s_addr == (unsigned int)endpointIpAddr.s_addr)
		{
			if(session.get())
			{
				if(tmpSession->m_skinnyLastCallInfoTime > session->m_skinnyLastCallInfoTime)
				{
					session = tmpSession;
				}
			}
			else
			{
				session = tmpSession;
			}
		}
	}

	return session;
}

//bool RtpSessions::ChangeCallId(RtpSessionRef& session, unsigned int newId)
//{
//	bool result = false;
//	if(newId)
//	{
//		CStdString newCallId = GenerateSkinnyCallId(session->m_endPointIp, newId);
//
//		std::map<CStdString, RtpSessionRef>::iterator pair = m_byCallId.find(newCallId);
//		if (pair == m_byCallId.end())
//		{
//			// Ok, no session exists with the new Call ID, go ahead
//			result = true;
//			CStdString oldCallId = session->m_callId;
//			m_byCallId.erase(oldCallId);
//			session->m_callId = newCallId;
//			m_byCallId.insert(std::make_pair(newCallId, session));
//
//			if(m_log->isInfoEnabled())
//			{
//				CStdString logMsg;
//				logMsg.Format("[%s] callId %s becomes %s", session->m_trackingId, oldCallId, newCallId);
//				LOG4CXX_INFO(m_log, logMsg);
//			}
//		}
//		else
//		{
//			// a session already exists with the new Call ID, ignore
//		}
//	}
//	return result;
//}


void RtpSessions::SetMediaAddress(RtpSessionRef& session, struct in_addr mediaIp, unsigned short mediaPort)
{
	CStdString logMsg;
	CStdString ipAndPort;
	char szMediaIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&mediaIp, szMediaIp, sizeof(szMediaIp));
	ipAndPort.Format("%s,%u", szMediaIp, mediaPort);
	bool doChangeMediaAddress = true;

	std::map<CStdString, RtpSessionRef>::iterator pair = m_byIpAndPort.find(ipAndPort);
	if (pair != m_byIpAndPort.end())
	{
		// A session exists on the same IP+port
		RtpSessionRef oldSession = pair->second;
		if(oldSession->m_protocol == RtpSession::ProtRawRtp)
		{
			logMsg.Format("[%s] on %s replaces [%s]", 
							session->m_trackingId, ipAndPort, oldSession->m_trackingId); 
			LOG4CXX_INFO(m_log, logMsg);
			Stop(oldSession);
		}
		else
		{
			doChangeMediaAddress = false;
			logMsg.Format("[%s] on %s will not replace [%s]", 
							session->m_trackingId, ipAndPort, oldSession->m_trackingId); 
			LOG4CXX_INFO(m_log, logMsg);
		}
	}
	if(doChangeMediaAddress)
	{
		if(m_log->isInfoEnabled())
		{
			char szEndPointIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&session->m_endPointIp, szEndPointIp, sizeof(szEndPointIp));
			logMsg.Format("[%s] media address:%s callId:%s endpoint:%s", session->m_trackingId, ipAndPort, session->m_callId, szEndPointIp);
			LOG4CXX_INFO(m_log, logMsg);
		}

		session->m_ipAndPort = ipAndPort;
		m_byIpAndPort.insert(std::make_pair(session->m_ipAndPort, session));

		CStdString numSessions = IntToString(m_byIpAndPort.size());
		LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);
	}
}

CStdString RtpSessions::GenerateSkinnyCallId(struct in_addr endpointIp, unsigned int callId)
{
	char szEndPointIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&endpointIp, szEndPointIp, sizeof(szEndPointIp));
	CStdString skinnyCallId;
	skinnyCallId.Format("%u@%s", callId, szEndPointIp);
	return skinnyCallId;
}

void RtpSessions::ReportSkinnyOpenReceiveChannelAck(SkOpenReceiveChannelAckStruct* openReceive)
{
	if(DLLCONFIG.m_skinnyIgnoreOpenReceiveChannelAck)
	{
		return;
	}
	RtpSessionRef session = findNewestByEndpointIp(openReceive->endpointIpAddr);
	if(session.get())
	{
		if(session->m_ipAndPort.size() == 0 || DLLCONFIG.m_skinnyDynamicMediaAddress)
		{
			session->m_skinnyPassThruPartyId = openReceive->passThruPartyId;
			SetMediaAddress(session, openReceive->endpointIpAddr, openReceive->endpointTcpPort);
		}
		else
		{
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] OpenReceiveChannelAck: already got media address");
		}
	}
	else
	{
		// Discard because we have not seen any CallInfo Message before
		LOG4CXX_INFO(m_log, "Skinny OpenReceiveChannelAck without a CallInfoMessage");
	}
}


void RtpSessions::ReportSkinnyStartMediaTransmission(SkStartMediaTransmissionStruct* startMedia, IpHeaderStruct* ipHeader)
{
	RtpSessionRef session = findNewestByEndpointIp(ipHeader->ip_dest);

	if(session.get())
	{
		if(session->m_ipAndPort.size() == 0 || DLLCONFIG.m_skinnyDynamicMediaAddress)
		{
			session->m_skinnyPassThruPartyId = startMedia->passThruPartyId;
			SetMediaAddress(session, startMedia->remoteIpAddr, startMedia->remoteTcpPort);
		}
		else
		{
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] StartMediaTransmission: already got media address");
		}
	}
	else
	{
		// Discard because we have not seen any CallInfo Message before
		LOG4CXX_INFO(m_log, "Skinny StartMediaTransmission without a CallInfoMessage");
	}
}

void RtpSessions::ReportSkinnyStopMediaTransmission(SkStopMediaTransmissionStruct* stopMedia, IpHeaderStruct* ipHeader)
{
	if(DLLCONFIG.m_skinnyIgnoreStopMediaTransmission)
	{
		return;
	}
	CStdString conferenceId;
	CStdString passThruPartyId;
	CStdString skinnyCallId;
	std::map<CStdString, RtpSessionRef>::iterator pair = m_byCallId.end();
	RtpSessionRef session;

	// Try to locate the session using 1. conferenceId or 2. endpoint/passThruPartyId
	if(stopMedia->conferenceId != 0)
	{
		conferenceId = IntToString(stopMedia->conferenceId);
		skinnyCallId = GenerateSkinnyCallId(ipHeader->ip_dest, stopMedia->conferenceId);
		pair = m_byCallId.find(skinnyCallId);
		if (pair != m_byCallId.end())
		{
			session = pair->second;
		}
	}
	if(session.get() == NULL && stopMedia->passThruPartyId != 0)
	{
		session = findByEndpointIp(ipHeader->ip_dest, stopMedia->passThruPartyId);
	}
	if(session.get())
	{
		if(session->m_onHold)
		{
			if(m_log->isInfoEnabled())
			{
				CStdString logMsg;
				logMsg.Format("[%s] Ignoring Skinny StopMedia conferenceId:%s passThruPartyId:%s because on hold", session->m_trackingId, conferenceId, passThruPartyId);
				LOG4CXX_INFO(m_log, logMsg);
			}
		}
		else
		{
			if(m_log->isInfoEnabled())
			{
				CStdString logMsg;
				logMsg.Format("[%s] Skinny StopMedia conferenceId:%s passThruPartyId:%s", session->m_trackingId, conferenceId, passThruPartyId);
				LOG4CXX_INFO(m_log, logMsg);
			}

			Stop(session);
		}
	}
}

void RtpSessions::ReportSkinnyLineStat(SkLineStatStruct* lineStat, IpHeaderStruct* ipHeader)
{
	if(strlen(lineStat->lineDirNumber) > 1)
	{
		EndpointInfoRef endpoint;
		std::map<unsigned int, EndpointInfoRef>::iterator pair;
		pair = m_endpoints.find((unsigned int)(ipHeader->ip_dest.s_addr));
		if(pair != m_endpoints.end())
		{
			// Update the existing endpoint	info
			endpoint = pair->second;
			endpoint->m_extension = lineStat->lineDirNumber;

		}
		else
		{
			// Create endpoint info for the new endpoint
			endpoint.reset(new EndpointInfo());
			endpoint->m_extension = lineStat->lineDirNumber;
			m_endpoints.insert(std::make_pair((unsigned int)(ipHeader->ip_dest.s_addr), endpoint));
		}
		if(endpoint.get())
		{
			CStdString logMsg;
			char szEndpointIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_dest, szEndpointIp, sizeof(szEndpointIp));

			logMsg.Format("Extension:%s is on endpoint:%s", endpoint->m_extension, szEndpointIp);
			LOG4CXX_INFO(m_log, logMsg);
		}
	}
}

void RtpSessions::ReportSkinnySoftKeyHold(SkSoftKeyEventMessageStruct* skEvent, IpHeaderStruct* ipHeader)
{
	RtpSessionRef session;
	CStdString logMsg;

	std::map<CStdString, RtpSessionRef>::iterator pair;
	CStdString callId = GenerateSkinnyCallId(ipHeader->ip_src, skEvent->callIdentifier);
	pair = m_byCallId.find(callId);
	if (pair != m_byCallId.end())
	{
		session = pair->second;
	}
	if(session.get())
	{
		session->m_onHold = true;
		logMsg.Format("[%s] Going on hold due to SoftKeyEvent: HOLD", session->m_trackingId);
		LOG4CXX_INFO(m_log, logMsg);
	}
	else
	{
		char szEndpointIp[16];

		ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_src, szEndpointIp, sizeof(szEndpointIp));
		logMsg.Format("Received HOLD notification from endpoint %s but couldn't find any valid RTP Session",
				szEndpointIp);

		LOG4CXX_WARN(m_log, logMsg);
	}
}

void RtpSessions::ReportSkinnySoftKeyResume(SkSoftKeyEventMessageStruct* skEvent, IpHeaderStruct* ipHeader)
{
	RtpSessionRef session;
	CStdString logMsg;

	std::map<CStdString, RtpSessionRef>::iterator pair;
	CStdString callId = GenerateSkinnyCallId(ipHeader->ip_src, skEvent->callIdentifier);
	pair = m_byCallId.find(callId);
	if (pair != m_byCallId.end())
	{
		session = pair->second;
	}
    if(session.get())
    {
		session->m_onHold = false;
		logMsg.Format("[%s] Going off hold due to SoftKeyEvent: RESUME", session->m_trackingId);
		LOG4CXX_INFO(m_log, logMsg);
    }
    else
    {
        char szEndpointIp[16];

        ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_src, szEndpointIp, sizeof(szEndpointIp));
        logMsg.Format("Received RESUME notification from endpoint %s but couldn't find any valid RTP Session",
                        szEndpointIp);

        LOG4CXX_WARN(m_log, logMsg);
    }
}

EndpointInfoRef RtpSessions::GetEndpointInfo(struct in_addr endpointIp)
{
	std::map<unsigned int, EndpointInfoRef>::iterator pair;
	pair = m_endpoints.find((unsigned int)(endpointIp.s_addr));
	if(pair != m_endpoints.end())
	{
		return pair->second;
	}
	return EndpointInfoRef();
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
	RtpSessionRef session;
	CStdString logMsg;

	int sourcePort = rtpPacket->m_sourcePort;
	int destPort = rtpPacket->m_destPort;

	if(DLLCONFIG.m_sangomaEnable && sourcePort == destPort)
	{
		if(sourcePort > DLLCONFIG.m_sangomaTxTcpPortStart)
		{
			// This is a TX packet
			sourcePort = sourcePort - DLLCONFIG.m_sangomaTcpPortDelta;
			rtpPacket->m_sourcePort = sourcePort;
			// flip the least significant bit of the most significant byte
			rtpPacket->m_sourceIp.s_addr ^= 0x00000001;
		}
		else
		{
			// This is an RX packet
			sourcePort = sourcePort + DLLCONFIG.m_sangomaTcpPortDelta;
			rtpPacket->m_sourcePort = sourcePort;
			// flip the least significant bit of the most significant byte
			rtpPacket->m_destIp.s_addr ^= 0x00000001;
		}
	}

	// Add RTP packet to session with matching source or dest IP+Port. 
	// On CallManager there might be two sessions with two different CallIDs for one 
	// phone call, so this RTP packet can potentially be reported to two sessions.

	// Does a session exist with this source Ip+Port
	CStdString port = IntToString(sourcePort);
	char szSourceIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_sourceIp, szSourceIp, sizeof(szSourceIp));
	CStdString sourceIpAndPort = CStdString(szSourceIp) + "," + port;
	std::map<CStdString, RtpSessionRef>::iterator pair;

	pair = m_byIpAndPort.find(sourceIpAndPort);
	if (pair != m_byIpAndPort.end())
	{
		session1 = pair->second;
		if (session1.get() != NULL)
		{
			// Found a session give it the RTP packet info
			session = session1;
			if(session1->AddRtpPacket(rtpPacket))
			{
				numSessionsFound++;
			}
			else
			{
				// RTP discontinuity detected
				Stop(session1);
			}
		}
	}

	// Does a session exist with this destination Ip+Port
	port = IntToString(destPort);
	char szDestIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_destIp, szDestIp, sizeof(szDestIp));
	CStdString destIpAndPort = CStdString(szDestIp) + "," + port;

	pair = m_byIpAndPort.find(destIpAndPort);
	if (pair != m_byIpAndPort.end())
	{
		session2 = pair->second;
		if (session2.get() != NULL)
		{
			// Found a session give it the RTP packet info
			session = session2;
			if(session2->AddRtpPacket(rtpPacket))
			{
				numSessionsFound++;
			}
			else
			{
				// RTP discontinuity detected
				Stop(session2);
			}
		}
	}

	if(numSessionsFound == 2 && session1.get() != session2.get())
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
			if(session1->m_numRtpPackets < session2->m_numRtpPackets && session1->m_numRtpPackets < 5)
			{
				mergerSession = session2;
				mergeeSession = session1;
			}
			else if(session2->m_numRtpPackets < session1->m_numRtpPackets && session2->m_numRtpPackets < 5)
			{
				mergerSession = session1;
				mergeeSession = session2;
			}
			else if(session1->m_direction == CaptureEvent::DirOut)
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

		// Previous rules can be overruled by configuration, useful for merging sessions by "PSTN trunks"
		if(DLLCONFIG.IsRtpTrackingIpAddress(rtpPacket->m_sourceIp))
		{
			mergerSession = session1;
			mergeeSession = session2;	
		}
		else if(DLLCONFIG.IsRtpTrackingIpAddress(rtpPacket->m_destIp))
		{
			mergerSession = session2;
			mergeeSession = session1;	
		}

		if(m_log->isInfoEnabled())
		{
			CStdString debug;
			debug.Format("Merging [%s] %s with callid:%s into [%s] %s with callid:%s",
				mergeeSession->m_trackingId, mergeeSession->m_ipAndPort, mergeeSession->m_callId,
				mergerSession->m_trackingId, mergerSession->m_ipAndPort, mergerSession->m_callId);
			LOG4CXX_INFO(m_log, debug);
		}
		Stop(mergeeSession);
	}
	else if(numSessionsFound == 1 )
	{
		if (session->m_numRtpPackets == 1 && session->m_protocol == RtpSession::ProtSip)
		{
			// This was the first packet of the session, check whether it is tracked on the right IP address
			in_addr trackingIp;
			unsigned short trackingPort = 0;
			if(DLLCONFIG.IsRtpTrackingIpAddress(rtpPacket->m_destIp))
			{
				trackingIp = rtpPacket->m_destIp;
				trackingPort = rtpPacket->m_destPort;
			}
			else if(DLLCONFIG.IsRtpTrackingIpAddress(rtpPacket->m_sourceIp))
			{
				trackingIp = rtpPacket->m_sourceIp;
				trackingPort = rtpPacket->m_sourcePort;
			}
			if(trackingPort)
			{
				// Remove session from IP+Port index
				m_byIpAndPort.erase(session->m_ipAndPort);

				// ... and reinsert
				SetMediaAddress(session, trackingIp, trackingPort);
			}
		}
	}
	else if((numSessionsFound == 0) && (CONFIG.m_lookBackRecording == true))
	{
		// create new Raw RTP session and insert into IP+Port map
		CStdString trackingId = m_alphaCounter.GetNext();
		RtpSessionRef session(new RtpSession(trackingId));
		session->m_protocol = RtpSession::ProtRawRtp;

		// Make sure the session is tracked by the right IP address
		CStdString ipAndPort;
		if(DLLCONFIG.IsRtpTrackingIpAddress(rtpPacket->m_sourceIp))
		{
			ipAndPort = sourceIpAndPort;
		}
		else if(DLLCONFIG.m_sangomaEnable)
		{
			ipAndPort = sourceIpAndPort;
		}
		else
		{
			ipAndPort = destIpAndPort;
		}
		session->m_ipAndPort = ipAndPort;	// (1) In the case of a PSTN Gateway automated answer, This is the destination IP+Port of the first packet which is good, because it is usually the IP+Port of the PSTN Gateway.

		session->AddRtpPacket(rtpPacket);
		m_byIpAndPort.insert(std::make_pair(ipAndPort, session));

		CStdString numSessions = IntToString(m_byIpAndPort.size());
		LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

		LOG4CXX_INFO(m_log, "[" + trackingId + "] created by RTP packet");
	}
}

void RtpSessions::StopAll()
{
	time_t forceExpiryTime = time(NULL) + 2*DLLCONFIG.m_rtpSessionWithSignallingTimeoutSec;
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
		int timeoutSeconds = 0;
		if(session->m_protocol == RtpSession::ProtRawRtp)
		{
			timeoutSeconds = DLLCONFIG.m_rtpSessionTimeoutSec;
		}
		else
		{
			if(session->m_onHold) {
				timeoutSeconds = DLLCONFIG.m_rtpSessionOnHoldTimeOutSec;
			} else {
				timeoutSeconds = DLLCONFIG.m_rtpSessionWithSignallingTimeoutSec;
			}
		}
		if((now - session->m_lastUpdated) > timeoutSeconds)
		{
			toDismiss.push_back(session);
		}
	}

	// discard inactive sessions
	for (std::list<RtpSessionRef>::iterator it = toDismiss.begin(); it != toDismiss.end() ; it++)
	{
		RtpSessionRef session = *it;
		LOG4CXX_INFO(m_log,  "[" + session->m_trackingId + "] " + session->m_ipAndPort + " Expired");
		Stop(session);
	}

	// Go round the callId session index and find inactive sessions
	toDismiss.clear();
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		RtpSessionRef session = pair->second;

		if(session->m_onHold) {
			if((now - session->m_lastUpdated) > DLLCONFIG.m_rtpSessionOnHoldTimeOutSec)
	                {
        	                toDismiss.push_back(session);
                	}
		} else {
			if((now - session->m_lastUpdated) > DLLCONFIG.m_rtpSessionWithSignallingTimeoutSec)
			{
				toDismiss.push_back(session);
			}
		}
	}

	// discard inactive sessions
	for (std::list<RtpSessionRef>::iterator it2 = toDismiss.begin(); it2 != toDismiss.end() ; it2++)
	{
		RtpSessionRef session = *it2;
		LOG4CXX_INFO(m_log,  "[" + session->m_trackingId + "] " + session->m_ipAndPort + " Expired");
		Stop(session);
	}
}

void RtpSessions::StartCapture(CStdString& party)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if (session->PartyMatches(party))
		{
			session->m_keep = true;
			found = true;
		}
	}

	if(found)
	{
		logMsg.Format("[%s] Started capture, party:%s", session->m_trackingId, party);
	}	
	else
	{
		logMsg.Format("No session has party %s", party);
	}
	
	LOG4CXX_INFO(m_log, logMsg);
}

//==========================================================
SipInviteInfo::SipInviteInfo()
{
	m_fromRtpIp.s_addr = 0;
	m_validated = false;
	m_attrSendonly = false;
}

void SipInviteInfo::ToString(CStdString& string)
{
	char fromRtpIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_fromRtpIp, fromRtpIp, sizeof(fromRtpIp));

	char senderIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));

	char receiverIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	CStdString senderMac, receiverMac;

	MemMacToHumanReadable((unsigned char*)m_senderMac, senderMac);
	MemMacToHumanReadable((unsigned char*)m_receiverMac, receiverMac);

	string.Format("sender:%s from:%s RTP:%s,%s to:%s rcvr:%s callid:%s smac:%s rmac:%s", senderIp, m_from, fromRtpIp, m_fromRtpPort, m_to, receiverIp, m_callId, senderMac, receiverMac);
}

//==========================================================
SipFailureMessageInfo::SipFailureMessageInfo()
{
	m_senderIp.s_addr = 0;
	m_receiverIp.s_addr = 0;
	memset(m_senderMac, 0, sizeof(m_senderMac));
	memset(m_receiverMac, 0, sizeof(m_receiverMac));
}

void SipFailureMessageInfo::ToString(CStdString& string)
{
	char senderIp[16], receiverIp[16];
	CStdString senderMac, receiverMac;

	MemMacToHumanReadable((unsigned char*)m_senderMac, senderMac);
	MemMacToHumanReadable((unsigned char*)m_receiverMac, receiverMac);
	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	string.Format("sender:%s rcvr:%s smac:%s rmac:%s callid:%s errorcode:%s errorstr:\"%s\"", senderIp, receiverIp, senderMac, receiverMac, m_callId, m_errorCode, m_errorString);
}

Sip200OkInfo::Sip200OkInfo()
{
	m_mediaIp.s_addr = 0;
	m_hasSdp = false;
}

