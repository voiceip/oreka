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
	m_rtcpLocalParty = false;
	m_rtcpRemoteParty = false;
	m_remotePartyReported = false;
	m_localPartyReported = false;
	m_sessionTelephoneEventPtDefined = false;
	m_rtpIp.s_addr = 0;
	m_skinnyLineInstance = 0;
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

void RtpSession::ReportRtcpSrcDescription(RtcpSrcDescriptionPacketInfoRef& rtcpInfo)
{
	if(!m_rtcpLocalParty)
	{
		m_rtcpLocalParty = true;

		if(DLLCONFIG.m_inInMode == true)
		{
			char realm[256], *d = NULL;

			memset(realm, 0, sizeof(realm));
			ACE_OS::snprintf(realm, sizeof(realm), "%s", (PCSTR)rtcpInfo->m_cnameDomain);

			if((d = strchr(realm, '.')))
			{
				*d = '\0';
			}

			m_localParty.Format("%s@%s", rtcpInfo->m_cnameUsername, realm);
		}
		else
		{
			m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(rtcpInfo->m_cnameUsername);
		}
			
		LOG4CXX_INFO(m_log, "[" + m_trackingId + "] Set local party to RTCP CNAME:" + m_localParty);
		
		// In some cases, we obtain this information after the session
		// has long started.  Therefore, in order for the reporting
		// to reflect the RTCP CNAME information, we need to report.
		if(m_localPartyReported)
		{
			CaptureEventRef event(new CaptureEvent());
			event->m_type = CaptureEvent::EtLocalParty;
			event->m_value = m_localParty;
			g_captureEventCallBack(event, m_capturePort);
		}
	}
	else if(!m_rtcpRemoteParty)
	{
		CStdString testParty;

		if(DLLCONFIG.m_inInMode == true)
		{
			char realm[256], *d = NULL;

			memset(realm, 0, sizeof(realm));
			ACE_OS::snprintf(realm, sizeof(realm), "%s", (PCSTR)rtcpInfo->m_cnameDomain);

			if((d = strchr(realm, '.')))
			{
				*d = '\0';
			}

			testParty.Format("%s@%s", rtcpInfo->m_cnameUsername, realm);
		}
		else
		{
			testParty = rtcpInfo->m_cnameUsername;
		}
			
		if(testParty.Equals(m_localParty) == false)
		{
			m_rtcpRemoteParty = true;

			if(DLLCONFIG.m_inInMode == true)
			{
				m_remoteParty = testParty;
			}
			else
			{
				m_remoteParty = rtcpInfo->m_cnameUsername;
			}

			LOG4CXX_INFO(m_log, "[" + m_trackingId + "] Set remote party to RTCP CNAME:" + m_remoteParty);

			if(m_remotePartyReported)
			{
				CaptureEventRef event(new CaptureEvent());
				event->m_type = CaptureEvent::EtRemoteParty;
				event->m_value = m_remoteParty;
				g_captureEventCallBack(event, m_capturePort);
			}
		}
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
		m_capturePort = m_trackingId;
	}

	if(sourceIsLocal)
	{
		if(!m_rtcpLocalParty)
		{
	                /* With Raw RTP, the local party is not obtained through any intelligent
        	         * signalling so we should probably do this check here? */
                	if(DLLCONFIG.m_useMacIfNoLocalParty)
	                {
        	                MemMacToHumanReadable((unsigned char*)rtpPacket->m_sourceMac, m_localParty);
                	}
	                else
        	        {
				CStdString lp(szSourceIp);
                	        m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(lp);
	                }
		}
		if(!m_rtcpRemoteParty)
		{
			m_remoteParty = szDestIp;
		}

		m_localIp = rtpPacket->m_sourceIp;
		m_remoteIp = rtpPacket->m_destIp;
		memcpy(m_localMac, rtpPacket->m_sourceMac, sizeof(m_localMac));
		memcpy(m_remoteMac, rtpPacket->m_destMac, sizeof(m_remoteMac));
	}
	else
	{
		if(!m_rtcpLocalParty)
		{
	                /* With Raw RTP, the local party is not obtained through any intelligent
        	         * signalling so we should probably do this check here? */
			if(DLLCONFIG.m_useMacIfNoLocalParty)
        	        {
                	        MemMacToHumanReadable((unsigned char*)rtpPacket->m_destMac, m_localParty);
			}
			else
			{
				CStdString lp(szDestIp);
				m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(lp);
			}
		}
		if(!m_rtcpRemoteParty)
		{
			m_remoteParty = szSourceIp;
		}

		m_localIp = rtpPacket->m_destIp;
		m_remoteIp = rtpPacket->m_sourceIp;
                memcpy(m_localMac, rtpPacket->m_destMac, sizeof(m_localMac));
                memcpy(m_remoteMac, rtpPacket->m_sourceMac, sizeof(m_remoteMac));
	}
}

bool RtpSession::MatchesSipDomain(CStdString& domain)
{
	for(std::list<CStdString>::iterator it = DLLCONFIG.m_sipDomains.begin(); it != DLLCONFIG.m_sipDomains.end(); it++)
	{
		CStdString element = *it;

		if(element.CompareNoCase(domain) == 0)
		{
			return true;
		}
	}
	return false;
}

bool RtpSession::IsInSkinnyReportingList(CStdString item)
{
	for(std::list<CStdString>::iterator it = DLLCONFIG.m_skinnyReportTags.begin(); it != DLLCONFIG.m_skinnyReportTags.end(); it++)
	{
		CStdString element = *it;

		if(element.CompareNoCase(item) == 0)
		{
			return true;
		}
	}

	return false;
}

void RtpSession::ProcessMetadataSipIncoming()
{
	if((DLLCONFIG.m_sipRequestUriAsLocalParty == true) && (m_invite->m_requestUri.CompareNoCase(m_invite->m_to) != 0))
	{
		m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_requestUri);
		m_remoteParty = m_invite->m_from;
		m_direction = CaptureEvent::DirIn;
		m_localEntryPoint = m_invite->m_to;
	}
	else
	{
		m_remoteParty = m_invite->m_from;
		m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_to);
		m_direction = CaptureEvent::DirIn;
	}

	m_remotePartyName = m_invite->m_fromName;
	m_localPartyName = m_invite->m_toName;

	if(!m_remotePartyName.size())
	{
		m_remotePartyName = m_remoteParty;
	}
	if(!m_localPartyName.size())
	{
		m_localPartyName = m_localParty;
	}

	m_capturePort = m_trackingId;
	m_localIp = m_inviteeIp;
	m_remoteIp = m_invitorIp;
	memcpy(m_localMac, m_inviteeMac, sizeof(m_localMac));
	memcpy(m_remoteMac, m_invitorMac, sizeof(m_remoteMac));
}

