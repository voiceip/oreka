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
#include "ace/OS_NS_sys_time.h"
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
	char m_senderMac[6];
	char m_receiverMac[6];
	CStdString m_fromRtpPort;
	CStdString m_from;
	CStdString m_to;
	CStdString m_callId;
	CStdString m_requestUri;
	bool m_validated;		// true when an RTP stream has been seen for the INVITE
	bool m_attrSendonly;		// true if the SDP has a:sendonly
	std::map<CStdString, CStdString> m_extractedFields;
	CStdString m_telephoneEventPayloadType;
	bool m_telephoneEventPtDefined;
	CStdString m_fromDomain;
	CStdString m_toDomain;
	CStdString m_fromName;
	CStdString m_toName;
	CStdString m_userAgent;

	time_t m_recvTime;
};
typedef boost::shared_ptr<SipInviteInfo> SipInviteInfoRef;

class SipFailureMessageInfo
{
public:
	SipFailureMessageInfo();
	virtual void ToString(CStdString& string);
	virtual void ToString(CStdString& string, SipInviteInfoRef inviteInfo);

	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
	char m_senderMac[6];
	char m_receiverMac[6];
	CStdString m_callId;

	CStdString m_errorCode;
	CStdString m_errorString;
};
typedef boost::shared_ptr<SipFailureMessageInfo> SipFailureMessageInfoRef;

class SipByeInfo
{
public:
	SipByeInfo();
	void ToString(CStdString& string);

	CStdString m_callId;
	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
};


class Sip200OkInfo
{
public:
	Sip200OkInfo();
	void ToString(CStdString& string);

	CStdString m_callId;
	bool m_hasSdp;
	struct in_addr m_mediaIp;
	CStdString m_mediaPort;

	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
	CStdString m_from;
	CStdString m_to;
};
typedef boost::shared_ptr<Sip200OkInfo> Sip200OkInfoRef;


class SipSessionProgressInfo
{
public:
	SipSessionProgressInfo();
	void ToString(CStdString& string);

	CStdString m_callId;
	struct in_addr m_mediaIp;
	CStdString m_mediaPort;

	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
	CStdString m_from;
	CStdString m_to;
};
typedef boost::shared_ptr<SipSessionProgressInfo> SipSessionProgressInfoRef;

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
	void ReportSipErrorPacket(SipFailureMessageInfoRef& info);
	void ReportRtcpSrcDescription(RtcpSrcDescriptionPacketInfoRef& rtcpInfo);
	bool OrkUidMatches(CStdString &oUid);
	bool PartyMatches(CStdString &party);
	bool NativeCallIdMatches(CStdString &callid);
	void UpdateMetadataSkinny();
	void ReportSkinnyCallInfo(SkCallInfoStruct*, IpHeaderStruct* ipHeader);
	CStdString GetOrkUid();

	CStdString m_capturePort;
	CStdString m_trackingId;
	CStdString m_ipAndPort;	// IP address and UDP port of one side of the RTP session, serves as a key for session storage and retrieval. Not necessarily the same as the capturePort (capturePort is usually the client (phone) IP+port)
	struct in_addr m_rtpIp;	// IP address of one side of the RTP session
	CStdString m_callId;
	SipInviteInfoRef m_invite;
	ACE_Time_Value m_creationDate;		// When the session is first created
	time_t m_beginDate;			// When the session has seen a few RTP packets
	time_t m_lastUpdated;
	ProtocolEnum m_protocol;
	CStdString m_localParty;
	CStdString m_remoteParty;
	CStdString m_localEntryPoint;
	CStdString m_localPartyName;
	CStdString m_remotePartyName;
	bool m_localPartyReported;
	bool m_remotePartyReported;
	bool m_rtcpLocalParty;
	bool m_rtcpRemoteParty;
	CaptureEvent::DirectionEnum m_direction;
	int m_numRtpPackets;
	struct in_addr m_endPointIp;		// only used for Skinny
	int m_skinnyPassThruPartyId;
	ACE_Time_Value m_skinnyLastCallInfoTime;
	int m_skinnyLineInstance;
	bool m_onHold;
	bool m_keep;
	bool m_nonLookBackSessionStarted;
	std::list<CStdString> m_otherIpAndPortMappings;

