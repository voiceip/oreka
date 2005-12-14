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

#ifndef __RTPSESSION_H__
#define __RTPSESSION_H__

#include "RtpSession.h"
#include "Rtp.h"
#include <map>
#include "ace/Singleton.h"
#include "PacketHeaderDefs.h"

class SipInviteInfo
{
public:
	void ToString(CStdString& string);

	struct in_addr m_fromIp;
	CStdString m_fromRtpPort;
	CStdString m_from;
	CStdString m_to;
	CStdString m_callId;
};
typedef boost::shared_ptr<SipInviteInfo> SipInviteInfoRef;

class SipByeInfo
{
public:
	CStdString m_callId;
};


class SccpCallInfoMessageInfo
{
	CStdString m_callingParty;
	CStdString m_calledParty;
	CStdString m_callId;
};
typedef boost::shared_ptr<SccpCallInfoMessageInfo> SccpCallInfoMessageInfoRef;


class SccpStartMediaTransmissionInfo
{
	struct in_addr m_remoteIp;
	int m_remoteTcpPort;
	CStdString m_callId;
};
typedef boost::shared_ptr<SccpStartMediaTransmissionInfo> SccpStartMediaTransmissionInfoRef;


class SccpStopMediaTransmissionInfo
{
	CStdString m_callId;
};
typedef boost::shared_ptr<SccpStopMediaTransmissionInfo> SccpStopMediaTransmissionInfoRef;


class RtpSession
{
public:
#define PROT_RAW_RTP "RawRtp"
#define PROT_SIP "Sip"
#define PROT_SKINNY "Skinny"
#define PROT_UNKN "Unkn"
	typedef enum{ProtRawRtp, ProtSip, ProtSkinny, ProtUnkn} ProtocolEnum;
	static int ProtocolToEnum(CStdString& protocol);
	static CStdString ProtocolToString(int protocolEnum);

	RtpSession(CStdString& trackingId);
	void Stop();
	void Start();
	void AddRtpPacket(RtpPacketInfoRef& rtpPacket);
	void ReportSipInvite(SipInviteInfoRef& invite);

	CStdString m_trackingId;
	CStdString m_ipAndPort;	// IP address and TCP port of one side of the session, serves as a key for session storage and retrieval. Not necessarily the same as the capturePort (capturePort is usually the client (phone) IP+port)
	CStdString m_callId;
	SipInviteInfoRef m_invite;
	time_t m_lastUpdated;
	ProtocolEnum m_protocol;
	CStdString m_localParty;
	CStdString m_remoteParty;
	CaptureEvent::DirectionEnum m_direction;
	int m_numRtpPackets;

private:
	void ProcessMetadataSip(RtpPacketInfoRef&);
	void ProcessMetadataSipIncoming();
	void ProcessMetadataSipOutgoing();
	void ProcessMetadataRawRtp(RtpPacketInfoRef&);
	void ProcessMetadataSkinny(RtpPacketInfoRef& rtpPacket);
	void ReportMetadata();

	RtpPacketInfoRef m_lastRtpPacket;
	RtpPacketInfoRef m_lastRtpPacketSide1;
	RtpPacketInfoRef m_lastRtpPacketSide2;
	RtpRingBuffer m_rtpRingBuffer;
	struct in_addr m_invitorIp;
	int m_invitorTcpPort;
	struct in_addr m_inviteeIp;
	int m_inviteeTcpPort;
	LoggerPtr m_log;
	CStdString m_capturePort;
	bool m_started;
	int m_rtpTimestampCorrectiveOffset;
};
typedef boost::shared_ptr<RtpSession> RtpSessionRef;

class RtpSessions
{
public:
	RtpSessions();
	void Stop(RtpSessionRef& session);
	void ReportSipInvite(SipInviteInfoRef& invite);
	void ReportSipBye(SipByeInfo bye);
	void ReportSkinnyCallInfo(SkCallInfoStruct*);
	void ReportSkinnyStartMediaTransmission(SkStartMediaTransmissionStruct*);
	void ReportSkinnyStopMediaTransmission(SkStopMediaTransmissionStruct*);
	void ReportRtpPacket(RtpPacketInfoRef& rtpPacket);
	void Hoover(time_t now);
private:
	std::map<CStdString, RtpSessionRef> m_byIpAndPort;
	std::map<CStdString, RtpSessionRef> m_byCallId;
	LoggerPtr m_log;
	AlphaCounter alphaCounter;
};
typedef ACE_Singleton<RtpSessions, ACE_Thread_Mutex> RtpSessionsSingleton;

#endif

