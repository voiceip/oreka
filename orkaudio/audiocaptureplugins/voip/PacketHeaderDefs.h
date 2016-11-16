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
	unsigned short type;

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

	bool isFragmented() {
		return (!isLastFragment() || offset()>0) ;
	}
	bool isLastFragment() {
		return fragmentFlags()%2 == 0;
	}
	size_t payloadLen() {
		return packetLen() - headerLen();
	}
	unsigned short packetId () {
		return ntohs(ip_id);
	}
	size_t offset() { 
		   return ((ntohs(ip_off)) & 0x1FFF) << 3; // last 13 bits * 8
	}
	unsigned int fragmentFlags() {
		return (ntohs(ip_off)) >> 13; // first 3 bits
	}
	size_t headerLen() {
		return ip_hl*4;
	}
	size_t packetLen() {
		return ntohs(ip_len);
	}
	void setPacketLen(size_t len) {
		ip_len = htons(len);
	}
	CStdString log() {
		CStdString s;
		s.Format("ipHeaderLen:%u packetLen:%u payloadLen:%u",headerLen(),packetLen(),payloadLen());
		return s;
	}
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

typedef struct {
	unsigned short flagVersion;
	unsigned short protocolType;
	unsigned int seq;
} GreHeaderStruct;

typedef struct {
	unsigned char data [8];
} ErspanHeaderStruct;
//===================================================================
// Cisco Callmanager -> endpoint messages
typedef struct
{
	unsigned int len;
	unsigned int headerVersion;
	unsigned int messageType;
} SkinnyHeaderStruct;

typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int conferenceId;
	unsigned int passThruPartyId;
	struct in_addr remoteIpAddr;
	unsigned int remoteTcpPort;
	// and some more fields
} SkStartMediaTransmissionStruct;

bool SkinnyValidateStartMediaTransmission(SkStartMediaTransmissionStruct *, u_char* packetEnd);

typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int conferenceId;
	unsigned int passThruPartyId;
	unsigned int stuff1;
	struct in_addr remoteIpAddr;
	unsigned int stuff2;
	unsigned int stuff3;
	unsigned int stuff4;
	unsigned int remoteTcpPort;
	// Other stuff
} SkCcm7_1StartMediaTransmissionStruct;

bool SkinnyValidateCcm7_1StartMediaTransmission(SkCcm7_1StartMediaTransmissionStruct *, u_char* packetEnd);

typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int conferenceId;
	unsigned int passThruPartyId;
} SkStopMediaTransmissionStruct;

bool SkinnyValidateStopMediaTransmission(SkStopMediaTransmissionStruct*, u_char* packetEnd);

typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int conferenceId;
	unsigned int passThruPartyId;
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
	unsigned int lineInstance;
	unsigned int callId;
	unsigned int callType;
} SkCallInfoStruct;

bool SkinnyValidateCallInfo(SkCallInfoStruct *, u_char* packetEnd);

typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int callState;
	unsigned int line;
	unsigned int callId;
} SkCallStateMessageStruct;

bool SkinyValidateCallStateMessage(SkCallStateMessageStruct*, u_char* packetEnd);

#define SKINNY_CCM5_PARTIES_BLOCK_SIZE 76
typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int unknown1;
	unsigned int callId;
	unsigned int callType;
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
	unsigned int openReceiveChannelStatus;
	struct in_addr endpointIpAddr;
	unsigned int endpointTcpPort;
	unsigned int passThruPartyId;
} SkOpenReceiveChannelAckStruct;

bool SkinnyValidateOpenReceiveChannelAck(SkOpenReceiveChannelAckStruct *, u_char* packetEnd);

typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int openReceiveChannelStatus;
	unsigned int stuff1;
	struct in_addr endpointIpAddr;
	unsigned int passThruPartyId;
	unsigned int stuff2;
	unsigned int stuff3;
	unsigned int endpointTcpPort;
} SkCcm7_1SkOpenReceiveChannelAckStruct;

bool SkinnyValidateCcm7_1SkOpenReceiveChannelAckStruct(SkCcm7_1SkOpenReceiveChannelAckStruct *orca, u_char* packetEnd);

typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int softKeyEvent;
	unsigned int lineInstance;
	unsigned int callIdentifier;
} SkSoftKeyEventMessageStruct;

bool SkinnyValidateSoftKeyEvent(SkSoftKeyEventMessageStruct *, u_char* packetEnd);


typedef struct
{
	SkinnyHeaderStruct header;
	unsigned int lineInstance;
	unsigned int callIdentifier;
	unsigned int softKeySetDescription;
} SkSoftKeySetDescriptionStruct;

