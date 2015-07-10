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

#ifndef __IAX2SESSION_H__
#define __IAX2SESSION_H__

#include <log4cxx/logger.h>
#include "Iax2Session.h"
#include <map>
#include "ace/Singleton.h"
#include "PacketHeaderDefs.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
using namespace log4cxx;

/* The frame types we're interested in */
#define IAX2_FRAME_VOICE	2
#define IAX2_FRAME_CONTROL	4
#define IAX2_FRAME_IAX		6

/* The full frame subclass values for IAX2_FRAME_IAX frames
 * which we're interested in */
#define IAX2_COMMAND_NEW        1
#define IAX2_COMMAND_HANGUP     5
#define IAX2_COMMAND_REJECT     6
#define IAX2_COMMAND_ACCEPT     7
#define IAX2_COMMAND_AUTHREQ    8

/* The control frame subclass values for IAX2_FRAME_CONTROL
 * which we're interested in */
#define IAX2_CONTROL_HANGUP	1

/* The information elements we're interested in */
#define IAX2_IE_CALLED_NUMBER	1
#define IAX2_IE_CALLING_NUMBER	2
#define IAX2_IE_CALLING_NAME    4
#define IAX2_IE_USERNAME	6
#define IAX2_IE_FORMAT		9
#define IAX2_IE_AUTHMETHODS	14
#define IAX2_IE_CHALLENGE	15

struct iax2_ies {
	char *caller;
	char *callee;	/* Mandatory for NEW */
	unsigned int format;	/* Format Mandatory for ACCEPT */
	char *username;	/* Mandatory for AUTHREQ */
	unsigned int authmethods; /* Mandatory for AUTHREQ */
	char *challenge; /* Mandatory for AUTHREQ */
	char *calling_name;
};

/* Supported Voice Codecs */
/* G.723.1 compression */
#define IAX2_CODEC_G723_1	(1 << 0)
/* GSM compression */
#define IAX2_CODEC_GSM		(1 << 1)
/* Raw mu-law data (G.711) */
#define IAX2_CODEC_ULAW		(1 << 2)
/* Raw A-law data (G.711) */
#define IAX2_CODEC_ALAW         (1 << 3)
/* ADPCM (G.726, 32kbps) */
#define IAX2_CODEC_G726         (1 << 4)
/* ADPCM (IMA) */
#define IAX2_CODEC_ADPCM        (1 << 5)
/* Raw 16-bit Signed Linear (8000 Hz) PCM */
#define IAX2_CODEC_SLINEAR      (1 << 6)
/* LPC10, 180 samples/frame */
#define IAX2_CODEC_LPC10        (1 << 7)
/* G.729A audio */
#define IAX2_CODEC_G729A        (1 << 8)
/* SpeeX Free Compression */
#define IAX2_CODEC_SPEEX        (1 << 9)
/* iLBC Free Compression */
#define IAX2_CODEC_ILBC         (1 << 10)

class Iax2AcceptInfo
{
public:
	Iax2AcceptInfo();
	void ToString(CStdString& string);

        struct in_addr m_senderIp;
        struct in_addr m_receiverIp;

        CStdString m_sender_callno;
        CStdString m_receiver_callno;
};
typedef oreka::shared_ptr<Iax2AcceptInfo> Iax2AcceptInfoRef;


class Iax2AuthreqInfo
{
public:
	Iax2AuthreqInfo();
	void ToString(CStdString& string);

	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;

	CStdString m_sender_callno;
	CStdString m_receiver_callno;
};
typedef oreka::shared_ptr<Iax2AuthreqInfo> Iax2AuthreqInfoRef;

class Iax2NewInfo
{
public:
	Iax2NewInfo();
	void ToString(CStdString& string);

	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;

	CStdString m_caller; /* CALLING NUMBER */
	CStdString m_callee; /* CALLED NUMBER */
	CStdString m_callNo; /* This is the source call number */
	CStdString m_callingName; /* Calling Name */
	CStdString m_localExtension;

	bool m_validated; // true when an IAX2 voice stream has been seen for the NEW
};
typedef oreka::shared_ptr<Iax2NewInfo> Iax2NewInfoRef;

class Iax2HangupInfo
{
public:
	Iax2HangupInfo();
	void ToString(CStdString& string);
	/* Even if we did not have the destination call number,
	 * we will be able to look it up using m_byDestIpAndCallNo */
	struct in_addr m_senderIp;
	struct in_addr m_receiverIp;

	CStdString m_sender_callno;
	CStdString m_receiver_callno;
};
typedef oreka::shared_ptr<Iax2HangupInfo> Iax2HangupInfoRef;

#define IAX2_FRAME_FULL         1
#define IAX2_FRAME_MINI         2
#define IAX2_FRAME_TRUNK        3

/* IAX2_FRAME_VOICE Packet Information.  May be full or mini.*/
class Iax2PacketInfo
{
public:
	Iax2PacketInfo();
        void ToString(CStdString& string);

