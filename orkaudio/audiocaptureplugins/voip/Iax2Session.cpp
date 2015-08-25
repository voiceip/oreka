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
#include "Iax2Session.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include <list>
#include "ConfigManager.h"
#include "VoIpConfig.h"
#include "ace/OS_NS_arpa_inet.h"

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;

Iax2Session::Iax2Session(CStdString& trackingId)
{
	m_trackingId = trackingId;
	m_lastUpdated = time(NULL);
	m_log = Logger::getLogger("Iax2session");
	m_invitorIp.s_addr = 0;
	m_inviteeIp.s_addr = 0;
	m_localIp.s_addr = 0;
	m_remoteIp.s_addr = 0;
	m_direction = CaptureEvent::DirUnkn;
	m_numIax2Packets = 0;
	m_started = false;
	m_stopped = false;
	m_beginDate = 0;
	m_codec = 0;
	m_hasDuplicateIax2 = false;
	m_iax2_state = IAX2_STATE_WAITING;
	m_invitor_scallno = 0;
	m_invitee_scallno = 0;
	m_channel1SeqNo = 1;
	m_channel2SeqNo = 1;

	/* Not sure if IAX2 needs these... */
	m_highestIax2SeqNumDelta = 0;
	m_minIax2SeqDelta = (double)DLLCONFIG.m_rtpDiscontinuityMinSeqDelta;
	m_minIax2TimestampDelta = (double)DLLCONFIG.m_rtpDiscontinuityMinSeqDelta * 160;
}

void Iax2Session::Stop()
{
	CStdString logMsg;

	logMsg.Format("[%s] %s Session stop, numIax2Pkts:%d dupl:%d seqDelta:%d "
			"lastUpdated:%u", m_trackingId, m_capturePort, m_numIax2Packets,
			m_hasDuplicateIax2, m_highestIax2SeqNumDelta, m_lastUpdated);

	LOG4CXX_INFO(m_log, logMsg);

	if(m_started && !m_stopped) {
		CaptureEventRef stopEvent(new CaptureEvent);
		stopEvent->m_type = CaptureEvent::EtStop;
		stopEvent->m_timestamp = m_lastUpdated;
		g_captureEventCallBack(stopEvent, m_capturePort);
		m_stopped = true;
	}
}

void Iax2Session::Start()
{
	CaptureEventRef startEvent(new CaptureEvent);

	m_started = true;
	time(&m_beginDate);
	GenerateOrkUid();
	startEvent->m_type = CaptureEvent::EtStart;
	startEvent->m_timestamp = m_beginDate;
	startEvent->m_value = m_trackingId;
	CStdString timestamp = IntToString(startEvent->m_timestamp);

	LOG4CXX_INFO(m_log,  "[" + m_trackingId + "] " + m_capturePort + " " +
			IAX2_PROTOCOL_STR + " Session start, timestamp:" + timestamp);

	g_captureEventCallBack(startEvent, m_capturePort);
}

void Iax2Session::GenerateOrkUid()
{
	struct tm date = {0};
	int month, year;

	ACE_OS::localtime_r(&m_beginDate, &date);

	month = date.tm_mon + 1;
	year = date.tm_year + 1900;

	m_orkUid.Format("%.4d%.2d%.2d_%.2d%.2d%.2d_%s", year, month, date.tm_mday,
			date.tm_hour, date.tm_min, date.tm_sec, m_trackingId);
}

/* We index with the invitor because the invitor is the first party
 * that provides us with complete information i.e the IP address
 * and the call number, which we need.  Remember, though that
 * call numbers are scarce by nature and may be reused. */
void Iax2Session::ProcessMetadataIax2Incoming()
{
	char szNewSrcIp[16];

	ACE_OS::inet_ntop(AF_INET, (void*)&m_invitorIp, szNewSrcIp, sizeof(szNewSrcIp));

	m_remoteParty = m_new->m_caller;
	m_localParty = (m_new->m_localExtension.size() ? m_new->m_localExtension : m_new->m_callee);
	m_localEntryPoint = (m_new->m_localExtension.size() ? m_new->m_callee : "");
	m_direction = CaptureEvent::DirIn;
	m_localIp = m_inviteeIp;
	m_remoteIp = m_invitorIp;
	m_capturePort.Format("%s,%d", szNewSrcIp, m_invitor_scallno);
}

/* We index with the invitor because the invitor is the first party
 * that provides us with complete information i.e the IP address
 * and the call number, which we need.  Remember, though that
 * call numbers are scarce by nature and may be reused. */
void Iax2Session::ProcessMetadataIax2Outgoing()
{
	char szNewSrcIp[16];

	ACE_OS::inet_ntop(AF_INET, (void*)&m_invitorIp, szNewSrcIp, sizeof(szNewSrcIp));
	m_remoteParty = m_new->m_callee;
	m_localParty = (m_new->m_localExtension.size() ? m_new->m_localExtension : m_new->m_caller);
	m_localEntryPoint = (m_new->m_localExtension.size() ? m_new->m_caller : "");
	m_direction = CaptureEvent::DirOut;
	m_capturePort.Format("%s,%d", szNewSrcIp, m_invitor_scallno);
	m_localIp = m_invitorIp;
	m_remoteIp = m_inviteeIp;
}

