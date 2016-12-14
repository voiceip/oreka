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
#include "VoIpSession.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include <list>
#include "ConfigManager.h"
#include "VoIpConfig.h"
#include "ace/OS_NS_arpa_inet.h"
#include "MemUtils.h"
#include <boost/algorithm/string/predicate.hpp>

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;

#define CONFERENCE_TRANSFER_TRACKING_TAG_KEY "orig-orkuid"

#ifdef TESTING
	VoIpSessions* VoIpSessionsSingleton::voipSessions = NULL;
#endif

VoIpSession::VoIpSession(CStdString& trackingId)
{
	m_trackingId = trackingId;
	m_lastUpdated = time(NULL);
	m_creationDate = ACE_OS::gettimeofday();
	m_skinnyLastCallInfoTime = m_creationDate;
	m_sipLastInvite = m_creationDate;
	m_log = Logger::getLogger("rtpsession");
	m_direction = CaptureEvent::DirUnkn;
	m_localSide = CaptureEvent::LocalSideUnkn;
	m_protocol = ProtUnkn;
	m_numRtpPackets = 0;
	m_numIgnoredRtpPackets = 0;
	m_metadataProcessed = false;
	m_started = false;
	m_stopped = false;
	m_onHold = false;
	m_keepRtp = true;
	if(CONFIG.m_lookBackRecording == false)
	{
		m_keepRtp = false;
	}
	m_nonLookBackSessionStarted = false;
	m_beginDate = 0;
	m_hasDuplicateRtp = false;
	m_highestRtpSeqNumDelta = 0;
	m_minRtpSeqDelta = (double)DLLCONFIG.m_rtpDiscontinuityMinSeqDelta;
	m_minRtpTimestampDelta = (double)DLLCONFIG.m_rtpDiscontinuityMinSeqDelta * 160;		// arbitrarily based on 160 samples per packet (does not need to be precise)
	m_skinnyPassThruPartyId = 0;
	memset(m_localMac, 0, sizeof(m_localMac));
	memset(m_remoteMac, 0, sizeof(m_remoteMac));
	m_currentRtpEventTs = 0;
	m_rtcpLocalParty = false;
	m_rtcpRemoteParty = false;
	m_remotePartyReported = false;
	m_localPartyReported = false;
	m_sessionTelephoneEventPtDefined = false;
	m_rtpIp.s_addr = 0;
	m_skinnyLineInstance = 0;
	m_onDemand = false;
	m_lastRtpStreamStart = 0;
	m_rtpNumMissingPkts = 0;
	m_rtpNumSeqGaps = 0;
	m_holdDuration = 0;
	m_ipAndPort = 0;
	m_isCallPickUp = false;
	m_lastKeepAlive = time(NULL);
	m_numAlienRtpPacketsS1 = 0;
	m_numAlienRtpPacketsS2 = 0;
	m_ssrcCandidate = -1;
	m_mappedS1S2 =  false;
	m_orekaRtpPayloadType = 0;
}

void VoIpSession::Stop()
{
	CStdString logMsg;
	logMsg.Format("[%s] %s Session stop, numRtpPkts:%d dupl:%d seqDelta:%d rtpNumMissingPkts:%d rtpNumSeqGaps:%d lastUpdated:%u", m_trackingId, m_capturePort, m_numRtpPackets, m_hasDuplicateRtp, m_highestRtpSeqNumDelta, m_rtpNumMissingPkts, m_rtpNumSeqGaps, m_lastUpdated);
	LOG4CXX_INFO(m_log, logMsg);

	if(m_started && !m_stopped)
	{
		// Report local side
		if(m_lastRtpPacketSide1.get())
		{
			if(!MatchesReferenceAddresses(m_lastRtpPacketSide1->m_sourceIp))
			{
				m_localSide = CaptureEvent::LocalSideSide1;
			}
			else
			{
				VoIpEndpointInfoRef endpoint;

				endpoint = VoIpSessionsSingleton::instance()->GetVoIpEndpointInfoByIp(&m_lastRtpPacketSide1->m_sourceIp);
				if(endpoint.get())
				{
					m_localSide = CaptureEvent::LocalSideSide1;
				}
			}
		}

		if(m_lastRtpPacketSide2.get())
		{
			if(!MatchesReferenceAddresses(m_lastRtpPacketSide2->m_sourceIp))
			{
				if(m_localSide == CaptureEvent::LocalSideSide1)
				{
					m_localSide = CaptureEvent::LocalSideBoth;
				}
				else
				{
					m_localSide = CaptureEvent::LocalSideSide2;
				}
			}
			else
			{
				VoIpEndpointInfoRef endpoint;

				endpoint = VoIpSessionsSingleton::instance()->GetVoIpEndpointInfoByIp(&m_lastRtpPacketSide2->m_sourceIp);
				if(endpoint.get())
				{
					if(m_localSide == CaptureEvent::LocalSideSide1)
					{
						m_localSide = CaptureEvent::LocalSideBoth;
					}
					else
					{
						m_localSide = CaptureEvent::LocalSideSide2;
					}
				}
			}
		}

		if(DLLCONFIG.m_urlExtractorEnable == true)
		{
			VoIpEndpointInfoRef endpoint;
			endpoint = VoIpSessionsSingleton::instance()->GetVoIpEndpointInfo(m_localIp, 0);
			if(!endpoint.get())
			{
				endpoint = VoIpSessionsSingleton::instance()->GetVoIpEndpointInfo(m_remoteIp, 0);
			}

			if(endpoint.get())
			{
				std::map<CStdString, UrlExtractionValueRef>::iterator it;
				it = endpoint->m_urlExtractionMap.find(DLLCONFIG.m_remotePartyUseExtractedKey);
				if(it != endpoint->m_urlExtractionMap.end())
				{
					if((it->second->m_timestamp >= m_creationDate.sec()) && (it->second->m_timestamp <= m_lastUpdated))
					{
						m_remoteParty = it->second->m_value;
						CaptureEventRef event(new CaptureEvent());
						event->m_type = CaptureEvent::EtRemoteParty;
						event->m_value = m_remoteParty;
						g_captureEventCallBack(event, m_capturePort);
					}
				}
			}

		}

		CaptureEventRef event(new CaptureEvent);
		event->m_type = CaptureEvent::EtLocalSide;
		event->m_value = CaptureEvent::LocalSideToString(m_localSide);
		g_captureEventCallBack(event, m_capturePort);

		CaptureEventRef stopEvent(new CaptureEvent);
		stopEvent->m_type = CaptureEvent::EtStop;
		stopEvent->m_timestamp = m_lastUpdated;
		g_captureEventCallBack(stopEvent, m_capturePort);
		m_stopped = true;
	}
}

bool VoIpSession::Stopped()
{
	return m_stopped;
}

RtpPacketInfoRef VoIpSession::GetLastRtpPacket()
{
	return m_lastRtpPacket;
}

void VoIpSession::ReportRtcpSrcDescription(RtcpSrcDescriptionPacketInfoRef& rtcpInfo)
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
			CStdString lp;
			lp = m_localParty;
			m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(lp);
		}
		else
		{
			m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(rtcpInfo->m_cnameUsername);
		}
			
		LOG4CXX_INFO(m_log, "[" + m_trackingId + "] Set local party to RTCP CNAME:" + m_localParty);
		
		// In some cases, we obtain this information after the session
		// has long started.  Therefore, in order for the reporting
		// to reflect the RTCP CNAME information, we need to report.
		if(m_localPartyReported)
		{
			if(DLLCONFIG.m_localPartyAddLocalIp == true)
			{
				char szLocalIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
				m_localParty.Format("%s@%s", m_localParty, szLocalIp);
			}
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
				m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(testParty);
			}
			else
			{
				CStdString rp = rtcpInfo->m_cnameUsername;
				m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(rp);
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

void VoIpSession::Start()
{
	m_beginDate = time(NULL);
	if(m_started == false)
	{
		GenerateOrkUid();
	}
	m_started = true;
	CaptureEventRef startEvent(new CaptureEvent);
	startEvent->m_type = CaptureEvent::EtStart;
	startEvent->m_timestamp = m_beginDate;
	startEvent->m_value = m_trackingId;
	CStdString timestamp = IntToString(startEvent->m_timestamp);
	LOG4CXX_INFO(m_log,  "[" + m_trackingId + "] " + m_capturePort + " " + ProtocolToString(m_protocol) + " Session start, timestamp:" + timestamp);
	g_captureEventCallBack(startEvent, m_capturePort);

	if (DLLCONFIG.m_SkinnyTrackConferencesTransfers == true)
	{
		SkinnyTrackConferencesTransfers(m_callId, m_capturePort);
	}
}

void VoIpSession::GenerateOrkUid()
{
	struct tm date = {0};
	ACE_OS::localtime_r(&m_beginDate, &date);
	int month = date.tm_mon + 1;				// january=0, decembre=11
	int year = date.tm_year + 1900;
	m_orkUid.Format("%.4d%.2d%.2d_%.2d%.2d%.2d_%s", year, month, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec, m_trackingId);
}


void VoIpSession::ProcessMetadataRawRtp(RtpPacketInfoRef& rtpPacket)
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
                	        m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(lp);
	                }
		}
		if(!m_rtcpRemoteParty)
		{
			CStdString rp(szDestIp);
			m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(rp);
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
				m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(lp);
			}
		}
		if(!m_rtcpRemoteParty)
		{
			CStdString rp(szSourceIp);
			m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(rp);
		}

		m_localIp = rtpPacket->m_destIp;
		m_remoteIp = rtpPacket->m_sourceIp;
                memcpy(m_localMac, rtpPacket->m_destMac, sizeof(m_localMac));
                memcpy(m_remoteMac, rtpPacket->m_sourceMac, sizeof(m_remoteMac));
	}
}

bool VoIpSession::MatchesSipDomain(CStdString& domain)
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

bool VoIpSession::IsInSkinnyReportingList(CStdString item)
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

void VoIpSession::ProcessMetadataSipIncoming()
{
	if((DLLCONFIG.m_sipRequestUriAsLocalParty == true) && (m_invite->m_requestUri.CompareNoCase(m_invite->m_to) != 0) && (m_invite->m_requestUri.length() != 0))
	{
		m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_requestUri);
		m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_from);
		m_direction = CaptureEvent::DirIn;
		m_localEntryPoint = m_invite->m_to;
	}
	else
	{
		m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_from);
		m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_to);
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

	m_localIp = m_invite->m_receiverIp;
	m_remoteIp = m_invite->m_senderIp;
	m_capturePort = m_trackingId;
	memcpy(m_localMac, m_invite->m_receiverMac, sizeof(m_localMac));
	memcpy(m_remoteMac, m_invite->m_senderMac, sizeof(m_remoteMac));
}

void VoIpSession::ProcessMetadataSipOutgoing()
{
	if((DLLCONFIG.m_sipRequestUriAsLocalParty == true) && (m_invite->m_requestUri.CompareNoCase(m_invite->m_to) != 0) && (m_invite->m_requestUri.length() != 0))
	{
		m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_requestUri);
		m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_from);
		m_direction = CaptureEvent::DirIn;
		m_localEntryPoint = m_invite->m_to;
	}
	else
	{
		m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_to);
		m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_from);
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

	m_localIp = m_invite->m_senderIp;
	m_remoteIp = m_invite->m_receiverIp;
	m_capturePort = m_trackingId;
	memcpy(m_localMac, m_invite->m_senderMac, sizeof(m_localMac));
	memcpy(m_remoteMac, m_invite->m_receiverMac, sizeof(m_remoteMac));
}

