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

#pragma warning( disable: 4200 ) // disables warning C4200: nonstandard extension used : zero-sized array in struct/union

#include "ace/OS_NS_arpa_inet.h"
#include "StdString.h"


// Structure of Ethernet header
typedef struct
{
	unsigned char destinationMac[6];
	unsigned char sourceMac[6];
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
    unsigned int seq;			// sequence number
    unsigned int ack;			// acknowledgement id
    unsigned char x2:4;			// unused
    unsigned char off:4;		// data offset
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

// Structure of RTP payload format for RTP events (ref RFC 2833, section
// 3.5)
typedef struct
{
	unsigned char event;
	unsigned char er_volume;	// Also contains end and error booleans on bit 8 and 7 respectively.
	unsigned short duration;
} RtpEventPayloadFormat;

// Structure of common header for RTCP
typedef struct {
	unsigned char vpc;		// version (2 bits), padding (1 bit), count (5 bits)
	unsigned char pt;		// RTCP packet type
	unsigned short length;		// RTCP packet length in words without this word (i.e minus 1)
} RtcpCommonHeaderStruct;

typedef struct {
	unsigned char type;
	unsigned char length;
	unsigned char data[0];
} RtcpSdesCsrcItem;

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

bool SkinnyValidateStartMediaTransmission(SkStartMediaTransmissionStruct *, u_char* packetEnd);


typedef struct
{
	SkinnyHeaderStruct header;
	unsigned long conferenceId;
	unsigned long passThruPartyId;
} SkStopMediaTransmissionStruct;

bool SkinnyValidateStopMediaTransmission(SkStopMediaTransmissionStruct*, u_char* packetEnd);

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

bool SkinnyValidateCallInfo(SkCallInfoStruct *, u_char* packetEnd);


#define SKINNY_CCM5_PARTIES_BLOCK_SIZE 76
typedef struct
{
	SkinnyHeaderStruct header;
	unsigned long unknown1;
	unsigned long callId;
	unsigned long callType;
	char unknown2[20];
	char parties[0];
} SkCcm5CallInfoStruct;

bool SkinnyValidateCcm5CallInfo(SkCcm5CallInfoStruct *, u_char* packetEnd);

#define SKINNY_LINE_DIR_NUMBER_SIZE 24
#define SKINNY_DISPLAY_NAME_SIZE 40
typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int lineNumber;
	char lineDirNumber[SKINNY_LINE_DIR_NUMBER_SIZE];
	char displayName[SKINNY_DISPLAY_NAME_SIZE];
} SkLineStatStruct;

bool SkinnyValidateLineStat(SkLineStatStruct*, u_char* packetEnd);

// Endpoint -> Cisco Callmanager messages
typedef struct
{
	SkinnyHeaderStruct header;
	unsigned long openReceiveChannelStatus;
	struct in_addr endpointIpAddr;
	unsigned long endpointTcpPort;
	unsigned long passThruPartyId;
} SkOpenReceiveChannelAckStruct;

bool SkinnyValidateOpenReceiveChannelAck(SkOpenReceiveChannelAckStruct *, u_char* packetEnd);

typedef struct
{
	SkinnyHeaderStruct header;
	unsigned long softKeyEvent;
	unsigned long lineInstance;
	unsigned long callIdentifier;
} SkSoftKeyEventMessageStruct;

bool SkinnyValidateSoftKeyEvent(SkSoftKeyEventMessageStruct *, u_char* packetEnd);

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
#define SKINNY_MSG_CCM5_CALL_INFO_MESSAGE "Ccm5CallInfoMessage"
#define SKINNY_MSG_SOFT_KEY_EVENT_MESSAGE "SoftKeyEventMessage"

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
	SkCcm5CallInfoMessage = 0x14A,
	SkSoftKeyEventMessage = 0x0026,
	SkUnkn = 0x0
} SkinnyMessageEnum;
int SkinnyMessageToEnum(CStdString& msg);
CStdString SkinnyMessageToString(int msgEnum);

#define SKINNY_SOFTKEY_MAX_EVENT	18
#define SKINNY_SOFTKEY_MIN_EVENT	1

