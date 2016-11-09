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

#ifndef __SIPHEADERS_H__
#define __SIPHEADERS_H__

#include "Rtp.h"
#include <map>
#include "PacketHeaderDefs.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include "boost/multi_index/indexed_by.hpp"
#include "boost/multi_index/sequenced_index.hpp"
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>


class Sip302MovedTemporarilyInfo
{
public:
	Sip302MovedTemporarilyInfo();
	void ToString(CStdString& string);

	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
	CStdString m_from;
	CStdString m_to;
	CStdString m_contact;
	CStdString m_callId;
	CStdString m_fromDomain;
	CStdString m_toDomain;
	CStdString m_contactDomain;
	CStdString m_fromName;
	CStdString m_toName;
	CStdString m_contactName;
};
typedef oreka::shared_ptr<Sip302MovedTemporarilyInfo> Sip302MovedTemporarilyInfoRef;

class SipInviteInfo
{
public:
	SipInviteInfo();
	void ToString(CStdString& string);

	struct in_addr m_senderIp;
	struct in_addr m_originalSenderIp;
	struct in_addr m_receiverIp;
	struct in_addr m_fromRtpIp;
	char m_senderMac[6];
	char m_receiverMac[6];
	CStdString m_fromRtpPort;
	CStdString m_from;
	CStdString m_to;
	CStdString m_callId;
	CStdString m_replacesId;
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
	CStdString m_sipDialedNumber;
	CStdString m_sipRemoteParty;
	CStdString m_contact;
	CStdString m_contactName;
	CStdString m_contactDomain;
	bool m_SipGroupPickUpPatternDetected;
	int m_orekaRtpPayloadType;

	time_t m_recvTime;
};
typedef oreka::shared_ptr<SipInviteInfo> SipInviteInfoRef;

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
typedef oreka::shared_ptr<SipFailureMessageInfo> SipFailureMessageInfoRef;

class SipByeInfo
{
public:
	SipByeInfo();
	void ToString(CStdString& string);

	CStdString m_callId;
	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
	CStdString m_from;
	CStdString m_to;
	CStdString m_fromDomain;
	CStdString m_toDomain;
	CStdString m_fromName;
	CStdString m_toName;
};
typedef oreka::shared_ptr<SipByeInfo> SipByeInfoRef;

class SipNotifyInfo
{
public:
	SipNotifyInfo();
	//void ToString(CStdString& string);

	CStdString m_callId;
	CStdString m_fromRtpPort;
	CStdString m_byIpAndPort;
	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
	CStdString m_dsp;

	CStdString ToString() {
		CStdString retVal;
		retVal.Format("callid:%s display:%s",m_callId,m_dsp);
		return retVal;
	}
};
typedef oreka::shared_ptr<SipNotifyInfo> SipNotifyInfoRef;
typedef oreka::shared_ptr<const SipNotifyInfo> ConstSipNotifyInfoRef;

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
typedef oreka::shared_ptr<Sip200OkInfo> Sip200OkInfoRef;

class SipSubscribeInfo
{
public:
	SipSubscribeInfo();
	CStdString m_callId;
	CStdString m_event;
};
typedef oreka::shared_ptr<SipSubscribeInfo> SipSubscribeInfoRef;

class SipSessionProgressInfo
{
public:
	SipSessionProgressInfo();
	void ToString(CStdString& string);

	CStdString m_callId;
	struct in_addr m_mediaIp;
	CStdString m_mediaPort;

	struct in_addr m_senderIp;
	struct in_addr m_originalSenderIp;
	struct in_addr m_receiverIp;
	CStdString m_from;
	CStdString m_to;
};
typedef oreka::shared_ptr<SipSessionProgressInfo> SipSessionProgressInfoRef;

class SipInfo
{
public:
	SipInfo();
	CStdString m_callId;
	CStdString m_dtmfDigit;
	CStdString m_cSeq;
	bool m_onDemand;
	bool m_onDemandOff;
};
typedef oreka::shared_ptr<SipInfo> SipInfoRef;

//============================================================
typedef boost::multi_index_container
		<
			SipSubscribeInfoRef,
			boost::multi_index::indexed_by
			<
				boost::multi_index::sequenced<>,
				boost::multi_index::ordered_unique<boost::multi_index::member<SipSubscribeInfo, CStdString,&SipSubscribeInfo::m_callId> >
			>
		> SipSubscribeMap;

enum IndexType
{
     IndexSequential, // = zero (the first index is the sequenced index)
     IndexSearchable // = one (the second index is the ordered index on the first member of the pair)
     //would be able to add more type of index indices
};
typedef SipSubscribeMap::nth_index<IndexSequential>::type SipSubscribeSeqIndex;
typedef SipSubscribeMap::nth_index<IndexSearchable>::type SipSubscribeSearchIndex;

class SipRefer
{
public:
	SipRefer();
	void ToString(CStdString& string);
	time_t m_timestamp;
	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
	CStdString m_from;
	CStdString m_to;
	CStdString m_toDomain;
	CStdString m_fromDomain;
	CStdString m_fromName;
	CStdString m_toName;
	CStdString m_callId;
//referredByParty: the party that refers the call to referredToParty
//referredToParty: the party that receives the transfer from referredByParty
//referredParty: the party that was originally in contact with referredByParty (via either inbound or outbound call) and that is transferred by referredByParty to referredToParty.
	CStdString m_referToParty;
	CStdString m_referredByParty;
	CStdString m_referredParty;
	CStdString m_origOrkUid;
};
typedef oreka::shared_ptr<SipRefer> SipReferRef;

#endif
