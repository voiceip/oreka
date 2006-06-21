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


#ifndef __PACKETHEADERDEFS_H__
#define __PACKETHEADERDEFS_H__

#include "ace/OS_NS_arpa_inet.h"
#include "StdString.h"


// Structure of Ethernet header
typedef struct
{
	unsigned char sourceMac[6];
	unsigned char destinationMac[6];
	unsigned short length;

} EthernetHeaderStruct;

// Structure of an internet header, naked of options, only valid for LITTLE ENDIAN
typedef struct
{
	unsigned char	ip_hl:4;		// Header length
	unsigned char	ip_v:4;			// IP protocol version
	unsigned char	ip_tos;			// Type of service 
	unsigned short	ip_len;			// Total length 
	unsigned short	ip_id;			// Identification
	unsigned short	ip_off;			// Fragment offset field
	unsigned char	ip_ttl;			// Time to live
	unsigned char	ip_p;			// Protocol
	unsigned short	ip_sum;			// Header checksum
	struct in_addr	ip_src;			// Source address
	struct in_addr	ip_dest;		// Destination address
} IpHeaderStruct;


// Strucutre of a TCP header, only valid for LITTLE ENDIAN
typedef struct
{
    unsigned short source;		// source port
    unsigned short dest;		// destination port
    unsigned long seq;			// sequence number
    unsigned long ack;			// acknowledgement id
    unsigned int x2:4;			// unused
    unsigned int off:4;			// data offset
    unsigned char flags;		// flags field
    unsigned short win;			// window size
    unsigned short sum;			// tcp checksum
    unsigned short urp;			// urgent pointer

} TcpHeaderStruct;
#define TCP_HEADER_LENGTH 20


// Structure of UDP header
typedef struct
{
	unsigned short source;			// Source port
	unsigned short dest;			// Destination port
	unsigned short len;				// UDP length
	unsigned short check;			// UDP Checksum
} UdpHeaderStruct;

#define RTP_PT_PCMU 0
#define RTP_PT_PCMA 8

// Structure of RTP header, only valid for little endian
typedef struct 
{
	unsigned short cc:4;		// CSRC count
	unsigned short x:1;			// header extension flag
	unsigned short p:1;			// padding flag
	unsigned short version:2;	// protocol version
	unsigned short pt:7;		// payload type
	unsigned short m:1;			// marker bit
	unsigned short seq;			// sequence number
	unsigned int ts;			// timestamp
	unsigned int ssrc;			// synchronization source
	//unsigned int csrc[1];		// optional CSRC list
} RtpHeaderStruct;

//===================================================================
// Cisco Callmanager -> endpoint messages
typedef struct
{
	unsigned long len;
	unsigned long reserved;
	unsigned long messageType;
} SkinnyHeaderStruct;

typedef struct
{
	SkinnyHeaderStruct header;
	unsigned long conferenceId;
	unsigned long passThruPartyId;
	struct in_addr remoteIpAddr;
	unsigned long remoteTcpPort;
	// and some more fields
} SkStartMediaTransmissionStruct;

bool SkinnyValidateStartMediaTransmission(SkStartMediaTransmissionStruct *);


typedef struct
{
	SkinnyHeaderStruct header;
	unsigned long conferenceId;
	unsigned long passThruPartyId;
} SkStopMediaTransmissionStruct;


typedef struct
{
	SkinnyHeaderStruct header;
	unsigned long conferenceId;
	unsigned long passThruPartyId;
} SkCloseReceiveChannelStruct;

#define SKINNY_CALLING_PARTY_SIZE 24
#define SKINNY_CALLED_PARTY_SIZE 24
#define SKINNY_CALLING_PARTY_NAME_SIZE 40
#define SKINNY_CALLED_PARTY_NAME_SIZE 40
typedef struct
{
	SkinnyHeaderStruct header;
	char callingPartyName[SKINNY_CALLING_PARTY_NAME_SIZE];
	char callingParty[SKINNY_CALLING_PARTY_SIZE];
	char calledPartyName[SKINNY_CALLED_PARTY_NAME_SIZE];
	char calledParty[SKINNY_CALLED_PARTY_SIZE];
	unsigned long lineInstance;
	unsigned long callId;
	unsigned long callType;
} SkCallInfoStruct;

bool SkinnyValidateCallInfo(SkCallInfoStruct *);


typedef struct
{
	SkinnyHeaderStruct header;
	unsigned long unknown1;
	unsigned long callId;
	char unknown2[24];
	char parties[76];
} SkNewCallInfoStruct;

#define SKINNY_LINE_DIR_NUMBER_SIZE 24
#define SKINNY_DISPLAY_NAME_SIZE 40
typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int lineNumber;
	char lineDirNumber[SKINNY_LINE_DIR_NUMBER_SIZE];
	char displayName[SKINNY_DISPLAY_NAME_SIZE];
} SkLineStatStruct;

bool SkinnyValidateLineStat(SkLineStatStruct*);

// Endpoint -> Cisco Callmanager messages
typedef struct
{
	SkinnyHeaderStruct header;
	unsigned long openReceiveChannelStatus;
	struct in_addr endpointIpAddr;
	unsigned long endpointTcpPort;
	unsigned long passThruPartyId;
} SkOpenReceiveChannelAckStruct;

bool SkinnyValidateOpenReceiveChannelAck(SkOpenReceiveChannelAckStruct *);


#define SKINNY_CTRL_PORT 2000
#define SKINNY_MIN_MESSAGE_SIZE 12
#define SKINNY_HEADER_LENGTH 8

#define SKINNY_MSG_UNKN "Unkn"
#define SKINNY_MSG_START_MEDIA_TRANSMISSION "StartMediaTransmission"
#define SKINNY_MSG_STOP_MEDIA_TRANSMISSION "StopMediaTransmission"
#define SKINNY_MSG_CLOSE_RECEIVE_CHANNEL "CloseReceiveChannel"
#define SKINNY_MSG_CALL_INFO_MESSAGE "CallInfoMessage"
#define SKINNY_MSG_OPEN_RECEIVE_CHANNEL_ACK "OpenReceiveChannelAck"
#define SKINNY_MSG_LINE_STAT_MESSAGE "LineStatMessage"

#define SKINNY_CALL_TYPE_INBOUND 1
#define SKINNY_CALL_TYPE_OUTBOUND 2
#define SKINNY_CALL_TYPE_FORWARD 3

typedef enum
{
	SkOpenReceiveChannelAck = 0x0022,
	SkStartMediaTransmission = 0x008A,
	SkStopMediaTransmission = 0x008B,
	SkCallInfoMessage = 0x008F,
	SkLineStatMessage = 0x0092,
	SkCloseReceiveChannel = 0x0106,
	SkUnkn = 0x0
} SkinnyMessageEnum;
int SkinnyMessageToEnum(CStdString& msg);
CStdString SkinnyMessageToString(int msgEnum);

#endif