void RtpSession::ProcessMetadataSipOutgoing()
{
	if((DLLCONFIG.m_sipRequestUriAsLocalParty == true) && (m_invite->m_requestUri.CompareNoCase(m_invite->m_to) != 0))
	{
		m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_requestUri);
		m_remoteParty = m_invite->m_from;
		m_direction = CaptureEvent::DirIn;
		m_localEntryPoint = m_invite->m_to;
	}
	else
	{
		m_remoteParty = m_invite->m_to;
		m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_from);
		m_direction = CaptureEvent::DirOut;
	}

	m_remotePartyName = m_invite->m_toName;
	m_localPartyName = m_invite->m_fromName;

	if(!m_remotePartyName.size())
	{
		m_remotePartyName = m_remoteParty;
	}
	if(!m_localPartyName.size())
	{
		m_localPartyName = m_localParty;
	}

	m_capturePort = m_trackingId;
	m_localIp = m_invitorIp;
	m_remoteIp = m_inviteeIp;
	memcpy(m_localMac, m_invitorMac, sizeof(m_localMac));
	memcpy(m_remoteMac, m_inviteeMac, sizeof(m_remoteMac));
}

void RtpSession::UpdateMetadataSkinny()
{
	// Report Local party
	CaptureEventRef event(new CaptureEvent());
	event->m_type = CaptureEvent::EtLocalParty;
	if(DLLCONFIG.m_skinnyNameAsLocalParty == true && m_localPartyName.size())
	{
		event->m_value = m_localPartyName;
	}
	else
	{
		event->m_value = m_localParty;
	}
	g_captureEventCallBack(event, m_capturePort);

	// Report remote party
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtRemoteParty;
	event->m_value = m_remoteParty;
	g_captureEventCallBack(event, m_capturePort);

	// Trigger metadata update
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtUpdate;
	g_captureEventCallBack(event, m_capturePort);
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
		if((DLLCONFIG.m_sipRequestUriAsLocalParty == true) && (m_invite->m_requestUri.CompareNoCase(m_invite->m_to) != 0))
		{
			m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_requestUri);
			m_remoteParty = m_invite->m_from;
			m_direction = CaptureEvent::DirIn;
			m_localEntryPoint = m_invite->m_to;
		}
		else
		{
			m_remoteParty = invite->m_from;
			m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(invite->m_to);
		}

		m_remotePartyName = m_invite->m_fromName;
		m_localPartyName = m_invite->m_toName;

		if(!m_remotePartyName.size())
		{
			m_remotePartyName = m_remoteParty;
		}
		if(!m_localPartyName.size())
		{
			m_localPartyName = m_localParty;
		}

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

		if(DLLCONFIG.m_sipReportNamesAsTags == true)
		{
			CStdString key, value;

			key = "localname";
			value = m_localPartyName;
			event.reset(new CaptureEvent());
			event->m_type = CaptureEvent::EtKeyValue;
			event->m_key = key;
			event->m_value = value;
			g_captureEventCallBack(event, m_capturePort);

			key = "remotename";
			value = m_remotePartyName;
			event.reset(new CaptureEvent());
			event->m_type = CaptureEvent::EtKeyValue;
			event->m_key = key;
			event->m_value = value;
			g_captureEventCallBack(event, m_capturePort);
		}

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

bool RtpSession::MatchesReferenceAddresses(struct in_addr inAddr)
{
	char szInAddr[16];

	ACE_OS::inet_ntop(AF_INET, (void*)&inAddr, szInAddr, sizeof(szInAddr));
	for(std::list<CStdString>::iterator it = DLLCONFIG.m_sipDirectionReferenceIpAddresses.begin(); it != DLLCONFIG.m_sipDirectionReferenceIpAddresses.end(); it++)
	{
		CStdString element = *it;

		if(element.CompareNoCase(szInAddr) == 0)
		{
			return true;
		}
	}

	return false;
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

	if(DLLCONFIG.m_sipDirectionReferenceIpAddresses.size())
	{
		if(MatchesReferenceAddresses(m_invite->m_senderIp))
		{
			ProcessMetadataSipIncoming();
		}
		else
		{
			ProcessMetadataSipOutgoing();
		}
	}
	else
	{
		// work out capture port and direction
		if(MatchesSipDomain(m_invite->m_fromDomain) && MatchesSipDomain(m_invite->m_toDomain))
		{
			// Both match at least one entry
			ProcessMetadataSipOutgoing();
		}
		else if(MatchesSipDomain(m_invite->m_fromDomain))
		{
			// Only from domain matches
			ProcessMetadataSipOutgoing();
		}
		else if(MatchesSipDomain(m_invite->m_toDomain))
		{
			// Only to domain matches
			ProcessMetadataSipIncoming();
		}
		else if(MatchesStringList(m_invite->m_userAgent, DLLCONFIG.m_sipDirectionReferenceUserAgents))
		{
			ProcessMetadataSipIncoming();
		}
		else
		{
			// Default to outgoing whereby m_from is the local party and m_to is
			// the remote party
			ProcessMetadataSipOutgoing();
		}
	}
}