void VoIpSession::UpdateMetadataSkinny()
{
	// Report Local party
	CaptureEventRef event(new CaptureEvent());
	event->m_type = CaptureEvent::EtLocalParty;
	if(DLLCONFIG.m_skinnyNameAsLocalParty == true && m_localPartyName.size())
	{
		if(DLLCONFIG.m_localPartyAddLocalIp == true)
		{
			char szLocalIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
			m_localPartyName.Format("%s@%s", m_localPartyName, szLocalIp);
		}
		event->m_value = m_localPartyName;
	}
	else
	{
		if(DLLCONFIG.m_localPartyAddLocalIp == true)
		{
			char szLocalIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
			m_localParty.Format("%s@%s", m_localParty, szLocalIp);
		}
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

void VoIpSession::UpdateMetadataSipOnRtpChange(RtpPacketInfoRef& rtpPacket, bool sourceRtpAddressIsNew)
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
			m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_requestUri);
			m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_invite->m_from);
			m_direction = CaptureEvent::DirIn;
			m_localEntryPoint = m_invite->m_to;
		}
		else
		{
			m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(invite->m_from);
			m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(invite->m_to);
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

		if(DLLCONFIG.m_localPartyAddLocalIp == true)
		{
			char szLocalIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
			m_localParty.Format("%s@%s", m_localParty, szLocalIp);
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

bool VoIpSession::MatchesReferenceAddresses(struct in_addr inAddr)
{
	return DLLCONFIG.m_sipDirectionReferenceIpAddresses.Matches(inAddr);
}

void VoIpSession::ProcessMetadataSip(RtpPacketInfoRef& rtpPacket)
{
	if(m_sipDialedNumber.length() != 0)
	{
		ProcessMetadataSipOutgoing();
		m_remoteParty = m_invite->m_sipDialedNumber;
		m_localParty = m_invite->m_to;
		m_direction = CaptureEvent::DirOut;
	
		return;
	}

	if(DLLCONFIG.m_sipDirectionReferenceIpAddresses.m_asciiIpRanges.size())
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

	if(m_invite->m_SipGroupPickUpPatternDetected == true)
	{
		m_direction = CaptureEvent::DirIn;
	}

	if(m_sipRemoteParty != "")
	{
		m_remoteParty = m_sipRemoteParty;
	}

}

void VoIpSession::ProcessMetadataSkinny(RtpPacketInfoRef& rtpPacket)
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


void VoIpSession::ReportMetadata()
{
	CStdString logMsg;
	char szLocalIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
	char szRemoteIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_remoteIp, szRemoteIp, sizeof(szRemoteIp));

	if(DLLCONFIG.m_localPartyForceLocalIp)
	{
		CStdString lp(szLocalIp);
		m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(lp);
	}
	// Check if we don't have the local party based on the endpoint IP address
	else if(m_localParty.IsEmpty())
	{
		if(m_protocol == ProtSkinny)
		{
			VoIpEndpointInfoRef endpointInfo = VoIpSessionsSingleton::instance()->GetVoIpEndpointInfo(m_endPointIp, m_endPointSignallingPort);
			if(endpointInfo.get())
			{
				m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(endpointInfo->m_extension);
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
			m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(lp);
		}
	}

	if(DLLCONFIG.m_localPartyForceLocalMac)
	{
		m_localParty = "";
		MemMacToHumanReadable((unsigned char*)m_localMac, m_localParty);
		m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(m_localParty);
	}

	//If this session is Call Pick Up, then we revert the direction
	if(m_isCallPickUp)
	{
		if(m_direction == CaptureEvent::DirIn)
		{
			m_direction = CaptureEvent::DirOut;
		}
		else
		{
			m_direction = CaptureEvent::DirIn;
		}
		LOG4CXX_INFO(m_log, "[" + m_trackingId + "] " + "is Sip Service Call Pick Up session, reverted call direction");
	}

	if (DLLCONFIG.m_localPartyNameMapEnable) {
		static std::map<CStdString, CStdString> s_localPartyNameMap;
		CStdString localIp(szLocalIp);
		CStdString tempLocalPartyName = s_localPartyNameMap[localIp];

		if (m_localPartyName != m_localParty && !(m_localPartyName.length()==0) && m_localPartyName != tempLocalPartyName) {
			s_localPartyNameMap[localIp] = m_localPartyName;

			logMsg.Format("[%s] Saved localPartyName:%s localIp:%s in localPartyNameMap",m_capturePort,m_localPartyName,localIp);
			LOG4CXX_DEBUG(m_log, logMsg);
		}
		else if (!(tempLocalPartyName.length()==0) && m_localPartyName != tempLocalPartyName) {
			logMsg.Format("[%s] Retrieved localPartyName:%s for localIp:%s from localPartyNameMap, old name:%s",m_capturePort,tempLocalPartyName,localIp,m_localPartyName);
			LOG4CXX_INFO(m_log, logMsg);

			m_localPartyName = tempLocalPartyName;
		}
	}

	// Report Local party
	CaptureEventRef event(new CaptureEvent());
	event->m_type = CaptureEvent::EtLocalParty;
	// TODO, we might consider deprecating m_skinnyNameAsLocalParty in favour of m_localPartyUseName at some point
	if( ( m_protocol == ProtSkinny && DLLCONFIG.m_skinnyNameAsLocalParty == true && m_localPartyName.size() )   || 
		( (DLLCONFIG.m_partiesUseName || DLLCONFIG.m_localPartyUseName) == true && m_localPartyName.size() )		 )
	{
		if(DLLCONFIG.m_localPartyAddLocalIp == true)
		{
			char szLocalIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
			m_localPartyName.Format("%s@%s", m_localPartyName, szLocalIp);
		}
		event->m_value = m_localPartyName;
	}
	else
	{
		if(DLLCONFIG.m_localPartyAddLocalIp == true)
		{
			char szLocalIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
			m_localParty.Format("%s@%s", m_localParty, szLocalIp);
		}
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

	if(m_onDemand == true)
	{
		// Report ondemand status
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key  = CStdString("ondemand");
		event->m_value = CStdString("true");
		g_captureEventCallBack(event, m_capturePort);
	}

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

void VoIpSession::ReportDtmfDigit(int channel, CStdString digitValue,  unsigned int digitDuration, unsigned int digitVolume, unsigned int rtpEventTs, unsigned int rtpEventSeqNo)
{
	if(m_dtmfDigitString.length() > 100)
	{
		m_dtmfDigitString.clear();
	}
	m_dtmfDigitString += digitValue;
	LOG4CXX_INFO(m_log, "[" + m_trackingId + "] DTMF event [ " + digitValue + " ] new string:" + m_dtmfDigitString);
	if(DLLCONFIG.m_onDemandViaDtmfDigitsString.length() > 0)
	{
		if(m_dtmfDigitString.find(DLLCONFIG.m_onDemandViaDtmfDigitsString) != std::string::npos)
		{
			m_keepRtp = true;
			CStdString side = "both";
			MarkAsOnDemand(side);
			m_dtmfDigitString.clear();
			LOG4CXX_INFO(m_log, "[" + m_trackingId + "] recording started or resumed due to DTMF string pressed");
		}
	}
	if(DLLCONFIG.m_onDemandPauseViaDtmfDigitsString.length() > 0)
	{
		if(m_dtmfDigitString.find(DLLCONFIG.m_onDemandPauseViaDtmfDigitsString) != std::string::npos)
		{
			m_keepRtp = false;
			m_dtmfDigitString.clear();
			LOG4CXX_INFO(m_log, "[" + m_trackingId + "] recording paused due to DTMF string pressed");
		}
	}

	int rtpEvent = DtmfDigitToEnum(digitValue);
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

	if(CONFIG.m_dtmfReportingDetailed == true)
	{
		dtmfEventString.Format("event:%d timestamp:%u duration:%d volume:%d seqno:%d offsetms:%d channel:%d", rtpEvent, rtpEventTs, digitDuration, digitVolume, rtpEventSeqNo, msDiff, channel);
		dtmfEventKey.Format("RtpDtmfEvent_%u", rtpEventTs);
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key = dtmfEventKey;
		event->m_value = dtmfEventString;
		g_captureEventCallBack(event, m_capturePort);
		LOG4CXX_INFO(m_log, "[" + m_trackingId + "] RTP DTMF event [ " + dtmfEventString + " ]");
	}

	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtKeyValue;
	event->m_key = "dtmfdigit";
	event->m_value = digitValue;
	event->m_offsetMs = msDiff;
	g_captureEventCallBack(event, m_capturePort);
}

void VoIpSession::GoOnHold(time_t onHoldTime)
{
	if(m_started != true)
	{
		return;
	}
	m_onHold = true;
	m_holdBegin = onHoldTime;
}

void VoIpSession::GoOffHold(time_t offHoldTime)
{
        m_lastUpdated = offHoldTime;
	m_onHold = false;
	m_holdDuration += offHoldTime - m_holdBegin;

	if(DLLCONFIG.m_holdReportStats)
	{
		// Report holdtime to Audio Tape
		CaptureEventRef event(new CaptureEvent());
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key = "holdtime";
		event->m_value.Format("%d", m_holdDuration);
		g_captureEventCallBack(event,  m_capturePort);
	}
}

void VoIpSession::HandleRtpEvent(RtpPacketInfoRef& rtpPacket, int channel)
{
	CStdString logMsg;

	if(rtpPacket->m_payloadSize < sizeof(RtpEventPayloadFormat))
	{
		LOG4CXX_WARN(m_log, "[" + m_trackingId + "] Payload size for event packet too small");
		return;
	}

	if(DLLCONFIG.m_rtpDtmfOnlyLocal)
	{
		if(rtpPacket->m_sourceIp.s_addr != m_localIp.s_addr)
		{
			return;
		}
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

	if(m_currentRtpEventTs != rtpEventInfo->m_startTimestamp)
	{
		m_currentRtpEventTs = rtpEventInfo->m_startTimestamp;
		ReportDtmfDigit(channel, DtmfDigitToString(rtpEventInfo->m_event), rtpEventInfo->m_duration, rtpEventInfo->m_volume, rtpEventInfo->m_startTimestamp, rtpPacket->m_seqNum);
	}

	return;
}

// Returns false if the packet does not belong to the session (RTP timestamp discontinuity)
bool VoIpSession::AddRtpPacket(RtpPacketInfoRef& rtpPacket)
{
	CStdString logMsg;
	unsigned char channel = 0;

	if(DLLCONFIG.m_rtpLogAllSsrc == true)
	{
		std::map<unsigned int, int>::iterator it;
		it = m_loggedSsrcMap.find(rtpPacket->m_ssrc);
		if(it == m_loggedSsrcMap.end())
		{
			m_loggedSsrcMap.insert(std::make_pair(rtpPacket->m_ssrc, 1));
		}
	}
	
	if( m_metadataProcessed == false )
	{
		m_metadataProcessed = true;
		
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

	if(CONFIG.m_lookBackRecording == false && m_nonLookBackSessionStarted == false && (m_protocol != ProtRawRtp || (DLLCONFIG.m_trackRawRtpSessionInNonLookBackMode == true && m_numIgnoredRtpPackets == DLLCONFIG.m_rtpMinAmountOfPacketsBeforeStart))  )
	{
		Start();
		ReportMetadata();
		m_nonLookBackSessionStarted = true;
	}

	if(m_protocol == ProtSip)
	{
		if(DLLCONFIG.m_rtpReportDtmf)
		{
			//Check if this is a telephone-event
			if(m_sessionTelephoneEventPtDefined)
			{
				if(rtpPacket->m_payloadType == m_telephoneEventPayloadType)
				{
					// This is a telephone-event
					HandleRtpEvent(rtpPacket, channel);
					return true;
				}
			}
		}
	}

	if(!m_keepRtp)
	{
		if(m_nonLookBackSessionStarted == true && (time(NULL) - m_lastKeepAlive > 1) )
		{
			// In case of non-lookback send a keep-alive every second
			CaptureEventRef event(new CaptureEvent());
			event->m_type = CaptureEvent::EtUnknown;
			event->m_value = "";

			m_lastKeepAlive = time(NULL);
			g_captureEventCallBack(event,m_capturePort);
		}

		m_lastUpdated = rtpPacket->m_arrivalTimestamp;
		m_numIgnoredRtpPackets++;
		return true;
	}

	if(CONFIG.m_lookBackRecording == false && m_numRtpPackets == 0)
	{
		Start();
		ReportMetadata();
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

	// If we are on hold, unmark this
	if(m_onHold)
	{
		if(m_lastRtpPacket.get())
		{
			if( rtpPacket->m_payloadType > 18 )
			{
				// Some telephony systems have RTP "keepalive" packets while on hold in the dynamic payload type range. Ignore them.
				
				return true; // dismiss packet 
			}

			if( (rtpPacket->m_arrivalTimestamp - m_lastRtpPacket->m_arrivalTimestamp) >  1)
			{
				// There's been an RTP interruption of a least 1 second, 
				// presence of new RTP indicates session has gone out of hold
				logMsg =  "[" + m_trackingId + "] Session going off hold due to RTP activity";
				LOG4CXX_INFO(m_log, logMsg);
				GoOffHold(rtpPacket->m_arrivalTimestamp);
			}
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
		if( rtpPacket->m_ssrc == m_lastRtpPacketSide1->m_ssrc  )
		{
			// Subsequent RTP packet for side 1
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
				//In case of dialer session, rtp keeps on going even when the call is done
				//If we detect the pausing in rtp, we break the session
				int deltaTimestamp = rtpPacket->m_timestamp - m_lastRtpPacketSide1->m_timestamp;
				if(DLLCONFIG.m_rtpBreakupOnStreamPause == true && (abs(seqNumDelta) * 160) < deltaTimestamp)
				{
					if((double)rtpPacket->m_seqNum < (double)m_lastRtpPacketSide1->m_seqNum)	//seq reset
					{
						seqNumDelta = 65536 - (double)m_lastRtpPacketSide1->m_seqNum + (double)rtpPacket->m_seqNum;
					}
					if((seqNumDelta * 160) < deltaTimestamp)
					{
						logMsg.Format("[%s] RTP stream pause detected, breaking up s1: before: seq:%u ts:%u after: seq:%u ts:%u",
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

				if(seqNumDelta > 1)
				{
					if(seqNumDelta <= DLLCONFIG.m_rtpSeqGapThreshold)
					{
						m_rtpNumSeqGaps += 1;
						m_rtpNumMissingPkts += ((unsigned int)seqNumDelta - 1);
					}
				}
			}
			m_lastRtpPacketSide1 = rtpPacket;
			channel = 1;
			m_numAlienRtpPacketsS1 = 0;
		}
		else
		{
			if(m_lastRtpPacketSide2.get() == NULL)
			{
				// First RTP packet for side 2
				m_lastRtpPacketSide2 = rtpPacket;
				channel = 2;

				if(m_log->isInfoEnabled())
				{
					rtpPacket->ToString(logMsg);
					logMsg =  "[" + m_trackingId + "] 1st packet s2: " + logMsg;
					LOG4CXX_INFO(m_log, logMsg);
				}
			}
			else
			{
				if(rtpPacket->m_ssrc == m_lastRtpPacketSide2->m_ssrc)
				{
					// Subsequent RTP packet for side 2
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

						int deltaTimestamp = rtpPacket->m_timestamp - m_lastRtpPacketSide2->m_timestamp;
						if(DLLCONFIG.m_rtpBreakupOnStreamPause == true )
						{
							if((double)rtpPacket->m_seqNum < (double)m_lastRtpPacketSide2->m_seqNum)	//seq reset
							{
								seqNumDelta = 65536 - (double)m_lastRtpPacketSide2->m_seqNum + (double)rtpPacket->m_seqNum;
							}
							if((seqNumDelta * 160) < deltaTimestamp)
							{
								logMsg.Format("[%s] RTP stream pause detected, breaking up s2: before: seq:%u ts:%u after: seq:%u ts:%u",
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

					m_lastRtpPacketSide2 = rtpPacket;
					channel = 2;
					m_numAlienRtpPacketsS2 = 0;
				}
				else
				{
					// this packet does not match either s1 or s2 (on the basis of SSRC)
					if(m_ssrcCandidate == -1)
					{
						m_ssrcCandidate = rtpPacket->m_ssrc;
						m_ssrcCandidateTimestamp = (unsigned int)rtpPacket->m_arrivalTimestamp;
						rtpPacket->ToString(logMsg);
						logMsg.Format("[%s] ssrc candidate: %s", m_trackingId, logMsg);
						LOG4CXX_INFO(m_log, logMsg);
					}
					else if(rtpPacket->m_ssrc == m_ssrcCandidate)
					{
						m_ssrcCandidateTimestamp = (unsigned int)rtpPacket->m_arrivalTimestamp;
						m_numAlienRtpPacketsS1++;
						m_numAlienRtpPacketsS2++;
					}
					else
					{
						if((time(NULL) - m_ssrcCandidateTimestamp) > 2)
						{
							m_ssrcCandidate = -1;
							m_numAlienRtpPacketsS1 = 0;
							m_numAlienRtpPacketsS2 = 0;
							logMsg.Format("[%s] ssrc:0x%x candidate stopped", m_trackingId, rtpPacket->m_ssrc);
							LOG4CXX_INFO(m_log, logMsg);
						}
						if(DLLCONFIG.m_rtpLogAllSsrc ==  true)
						{
							std::map<unsigned int, int>::iterator it;
							it = m_loggedSsrcMap.find(rtpPacket->m_ssrc);
							if(it == m_loggedSsrcMap.end())
							{
								m_loggedSsrcMap.insert(std::make_pair(rtpPacket->m_ssrc, 1));
								logMsg.Format("[%s] detects unestablished ssrc:0x%x", m_trackingId, rtpPacket->m_ssrc);
								LOG4CXX_INFO(m_log, logMsg);
							}
						}
					}

					bool remapped = false;

					if(m_numAlienRtpPacketsS1 > 10)
					{
						// We have seen 10 alien packets and no s1 packets during the same period of time
						m_numAlienRtpPacketsS1 = 0;
						m_ssrcCandidate = -1;
						remapped = true;
						channel = 1;
						m_lastRtpPacketSide1 = rtpPacket;

						rtpPacket->ToString(logMsg);
						logMsg.Format("[%s] s1 remapped to: %s", m_trackingId, logMsg);
						LOG4CXX_INFO(m_log, logMsg);
					}
					else if(m_numAlienRtpPacketsS2 > 10)
					{
						// We have seen 10 alien packets and no s2 packets during the same period of time
						m_numAlienRtpPacketsS2 = 0;
						m_ssrcCandidate = -1;
						remapped = true;
						channel = 2;
						m_lastRtpPacketSide2 = rtpPacket;

						rtpPacket->ToString(logMsg);
						logMsg.Format("[%s] s2 remapped to: %s", m_trackingId, logMsg);
						LOG4CXX_INFO(m_log, logMsg);
					}
					else
					{
						// dismiss packet so that it does not disrupt the current established stream
						return true;
					}

					if (remapped)
					{
						m_lastRtpStreamStart = time(NULL);
					}
				}
			}
		}
	}

	//re-evaluate s1/s2 mapping
	if (DLLCONFIG.m_rtpS1S2MappingDeterministic && !m_mappedS1S2 && m_lastRtpPacketSide1.get() && m_lastRtpPacketSide2.get()) {
		if (m_protocol==ProtSip || m_protocol==ProtSkinny)  // cascading if blocks, intentional, not a bug 
		if ( ((unsigned int)m_lastRtpPacketSide1->m_sourceIp.s_addr != (unsigned int)m_localIp.s_addr) == DLLCONFIG.m_rtpS1S2MappingDeterministicS1IsLocal ) 
		{ 
			RtpPacketInfoRef tmpRtpPacketInfoRef = m_lastRtpPacketSide1;
			m_lastRtpPacketSide1 = m_lastRtpPacketSide2;
			m_lastRtpPacketSide2 = tmpRtpPacketInfoRef;
			channel = (channel%2)+1; // 1 if it was 2 , 2 if it was 1

			LOG4CXX_INFO(m_log, "[" + m_trackingId + "] deterministic audio channel mapping: swapped s1 and s2");
		}
		m_mappedS1S2 = true;
	}

	m_numRtpPackets++;
	m_lastUpdated = rtpPacket->m_arrivalTimestamp;

	bool hasSourceAddress = m_rtpAddressList.HasAddressOrAdd(rtpPacket->m_sourceIp, rtpPacket->m_sourcePort);
	bool hasDestAddress = m_rtpAddressList.HasAddressOrAdd(rtpPacket->m_destIp, rtpPacket->m_destPort);
	if(	hasSourceAddress == false || hasDestAddress == false )
	{
		// A new RTP stream has been detected
		// (but RTP packets for this new stream will only start to be recorded when
		// the new stream is confirmed, see numAlienRtpPackets)
		// NOTE: only the first packet of a bidirectional RTP steam is logged here,
		// so for now, the log does not contain any trace of the other side until s2 is remapped.
		rtpPacket->ToString(logMsg);
		logMsg.Format("[%s] new RTP stream: %s", m_trackingId, logMsg);
		LOG4CXX_INFO(m_log, logMsg);

		if(m_protocol == ProtSip && m_started && DLLCONFIG.m_sipAllowMetadataUpdateOnRtpChange)	// make sure this only happens if ReportMetadata() already been called for the session
		{
			UpdateMetadataSipOnRtpChange(rtpPacket, hasDestAddress);
		}
	}

	if(m_log->isDebugEnabled())
	{
		CStdString debug;
		debug.Format("[%s] %s: Add RTP packet srcPort:%u dstPort:%u seq:%u ts:%u  arrival:%u ch:%d", m_trackingId, m_capturePort, rtpPacket->m_sourcePort, rtpPacket->m_destPort, rtpPacket->m_seqNum, rtpPacket->m_timestamp, rtpPacket->m_arrivalTimestamp, channel);
		LOG4CXX_DEBUG(m_log, debug);
	}

	if(		(m_protocol == ProtRawRtp && m_numRtpPackets == DLLCONFIG.m_rtpMinAmountOfPacketsBeforeStart)	||
			(m_protocol == ProtSkinny && m_numRtpPackets == 2)	||
			(m_protocol == ProtSip && m_numRtpPackets == 2)			)
	{
		// We've got enough packets to start the session.
		// For Raw RTP, the high number is to make sure we have a "real" raw RTP session, not a leftover from a SIP/Skinny session
		if(CONFIG.m_lookBackRecording == true) 
		{
			Start();
			ReportMetadata();
		}
	}

	if(m_started)
	{
		CStdString payloadType;
	
		payloadType.Format("%d",rtpPacket->m_payloadType);

		for(std::list<CStdString>::iterator it = CONFIG.m_speexPayloadTypes.begin() ; it != CONFIG.m_speexPayloadTypes.end() ; it++)
		{
			if( *it == payloadType ) 
			{
				rtpPacket->m_payloadType = 66;
				break;
			}
		}

		AudioChunkDetails details;
		details.m_arrivalTimestamp = rtpPacket->m_arrivalTimestamp;
		details.m_numBytes = rtpPacket->m_payloadSize;
		details.m_timestamp = rtpPacket->m_timestamp;
		details.m_sequenceNumber = rtpPacket->m_seqNum;
		details.m_channel = channel;
		details.m_encoding = AlawAudio;
		details.m_numBytes = rtpPacket->m_payloadSize;
		if(m_orekaRtpPayloadType != 0)
		{
			details.m_rtpPayloadType = m_orekaRtpPayloadType;
		}
		else
		{
			details.m_rtpPayloadType = rtpPacket->m_payloadType;
		}
		AudioChunkRef chunk(new AudioChunk());
		chunk->SetBuffer(rtpPacket->m_payload, details);
		g_audioChunkCallBack(chunk, m_capturePort);

	}
	return true;
}

void VoIpSession::ReportSipBye(SipByeInfoRef& bye)
{
	CStdString byeString;
	CStdString logMsg;

	bye->ToString(byeString);
	if(DLLCONFIG.m_dahdiIntercept == true)
	{
		// With Xorcom interception, we update whichever party is currently
		// set to "s" with the new party in either m_from or m_to of the
		// BYE

		if(m_localParty.CompareNoCase(CStdString("s")) == 0)
		{
			if(m_remoteParty.CompareNoCase(bye->m_to) != 0)
			{
				// remoteparty is set to m_from
				m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(bye->m_to);
				logMsg.Format("[%s] dahdiIntercept: reset localparty:%s from BYE:%s", m_trackingId, m_localParty, byeString);
			}
			else
			{
				// remoteparty is set to m_to
				m_localParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(bye->m_from);
				logMsg.Format("[%s] dahdiIntercept: reset localparty:%s from BYE:%s", m_trackingId, m_localParty, byeString);
			}

			// Report Local party
			CaptureEventRef event(new CaptureEvent());
			event->m_type = CaptureEvent::EtLocalParty;
			event->m_value = m_localParty;
			g_captureEventCallBack(event, m_capturePort);
		}
		else if(m_remoteParty.CompareNoCase(CStdString("s")) == 0)
		{
			CStdString translatedTo, translatedFrom;

			translatedTo = VoIpSessionsSingleton::instance()->GetLocalPartyMap(bye->m_to);
			translatedFrom = VoIpSessionsSingleton::instance()->GetLocalPartyMap(bye->m_from);

			if(m_localParty.CompareNoCase(translatedTo) != 0)
			{
				// localparty is set to m_from
				m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(bye->m_to);
				logMsg.Format("[%s] dahdiIntercept: reset remoteparty:%s from BYE:%s", m_trackingId, m_remoteParty, byeString);
			}
			else
			{
				// localparty is set to m_to
				m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(bye->m_from);
				logMsg.Format("[%s] dahdiIntercept: reset remoteparty:%s from BYE:%s", m_trackingId, m_remoteParty, byeString);
			}

			// Report remote party
			CaptureEventRef event(new CaptureEvent());
			event->m_type = CaptureEvent::EtRemoteParty;
			event->m_value = m_remoteParty;
			g_captureEventCallBack(event, m_capturePort);
		}
		else
		{
			logMsg.Format("[%s] dahdiIntercept: ignoring BYE:%s", m_trackingId, byeString);
		}

		LOG4CXX_INFO(m_log, logMsg);
	}
}


bool IsNecExternal(SipInviteInfoRef& invite) {
	CStdString from_or_to = DLLCONFIG.m_sipDirectionReferenceIpAddresses.Matches(invite->m_senderIp)?invite->m_from:invite->m_to;

	if (MatchesStringList(from_or_to, DLLCONFIG.m_necVoipGatewayNames)) {
		return true;
	}

	return false;
}

void VoIpSession::ReportSipNotify(SipNotifyInfoRef& notify)
{
	if (IsNecExternal(m_invite))
	{
		LOG4CXX_INFO(m_log, "external notify");
		if (m_started) {
			// NecSip is not updated by any other update routine so is "safe"
			m_remotePartyNecSip = notify->m_dsp;

			// Report Remote party to Audio Tape
			CaptureEventRef event(new CaptureEvent());
			event->m_type = CaptureEvent::EtRemoteParty;
			event->m_value = m_remotePartyNecSip;
			g_captureEventCallBack(event, m_capturePort);
		}
		else {
			LOG4CXX_INFO(m_log, "not started");
			m_invite->m_sipRemoteParty = notify->m_dsp;
			m_sipRemoteParty = notify->m_dsp;
		}
	}
}

void VoIpSession::ReportSipInvite(SipInviteInfoRef& invite)
{
	if(DLLCONFIG.m_sipMetadataUseLastInvite || m_invite.get() == NULL)
	{
		m_invite = invite;
	}
	else
	{
		CStdString inviteString;
		CStdString logMsg;
		invite->ToString(inviteString);

		logMsg.Format("[%s] associating INVITE:%s", m_trackingId, inviteString);
		LOG4CXX_INFO(m_log, logMsg);
	}
	if(invite->m_from.CompareNoCase(invite->m_to) == 0 && invite->m_contact.length() > 0)
	{
		m_remoteParty = VoIpSessionsSingleton::instance()->GetLocalPartyMap(invite->m_contact);
		m_direction = CaptureEvent::DirOut;

		CaptureEventRef event(new CaptureEvent());
		event->m_type = CaptureEvent::EtRemoteParty;
		event->m_value = m_remoteParty;
		g_captureEventCallBack(event, m_capturePort);
		

		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtDirection;
		event->m_value = CaptureEvent::DirectionToString(m_direction);
		g_captureEventCallBack(event, m_capturePort);
	}
	m_invites.push_front(invite);
	if(invite->m_telephoneEventPtDefined)
	{
		m_telephoneEventPayloadType = StringToInt(invite->m_telephoneEventPayloadType);
		m_sessionTelephoneEventPtDefined = true;
	}

	if(invite->m_sipRemoteParty != "")
	{
		m_sipRemoteParty = invite->m_sipRemoteParty;
	}

	// Use the RTP codec that was extracted from SDP if available
	//- WARNING, if near end and far end use different codecs, it will force the codec detected in the SDP onto both streams and will not work
	//- might need to fix this at some point
	if(invite->m_orekaRtpPayloadType != 0)
	{
		m_orekaRtpPayloadType = invite->m_orekaRtpPayloadType;
	}

	// Gather extracted fields
	if(m_started)
	{
		std::map<CStdString, CStdString>::iterator i = invite->m_extractedFields.begin();
		for( ; i != invite->m_extractedFields.end(); ++i )
		{		
			// Report Key Values to Audio Tape
			CaptureEventRef event(new CaptureEvent());
			event->m_type = CaptureEvent::EtKeyValue;
			event->m_key = i->first;
			event->m_value = i->second;
			g_captureEventCallBack(event, m_capturePort);
		}
	}
	else
	{
		std::copy(invite->m_extractedFields.begin(), invite->m_extractedFields.end(), std::inserter(m_tags, m_tags.begin()));
	}

	if(DLLCONFIG.m_sipOnDemandFieldName.size())
	{
		std::map<CStdString, CStdString>::iterator pair;

		pair = m_tags.find(DLLCONFIG.m_sipOnDemandFieldName);
		if(pair != m_tags.end())
		{
			CStdString value;

			value = pair->second;
			if(value.find(DLLCONFIG.m_sipOnDemandFieldValue) != std::string::npos)
			{
				m_keepRtp = true;
				m_onDemand = true;
				LOG4CXX_INFO(m_log, "[" + m_trackingId + "] " + DLLCONFIG.m_sipOnDemandFieldName + ":" + value + " triggered recording");
			}
		}
	}
}

void VoIpSession::ReportSipInfo(SipInfoRef& info)
{
	if(m_lastSipInfo.get() != NULL && m_lastSipInfo->m_cSeq.CompareNoCase(info->m_cSeq) == 0)
	{
		return;
	}

	m_lastSipInfo = info;
	if(info->m_dtmfDigit.length() > 0 && DLLCONFIG.m_sipInfoDtmfRfc2976Detect == true)
	{
		ReportDtmfDigit(0, info->m_dtmfDigit, 0, 0, 0, 0);
	}
	if((DLLCONFIG.m_sipOnDemandFieldValue.length() > 0) && (info->m_onDemand == true))
	{
		LOG4CXX_INFO(m_log, "[" + m_trackingId + "] " + DLLCONFIG.m_sipOnDemandFieldName + ":" + DLLCONFIG.m_sipOnDemandFieldValue + " triggered ondemand");
		CStdString side = "both";
		MarkAsOnDemand(side);
	}
	if((DLLCONFIG.m_sipOnDemandFieldValueOff.length() > 0) && (info->m_onDemandOff == true))
	{
		LOG4CXX_INFO(m_log, "[" + m_trackingId + "] " + DLLCONFIG.m_sipOnDemandFieldName + ":" + DLLCONFIG.m_sipOnDemandFieldValueOff + " triggered ondemand off");
		MarkAsOnDemandOff();
	}
}

void VoIpSession::ReportSipRefer(SipReferRef& info)
{
	info->m_origOrkUid = m_orkUid;
	CaptureEventRef event (new CaptureEvent());
	event->m_type = CaptureEvent::EtKeyValue;
	event->m_key = CONFERENCE_TRANSFER_TRACKING_TAG_KEY;
	event->m_value = info->m_origOrkUid;
	g_captureEventCallBack(event, m_capturePort);
}

void VoIpSession::ReportSipErrorPacket(SipFailureMessageInfoRef& info)
{
	CaptureEventRef event(new CaptureEvent());
	event->m_type = CaptureEvent::EtKeyValue;
	event->m_key = CStdString("failed");
	event->m_value = CStdString("true");
	g_captureEventCallBack(event, m_capturePort);
}

int VoIpSession::ProtocolToEnum(CStdString& protocol)
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

CStdString VoIpSession::ProtocolToString(int protocolEnum)
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

bool VoIpSession::OrkUidMatches(CStdString &oUid)
{
	if(m_orkUid.CompareNoCase(oUid) == 0)
	{
		return true;
	}

	return false;
}

bool VoIpSession::PartyMatches(CStdString &party)
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

bool VoIpSession::NativeCallIdMatches(CStdString& callid)
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

void VoIpSession::ReportSkinnyCallInfo(SkCallInfoStruct* callInfo, IpHeaderStruct* ipHeader)
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

void VoIpSession::ReportSkinnyCallStateMessage(SkCallStateMessageStruct* callStateMessage, IpHeaderStruct* ipHeader)
{
	if(callStateMessage->callState == 8 && DLLCONFIG.m_skinnyIgnoreHold == false)	//on hold
	{
		CStdString logMsg;
		GoOnHold(time(NULL));
		logMsg.Format("[%s] Going on hold due to CallStateMessage: HOLD", m_trackingId);
		LOG4CXX_INFO(m_log,logMsg);

	}
}

CStdString VoIpSession::GetOrkUid()
{
	return m_orkUid;
}

void VoIpSession::MarkAsOnDemand(CStdString& side)
{
	// Report direction
	m_onDemand = true;
	if(m_started == true)
	{
		CaptureEventRef event(new CaptureEvent());
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key  = CStdString("ondemand");
		event->m_value = CStdString("true");
		g_captureEventCallBack(event, m_capturePort);

		// Report audio keep direction
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtAudioKeepDirection;
		event->m_value = side;
		g_captureEventCallBack(event, m_capturePort);

		// Trigger metadata update
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtUpdate;
		g_captureEventCallBack(event, m_capturePort);
	}
}

void VoIpSession::MarkAsOnDemandOff()
{
	m_onDemand = false;
	if(m_started == true)
	{
		CaptureEventRef event(new CaptureEvent());
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key  = CStdString("ondemand");
		event->m_value = CStdString("false");
		g_captureEventCallBack(event, m_capturePort);

		// Trigger metadata update
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtUpdate;
		g_captureEventCallBack(event, m_capturePort);
	}
}

void VoIpSession::SkinnyTrackConferencesTransfers(CStdString callId, CStdString capturePort)
{
	CStdString logMsg;
	VoIpEndpointInfoRef endpoint;
	time_t time_now = time(NULL);
	std::map<CStdString, VoIpEndpointInfoRef>::iterator it;

	endpoint = VoIpSessionsSingleton::instance()->GetVoIpEndpointInfo(m_endPointIp, m_endPointSignallingPort);

	if(endpoint.get() == NULL)
	{
		CStdString endpointIp;
	  	char szIp[16];
		ACE_OS::inet_ntop(AF_INET, (void*)&m_endPointIp, szIp, sizeof(szIp));
		endpointIp.Format("%s", szIp);
	  	logMsg.Format("SkinnyTrackConferencesTransfers:[%s] could not find associated endpoint:%s",m_trackingId, endpointIp);
		LOG4CXX_INFO(m_log, logMsg);
		return;
	}
	
	if((time_now - endpoint->m_lastConferencePressed) > 3 && (time_now - endpoint->m_lastConnectedWithConference) > 3)
	{
		endpoint->m_origOrkUid = "";
	}
	else
	{
		if(endpoint->m_origOrkUid != "")
		{
			CaptureEventRef event (new CaptureEvent());
			event->m_type = CaptureEvent::EtKeyValue;
			event->m_key = CONFERENCE_TRANSFER_TRACKING_TAG_KEY;
			event->m_value = endpoint->m_origOrkUid;
			g_captureEventCallBack(event, capturePort);
			logMsg.Format("SkinnyTrackConferencesTransfers:[%s] new leg, tagging with orig-orkuid:%s",m_trackingId, endpoint->m_origOrkUid);
			LOG4CXX_INFO(m_log, logMsg);
		}
	}

}

bool VoIpSession::IsMatchedLocalOrRemoteIp(struct in_addr ip)
{
	if(ip.s_addr == m_localIp.s_addr || ip.s_addr == m_remoteIp.s_addr)
	{
		return true;
	}
	else
		return false;
}

//=====================================================================
VoIpSessions::VoIpSessions()
{
	m_log = Logger::getLogger("rtpsessions");
	if(CONFIG.m_debug)
	{
		m_alphaCounter.Reset();
	}
}


void VoIpSessions::ReportSipInvite(SipInviteInfoRef& invite)
{
	if(DLLCONFIG.m_sipIgnoredMediaAddresses.Matches(invite->m_fromRtpIp))
	{
		LOG4CXX_INFO(m_log, "INVITE disregarded by SipIgnoredMediaAddresses parameter");
		return;
	}

	char* errorLocation = NULL;
	PCSTR szRtpPort = (PCSTR)(invite->m_fromRtpPort);
	int rtpPortAsInt = strtol(szRtpPort, &errorLocation, 10);
	if(*errorLocation != '\0')
	{
		rtpPortAsInt = 0;
	}

	unsigned short rtpPort = 0;
	if(rtpPortAsInt>0 && rtpPortAsInt<65535)
	{
		rtpPort = rtpPortAsInt;
	}
	unsigned long long ipAndPort;

	Craft64bitMediaAddress(ipAndPort, invite->m_fromRtpIp, rtpPortAsInt);
	VoIpSessionRef session = findByMediaAddress(invite->m_fromRtpIp, rtpPortAsInt);

	if(session.get() == NULL && DLLCONFIG.m_sipTrackMediaAddressOnSender)
	{
		Craft64bitMediaAddress(ipAndPort, invite->m_originalSenderIp, rtpPortAsInt);
		VoIpSessionRef session = findByMediaAddress(invite->m_originalSenderIp, rtpPortAsInt);
	}
	if(session.get())
	{
		// A session already exists on this media address
		if(session->m_protocol == VoIpSession::ProtRawRtp)
		{
			// Do nothing here so that we end up stopping this Raw RTP session 
			// and creating new session below
		}
	    else if(session->m_protocol == VoIpSession::ProtSip && DLLCONFIG.m_rtpAllowMultipleMappings == true)
	    {
	    	// Do nothing here so that this new session ends up receiving
			// all future RTP packets to this media address
	    }

		else if (DLLCONFIG.m_rtpAllowMultipleMappings == false && invite->m_callId.Equals(session->m_callId) == false)
		{
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] Detecting new SIP callId:" +  invite->m_callId + " stopping...");
			// Forcing previous session type to Raw RTP so that SetMediaAddress() successfully
			// replaces it by a new session below.
			session->m_protocol = VoIpSession::ProtRawRtp;
		}
		else
		{
			if(invite->m_attrSendonly)
			{
				session->GoOnHold(invite->m_recvTime);
				LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going on hold");
				return;
			}
			else
			{
				if(session->m_onHold && DLLCONFIG.m_sipInviteCanPutOffHold)
				{
					session->GoOffHold(invite->m_recvTime);
					session->m_lastUpdated = time(NULL);	// so that timeout countdown is reset
					LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going off hold");
					SetMediaAddress(session, invite->m_fromRtpIp, rtpPort);
					return;
				}
			}

			//now we look to see if this INVITE is from a endpont which just previously sent out SIP SUBSCRIBE for Call Pick Up service
			if(DLLCONFIG.m_sipCallPickUpSupport)
			{
				TrySessionCallPickUp(invite->m_replacesId, session->m_isCallPickUp);
			}
			session->ReportSipInvite(invite);
			return;
		}
	}
	std::map<CStdString, VoIpSessionRef>::iterator pair;
	pair = m_byCallId.find(invite->m_callId);
	if (pair != m_byCallId.end())
	{
		// The session already exists
		VoIpSessionRef session = pair->second;

		/*
		 * If the sendonly attribute is present then our call is
		 * going on hold.
		 */
		if(invite->m_attrSendonly)
		{
			session->GoOnHold(invite->m_recvTime);
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going on hold");
			return;
		}
		else
		{
			/* If we're already on hold and sendonly is not present
			 * then we go off hold */
			if(session->m_onHold && DLLCONFIG.m_sipInviteCanPutOffHold)
			{
				session->GoOffHold(invite->m_recvTime);
				session->m_lastUpdated = time(NULL);	// so that timeout countdown is reset
				LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] SIP session going off hold");
				SetMediaAddress(session, invite->m_fromRtpIp, rtpPort);
				return;
			}
		}

		if(session->m_ipAndPort != ipAndPort && DLLCONFIG.m_sipDynamicMediaAddress)
		{
			SetMediaAddress(session, invite->m_fromRtpIp, rtpPort);
			if(DLLCONFIG.m_sipTrackMediaAddressOnSender)
			{
				SetMediaAddress(session, invite->m_originalSenderIp, rtpPort);
			}
		}
		if(DLLCONFIG.m_sipCallPickUpSupport)
		{
			TrySessionCallPickUp(invite->m_replacesId, session->m_isCallPickUp);
		}
		session->ReportSipInvite(invite);
		return;
	}

	// create new session and insert into both maps
	CStdString trackingId = m_alphaCounter.GetNext();
	VoIpSessionRef newSession(new VoIpSession(trackingId));
	if(DLLCONFIG.m_sipCallPickUpSupport)
	{
		TrySessionCallPickUp(invite->m_replacesId, newSession->m_isCallPickUp);
	}
	newSession->m_callId = invite->m_callId;
	newSession->m_protocol = VoIpSession::ProtSip;
	newSession->m_sipDialedNumber = invite->m_sipDialedNumber;

	if (DLLCONFIG.m_sipNotifySupport) {
		struct in_addr& endpointIp = DLLCONFIG.m_sipDirectionReferenceIpAddresses.Matches(invite->m_senderIp)? invite->m_receiverIp: invite->m_senderIp;
		VoIpEndpointInfoRef infoRef = GetVoIpEndpointInfo(endpointIp, 0);

		if (infoRef && infoRef->m_preInviteNotifyRef && IsNecExternal(invite) ) {
			invite->m_sipRemoteParty = infoRef->m_preInviteNotifyRef->m_dsp;
			infoRef->m_preInviteNotifyRef.reset() ; // Use it only once
		}
	}

	newSession->ReportSipInvite(invite);
	newSession->m_sipLastInvite = ACE_OS::gettimeofday();
	SetMediaAddress(newSession, invite->m_fromRtpIp, rtpPort);
	if(DLLCONFIG.m_sipTrackMediaAddressOnSender)
	{
		SetMediaAddress(newSession, invite->m_originalSenderIp, rtpPort);
	}
	m_byCallId.insert(std::make_pair(newSession->m_callId, newSession));

	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

	CStdString inviteString;
	invite->ToString(inviteString);
	LOG4CXX_INFO(m_log, "[" + trackingId + "] created by INVITE:" + inviteString);
}

void VoIpSessions::ReportSipSubscribe(SipSubscribeInfoRef& subscribe)
{
	int sipSubscripeMapSize = m_sipSubscribeMap.size();

	//Keep the size of the map under 200 Sip Subscribe messages would be reasonable. Ideally, each element will be removed after its followed INVITE comes
	if(sipSubscripeMapSize > 200)
	{
		SipSubscribeSeqIndex& seqIndex = m_sipSubscribeMap.get<IndexSequential>();
		seqIndex.pop_front();
	}

	//Get the map instant with search indices to check
	SipSubscribeSearchIndex& srchIndex = m_sipSubscribeMap.get<IndexSearchable>();
	SipSubscribeSearchIndex::iterator it = srchIndex.find(subscribe->m_callId);
	if (it == srchIndex.end())
	{
		srchIndex.insert(subscribe);	//even insert new element with different type of index, new element will be appened at the end to preserve Sequence indices
	}

}

void VoIpSessions::ReportSipErrorPacket(SipFailureMessageInfoRef& info)
{
	std::map<CStdString, VoIpSessionRef>::iterator pair;

	pair = m_byCallId.find(info->m_callId);
	if (pair != m_byCallId.end())
	{
		VoIpSessionRef session = pair->second;

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

void VoIpSessions::ReportSipSessionProgress(SipSessionProgressInfoRef& info)
{
	std::map<CStdString, VoIpSessionRef>::iterator pair;

	pair = m_byCallId.find(info->m_callId);
	if (pair != m_byCallId.end())
	{
		VoIpSessionRef session = pair->second;
		unsigned short mediaPort = ACE_OS::atoi(info->m_mediaPort);

		SetMediaAddress(session, info->m_mediaIp, mediaPort);
		if(DLLCONFIG.m_sipTrackMediaAddressOnSender)
		{
			SetMediaAddress(session, info->m_senderIp, mediaPort);
		}
	}
}

void VoIpSessions::ReportSip302MovedTemporarily(Sip302MovedTemporarilyInfoRef& info)
{
	CStdString m_trackingId;

	// Contact: is mapped to the To:
	SaveLocalPartyMap(info->m_contact, info->m_to);

	// If there is already a session, log that information
	std::map<CStdString, VoIpSessionRef>::iterator pair;

	pair = m_byCallId.find(info->m_callId);
	if (pair != m_byCallId.end())
	{
		VoIpSessionRef session = pair->second;

		m_trackingId = session->m_trackingId;
	}

	if(m_trackingId.size())
	{
		LOG4CXX_INFO(m_log, "[" + m_trackingId + "] " + info->m_contact + " mapped to " + info->m_to + " by SIP 302 Moved Temporarily");
	}
	else
	{
		LOG4CXX_INFO(m_log, info->m_contact + " mapped to " + info->m_to + " by SIP 302 Moved Temporarily (session unknown)");
	}
}

void VoIpSessions::ReportSip200Ok(Sip200OkInfoRef info)
{
	std::map<CStdString, VoIpSessionRef>::iterator pair;

	pair = m_byCallId.find(info->m_callId);
	if (pair != m_byCallId.end())
	{
		VoIpSessionRef session = pair->second;
		unsigned short mediaPort = ACE_OS::atoi(info->m_mediaPort);

		if(info->m_hasSdp && DLLCONFIG.m_sipUse200OkMediaAddress && DLLCONFIG.m_sipDynamicMediaAddress)
		{
			SetMediaAddress(session, info->m_mediaIp, mediaPort);
		}
		else if(info->m_hasSdp && DLLCONFIG.m_sipUse200OkMediaAddress && !session->m_numRtpPackets) 
		{
			// Session has not yet received RTP packets
			if(!session->m_rtpIp.s_addr || DLLCONFIG.m_rtpAllowMultipleMappings)
			{
				// Session has empty RTP address or can have multiple RTP addresses.
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

void VoIpSessions::ReportSipBye(SipByeInfoRef& bye)
{
	std::map<CStdString, VoIpSessionRef>::iterator pair;
	pair = m_byCallId.find(bye->m_callId);

	if (pair != m_byCallId.end())
	{
		// Session found: stop it
		VoIpSessionRef session = pair->second;

		session->ReportSipBye(bye);
		Stop(session);
	}
}


static inline bool isNotAlnum(char c) { return !isalnum(c); }

void VoIpSessions::ReportSipNotify(SipNotifyInfoRef& notify)
{
	VoIpSessionRef session;
	for(std::map<CStdString, VoIpSessionRef>::iterator pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++) {
		VoIpSessionRef tmpSession = pair->second;
		unsigned int invite_s_addr = DLLCONFIG.m_sipDirectionReferenceIpAddresses.Matches(tmpSession->m_invite->m_senderIp)? tmpSession->m_invite->m_receiverIp.s_addr: tmpSession->m_invite->m_senderIp.s_addr;

		if ( (invite_s_addr == (unsigned int) notify->m_receiverIp.s_addr) && (!session || tmpSession->m_sipLastInvite > session->m_sipLastInvite) ) {
			session = tmpSession;
		}
 	}

	if (notify->m_dsp.size() < 4) {
		return; 
	}

	if (find_if(notify->m_dsp.begin(), notify->m_dsp.end(), isNotAlnum) != notify->m_dsp.end()) {
		return;
	}

	if (session) {
		session->ReportSipNotify(notify);
	}
	else {
		unsigned long long recieverIp;
		Craft64bitMediaAddress(recieverIp, notify->m_receiverIp, 0);

		std::map<unsigned long long, VoIpEndpointInfoRef>::iterator it = m_endpoints.find(recieverIp);
		if(it == m_endpoints.end()) {
			it = m_endpoints.insert(it, std::make_pair(recieverIp, VoIpEndpointInfoRef(new VoIpEndpointInfo()) ));
		}
		it->second->m_preInviteNotifyRef = notify;
	}
}

void VoIpSessions::ReportSipInfo(SipInfoRef& info)
{
	std::map<CStdString, VoIpSessionRef>::iterator it;
	it = m_byCallId.find(info->m_callId);
	if(it != m_byCallId.end())
	{
		it->second->ReportSipInfo(info);
	}
}

void VoIpSessions::ReportSipRefer(SipReferRef& info)
{
	std::map<CStdString, VoIpSessionRef>::iterator it;
	it = m_byCallId.find(info->m_callId);
	if(it != m_byCallId.end())
	{
		info->m_referToParty = GetLocalPartyMap(info->m_referToParty);
		info->m_referredByParty = GetLocalPartyMap(info->m_referredByParty);
		it->second->ReportSipRefer(info);
		m_sipReferList.push_back(info);
	}
}

void VoIpSessions::UpdateEndpointWithCallInfo(SkCallInfoStruct* callInfo, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	CStdString extension;
	CStdString callId = GenerateSkinnyCallId(ipHeader->ip_dest, ntohs(tcpHeader->dest), callInfo->callId);

	switch(callInfo->callType)
	{
	case SKINNY_CALL_TYPE_INBOUND:
	{
		if(m_skinnyGlobalNumbersList.find(callInfo->calledParty) == m_skinnyGlobalNumbersList.end())
		{
			extension = callInfo->calledParty;
			SetEndpointExtension(extension, &ipHeader->ip_dest, callId, ntohs(tcpHeader->dest));
		}
		break;
	}
	case SKINNY_CALL_TYPE_OUTBOUND:
	{
		if(m_skinnyGlobalNumbersList.find(callInfo->callingParty) == m_skinnyGlobalNumbersList.end())
                {
			extension = callInfo->callingParty;
			SetEndpointExtension(extension, &ipHeader->ip_dest, callId, ntohs(tcpHeader->dest));
		}
		break;
	}
	case SKINNY_CALL_TYPE_FORWARD:
	{
		if(m_skinnyGlobalNumbersList.find(callInfo->calledParty) == m_skinnyGlobalNumbersList.end())
                {
			extension = callInfo->calledParty;
			SetEndpointExtension(extension, &ipHeader->ip_dest, callId, ntohs(tcpHeader->dest));
		}
		break;
	}
	}
}

void VoIpSessions::UpdateSessionWithCallInfo(SkCallInfoStruct* callInfo, VoIpSessionRef& session)
{
	session->m_skinnyLineInstance = callInfo->lineInstance;
	CStdString lp;
	CStdString lpn;
	CStdString rp;
	CStdString logMsg;
	char szEndPointIp[16];

	VoIpEndpointInfoRef endpoint = GetVoIpEndpointInfo(session->m_endPointIp, session->m_endPointSignallingPort);
	ACE_OS::inet_ntop(AF_INET, (void*)&session->m_endPointIp, szEndPointIp, sizeof(szEndPointIp));

	switch(callInfo->callType)
	{
	case SKINNY_CALL_TYPE_INBOUND:
		lp = callInfo->calledParty;
		lpn = callInfo->calledPartyName;
		rp = callInfo->callingParty;
		if(m_skinnyGlobalNumbersList.find(lp) == m_skinnyGlobalNumbersList.end())
		{
			session->m_localParty = GetLocalPartyMap(lp);
		}
		session->m_localPartyName = GetLocalPartyMap(lpn);
		session->m_remoteParty = GetLocalPartyMap(rp);
		session->m_direction = CaptureEvent::DirIn;
		break;
	case SKINNY_CALL_TYPE_FORWARD:
		lp = callInfo->calledParty;
		lpn = callInfo->calledPartyName;
		rp = callInfo->callingParty;
		if(endpoint.get() && ((endpoint->m_extension).size() > 0))
		{
			if(m_skinnyGlobalNumbersList.find(lp) == m_skinnyGlobalNumbersList.end())
			{
				session->m_localParty = GetLocalPartyMap(endpoint->m_extension);
			}
			session->m_localEntryPoint = lp;
			logMsg.Format("[%s] callType is FORWARD: set localparty:%s (obtained from endpoint:%s)", session->m_trackingId, session->m_localParty, szEndPointIp);
			LOG4CXX_DEBUG(m_log, logMsg);
		}
		session->m_localPartyName = GetLocalPartyMap(lpn);
		session->m_remoteParty = GetLocalPartyMap(rp);
		session->m_direction = CaptureEvent::DirIn;
		break;
	case SKINNY_CALL_TYPE_OUTBOUND:
		lp = callInfo->callingParty;
		lpn = callInfo->callingPartyName;
		rp = callInfo->calledParty;
		if(m_skinnyGlobalNumbersList.find(lp) == m_skinnyGlobalNumbersList.end())
		{
			session->m_localParty = GetLocalPartyMap(lp);
		}
		session->m_localPartyName = GetLocalPartyMap(lpn);
		session->m_remoteParty = GetLocalPartyMap(rp);
		session->m_direction = CaptureEvent::DirOut;
		break;
	default:
		lp = callInfo->calledParty;
		lpn = callInfo->calledPartyName;
		rp = callInfo->callingParty;
		if(m_skinnyGlobalNumbersList.find(lp) == m_skinnyGlobalNumbersList.end())
		{
			session->m_localParty = GetLocalPartyMap(lp);
		}
		session->m_localPartyName = GetLocalPartyMap(lpn);
		session->m_remoteParty = GetLocalPartyMap(rp);
	}
}

VoIpEndpointInfoRef VoIpSessions::GetVoIpEndpointInfoByIp(struct in_addr *ip)
{
	std::map<unsigned long long, VoIpEndpointInfoRef>::iterator pair;
	VoIpEndpointInfoRef endpoint;

	for(pair = m_endpoints.begin(); pair != m_endpoints.end(); pair++)
	{
		VoIpEndpointInfoRef ep = pair->second;

		if(ep.get() && (ep->m_ip.s_addr == ip->s_addr))
		{
			endpoint = ep;
			break;
		}
	}

	return endpoint;
}

bool VoIpSessions::SkinnyFindMostLikelySessionForRtp(RtpPacketInfoRef& rtpPacket, VoIpEndpointInfoRef& endpoint)
{
	std::map<CStdString, VoIpSessionRef>::iterator sessionpair;
	VoIpSessionRef session;
	bool srcmatch = false;
	CStdString logMsg;

	endpoint = GetVoIpEndpointInfoByIp(&rtpPacket->m_sourceIp);
	if(endpoint.get())
	{
		srcmatch = true;
	}
	else
	{
		endpoint = GetVoIpEndpointInfoByIp(&rtpPacket->m_destIp);
		if(!endpoint.get())
		{
			return false;
		}
	}

	if(!(endpoint->m_latestCallId).size())
	{
		return false;
	}

	sessionpair = m_byCallId.find(endpoint->m_latestCallId);
	if(sessionpair == m_byCallId.end())
	{
		return false;
	}

	session = sessionpair->second;
	if(session->Stopped())
	{
		return false;
	}

	char szEndPointIp[16];
	char szRtpSrcIp[16];
	char szRtpDstIp[16];

	session->m_endPointSignallingPort = endpoint->m_skinnyPort;
	ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_sourceIp, szRtpSrcIp, sizeof(szRtpSrcIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_destIp, szRtpDstIp, sizeof(szRtpDstIp));

	if(srcmatch == true)
	{
		SetMediaAddress(session, rtpPacket->m_sourceIp, rtpPacket->m_sourcePort);
		ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_sourceIp, szEndPointIp, sizeof(szEndPointIp));
	}
	else
	{
		SetMediaAddress(session, rtpPacket->m_destIp, rtpPacket->m_destPort);
		ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_destIp, szEndPointIp, sizeof(szEndPointIp));
	}

	logMsg.Format("[%s] RTP stream detected on endpoint:%s extension:%s RTP:%s,%d %s,%d callid:%s", session->m_trackingId, szEndPointIp, endpoint->m_extension, szRtpSrcIp, rtpPacket->m_sourcePort, szRtpDstIp, rtpPacket->m_destPort, endpoint->m_latestCallId);
	LOG4CXX_INFO(m_log, logMsg);

	return true;
}

bool VoIpSessions::SkinnyFindMostLikelySessionForRtpBehindNat(RtpPacketInfoRef& rtpPacket)
{
	std::map<CStdString, VoIpSessionRef>::iterator it;
	VoIpSessionRef session, tmpSession, mostRecentSession;
	CStdString logMsg;

	ACE_Time_Value latestSkinnyTimestamp, rtpArrivalTimestamp;
	rtpArrivalTimestamp = ACE_OS::gettimeofday();

	for(it=m_byCallId.begin(); it!=m_byCallId.end(); it++)
	{
		tmpSession = it->second;
		if(tmpSession.get() == NULL)
		{
			continue;
		}
		if(((unsigned int)tmpSession->m_endPointIp.s_addr) == ((unsigned int)rtpPacket->m_sourceIp.s_addr) || ((unsigned int)tmpSession->m_endPointIp.s_addr) == ((unsigned int)rtpPacket->m_destIp.s_addr))
		{
			if((tmpSession->m_skinnyLastCallInfoTime >= latestSkinnyTimestamp) && (tmpSession->m_ipAndPort == 0))
			{
				latestSkinnyTimestamp = tmpSession->m_skinnyLastCallInfoTime;
				mostRecentSession = tmpSession;
			}
		}
	}
	if(mostRecentSession.get() != NULL)
	{
		if(((int)(rtpArrivalTimestamp.sec() - latestSkinnyTimestamp.sec()) > 30) )
		{
			return false;
		}

		session = mostRecentSession;
		CStdString rtpString;
		rtpPacket->ToString(rtpString);
		logMsg.Format("[%s] SkinnyFindMostLikelySessionForRtpBehindNat: associating RTP: %s", mostRecentSession->m_trackingId, rtpString);
		LOG4CXX_INFO(m_log, logMsg);
	}

	if(session.get() == NULL)
	{
		return false;
	}
	if(session->Stopped())
	{
		return false;
	}

	if(((unsigned int)session->m_endPointIp.s_addr) == ((unsigned int)rtpPacket->m_sourceIp.s_addr))
	{
		SetMediaAddress(session, rtpPacket->m_destIp, rtpPacket->m_destPort);
	}
	else
	{
		SetMediaAddress(session, rtpPacket->m_sourceIp, rtpPacket->m_sourcePort);
	}

	return true;
}


void VoIpSession::ReportMetadataUpdateSkinny() {

	CaptureEventRef event(new CaptureEvent());

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

	if (m_started) {
		CaptureEventRef event(new CaptureEvent());
		event->m_type = CaptureEvent::EtUpdate;
		event->m_value = m_remoteParty;
		g_captureEventCallBack(event, m_capturePort);
	}
}

void VoIpSessions::ReportSkinnyCallInfo(SkCallInfoStruct* callInfo, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	CStdString callId = GenerateSkinnyCallId(ipHeader->ip_dest, ntohs(tcpHeader->dest), callInfo->callId);
	CStdString logMsg;

	UpdateEndpointWithCallInfo(callInfo, ipHeader, tcpHeader);

	std::map<CStdString, VoIpSessionRef>::iterator pair;
	pair = m_byCallId.find(callId);

	if (pair != m_byCallId.end())
	{
		// CM can resend the same message more than once in a session, 
		// just update timestamp
		VoIpSessionRef existingSession = pair->second;
		existingSession->m_skinnyLastCallInfoTime = ACE_OS::gettimeofday();
		
		CStdString oldVal = existingSession->m_remoteParty;;

		if(DLLCONFIG.m_skinnyAllowCallInfoUpdate)
		{
			UpdateSessionWithCallInfo(callInfo, existingSession);
		}

		existingSession->ReportSkinnyCallInfo(callInfo, ipHeader);

		if (!(existingSession->m_remoteParty.length()==0) && oldVal != existingSession->m_remoteParty) {
			// sometimes, the remote party is updated by the CM to the correct value after we have started recording the current call leg, e.g. trace 0771e7efb755dae6076f911508ea3ab7 for the last leg of a transfer, so we need to update orktrack.
			existingSession->ReportMetadataUpdateSkinny();
		}

		return;
	}

	if(DLLCONFIG.m_skinnyAllowLateCallInfo == true)
	{
		VoIpSessionRef ipPortSession = findByEndpointIpUsingIpAndPort(ipHeader->ip_dest);

		if(ipPortSession.get() && ipPortSession->m_callId.IsEmpty())
		{
			// The session has not already had a CallInfo, update it with CallInfo data
			ipPortSession->m_skinnyLastCallInfoTime = ACE_OS::gettimeofday();
			ipPortSession->m_callId = callId;
			UpdateSessionWithCallInfo(callInfo, ipPortSession);
			ipPortSession->UpdateMetadataSkinny();
			ipPortSession->ReportSkinnyCallInfo(callInfo, ipHeader);

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
	VoIpSessionRef session(new VoIpSession(trackingId));
	session->m_callId = callId;
	session->m_endPointIp = ipHeader->ip_dest;	// CallInfo message always goes from CM to endpoint
	session->m_endPointSignallingPort = ntohs(tcpHeader->dest);
	session->m_protocol = VoIpSession::ProtSkinny;
	session->m_skinnyLastCallInfoTime = ACE_OS::gettimeofday();
	UpdateSessionWithCallInfo(callInfo, session);
	session->ReportSkinnyCallInfo(callInfo, ipHeader);

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
		VoIpSessionRef previousSession = findNewestRtpByEndpointIp(ipHeader->ip_dest);
		if(previousSession.get())
		{
			if(DLLCONFIG.m_skinnyCallInfoStopsPreviousToleranceSec == 0)
			{
				logMsg.Format("[%s] stopped by [%s] CallInfo (SkinnyCallInfoStopsPreviousToleranceSec:0)", previousSession->m_trackingId, session->m_trackingId);
				LOG4CXX_INFO(m_log, logMsg);
				Stop(previousSession);
			}
			else
			{
				int diff = 0;

				diff = (time(NULL) - previousSession->m_lastRtpStreamStart);
				if(diff <= DLLCONFIG.m_skinnyCallInfoStopsPreviousToleranceSec)
				{
					logMsg.Format("[%s] stopped by [%s] CallInfo, last RTP stream had just started (%d sec. ago)", previousSession->m_trackingId, session->m_trackingId, diff);
					LOG4CXX_INFO(m_log, logMsg);
					Stop(previousSession);
				}
				else
				{
					logMsg.Format("[%s] not stopped by [%s] CallInfo, last RTP stream started long ago (%d sec. ago)", previousSession->m_trackingId, session->m_trackingId, diff);
					LOG4CXX_INFO(m_log, logMsg);
				}
			}
		}
	}

	m_byCallId.insert(std::make_pair(session->m_callId, session));

	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);

}

void VoIpSessions::ReportSkinnyCallStateMessage(SkCallStateMessageStruct* callStateMessage, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	CStdString logMsg;
	CStdString callId = GenerateSkinnyCallId(ipHeader->ip_dest, ntohs(tcpHeader->dest), callStateMessage->callId);
	std::map<CStdString, VoIpSessionRef>::iterator pair;
	pair = m_byCallId.find(callId);
	VoIpSessionRef session;
	if (pair != m_byCallId.end())
	{
		session = pair->second;
		session->ReportSkinnyCallStateMessage(callStateMessage, ipHeader);
	}
	else
	{
		char szEndpointIp[16];
		struct in_addr endpointIp = ipHeader->ip_dest;
		ACE_OS::inet_ntop(AF_INET, (void*)&endpointIp, szEndpointIp, sizeof(szEndpointIp));
		logMsg.Format("hold detected on endpoint:%s but session not found. Try SkinnyIgnoreStopMediaTransmission", szEndpointIp);
		LOG4CXX_INFO(m_log, logMsg);
	}
}

VoIpSessionRef VoIpSessions::findByMediaAddress(struct in_addr ipAddress, unsigned short udpPort)
{
	unsigned long long mediaAddress;
	Craft64bitMediaAddress(mediaAddress, ipAddress, udpPort);

	VoIpSessionRef session;
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	pair = m_byIpAndPort.find(mediaAddress);
	if (pair != m_byIpAndPort.end())
	{
		session = pair->second;
	}
	return session;
}


VoIpSessionRef VoIpSessions::findByEndpointIpUsingIpAndPort(struct in_addr endpointIpAddr)
{
	VoIpSessionRef session;
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;

	// Scan all sessions and try to find a session on the same IP endpoint
	// This function uses the m_byIpAndPort mapping unlike findByEndpointIp()

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end(); pair++)
	{
		VoIpSessionRef tmpSession = pair->second;

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
VoIpSessionRef VoIpSessions::findByEndpointIp(struct in_addr endpointIpAddr, int passThruPartyId)
{
	VoIpSessionRef session;
	std::map<CStdString, VoIpSessionRef>::iterator pair;

	// Scan all sessions and try to find a session on the same IP endpoint
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		VoIpSessionRef tmpSession = pair->second;

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

// Find session with newest RTP
VoIpSessionRef VoIpSessions::findNewestRtpByEndpointIp(struct in_addr endpointIpAddr)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	VoIpSessionRef session;
	RtpPacketInfoRef lastPacket;
	int latest = 0;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end(); pair++)
	{
		VoIpSessionRef tmpSession = pair->second;

		if((unsigned int)tmpSession->m_endPointIp.s_addr == (unsigned int)endpointIpAddr.s_addr)
		{
			lastPacket = tmpSession->GetLastRtpPacket();
			if(lastPacket.get())
			{
				if(lastPacket->m_arrivalTimestamp > latest)
				{
					latest = lastPacket->m_arrivalTimestamp;
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

// Find a session by Skinny endpoint IP address and by Skinny Line ID
VoIpSessionRef VoIpSessions::findByEndpointIpAndLineInstance(struct in_addr endpointIpAddr, int lineInstance)
{
	VoIpSessionRef session;
	std::map<CStdString, VoIpSessionRef>::iterator pair;

	// Scan all sessions and try to find a session on the same IP endpoint
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		VoIpSessionRef tmpSession = pair->second;

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

VoIpSessionRef VoIpSessions::SipfindNewestBySenderIp(struct in_addr receiverIpAddr)
{
	VoIpSessionRef session;
	std::map<CStdString, VoIpSessionRef>::iterator pair;

	// Scan all sessions and try to find the most recently signalled session on the IP endpoint
	// This always scans the entire session list, might be good to index sessions by endpoint at some point
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		VoIpSessionRef tmpSession = pair->second;

		if((unsigned int)tmpSession->m_invite->m_senderIp.s_addr == (unsigned int)receiverIpAddr.s_addr)
		{
			if(session.get())
			{
				if(tmpSession->m_sipLastInvite > session->m_sipLastInvite)
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

VoIpSessionRef VoIpSessions::findNewestByEndpoint(struct in_addr endpointIpAddr, unsigned short endpointSignallingPort)
{
	VoIpSessionRef session;
	std::map<CStdString, VoIpSessionRef>::iterator pair;
	unsigned short sessionSignallingPort = 0;

	// Scan all sessions and try to find the most recently signalled session on the IP endpoint
	// This always scans the entire session list, might be good to index sessions by endpoint at some point
	for(pair = m_byCallId.begin(); pair != m_byCallId.end(); pair++)
	{
		VoIpSessionRef tmpSession = pair->second;

		if(DLLCONFIG.m_skinnyBehindNat)
		{
			sessionSignallingPort = tmpSession->m_endPointSignallingPort;
		}
		else
		{
			// Not behind NAT: make sure that we match on endpoint IP address only
			endpointSignallingPort = 0;
			sessionSignallingPort = 0;
		}

		if((unsigned int)tmpSession->m_endPointIp.s_addr == (unsigned int)endpointIpAddr.s_addr &&
			sessionSignallingPort == endpointSignallingPort )
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

//bool VoIpSessions::ChangeCallId(VoIpSessionRef& session, unsigned int newId)
//{
//	bool result = false;
//	if(newId)
//	{
//		CStdString newCallId = GenerateSkinnyCallId(session->m_endPointIp, newId);
//
//		std::map<CStdString, VoIpSessionRef>::iterator pair = m_byCallId.find(newCallId);
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

/*
 * Deprecated, use Craft64bitMediaAddress instead
 * */
void VoIpSessions::CraftMediaAddress(CStdString& mediaAddress, struct in_addr ipAddress, unsigned short udpPort)
{
	char szIpAddress[16];

	if(DLLCONFIG.m_rtpTrackByUdpPortOnly == false)
	{
		ACE_OS::inet_ntop(AF_INET, (void*)&ipAddress, szIpAddress, sizeof(szIpAddress));
		mediaAddress.Format("%s,%u", szIpAddress, udpPort);
	}
	else
	{
		mediaAddress.Format("%u", udpPort);
	}
}

void VoIpSessions::Craft64bitMediaAddress(unsigned long long& mediaAddress, struct in_addr ipAddress, unsigned short udpPort)
{
	if(DLLCONFIG.m_rtpTrackByUdpPortOnly == false)
	{
		mediaAddress = (((unsigned long long)ipAddress.s_addr) << 16) | udpPort;
	}
	else
	{
		mediaAddress = (unsigned long long)udpPort;
	}
}

CStdString VoIpSessions::MediaAddressToString(unsigned long long ipAndPort)
{
	CStdString strMediaAddress;

	if(DLLCONFIG.m_rtpTrackByUdpPortOnly == false)
	{
		char szIp[16];
		unsigned int ip;
		unsigned short port;
		ip = ipAndPort >> 16;
		port = ipAndPort & 0xffff;
		ACE_OS::inet_ntop(AF_INET, (void*)&ip, szIp, sizeof(szIp));
		strMediaAddress.Format("%s,%u", szIp, port);
		return strMediaAddress;
	}
	else
	{
		strMediaAddress.Format("%u", ipAndPort);
		return strMediaAddress;
	}

}

void VoIpSessions::SetMediaAddress(VoIpSessionRef& session, struct in_addr mediaIp, unsigned short mediaPort)
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
	
	if(!DLLCONFIG.m_mediaAddressAllowedIpRanges.Empty())
	{
		if(DLLCONFIG.m_mediaAddressAllowedIpRanges.Matches(mediaIp) == false)
		{
			char szMediaIp[16];
			CStdString logMsg;
			ACE_OS::inet_ntop(AF_INET, (void*)&mediaIp, szMediaIp, sizeof(szMediaIp));

			logMsg.Format("[%s] %s,%d is not allowed by MediaAddressAllowedIpRanges", session->m_trackingId, szMediaIp, mediaPort);
			LOG4CXX_INFO(m_log, logMsg);
			return;
		}
	}

	CStdString logMsg;

	unsigned long long mediaAddress;
	Craft64bitMediaAddress(mediaAddress, mediaIp, mediaPort);

	bool doChangeMediaAddress = true;

	VoIpSessionRef oldSession = findByMediaAddress(mediaIp, mediaPort);
	if(oldSession.get())
	{
		// A session exists on the same IP+port

		if(oldSession->m_trackingId.Equals(session->m_trackingId))
		{
			// Old and new are the same session, do nothing
			doChangeMediaAddress = false;
		}
		else if(oldSession->m_protocol == VoIpSession::ProtRawRtp || oldSession->m_numRtpPackets == 0 ||
			(session->m_protocol == VoIpSession::ProtSkinny && DLLCONFIG.m_skinnyAllowMediaAddressTransfer)
			|| (session->m_protocol == VoIpSession::ProtSip && DLLCONFIG.m_sipAllowMediaAddressTransfer))
		{
			logMsg.Format("[%s] on %s replaces [%s]",
							session->m_trackingId, MediaAddressToString(mediaAddress), oldSession->m_trackingId);
			LOG4CXX_INFO(m_log, logMsg);
			if(oldSession->m_protocol == VoIpSession::ProtRawRtp)
			{
				// Pure RTP session: stop it now or it will never be hoovered.
				// (Do not stop signalled sessions, better let them timeout and be hoovered. Useful for skinny internal calls where media address back and forth must not kill sessions with the best metadata.)
				Stop(oldSession);
			}
			else
			{
				// Signalled session, just remove them from the media address map so we make room for the new mapping
				RemoveFromMediaAddressMap(oldSession, mediaAddress);
			}
		}
		else
		{
			doChangeMediaAddress = false;
			logMsg.Format("[%s] on %s will not replace [%s]",
							session->m_trackingId, MediaAddressToString(mediaAddress), oldSession->m_trackingId);
			LOG4CXX_INFO(m_log, logMsg);
		}
	}
	if(doChangeMediaAddress)
	{
		if(m_log->isInfoEnabled())
		{
			char szEndPointIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&session->m_endPointIp, szEndPointIp, sizeof(szEndPointIp));
			logMsg.Format("[%s] media address:%s %s callId:%s endpoint:%s", session->m_trackingId, MediaAddressToString(mediaAddress), VoIpSession::ProtocolToString(session->m_protocol),session->m_callId, szEndPointIp);
			LOG4CXX_INFO(m_log, logMsg);
		}

		if (DLLCONFIG.m_rtpAllowMultipleMappings == false)
		{
			RemoveFromMediaAddressMap(session, session->m_ipAndPort);	// remove old mapping of the new session before remapping
			session->m_mediaAddresses.clear();
			//m_byIpAndPort.erase(session->m_ipAndPort);
		}
		session->m_mediaAddresses.push_back(mediaAddress);
		session->m_ipAndPort = mediaAddress;
		session->m_rtpIp = mediaIp;
		m_byIpAndPort.insert(std::make_pair(session->m_ipAndPort, session));	// insert new mapping

		CStdString numSessions = IntToString(m_byIpAndPort.size());
		LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);
	}
}

void VoIpSessions::RemoveFromMediaAddressMap(VoIpSessionRef& session, unsigned long long& mediaAddress)
{
	if(mediaAddress == 0)
	{
		return;
	}

	// Defensively check if the session referenced in the media address map actually is the same session
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	pair = m_byIpAndPort.find(mediaAddress);
	if (pair != m_byIpAndPort.end())
	{
		VoIpSessionRef sessionOnMap = pair->second;
		if(sessionOnMap.get() == session.get())
		{
			// They are the same session, all good
			m_byIpAndPort.erase(mediaAddress);
			CStdString numSessions = IntToString(m_byIpAndPort.size());
			LOG4CXX_DEBUG(m_log, CStdString("ByIpAndPort: ") + numSessions);
		}
		else
		{
			CStdString sessionOnMapTrackingId;
			if(sessionOnMap.get())
			{
				sessionOnMapTrackingId = sessionOnMap->m_trackingId;
			}
			else
			{
				sessionOnMapTrackingId = "null";
			}
			CStdString logString;
			logString.Format("rtp:%s belongs to [%s] not to [%s]", MediaAddressToString(mediaAddress), sessionOnMapTrackingId, session->m_trackingId);
			LOG4CXX_INFO(m_log, logString);
		}
	}
}


CStdString VoIpSessions::GenerateSkinnyCallId(struct in_addr endpointIp, unsigned short endpointSkinnyPort, unsigned int callId)
{
	char szEndPointIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&endpointIp, szEndPointIp, sizeof(szEndPointIp));
	CStdString skinnyCallId;
	if(DLLCONFIG.m_skinnyBehindNat == true)
	{
		skinnyCallId.Format("%u@%s,%u", callId, szEndPointIp, endpointSkinnyPort);
	}
	else
	{
		skinnyCallId.Format("%u@%s", callId, szEndPointIp);
	}
	return skinnyCallId;
}

void VoIpSessions::ReportSkinnyOpenReceiveChannelAck(SkOpenReceiveChannelAckStruct* openReceive, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	if(DLLCONFIG.m_skinnyIgnoreOpenReceiveChannelAck)
	{
		return;
	}
	VoIpSessionRef session = findNewestByEndpoint(openReceive->endpointIpAddr, ntohs(tcpHeader->source));
	if(session.get())
	{
		if(session->m_ipAndPort == 0 || DLLCONFIG.m_skinnyDynamicMediaAddress)
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
			VoIpSessionRef sessionExisting;

			sessionExisting = findByEndpointIp(openReceive->endpointIpAddr, 0);
			if(!sessionExisting.get())
			{
				VoIpEndpointInfoRef endpoint = GetVoIpEndpointInfo(openReceive->endpointIpAddr, ntohs(tcpHeader->source));
				char szEndpointIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&openReceive->endpointIpAddr, szEndpointIp, sizeof(szEndpointIp));

				// create new session and insert into the ipAndPort
				CStdString trackingId = m_alphaCounter.GetNext();
				VoIpSessionRef session(new VoIpSession(trackingId));
				session->m_endPointIp = openReceive->endpointIpAddr;
				session->m_endPointSignallingPort = ntohs(tcpHeader->source);
				session->m_protocol = VoIpSession::ProtSkinny;
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


void VoIpSessions::ReportSkinnyStartMediaTransmission(SkStartMediaTransmissionStruct* startMedia, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	VoIpSessionRef session = findNewestByEndpoint(ipHeader->ip_dest, ntohs(tcpHeader->dest));

	if(session.get())
	{
		if(session->m_ipAndPort == 0 || DLLCONFIG.m_skinnyDynamicMediaAddress)
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
			VoIpSessionRef sessionExisting;

			sessionExisting = findByEndpointIp(ipHeader->ip_dest, 0);
			if(!sessionExisting.get())
			{
				VoIpEndpointInfoRef endpoint = GetVoIpEndpointInfo(ipHeader->ip_dest, ntohs(tcpHeader->source));
				char szEndpointIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_dest, szEndpointIp, sizeof(szEndpointIp));

				// create new session and insert into the ipAndPort
				CStdString trackingId = m_alphaCounter.GetNext();
				VoIpSessionRef session(new VoIpSession(trackingId));
				session->m_endPointIp = ipHeader->ip_dest;	// CallInfo StartMediaTransmission always goes from CM to endpoint 
				session->m_protocol = VoIpSession::ProtSkinny;
				session->m_endPointSignallingPort = ntohs(tcpHeader->dest);
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

void VoIpSessions::ReportSkinnyStopMediaTransmission(SkStopMediaTransmissionStruct* stopMedia, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	if(DLLCONFIG.m_skinnyIgnoreStopMediaTransmission)
	{
		return;
	}
	CStdString conferenceId;
	CStdString passThruPartyId;
	CStdString skinnyCallId;
	std::map<CStdString, VoIpSessionRef>::iterator pair = m_byCallId.end();
	VoIpSessionRef session;

	// Try to locate the session using 1. conferenceId or 2. endpoint/passThruPartyId
	if(stopMedia->conferenceId != 0)
	{
		conferenceId = IntToString(stopMedia->conferenceId);
		skinnyCallId = GenerateSkinnyCallId(ipHeader->ip_dest, ntohs(tcpHeader->dest), stopMedia->conferenceId);
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

void VoIpSessions::SetEndpointExtension(CStdString& extension, struct in_addr* endpointIp, CStdString& callId, unsigned short skinnyPort)
{
	std::map<unsigned long long, VoIpEndpointInfoRef>::iterator pair;
	VoIpEndpointInfoRef endpoint;
	char szEndpointIp[16];
	unsigned long long ipAndPort;

	ACE_OS::inet_ntop(AF_INET, (void*)endpointIp, szEndpointIp, sizeof(szEndpointIp));
	Craft64bitMediaAddress(ipAndPort, *endpointIp, skinnyPort);

	pair = m_endpoints.find(ipAndPort);
	if(pair != m_endpoints.end())
	{
		// Update the existing endpoint	info
		endpoint = pair->second;
		endpoint->m_extension = extension;
		if(callId.size())
		{
			endpoint->m_latestCallId = callId;
		}
	}
	else
	{
		// Create endpoint info for the new endpoint
		CStdString logMsg;

		endpoint.reset(new VoIpEndpointInfo());
		endpoint->m_extension = extension;
		endpoint->m_skinnyPort = skinnyPort;

		memcpy(&endpoint->m_ip, endpointIp, sizeof(endpoint->m_ip));
		if(callId.size())
		{
			endpoint->m_latestCallId = callId;
		}
		m_endpoints.insert(std::make_pair(ipAndPort, endpoint));
		logMsg.Format("New endpoint created:%s callId:%s map:%s", endpoint->m_extension, endpoint->m_latestCallId, MediaAddressToString(ipAndPort));
		LOG4CXX_DEBUG(m_log, logMsg);
	}
	if(endpoint.get())
	{
		CStdString logMsg;

		logMsg.Format("Extension:%s callId:%s is on endpoint:%s", endpoint->m_extension, endpoint->m_latestCallId, szEndpointIp);
		LOG4CXX_INFO(m_log, logMsg);
	}
}

void VoIpSessions::ReportSkinnyLineStat(SkLineStatStruct* lineStat, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	CStdString callId = "";

	if(strlen(lineStat->lineDirNumber) > 1)
	{
		CStdString extension;

		extension = lineStat->lineDirNumber;
		SetEndpointExtension(extension, &ipHeader->ip_dest, callId, ntohs(tcpHeader->dest));
	}
}

void VoIpSessions::ReportSkinnySoftKeyHold(SkSoftKeyEventMessageStruct* skEvent, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	if(DLLCONFIG.m_skinnyIgnoreHold == true)
	{
		return;
	}
	VoIpSessionRef session;
	CStdString logMsg;

	std::map<CStdString, VoIpSessionRef>::iterator pair;
	CStdString callId = GenerateSkinnyCallId(ipHeader->ip_src, ntohs(tcpHeader->source), skEvent->callIdentifier);
	pair = m_byCallId.find(callId);
	if (pair != m_byCallId.end())
	{
		session = pair->second;
	}
	if(session.get())
	{
		session->GoOnHold(time(NULL));
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

void VoIpSessions::ReportSkinnySoftKeyResume(SkSoftKeyEventMessageStruct* skEvent, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	VoIpSessionRef session;
	CStdString logMsg;

	std::map<CStdString, VoIpSessionRef>::iterator pair;
	CStdString callId = GenerateSkinnyCallId(ipHeader->ip_src, ntohs(tcpHeader->source), skEvent->callIdentifier);
	pair = m_byCallId.find(callId);
	if (pair != m_byCallId.end())
	{
		session = pair->second;
	}
    if(session.get())
    {
		session->GoOffHold(time(NULL));
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
void VoIpSessions::ReportSkinnySoftKeyConfPressed(struct in_addr endpointIp, TcpHeaderStruct* tcpHeader)
{
	CStdString logMsg;
	time_t time_now = time(NULL);
	unsigned short skinnyPort;
	skinnyPort = ntohs(tcpHeader->source);
	VoIpEndpointInfoRef endpoint;
	char szEndpointIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&endpointIp, szEndpointIp, sizeof(szEndpointIp));

	endpoint = GetVoIpEndpointInfo(endpointIp, skinnyPort);
	if(endpoint == NULL)
	{
		logMsg.Format("ReportSkinnySoftKeyConfPressed: unable to find endpoint:%s,%d", szEndpointIp, skinnyPort);
		LOG4CXX_WARN(m_log, logMsg);
		return;
	}

	std::map<CStdString, VoIpSessionRef> ::iterator rtpSessionPair;
	VoIpSessionRef rtpSession;
	rtpSessionPair = m_byCallId.find(endpoint->m_latestCallId);
	if(rtpSessionPair == m_byCallId.end())
	{
		logMsg.Format("ReportSkinnySoftKeyConfPressed: unable to find callId:%s for endpoint:%s,%d", endpoint->m_latestCallId, szEndpointIp, skinnyPort);
		LOG4CXX_WARN(m_log, logMsg);
		return;
	}
	rtpSession = rtpSessionPair->second;

	if(endpoint->m_lastConnectedWithConference < endpoint->m_lastConferencePressed || endpoint->m_lastConferencePressed == 0)
	{
		CaptureEventRef event (new CaptureEvent());
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key = CONFERENCE_TRANSFER_TRACKING_TAG_KEY;
		event->m_value = rtpSession->GetOrkUid();
		endpoint->m_origOrkUid = rtpSession->GetOrkUid();
		g_captureEventCallBack(event, rtpSession->m_capturePort);

		logMsg.Format("ReportSkinnySoftKeyConfPressed:[%s] new conference, endpoint:%s,%d, tagging with orig-orkuid:%s", rtpSession->m_trackingId , szEndpointIp, skinnyPort, rtpSession->GetOrkUid());
		LOG4CXX_INFO(m_log, logMsg);
	}

	else
	{
		logMsg.Format("ReportSkinnySoftKeyConfPressed:[%s] existing conference, endpoint:%s,%d", rtpSession->m_trackingId, szEndpointIp, skinnyPort);
		LOG4CXX_WARN(m_log, logMsg);
	}
	endpoint->m_lastConferencePressed = time(NULL);
}
void VoIpSessions::ReportSkinnySoftKeySetConfConnected(struct in_addr endpointIp, TcpHeaderStruct* tcpHeader)
{
	CStdString logMsg;
	unsigned short skinnyPort;
	skinnyPort = ntohs(tcpHeader->dest);
	char szEndpointIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&endpointIp, szEndpointIp, sizeof(szEndpointIp));
	VoIpEndpointInfoRef endpoint;
	endpoint = GetVoIpEndpointInfo(endpointIp, skinnyPort);
	if(endpoint == NULL)
	{
		logMsg.Format("ReportSkinnySoftKeyConfConnected unable to find endpoint:%s,%d", szEndpointIp, skinnyPort);
		LOG4CXX_WARN(m_log, logMsg);
		return;
	}
	endpoint->m_lastConnectedWithConference = time(NULL);
}

void VoIpSessions::ReportSkinnySoftKeySetTransfConnected(SkSoftKeySetDescriptionStruct* skEvent, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	CStdString logMsg, skinnyCallId;
	skinnyCallId = GenerateSkinnyCallId(ipHeader->ip_dest, ntohs(tcpHeader->dest), skEvent->callIdentifier);
	std::map<CStdString, VoIpSessionRef>::iterator it = m_byCallId.end();
	VoIpSessionRef session;

	it = m_byCallId.find(skinnyCallId);
	if (it != m_byCallId.end())
	{
		session = it->second;
	}

	if(session.get())
	{
		logMsg.Format("[%s] is stop due to SoftkeySetTransferConnected", session->m_trackingId);
		LOG4CXX_INFO(m_log, logMsg);
		Stop(session);
	}
}

VoIpEndpointInfoRef VoIpSessions::GetVoIpEndpointInfo(struct in_addr endpointIp, unsigned short skinnyPort)
{
	char szEndpointIp[16];
	unsigned long long ipAndPort;
	std::map<unsigned long long, VoIpEndpointInfoRef>::iterator pair;

	ACE_OS::inet_ntop(AF_INET, (void*)&endpointIp, szEndpointIp, sizeof(szEndpointIp));
	Craft64bitMediaAddress(ipAndPort, endpointIp, skinnyPort);

	pair = m_endpoints.find(ipAndPort);
	if(pair != m_endpoints.end())
	{
		return pair->second;
	}

	return VoIpEndpointInfoRef();
}


void VoIpSessions::Stop(VoIpSessionRef& session)
{
	TaggingSipTransferCalls(session);
	session->Stop();

	if(session->m_callId.size() > 0)
	{
		m_byCallId.erase(session->m_callId);
	}

	std::list<unsigned long long>::iterator it;
	for(it = session->m_mediaAddresses.begin(); it != session->m_mediaAddresses.end(); it++)
	{
		unsigned long long mediaAddress = *it;
		RemoveFromMediaAddressMap(session, mediaAddress);
	}
	session->m_mediaAddresses.clear();
}

bool VoIpSessions::ReportRtcpSrcDescription(RtcpSrcDescriptionPacketInfoRef& rtcpInfo)
{
	VoIpSessionRef session;

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


void VoIpSessions::ReportRtpPacket(RtpPacketInfoRef& rtpPacket)
{
	int numSessionsFound = 0;
	VoIpSessionRef session1;
	VoIpSessionRef session2;
	VoIpSessionRef session;
	CStdString logMsg;

	int sourcePort = rtpPacket->m_sourcePort;
	int destPort = rtpPacket->m_destPort;
	bool sourceAddressIsTracked = false;

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
		sourceAddressIsTracked = true;
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
		VoIpSessionRef mergerSession;
		VoIpSessionRef mergeeSession;

		if(session1->m_protocol == VoIpSession::ProtRawRtp)
		{
			mergerSession = session2;
			mergeeSession = session1;
		}
		else if(session2->m_protocol == VoIpSession::ProtRawRtp)
		{
			mergerSession = session1;
			mergeeSession = session2;
		}
		else
		{
			// We dont merge/stop the sessions from information taking from SIP INVITE in Multi Mapping mode
			// Instead we keep both sessions and redirect the media to correct session.
			// In multiple mapping mode, each session can have several entries in the byIpAndPort map whereas m_ipAndPort only contains the latest media address.
			// So removing the session from the map with the m_ipAndPort key can be wrong
			if(DLLCONFIG.m_rtpAllowMultipleMappings == true && session1->m_protocol == VoIpSession::ProtSip && session2->m_protocol == VoIpSession::ProtSip)
			{
				if(session1->m_sipLastInvite > session2->m_sipLastInvite)
				{
					if(session2->m_numRtpPackets != 1)			//not the first packet
					{
						unsigned long long ipAndPort;
						Craft64bitMediaAddress(ipAndPort, rtpPacket->m_destIp, destPort);
						logMsg.Format("session[%s] takes over media address:%s from [%s]", session1->m_trackingId, MediaAddressToString(ipAndPort), session2->m_trackingId);
						LOG4CXX_INFO(m_log, logMsg);
						std::map<unsigned long long, VoIpSessionRef>::iterator it;
						it = m_byIpAndPort.find(ipAndPort);
						if(it != m_byIpAndPort.end())
						{
							m_byIpAndPort.erase(it);
						}
					}
				}
				else
				{
					if(session1->m_numRtpPackets != 1)			//not the first packet
					{
						unsigned long long ipAndPort;
						Craft64bitMediaAddress(ipAndPort, rtpPacket->m_sourceIp, sourcePort);
						logMsg.Format("session[%s] takes over media address:%s from [%s]", session2->m_trackingId, MediaAddressToString(ipAndPort), session1->m_trackingId);
						LOG4CXX_INFO(m_log, logMsg);
						std::map<unsigned long long, VoIpSessionRef>::iterator it;
						it = m_byIpAndPort.find(ipAndPort);
						if(it != m_byIpAndPort.end())
						{
							m_byIpAndPort.erase(it);
						}
					}
				}
				return;
			}
			else if(session1->m_numRtpPackets < session2->m_numRtpPackets && session1->m_numRtpPackets < 5)
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
				mergeeSession->m_trackingId, MediaAddressToString(mergeeSession->m_ipAndPort), mergeeSession->m_callId,
				mergerSession->m_trackingId, MediaAddressToString(mergerSession->m_ipAndPort), mergerSession->m_callId);
			LOG4CXX_INFO(m_log, debug);
		}
		Stop(mergeeSession);
	}
	else if(numSessionsFound == 1 )
	{
		if(DLLCONFIG.m_mediaAddressUseSecondRtpAddress == true)	//either source or dest media address is not already in tracking map
		{
			unsigned short trackingPort = 0;
			in_addr trackingIp;
			if(sourceAddressIsTracked == true)	//we need to add dest port into tracking map
			{
				trackingPort = rtpPacket->m_destPort;
				trackingIp = rtpPacket->m_destIp;
				SetMediaAddress(session, trackingIp, trackingPort);
			}
			else
			{
				trackingPort = rtpPacket->m_sourcePort;
				trackingIp = rtpPacket->m_sourceIp;
				SetMediaAddress(session, trackingIp, trackingPort);
			}
		}
		if (session->m_numRtpPackets == 1 && session->m_protocol == VoIpSession::ProtSip)
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
				SetMediaAddress(session, trackingIp, trackingPort);
			}
		}
	}
	else if((numSessionsFound == 0) && ((CONFIG.m_lookBackRecording == true) || DLLCONFIG.m_trackRawRtpSessionInNonLookBackMode == true))
	{
		VoIpEndpointInfoRef endpoint;

		// Check if a Skinny session can be found on the endpoint
		if(DLLCONFIG.m_skinnyRtpSearchesForCallInfo)
		{
			if(DLLCONFIG.m_skinnyBehindNat != true)
			{
				if(SkinnyFindMostLikelySessionForRtp(rtpPacket, endpoint) == true)
				{
					return;
				}
			}
			else
			{
				if(SkinnyFindMostLikelySessionForRtpBehindNat(rtpPacket) == true)
				{
					return;
				}
			}
		}

		// create new Raw RTP session and insert into IP+Port map
		CStdString trackingId = m_alphaCounter.GetNext();
		VoIpSessionRef session(new VoIpSession(trackingId));
		session->m_protocol = VoIpSession::ProtRawRtp;

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
		CStdString rtpString;
		rtpPacket->ToString(rtpString);
		LOG4CXX_INFO(m_log, "[" + trackingId + "] created by RTP packet " + rtpString);

		if(endpoint.get())
		{
			// Skinny endpoint was got but no RTP session or existing
			// RTP session is already stopped
			char szEndpointIp[16];
			char szRtpSrcIp[16];
			char szRtpDstIp[16];
			CStdString logMsg;

			ACE_OS::inet_ntop(AF_INET, (void*)&endpoint->m_ip, szEndpointIp, sizeof(szEndpointIp));
			ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_sourceIp, szRtpSrcIp, sizeof(szRtpSrcIp));
			ACE_OS::inet_ntop(AF_INET, (void*)&rtpPacket->m_destIp, szRtpDstIp, sizeof(szRtpDstIp));

			session->m_localParty = endpoint->m_extension;
			if(endpoint->m_ip.s_addr == rtpPacket->m_sourceIp.s_addr)
			{
				session->m_remoteParty = szRtpDstIp;
			}
			else
			{
				session->m_remoteParty = szRtpSrcIp;
			}

			logMsg.Format("[%s] RTP stream detected on endpoint:%s extension:%s RTP:%s,%d %s,%d", session->m_trackingId, szEndpointIp, endpoint->m_extension, szRtpSrcIp, rtpPacket->m_sourcePort, szRtpDstIp, rtpPacket->m_destPort);
			LOG4CXX_INFO(m_log, logMsg);

			return;
		}
	}
}

void VoIpSessions::TrySessionCallPickUp(CStdString replacesCallId, bool& result)
{
	CStdString logMsg;

	SipSubscribeSearchIndex& srchIndex = m_sipSubscribeMap.get<IndexSearchable>();
	SipSubscribeSearchIndex::iterator it = srchIndex.find(replacesCallId);
	if (it != srchIndex.end())
	{
		logMsg.Format("SipSubscribe callID:%s, event:%s is matched with INVITE", (*it)->m_callId, (*it)->m_event);
		LOG4CXX_INFO(m_log, logMsg);
		srchIndex.erase(it);
		result = true;
	}
}


void VoIpSessions::UnEscapeUrl(CStdString& in, CStdString& out)
{
	// Translates all %xx escaped sequences to corresponding ascii characters
	out = "";
	char pchHex[3];
	for (unsigned int i = 0; i<in.size(); i++)
	{
		switch (in.GetAt(i))
		{
			case '+':
				out += ' ';
				break;
			case '%':
				if (in.GetAt(++i) == '%')
				{
					out += '%';
					break;
				}
				pchHex[0] = in.GetAt(i);
				pchHex[1] = in.GetAt(++i);
				pchHex[2] = 0;
				out += (char)strtol(pchHex, NULL, 16);
				break;
			default:
				out += in.GetAt(i);
		}
	}
}

void VoIpSessions::UrlExtraction(CStdString& input, struct in_addr* endpointIp)
{
	// read string and extract values into map
	UrlState state = VoIpSessions::UrlStartState;
	CStdString key;
	CStdString value;
	CStdString errorDescription;

	input.Trim();

	for(unsigned int i=0; i<input.length() && state!= VoIpSessions::UrlErrorState; i++)
	{
		TCHAR character = input[i];

		switch(state)
		{
		case VoIpSessions::UrlStartState:
			if(character == '&')
			{
				;	// ignore ampersands
			}
			else if ( ACE_OS::ace_isalnum(character) )
			{
				state = VoIpSessions::UrlKeyState;
				key = character;
			}
			else
			{
				state = VoIpSessions::UrlErrorState;
				errorDescription = "Cannot find key start, keys must be alphanum";
			}
			break;
		case VoIpSessions::UrlKeyState:
			if( ACE_OS::ace_isalnum(character) )
			{
				key += character;
			}
			else if (character == '=')
			{
				state = VoIpSessions::UrlValueState;
				value.Empty();
			}
			else
			{
				state = VoIpSessions::UrlErrorState;
				errorDescription = "Invalid key character, keys must be alphanum";
			}
			break;
		case VoIpSessions::UrlValueState:
			if( character == '=')
			{
				state = VoIpSessions::UrlErrorState;
				errorDescription = "Value followed by = sign, value should always be followed by space sign";
			}
			else if (character == '&')
			{
				state = VoIpSessions::UrlStartState;
			}
			else
			{
				value += character;
			}
			break;
		default:
			state = VoIpSessions::UrlErrorState;
			errorDescription = "Non-existing state";
		}	// switch(state)

		if ( (state == VoIpSessions::UrlStartState) || (i == (input.length()-1)) )
		{
			if (!key.IsEmpty())
			{
				// Url unescape
				CStdString unescapedValue;
				UnEscapeUrl(value, unescapedValue);

				std::map<unsigned long long, VoIpEndpointInfoRef>::iterator pair;
				VoIpEndpointInfoRef endpoint;
				unsigned long long ipAndPort;

				Craft64bitMediaAddress(ipAndPort, *endpointIp, 0);

				pair = m_endpoints.find(ipAndPort);
				if(pair != m_endpoints.end())
				{
					endpoint = pair->second;
					std::map<CStdString, UrlExtractionValueRef>::iterator it;
					it = endpoint->m_urlExtractionMap.find(key);
					if(it != endpoint->m_urlExtractionMap.end())
					{
						it->second->m_value = unescapedValue;
						it->second->m_timestamp = time(NULL);
					}
					else
					{
						UrlExtractionValueRef urlExtractionValue(new UrlExtractionValue(unescapedValue));
						endpoint->m_urlExtractionMap.insert(std::make_pair(key, urlExtractionValue));
					}
				}
				else
				{
					UrlExtractionValueRef urlExtractionValue(new UrlExtractionValue(unescapedValue));
					endpoint.reset(new VoIpEndpointInfo);
					memcpy(&endpoint->m_ip, endpointIp, sizeof(endpoint->m_ip));
					endpoint->m_urlExtractionMap.insert(std::make_pair(key, urlExtractionValue));
					m_endpoints.insert(std::make_pair(ipAndPort, endpoint));

				}

				key.Empty();
				value.Empty();
			}
		}
	}

}

void VoIpSessions::ReportOnDemandMarkerByIp(struct in_addr endpointIp)
{
	VoIpSessionRef session, chosenSession;
	std::list<VoIpSessionRef> sessionsOnIp;
	std::map<unsigned long long, VoIpSessionRef>::iterator it;
	for(it = m_byIpAndPort.begin(); it != m_byIpAndPort.end(); it++)
	{
		session = it->second;
		if(session.get() != NULL)
		{
			if(session->IsMatchedLocalOrRemoteIp(endpointIp) == true && session->m_numRtpPackets > 0)
			{
				sessionsOnIp.push_back(session);
			}
		}
	}

	std::list<VoIpSessionRef>::iterator it2;
	time_t latestRtp = 0;
	for(it2 = sessionsOnIp.begin(); it2 != sessionsOnIp.end(); it2++)
	{
		session = (*it2);
		if(session.get() != NULL)
		{
			if(session->m_lastUpdated > latestRtp)
			{
				latestRtp = session->m_lastUpdated;
				chosenSession = session;
			}
		}
	}
	if(chosenSession.get() != NULL)
	{
		chosenSession->m_keepRtp = true;
		CStdString side = "both";
		chosenSession->MarkAsOnDemand(side);
	}

}

void VoIpSessions::TaggingSipTransferCalls(VoIpSessionRef& session)
{
	if((session.get() == NULL) || (session->m_protocol != VoIpSession::ProtSip) || (m_sipReferList.size() < 1))
	{
		return;
	}
	CStdString fromUser, toUser, logMsg;
	fromUser = VoIpSessionsSingleton::instance()->GetLocalPartyMap(session->m_invite->m_from);
	toUser = VoIpSessionsSingleton::instance()->GetLocalPartyMap(session->m_invite->m_to);
	std::list<SipReferRef>::iterator it;
	it = m_sipReferList.begin();
	while(it != m_sipReferList.end())
	{
		SipReferRef sipRefer = *it;
		int referLife = (int)(time(NULL) - sipRefer->m_timestamp);
		if(referLife > DLLCONFIG.m_transferTimeOutInSec )
		{
			it = m_sipReferList.erase(it);
			continue;
		}

		int timeDifFromCreation = (int)(sipRefer->m_timestamp - session->m_creationDate.sec());
		if((timeDifFromCreation > -2)
			&&(
					(toUser.CompareNoCase(sipRefer->m_referToParty) == 0)
			   )
			&&(
					fromUser.CompareNoCase(sipRefer->m_referredByParty) == 0 || (fromUser.CompareNoCase(sipRefer->m_from) == 0 || fromUser.CompareNoCase(sipRefer->m_to) == 0)
			   )
		   )
		{
			CaptureEventRef event (new CaptureEvent());
			event->m_type = CaptureEvent::EtKeyValue;
			event->m_key = CONFERENCE_TRANSFER_TRACKING_TAG_KEY;
			event->m_value = sipRefer->m_origOrkUid;
			g_captureEventCallBack(event, session->m_capturePort);
			logMsg.Format("transfer via SIP REFER:[%s] new leg, tagging with orig-orkuid:%s",session->m_trackingId, sipRefer->m_origOrkUid);
			LOG4CXX_INFO(m_log, logMsg);

			//Try to swap the correct external number into remote party, otherwise its displayed as internal ext<->internal ext call
			//Party which is matched siprefer->m_referredByParty need to be replaced
			//This swap will only work if the PBX(es) IP(s) is(are) present in SipDirectionReferenceIpAddresses
			if(session->m_remoteParty.CompareNoCase(sipRefer->m_referredByParty) == 0)
			{
				session->m_remoteParty = sipRefer->m_referredParty;
				CaptureEventRef event(new CaptureEvent());
				event->m_type = CaptureEvent::EtRemoteParty;
				event->m_value = session->m_remoteParty;
				g_captureEventCallBack(event, session->m_capturePort);
			}
		}
		it++;
	}
}

void VoIpSessions::CopyMetadataToNewSession(VoIpSessionRef& oldSession, VoIpSessionRef& newSession)
{
	if(oldSession.get() == NULL)
	{
		return;
	}
	newSession->m_ipAndPort = oldSession->m_ipAndPort;
	newSession->m_rtpIp = oldSession->m_rtpIp;
	newSession->m_callId = oldSession->m_callId;
	newSession->m_invite = oldSession->m_invite;
	newSession->m_protocol = oldSession->m_protocol;
	newSession->m_remotePartyNecSip = oldSession->m_remotePartyNecSip;
	newSession->m_localParty = oldSession->m_localParty;
	newSession->m_remoteParty = oldSession->m_remoteParty;
	newSession->m_localEntryPoint = oldSession->m_localEntryPoint;
	newSession->m_localPartyName = oldSession->m_localPartyName;
	newSession->m_remotePartyName = oldSession->m_remotePartyName;
	newSession->m_localPartyReported = oldSession->m_localPartyReported;
	newSession->m_remotePartyReported = oldSession->m_remotePartyReported;
	newSession->m_rtcpLocalParty = oldSession->m_rtcpLocalParty;
	newSession->m_rtcpRemoteParty = oldSession->m_rtcpRemoteParty;
	newSession->m_direction = oldSession->m_direction;
	newSession->m_localSide = oldSession->m_localSide;
	newSession->m_endPointIp = oldSession->m_endPointIp;
	newSession->m_endPointSignallingPort = oldSession->m_endPointSignallingPort;
	newSession->m_skinnyPassThruPartyId = oldSession->m_skinnyPassThruPartyId;
	newSession->m_sipLastInvite = oldSession->m_sipLastInvite;
	newSession->m_skinnyLastCallInfoTime = oldSession->m_skinnyLastCallInfoTime;
	newSession->m_skinnyLineInstance = oldSession->m_skinnyLineInstance;
	newSession->m_mediaAddresses = oldSession->m_mediaAddresses;
	newSession->m_sipDialedNumber = oldSession->m_sipDialedNumber;
	newSession->m_sipRemoteParty = oldSession->m_sipRemoteParty;
	newSession->m_isCallPickUp = oldSession->m_isCallPickUp;
	newSession->m_ssrcCandidate = oldSession->m_ssrcCandidate;

	newSession->m_metadataProcessed = true;
}

void VoIpSessions::ClearLocalPartyMap()
{
       m_localPartyMap.clear();
}

void VoIpSessions::StopAll()
{
	time_t forceExpiryTime = time(NULL) + 2*DLLCONFIG.m_rtpSessionOnHoldTimeOutSec;
	Hoover(forceExpiryTime);
}

void VoIpSessions::Hoover(time_t now)
{
	CStdString numSessions = IntToString(m_byIpAndPort.size());
	LOG4CXX_DEBUG(m_log, "Hoover - check " + numSessions + " sessions time:" + IntToString(now));

	// Go round the ipAndPort session index and find inactive sessions
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	std::map<CStdString, VoIpSessionRef>::iterator pair2;
	std::list<VoIpSessionRef> toDismiss;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end(); pair++)
	{
		VoIpSessionRef session = pair->second;
		int timeoutSeconds = 0;
		if(session->m_protocol == VoIpSession::ProtRawRtp)
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
	for (std::list<VoIpSessionRef>::iterator it = toDismiss.begin(); it != toDismiss.end() ; it++)
	{
		VoIpSessionRef session = *it;
		CStdString logMsg;
		logMsg.Format("[%s] %s Expired (RTP) ts:%u", session->m_trackingId, MediaAddressToString(session->m_ipAndPort), session->m_lastUpdated);
		LOG4CXX_INFO(m_log, logMsg);
		Stop(session);
	}

	// Go round the callId session index and find inactive sessions
	toDismiss.clear();
	for(pair2 = m_byCallId.begin(); pair2 != m_byCallId.end(); pair2++)
	{
		VoIpSessionRef session = pair2->second;

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
	for (std::list<VoIpSessionRef>::iterator it2 = toDismiss.begin(); it2 != toDismiss.end() ; it2++)
	{
		VoIpSessionRef session = *it2;
		CStdString logMsg;
		logMsg.Format("[%s] %s Expired (CallID) ts:%u", session->m_trackingId, MediaAddressToString(session->m_ipAndPort), session->m_lastUpdated);
		Stop(session);
	}
}

void VoIpSessions::StartCaptureOrkuid(CStdString& orkuid, CStdString& side)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	VoIpSessionRef session;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->OrkUidMatches(orkuid))
		{
			session->m_keepRtp = true;
			found = true;
		}
	}

	if(found)
	{
		if((CaptureEvent::AudioKeepDirectionEnum)CaptureEvent::AudioKeepDirectionToEnum(side) == CaptureEvent::AudioKeepDirectionInvalid)
		{
			LOG4CXX_WARN(m_log, "[" + session->m_trackingId + "] invalid side:" + side);
		}

		session->MarkAsOnDemand(side);

		logMsg.Format("[%s] StartCaptureOrkuid: Started capture, orkuid:%s side:%s(%d)", session->m_trackingId, orkuid, side, CaptureEvent::AudioKeepDirectionToEnum(side));
	}
	else
	{
		logMsg.Format("StartCaptureOrkuid: No session has orkuid:%s side:%s", orkuid, side);
	}

	LOG4CXX_INFO(m_log, logMsg);
}

CStdString VoIpSessions::StartCaptureNativeCallId(CStdString& nativecallid, CStdString& side)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	VoIpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->NativeCallIdMatches(nativecallid))
		{
			session->m_keepRtp = true;
			found = true;
			orkUid = session->GetOrkUid();
		}
	}

	if(found)
	{
		if((CaptureEvent::AudioKeepDirectionEnum)CaptureEvent::AudioKeepDirectionToEnum(side) == CaptureEvent::AudioKeepDirectionInvalid)
		{
			LOG4CXX_WARN(m_log, "[" + session->m_trackingId + "] invalid side:" + side);
		}

		session->MarkAsOnDemand(side);

		logMsg.Format("[%s] StartCaptureNativeCallId: Started capture, nativecallid:%s side:%s(%d)", session->m_trackingId, nativecallid, side, CaptureEvent::AudioKeepDirectionToEnum(side));
	}
	else
	{
		logMsg.Format("StartCaptureNativeCallId: No session has native callid:%s side:%s", nativecallid, side);
	}

	LOG4CXX_INFO(m_log, logMsg);

	return orkUid;
}

CStdString VoIpSessions::StartCapture(CStdString& party, CStdString& side)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	VoIpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if (session->PartyMatches(party))
		{
			session->m_keepRtp = true;
			found = true;
			orkUid = session->GetOrkUid();
		}
	}

	if(found)
	{
		if((CaptureEvent::AudioKeepDirectionEnum)CaptureEvent::AudioKeepDirectionToEnum(side) == CaptureEvent::AudioKeepDirectionInvalid)
		{
			LOG4CXX_WARN(m_log, "[" + session->m_trackingId + "] invalid side:" + side);
		}

		session->MarkAsOnDemand(side);

		logMsg.Format("[%s] StartCapture: Started capture, party:%s side:%s(%d)", session->m_trackingId, party, side, CaptureEvent::AudioKeepDirectionToEnum(side));
	}	
	else
	{
		logMsg.Format("StartCapture: No session has party %s side:%s", party, side);
	}
	
	LOG4CXX_INFO(m_log, logMsg);

	return orkUid;
}

CStdString VoIpSessions::PauseCapture(CStdString& party)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	VoIpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if (session->PartyMatches(party))
		{
			session->m_keepRtp = false;
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

void VoIpSessions::PauseCaptureOrkuid(CStdString& orkuid)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	VoIpSessionRef session;

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->OrkUidMatches(orkuid))
		{
			session->m_keepRtp = false;
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

CStdString VoIpSessions::PauseCaptureNativeCallId(CStdString& nativecallid)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	VoIpSessionRef session;
	CStdString orkUid = CStdString("");

	for(pair = m_byIpAndPort.begin(); pair != m_byIpAndPort.end() && found == false; pair++)
	{
		session = pair->second;

		if(session->NativeCallIdMatches(nativecallid))
		{
			session->m_keepRtp = false;
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

CStdString VoIpSessions::StopCapture(CStdString& party, CStdString& qos)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	VoIpSessionRef session;
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

		// This session might be stopped prematurely by this API call, preserve its metadata into a new session which will gather the subsequent RTP packets
		CStdString nextTrackId = m_alphaCounter.GetNext();
		VoIpSessionRef newSession(new VoIpSession(nextTrackId));
		CopyMetadataToNewSession(session, newSession);

		Stop(session);
		qos.Format("RtpNumPkts:%d RtpNumMissingPkts:%d RtpNumSeqGaps:%d RtpMaxSeqGap:%d", session->m_numRtpPackets, session->m_rtpNumMissingPkts, session->m_rtpNumSeqGaps, session->m_highestRtpSeqNumDelta);

		m_byIpAndPort.insert(std::make_pair(newSession->m_ipAndPort, newSession));
		m_byCallId.insert(std::make_pair(newSession->m_callId, newSession));
	}	
	else
	{
		logMsg.Format("StopCapture: No session has party %s", party);
		LOG4CXX_INFO(m_log, logMsg);
	}

	return orkUid;
}

void VoIpSessions::StopCaptureOrkuid(CStdString& orkuid, CStdString& qos)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	VoIpSessionRef session;

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

		// This session might be stopped prematurely by this API call, preserve its metadata into a new session which will gather the subsequent RTP packets
		CStdString nextTrackId = m_alphaCounter.GetNext();
		VoIpSessionRef newSession(new VoIpSession(nextTrackId));
		CopyMetadataToNewSession(session, newSession);

		Stop(session);
		qos.Format("RtpNumPkts:%d RtpNumMissingPkts:%d RtpNumSeqGaps:%d RtpMaxSeqGap:%d", session->m_numRtpPackets, session->m_rtpNumMissingPkts, session->m_rtpNumSeqGaps, session->m_highestRtpSeqNumDelta);

		m_byIpAndPort.insert(std::make_pair(newSession->m_ipAndPort, newSession));
		m_byCallId.insert(std::make_pair(newSession->m_callId, newSession));
	}
	else
	{
		logMsg.Format("StopCaptureOrkuid: No session has orkuid:%s", orkuid);
		LOG4CXX_INFO(m_log, logMsg);
	}
}

CStdString VoIpSessions::StopCaptureNativeCallId(CStdString& nativecallid, CStdString& qos)
{
	std::map<unsigned long long, VoIpSessionRef>::iterator pair;
	bool found = false;
	CStdString logMsg;
	VoIpSessionRef session;
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

		// This session might be stopped prematurely by this API call, preserve its metadata into a new session which will gather the subsequent RTP packets
		CStdString nextTrackId = m_alphaCounter.GetNext();
		VoIpSessionRef newSession(new VoIpSession(nextTrackId));
		CopyMetadataToNewSession(session, newSession);

		Stop(session);
		qos.Format("RtpNumPkts:%d RtpNumMissingPkts:%d RtpNumSeqGaps:%d RtpMaxSeqGap:%d", session->m_numRtpPackets, session->m_rtpNumMissingPkts, session->m_rtpNumSeqGaps, session->m_highestRtpSeqNumDelta);

		m_byIpAndPort.insert(std::make_pair(newSession->m_ipAndPort, newSession));
		m_byCallId.insert(std::make_pair(newSession->m_callId, newSession));
	}
	else
	{
		logMsg.Format("StopCaptureNativeCallId: No session has native callid:%s", nativecallid);
		LOG4CXX_INFO(m_log, logMsg);
	}

	return orkUid;
}

void VoIpSessions::SaveLocalPartyMap(CStdString& oldparty, CStdString& newparty)
{
	m_localPartyMap.insert(std::make_pair(oldparty, newparty));
	LOG4CXX_DEBUG(m_log, "Saved map oldparty:" + oldparty + " newparty:" + newparty);
}

CStdString VoIpSessions::GetLocalPartyMap(CStdString& oldlocalparty)
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

void VoIpSessions::SaveSkinnyGlobalNumbersList(CStdString& number)
{
	m_skinnyGlobalNumbersList.insert(std::make_pair(number, 0));
	LOG4CXX_DEBUG(m_log, "Saved skinny global number:" + number);
}
//============================================================
UrlExtractionValue::UrlExtractionValue()
{
	m_timestamp = time(NULL);
}

UrlExtractionValue::UrlExtractionValue(CStdString value)
{
	m_value = value;
	m_timestamp = time(NULL);
}

//================================================

VoIpEndpointInfo::VoIpEndpointInfo()
{
	m_lastConferencePressed = 0;
	m_lastConnectedWithConference = 0;
	m_origOrkUid = "";
}
