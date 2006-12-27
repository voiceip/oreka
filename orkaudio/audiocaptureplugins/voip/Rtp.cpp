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


void RtpPacketInfo::ToString(CStdString& string)
{
	char sourceIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_sourceIp, sourceIp, sizeof(sourceIp));
	char destIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_destIp, destIp, sizeof(destIp));
	string.Format("%s,%d %s,%d seq:%u ts:%u len:%d type:%d", sourceIp, m_sourcePort, destIp, m_destPort, m_seqNum, m_timestamp, m_payloadSize, m_payloadType);
}

