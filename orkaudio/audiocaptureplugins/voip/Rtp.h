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
#include "shared_ptr.h"
#include "StdString.h"


// useful info we extract from an RTP packet
class RtpPacketInfo
{
public:
	void ToString(CStdString& string);
	RtpPacketInfo();

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
	unsigned int m_ssrc;
	unsigned int m_timestamp;
	time_t m_arrivalTimestamp;
};
typedef oreka::shared_ptr<RtpPacketInfo> RtpPacketInfoRef;

class RtpEventInfo
{
public:
	void ToString(CStdString& string);

	unsigned short m_event;
	unsigned short m_end;
	unsigned short m_reserved;
	unsigned short m_volume;
	unsigned short m_duration;
	unsigned int m_startTimestamp;
};
typedef oreka::shared_ptr<RtpEventInfo> RtpEventInfoRef;

class RtcpSrcDescriptionPacketInfo
{
public:
	void ToString(CStdString& string);

	struct in_addr m_sourceIp;
	struct in_addr m_destIp;
	unsigned short m_sourcePort;
	unsigned short m_destPort;

	CStdString m_cnameUsername;
	CStdString m_cnameDomain;
	CStdString m_cnamePort;
	CStdString m_fullCname;
};
typedef oreka::shared_ptr<RtcpSrcDescriptionPacketInfo> RtcpSrcDescriptionPacketInfoRef;

#endif

