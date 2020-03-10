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

#ifndef _SIPTCP_H__
#define _SIPTCP_H__ 1

#include "SafeBuffer.h"

bool TrySipTcp(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader);

using namespace log4cxx;

// ============================================================

class SipTcpStream
{
public:
	SipTcpStream();
	~SipTcpStream();
	void ToString(CStdString& string);
	void AddTcpPacket(u_char *pBuffer, int packetLen);
	void AddTcpPacket(TcpHeaderStruct* tcpHeader);

	u_char* GetBufferWithOffset() {
		return m_tcpBuffer->GetBuffer() + m_offset;
	}
	
	u_char* GetBufferEnd() {
		return m_tcpBuffer->GetBuffer() + m_tcpBuffer->Size();
	}

	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;
	int m_senderPort;
	int m_receiverPort;
	unsigned long int m_expectingSeqNo;
	unsigned long int m_lastSeqNo;

	size_t m_offset;
	int m_entryTime;
	SafeBufferRef m_tcpBuffer;
private:
};
typedef oreka::shared_ptr<SipTcpStream> SipTcpStreamRef;

#endif