        struct in_addr m_sourceIp;
        struct in_addr m_destIp;
        unsigned short m_sourcecallno; /* Always there */
        unsigned short m_destcallno; /* May be zero for mini frames */

        unsigned int m_frame_type;
        unsigned int m_payloadSize;
        unsigned short m_payloadType;
        unsigned char* m_payload;
        unsigned short m_seqNum;
        unsigned int m_timestamp;
        time_t m_arrivalTimestamp;
};
typedef oreka::shared_ptr<Iax2PacketInfo> Iax2PacketInfoRef;

// ============================================================

#define IAX2_STATE_WAITING      1
#define IAX2_STATE_LINKED       2
#define IAX2_STATE_UP           3

class Iax2Session
{
public:
	Iax2Session(CStdString& trackingId);
#define IAX2_PROTOCOL_NUM	1
#define IAX2_PROTOCOL_STR	"IAX2"
	void Stop();
	void Start();
	bool AddIax2Packet(Iax2PacketInfoRef& iax2Packet);
	void ReportIax2New(Iax2NewInfoRef& newinfo);
        void ReportIax2Accept(Iax2AcceptInfoRef& acceptinfo);
        void ReportIax2Authreq(Iax2AuthreqInfoRef& authreq);

	CStdString m_trackingId;
	CStdString m_srcIpAndCallNo;
	CStdString m_destIpAndCallNo;
	Iax2NewInfoRef m_new;
	time_t m_lastUpdated;
	CStdString m_localParty;
	CStdString m_remoteParty;
	CStdString m_localEntryPoint;
	CaptureEvent::DirectionEnum m_direction;
	int m_numIax2Packets;
	int m_codec;
	int m_iax2_state;

private:
	/* XXX Way of "freeing" sessions which've been idle for a 
	 * while? Just in case they never hang up.  Answer is Hoover() */
	void ProcessMetadataIax2(Iax2PacketInfoRef&);
	void ProcessMetadataIax2Incoming();
	void ProcessMetadataIax2Outgoing();
	void UpdateMetadataIax2(Iax2PacketInfoRef& iax2Packet, bool);
	void ProcessMetadataRawIax2(Iax2PacketInfoRef&);
	void ProcessMetadataSkinny(Iax2PacketInfoRef& iax2Packet);
	void ReportMetadata();
	void GenerateOrkUid();
	int RtpTimestamp();

	Iax2PacketInfoRef m_lastIax2Packet;
	Iax2PacketInfoRef m_lastIax2PacketSide1;
	Iax2PacketInfoRef m_lastIax2PacketSide2;

	struct in_addr m_invitorIp;
	struct in_addr m_inviteeIp;
	struct in_addr m_localIp;
	struct in_addr m_remoteIp;

        unsigned short m_invitor_scallno;
        unsigned short m_invitee_scallno;

	//struct in_addr m_localMac;
	//struct in_addr m_remoteMac;
	LoggerPtr m_log;
	CStdString m_capturePort;
	bool m_started;
	bool m_stopped;
	time_t m_beginDate;
	CStdString m_orkUid;
	bool m_hasDuplicateIax2;
	unsigned int m_highestIax2SeqNumDelta;
	double m_minIax2SeqDelta;
	double m_minIax2TimestampDelta;
	//TcpAddressList m_iax2AddressList;
	std::list<Iax2NewInfoRef> m_news;
	std::map<CStdString, CStdString> m_tags;

	unsigned short m_channel1SeqNo;
	unsigned short m_channel2SeqNo;
};
typedef oreka::shared_ptr<Iax2Session> Iax2SessionRef;

//===================================================================
class Iax2Sessions
{
public:
	Iax2Sessions();
	void Stop(Iax2SessionRef& session);
	void StopAll();
	void ReportIax2New(Iax2NewInfoRef& invite);
	void ReportIax2Hangup(Iax2HangupInfoRef& hangup);
	void ReportIax2Accept(Iax2AcceptInfoRef& acceptinfo);
	void ReportIax2Authreq(Iax2AuthreqInfoRef& authreq);
	bool ReportIax2Packet(Iax2PacketInfoRef& iax2Packet);
	void Hoover(time_t now);
private:
	std::map<CStdString, Iax2SessionRef> m_bySrcIpAndCallNo; /* With IAX2 the callnos can easily
							       * be duplicated across machines
							       * so we need to use the ip and callno */
	std::map<CStdString, Iax2SessionRef> m_byDestIpAndCallNo; /* Invitee Ip and Call Id */
	LoggerPtr m_log;
	AlphaCounter m_alphaCounter;
};
typedef ACE_Singleton<Iax2Sessions, ACE_Thread_Mutex> Iax2SessionsSingleton;

/* Miscellaneous */
static inline unsigned int get_unaligned_uint32(void *p)
{
        struct pckd { unsigned int d; };
	struct pckd *pp = (struct pckd *)p;

        return pp->d;
}

#endif