/* Is this function ever necessary? */
void Iax2Session::UpdateMetadataIax2(Iax2PacketInfoRef& iax2Packet, bool sourceIax2AddressIsNew)
{
	// Find out if the new IAX2 packet could match one of the IAX2 NEW messages associated with the session
	Iax2NewInfoRef invite, tmpNew;
	std::list<Iax2NewInfoRef>::iterator it;

	for(it = m_news.begin(); it != m_news.end(); it++) {
		tmpNew = *it;

		if(tmpNew->m_validated)
			break;

		if(sourceIax2AddressIsNew) {
			if((unsigned int)(iax2Packet->m_sourceIp.s_addr) ==
				(unsigned int)(tmpNew->m_receiverIp.s_addr))
				invite = tmpNew;
		} else {
			if((unsigned int)(iax2Packet->m_destIp.s_addr) ==
				(unsigned int)(tmpNew->m_receiverIp.s_addr))
				invite = tmpNew;
		}
	}

	if(invite.get()) {
		CStdString inviteString, iax2String, logMsg;

		// The NEW has generated an IAX2 voice stream
		invite->m_validated = true;

		// Update session metadata with NEW info
		m_remoteParty = invite->m_caller;
		m_localParty = (invite->m_localExtension.size() ? invite->m_localExtension : invite->m_callee);
		m_localEntryPoint = (invite->m_localExtension.size() ? invite->m_callee : "");
		m_localIp = invite->m_receiverIp;

		// Do some logging
		invite->ToString(inviteString);
		iax2Packet->ToString(iax2String);

		logMsg.Format("[%s] metadata update: local:%s remote:%s "
				"IAX2 Pkt:%s NEW Info:%s", m_trackingId, m_localParty,
				m_remoteParty, iax2String, inviteString);
		LOG4CXX_INFO(m_log, logMsg);

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

		// Report local entry point
		if(m_localEntryPoint.size())
		{
			event.reset(new CaptureEvent());
			event->m_type = CaptureEvent::EtLocalEntryPoint;
			event->m_value = m_localEntryPoint;
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

void Iax2Session::ProcessMetadataIax2(Iax2PacketInfoRef& iax2Packet)
{
	/* This is the first full IAX2_FRAME_VOICE frame which will tell us
	 * the codec being used */
	if(((unsigned int)iax2Packet->m_sourceIp.s_addr != (unsigned int)m_invitorIp.s_addr) &&
	   ((unsigned int)iax2Packet->m_destIp.s_addr != (unsigned int)m_invitorIp.s_addr))
	{
		LOG4CXX_ERROR(m_log,  "[" + m_trackingId + "] " + m_srcIpAndCallNo + " alien IAX2 packet");
	}

	/* Obtain the codec information */
	m_codec = iax2Packet->m_payloadType;

	// work out capture port and direction
	if(DLLCONFIG.IsMediaGateway(m_invitorIp)) {
		if(DLLCONFIG.IsMediaGateway(m_inviteeIp)) {
			// Media gateway talking to media gateway, this is probably incoming
			ProcessMetadataIax2Incoming();
		} else if(DLLCONFIG.IsPartOfLan(m_inviteeIp)) {
			// Gateway to LAN, this is pobably incoming
			ProcessMetadataIax2Incoming();
		} else {
			// Gateway to outside address, probably outgoing but treat as incoming for now because
			// It can be due to misconfigured LAN Mask, odds are it's still incoming.
			ProcessMetadataIax2Incoming();
		}
	} else if (DLLCONFIG.IsPartOfLan(m_invitorIp)) {
		ProcessMetadataIax2Outgoing();
	} else {
		// SIP invitor media IP address is an outside IP address
		if(DLLCONFIG.IsMediaGateway(m_inviteeIp)) {
			ProcessMetadataIax2Incoming();
		} else if(DLLCONFIG.IsPartOfLan(m_inviteeIp)) {
			ProcessMetadataIax2Incoming();
		} else {
			// SIP invitee media address is an outside IP address
			ProcessMetadataIax2Outgoing();
		}
	}
}

void Iax2Session::ReportMetadata()
{
	char szLocalIp[16], szRemoteIp[16];

	ACE_OS::inet_ntop(AF_INET, (void*)&m_localIp, szLocalIp, sizeof(szLocalIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&m_remoteIp, szRemoteIp, sizeof(szRemoteIp));

	// Make sure Local Party is always reported
	if(m_localParty.IsEmpty()) {
		m_localParty = szLocalIp;
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

	// Report local entry point
	if(m_localEntryPoint.size())
	{
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtLocalEntryPoint;
		event->m_value = m_localEntryPoint;
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
	event->m_value = m_srcIpAndCallNo;
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

static const char *iax2_state_to_str(int iax2state)
{
	switch(iax2state) {
	case IAX2_STATE_WAITING:
		return "WAITING";
	case IAX2_STATE_LINKED:
                return "LINKED";
        case IAX2_STATE_UP:
                return "UP";
	default:
		return "UNKNOWN";
	}

	/* NOT REACHED */
	return "UNKNOWN";
}

/*
 * Returns false if packet is received out-of-sync (when we are in the wrong state)
 */
bool Iax2Session::AddIax2Packet(Iax2PacketInfoRef& iax2Packet)
{
	CStdString logMsg;
	unsigned char channel = 0;

	/* What is our state?  We need to be in the IAX2_STATE_LINKED state
	 * or the IAX2_STATE_UP state to receive any voice frames */
	if((m_iax2_state != IAX2_STATE_LINKED) && (m_iax2_state != IAX2_STATE_UP)) {
		logMsg.Format("[%s] receiving voice packets while in %s state? "
				"Destroying session...", m_trackingId,
				iax2_state_to_str(m_iax2_state));
		LOG4CXX_INFO(m_log, logMsg);
		return false;
	}

	/* We do not have a voice packet yet */
	if(m_lastIax2Packet.get() == NULL) {
		/* The first IAX2 voice frame is expected to be a full frame,
		 * complete with the payload type information.  If this is not
		 * the case, we will continue dropping any mini frames */

		if(iax2Packet->m_frame_type != IAX2_FRAME_FULL) {
			logMsg.Format("[%s] 1st packet is not a full frame! Dropping...", m_trackingId);
			LOG4CXX_ERROR(m_log, logMsg);
			return true;
		}

		ProcessMetadataIax2(iax2Packet);
		m_iax2_state = IAX2_STATE_UP;
	}

	/* If we get another full voice packet, we need to update our codec 
	 * information */
	if(iax2Packet->m_frame_type == IAX2_FRAME_FULL)
	{
		m_codec = iax2Packet->m_payloadType;
	}

	m_lastIax2Packet = iax2Packet;
	if(m_lastIax2PacketSide1.get() == NULL) {
		// First IAX2 packet for side 1
		m_lastIax2PacketSide1 = iax2Packet;
		channel = 1;
		if(m_log->isInfoEnabled())
		{
			iax2Packet->ToString(logMsg);
			logMsg =  "[" + m_trackingId + "] 1st packet s1: " + logMsg;
			LOG4CXX_INFO(m_log, logMsg);
		}
	} else {
		// Comparing destination IP address to find out if side1, see (1)
		if((unsigned int)iax2Packet->m_destIp.s_addr == (unsigned int)m_lastIax2PacketSide1->m_destIp.s_addr)
		{
			if(DLLCONFIG.m_Iax2RewriteTimestamps == true)
			{
				iax2Packet->m_timestamp =+ m_lastIax2PacketSide1->m_timestamp + 20;
			}
			else if(iax2Packet->m_timestamp == m_lastIax2PacketSide1->m_timestamp)
			{
				m_hasDuplicateIax2 = true;
				return true;	// dismiss duplicate IAX2 packet
			}
			/* XXX Detect discontinuity using timestamps? */
			/*
			else
			{
				double seqNumDelta = (double)iax2Packet->m_seqNum - (double)m_lastIax2PacketSide1->m_seqNum;
				if(DLLCONFIG.m_iax2DiscontinuityDetect)
				{
					double timestampDelta = (double)iax2Packet->m_timestamp - (double)m_lastIax2PacketSide1->m_timestamp;
					if(	abs(seqNumDelta) > m_minIax2SeqDelta  &&
						abs(timestampDelta) > m_minIax2TimestampDelta)	
					{
						logMsg.Format("[%s] IAX2 discontinuity s1: before: seq:%u ts:%u after: seq:%u ts:%u", 
							m_trackingId, m_lastIax2PacketSide1->m_seqNum, m_lastIax2PacketSide1->m_timestamp, 
							iax2Packet->m_seqNum, iax2Packet->m_timestamp);
						LOG4CXX_INFO(m_log, logMsg);
						return false;
					}
				}
				if(seqNumDelta > (double)m_highestIax2SeqNumDelta)
				{
					m_highestIax2SeqNumDelta = (unsigned int)seqNumDelta;
				}
			} */
			m_lastIax2PacketSide1 = iax2Packet;
			channel = 1;
		}
		else
		{
			if(m_lastIax2PacketSide2.get() == NULL)
			{
				// First IAX2 packet for side 2
				if(m_log->isInfoEnabled())
				{
					iax2Packet->ToString(logMsg);
					logMsg =  "[" + m_trackingId + "] 1st packet s2: " + logMsg;
					LOG4CXX_INFO(m_log, logMsg);
				}
			}
			else
			{
				if(DLLCONFIG.m_Iax2RewriteTimestamps == true)
				{
					iax2Packet->m_timestamp += m_lastIax2PacketSide2->m_timestamp + 20;
				}
				else if(iax2Packet->m_timestamp == m_lastIax2PacketSide2->m_timestamp)
				{
					m_hasDuplicateIax2 = true;
					return true;	// dismiss duplicate IAX2 packet
				}
				/* XXX Detect discontinuity using timestamps? */
                                /*
				else
				{
					double seqNumDelta = (double)iax2Packet->m_seqNum - (double)m_lastIax2PacketSide2->m_seqNum;
					if(DLLCONFIG.m_iax2DiscontinuityDetect)
					{
						double timestampDelta = (double)iax2Packet->m_timestamp - (double)m_lastIax2PacketSide2->m_timestamp;
						if(	abs(seqNumDelta) > m_minIax2SeqDelta  &&
							abs(timestampDelta) > m_minIax2TimestampDelta)	
						{
							logMsg.Format("[%s] IAX2 discontinuity s2: before: seq:%u ts:%u after: seq:%u ts:%u", 
								m_trackingId, m_lastIax2PacketSide2->m_seqNum, m_lastIax2PacketSide2->m_timestamp, 
								iax2Packet->m_seqNum, iax2Packet->m_timestamp);
							LOG4CXX_INFO(m_log, logMsg);
							return false;
						}
					}
					if(seqNumDelta > (double)m_highestIax2SeqNumDelta)
					{
						m_highestIax2SeqNumDelta = (unsigned int)seqNumDelta;
					}
				}*/
			}
			m_lastIax2PacketSide2 = iax2Packet;
			channel = 2;
		}
	}

	m_numIax2Packets++;

	/* Can stream change happen with IAX2??
	bool hasSourceAddress = m_iax2AddressList.HasAddressOrAdd(iax2Packet->m_sourceIp, iax2Packet->m_sourcePort);
	bool hasDestAddress = m_iax2AddressList.HasAddressOrAdd(iax2Packet->m_destIp, iax2Packet->m_destPort);
	if(	hasSourceAddress == false || hasDestAddress == false )
	{
		iax2Packet->ToString(logMsg);
		logMsg.Format("[%s] new IAX2 stream s%d: %s", 
							m_trackingId, channel, logMsg);
		LOG4CXX_INFO(m_log, logMsg);

		if(m_protocol == ProtIax2 && m_started)	// make sure this only happens if ReportMetadata() already been called for the session
		{
			UpdateMetadataIax2(iax2Packet, hasDestAddress);
		}
	}
	*/

	if(m_numIax2Packets == 1) {
		Start();
		ReportMetadata();
	}

	unsigned short seq = 0;

	if(channel == 1)
	{
		m_channel1SeqNo += 1;
		if(m_channel1SeqNo >= 65535)
		{
			m_channel1SeqNo = 1;
		}

		seq = m_channel1SeqNo;
	}
	else
	{
		m_channel2SeqNo += 1;
		if(m_channel2SeqNo >= 65535)
		{
			m_channel2SeqNo = 1;
		}

		seq = m_channel2SeqNo;
	}

	if(m_started) {
		AudioChunkDetails details;
		AudioChunkRef chunk(new AudioChunk());

		details.m_arrivalTimestamp = iax2Packet->m_arrivalTimestamp;
		details.m_numBytes = iax2Packet->m_payloadSize;
		details.m_timestamp = (channel == 1 ? m_channel1SeqNo : m_channel2SeqNo) * RtpTimestamp();
		details.m_rtpPayloadType = m_codec;
		details.m_channel = channel;
		details.m_sequenceNumber = seq;
		details.m_encoding = AlawAudio;

		chunk->SetBuffer(iax2Packet->m_payload, details);
		g_audioChunkCallBack(chunk, m_capturePort);

		m_lastUpdated = iax2Packet->m_arrivalTimestamp;
	}

	return true;
}

int Iax2Session::RtpTimestamp()
{
	// This is obtained from the definition of the RTP timestamp which is
	// For audio, the timestamp is incremented by the packetization interval times the sampling rate. For example, for audio packets containing 20 ms of audio sampled at 8,000 Hz, the timestamp for each block of audio increases by 160, even if the block is not sent due to silence suppression. Also, note that the actual sampling rate will differ slightly from this nominal rate, but the sender typically has no reliable way to measure this divergence
	// as obtained from http://www.cs.columbia.edu/~hgs/rtp/faq.html#timestamp-computed

	int ts = 0;

	switch(m_codec)
	{
	case 0: // ULAW 8Khz sample rate
	case 8: // ALAW 8Khz sample rate
		ts = 160; // (20 ms) * (8 samples per ms)
	case 18: // G729
		ts = 20; // (20 ms) * (1 sample per ms)
	case 4: // G723
		ts = 24; // (30 ms) * (0.7875 samples per ms)
	default:
		ts = 160;
	}

	return ts;
}

/* Report AUTHREQ so as to get the invitee call number */
void Iax2Session::ReportIax2Authreq(Iax2AuthreqInfoRef& authreq)
{
	m_invitee_scallno = StringToInt(authreq->m_sender_callno);
}

/* Obtain the invitee call number from the ACCEPT, in case an AUTHREQ
 * was not sent */
void Iax2Session::ReportIax2Accept(Iax2AcceptInfoRef& acceptinfo)
{
        m_invitee_scallno = StringToInt(acceptinfo->m_sender_callno);
}

/* Report NEW */
void Iax2Session::ReportIax2New(Iax2NewInfoRef& invite)
{
	CStdString key;
	CStdString value;

	if(DLLCONFIG.m_iax2TreatCallerIdNameAsXUniqueId == true)
	{
		int pos = 0;

		pos = invite->m_callingName.Find(' ');
		if(pos >= 0)
		{
			CStdString uniqueId;
			CStdString localExt;

			if(pos > 0)
			{
				uniqueId = invite->m_callingName.Left(pos);
			}

			localExt = invite->m_callingName.substr(pos+1);
			LOG4CXX_DEBUG(m_log, "separated IAX2 calling name: uniqueid:" + uniqueId + " localext:" + localExt);

			key = "X-Unique-ID";
			value = uniqueId;
			m_tags.insert(std::make_pair(key, value));
			invite->m_localExtension = localExt;
		}
		else
		{
			key = "X-Unique-ID";
			value = invite->m_callingName;
			m_tags.insert(std::make_pair(key, value));
		}
	}

	if(m_new.get() == NULL) {
	        char szFromIax2Ip[16];

	        ACE_OS::inet_ntop(AF_INET, (void*)&invite->m_senderIp, szFromIax2Ip, sizeof(szFromIax2Ip));

		m_new = invite;
		m_invitorIp = invite->m_senderIp;
		m_inviteeIp = invite->m_receiverIp;
		m_invitor_scallno = StringToInt(invite->m_callNo);
		m_srcIpAndCallNo = CStdString(szFromIax2Ip) + "," + invite->m_callNo;
	} else {
		CStdString inviteString;
		invite->ToString(inviteString);
		CStdString logMsg;
		logMsg.Format("[%s] associating NEW:%s", m_trackingId, inviteString);
		LOG4CXX_INFO(m_log, logMsg);
	}

	m_news.push_front(invite);
}


//=====================================================================
Iax2Sessions::Iax2Sessions()
{
	m_log = Logger::getLogger("iax2sessions");
	if(CONFIG.m_debug) {
		m_alphaCounter.Reset();
	}
}

/* In the cases where AUTHREQ was not sent, we need to still obtain the
 * invitee call number and make a map for it */
void Iax2Sessions::ReportIax2Accept(Iax2AcceptInfoRef& acceptinfo)
{
        CStdString srcIpAndCallNo, log_msg, destIpAndCallNo, numSessions, acceptString;
        std::map<CStdString, Iax2SessionRef>::iterator pair;
        char invitor_ip[16], invitee_ip[16];
        Iax2SessionRef session;

        /* Assumption is that the receiver is the invitor */
        ACE_OS::inet_ntop(AF_INET, (void*)&acceptinfo->m_receiverIp, invitor_ip, sizeof(invitor_ip));
        ACE_OS::inet_ntop(AF_INET, (void*)&acceptinfo->m_senderIp, invitee_ip, sizeof(invitee_ip));
        srcIpAndCallNo = CStdString(invitor_ip) + "," + acceptinfo->m_receiver_callno;
        destIpAndCallNo = CStdString(invitee_ip) + "," + acceptinfo->m_sender_callno;

        pair = m_bySrcIpAndCallNo.find(srcIpAndCallNo);
        if(pair != m_bySrcIpAndCallNo.end()) {
		session = pair->second;
		if(session.get() != NULL) {
			/* Excellent! We have an existing session.  Now check if we have
			 * already mapped the invitee call number and IP address */

			pair = m_byDestIpAndCallNo.find(destIpAndCallNo);
			if(pair != m_byDestIpAndCallNo.end()) {
				/* We already have a mapping and all necessary information */

				session = pair->second;
				if(session.get() != NULL) {
					session->m_iax2_state = IAX2_STATE_LINKED;
					LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] "
							"received ACCEPT. Session already "
							"registered.  Now LINKED.");
					return;
				}
			}

			/* We need to enter the information */
			log_msg.Format("[%s] got ACCEPT for this session.  Registering "
					"sender call number", session->m_trackingId);
	                LOG4CXX_INFO(m_log, log_msg);

			session->m_destIpAndCallNo = destIpAndCallNo;
			session->m_iax2_state = IAX2_STATE_LINKED;

			m_byDestIpAndCallNo.insert(std::make_pair(session->m_destIpAndCallNo, session));
			session->ReportIax2Accept(acceptinfo);

			acceptinfo->ToString(acceptString);
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] supplemented "
					"by IAX2 ACCEPT:" + acceptString + ". Status now LINKED");

	                return;
		}
	}

        /* We don't have an existing session? */
        acceptinfo->ToString(acceptString);
        log_msg.Format("Got ACCEPT [%s] but couldn't find corresponding NEW", acceptString);
        LOG4CXX_ERROR(m_log, log_msg);

        return;
}

/* After a NEW has been sent, an AUTHREQ may be sent by the sender and this
 * is the stage where we grab the callno of the sender */
void Iax2Sessions::ReportIax2Authreq(Iax2AuthreqInfoRef& authreq)
{
	CStdString srcIpAndCallNo, log_msg, destIpAndCallNo, numSessions, authreqString;
	std::map<CStdString, Iax2SessionRef>::iterator pair;
	char invitor_ip[16], invitee_ip[16];
	Iax2SessionRef session;

	/* Assumption is that the receiver is the invitor */
	ACE_OS::inet_ntop(AF_INET, (void*)&authreq->m_receiverIp, invitor_ip, sizeof(invitor_ip));
	ACE_OS::inet_ntop(AF_INET, (void*)&authreq->m_senderIp, invitee_ip, sizeof(invitee_ip));
	srcIpAndCallNo = CStdString(invitor_ip) + "," + authreq->m_receiver_callno;
	destIpAndCallNo = CStdString(invitee_ip) + "," + authreq->m_sender_callno;

	pair = m_bySrcIpAndCallNo.find(srcIpAndCallNo);
	if(pair != m_bySrcIpAndCallNo.end()) {
		/* Excellent! We have an existing session */
		session = pair->second;

		if(session.get() != NULL) {
			session->m_destIpAndCallNo = destIpAndCallNo;
			m_byDestIpAndCallNo.insert(std::make_pair(session->m_destIpAndCallNo, session));
			session->ReportIax2Authreq(authreq);
			session->m_iax2_state = IAX2_STATE_WAITING;

			authreq->ToString(authreqString);
			LOG4CXX_INFO(m_log, "[" + session->m_trackingId + "] "
					"supplemented by IAX2 AUTHREQ:" + authreqString +
					". Status now WAITING");

			return;
		}
	}

	/* We don't have an existing session? */
	authreq->ToString(authreqString);
	log_msg.Format("Got AUTHREQ [%s] but couldn't find corresponding NEW", authreqString);
	LOG4CXX_ERROR(m_log, log_msg);

	return;
}

void Iax2Sessions::ReportIax2New(Iax2NewInfoRef& invite)
{
	char szFromIax2Ip[16];
	std::map<CStdString, Iax2SessionRef>::iterator pair;

	ACE_OS::inet_ntop(AF_INET, (void*)&invite->m_senderIp, szFromIax2Ip, sizeof(szFromIax2Ip));
	CStdString IpAndCallNo = CStdString(szFromIax2Ip) + "," + invite->m_callNo;

	pair = m_bySrcIpAndCallNo.find(IpAndCallNo);
	if (pair != m_bySrcIpAndCallNo.end()) {
		// The session already exists, check the state
		CStdString logmsg;

		Iax2SessionRef session = pair->second;

		if(session.get() != NULL) {
			if(session->m_iax2_state == IAX2_STATE_UP) {
				CStdString log_msg;

				log_msg.Format("[%s] is in the UP state but we've "
					"got another NEW from this same IP %s and same "
					"source call number %s", session->m_trackingId,
					szFromIax2Ip, invite->m_callNo);
	
				LOG4CXX_ERROR(m_log, log_msg);
				return;
			}
		}

		/* Stop this session and proceed */
		Stop(session);
	}

	/* Create a new session */
	CStdString trackingId = m_alphaCounter.GetNext();
	Iax2SessionRef session(new Iax2Session(trackingId));
	session->m_srcIpAndCallNo = IpAndCallNo;
	session->ReportIax2New(invite);
	m_bySrcIpAndCallNo.insert(std::make_pair(session->m_srcIpAndCallNo, session));

	CStdString numSessions = IntToString(m_bySrcIpAndCallNo.size());
	LOG4CXX_DEBUG(m_log, CStdString("BySrcIpAndCallNo: ") + numSessions);

	CStdString inviteString;
	invite->ToString(inviteString);
	LOG4CXX_INFO(m_log, "[" + trackingId + "] created by IAX2 NEW:" + inviteString + " for " + session->m_srcIpAndCallNo);
}

/* This function is called as a result of a) an IAX2 HANGUP; b) a
 * CONTROL HANGUP; or c) an IAX2 REJECT */
void Iax2Sessions::ReportIax2Hangup(Iax2HangupInfoRef& bye)
{
        CStdString senderIpAndCallNo, log_msg, receiverIpAndCallNo, numSessions, hangupString;
        std::map<CStdString, Iax2SessionRef>::iterator pair;
        char sender_ip[16], receiver_ip[16];
        Iax2SessionRef session;

        ACE_OS::inet_ntop(AF_INET, (void*)&bye->m_receiverIp, receiver_ip, sizeof(receiver_ip));
        ACE_OS::inet_ntop(AF_INET, (void*)&bye->m_senderIp, sender_ip, sizeof(sender_ip));
        receiverIpAndCallNo = CStdString(receiver_ip) + "," + bye->m_receiver_callno;
        senderIpAndCallNo = CStdString(sender_ip) + "," + bye->m_sender_callno;

	/* The recipient of the HANGUP is the Invitor */
	pair = m_bySrcIpAndCallNo.find(receiverIpAndCallNo);
	if (pair != m_bySrcIpAndCallNo.end()) {
		Iax2SessionRef session = pair->second;
		if(session.get() != NULL) {
			log_msg.Format("[%s] %s: Hanging up session", session->m_trackingId, session->m_srcIpAndCallNo);
			LOG4CXX_INFO(m_log, log_msg);
			Stop(session);
		}
	}

	/* The sender of the HANGUP is the Invitor */
	pair = m_bySrcIpAndCallNo.find(senderIpAndCallNo);
        if (pair != m_bySrcIpAndCallNo.end()) {
                Iax2SessionRef session = pair->second;
		if(session.get() != NULL) {
			log_msg.Format("[%s] %s: Hanging up session", session->m_trackingId, session->m_srcIpAndCallNo);
        	        LOG4CXX_INFO(m_log, log_msg);
                	Stop(session);
		}
        }

	/* The recipient of the HANGUP is the Invitee */
	pair = m_byDestIpAndCallNo.find(receiverIpAndCallNo);
        if (pair != m_byDestIpAndCallNo.end()) {
                Iax2SessionRef session = pair->second;
		if(session.get() != NULL) {
			log_msg.Format("[%s] %s: Hanging up session", session->m_trackingId, session->m_srcIpAndCallNo);
        	        LOG4CXX_INFO(m_log, log_msg);
                	Stop(session);
		}
        }

	/* The sender of the HANGUP is the Invitee */
        pair = m_byDestIpAndCallNo.find(senderIpAndCallNo);
        if (pair != m_byDestIpAndCallNo.end()) {
                Iax2SessionRef session = pair->second;
		if(session.get() != NULL) {
			log_msg.Format("[%s] %s: Hanging up session", session->m_trackingId, session->m_srcIpAndCallNo);
        	        LOG4CXX_INFO(m_log, log_msg);
                	Stop(session);
		}
        }
}

void Iax2Sessions::Stop(Iax2SessionRef& session)
{
	CStdString numSessions;

	session->Stop();

	if(session->m_srcIpAndCallNo.size() > 0) {
		m_bySrcIpAndCallNo.erase(session->m_srcIpAndCallNo);

		numSessions = IntToString(m_bySrcIpAndCallNo.size());
		LOG4CXX_DEBUG(m_log, CStdString("BySrcIpAndPort: ") + numSessions);
	}

	if(session->m_destIpAndCallNo.size() > 0) {
		m_byDestIpAndCallNo.erase(session->m_destIpAndCallNo);

		numSessions = IntToString(m_byDestIpAndCallNo.size());
                LOG4CXX_DEBUG(m_log, CStdString("ByDestIpAndPort: ") + numSessions);
	}
}

/* Returns false if there is no matching session */
bool Iax2Sessions::ReportIax2Packet(Iax2PacketInfoRef& iax2Packet)
{
	Iax2SessionRef session;
	CStdString logMsg, sourcecallno, IpAndCallNo;
	std::map<CStdString, Iax2SessionRef>::iterator pair;
	char szSourceIp[16];

	/* Add this IAX2 voice frame to a session which matches either the source
	 * IP address and call number or the destination IP address and call number.
	 * In this context "source IP" refers to the IP address of the machine which
	 * initiated the session i.e sent the "NEW".  Likewise "destination IP"
	 * refers to the machine to whom the "NEW" was sent.  There is no chance
	 * that there could be a duplicate */

	ACE_OS::inet_ntop(AF_INET, (void*)&iax2Packet->m_sourceIp, szSourceIp,
			  sizeof(szSourceIp));
	sourcecallno = IntToString(iax2Packet->m_sourcecallno);

	IpAndCallNo = CStdString(szSourceIp) + "," + sourcecallno;

	pair = m_bySrcIpAndCallNo.find(IpAndCallNo);
	if(pair != m_bySrcIpAndCallNo.end()) {
		session = pair->second;

		if(session.get() != NULL) {
			if(!session->AddIax2Packet(iax2Packet)) {
				/* Discontinuity detected? */
				Stop(session);
			} else {
				return true;
			}
		}

		return false;
	}

	/* Search in the destination IP map */
	pair = m_byDestIpAndCallNo.find(IpAndCallNo);
	if(pair != m_byDestIpAndCallNo.end()) {
		session = pair->second;

                if(session.get() != NULL) {
                        if(!session->AddIax2Packet(iax2Packet)) {
                                /* Discontinuity detected? */
                                Stop(session);
                        } else {
				return true;
			}
                }

                return false;
        }

	/* XXX Tracking?? */
	CStdString pktinfo;
	iax2Packet->ToString(pktinfo);
	//LOG4CXX_INFO(m_log, "Could not figure out where to place packet from "+IpAndCallNo+": [" + pktinfo +"]");

	return false;
}

void Iax2Sessions::StopAll()
{
	time_t forceExpiryTime = time(NULL) + 2*DLLCONFIG.m_rtpSessionWithSignallingTimeoutSec;
	Hoover(forceExpiryTime);
}

void Iax2Sessions::Hoover(time_t now)
{
	CStdString numSessions;
	int timeoutSeconds = 0;
	Iax2SessionRef session;
	std::map<CStdString, Iax2SessionRef>::iterator pair;
	std::list<Iax2SessionRef> toDismiss;
	std::list<Iax2SessionRef>::iterator it;

	numSessions = IntToString(m_bySrcIpAndCallNo.size());
	LOG4CXX_DEBUG(m_log, "Hoover - check " + numSessions + " sessions time:" + IntToString(now));
	timeoutSeconds = DLLCONFIG.m_rtpSessionWithSignallingTimeoutSec;

	for(pair=m_bySrcIpAndCallNo.begin(); pair!=m_bySrcIpAndCallNo.end(); pair++) {
		session = pair->second;

		if((now - session->m_lastUpdated) > timeoutSeconds)
			toDismiss.push_back(session);
	}

	for (it=toDismiss.begin(); it!=toDismiss.end(); it++) {
		session = *it;
		LOG4CXX_INFO(m_log,  "[" + session->m_trackingId + "] " + session->m_srcIpAndCallNo + " Expired");
		Stop(session);
	}

	/* Just in case? */
	toDismiss.clear();
	for(pair=m_byDestIpAndCallNo.begin(); pair!=m_byDestIpAndCallNo.end(); pair++) {
		session = pair->second;

		if((now - session->m_lastUpdated) > timeoutSeconds)
			toDismiss.push_back(session);
	}

	for (it=toDismiss.begin(); it!=toDismiss.end(); it++) {
                session = *it;
                LOG4CXX_INFO(m_log,  "[" + session->m_trackingId + "] " + session->m_destIpAndCallNo + " Expired");
                Stop(session);
        }
}

//==========================================================
Iax2NewInfo::Iax2NewInfo()
{
	m_senderIp.s_addr = 0;
	m_receiverIp.s_addr = 0;
	m_caller = "HIDDEN"; /* Unless obtained */
	m_validated= false;
}

void Iax2NewInfo::ToString(CStdString& string)
{
	char senderIp[16], receiverIp[16];

	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp,
			sizeof(receiverIp));

	string.Format("sender:%s receiver: %s caller:%s callee:%s srccallno: %s callername:%s localextension:%s",
			senderIp, receiverIp, m_caller, m_callee, m_callNo, m_callingName, m_localExtension);
}

//==========================================================
Iax2AuthreqInfo::Iax2AuthreqInfo()
{
	m_senderIp.s_addr = 0;
        m_receiverIp.s_addr = 0;
}

void Iax2AuthreqInfo::ToString(CStdString& string)
{
        char senderIp[16], receiverIp[16];

        ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
        ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp,
                        sizeof(receiverIp));

	string.Format("sender:%s receiver:%s sender_callno:%s receiver_callno:%s",
			senderIp, receiverIp, m_sender_callno, m_receiver_callno);
}

//==========================================================
Iax2AcceptInfo::Iax2AcceptInfo()
{
        m_senderIp.s_addr = 0;
        m_receiverIp.s_addr = 0;
}

void Iax2AcceptInfo::ToString(CStdString& string)
{
        char senderIp[16], receiverIp[16];

        ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
        ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp,
                        sizeof(receiverIp));

        string.Format("sender:%s receiver:%s sender_callno:%s receiver_callno:%s",
                        senderIp, receiverIp, m_sender_callno, m_receiver_callno);
}

//==========================================================
Iax2HangupInfo::Iax2HangupInfo()
{
        m_senderIp.s_addr = 0;
        m_receiverIp.s_addr = 0;
}

void Iax2HangupInfo::ToString(CStdString& string)
{
        char senderIp[16], receiverIp[16];

        ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
        ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp,
                        sizeof(receiverIp));

        string.Format("sender:%s receiver:%s sender_callno:%s receiver_callno:%s",
                        senderIp, receiverIp, m_sender_callno, m_receiver_callno);
}

//==========================================================
Iax2PacketInfo::Iax2PacketInfo()
{
}

void Iax2PacketInfo::ToString(CStdString& string)
{
	char senderIp[16], receiverIp[16];

        ACE_OS::inet_ntop(AF_INET, (void*)&m_sourceIp, senderIp, sizeof(senderIp));
        ACE_OS::inet_ntop(AF_INET, (void*)&m_destIp, receiverIp,
                        sizeof(receiverIp));

	string.Format("sender:%s receiver:%s sender_callno:%d receiver_callno:%d "
			"type:%d size:%d timestamp:%u", senderIp, receiverIp,
			m_sourcecallno, m_destcallno, m_payloadType,
			m_payloadSize, m_timestamp);
}