void RtpSession::ProcessMetadataSkinny(RtpPacketInfoRef& rtpPacket)
{
	//  In Skinny we always want the endpoint (phone) to be used as a local IP address
	char szEndpointIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_endPointIp, szEndpointIp, sizeof(szEndpointIp));
	m_capturePort = m_trackingId;

	if( ( (unsigned int)m_endPointIp.s_addr) == ((unsigned int)rtpPacket->m_destIp.s_addr) )
	{
		//m_capturePort.Format("%s,%u", szEndpointIp, rtpPacket->m_destPort);

		m_localIp = rtpPacket->m_destIp;
		m_remoteIp = rtpPacket->m_sourceIp;
		memcpy(m_localMac, rtpPacket->m_destMac, sizeof(m_localMac));
		memcpy(m_remoteMac, rtpPacket->m_sourceMac, sizeof(m_remoteMac));
	}
	else
	{
		//m_capturePort.Format("%s,%u", szEndpointIp, rtpPacket->m_sourcePort);

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

	if(DLLCONFIG.m_localPartyForceLocalIp)
	{
		CStdString lp(szLocalIp);
		m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(lp);
	}
	// Check if we don't have the local party based on the endpoint IP address
	else if(m_localParty.IsEmpty())
	{
		if(m_protocol == ProtSkinny)
		{
			EndpointInfoRef endpointInfo = RtpSessionsSingleton::instance()->GetEndpointInfo(m_endPointIp);
			if(endpointInfo.get())
			{
				m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(endpointInfo->m_extension);
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
			CStdString lp(szLocalIp);
			m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(lp);
		}
	}

	if(DLLCONFIG.m_localPartyForceLocalMac)
	{
		m_localParty = "";
		MemMacToHumanReadable((unsigned char*)m_localMac, m_localParty);
		m_localParty = RtpSessionsSingleton::instance()->GetLocalPartyMap(m_localParty);
	}

	// Report Local party
	CaptureEventRef event(new CaptureEvent());
	event->m_type = CaptureEvent::EtLocalParty;
	// TODO, we might consider deprecating m_skinnyNameAsLocalParty in favour of m_localPartyUseName at some point
	if( ( m_protocol == ProtSkinny && DLLCONFIG.m_skinnyNameAsLocalParty == true && m_localPartyName.size() )   || 
		( (DLLCONFIG.m_partiesUseName || DLLCONFIG.m_localPartyUseName) == true && m_localPartyName.size() )		 )
	{
		event->m_value = m_localPartyName;
	}
	else
	{
		event->m_value = m_localParty;
	}
	g_captureEventCallBack(event, m_capturePort);
	m_localPartyReported = true;

	// Report remote party
	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtRemoteParty;
	if(DLLCONFIG.m_partiesUseName == true && m_remotePartyName.size())
	{
		event->m_value = m_remotePartyName;
	}
	else
	{
		event->m_value = m_remoteParty;
	}
	g_captureEventCallBack(event, m_capturePort);
	m_remotePartyReported = true;

	// Report local entry point
	if(m_localEntryPoint.size())
	{
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtLocalEntryPoint;
		event->m_value = m_localEntryPoint;
		g_captureEventCallBack(event, m_capturePort);
	}

	if(DLLCONFIG.m_sipReportNamesAsTags == true)
	{
		CStdString key, value;

		key = "localname";
		value = m_localPartyName;
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key = key;
		event->m_value = value;
		g_captureEventCallBack(event, m_capturePort);

		key = "remotename";
		value = m_remotePartyName;
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key = key;
		event->m_value = value;
		g_captureEventCallBack(event, m_capturePort);
	}

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
	ACE_Time_Value timeNow;
	ACE_Time_Value beginTime;
	ACE_Time_Value timeDiff;
	int msDiff = 0;

	beginTime.set(m_beginDate, 0);
	timeNow = ACE_OS::gettimeofday();
	timeDiff = timeNow - beginTime;
	msDiff = (timeDiff.sec() * 1000) + (timeDiff.usec() / 1000);

	dtmfEventString.Format("event:%d timestamp:%d duration:%d volume:%d seqno:%d offsetms:%d", m_currentRtpEvent, m_currentRtpEventTs, m_currentDtmfDuration, m_currentDtmfVolume, m_currentSeqNo, msDiff);
	dtmfEventKey.Format("%d_RtpDtmfEvent", m_currentRtpEventTs);
	event->m_type = CaptureEvent::EtKeyValue;
	event->m_key = dtmfEventKey;
	event->m_value = dtmfEventString;
	g_captureEventCallBack(event, m_capturePort);

	LOG4CXX_INFO(m_log, "[" + m_trackingId + "] RTP DTMF event [ " + dtmfEventString + " ]");
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
	rtpEventInfo->m_end = (payloadFormat->er_volume & 0x80) ? 1 : 0;
	rtpEventInfo->m_reserved = (payloadFormat->er_volume & 0x40) ? 1 : 0;
	rtpEventInfo->m_volume = (unsigned short)(payloadFormat->er_volume & 0x3F);
	rtpEventInfo->m_duration = ntohs(payloadFormat->duration);
	rtpEventInfo->m_startTimestamp = rtpPacket->m_timestamp;

	if(m_log->isDebugEnabled())
	{
		CStdString eventString;
		rtpEventInfo->ToString(eventString);
		logMsg.Format("[%s] RTP DTMF Event Packet: %s", m_trackingId, eventString);
		LOG4CXX_DEBUG(m_log, logMsg);
	}

	if((m_currentRtpEvent != 65535) && (m_currentRtpEvent != rtpEventInfo->m_event))
	{
		RecordRtpEvent();
	}
	else if(rtpEventInfo->m_end)
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

	if(!rtpEventInfo->m_end)
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
		if(DLLCONFIG.m_rtpReportDtmf)
		{
			/* Check if this is a telephone-event */
			if(m_sessionTelephoneEventPtDefined)
			{
				if(rtpPacket->m_payloadType == StringToInt(m_telephoneEventPayloadType))
				{
					// This is a telephone-event
					HandleRtpEvent(rtpPacket);
					return true;
				}
			}
		}
	}

	// If we are on hold, unmark this
	if(m_onHold)
	{
		if(m_lastRtpPacket.get())
		{
			if( (rtpPacket->m_arrivalTimestamp - m_lastRtpPacket->m_arrivalTimestamp) >  1)
			{
				// There's been an RTP interruption of a least 1 second, 
				// presence of new RTP indicates session has gone out of hold
				logMsg =  "[" + m_trackingId + "] Session going off hold due to RTP activity";
				LOG4CXX_INFO(m_log, logMsg);
				m_onHold = false;
			}
		}
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
		LOG4CXX_INFO(m_log, logMsg);
	}
	m_invites.push_front(invite);
	if(invite->m_telephoneEventPtDefined)
	{
		m_telephoneEventPayloadType = invite->m_telephoneEventPayloadType;
		m_sessionTelephoneEventPtDefined = true;
	}

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

bool RtpSession::NativeCallIdMatches(CStdString& callid)
{
	if(callid.size() > 0)
	{
		if(m_callId.CompareNoCase(callid) == 0)
		{
			return true;
		}
	}
	return false;
}

void RtpSession::ReportSkinnyCallInfo(SkCallInfoStruct* callInfo, IpHeaderStruct* ipHeader)
{
	std::map<CStdString, CStdString>::iterator pair;

	if(IsInSkinnyReportingList(CStdString("localpartyname")))
	{
		CStdString key, value;

		key = "localpartyname";
		value = callInfo->callingPartyName;

		pair = m_tags.find(key);
		if(pair == m_tags.end())
		{
			m_tags.insert(std::make_pair(key, value));
		}
	}

	if(IsInSkinnyReportingList(CStdString("callmanager")))
	{
		CStdString key, value;
		char szIp[16];

		ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_src, szIp, sizeof(szIp));
		key = "callmanager";
		value = szIp;

		pair = m_tags.find("callmanager");
		if(pair == m_tags.end())
		{
			m_tags.insert(std::make_pair(key, value));
		}
	}
}