private:
	void ProcessMetadataSip(RtpPacketInfoRef&);
	void ProcessMetadataSipIncoming();
	void ProcessMetadataSipOutgoing();
	void UpdateMetadataSip(RtpPacketInfoRef& rtpPacket, bool);
	void ProcessMetadataRawRtp(RtpPacketInfoRef&);
	void ProcessMetadataSkinny(RtpPacketInfoRef& rtpPacket);
	void ReportMetadata();
	void GenerateOrkUid();
	void HandleRtpEvent(RtpPacketInfoRef& rtpPacket);
	void RecordRtpEvent();
	bool MatchesSipDomain(CStdString& domain);
	bool MatchesReferenceAddresses(struct in_addr inAddr);
	bool IsInSkinnyReportingList(CStdString item);

	RtpPacketInfoRef m_lastRtpPacket;
	RtpPacketInfoRef m_lastRtpPacketSide1;
	RtpPacketInfoRef m_lastRtpPacketSide2;
	//RtpRingBuffer m_rtpRingBuffer;
	struct in_addr m_invitorIp;
	int m_invitorTcpPort;
	struct in_addr m_inviteeIp;
	int m_inviteeTcpPort;
	char m_invitorMac[6];
	char m_inviteeMac[6];
	struct in_addr m_localIp;
	struct in_addr m_remoteIp;
	char m_localMac[6];
	char m_remoteMac[6];
	//struct in_addr m_localMac;
	//struct in_addr m_remoteMac;
	LoggerPtr m_log;
	bool m_started;
	bool m_stopped;
	CStdString m_orkUid;
	bool m_hasDuplicateRtp;
	unsigned int m_highestRtpSeqNumDelta;
	double m_minRtpSeqDelta;
	double m_minRtpTimestampDelta;
	TcpAddressList m_rtpAddressList;
	std::list<SipInviteInfoRef> m_invites;
	std::map<CStdString, CStdString> m_tags;
	bool m_sessionTelephoneEventPtDefined;
	CStdString m_telephoneEventPayloadType;

	unsigned short m_currentRtpEvent;
	unsigned int m_currentRtpEventTs;
	unsigned int m_currentDtmfDuration;
	unsigned int m_currentDtmfVolume;
	unsigned int m_currentSeqNo;
	unsigned int m_lastEventEndSeqNo;
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
	void ReportSkinnySoftKeyHold(SkSoftKeyEventMessageStruct* skEvent, IpHeaderStruct* ipHeader);
	void ReportSkinnySoftKeyResume(SkSoftKeyEventMessageStruct* skEvent, IpHeaderStruct* ipHeader);
	void ReportRtpPacket(RtpPacketInfoRef& rtpPacket);
	bool ReportRtcpSrcDescription(RtcpSrcDescriptionPacketInfoRef& rtcpInfo);
	void ReportSipErrorPacket(SipFailureMessageInfoRef& sipError);
	void ReportSip200Ok(Sip200OkInfoRef info);
	void ReportSipSessionProgress(SipSessionProgressInfoRef& info);
	void Hoover(time_t now);
	EndpointInfoRef GetEndpointInfo(struct in_addr endpointIp);
	CStdString StartCapture(CStdString& party);
	void StartCaptureOrkuid(CStdString& orkuid);
	CStdString StartCaptureNativeCallId(CStdString& nativecallid);
	CStdString PauseCaptureNativeCallId(CStdString& nativecallid);
	CStdString PauseCapture(CStdString& party);
	void PauseCaptureOrkuid(CStdString& orkuid);
	CStdString StopCapture(CStdString& party);
	void StopCaptureOrkuid(CStdString& orkuid);
	CStdString StopCaptureNativeCallId(CStdString& nativecallid);
	void SaveLocalPartyMap(CStdString& oldparty, CStdString& newparty);
	CStdString GetLocalPartyMap(CStdString& oldlocalparty);

private:
	void CraftMediaAddress(CStdString& mediaAddress, struct in_addr ipAddress, unsigned short udpPort);
	RtpSessionRef findByMediaAddress(struct in_addr ipAddress, unsigned short udpPort);
	RtpSessionRef findByEndpointIp(struct in_addr endpointIpAddr, int passThruPartyId = 0);
	RtpSessionRef findNewestByEndpointIp(struct in_addr endpointIpAddr);
	RtpSessionRef findByEndpointIpUsingIpAndPort(struct in_addr endpointIpAddr);
	RtpSessionRef findByEndpointIpAndLineInstance(struct in_addr endpointIpAddr, int lineInstance);
	bool ChangeCallId(RtpSessionRef& session, unsigned int newId);
	void SetMediaAddress(RtpSessionRef& session, struct in_addr mediaIp, unsigned short mediaPort);
	void MapOtherMediaAddress(RtpSessionRef& session, CStdString& ipAndPort);
	CStdString GenerateSkinnyCallId(struct in_addr endpointIp, unsigned int callId);
	void UpdateSessionWithCallInfo(SkCallInfoStruct*, RtpSessionRef&);

	std::map<CStdString, RtpSessionRef> m_byIpAndPort;
	std::map<CStdString, RtpSessionRef> m_byCallId;
	std::map<unsigned int, EndpointInfoRef> m_endpoints;
	std::map<CStdString, CStdString> m_localPartyMap;
	LoggerPtr m_log;
	AlphaCounter m_alphaCounter;
};
typedef ACE_Singleton<RtpSessions, ACE_Thread_Mutex> RtpSessionsSingleton;

#endif

