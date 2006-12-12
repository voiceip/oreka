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

#include <log4cxx/logger.h>
#include "RtpSession.h"
#include "Rtp.h"
#include <map>
#include "ace/Singleton.h"
#include "PacketHeaderDefs.h"

using namespace log4cxx;

class SipInviteInfo
{
public:
	SipInviteInfo();
	void ToString(CStdString& string);

	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
	struct in_addr m_fromRtpIp;
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

//=============================================================

class EndpointInfo
{
public:
	CStdString m_extension;
};
typedef boost::shared_ptr<EndpointInfo> EndpointInfoRef;


// ============================================================

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
	bool AddRtpPacket(RtpPacketInfoRef& rtpPacket);
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
	struct in_addr m_endPointIp;		// only used for Skinny

private:
	void ProcessMetadataSip(RtpPacketInfoRef&);
	void ProcessMetadataSipIncoming();
	void ProcessMetadataSipOutgoing();
	void ProcessMetadataRawRtp(RtpPacketInfoRef&);
	void ProcessMetadataSkinny(RtpPacketInfoRef& rtpPacket);
	void ReportMetadata();
	void GenerateOrkUid();

	RtpPacketInfoRef m_lastRtpPacket;
	RtpPacketInfoRef m_lastRtpPacketSide1;
	RtpPacketInfoRef m_lastRtpPacketSide2;
	//RtpRingBuffer m_rtpRingBuffer;
	struct in_addr m_invitorIp;
	int m_invitorTcpPort;
	struct in_addr m_inviteeIp;
	int m_inviteeTcpPort;
	struct in_addr m_localIp;
	struct in_addr m_remoteIp;
	//struct in_addr m_localMac;
	//struct in_addr m_remoteMac;
	LoggerPtr m_log;
	CStdString m_capturePort;
	bool m_started;
	bool m_stopped;
	time_t m_beginDate;
	CStdString m_orkUid;
	bool m_hasDuplicateRtp;
	unsigned int m_highestRtpSeqNumDelta;
	double m_minRtpSeqDelta;
	double m_minRtpTimestampDelta;
};
typedef boost::shared_ptr<RtpSession> RtpSessionRef;

//===================================================================
class RtpSessions
{
public:
	RtpSessions();
	void Stop(RtpSessionRef& session);
	void StopAll();
	void ReportSipInvite(SipInviteInfoRef& invite);
	void ReportSipBye(SipByeInfo bye);
	void ReportSkinnyCallInfo(SkCallInfoStruct*, IpHeaderStruct* ipHeader);
	void ReportSkinnyStartMediaTransmission(SkStartMediaTransmissionStruct*, IpHeaderStruct* ipHeader);
	void ReportSkinnyStopMediaTransmission(SkStopMediaTransmissionStruct*, IpHeaderStruct* ipHeader);
	void ReportSkinnyOpenReceiveChannelAck(SkOpenReceiveChannelAckStruct*);
	void ReportSkinnyLineStat(SkLineStatStruct*, IpHeaderStruct* ipHeader);
	void ReportRtpPacket(RtpPacketInfoRef& rtpPacket);
	void Hoover(time_t now);
	EndpointInfoRef GetEndpointInfo(struct in_addr endpointIp);
private:
	RtpSessionRef findByEndpointIp(struct in_addr);
	void ChangeCallId(RtpSessionRef& session, unsigned int newId);
	void SetMediaAddress(RtpSessionRef& session, struct in_addr mediaIp, unsigned short mediaPort);
	CStdString GenerateSkinnyCallId(struct in_addr endpointIp, unsigned int callId);

	std::map<CStdString, RtpSessionRef> m_byIpAndPort;
	std::map<CStdString, RtpSessionRef> m_byCallId;
	std::map<unsigned int, EndpointInfoRef> m_endpoints;
	LoggerPtr m_log;
	AlphaCounter alphaCounter;
};
typedef ACE_Singleton<RtpSessions, ACE_Thread_Mutex> RtpSessionsSingleton;

#endif