bool SkinnyValidateSoftKeySetDescription(SkSoftKeySetDescriptionStruct*, u_char* packetEnd);

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
#define SKINNY_MSG_SOFT_KEY_SET_DESCRIPTION "SoftKeySetDescription"
#define SKINNY_MSG_CALL_STATE_MESSAGE "CallStateMessage"

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
	SkSoftKeySetDescription = 0x0110,
	SkCallStateMessage = 0x0111,
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
//==================================================================
#define SKINNY_SOFTKEYSET_MAX_EVENT	10
#define SKINNY_SOFTKEYSET_MIN_EVENT	0
class SoftKeySetDescription
{
public:
#define SKINNY_SOFTKEYSET_ONHOOK "OnHook"
#define SKINNY_SOFTKEYSET_CONNECTED "Connected"
#define SKINNY_SOFTKEYSET_ONHOLD "OnHold"
#define SKINNY_SOFTKEYSET_RINGIN "RingIn"
#define SKINNY_SOFTKEYSET_OFFHOOK "OffHook"
#define SKINNY_SOFTKEYSET_TRANSFER "ConnectedWithTransfer"
#define SKINNY_SOFTKEYSET_DIGITS "DigitsAfterDialingFirstDigit"
#define SKINNY_SOFTKEYSET_CONFERENCE "ConnectedWithConference"
#define SKINNY_SOFTKEYSET_RINGOUT "RingOut"
#define SKINNY_SOFTKEYSET_ONHOOKFEATURE "OnHookWithFeature"
#define SKINNY_SOFTKEYSET_UNKN "Unknown"
	typedef enum {
		SkSoftKeySetOnHook = 0,
		SkSoftKeySetConnected = 1,
		SkSoftKeySetOnHold = 2,
		SkSoftKeySetRIngIn = 3,
		SkSoftKeySetOffHook = 4,
		SkSoftKeySetTransfer = 5,
		SkSoftKeySetDigits = 6,
		SkSoftKeySetConference = 7,
		SkSoftKeySetRingOut = 8,
		SkSoftKeySetOffHookFeature = 9,
		SkSoftKeySetUnkn = 10
	} SoftKeySetDescriptionEnum;
	static int SoftKeySetDescriptionToEnum(CStdString& event);
	static CStdString SoftKeySetDescriptionToString(int event);
private:
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

#define SIP_METHOD_NOTIFY_SIZE 6
#define SIP_METHOD_SUBSCRIBE_SIZE 9
#define SIP_METHOD_INVITE_SIZE 6
#define SIP_METHOD_ACK_SIZE 3
#define SIP_METHOD_BYE_SIZE 3
#define SIP_RESPONSE_200_OK_SIZE 11
#define SIP_RESPONSE_SESSION_PROGRESS_SIZE 28
#define SIP_RESPONSE_302_MOVED_TEMPORARILY_SIZE 29
#define SIP_INFO_SIZE 4
#define SIP_METHOD_NOTIFY "NOTIFY"
#define SIP_METHOD_SUBSCRIBE "SUBSCRIBE"
#define SIP_METHOD_INVITE "INVITE"
#define SIP_METHOD_ACK "ACK"
#define SIP_METHOD_BYE "BYE"
#define SIP_METHOD_200_OK "200 OK"
#define SIP_RESPONSE_200_OK "SIP/2.0 200"
#define SIP_RESPONSE_SESSION_PROGRESS "SIP/2.0 183 Session Progress"
#define SIP_RESPONSE_302_MOVED_TEMPORARILY "SIP/2.0 302 Moved Temporarily"
#define SIP_METHOD_INFO "INFO"
#define SIP_METHOD_INFO_SIZE 4
#define SIP_METHOD_REFER "REFER"
#define SIP_METHOD_REFER_SIZE 5

#define DTMF_DIGIT_ZERO "0"
#define DTMF_DIGIT_ONE "1"
#define DTMF_DIGIT_TWO "2"
#define DTMF_DIGIT_THREE "3"
#define DTMF_DIGIT_FOUR "4"
#define DTMF_DIGIT_FIVE "5"
#define DTMF_DIGIT_SIX "6"
#define DTMF_DIGIT_SEVEN "7"
#define DTMF_DIGIT_EIGHT "8"
#define DTMF_DIGIT_NINE "9"
#define DTMF_DIGIT_START "*"
#define DTMF_DIGIT_SHARP "#"
#define DTMF_DIGIT_UNKN "unknown"
typedef enum{
	DtmfDigitZero = 0,
	DtmfDigitOne = 1,
	DtmfDigitTwo = 2,
	DtmfDigitThree = 3,
	DtmfDigitFour = 4,
	DtmfDigitFive = 5,
	DtmfDigitSix = 6,
	DtmfDigitSeven = 7,
	DtmfDigitEight = 8,
	DtmfDigitNine = 9,
	DtmfDigitStart = 10,
	DtmfDigitSharp = 11,
	DtmfDigitUnknown = 12
}DtmfDigitEnum;

int DtmfDigitToEnum(CStdString& msg);
CStdString DtmfDigitToString(int msgEnum);
//==============================================
#pragma pack(push,1)	// avoid padded bytes at the end of the struct
typedef struct {
	unsigned short protocol;	//htons(0xbeef);
	unsigned char version;		//htons(0x01);
	unsigned int seq;
	unsigned short totalPacketLength;
} OrkEncapsulationStruct;
#pragma pack(pop)

#endif