CStdString RtpSession::GetOrkUid()
{
	return m_orkUid;
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
	if(DLLCONFIG.m_sipIgnoredMediaAddresses.Matches(invite->m_fromRtpIp))
	{
		LOG4CXX_INFO(m_log, "INVITE disregarded by SipIgnoredMediaAddresses parameter");
		return;
	}

	int rtpPortAsInt = StringToInt(invite->m_fromRtpPort);
	unsigned short rtpPort = 0;
	if(rtpPortAsInt>0 && rtpPortAsInt<65535)
	{
		rtpPort = rtpPortAsInt;
	}
	CStdString ipAndPort;
	CraftMediaAddress(ipAndPort, invite->m_fromRtpIp, rtpPortAsInt);

	RtpSessionRef session = findByMediaAddress(invite->m_fromRtpIp, rtpPortAsInt);
	if(session.get())
	{
		// A session already exists on this media address
		if(session->m_protocol == RtpSession::ProtRawRtp)
		{
			// Do nothing here so that we end up stopping this Raw RTP session 
			// and creating new session below
		}
		else if (invite->m_callId.Equals(session->m_callId) == false)
		{
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] Detecting new SIP callId:" +  invite->m_callId + " stopping...");
			// Forcing previous session type to Raw RTP so that SetMediaAddress() successfully
			// replaces it by a new session below.
			session->m_protocol = RtpSession::ProtRawRtp;
		}
		else
		{
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
					session->m_lastUpdated = time(NULL);	// so that timeout countdown is reset
					LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going off hold");
					return;
				}
			}

			session->ReportSipInvite(invite);
			return;
		}
	}
	std::map<CStdString, RtpSessionRef>::iterator pair;
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
				session->m_lastUpdated = time(NULL);	// so that timeout countdown is reset
				LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going off hold");
				return;
			}
		}

		if(!session->m_ipAndPort.Equals(ipAndPort) && DLLCONFIG.m_sipDynamicMediaAddress)
		{
			SetMediaAddress(session, invite->m_fromRtpIp, rtpPort);
		}
		return;
	}

	// create new session and insert into both maps
	CStdString trackingId = m_alphaCounter.GetNext();
	RtpSessionRef newSession(new RtpSession(trackingId));
	newSession->m_callId = invite->m_callId;
	newSession->m_protocol = RtpSession::ProtSip;
	newSession->ReportSipInvite(invite);
	SetMediaAddress(newSession, invite->m_fromRtpIp, rtpPort);
	m_byCallId.insert(std::make_pair(newSession->m_callId, newSession));

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

			CStdString sipError;

			info->ToString(sipError, session->m_invite);
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP Error packet: " + sipError);

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

void RtpSessions::ReportSipSessionProgress(SipSessionProgressInfoRef& info)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;

	pair = m_byCallId.find(info->m_callId);
	if (pair != m_byCallId.end())
	{
		RtpSessionRef session = pair->second;
		unsigned short mediaPort = ACE_OS::atoi(info->m_mediaPort);

		SetMediaAddress(session, info->m_mediaIp, mediaPort);
	}
}

