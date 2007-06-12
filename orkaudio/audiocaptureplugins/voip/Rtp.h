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

#ifndef __RTP_H__
#define __RTP_H__

#include "ace/OS_NS_arpa_inet.h"
#include "boost/shared_ptr.hpp"
#include "StdString.h"


// useful info we extract from an RTP packet
class RtpPacketInfo
{
public:
	void ToString(CStdString& string);

	//CStdString m_sourceMac;
	//CStdString m_destMac;
	char m_sourceMac[6];
	char m_destMac[6];

	struct in_addr m_sourceIp;
	struct in_addr m_destIp;
	unsigned short m_sourcePort;
	unsigned short m_destPort;
	unsigned int m_payloadSize;
	unsigned short m_payloadType;
	unsigned char* m_payload;
	unsigned short m_seqNum;
	unsigned int m_timestamp;
	time_t m_arrivalTimestamp;
};
typedef boost::shared_ptr<RtpPacketInfo> RtpPacketInfoRef;


#endif

