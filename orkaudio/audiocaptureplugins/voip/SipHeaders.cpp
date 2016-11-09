/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */

#include "SipHeaders.h"
#include "ace/OS_NS_arpa_inet.h"
#include "Utils.h"
#include "MemUtils.h"

SipSubscribeInfo::SipSubscribeInfo()
{

}
//==========================================================
SipInviteInfo::SipInviteInfo()
{
	m_fromRtpIp.s_addr = 0;
	m_validated = false;
	m_attrSendonly = false;
	m_telephoneEventPtDefined = false;
	m_SipGroupPickUpPatternDetected = false;
	m_orekaRtpPayloadType = 0;
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

	string.Format("sender:%s from:%s@%s RTP:%s,%s to:%s@%s rcvr:%s callid:%s smac:%s rmac:%s fromname:%s toname:%s ua:%s requesturi:%s a:sendonly:%s telephone-event-payload-type:%s", senderIp, m_from, m_fromDomain, fromRtpIp, m_fromRtpPort, m_to, m_toDomain, receiverIp, m_callId, senderMac, receiverMac, m_fromName, m_toName, m_userAgent, m_requestUri, ((m_attrSendonly == true) ? "present" : "not-present"), m_telephoneEventPayloadType);
}

//==========================================================
Sip302MovedTemporarilyInfo::Sip302MovedTemporarilyInfo()
{
	m_senderIp.s_addr = 0;
	m_receiverIp.s_addr = 0;
}

void Sip302MovedTemporarilyInfo::ToString(CStdString& string)
{
	char senderIp[16];
	char receiverIp[16];

	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	string.Format("sender:%s rcvr:%s from:%s@%s to:%s@%s contact:%s@%s fromname:%s toname:%s contactname:%s callid:%s", senderIp, receiverIp, m_from, m_fromDomain, m_to, m_toDomain, m_contact, m_contactDomain, m_fromName, m_toName, m_contactName, m_callId);
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

	string.Format("sender:%s rcvr:%s callid:%s from:%s to:%s fromDomain:%s toDomain:%s fromName:%s toName:%s", senderIp, receiverIp, m_callId, m_from, m_to, m_fromDomain, m_toDomain, m_fromName, m_toName);
}

//================================================
SipNotifyInfo::SipNotifyInfo()
{
	m_senderIp.s_addr = 0;
	m_receiverIp.s_addr = 0;
}
//================================================
SipInfo::SipInfo()
{
	m_onDemand = false;
	m_onDemandOff = false;
}
//==================================================
SipRefer::SipRefer()
{
	m_senderIp.s_addr = 0;
	m_receiverIp.s_addr = 0;
}

void SipRefer::ToString(CStdString& string)
{
	char senderIp[16];
	char receiverIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	string.Format("sender:%s rcvr:%s from:%s@%s to:%s@%s fromname:%s toname:%s referto:%s referredby:%s referredParty:%s callid:%s", senderIp, receiverIp, m_from, m_fromDomain, m_to, m_toDomain, m_fromName, m_toName, m_referToParty, m_referredByParty, m_referredParty, m_callId);
}