void RtpSessions::ReportSip200Ok(Sip200OkInfoRef info)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;

	pair = m_byCallId.find(info->m_callId);
	if (pair != m_byCallId.end())
	{
		RtpSessionRef session = pair->second;
		unsigned short mediaPort = ACE_OS::atoi(info->m_mediaPort);

		if(info->m_hasSdp && DLLCONFIG.m_sipUse200OkMediaAddress && DLLCONFIG.m_sipDynamicMediaAddress)
		{
			SetMediaAddress(session, info->m_mediaIp, mediaPort);
		}
		else if(info->m_hasSdp && DLLCONFIG.m_sipUse200OkMediaAddress && !session->m_numRtpPackets) 
		{
			if(!session->m_rtpIp.s_addr)
			{
				// Session has empty RTP address
				SetMediaAddress(session, info->m_mediaIp, mediaPort);
			}
			else
			{
				if(!DLLCONFIG.m_lanIpRanges.Matches(session->m_rtpIp))
				{
					// Session has a public IP
					if(!DLLCONFIG.m_lanIpRanges.Matches(info->m_mediaIp))
					{
						SetMediaAddress(session, info->m_mediaIp, mediaPort);
					}
				}
				else
				{
					// Session has a private IP
					SetMediaAddress(session, info->m_mediaIp, mediaPort);
				}
			}
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
	session->m_skinnyLineInstance = callInfo->lineInstance;
	CStdString lp;

	switch(callInfo->callType)
	{
	case SKINNY_CALL_TYPE_INBOUND:
	case SKINNY_CALL_TYPE_FORWARD:
		lp = callInfo->calledParty;
		session->m_localParty = GetLocalPartyMap(lp);
		session->m_remoteParty = callInfo->callingParty;
		session->m_direction = CaptureEvent::DirIn;
		break;
	case SKINNY_CALL_TYPE_OUTBOUND:
		lp = callInfo->callingParty;
		session->m_localParty = GetLocalPartyMap(lp);
		session->m_remoteParty = callInfo->calledParty;
		session->m_direction = CaptureEvent::DirOut;
		break;
	default:
		lp = callInfo->calledParty;
		session->m_localParty = GetLocalPartyMap(lp);
		session->m_remoteParty = callInfo->callingParty;		
	}
}


void RtpSessions::ReportSkinnyCallInfo(SkCallInfoStruct* callInfo, IpHeaderStruct* ipHeader)
{
	CStdString callId = GenerateSkinnyCallId(ipHeader->ip_dest, callInfo->callId);
	CStdString logMsg;

	std::map<CStdString, RtpSessionRef>::iterator pair;
	pair = m_byCallId.find(callId);

	if (pair != m_byCallId.end())
	{
		// CM can resend the same message more than once in a session, 
		// just update timestamp
		RtpSessionRef existingSession = pair->second;
		existingSession->m_skinnyLastCallInfoTime = ACE_OS::gettimeofday();

		if(DLLCONFIG.m_skinnyNameAsLocalParty == true)
		{
			if(!(existingSession->m_localPartyName).size())
			{
				(existingSession->m_localPartyName).Format("%s", callInfo->callingPartyName);
				logMsg.Format("[%s] setting localpartyname:%s", existingSession->m_trackingId, callInfo->callingPartyName);
				LOG4CXX_INFO(m_log, logMsg);
			}
		}

		if(DLLCONFIG.m_skinnyAllowCallInfoUpdate)
		{
			UpdateSessionWithCallInfo(callInfo, existingSession);
		}

		existingSession->ReportSkinnyCallInfo(callInfo, ipHeader);

		return;
	}

	if(DLLCONFIG.m_skinnyAllowLateCallInfo == true)
	{
		RtpSessionRef ipPortSession = findByEndpointIpUsingIpAndPort(ipHeader->ip_dest);

		if(ipPortSession.get() && ipPortSession->m_callId.IsEmpty())
		{
			// The session has not already had a CallInfo, update it with CallInfo data
			ipPortSession->m_skinnyLastCallInfoTime = ACE_OS::gettimeofday();
			ipPortSession->m_callId = callId;
			UpdateSessionWithCallInfo(callInfo, ipPortSession);
			ipPortSession->UpdateMetadataSkinny();
			ipPortSession->ReportSkinnyCallInfo(callInfo, ipHeader);

			if(DLLCONFIG.m_skinnyNameAsLocalParty == true)
			{
				if(!(ipPortSession->m_localPartyName).size())
				{
					(ipPortSession->m_localPartyName).Format("%s", callInfo->callingPartyName);
	                	        logMsg.Format("[%s] setting localpartyname:%s", ipPortSession->m_trackingId, callInfo->callingPartyName);
        	                	LOG4CXX_INFO(m_log, logMsg);
				}
			}

			if(m_log->isInfoEnabled())
			{
				CStdString logMsg;

				CStdString dir = CaptureEvent::DirectionToString(ipPortSession->m_direction);
				char szEndPointIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&ipPortSession->m_endPointIp, szEndPointIp, sizeof(szEndPointIp));

				logMsg.Format("[%s] LATE Skinny CallInfo callId %s local:%s remote:%s dir:%s endpoint:%s", ipPortSession->m_trackingId,
						ipPortSession->m_callId, ipPortSession->m_localParty, ipPortSession->m_remoteParty, dir, szEndPointIp);
				LOG4CXX_INFO(m_log, logMsg);
			}

			m_byCallId.insert(std::make_pair(ipPortSession->m_callId, ipPortSession));
			return;
		}
	}

	// create new session and insert into the callid map
	CStdString trackingId = m_alphaCounter.GetNext();
	RtpSessionRef session(new RtpSession(trackingId));
	session->m_callId = callId;
	session->m_endPointIp = ipHeader->ip_dest;	// CallInfo message always goes from CM to endpoint 
	session->m_protocol = RtpSession::ProtSkinny;
	UpdateSessionWithCallInfo(callInfo, session);
	session->ReportSkinnyCallInfo(callInfo, ipHeader);

	if(DLLCONFIG.m_skinnyNameAsLocalParty == true)
	{
		(session->m_localPartyName).Format("%s", callInfo->callingPartyName);
		logMsg.Format("[%s] setting localpartyname:%s", session->m_trackingId, callInfo->callingPartyName);
		LOG4CXX_INFO(m_log, logMsg);
	}

	char szEndPointIp[16];
	szEndPointIp[0] = '\0';

	if(m_log->isInfoEnabled())
	{
		CStdString logMsg;
		CStdString dir = CaptureEvent::DirectionToString(session->m_direction);
		ACE_OS::inet_ntop(AF_INET, (void*)&session->m_endPointIp, szEndPointIp, sizeof(szEndPointIp));

		logMsg.Format("[%s] Skinny CallInfo callId %s local:%s remote:%s dir:%s line:%d endpoint:%s", session->m_trackingId,
			session->m_callId, session->m_localParty, session->m_remoteParty, dir, session->m_skinnyLineInstance, szEndPointIp);
		LOG4CXX_INFO(m_log, logMsg);
	}

	if(DLLCONFIG.m_skinnyCallInfoStopsPrevious == true)
	{
		RtpSessionRef previousSession = findByEndpointIpAndLineInstance(ipHeader->ip_dest, callInfo->lineInstance);
		if(previousSession.get())
		{
			logMsg.Format("[%s] CallInfo stops [%s] line:%u endpoint:%s", session->m_trackingId, previousSession->m_trackingId, callInfo->lineInstance, szEndPointIp);
			LOG4CXX_INFO(m_log, logMsg);
			Stop(previousSession);
		}
	}

	m_byCallId.insert(std::make_pair(session->m_callId, session));

	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

}

RtpSessionRef RtpSessions::findByMediaAddress(struct in_addr ipAddress, unsigned short udpPort)
{
	CStdString mediaAddress;
	CraftMediaAddress(mediaAddress, ipAddress, udpPort);

	RtpSessionRef session;
	std::map<CStdString, RtpSessionRef>::iterator pair;
	pair = m_byIpAndPort.find(mediaAddress);
	if (pair != m_byIpAndPort.end())
	{
		session = pair->second;
	}
	return session;
}


RtpSessionRef RtpSessions::findByEndpointIpUsingIpAndPort(struct in_addr endpointIpAddr)
{
	RtpSessionRef session;
	std::map<CStdString, RtpSessionRef>::iterator pair;

	// Scan all sessions and try to find a session on the same IP endpoint
	// This function uses the m_byIpAndPort mapping unlike findByEndpointIp()

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end(); pair++)
	{
		RtpSessionRef tmpSession = pair->second;

		if((unsigned int)tmpSession->m_endPointIp.s_addr == (unsigned int)endpointIpAddr.s_addr)
		{
			session = tmpSession;
			break;
		}
	}

	return session;
}

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

