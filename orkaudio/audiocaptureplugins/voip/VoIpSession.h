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

#ifndef __VOIPSESSION_H__
#define __VOIPSESSION_H__

#include <log4cxx/logger.h>
#include "Rtp.h"
#include <map>
#include "PacketHeaderDefs.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include "boost/multi_index/indexed_by.hpp"
#include "boost/multi_index/sequenced_index.hpp"
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "SipHeaders.h"
#include "Utils.h"
#include "../common/OrkSession.h"


#ifdef TESTING
	#define VIT virtual
#else
	#define VIT 
#endif

using namespace log4cxx;

class UrlExtractionValue
{
public:
	UrlExtractionValue();
	UrlExtractionValue(CStdString value);
	CStdString m_value;
	time_t m_timestamp;
};
typedef oreka::shared_ptr<UrlExtractionValue> UrlExtractionValueRef;

//=============================================================

class VoIpEndpointInfo
{
public:
	VoIpEndpointInfo();
	CStdString m_extension;
	CStdString m_latestCallId;
	struct in_addr m_ip;
	unsigned short m_skinnyPort;
	time_t m_lastConferencePressed;
	time_t m_lastConnectedWithConference;
	CStdString m_origOrkUid;
	std::map<CStdString, UrlExtractionValueRef> m_urlExtractionMap;		//map<urlParam, urlValue>
	ConstSipNotifyInfoRef m_preInviteNotifyRef;
};
typedef oreka::shared_ptr<VoIpEndpointInfo> VoIpEndpointInfoRef;

// ============================================================

class VoIpSession : public OrkSession
{
public:
#define PROT_RAW_RTP "RawRtp"
#define PROT_SIP "Sip"
#define PROT_SKINNY "Skinny"
#define PROT_UNKN "Unkn"
	typedef enum{ProtRawRtp, ProtSip, ProtSkinny, ProtUnkn} ProtocolEnum;
	static int ProtocolToEnum(CStdString& protocol);
	static CStdString ProtocolToString(int protocolEnum);

	VoIpSession(CStdString& trackingId);
	void Stop();
	virtual void Start();
	virtual void ReportMetadata();
	bool AddRtpPacket(RtpPacketInfoRef& rtpPacket);
	void ReportSipNotify(SipNotifyInfoRef& notify);
	void ReportSipBye(SipByeInfoRef& bye);
	void ReportSipInvite(SipInviteInfoRef& invite);
	void ReportSipErrorPacket(SipFailureMessageInfoRef& info);
	void ReportSipInfo(SipInfoRef& info);
	void ReportSipRefer(SipReferRef& info);
	void ReportRtcpSrcDescription(RtcpSrcDescriptionPacketInfoRef& rtcpInfo);
	bool OrkUidMatches(CStdString &oUid);
	bool PartyMatches(CStdString &party);
	bool NativeCallIdMatches(CStdString &callid);
	void UpdateMetadataSkinny();
	void ReportSkinnyCallInfo(SkCallInfoStruct*, IpHeaderStruct* ipHeader);
	void ReportSkinnyCallStateMessage(SkCallStateMessageStruct*, IpHeaderStruct* ipHeader);
	void GoOnHold(time_t onHoldTime);
	void GoOffHold(time_t offHoldTime);
	CStdString GetOrkUid();
	void MarkAsOnDemand(CStdString& side);
	void MarkAsOnDemandOff();
	bool Stopped();
	RtpPacketInfoRef GetLastRtpPacket();
	void SkinnyTrackConferencesTransfers(CStdString callId, CStdString capturePort);
	bool IsMatchedLocalOrRemoteIp(struct in_addr ip);

	unsigned long long m_ipAndPort;	// IP address and UDP port of one side of the RTP session, serves as a key for session storage and retrieval. Not necessarily the same as the capturePort (capturePort is usually the client (phone) IP+port)
	struct in_addr m_rtpIp;	// IP address of one side of the RTP session
	CStdString m_callId;
	SipInviteInfoRef m_invite;
	OrkTimeValue m_creationDate;		// When the session is first created
	time_t m_lastUpdated;
	time_t m_lastKeepAlive;
	ProtocolEnum m_protocol;
	CStdString m_remotePartyNecSip;
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
	CaptureEvent::LocalSideEnum m_localSide;

	int m_numRtpPackets;
	int m_numIgnoredRtpPackets;
	bool m_metadataProcessed;
	unsigned int m_highestRtpSeqNumDelta;
	double m_minRtpSeqDelta;
	double m_minRtpTimestampDelta;
	unsigned int m_rtpNumMissingPkts;
	unsigned int m_rtpNumSeqGaps;