class SoftKeyEvent
{
public:
#define SKINNY_SOFTKEY_REDIAL "Redial"
#define SKINNY_SOFTKEY_NEWCALL "NewCall"
#define SKINNY_SOFTKEY_HOLD "Hold"
#define SKINNY_SOFTKEY_TRNSFER "Trnsfer"
#define SKINNY_SOFTKEY_CFWDALL "CFwdAll"
#define SKINNY_SOFTKEY_CFWDBUSY "CFwdBusy"
#define SKINNY_SOFTKEY_CFWDNOANSWER "CFwdNoAnswer"
#define SKINNY_SOFTKEY_BACKSPACE "BackSpace"
#define SKINNY_SOFTKEY_ENDCALL "EndCall"
#define SKINNY_SOFTKEY_RESUME "Resume"
#define SKINNY_SOFTKEY_ANSWER "Answer"
#define SKINNY_SOFTKEY_INFO "Info"
#define SKINNY_SOFTKEY_CONFRN "Confrn"
#define SKINNY_SOFTKEY_PARK "Park"
#define SKINNY_SOFTKEY_JOIN "Join"
#define SKINNY_SOFTKEY_MEETMECONFRN "MeetMeConfrn"
#define SKINNY_SOFTKEY_CALLPICKUP "CallPickUp"
#define SKINNY_SOFTKEY_GRPCALLPICKUP "GrpCallPickUp"
#define SKINNY_SOFTKEY_UNKN "Unknown"

	typedef enum {
		SkSoftKeyRedial = 1,
		SkSoftKeyNewCall = 2,
		SkSoftKeyHold = 3,
		SkSoftKeyTrnsfer = 4,
		SkSoftKeyCFwdAll = 5,
		SkSoftKeyCFwdBusy = 6,
		SkSoftKeyCFwdNoAnswer = 7,
		SkSoftKeyBackSpace = 8,
		SkSoftKeyEndCall = 9,
                SkSoftKeyResume = 10,
		SkSoftKeyAnswer = 11,
		SkSoftKeyInfo = 12,
                SkSoftKeyConfrn = 13,
                SkSoftKeyPark = 14,
                SkSoftKeyJoin = 15,
                SkSoftKeyMeetMeConfrn = 16,
                SkSoftKeyCallPickUp = 17,
                SkSoftKeyGrpCallPickUp = 18,
		SkSoftKeyUnkn = 0
	} SoftKeyEventEnum;

	static int SoftKeyEventToEnum(CStdString& event);
	static CStdString SoftKeyEventToString(int event);
};


//===================================================================
/*
 * IAX2 Packet Headers
 */

/* XXX No support for encryption */

/* Full frame */
struct Iax2FullHeader {
	unsigned short scallno;
	unsigned short dcallno;
	unsigned int ts;
	unsigned char oseqno;
	unsigned char iseqno;
	unsigned char type;
	unsigned char c_sub;
	unsigned char ie_data[0];
};

/* Mini frame for voice */
struct Iax2MiniHeader {
	unsigned short scallno;
	unsigned short ts; /* Low 16 bits from transmitting peer's full 32-bit ts */
	unsigned char data[0];
};

/* Meta trunk frame */
struct Iax2MetaTrunkHeader {
	unsigned short meta; /* zero for meta frames */
	unsigned char metacmd;
	unsigned char cmddata;
	unsigned int ts;
	unsigned char data[0];
};

/* Mini trunked frame with timestamps (trunk timestamps
 * flag is set to 0 */
struct Iax2MetaTrunkEntry {
	unsigned short scallno;
	unsigned short len;
	unsigned char data[0];
};

/* Mini trunked frame with timestamps (trunk timestamps
 * flag is set to 1 */
struct Iax2MetaTrunkEntryTs {
	unsigned short len;
	struct Iax2MiniHeader mini;
};

//==============================================
// SIP

#define SIP_METHOD_INVITE_SIZE 6
#define SIP_METHOD_ACK_SIZE 3
#define SIP_METHOD_BYE_SIZE 3
#define SIP_RESPONSE_200_OK_SIZE 11
#define SIP_RESPONSE_SESSION_PROGRESS_SIZE 28
#define SIP_METHOD_INVITE "INVITE"
#define SIP_METHOD_ACK "ACK"
#define SIP_METHOD_BYE "BYE"
#define SIP_RESPONSE_200_OK "SIP/2.0 200"
#define SIP_RESPONSE_SESSION_PROGRESS "SIP/2.0 183 Session Progress"

#endif