// Find a session by Skinny endpoint IP address and by Skinny Line ID
RtpSessionRef RtpSessions::findByEndpointIpAndLineInstance(struct in_addr endpointIpAddr, int lineInstance)
{
	RtpSessionRef session;
	std::map<CStdString, RtpSessionRef>::iterator pair;

	// Scan all sessions and try to find a session on the same IP endpoint
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		RtpSessionRef tmpSession = pair->second;

		if((unsigned int)tmpSession->m_endPointIp.s_addr == (unsigned int)endpointIpAddr.s_addr)
		{
			if(tmpSession->m_skinnyLineInstance == lineInstance)
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


void RtpSessions::CraftMediaAddress(CStdString& mediaAddress, struct in_addr ipAddress, unsigned short udpPort)
{
	char szIpAddress[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&ipAddress, szIpAddress, sizeof(szIpAddress));
	mediaAddress.Format("%s,%u", szIpAddress, udpPort);
}

void RtpSessions::SetMediaAddress(RtpSessionRef& session, struct in_addr mediaIp, unsigned short mediaPort)
{
	if(mediaPort == 0)
	{
		return;
	}
	if(DLLCONFIG.m_mediaAddressBlockedIpRanges.Matches(mediaIp))
	{
		char szMediaIp[16];
		CStdString logMsg;
		ACE_OS::inet_ntop(AF_INET, (void*)&mediaIp, szMediaIp, sizeof(szMediaIp));

		logMsg.Format("[%s] %s,%d rejected by MediaAddressBlockedIpRanges", session->m_trackingId, szMediaIp, mediaPort);
		LOG4CXX_INFO(m_log, logMsg);

		return;
	}

	CStdString logMsg;

	CStdString mediaAddress;
	CraftMediaAddress(mediaAddress, mediaIp, mediaPort);

	bool doChangeMediaAddress = true;

	RtpSessionRef oldSession = findByMediaAddress(mediaIp, mediaPort);
	if(oldSession.get())
	{
		// A session exists on the same IP+port
		if(oldSession->m_protocol == RtpSession::ProtRawRtp || oldSession->m_numRtpPackets == 0)
		{
			logMsg.Format("[%s] on %s replaces [%s]", 
							session->m_trackingId, mediaAddress, oldSession->m_trackingId); 
			LOG4CXX_INFO(m_log, logMsg);
			Stop(oldSession);
		}
		else
		{
			doChangeMediaAddress = false;
			logMsg.Format("[%s] on %s will not replace [%s]", 
							session->m_trackingId, mediaAddress, oldSession->m_trackingId); 
			LOG4CXX_INFO(m_log, logMsg);
		}
	}
	if(doChangeMediaAddress)
	{
		if(m_log->isInfoEnabled())
		{
			char szEndPointIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&session->m_endPointIp, szEndPointIp, sizeof(szEndPointIp));
			logMsg.Format("[%s] media address:%s %s callId:%s endpoint:%s", session->m_trackingId, mediaAddress, RtpSession::ProtocolToString(session->m_protocol),session->m_callId, szEndPointIp);
			LOG4CXX_INFO(m_log, logMsg);
		}

		m_byIpAndPort.erase(session->m_ipAndPort);
		session->m_ipAndPort = mediaAddress;
		session->m_rtpIp = mediaIp;
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
		if(DLLCONFIG.m_skinnyAllowLateCallInfo == true)
		{
			RtpSessionRef sessionExisting;

			sessionExisting = findByEndpointIp(openReceive->endpointIpAddr, 0);
			if(!sessionExisting.get())
			{
				EndpointInfoRef endpoint = GetEndpointInfo(openReceive->endpointIpAddr);
				char szEndpointIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&openReceive->endpointIpAddr, szEndpointIp, sizeof(szEndpointIp));

				// create new session and insert into the ipAndPort
				CStdString trackingId = m_alphaCounter.GetNext();
				RtpSessionRef session(new RtpSession(trackingId));
				session->m_endPointIp = openReceive->endpointIpAddr;
				session->m_protocol = RtpSession::ProtSkinny;
				session->m_skinnyPassThruPartyId = openReceive->passThruPartyId;

				if(endpoint.get())
				{
					session->m_localParty = GetLocalPartyMap(endpoint->m_extension);
				}
				else
				{
					CStdString lp(szEndpointIp);
					session->m_localParty = GetLocalPartyMap(lp);
				}	
				SetMediaAddress(session, openReceive->endpointIpAddr, openReceive->endpointTcpPort);
			}
		}
		else
		{
			// Discard because we have not seen any CallInfo Message before
			LOG4CXX_INFO(m_log, "Skinny OpenReceiveChannelAck without a CallInfoMessage");
		}
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
		if(DLLCONFIG.m_skinnyAllowLateCallInfo == true)
		{
			RtpSessionRef sessionExisting;

			sessionExisting = findByEndpointIp(ipHeader->ip_dest, 0);
			if(!sessionExisting.get())
			{
				EndpointInfoRef endpoint = GetEndpointInfo(ipHeader->ip_dest);
				char szEndpointIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_dest, szEndpointIp, sizeof(szEndpointIp));

				// create new session and insert into the ipAndPort
				CStdString trackingId = m_alphaCounter.GetNext();
				RtpSessionRef session(new RtpSession(trackingId));
				session->m_endPointIp = ipHeader->ip_dest;	// CallInfo StartMediaTransmission always goes from CM to endpoint 
				session->m_protocol = RtpSession::ProtSkinny;
				session->m_skinnyPassThruPartyId = startMedia->passThruPartyId;

				if(endpoint.get())
				{
					session->m_localParty = GetLocalPartyMap(endpoint->m_extension);
				}
				else
				{
					CStdString lp(szEndpointIp);
					session->m_localParty = GetLocalPartyMap(lp);
				}	
				SetMediaAddress(session, startMedia->remoteIpAddr, startMedia->remoteTcpPort);
			}
		}
		else
		{
			// Discard because we have not seen any CallInfo Message before
			LOG4CXX_INFO(m_log, "Skinny StartMediaTransmission without a CallInfoMessage");
		}
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
	else
	{
		LOG4CXX_INFO(m_log, "Couldn't find Skinny session for StopMediaTransmission");
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
		session->m_lastUpdated = time(NULL);	// so that timeout countdown is reset
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

bool RtpSessions::ReportRtcpSrcDescription(RtcpSrcDescriptionPacketInfoRef& rtcpInfo)
{
	RtpSessionRef session;

	session = findByMediaAddress(rtcpInfo->m_sourceIp, rtcpInfo->m_sourcePort - 1);
	if(session.get() != NULL)
	{
		session->ReportRtcpSrcDescription(rtcpInfo);
		return true;
	}

	session = findByMediaAddress(rtcpInfo->m_destIp, rtcpInfo->m_destPort - 1);
	if(session.get() != NULL)
	{
		session->ReportRtcpSrcDescription(rtcpInfo);
		return true;
	}

	return false;
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
		if(rtpPacket->m_sourceIp.s_addr == rtpPacket->m_destIp.s_addr)
		{
			// Source and destination IP are the same, let's change one
			// side in order to simulate bidirection RTP
			// flip the least significant bit of the most significant byte
			rtpPacket->m_destIp.s_addr ^= 0x00000001;
		}

		if(sourcePort > DLLCONFIG.m_sangomaTxTcpPortStart)
		{
			// This is a TX packet
			sourcePort = sourcePort - DLLCONFIG.m_sangomaTcpPortDelta;
			rtpPacket->m_sourcePort = sourcePort;

			// swap source and dest IP addresses so that we simulate bidirectional RTP
			in_addr sourceIP = rtpPacket->m_sourceIp;
			rtpPacket->m_sourceIp = rtpPacket->m_destIp;
			rtpPacket->m_destIp = sourceIP;
		}
		else
		{
			// This is an RX packet
			sourcePort = sourcePort + DLLCONFIG.m_sangomaTcpPortDelta;
			rtpPacket->m_sourcePort = sourcePort;
		}
	}

	// Add RTP packet to session with matching source or dest IP+Port. 
	// On CallManager there might be two sessions with two different CallIDs for one 
	// phone call, so this RTP packet can potentially be reported to two sessions.

	// Does a session exist with this source Ip+Port
	session1 = findByMediaAddress(rtpPacket->m_sourceIp, sourcePort);
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

	// Does a session exist with this destination Ip+Port
	session2 = findByMediaAddress(rtpPacket->m_destIp, destPort);
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
		struct in_addr rtpIp;
		unsigned short rtpPort;

		if(DLLCONFIG.IsRtpTrackingIpAddress(rtpPacket->m_sourceIp))
		{
			rtpIp = rtpPacket->m_sourceIp;
			rtpPort = rtpPacket->m_sourcePort;
		}
		else if(DLLCONFIG.m_sangomaEnable)
		{
			rtpIp = rtpPacket->m_sourceIp;
			rtpPort = rtpPacket->m_sourcePort;
		}
		else
		{
			rtpIp = rtpPacket->m_destIp;
			rtpPort = rtpPacket->m_destPort;
		}
		// (1) In the case of a PSTN Gateway automated answer, The media address
		// is the destination IP+Port of the first packet which is good, 
		// because it is usually the IP+Port of the PSTN Gateway.

		session->m_endPointIp = rtpIp;
		SetMediaAddress(session, rtpIp, rtpPort);

		session->AddRtpPacket(rtpPacket);

		CStdString numSessions = IntToString(m_byIpAndPort.size());
		LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

		LOG4CXX_INFO(m_log, "[" + trackingId + "] created by RTP packet");
	}
}

