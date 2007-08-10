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
	m_arrivalTimestamp = 0;
}

void RtpPacketInfo::ToString(CStdString& string)
{
	char sourceIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_sourceIp, sourceIp, sizeof(sourceIp));
	char destIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_destIp, destIp, sizeof(destIp));
	string.Format("%s,%d %s,%d seq:%u ts:%u len:%d type:%d", sourceIp, m_sourcePort, destIp, m_destPort, m_seqNum, m_timestamp, m_payloadSize, m_payloadType);
}