	struct in_addr m_endPointIp;		// only used for Skinny
	unsigned short m_endPointSignallingPort;	// so far only used for Skinny
	int m_skinnyPassThruPartyId;
	OrkTimeValue m_sipLastInvite;
	OrkTimeValue m_skinnyLastCallInfoTime;
	int m_skinnyLineInstance;
	bool m_onHold;
	std::list<unsigned long long> m_mediaAddresses;
	time_t m_lastRtpStreamStart;
	int m_holdDuration;
	int m_holdBegin;
	CStdString m_sipDialedNumber;
	CStdString m_sipRemoteParty;
	bool m_isCallPickUp;
	unsigned int m_numAlienRtpPacketsS1;
	unsigned int m_numAlienRtpPacketsS2;
	unsigned int m_ssrcCandidate;
	void ReportMetadataUpdateSkinny();
	bool m_hasReceivedCallInfo;

private:
	void ProcessMetadataSip(RtpPacketInfoRef&);
	void ProcessMetadataSipIncoming();
	void ProcessMetadataSipOutgoing();
	void UpdateMetadataSipOnRtpChange(RtpPacketInfoRef& rtpPacket, bool);
	void ProcessMetadataRawRtp(RtpPacketInfoRef&);
	void ProcessMetadataSkinny(RtpPacketInfoRef& rtpPacket);
	void GenerateOrkUid();
	bool MatchesSipDomain(CStdString& domain);
	bool MatchesReferenceAddresses(struct in_addr inAddr);
	bool IsInSkinnyReportingList(CStdString item);
	virtual void TriggerOnDemandViaDtmf();
	void LogRtpPayloadMap();

	RtpPacketInfoRef m_lastRtpPacket;
	//RtpRingBuffer m_rtpRingBuffer;
	struct in_addr m_remoteIp;
	char m_localMac[6];
	char m_remoteMac[6];
	//struct in_addr m_localMac;
	//struct in_addr m_remoteMac;
	LoggerPtr m_log;
	CStdString m_orkUid;

	bool m_hasDuplicateRtp;

	TcpAddressList m_rtpAddressList;
	std::list<SipInviteInfoRef> m_invites;
	std::map<CStdString, CStdString> m_tags;

	unsigned int m_ssrcCandidateTimestamp;
	std::map<unsigned int, int> m_loggedSsrcMap;
	SipInfoRef m_lastSipInfo;
};
typedef oreka::shared_ptr<VoIpSession> VoIpSessionRef;