void RtpSessions::StopAll()
{
	time_t forceExpiryTime = time(NULL) + 2*DLLCONFIG.m_rtpSessionOnHoldTimeOutSec;
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
			if(session->m_onHold)
			{
				timeoutSeconds = DLLCONFIG.m_rtpSessionOnHoldTimeOutSec;
			}
			else
			{
				if(session->m_numRtpPackets)
				{
					timeoutSeconds = DLLCONFIG.m_rtpSessionWithSignallingTimeoutSec;
				}
				else
				{
					timeoutSeconds = DLLCONFIG.m_rtpSessionWithSignallingInitialTimeoutSec;
				}
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
		CStdString logMsg;
		logMsg.Format("[%s] %s Expired (RTP) ts:%u", session->m_trackingId, session->m_ipAndPort, session->m_lastUpdated);
		LOG4CXX_INFO(m_log, logMsg);
		Stop(session);
	}

	// Go round the callId session index and find inactive sessions
	toDismiss.clear();
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		RtpSessionRef session = pair->second;

		if(session->m_onHold)
		{
			if((now - session->m_lastUpdated) > DLLCONFIG.m_rtpSessionOnHoldTimeOutSec)
			{
				toDismiss.push_back(session);
			}
		}
		else
		{
			if(session->m_numRtpPackets)
			{
				if((now - session->m_lastUpdated) > DLLCONFIG.m_rtpSessionWithSignallingTimeoutSec)
				{
					toDismiss.push_back(session);
				}
			}
			else
			{
				if((now - session->m_lastUpdated) > DLLCONFIG.m_rtpSessionWithSignallingInitialTimeoutSec)
				{
					toDismiss.push_back(session);
				}
			}
		}
	}

	// discard inactive sessions
	for (std::list<RtpSessionRef>::iterator it2 = toDismiss.begin(); it2 != toDismiss.end() ; it2++)
	{
		RtpSessionRef session = *it2;
		CStdString logMsg;
		logMsg.Format("[%s] %s Expired (CallID) ts:%u", session->m_trackingId, session->m_ipAndPort, session->m_lastUpdated);
		Stop(session);
	}
}

void RtpSessions::StartCaptureOrkuid(CStdString& orkuid)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->OrkUidMatches(orkuid))
		{
			session->m_keep = true;
			found = true;
		}
	}

	if(found)
	{
		logMsg.Format("[%s] StartCaptureOrkuid: Started capture, orkuid:%s", session->m_trackingId, orkuid);
	}
	else
	{
		logMsg.Format("StartCaptureOrkuid: No session has orkuid:%s", orkuid);
	}

	LOG4CXX_INFO(m_log, logMsg);
}

CStdString RtpSessions::StartCaptureNativeCallId(CStdString& nativecallid)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->NativeCallIdMatches(nativecallid))
		{
			session->m_keep = true;
			found = true;
			orkUid = session->GetOrkUid();
		}
	}

	if(found)
	{
		logMsg.Format("[%s] StartCaptureNativeCallId: Started capture, nativecallid:%s", session->m_trackingId, nativecallid);
	}
	else
	{
		logMsg.Format("StartCaptureNativeCallId: No session has native callid:%s", nativecallid);
	}

	LOG4CXX_INFO(m_log, logMsg);

	return orkUid;
}

CStdString RtpSessions::StartCapture(CStdString& party)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if (session->PartyMatches(party))
		{
			session->m_keep = true;
			found = true;
			orkUid = session->GetOrkUid();
		}
	}

	if(found)
	{
		logMsg.Format("[%s] StartCapture: Started capture, party:%s", session->m_trackingId, party);
	}	
	else
	{
		logMsg.Format("StartCapture: No session has party %s", party);
	}
	
	LOG4CXX_INFO(m_log, logMsg);

	return orkUid;
}

CStdString RtpSessions::PauseCapture(CStdString& party)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if (session->PartyMatches(party))
		{
			session->m_keep = false;
			found = true;
			orkUid = session->GetOrkUid();
		}
	}

	if(found)
	{
		logMsg.Format("[%s] PauseCapture: Paused capture, party:%s", session->m_trackingId, party);
	}	
	else
	{
		logMsg.Format("PauseCapture: No session has party %s", party);
	}
	
	LOG4CXX_INFO(m_log, logMsg);

	return orkUid;
}

void RtpSessions::PauseCaptureOrkuid(CStdString& orkuid)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->OrkUidMatches(orkuid))
		{
			session->m_keep = false;
			found = true;
		}
	}

	if(found)
	{
		logMsg.Format("[%s] PauseCaptureOrkuid: Paused capture, orkuid:%s", session->m_trackingId, orkuid);
	}
	else
	{
		logMsg.Format("PauseCaptureOrkuid: No session has orkuid:%s", orkuid);
	}

	LOG4CXX_INFO(m_log, logMsg);
}

