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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "ace/OS_NS_arpa_inet.h"
#include "Rtp.h"

RtpPacketInfo::RtpPacketInfo()
{
	memset(m_sourceMac, 0, sizeof(m_sourceMac));
	memset(m_destMac, 0, sizeof(m_destMac));
	m_sourceIp.s_addr = 0;
	m_destIp.s_addr = 0;
	m_sourcePort = 0;
	m_destPort = 0;
	m_payloadSize = 0;
	m_payloadType = 0;
	m_payload = NULL;
	m_seqNum = 0;
	m_timestamp = 0;
	m_ssrc = 0;
	m_arrivalTimestamp = 0;
}

void RtpPacketInfo::ToString(CStdString& string)
{
	char sourceIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_sourceIp, sourceIp, sizeof(sourceIp));
	char destIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_destIp, destIp, sizeof(destIp));
	string.Format("%s,%d %s,%d ssrc:0x%x seq:%u ts:%u len:%d type:%d ats:%u",
		sourceIp, m_sourcePort, destIp, m_destPort, m_ssrc, m_seqNum,
		m_timestamp, m_payloadSize, m_payloadType, m_arrivalTimestamp);
}

void RtpEventInfo::ToString(CStdString& string)
{
	string.Format("digit:%d duration:%d end:%d timestamp:%u volume:%d reserved:%d", 
		m_event, m_duration, m_end, m_startTimestamp, m_volume, m_reserved);
}

void RtcpSrcDescriptionPacketInfo::ToString(CStdString& string)
{
	char sourceIp[16];
	char destIp[16];

	ACE_OS::inet_ntop(AF_INET, (void*)&m_sourceIp, sourceIp, sizeof(sourceIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&m_destIp, destIp, sizeof(destIp));

	string.Format("%s,%d %s,%d username:%s domain:%s port:%s", sourceIp, m_sourcePort, destIp, m_destPort, m_cnameUsername, m_cnameDomain, m_cnamePort);
}