//===================================================================
class VoIpSessions: public OrkSingleton<VoIpSessions>
{
public:
	VoIpSessions();
	void Stop(VoIpSessionRef& session);
	void StopAll();
	VIT void ReportSipInvite(SipInviteInfoRef& invite);
	VIT void ReportSipNotify(SipNotifyInfoRef& notify);
	VIT void ReportSipBye(SipByeInfoRef& bye);
	void ReportSipSubscribe(SipSubscribeInfoRef& subscribe);
	void ReportSkinnyCallInfo(SkCallInfoStruct*, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void ReportSkinnyCallStateMessage(SkCallStateMessageStruct*, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void ReportSkinnyStartMediaTransmission(SkStartMediaTransmissionStruct*, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void ReportSkinnyStopMediaTransmission(SkStopMediaTransmissionStruct*, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void ReportSkinnyOpenReceiveChannelAck(SkOpenReceiveChannelAckStruct* openReceive, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void SetEndpointExtension(CStdString& extension, struct in_addr* endpointIp, CStdString& callId, unsigned short skinnyPort);
	void ReportSkinnyLineStat(SkLineStatStruct*, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void ReportSkinnySoftKeyHold(SkSoftKeyEventMessageStruct* skEvent, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void ReportSkinnySoftKeyResume(SkSoftKeyEventMessageStruct* skEvent, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void ReportSkinnySoftKeyConfPressed(struct in_addr endpointIp, TcpHeaderStruct* tcpHeader);
	void ReportSkinnySoftKeySetConfConnected(struct in_addr endpointIp, TcpHeaderStruct* tcpHeader);
	void ReportSkinnySoftKeySetTransfConnected(SkSoftKeySetDescriptionStruct* skEvent, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void ReportRtpPacket(RtpPacketInfoRef& rtpPacket);
	bool ReportRtcpSrcDescription(RtcpSrcDescriptionPacketInfoRef& rtcpInfo);
	void ReportSipErrorPacket(SipFailureMessageInfoRef& sipError);
	VIT void ReportSip200Ok(Sip200OkInfoRef info);
	void ReportSipSessionProgress(SipSessionProgressInfoRef& info);
	void ReportSip302MovedTemporarily(Sip302MovedTemporarilyInfoRef& info);
	void ReportSipInfo(SipInfoRef& info);
	void ReportSipRefer(SipReferRef& info);
	void Hoover(time_t now);
	VoIpEndpointInfoRef GetVoIpEndpointInfoByIp(struct in_addr *ip);
	VoIpEndpointInfoRef GetVoIpEndpointInfo(struct in_addr endpointIp, unsigned short skinnyPort);
	CStdString StartCapture(CStdString& party, CStdString& side);
	void StartCaptureOrkuid(CStdString& orkuid, CStdString& side);
	CStdString StartCaptureNativeCallId(CStdString& nativecallid, CStdString& side);
	CStdString PauseCaptureNativeCallId(CStdString& nativecallid);
	CStdString PauseCapture(CStdString& party);
	void PauseCaptureOrkuid(CStdString& orkuid);
	CStdString StopCapture(CStdString& party, CStdString& qos);
	void StopCaptureOrkuid(CStdString& orkuid, CStdString& qos);
	CStdString StopCaptureNativeCallId(CStdString& nativecallid, CStdString& qos);
	void SaveLocalPartyMap(CStdString& oldparty, CStdString& newparty);
	void SaveSkinnyGlobalNumbersList(CStdString& number);
	CStdString GetLocalPartyMap(CStdString& oldlocalparty);
	typedef enum {UrlStartState, UrlKeyState, UrlValueState, UrlErrorState} UrlState;
	void UnEscapeUrl(CStdString& in, CStdString& out);
	void UrlExtraction(CStdString& input, struct in_addr* endpointIp);
	void ReportOnDemandMarkerByIp(struct in_addr endpointIp);
	void TaggingSipTransferCalls(VoIpSessionRef& session);
	void CopyMetadataToNewSession(VoIpSessionRef& oldSession, VoIpSessionRef& newSession);
	void ClearLocalPartyMap();
	const std::map<unsigned long long, VoIpSessionRef> & getByIpAndPort() const;

private:
	void CraftMediaAddress(CStdString& mediaAddress, struct in_addr ipAddress, unsigned short udpPort);
	void Craft64bitMediaAddress(unsigned long long& mediaAddress, struct in_addr ipAddress, unsigned short udpPort);
	CStdString MediaAddressToString(unsigned long long ipAndPort);
	VoIpSessionRef findByMediaAddress(struct in_addr ipAddress, unsigned short udpPort);
	VoIpSessionRef findByEndpointIp(struct in_addr endpointIpAddr, int passThruPartyId = 0);
	VoIpSessionRef SipfindNewestBySenderIp(struct in_addr receiverIpAddr);
	VoIpSessionRef findNewestByEndpoint(struct in_addr endpointIpAddr, unsigned short endpointSignallingPort);
	VoIpSessionRef findByEndpointIpUsingIpAndPort(struct in_addr endpointIpAddr);
	VoIpSessionRef findByEndpointIpAndLineInstance(struct in_addr endpointIpAddr, int lineInstance);
	VoIpSessionRef findNewestRtpByEndpointIp(struct in_addr endpointIpAddr);
	bool ChangeCallId(VoIpSessionRef& session, unsigned int newId);
	void SetMediaAddress(VoIpSessionRef& session, struct in_addr mediaIp, unsigned short mediaPort);
	void RemoveFromMediaAddressMap(VoIpSessionRef& session, unsigned long long& mediaAddress);
	CStdString GenerateSkinnyCallId(struct in_addr endpointIp, unsigned short endpointSkinnyPort, unsigned int callId);
	void UpdateEndpointWithCallInfo(SkCallInfoStruct* callInfo, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);
	void UpdateSessionWithCallInfo(SkCallInfoStruct*, VoIpSessionRef&);
	bool SkinnyFindMostLikelySessionForRtp(RtpPacketInfoRef& rtpPacket, VoIpEndpointInfoRef&);
	bool SkinnyFindMostLikelySessionForRtpBehindNat(RtpPacketInfoRef& rtpPacket);
	void TrySessionCallPickUp(CStdString replacesCallId, bool& result);

	std::map<unsigned long long, VoIpSessionRef> m_byIpAndPort;
	std::map<CStdString, VoIpSessionRef> m_byCallId;
	std::map<unsigned long long, VoIpEndpointInfoRef> m_endpoints;
	std::map<CStdString, CStdString> m_localPartyMap;
	std::map<CStdString, int> m_skinnyGlobalNumbersList;
	SipSubscribeMap m_sipSubscribeMap;
	std::list<SipReferRef> m_sipReferList;
	LoggerPtr m_log;
	AlphaCounter m_alphaCounter;
};

#ifdef TESTING
	#define TestVoIpSessionsSingleton VoIpSessions
	class VoIpSessionsSingleton {
		static VoIpSessions* voipSessions;
		public:
		static void set (VoIpSessions* vs) {voipSessions=vs;};
		static VoIpSessions* instance() {
			return voipSessions?voipSessions:TestVoIpSessionsSingleton::instance();
		}
	};
#else
	#define VoIpSessionsSingleton VoIpSessions
#endif

#endif