CStdString RtpSessions::PauseCaptureNativeCallId(CStdString& nativecallid)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->NativeCallIdMatches(nativecallid))
		{
			session->m_keep = false;
			found = true;
			orkUid = session->GetOrkUid();
		}
	}

	if(found)
	{
		logMsg.Format("[%s] PauseCaptureNativeCallId: Paused capture, nativecallid:%s", session->m_trackingId, nativecallid);
	}
	else
	{
		logMsg.Format("PauseCaptureNativeCallId: No session has native callid:%s", nativecallid);
	}

	LOG4CXX_INFO(m_log, logMsg);

	return orkUid;
}

CStdString RtpSessions::StopCapture(CStdString& party)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if (session->PartyMatches(party))
		{
			found = true;
			orkUid = session->GetOrkUid();
		}
	}

	if(found)
	{
		logMsg.Format("[%s] StopCapture: stopping capture, party:%s", session->m_trackingId, party);
		LOG4CXX_INFO(m_log, logMsg);
		Stop(session);
	}	
	else
	{
		logMsg.Format("StopCapture: No session has party %s", party);
		LOG4CXX_INFO(m_log, logMsg);
	}

	return orkUid;
}

void RtpSessions::StopCaptureOrkuid(CStdString& orkuid)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->OrkUidMatches(orkuid))
		{
			found = true;
		}
	}

	if(found)
	{
		logMsg.Format("[%s] StopCaptureOrkuid: stopping capture, orkuid:%s", session->m_trackingId, orkuid);
		LOG4CXX_INFO(m_log, logMsg);
		Stop(session);
	}
	else
	{
		logMsg.Format("StopCaptureOrkuid: No session has orkuid:%s", orkuid);
		LOG4CXX_INFO(m_log, logMsg);
	}
}

CStdString RtpSessions::StopCaptureNativeCallId(CStdString& nativecallid)
{
	std::map<CStdString, RtpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	RtpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->NativeCallIdMatches(nativecallid))
		{
			found = true;
			orkUid = session->GetOrkUid();
		}
	}

	if(found)
	{
		logMsg.Format("[%s] StopCaptureNativeCallId: stopping capture, nativecallid:%s", session->m_trackingId, nativecallid);
		LOG4CXX_INFO(m_log, logMsg);
		Stop(session);
	}
	else
	{
		logMsg.Format("StopCaptureNativeCallId: No session has native callid:%s", nativecallid);
		LOG4CXX_INFO(m_log, logMsg);
	}

	return orkUid;
}

void RtpSessions::SaveLocalPartyMap(CStdString& oldparty, CStdString& newparty)
{
	m_localPartyMap.insert(std::make_pair(oldparty, newparty));
	LOG4CXX_DEBUG(m_log, "Saved map oldparty:" + oldparty + " newparty:" + newparty);
}

CStdString RtpSessions::GetLocalPartyMap(CStdString& oldlocalparty)
{
	CStdString newlocalparty;
	std::map<CStdString, CStdString>::iterator pair;

	newlocalparty = oldlocalparty;

	pair = m_localPartyMap.find(oldlocalparty);
	if(pair != m_localPartyMap.end())
	{
		newlocalparty = pair->second;
		LOG4CXX_DEBUG(m_log, "Mapped oldparty:" + oldlocalparty + " to newparty:" + newlocalparty);
	}

	return newlocalparty;
}

//==========================================================
SipInviteInfo::SipInviteInfo()
{
	m_fromRtpIp.s_addr = 0;
	m_validated = false;
	m_attrSendonly = false;
	m_telephoneEventPtDefined = false;
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

	string.Format("sender:%s from:%s@%s RTP:%s,%s to:%s@%s rcvr:%s callid:%s smac:%s rmac:%s fromname:%s toname:%s ua:%s", senderIp, m_from, m_fromDomain, fromRtpIp, m_fromRtpPort, m_to, m_toDomain, receiverIp, m_callId, senderMac, receiverMac, m_fromName, m_toName, m_userAgent);
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

	string.Format("sender:%s rcvr:%s smac:%s rmac:%s callid:%s errorcode:%s reason:\"%s\"", senderIp, receiverIp, senderMac, receiverMac, m_callId, m_errorCode, m_errorString);
}

void SipFailureMessageInfo::ToString(CStdString& string, SipInviteInfoRef inviteInfo)
{
	char senderIp[16], receiverIp[16];
	CStdString senderMac, receiverMac;

	//MemMacToHumanReadable((unsigned char*)m_senderMac, senderMac);
	//MemMacToHumanReadable((unsigned char*)m_receiverMac, receiverMac);
	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	string.Format("sender:%s from:%s to:%s rcvr:%s callid:%s errorcode:%s reason:\"%s\"", senderIp, inviteInfo->m_from, inviteInfo->m_to, receiverIp, inviteInfo->m_callId, m_errorCode, m_errorString);
}

//============================
Sip200OkInfo::Sip200OkInfo()
{
	m_mediaIp.s_addr = 0;
	m_hasSdp = false;
}

void Sip200OkInfo::ToString(CStdString& string)
{
	char mediaIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_mediaIp, mediaIp, sizeof(mediaIp));

	char senderIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));

	char receiverIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	if(m_mediaPort.size())
	{
		string.Format("sender:%s from:%s RTP:%s,%s to:%s rcvr:%s callid:%s", senderIp, m_from, mediaIp, m_mediaPort, m_to, receiverIp, m_callId);
	}
	else
	{
		string.Format("sender:%s from:%s to:%s rcvr:%s callid:%s", senderIp, m_from, m_to, receiverIp, m_callId);
	}
}


//================================================
SipSessionProgressInfo::SipSessionProgressInfo()
{
	m_mediaIp.s_addr = 0;
	m_senderIp.s_addr = 0;
	m_receiverIp.s_addr = 0;
}

void SipSessionProgressInfo::ToString(CStdString& string)
{
	char mediaIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_mediaIp, mediaIp, sizeof(mediaIp));

	char senderIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));

	char receiverIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	string.Format("sender:%s from:%s RTP:%s,%s to:%s rcvr:%s callid:%s", senderIp, m_from, mediaIp, m_mediaPort, m_to, receiverIp, m_callId);
}

//================================================
SipByeInfo::SipByeInfo()
{
	m_senderIp.s_addr = 0;
	m_receiverIp.s_addr = 0;
}

void SipByeInfo::ToString(CStdString& string)
{
	char senderIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));

	char receiverIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	string.Format("sender:%s rcvr:%s callid:%s", senderIp, receiverIp, m_callId);
}

