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
#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning
#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#ifndef WIN32
#include "sys/socket.h"
#endif

#ifdef WIN32
#define snprintf _snprintf
#endif


#include <list>
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_strings.h"
#include "ace/OS_NS_dirent.h"
#include "ace/Singleton.h"
#include "ace/Min_Max.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/OS_NS_ctype.h"
#include "ace/Thread_Manager.h"
#include "ace/Thread_Mutex.h"
#include "ace/Thread_Semaphore.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "Utils.h"
#include "VoIpConfig.h"
#include "pcap.h"
#include "PacketHeaderDefs.h"
#include "Rtp.h"
#include "RtpSession.h"
#include "Iax2Session.h"
#include "SipTcp.h"

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;
extern OrkLogManager* g_logManager;

#include "LogManager.h"

static LoggerPtr s_packetLog;
static LoggerPtr s_packetStatsLog;
static LoggerPtr s_rtpPacketLog;
static LoggerPtr s_sipPacketLog;
static LoggerPtr s_sipTcpPacketLog;
static LoggerPtr s_skinnyPacketLog;
static LoggerPtr s_sipExtractionLog;
static LoggerPtr s_voipPluginLog;
static LoggerPtr s_rtcpPacketLog;
static time_t s_lastHooveringTime;
static time_t s_lastPause;
static time_t s_lastPacketTimestamp;
static ACE_Thread_Mutex s_mutex;
static ACE_Thread_Semaphore s_replaySemaphore;
int s_replayThreadCounter;
static bool s_liveCapture;
static time_t s_lastPcapStatsReportingTime;
static time_t s_lastPacketsPerSecondTime;
static unsigned int s_numPackets;
static unsigned int s_numPacketsPerSecond;
static unsigned int s_minPacketsPerSecond;
static unsigned int s_maxPacketsPerSecond;
static std::list<SipTcpStreamRef> s_SipTcpStreams;

VoIpConfigTopObjectRef g_VoIpConfigTopObjectRef;
#define DLLCONFIG g_VoIpConfigTopObjectRef.get()->m_config

#define PROMISCUOUS 1
#define LOCAL_PARTY_MAP_FILE	"localpartymap.csv"
#define ETC_LOCAL_PARTY_MAP_FILE	"/etc/orkaudio/localpartymap.csv"

//========================================================
class VoIp
{
public:
	VoIp();
	void Initialize();
	void Run();
	void Shutdown();
	void StartCapture(CStdString& port, CStdString& orkuid, CStdString& nativecallid, CStdString& side);
	void StopCapture(CStdString& port, CStdString& orkuid, CStdString& nativecallid, CStdString& qos);
	void PauseCapture(CStdString& port, CStdString& orkuid, CStdString& nativecallid);
	void SetOnHold(CStdString& port, CStdString& orkuid);
	void SetOffHold(CStdString& port, CStdString& orkuid);
	void ReportPcapStats();
	pcap_t* OpenDevice(CStdString& name);
	void AddPcapDeviceToMap(CStdString& deviceName, pcap_t* pcapHandle);
	void RemovePcapDeviceFromMap(pcap_t* pcapHandle);
	CStdString GetPcapDeviceName(pcap_t* pcapHandle);
	void ProcessLocalPartyMap(char *line, int ln);
	void LoadPartyMaps();
	void GetConnectionStatus(CStdString& msg);

private:
	void OpenDevices();
	void OpenPcapFile(CStdString& filename);
	void OpenPcapDirectory(CStdString& path);
	void SetPcapSocketBufferSize(pcap_t* pcapHandle);
	char* ApplyPcapFilter();

	pcap_t* m_pcapHandle;
	std::list<pcap_t*> m_pcapHandles;

	std::map<pcap_t*, CStdString> m_pcapDeviceMap;
	ACE_Thread_Mutex m_pcapDeviceMapMutex;
};

typedef ACE_Singleton<VoIp, ACE_Thread_Mutex> VoIpSingleton;

//=========================================================
// Convert a piece of memory to hex string
void memToHex(unsigned char* input, size_t len, CStdString&output)
{
	char byteAsHex[10];
	for(unsigned int i=0; i<len; i++)
	{
		sprintf(byteAsHex, "%.2x", input[i]);
		output += byteAsHex;
	}
}

// Same as standard memchr but case insensitive
inline char* memnchr(void *s, int c, size_t len)
{
	char lowerCase = tolower(c);
	char upperCase = toupper(c);
	char* stop = (char*)s + len;
	for(char* ptr = (char*)s ; ptr < stop; ptr++)
	{
		if(*ptr == lowerCase || *ptr == upperCase)
		{
			return ptr;
		}
	}
	return NULL;
}

static char* memFindStr(const char* toFind, char* start, char* stop)
{
	for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memchr(ptr+1, toFind[0],(stop - ptr - 1)))
	{
		if(ACE_OS::strncasecmp(toFind, ptr, ((int)strlen(toFind) > (stop-ptr) ? (stop-ptr) : strlen(toFind))) == 0)
		{
			return (ptr);
		}
	}
	return NULL;
}

// find the address that follows the given search string between start and stop pointers - case insensitive
char* memFindAfter(const char* toFind, char* start, char* stop)
{
	for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memnchr(ptr+1, toFind[0],(stop - ptr - 1)))
	{
		if(ACE_OS::strncasecmp(toFind, ptr, strlen(toFind)) == 0)
		{
			return (ptr+strlen(toFind));
		}
	}
	return NULL;
}

char* memFindEOL(char* start, char* limit)
{
	char* c = start;
	while(*c != '\r' && *c != '\n' && c<limit)
	{
		c++;
	}
	if(*c == '\r' || *c == '\n')
	{
		return c;
	}
	return start;
}

// Grabs a string in memory until encountering null char, a space a CR or LF chars
void GrabToken(char* in, char* limit, CStdString& out)
{
	for(char* c = in; *c != '\0' && *c != 0x20 && *c != 0x0D && *c != 0x0A && c<limit; c = c+1)
	{
		out += *c;
	}
}

// Same as GrabToken but includes spaces in the token grabbed as opposed to stopping
// when a space is encountered
void GrabTokenAcceptSpace(char* in, char* limit, CStdString& out)
{
	for(char* c = in; *c != '\0' && *c != 0x0D && *c != 0x0A && c<limit; c = c+1)
	{
		out += *c;
	}
}

// Same as GrabToken but skipping leading whitespaces
void GrabTokenSkipLeadingWhitespaces(char* in, char* limit, CStdString& out)
{
	char* c = in;
	while(*c == 0x20 && (*c != '\0' && *c != 0x0D && *c != 0x0A) && c<limit)
	{
		c = c+1;
	}
	GrabToken(c, limit, out);
}

void GrabAlphaNumToken(char * in, char* limit, CStdString& out)
{
	// Look for first alphanum character
	char* start = in;
	while (!ACE_OS::ace_isalnum(*start) && start<limit)
	{
		start++;
	}

	if(start != (limit -1))
	{
		for(char* c = start; ACE_OS::ace_isalnum(*c); c = c+1)
		{
			out += *c;
		}
	}
}

// Same as GrabAlphaNumToken but skipping leading whitespaces
void GrabAlphaNumTokenSkipLeadingWhitespaces(char* in, char* limit, CStdString& out)
{
	char* c = in;
	while(*c == 0x20 && (*c != '\0' && *c != 0x0D && *c != 0x0A) && c < limit)
	{
		c = c+1;
	}
	GrabAlphaNumToken(c, limit, out);
}

char* SkipWhitespaces(char* in, char* limit)
{
	char* c = in;
	while(*c == 0x20 && (*c != '\0' && *c != 0x0D && *c != 0x0A) && c < limit)
	{
		c = c+1;
	}
	return c;
}

void GrabSipUriDomain(char* in, char* limit, CStdString& out)
{
	char* userStart = SkipWhitespaces(in, limit);
	if(userStart >= limit)
	{
		return;
	}

	char* domainStart = strchr(userStart, '@');
	if(!domainStart)
	{
		return;
	}

	domainStart += 1;
	if(*domainStart == '\0' || domainStart >= limit)
	{
		return;
	}

	for(char *c = domainStart; (ACE_OS::ace_isalnum(*c) || *c == '.' || *c == '-' || *c == '_') && (c < limit); c = c+1)
	{
		out += *c;
	}
}

void GrabSipName(char* in, char* limit, CStdString& out)
{
	char* nameStart = SkipWhitespaces(in, limit);
	char* nameEnd = memFindStr("<sip:", nameStart, limit);

	if(nameStart >= limit)
	{
		return;
	}

	if(nameEnd == NULL)
	{
		return;
	}

	if(nameEnd <= nameStart)
	{
		return;
	}

	// Get all characters before the <sip:
	for(char *c = nameStart; c < nameEnd; c = c+1)
	{
		if(c == nameStart && *c == '"')
		{
			continue;
		}
		if(((c+2 == nameEnd) || (c+1 == nameEnd)) && *c == '"')
		{
			break;
		}
		if(c+1 == nameEnd && *c == ' ')
		{
			break;
		}
		out += *c;
	}
}

void GrabSipUserAddress(char* in, char* limit, CStdString& out)
{
	char* userStart = SkipWhitespaces(in, limit);
	bool passedUserPart = false;

	if(userStart >= limit)
	{
		return;
	}

	/* Taken from RFC 1035, section 2.3.1 recommendation for
	 * domain names, we will add checks for '.' and '@' to allow
	 * the host part */
	for(char* c = userStart; (ACE_OS::ace_isalnum(*c) || *c == '#' || *c == '*' || *c == '.' || *c == '+' || *c == '-' || *c == '_' || *c == ':' || *c == '@' ) && c < limit ; c = c+1)
	{
		if(*c == '@' && !passedUserPart)
		{
			passedUserPart = true;
		}

		if(*c == ':' && passedUserPart)
		{
			break;
		}

		out += *c;
	}
}

void GrabSipUriUser(char* in, char* limit, CStdString& out)
{
	char* userStart = SkipWhitespaces(in, limit);
	if(userStart>=limit)
	{
		return;
	}
	// What stops a SIP URI user is a ':' (separating user from pwd) or an '@' (separating user from hostname)
	// but no need to test for these as we only allow alphanums and '#'
	for(char* c = userStart; (ACE_OS::ace_isalnum(*c) || *c == '#' || *c == '*' || *c == '.' || *c == '+' || *c == '-' || *c == '_' ) && c < limit ; c = c+1)
	{
		out += *c;
	}
}


void GrabString(char* start, char* stop, CStdString& out)
{
	char* c = start;
	while(c <= stop)
	{
		out += *c++;
	}
}

// Grabs a line of characters in memory from start pointer
// returns the end of line
char* GrabLine(char* start, char* limit, CStdString& out)
{
	char* c = start;
	while(c < limit && *c != 0x0D && *c != 0x0A)
	{
		out += *c++;
	}
	return c;
}

// Grabs a line of characters in memory skipping any leading
// whitespaces.  This is intended for use in the case of SIP
// headers, ref RFC 3261, section 7.3.1
void GrabLineSkipLeadingWhitespace(char* start, char* limit, CStdString& out)
{
	char* c = SkipWhitespaces(start, limit);

	GrabLine(c, limit, out);
}

static int iax2_codec_to_rtp_payloadtype(int codec)
{
	switch(codec) {
	case IAX2_CODEC_ULAW:
		return 0;
	case IAX2_CODEC_G726:
		return 2;
	case IAX2_CODEC_GSM:
		return 3;
	case IAX2_CODEC_G723_1:
		return 4;
	case IAX2_CODEC_ADPCM:
		return 5;
	case IAX2_CODEC_LPC10:
		return 7;
	case IAX2_CODEC_ALAW:
		return 8;
	case IAX2_CODEC_SLINEAR:
		return 9;
	case IAX2_CODEC_G729A:
		return 18;
	case IAX2_CODEC_ILBC:
		return 97;
	default:
		return -1;
	}

	/* NOT REACHED */
	return -1;
}

static int get_uncompressed_subclass(unsigned char c_sub)
{
        if (c_sub & 0x80) {
		/* 'C' bit is set (refer to standard) */
                if (c_sub == 0xFF)
                        return -1;
                else
                        return 1 << (c_sub & ~0x80 & 0x1F);
        } else {
		/* 'C' bit in SubClass component not set */
                return c_sub;
	}
}

static int parse_iax2_ies(struct iax2_ies *ies, unsigned char *data, int datalen)
{
	int len = 0, ie = 0, odlen = datalen, pass=1;
	CStdString logmsg;

	memset(ies, 0, (int)sizeof(struct iax2_ies));
	while(datalen >= 2) {
		ie = data[0];
		len = data[1];

		//logmsg.Format("Looking up IE %d (len=%d)", ie, len);
                //LOG4CXX_INFO(s_packetLog, logmsg);

		if (len > datalen - 2) {
			/* Strange.  The quoted length of the IE is past the actual
			 * bounds of the IEs size */
			logmsg.Format("Error parsing IEs Pass=%d Length of IE=%d, "
					"datalen-2=%d, IE=%d, OrigDlen=%d", pass, len, datalen-2, ie, odlen); 
			LOG4CXX_INFO(s_packetLog, logmsg);
			return -1;
		}

		switch(ie) {
		case IAX2_IE_CALLING_NAME:
			ies->calling_name = (char *)data + 2;
			break;
		case IAX2_IE_CALLED_NUMBER:
			ies->callee = (char *)data + 2;
			break;
		case IAX2_IE_CALLING_NUMBER:
			ies->caller = (char *)data + 2;
                        break;
		case IAX2_IE_FORMAT:
			if(len == (int)sizeof(unsigned int))
				ies->format = ntohl(get_unaligned_uint32(data+2));
			else
				ies->format = 0; /* Invalid */
			break;
		case IAX2_IE_USERNAME:
			ies->username = (char *)data + 2;
                        break;
                case IAX2_IE_AUTHMETHODS:
			if(len == (int)sizeof(unsigned int))
                                ies->authmethods = ntohl(get_unaligned_uint32(data+2));
                        else
                                ies->authmethods = 0; /* Invalid */
                        break;
                case IAX2_IE_CHALLENGE:
                        ies->challenge = (char *)data + 2;
                        break;
		default:
			/* Ignore the rest */
			break;
		}

#if 0 /* Debug headaches caused by udpHeader->len */
		char tmpt[256];
		memset(tmpt, 0, sizeof(tmpt));
		memcpy(tmpt, data+2, len);
                logmsg.Format("Got %s", tmpt);
		LOG4CXX_INFO(s_packetLog, logmsg);
#endif

		data[0] = 0;
		datalen -= (len + 2);
		data += (len + 2);
		pass++;
	}

	*data = '\0';
	if(datalen) {
		/* IE contents likely to be invalid because we should have totally
		 * consumed the entire amount of data */
                CStdString logmsg;

                logmsg.Format("Error parsing IEs. datalen left=%d", len, datalen, ie);
                LOG4CXX_INFO(s_packetLog, logmsg);

		return -1;
	}

	return 0;
}

void iax2_dump_frame(struct Iax2FullHeader *fh, char *source, char *dest)
{
        const char *frames[] = {
                "(0?)",
                "DTMF   ",
                "VOICE  ",
                "VIDEO  ",
                "CONTROL",
                "NULL   ",
                "IAX    ",
                "TEXT   ",
                "IMAGE  ",
                "HTML   ",
                "CNG    " };
        const char *iaxs[] = {
                "(0?)",
                "NEW    ",
                "PING   ",
                "PONG   ",
                "ACK    ",
                "HANGUP ",
                "REJECT ",
                "ACCEPT ",
                "AUTHREQ",
                "AUTHREP",
                "INVAL  ",
                "LAGRQ  ",
                "LAGRP  ",
                "REGREQ ",
                "REGAUTH",
                "REGACK ",
                "REGREJ ",
                "REGREL ",
                "VNAK   ",
                "DPREQ  ",
                "DPREP  ",
                "DIAL   ",
                "TXREQ  ",
                "TXCNT  ",
                "TXACC  ",
                "TXREADY",
                "TXREL  ",
                "TXREJ  ",
                "QUELCH ",
                "UNQULCH",
                "POKE   ",
                "PAGE   ",
                "MWI    ",
                "UNSPRTD",
                "TRANSFR",
                "PROVISN",
                "FWDWNLD",
                "FWDATA "
        };
        const char *cmds[] = {
                "(0?)",
                "HANGUP ",
                "RING   ",
                "RINGING",
                "ANSWER ",
                "BUSY   ",
                "TKOFFHK",
                "OFFHOOK" };
        char class2[20];
        char subclass2[20];
	CStdString tmp;
        const char *cclass;
        const char *subclass;
        //char *dir;

        if (fh->type >= (int)sizeof(frames)/(int)sizeof(frames[0])) {
                snprintf(class2, sizeof(class2), "(%d?)", fh->type);
                cclass = class2;
        } else {
                cclass = frames[(int)fh->type];
        }

        if (fh->type == IAX2_FRAME_IAX) {
                if (fh->c_sub >= (int)sizeof(iaxs)/(int)sizeof(iaxs[0])) {
                        snprintf(subclass2, sizeof(subclass2), "(%d?)", fh->c_sub);
                        subclass = subclass2;
                } else {
                        subclass = iaxs[(int)fh->c_sub];
                }
        } else if (fh->type == IAX2_FRAME_CONTROL) {
                if (fh->c_sub >= (int)sizeof(cmds)/(int)sizeof(cmds[0])) {
                        snprintf(subclass2, sizeof(subclass2), "(%d?)", fh->c_sub);
                        subclass = subclass2;
                } else {
                        subclass = cmds[(int)fh->c_sub];
                }
        } else {
                snprintf(subclass2, sizeof(subclass2), "%d", fh->c_sub);
                subclass = subclass2;
        }

        tmp.Format("IAX2-Frame -- OSeqno: %3.3d ISeqno: %3.3d Type: %s Subclass: %s",
                 fh->oseqno, fh->iseqno, cclass, subclass);
	LOG4CXX_INFO(s_packetLog, tmp);
	tmp.Format("   Timestamp: %05lums  SCall: %5.5d  DCall: %5.5d [Source: %s Dest: %s]",
			(unsigned long)ntohl(fh->ts),
			ntohs(fh->scallno) & ~0x8000, ntohs(fh->dcallno) & ~0x8000, source, dest);

	LOG4CXX_INFO(s_packetLog, tmp);
}

/* See if this is an IAX2 NEW.  If so, process */
bool TryIax2New(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
		UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
	struct iax2_ies ies;
	int ies_len = 0, udp_act_payload_len = 0;
	Iax2NewInfoRef info(new Iax2NewInfo());
	//char source_ip[16], dest_ip[16];
	CStdString logmsg;

	if(!DLLCONFIG.m_iax2Support)
		return false;

	memset(&ies, 0, sizeof(ies));
	udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
	if(udp_act_payload_len < (int)sizeof(*fh))
		return false; /* Frame too small */

        if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

	ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

#if 0  /* Debug headaches caused by udpHeader->len */
	/* Beware that udpHeader->len is not the length of the udpPayload
	 * but rather this includes the length of the UDP header as well.
	 * I.e watch out for the figure "8" as you debug ;-) */
	{
		char source_ip[16], dest_ip[16];

		ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_src, source_ip, sizeof(source_ip));
		ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_dest, dest_ip, sizeof(dest_ip));
	        iax2_dump_frame(fh, source_ip, dest_ip);
	}

        logmsg.Format("UDP_Payload=%p UDP+FH_Payload=%p FH->IEDATA=%p ies_len=%d "
                        "udpHeader->len-sizeof(fullhdr)=%d (ntohs(udpHeader->len)"
                        "-sizeof(UdpHeaderStruct))=%d", udpPayload, udpPayload+
                        sizeof(*fh), fh->ie_data, ies_len, (ntohs(udpHeader->len)-
                        sizeof(*fh)), (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct)));
        LOG4CXX_INFO(s_packetLog, logmsg);
#endif

	if(fh->type != IAX2_FRAME_IAX)
		return false; /* Frame type must be IAX */

	if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_NEW)
		return false; /* Subclass must be NEW */

	if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
		return false; /* Invalid "full" frame received */

	if(!ies.callee)
		return false; /* According to the SPEC, a NEW MUST have a
			       * callee (Called Number) */

	if(!strlen(ies.callee))
		return false; /* According to the SPEC, a NEW MUST have a
                               * callee (Called Number) */

	if(!ies.caller) {
		ies.caller = (char*)"WITHELD";
	} else {
		if(!strlen(ies.caller)) {
			ies.caller = (char*)"WITHELD";
		}
	}

	/* Statistically this is most likely a NEW IAX2 frame. */

	info->m_senderIp = ipHeader->ip_src;
	info->m_receiverIp = ipHeader->ip_dest;
	info->m_caller = CStdString(ies.caller);
	info->m_callee = CStdString(ies.callee);
	info->m_callingName = CStdString(ies.calling_name ? ies.calling_name : "");
	info->m_callNo = IntToString(ntohs(fh->scallno) & ~0x8000);

	/* Report the packet */
	Iax2SessionsSingleton::instance()->ReportIax2New(info);

	LOG4CXX_INFO(s_packetLog, "Processed IAX2 NEW frame ts:" + IntToString(ntohl(fh->ts)));

	return true;
}

bool TryIax2Accept(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        struct iax2_ies ies;
        int ies_len = 0, udp_act_payload_len = 0;
        Iax2AcceptInfoRef info(new Iax2AcceptInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        memset(&ies, 0, sizeof(ies));
        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

        if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_IAX)
                return false; /* Frame type must be IAX */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_ACCEPT)
                return false; /* Subclass must be ACCEPT */

        ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

	/* In this case, this just serves to test the integrity of the
	 * Information Elements */
        if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
                return false; /* Invalid "full" frame received */

	if(!ies.format)
		return false; /* According to the SPEC, ACCEPT must have
			       * a format specified */

	/* We have an ACCEPT */

        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
	info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
	info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        Iax2SessionsSingleton::instance()->ReportIax2Accept(info);

	LOG4CXX_INFO(s_packetLog, "Processed IAX2 ACCEPT frame ts:" + IntToString(ntohl(fh->ts)));

        return true;
}

bool TryIax2Authreq(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        struct iax2_ies ies;
        int ies_len = 0, udp_act_payload_len = 0;
	Iax2AuthreqInfoRef info(new Iax2AuthreqInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        memset(&ies, 0, sizeof(ies));
        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

	if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_IAX)
                return false; /* Frame type must be IAX */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_AUTHREQ)
                return false; /* Subclass must be AUTHREQ */

	ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

        /* In this case, this just serves to test the integrity of the
         * Information Elements */
        if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
                return false; /* Invalid "full" frame received */


	if(!ies.username)
		return false;  /* According to the spec AUTHREQ must have
			        * a user name.  Can it be empty? */

	/*
	if(!strlen(ies.username))
		return false;  * According to the spec AUTHREQ must have
                               * a user name. *
	*/

	if(!ies.authmethods)
		return false;  /* According to the spec AUTHREQ must have
                                * AUTHMETHODS */

	if(!ies.challenge)
                return false;  /* According to the spec, AUTHREQ must have
			        * a CHALLENGE string.  Can it be empty? */


	/* We have an AUTHREQ */
        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
        info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
        info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        /* Report the packet */
        Iax2SessionsSingleton::instance()->ReportIax2Authreq(info);

	LOG4CXX_INFO(s_packetLog, "Processed IAX2 AUTHREQ frame");

        return true;
}

/* HANGUP via IAX frame */
bool TryIax2Hangup(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        struct iax2_ies ies;
        int ies_len = 0, udp_act_payload_len = 0;
	Iax2HangupInfoRef info(new Iax2HangupInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        memset(&ies, 0, sizeof(ies));
        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

	if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_IAX)
                return false; /* Frame type must be IAX */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_HANGUP)
                return false; /* Subclass must be HANGUP */

        ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

        /* In this case, this just serves to test the integrity of the
         * Information Elements */
        if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
                return false; /* Invalid "full" frame received */

	/* We have a HANGUP */

        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
        info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
        info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        /* Report the packet */
        Iax2SessionsSingleton::instance()->ReportIax2Hangup(info);

	CStdString logMsg;
	info->ToString(logMsg);
        LOG4CXX_INFO(s_packetLog, "Processed IAX2 HANGUP frame: " + logMsg);

	return true;
}

/* HANGUP via CONTROL frame */
bool TryIax2ControlHangup(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        Iax2HangupInfoRef info(new Iax2HangupInfo());
	int udp_act_payload_len = 0;

        if(!DLLCONFIG.m_iax2Support)
                return false;

        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

	if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_CONTROL)
                return false; /* Frame type must be CONTROL */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_CONTROL_HANGUP)
                return false; /* Subclass must be HANGUP */

        /* We have a HANGUP */

        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
        info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
        info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        /* Report the packet */
        Iax2SessionsSingleton::instance()->ReportIax2Hangup(info);

        LOG4CXX_INFO(s_packetLog, "Processed IAX2 CONTROL HANGUP frame");

        return true;
}

bool TryIax2Reject(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        struct iax2_ies ies;
        int ies_len = 0, udp_act_payload_len = 0;
	Iax2HangupInfoRef info(new Iax2HangupInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        memset(&ies, 0, sizeof(ies));
        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

	if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_IAX)
                return false; /* Frame type must be IAX */

        if(get_uncompressed_subclass(fh->c_sub) != IAX2_COMMAND_REJECT)
                return false; /* Subclass must be REJECT */

	ies_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));

        /* In this case, this just serves to test the integrity of the
         * Information Elements */
        if(parse_iax2_ies(&ies, fh->ie_data, ies_len))
                return false; /* Invalid "full" frame received */

        /* We have a REJECT */

        info->m_senderIp = ipHeader->ip_src;
        info->m_receiverIp = ipHeader->ip_dest;
        info->m_sender_callno = IntToString(ntohs(fh->scallno) & ~0x8000);
        info->m_receiver_callno = IntToString(ntohs(fh->dcallno) & ~0x8000);

        /* Report the packet */
        Iax2SessionsSingleton::instance()->ReportIax2Hangup(info);

        LOG4CXX_INFO(s_packetLog, "Processed IAX2 REJECT frame");

        return true;
}

bool TryIax2FullVoiceFrame(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
        struct Iax2FullHeader *fh = (struct Iax2FullHeader *)udpPayload;
        int data_len = 0, codec = 0, pt = 0, udp_act_payload_len = 0;
	Iax2PacketInfoRef info(new Iax2PacketInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*fh))
                return false; /* Frame too small */

        if(!(ntohs(fh->scallno) & 0x8000))
                return false; /* Not a full frame */

        if(fh->type != IAX2_FRAME_VOICE)
                return false; /* Frame type must be VOICE */

	codec = get_uncompressed_subclass(fh->c_sub);
	if((pt = iax2_codec_to_rtp_payloadtype(codec)) < 0) {
		CStdString logmsg;

		logmsg.Format("Invalid payload type %d received for "
				"IAX_FRAME_VOICE, IAX2 codec %d", pt, codec);
		LOG4CXX_INFO(s_packetLog, logmsg);
		return false; /* Invalid codec type received */
	}

	data_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*fh));
	if(data_len == 0)
		return false; /* Empty packet? */

        /* We have a full VOICE frame */

	info->m_sourceIp = ipHeader->ip_src;
	info->m_destIp = ipHeader->ip_dest;
	info->m_sourcecallno = (ntohs(fh->scallno) & ~0x8000);
	info->m_destcallno = (ntohs(fh->dcallno) & ~0x8000);
	info->m_payloadSize = data_len;
	info->m_payload = udpPayload+sizeof(*fh);
	info->m_payloadType = pt;
	info->m_timestamp = ntohl(fh->ts);
	info->m_arrivalTimestamp = time(NULL);
	info->m_frame_type = IAX2_FRAME_FULL;

	Iax2SessionsSingleton::instance()->ReportIax2Packet(info);

	CStdString logmsg, packetInfo;
	info->ToString(packetInfo);
	logmsg.Format("Processed IAX2 FULL VOICE frame pt:%d", pt);
        LOG4CXX_INFO(s_packetLog, logmsg);
	if(s_packetLog->isDebugEnabled())
	{
		LOG4CXX_DEBUG(s_packetLog, packetInfo);
	}

	return true;
}

bool TryIax2MetaTrunkFrame(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	struct Iax2MetaTrunkHeader *mh = (struct Iax2MetaTrunkHeader *)udpPayload;
	struct Iax2MetaTrunkEntry *supermini = NULL;
	struct Iax2MetaTrunkEntryTs *mini = NULL;
	int content_type = 0; /* 0 means mini frames, 1 means super mini (no timestampes) */
	unsigned int data_len = 0;
	int entries = 0, udp_act_payload_len = 0;
	Iax2PacketInfoRef info(new Iax2PacketInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*mh))
                return false; /* Frame too small */

	if(mh->meta != 0)
		return false; /* Must be zero */

	if(mh->metacmd & 0x80)
		return false; /* 'V' bit must be set to zero */

	if(mh->metacmd != 1)
		return false; /* metacmd must be 1 */

	/* Get the length of the information apart from the
	 * meta trunk header */
	data_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*mh));
        if(data_len == 0)
                return false; /* Empty packet? */

	/* Step over the meta trunk header */
	udpPayload += sizeof(*mh);

	/* Determine whether the trunk contents have their own
	 * timestamps or not */
	if(mh->cmddata & 0x01)
		content_type = 1;
	else
		content_type = 0;

	if(content_type) {
		/* Have timestamps */

		while(data_len) {
			if(data_len < sizeof(*mini))
				break;

			mini = (struct Iax2MetaTrunkEntryTs *)udpPayload;

			if(data_len < sizeof(*mini)+ntohs(mini->len))
				break;

			info->m_sourceIp = ipHeader->ip_src;
			info->m_destIp = ipHeader->ip_dest;
			info->m_sourcecallno = (ntohs(mini->mini.scallno) & ~0x8000);
			info->m_destcallno = 0;
			info->m_payloadSize = ntohs(mini->len);
			info->m_payload = udpPayload+sizeof(*mini);
			info->m_payloadType = 0;
			info->m_timestamp = ntohs(mini->mini.ts);
			info->m_arrivalTimestamp = time(NULL);
			info->m_frame_type = IAX2_FRAME_MINI;

			Iax2SessionsSingleton::instance()->ReportIax2Packet(info);
			entries += 1;

			udpPayload += sizeof(*mini)+ntohs(mini->len);
			data_len -= sizeof(*mini)+ntohs(mini->len);
		}
	} else {
		/* Have no timestamps */
		while(data_len) {
			if(data_len < sizeof(*supermini))
				break;

			supermini = (struct Iax2MetaTrunkEntry *)udpPayload;

                        if(data_len < sizeof(*supermini)+ntohs(supermini->len))
				break;

			info->m_sourceIp = ipHeader->ip_src;
                        info->m_destIp = ipHeader->ip_dest;
                        info->m_sourcecallno = (ntohs(supermini->scallno) & ~0x8000);
                        info->m_destcallno = 0;
                        info->m_payloadSize = ntohs(supermini->len);
                        info->m_payload = udpPayload+sizeof(*supermini);
                        info->m_payloadType = 0;
                        info->m_timestamp = 0;
                        info->m_arrivalTimestamp = time(NULL);
                        info->m_frame_type = IAX2_FRAME_MINI;

                        Iax2SessionsSingleton::instance()->ReportIax2Packet(info);
			entries += 1;

                        udpPayload += sizeof(*supermini)+ntohs(supermini->len);
                        data_len -= sizeof(*supermini)+ntohs(supermini->len);
		}
	}


	if(entries > 0) {
		CStdString logmsg;

		logmsg.Format("Processed IAX2 Meta Trunk packet with %d entries", entries);
	        LOG4CXX_DEBUG(s_packetLog, logmsg);
		return true;
	}

	return false; /* No valid entries in this so-called meta trunk frame */
}

bool TryIax2MiniVoiceFrame(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,
                UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	struct Iax2MiniHeader *mini = (struct Iax2MiniHeader *)udpPayload;
        int data_len = 0, udp_act_payload_len = 0;
	Iax2PacketInfoRef info(new Iax2PacketInfo());

        if(!DLLCONFIG.m_iax2Support)
                return false;

        udp_act_payload_len = (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct));
        if(udp_act_payload_len < (int)sizeof(*mini))
                return false; /* Frame too small */

        if((ntohs(mini->scallno) & 0x8000))
                return false; /* Not a Mini frame */

	data_len = ((u_char*)ipHeader+ntohs(ipHeader->ip_len))-(udpPayload+sizeof(*mini));
        if(data_len == 0)
                return false; /* Empty packet? */

        info->m_sourceIp = ipHeader->ip_src;
        info->m_destIp = ipHeader->ip_dest;
        info->m_sourcecallno = (ntohs(mini->scallno) & ~0x8000);
        info->m_destcallno = 0;
        info->m_payloadSize = data_len;
        info->m_payload = udpPayload+sizeof(*mini);
        info->m_payloadType = 0;
	info->m_timestamp = ntohs(mini->ts);
        info->m_arrivalTimestamp = time(NULL);
        info->m_frame_type = IAX2_FRAME_MINI;

	CStdString logMsg, packetInfo;
	info->ToString(packetInfo);
	logMsg.Format("IAX2 mini frame: %s", packetInfo);
	LOG4CXX_DEBUG(s_packetLog, logMsg);

        return Iax2SessionsSingleton::instance()->ReportIax2Packet(info);
}

bool TryRtcp(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	RtcpCommonHeaderStruct* rtcpHeader = (RtcpCommonHeaderStruct*)udpPayload;
	RtcpCommonHeaderStruct* r = NULL;
	RtcpCommonHeaderStruct* rtcpEnd = NULL;
	RtcpCommonHeaderStruct* rtcpThisPktEnd = NULL;
	CStdString logMsg;

	if(!DLLCONFIG.m_rtcpDetect)
	{
		return false;
	}

	if((ntohs(udpHeader->len)-sizeof(UdpHeaderStruct)) < sizeof(RtcpCommonHeaderStruct))
	{
		// Packet too small
		return false;
	}

	unsigned short version = (rtcpHeader->vpc & 0x00c0) >> 6;
	unsigned short p = (rtcpHeader->vpc & 0x0020) >> 5;
	unsigned short count = (rtcpHeader->vpc & 0x001f);

	if(version != 2)
	{
		// Failed first header validity check in RFC1889 A.2
		return false;
	}

	if(rtcpHeader->pt != 200 && rtcpHeader->pt != 201)
	{
		// Failed second header validity check in RFC1889 A.2
		return false;
	}

	if(p != 0)
	{
		// Failed third header validity check in RFC1889 A.2
		return false;
	}

	rtcpEnd = (RtcpCommonHeaderStruct*)((char*)udpPayload + (ntohs(udpHeader->len)-sizeof(UdpHeaderStruct)));
	r = rtcpHeader;
	unsigned short mv = 0;

	r = (RtcpCommonHeaderStruct*)((unsigned int *)r + ntohs(r->length) + 1);
	while(r < rtcpEnd && ((rtcpEnd - r) >= (int)sizeof(RtcpCommonHeaderStruct)))
	{
		mv = (r->vpc & 0x00c0) >> 6;
		if(mv != 2)
		{
			break;
		}
		r = (RtcpCommonHeaderStruct*)((unsigned int *)r + ntohs(r->length) + 1);
	}

	if(r != rtcpEnd)
	{
		// Failed final header validity check in RFC1889 A.2
		return false;
	}

	char sourceIp[16], destIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_src, sourceIp, sizeof(sourceIp));
	ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_dest, destIp, sizeof(destIp));

	// As per RFC we should be fairly sure we have an RTCP packet and
	// henceforth our return value will be true

	// Now let's see whether we can obtain an SDES packet
	char cname[256];
	RtcpSdesCsrcItem *csrcItem = NULL;
	r = rtcpHeader;

	memset(cname, 0, sizeof(cname));

	r = (RtcpCommonHeaderStruct*)((unsigned int *)r + ntohs(r->length) + 1);
	while(r < rtcpEnd && ((rtcpEnd - r) >= (int)sizeof(RtcpCommonHeaderStruct)))
	{
		version = (r->vpc & 0x00c0) >> 6;
		p = (r->vpc & 0x0020) >> 5;
		count = (r->vpc & 0x001f);

		rtcpThisPktEnd = (RtcpCommonHeaderStruct*)((unsigned int *)r + ntohs(r->length) + 1);

		if(r->pt == 202 && count)
		{
			// Check if we have CNAME in the first CSRC
			csrcItem = (RtcpSdesCsrcItem *)((unsigned int *)r + 2);

			while((csrcItem < (RtcpSdesCsrcItem *)rtcpThisPktEnd) && (csrcItem->type != 1) && (csrcItem->type != 0))
			{
				csrcItem = (RtcpSdesCsrcItem *)((char*)csrcItem + (int)csrcItem->length);
			}

			if(csrcItem < (RtcpSdesCsrcItem *)rtcpThisPktEnd && csrcItem->type == 1)
			{
				break;
			}

			csrcItem = NULL;
		}

		r = (RtcpCommonHeaderStruct*)((unsigned int *)r + ntohs(r->length) + 1);
	}

	if(csrcItem == NULL)
	{
		// No CNAME
		return true;
	}

	RtcpSrcDescriptionPacketInfoRef info(new RtcpSrcDescriptionPacketInfo());

	info->m_sourceIp = ipHeader->ip_src;
	info->m_destIp =  ipHeader->ip_dest;
	info->m_sourcePort = ntohs(udpHeader->source);
	info->m_destPort = ntohs(udpHeader->dest);

	memcpy(cname, csrcItem->data, ((csrcItem->length > 254) ? 254 : csrcItem->length));

	if(csrcItem->length == 0 || ACE_OS::strncasecmp(cname, "ext", ((3 > csrcItem->length) ? csrcItem->length : 3)))
	{
		if(DLLCONFIG.m_inInMode == false)
		{
			// Not an extension
			return true;
		}
		else
		{
			if(csrcItem->length == 0)
			{
				return true;
			}
		}
	}

	info->m_fullCname = cname;

	/*
	 * Now parse the CNAME. As per RFC1889, 6.4.1, the CNAME is either
	 * in the format "user@host" or "host".  However we will also support
	 * "user@host:port" or "host:port"
	 */
	char *x = NULL, *y = NULL, *z = NULL;

	x = cname;
	y = ACE_OS::strchr(cname, '@');
	if(!y)
	{
		// CNAME is in the "host" or "host:port" format only, no user
		y = ACE_OS::strchr(cname, ':');
		if(!y)
		{
			// We have no port
			GrabToken(cname, cname+strlen(cname), info->m_cnameDomain);
		}
		else
		{
			*y++ = '\0';
			GrabToken(x, x+strlen(x), info->m_cnameDomain);
			if(*y)
			{
				GrabToken(y, y+strlen(y), info->m_cnamePort);
			}
		}
	}
	else
	{
		*y++ = '\0';
		GrabToken(x, x+strlen(x), info->m_cnameUsername);
		if(*y)
		{
			z = ACE_OS::strchr(y, ':');
			if(!z)
			{
				// We have no port
				GrabToken(y, y+strlen(y), info->m_cnameDomain);
			}
			else
			{
				*z++ = '\0';
				GrabToken(y, y+strlen(y), info->m_cnameDomain);
				if(*z)
				{
					GrabToken(z, z+strlen(z), info->m_cnamePort);
				}
			}
		}
	}

	info->ToString(logMsg);
	LOG4CXX_DEBUG(s_rtcpPacketLog, logMsg);

	RtpSessionsSingleton::instance()->ReportRtcpSrcDescription(info);

	return true;
}


bool TryRtp(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	bool result = false;
	RtpHeaderStruct* rtpHeader = (RtpHeaderStruct*)udpPayload;
	std::map<unsigned int, unsigned int>::iterator pair;

	/* Ensure that the UDP payload is at least sizeof(RtpHeaderStruct) */
	if(ntohs(udpHeader->len) < sizeof(RtpHeaderStruct))
		return false;

	if (rtpHeader->version == 2)
	{
		if((!(ntohs(udpHeader->source)%2) && !(ntohs(udpHeader->dest)%2)) || DLLCONFIG.m_rtpDetectOnOddPorts)	// udp ports usually even 
		{
			pair = DLLCONFIG.m_rtpPayloadTypeBlockList.find(rtpHeader->pt);
			if(pair != DLLCONFIG.m_rtpPayloadTypeBlockList.end())
			{
				if(s_rtpPacketLog->isDebugEnabled())
				{
					RtpPacketInfoRef rtpInfo(new RtpPacketInfo());
					u_char* payload = (u_char *)rtpHeader + sizeof(RtpHeaderStruct);
					u_char* packetEnd = (u_char *)ipHeader + ntohs(ipHeader->ip_len);
					u_int payloadLength = packetEnd - payload;
					CStdString logMsg;

					rtpInfo->m_sourceIp = ipHeader->ip_src;
					rtpInfo->m_destIp =  ipHeader->ip_dest;
					rtpInfo->m_sourcePort = ntohs(udpHeader->source);
					rtpInfo->m_destPort = ntohs(udpHeader->dest);
					rtpInfo->m_payloadSize = payloadLength;
					rtpInfo->m_payloadType = rtpHeader->pt;
					rtpInfo->m_seqNum = ntohs(rtpHeader->seq);
					rtpInfo->m_timestamp = ntohl(rtpHeader->ts);
					rtpInfo->m_payload = payload;
					rtpInfo->m_arrivalTimestamp = time(NULL);
					memcpy(rtpInfo->m_sourceMac, ethernetHeader->sourceMac, sizeof(rtpInfo->m_sourceMac));
					memcpy(rtpInfo->m_destMac, ethernetHeader->destinationMac, sizeof(rtpInfo->m_destMac));

					rtpInfo->ToString(logMsg);
					LOG4CXX_DEBUG(s_rtpPacketLog, "Dropped RTP packet with payload type:" + IntToString(rtpHeader->pt) + " " + logMsg);
				}

				return true;
			}

			if((rtpHeader->pt <= 34 &&  rtpHeader->pt != 13) || (rtpHeader->pt >= 97 && rtpHeader->pt < 127) )         
			// pt=13 is CN (Comfort Noise)
			// pt=34 is H263
			// pt=97 is IAX2 iLBC payload
			// pt > 98 is telephone-event in SIP
			{
				if(DLLCONFIG.m_rtpBlockedIpRanges.Matches(ipHeader->ip_src) || DLLCONFIG.m_rtpBlockedIpRanges.Matches(ipHeader->ip_dest))
				{
					if(s_rtpPacketLog->isDebugEnabled())
					{
						CStdString logMsg;
						char sourceIp[16];
						ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_src, sourceIp, sizeof(sourceIp));
						char destIp[16];
						ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_dest, destIp, sizeof(destIp));
						logMsg.Format("RTP packet filtered by rtpBlockedIpRanges: src:%s dst:%s", sourceIp, destIp);
						LOG4CXX_DEBUG(s_rtpPacketLog, logMsg);
					}
				}
				else
				{
					result = true;
					u_char* payload = (u_char *)rtpHeader + sizeof(RtpHeaderStruct);
					u_char* packetEnd = (u_char *)ipHeader + ntohs(ipHeader->ip_len);
					u_int payloadLength = packetEnd - payload;

					RtpPacketInfoRef rtpInfo(new RtpPacketInfo());
					rtpInfo->m_sourceIp = ipHeader->ip_src;
					rtpInfo->m_destIp =  ipHeader->ip_dest;
					rtpInfo->m_sourcePort = ntohs(udpHeader->source);
					rtpInfo->m_destPort = ntohs(udpHeader->dest);
					rtpInfo->m_payloadSize = payloadLength;
					rtpInfo->m_payloadType = rtpHeader->pt;
					rtpInfo->m_seqNum = ntohs(rtpHeader->seq);
					rtpInfo->m_timestamp = ntohl(rtpHeader->ts);
					rtpInfo->m_payload = payload;
					rtpInfo->m_arrivalTimestamp = time(NULL);
					memcpy(rtpInfo->m_sourceMac, ethernetHeader->sourceMac, sizeof(rtpInfo->m_sourceMac));
					memcpy(rtpInfo->m_destMac, ethernetHeader->destinationMac, sizeof(rtpInfo->m_destMac));

					if(s_rtpPacketLog->isDebugEnabled())
					{
						CStdString logMsg;
						rtpInfo->ToString(logMsg);
						LOG4CXX_DEBUG(s_rtpPacketLog, logMsg);
					}
					if(payloadLength < 900)		// sanity check, speech RTP payload should always be smaller
					{
						RtpSessionsSingleton::instance()->ReportRtpPacket(rtpInfo);
					}
				}
			}
			else
			{
				// unsupported CODEC
				if(s_rtpPacketLog->isDebugEnabled())
				{
					CStdString logMsg;
					char sourceIp[16];
					ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_src, sourceIp, sizeof(sourceIp));
					char destIp[16];
					ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_dest, destIp, sizeof(destIp));
					logMsg.Format("Unsupported codec:%x  src:%s dst:%s", rtpHeader->pt, sourceIp, destIp);
					LOG4CXX_DEBUG(s_rtpPacketLog, logMsg);
				}
			}
		}
	}
	return result;
}


bool TrySipBye(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(sipLength < (int)sizeof(SIP_METHOD_BYE) || sipEnd > (char*)packetEnd)
	{
		return false;
	}

	if (memcmp(SIP_METHOD_BYE, (void*)udpPayload, SIP_METHOD_BYE_SIZE) == 0)
	{
		result = true;
		int sipLength = ntohs(udpHeader->len);
		char* sipEnd = (char*)udpPayload + sipLength;
		SipByeInfoRef info(new SipByeInfo());

		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}

		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			GrabSipName(fromField, fromFieldEnd, info->m_fromName);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(sipUser, fromFieldEnd, info->m_fromDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(fromField, fromFieldEnd, info->m_fromDomain);
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			GrabSipName(toField, toFieldEnd, info->m_toName);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(sipUser, toFieldEnd, info->m_toDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(toField, toFieldEnd, info->m_toDomain);
			}
		}

		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		CStdString logMsg;
		info->ToString(logMsg);
		LOG4CXX_INFO(s_sipPacketLog, "BYE: " + logMsg);
		if(callIdField && DLLCONFIG.m_sipIgnoreBye == false)
		{
			RtpSessionsSingleton::instance()->ReportSipBye(info);
		}
	}
	return result;
}

bool TrySipNotify(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(sipLength < (int)sizeof(SIP_METHOD_BYE) || sipEnd > (char*)packetEnd)
	{
		return false;
	}

	if (memcmp(SIP_METHOD_NOTIFY, (void*)udpPayload, SIP_METHOD_NOTIFY_SIZE) == 0)
	{
		result = true;
		int sipLength = ntohs(udpHeader->len);
		char* sipEnd = (char*)udpPayload + sipLength;
		SipNotifyInfoRef info(new SipNotifyInfo());
		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
		}

		char* dspField = memFindAfter(SIP_FIELD_LINE3, (char*)udpPayload, sipEnd);
		if(!dspField)
		{
			dspField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}
		if(dspField)
		{
			GrabTokenSkipLeadingWhitespaces(dspField, sipEnd, info->m_dsp);
		
			CStdString logMsg;
			LOG4CXX_INFO(s_sipPacketLog, "NOTIFY: " + logMsg);
			if(callIdField && DLLCONFIG.m_sipNotifySupport == true)
			{
				RtpSessionsSingleton::instance()->ReportSipNotify(info);
			}
		}
	}
	return result;
}

bool TrySipInvite(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd);
bool TrySipBye(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd);
bool TrySipNotify(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd);

bool TryLogFailedSip(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	if(DLLCONFIG.m_sipLogFailedCalls == false)
	{
		return false;
	}

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	CStdString callId, errorCode, logMsg, errorString;

	if(sipLength < 9 || sipEnd > (char*)packetEnd)
	{
		return false;
	}

	if((memcmp("SIP/2.0 4", (void*)udpPayload, 9) == 0) ||
	   (memcmp("SIP/2.0 5", (void*)udpPayload, 9) == 0) ||
	   (memcmp("SIP/2.0 6", (void*)udpPayload, 9) == 0) ||
	   (memcmp("CANCEL ", (void*)udpPayload, 7) == 0))
	{
		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		char* eCode = memFindAfter("SIP/2.0 ", (char*)udpPayload, sipEnd);

		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, callId);
		}

		if((memcmp("CANCEL ", (void*)udpPayload, 7) == 0))
		{
			errorCode.Format("CANCEL");
			errorString.Format("User Agent CANCEL");
		}
		else
		{
			if(eCode)
			{
				GrabTokenSkipLeadingWhitespaces(eCode, sipEnd, errorCode);
				GrabLine((eCode+errorCode.size()+1), sipEnd, errorString);
			}
		}
	}

	if(!(callId.size() && errorCode.size()))
	{
		return false;
	}

	SipFailureMessageInfoRef info(new SipFailureMessageInfo());
	info->m_senderIp = ipHeader->ip_src;
	info->m_receiverIp = ipHeader->ip_dest;
	memcpy(info->m_senderMac, ethernetHeader->sourceMac, sizeof(info->m_senderMac));
	memcpy(info->m_receiverMac, ethernetHeader->destinationMac, sizeof(info->m_receiverMac));
	info->m_callId = callId;
	info->m_errorCode = errorCode;
	info->m_errorString = errorString;

	// Logging is done in RtpSessions.cpp
	//CStdString sipError;

	//info->ToString(sipError);
	//LOG4CXX_INFO(s_sipPacketLog, "SIP Error packet: " + sipError);

	RtpSessionsSingleton::instance()->ReportSipErrorPacket(info);

	return true;
}

static bool SipByeTcpToUdp(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,TcpHeaderStruct* tcpHeader, u_char *pBuffer, int bLength)
{
        UdpHeaderStruct udpHeader;

        udpHeader.source = tcpHeader->source;
        udpHeader.dest = tcpHeader->dest;
        udpHeader.len = htons(bLength+sizeof(UdpHeaderStruct));

        return TrySipBye(ethernetHeader, ipHeader, &udpHeader, pBuffer, pBuffer+bLength);
}

static bool SipInviteTcpToUdp(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader, u_char *pBuffer, int bLength)
{
	UdpHeaderStruct udpHeader;

	udpHeader.source = tcpHeader->source;
	udpHeader.dest = tcpHeader->dest;
	udpHeader.len = htons(bLength+sizeof(UdpHeaderStruct));

	return TrySipInvite(ethernetHeader, ipHeader, &udpHeader, pBuffer, pBuffer+bLength);
}

static bool SipFailedTcpToUdp(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader, u_char *pBuffer, int bLength)
{
	UdpHeaderStruct udpHeader;

        udpHeader.source = tcpHeader->source;
        udpHeader.dest = tcpHeader->dest;
        udpHeader.len = htons(bLength+sizeof(UdpHeaderStruct));

        return TryLogFailedSip(ethernetHeader, ipHeader, &udpHeader, pBuffer, pBuffer+bLength);
}

bool TrySipTcp(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	int tcpLengthPayloadLength = 0;
	bool result = false;
	std::list<SipTcpStreamRef> toErase;
	u_char* startTcpPayload;

	// Not checking typical SIP port numbers anymore. This is not done on UDP either.
	//if(ntohs(tcpHeader->source) != 5060 && ntohs(tcpHeader->dest) != 5060 &&
	//	ntohs(tcpHeader->source) != 8060 && ntohs(tcpHeader->dest) != 8060    )	 
	//{
		// interactive intelligence sometimes sends SIP traffic on port 8060
	//	return false;
	//}

	startTcpPayload = (u_char*)tcpHeader + (tcpHeader->off * 4);
	tcpLengthPayloadLength = ((u_char*)ipHeader+ntohs(ipHeader->ip_len)) - startTcpPayload;

    if(tcpLengthPayloadLength < SIP_METHOD_INVITE_SIZE+3)
    {
		LOG4CXX_DEBUG(s_sipTcpPacketLog, "Payload shorter");
		return false;
    }

	if((memcmp(SIP_METHOD_INVITE, (void*)startTcpPayload, SIP_METHOD_INVITE_SIZE) == 0) ||
	   (memcmp(SIP_METHOD_ACK, (void*)startTcpPayload, SIP_METHOD_ACK_SIZE) == 0) ||
	   (memcmp(SIP_METHOD_BYE, (void*)startTcpPayload, SIP_METHOD_BYE_SIZE) == 0) ||
	   (memcmp("SIP/2.0 4", (void*)startTcpPayload, 9) == 0) ||
	   (memcmp("SIP/2.0 5", (void*)startTcpPayload, 9) == 0) ||
	   (memcmp("SIP/2.0 6", (void*)startTcpPayload, 9) == 0) ||
	   (memcmp("CANCEL ", (void*)startTcpPayload, 7) == 0))
	{
		SipTcpStreamRef tcpstream(new SipTcpStream());
		int exists = 0;

		tcpstream->m_senderIp = ipHeader->ip_src;
		tcpstream->m_receiverIp = ipHeader->ip_dest;
		tcpstream->m_senderPort = ntohs(tcpHeader->source);
		tcpstream->m_receiverPort = ntohs(tcpHeader->dest);
		tcpstream->m_expectingSeqNo = ntohl(tcpHeader->seq)+tcpLengthPayloadLength;
		tcpstream->m_lastSeqNo = ntohl(tcpHeader->seq);
		tcpstream->AddTcpPacket(startTcpPayload, tcpLengthPayloadLength);

		// Ensure this is not a duplicate
	        for(std::list<SipTcpStreamRef>::iterator it = s_SipTcpStreams.begin(); it != s_SipTcpStreams.end(); it++)
        	{
               		SipTcpStreamRef tcpStreamList = *it;

	                if(((unsigned int)(tcpstream->m_senderIp.s_addr) == (unsigned int)(tcpStreamList->m_senderIp.s_addr)) &&
        	           ((unsigned int)(tcpstream->m_receiverIp.s_addr) == (unsigned int)(tcpStreamList->m_receiverIp.s_addr)) &&
                	   (tcpstream->m_senderPort == tcpStreamList->m_senderPort) &&
	                   (tcpstream->m_receiverPort == tcpStreamList->m_receiverPort) &&
        	           (tcpstream->m_expectingSeqNo == tcpStreamList->m_expectingSeqNo) &&
			   (tcpstream->m_lastSeqNo == tcpStreamList->m_lastSeqNo))
                	{
				exists = 1;
				break;
			}
		}

		if(exists == 1) {
			CStdString logMsg;

			logMsg.Format("Dropped duplicate TCP packet");
			LOG4CXX_INFO(s_sipTcpPacketLog, logMsg);
			return true;
		}

		if(tcpstream->SipRequestIsComplete()) {
			/* Hmm.. Lucky us */
			SafeBufferRef buffer = tcpstream->GetCompleteSipRequest();
			CStdString tcpStream;

			tcpstream->ToString(tcpStream);
			LOG4CXX_DEBUG(s_sipTcpPacketLog, "Obtained complete TCP Stream: " + tcpStream);

			bool usefulPacket = false;

			usefulPacket = SipInviteTcpToUdp(ethernetHeader, ipHeader, tcpHeader, buffer->GetBuffer(), buffer->Size());

			if(!usefulPacket)
			{
				usefulPacket = SipByeTcpToUdp(ethernetHeader, ipHeader, tcpHeader, buffer->GetBuffer(), buffer->Size());
			}

			if(!usefulPacket)
			{
				usefulPacket = SipFailedTcpToUdp(ethernetHeader, ipHeader, tcpHeader, buffer->GetBuffer(), buffer->Size());
			}

			return usefulPacket;
		}

		s_SipTcpStreams.push_back(tcpstream);

		CStdString tcpStream;
		
                tcpstream->ToString(tcpStream);
		LOG4CXX_INFO(s_sipTcpPacketLog, "Obtained incomplete TCP Stream: " + tcpStream);

		return true;
	}

	for(std::list<SipTcpStreamRef>::iterator it = s_SipTcpStreams.begin(); it != s_SipTcpStreams.end(); it++)
	{
		SipTcpStreamRef tcpstream = *it;
		int found = 0;

		if(((unsigned int)(tcpstream->m_senderIp.s_addr) == (unsigned int)(ipHeader->ip_src.s_addr)) &&
		   ((unsigned int)(tcpstream->m_receiverIp.s_addr) == (unsigned int)(ipHeader->ip_dest.s_addr)) &&
		   (tcpstream->m_senderPort == ntohs(tcpHeader->source)) &&
		   (tcpstream->m_receiverPort == ntohs(tcpHeader->dest)) &&
		   (tcpstream->m_expectingSeqNo == ntohl(tcpHeader->seq)) && !found)
		{
			result = true;
			found = 1;

			tcpstream->AddTcpPacket(startTcpPayload, tcpLengthPayloadLength);

			if(tcpstream->SipRequestIsComplete()) {
				SafeBufferRef buffer = tcpstream->GetCompleteSipRequest();
	                        CStdString tcpStream;
	
        	                tcpstream->ToString(tcpStream);
                	        LOG4CXX_INFO(s_sipTcpPacketLog, "TCP Stream updated to completion: " + tcpStream);

				bool usefulPacket = false;

				usefulPacket = SipInviteTcpToUdp(ethernetHeader, ipHeader, tcpHeader, buffer->GetBuffer(), buffer->Size());

				if(!usefulPacket)
				{
					usefulPacket = SipByeTcpToUdp(ethernetHeader, ipHeader, tcpHeader, buffer->GetBuffer(), buffer->Size());
				}

				if(!usefulPacket)
				{
					usefulPacket = SipFailedTcpToUdp(ethernetHeader, ipHeader, tcpHeader, buffer->GetBuffer(), buffer->Size());
				}

				toErase.push_back(tcpstream);
			}
		} else {
			if((time(NULL) - tcpstream->m_entryTime) >= 60)
				toErase.push_back(tcpstream);
		}
	}

	for(std::list<SipTcpStreamRef>::iterator it2 = toErase.begin(); it2 != toErase.end(); it2++)
        {
		SipTcpStreamRef tcpstream = *it2;

		s_SipTcpStreams.remove(tcpstream);
	}

	return result;
}

bool TrySipSessionProgress(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;

	if(sipLength < SIP_RESPONSE_SESSION_PROGRESS_SIZE || sipEnd > (char*)packetEnd)
	{
		;	// packet too short
	}
	else if(memcmp(SIP_RESPONSE_SESSION_PROGRESS, (void*)udpPayload, SIP_RESPONSE_SESSION_PROGRESS_SIZE) == 0)
	{
		bool hasSdp = false;
		SipSessionProgressInfoRef info(new SipSessionProgressInfo());

		result = true;

		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}

		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}

		char* audioField = NULL;
		char* connectionAddressField = NULL;

		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
			audioField = memFindAfter("m=audio ", callIdField, sipEnd);
			connectionAddressField = memFindAfter("c=IN IP4 ", callIdField, sipEnd);
		}
		if(audioField && connectionAddressField)
		{
			hasSdp = true;

			GrabToken(audioField, sipEnd, info->m_mediaPort);

			CStdString connectionAddress;
			GrabToken(connectionAddressField, sipEnd, connectionAddress);
			struct in_addr mediaIp;
			if(connectionAddress.size())
			{
				if(ACE_OS::inet_aton((PCSTR)connectionAddress, &mediaIp))
				{
					info->m_mediaIp = mediaIp;
				}
			}
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
			}
		}
		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		CStdString logMsg;

		info->ToString(logMsg);
		logMsg = "183 Session Progress: " + logMsg;
		if(!hasSdp)
		{
			logMsg = logMsg + " dropped because it lacks the SDP";
		}
		else
		{
			RtpSessionsSingleton::instance()->ReportSipSessionProgress(info);
		}
		LOG4CXX_INFO(s_sipPacketLog, logMsg);
	}
	return result;
}

bool TrySip200Ok(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;

	if(DLLCONFIG.m_sipTreat200OkAsInvite == true)
	{
		return false;
	}

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(sipLength < SIP_RESPONSE_200_OK_SIZE || sipEnd > (char*)packetEnd)
	{
		;	// packet too short
	}
	else if (memcmp(SIP_RESPONSE_200_OK, (void*)udpPayload, SIP_RESPONSE_200_OK_SIZE) == 0)
	{
		result = true;

		Sip200OkInfoRef info(new Sip200OkInfo());

		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}

		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}

		char* audioField = NULL;
		char* connectionAddressField = NULL;

		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
			audioField = memFindAfter("m=audio ", callIdField, sipEnd);
			connectionAddressField = memFindAfter("c=IN IP4 ", callIdField, sipEnd);
		}
		if(audioField && connectionAddressField)
		{
			info->m_hasSdp = true;

			GrabToken(audioField, sipEnd, info->m_mediaPort);

			CStdString connectionAddress;
			GrabToken(connectionAddressField, sipEnd, connectionAddress);
			struct in_addr mediaIp;
			if(connectionAddress.size())
			{
				if(ACE_OS::inet_aton((PCSTR)connectionAddress, &mediaIp))
				{
					info->m_mediaIp = mediaIp;
				}
			}
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
			}
		}
		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		CStdString logMsg;

		info->ToString(logMsg);
		logMsg = "200 OK: " + logMsg;
		if(info->m_hasSdp)
		{
			LOG4CXX_INFO(s_sipPacketLog, logMsg);
		}
		else
		{
			LOG4CXX_DEBUG(s_sipPacketLog, logMsg);
		}

		RtpSessionsSingleton::instance()->ReportSip200Ok(info);
	}
	return result;
}

bool TrySip302MovedTemporarily(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;
	bool drop = false;

	if(DLLCONFIG.m_sip302MovedTemporarilySupport == false)
	{
		return false;
	}

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	CStdString sipMethod = "302 Moved Temporarily";

	if(sipLength < SIP_RESPONSE_302_MOVED_TEMPORARILY_SIZE || sipEnd > (char*)packetEnd)
	{
		drop = true;	// packet too short
	}
	else if(memcmp(SIP_RESPONSE_302_MOVED_TEMPORARILY, (void*)udpPayload, SIP_RESPONSE_302_MOVED_TEMPORARILY_SIZE) != 0)
	{
		drop = true;
	}

	if(drop == false)
	{
		result = true;

		Sip302MovedTemporarilyInfoRef info(new Sip302MovedTemporarilyInfo());

		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}
		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}
		char* contactField = memFindAfter("Contact:", (char*)udpPayload, sipEnd);
		if(!contactField)
		{
			contactField = memFindAfter("\nc:", (char*)udpPayload, sipEnd);
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			GrabSipName(fromField, fromFieldEnd, info->m_fromName);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(sipUser, fromFieldEnd, info->m_fromDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(fromField, fromFieldEnd, info->m_fromDomain);
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			GrabSipName(toField, toFieldEnd, info->m_toName);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(sipUser, toFieldEnd, info->m_toDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(toField, toFieldEnd, info->m_toDomain);
			}
		}
		if(contactField)
		{
			CStdString contact;
			char* contactFieldEnd = GrabLine(contactField, sipEnd, contact);
			LOG4CXX_DEBUG(s_sipExtractionLog, "contact: " + contact);

			GrabSipName(contactField, contactFieldEnd, info->m_contactName);

			char* sipUser = memFindAfter("sip:", contactField, contactFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, contactFieldEnd, info->m_contact);
				}
				else
				{
					GrabSipUriUser(sipUser, contactFieldEnd, info->m_contact);
				}
				GrabSipUriDomain(sipUser, contactFieldEnd, info->m_contactDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(contactField, contactFieldEnd, info->m_contact);
				}
				else
				{
					GrabSipUriUser(contactField, contactFieldEnd, info->m_contact);
				}
				GrabSipUriDomain(contactField, contactFieldEnd, info->m_contactDomain);
			}

		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
		}

		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		if(!info->m_callId.size())
		{
			drop = true;
		}
		if(!info->m_contact)
		{
			drop = true;
		}

		CStdString logMsg;
		info->ToString(logMsg);
		logMsg = sipMethod + ": " + logMsg;
		LOG4CXX_INFO(s_sipPacketLog, logMsg);

		if(drop == false)
		{
			RtpSessionsSingleton::instance()->ReportSip302MovedTemporarily(info);
		}
		else
		{
			CStdString packetInfo;

			info->ToString(packetInfo);
			logMsg.Format("Dropped this %s: %s", sipMethod, packetInfo);
			LOG4CXX_INFO(s_sipPacketLog, logMsg);
		}
	}

	return result;
}

bool TrySipInvite(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload, u_char* packetEnd)
{
	bool result = false;
	bool drop = false;
	CStdString sipMethod;

	int sipLength = ntohs(udpHeader->len) - sizeof(UdpHeaderStruct);
	char* sipEnd = (char*)udpPayload + sipLength;
	if(sipLength < 3 || sipEnd > (char*)packetEnd)
	{
		drop = true;	// packet too short
	}
	else if(memcmp(SIP_METHOD_INVITE, (void*)udpPayload, SIP_METHOD_INVITE_SIZE) == 0)
	{
		sipMethod = SIP_METHOD_INVITE;
	}
	else if(memcmp(SIP_METHOD_ACK, (void*)udpPayload, SIP_METHOD_ACK_SIZE) == 0)
	{
		sipMethod = SIP_METHOD_ACK;
	}
	else if((DLLCONFIG.m_sipTreat200OkAsInvite == true) && (memcmp(SIP_RESPONSE_200_OK, (void*)udpPayload, SIP_RESPONSE_200_OK_SIZE) == 0))
	{
		sipMethod = SIP_METHOD_200_OK;
		LOG4CXX_DEBUG(s_sipExtractionLog, "TrySipInvite: packet matches 200 OK and SipTreat200OkAsInvite is enabled");
	}
	else
	{
		drop = true;
	}

	if (drop == false)
	{
		result = true;

		SipInviteInfoRef info(new SipInviteInfo());

		char* fromField = memFindAfter("From:", (char*)udpPayload, sipEnd);
		if(!fromField)
		{
			fromField = memFindAfter("\nf:", (char*)udpPayload, sipEnd);
		}
		char* toField = memFindAfter("To:", (char*)udpPayload, sipEnd);
		if(!toField)
		{
			toField = memFindAfter("\nt:", (char*)udpPayload, sipEnd);
		}
		char* callIdField = memFindAfter("Call-ID:", (char*)udpPayload, sipEnd);
		if(!callIdField)
		{
			callIdField = memFindAfter("\ni:", (char*)udpPayload, sipEnd);
		}

		char * dialedNumber = NULL;
		if(! DLLCONFIG.m_sipDialedNumberFieldName.empty() )
		{
			dialedNumber = memFindAfter(DLLCONFIG.m_sipDialedNumberFieldName + ":", (char*)udpPayload,sipEnd);
		}
		
		char * sipRemoteParty = NULL;
		if(! DLLCONFIG.m_sipRemotePartyFieldName.empty() )
		{
			sipRemoteParty = memFindAfter(DLLCONFIG.m_sipRemotePartyFieldName + ":", (char*)udpPayload,sipEnd);
		}

		char* localExtensionField = memFindAfter("x-Local-Extension:", (char*)udpPayload, sipEnd);
		char* audioField = NULL;
		char* connectionAddressField = NULL;
		char* attribSendonly = memFindAfter("a=sendonly", (char*)udpPayload, sipEnd);
		char* rtpmapAttribute = memFindAfter("\na=rtpmap:", (char*)udpPayload, sipEnd);
		char* userAgentField = memFindAfter("\nUser-Agent:", (char*)udpPayload, sipEnd);

		if(DLLCONFIG.m_sipRequestUriAsLocalParty == true)
		{
			char* sipUriAttribute = memFindAfter("INVITE ", (char*)udpPayload, sipEnd);

			if(sipUriAttribute)
			{
				if(s_sipExtractionLog->isDebugEnabled())
				{
					CStdString uri;
					GrabLine(sipUriAttribute, sipEnd, uri);
					LOG4CXX_DEBUG(s_sipExtractionLog, "uri: " + uri);
				}

				char* sipUriAttributeEnd = memFindEOL(sipUriAttribute, sipEnd);
				char* sipUser = memFindAfter("sip:", sipUriAttribute, sipUriAttributeEnd);

				if(sipUser)
				{
					if(DLLCONFIG.m_sipReportFullAddress)
					{
						GrabSipUserAddress(sipUser, sipUriAttributeEnd, info->m_requestUri);
					}
					else
					{
						GrabSipUriUser(sipUser, sipUriAttributeEnd, info->m_requestUri);
					}
				}
				else
				{
					if(DLLCONFIG.m_sipReportFullAddress)
					{
						GrabSipUserAddress(sipUriAttribute, sipUriAttributeEnd, info->m_requestUri);
					}
					else
					{
						GrabSipUriUser(sipUriAttribute, sipUriAttributeEnd, info->m_requestUri);
					}
				}

				if(s_sipExtractionLog->isDebugEnabled())
				{
					LOG4CXX_DEBUG(s_sipExtractionLog, "extracted uri: " + info->m_requestUri);
				}
			}
		}

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				LOG4CXX_DEBUG(s_sipExtractionLog, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			GrabSipName(fromField, fromFieldEnd, info->m_fromName);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(sipUser, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(sipUser, fromFieldEnd, info->m_fromDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(fromField, fromFieldEnd, info->m_from);
				}
				else
				{
					GrabSipUriUser(fromField, fromFieldEnd, info->m_from);
				}
				GrabSipUriDomain(fromField, fromFieldEnd, info->m_fromDomain);
			}
		}
		if(toField)
		{
			CStdString to;
			char* toFieldEnd = GrabLine(toField, sipEnd, to);
			LOG4CXX_DEBUG(s_sipExtractionLog, "to: " + to);

			GrabSipName(toField, toFieldEnd, info->m_toName);

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipUser, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(sipUser, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(sipUser, toFieldEnd, info->m_toDomain);
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(toField, toFieldEnd, info->m_to);
				}
				else
				{
					GrabSipUriUser(toField, toFieldEnd, info->m_to);
				}
				GrabSipUriDomain(toField, toFieldEnd, info->m_toDomain);
			}
			if(DLLCONFIG.m_sipGroupPickUpPattern == info->m_to)
			{
				info->m_SipGroupPickUpPatternDetected = true;
			}
		}
		if(dialedNumber)
		{
			CStdString token;
			GrabTokenSkipLeadingWhitespaces(dialedNumber, sipEnd, token);
			info->m_sipDialedNumber = token;
		}
		if(sipRemoteParty)
		{
			CStdString token;

			char* sip = memFindAfter("sip:", sipRemoteParty, sipEnd);
			if(sip)
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sip, sipEnd, token);
				}
				else
				{
					GrabSipUriUser(sip, sipEnd, token);
				}
			}
			else
			{
				if(DLLCONFIG.m_sipReportFullAddress)
				{
					GrabSipUserAddress(sipRemoteParty, sipEnd, token);
				}
				else
				{
					GrabSipUriUser(sipRemoteParty, sipEnd, token);
				}
			}
			info->m_sipRemoteParty = token;
		}
		if(callIdField)
		{
			GrabTokenSkipLeadingWhitespaces(callIdField, sipEnd, info->m_callId);
			audioField = memFindAfter("m=audio ", callIdField, sipEnd);
			connectionAddressField = memFindAfter("c=IN IP4 ", callIdField, sipEnd);
		}
		if(localExtensionField)
		{
			CStdString localExtension;
			GrabTokenSkipLeadingWhitespaces(localExtensionField, sipEnd, localExtension);
			if(localExtension.size() > 0)
			{
				info->m_from = localExtension;
			}
		}
		if(userAgentField)
		{
			GrabTokenSkipLeadingWhitespaces(userAgentField, sipEnd, info->m_userAgent);
		}
		if(audioField)
		{
			GrabToken(audioField, sipEnd, info->m_fromRtpPort);
		}
		if(attribSendonly)
		{
			info->m_attrSendonly = true;
		}
		if(connectionAddressField)
		{
			CStdString connectionAddress;
			GrabToken(connectionAddressField, sipEnd, connectionAddress);
			struct in_addr fromIp;
			if(connectionAddress.size())
			{
				if(ACE_OS::inet_aton((PCSTR)connectionAddress, &fromIp))
				{
					info->m_fromRtpIp = fromIp;

					if (DLLCONFIG.m_sipDropIndirectInvite)
					{
						if((unsigned int)fromIp.s_addr != (unsigned int)ipHeader->ip_src.s_addr)
						{
							// SIP invite SDP connection address does not match with SIP packet origin
							drop =true;
						}
					}
				}
			}
		}
		// SIP fields extraction
		for(std::list<CStdString>::iterator it = DLLCONFIG.m_sipExtractFields.begin(); it != DLLCONFIG.m_sipExtractFields.end(); it++)
		{
			CStdString fieldName = *it + ":";
			char* szField = memFindAfter((PSTR)(PCSTR)fieldName, (char*)udpPayload, sipEnd);
			if(szField)
			{
				CStdString field;

				// XXX
				// The line below was replaced because I experienced
				// cases where we would have a leading whitespace in the
				// tag which has been extracted.  However, since we are
				// dealing with SIP, RFC 3261, section 7.3.1 illustrates
				// that any leading whitespaces after the colon is not
				// in fact part of the header value.  Therefore, I
				// created the GrabLineSkipLeadingWhitespace() function
				// which I use in this particular case.
				//
				// Hope this is ok.
				//
				// --Gerald
				//
				//GrabLine(szField, sipEnd, field);

				GrabLineSkipLeadingWhitespace(szField, sipEnd, field);
				info->m_extractedFields.insert(std::make_pair(*it, field));
			}
		}

		if(DLLCONFIG.m_rtpReportDtmf)
		{
			if(rtpmapAttribute)
			{
				CStdString rtpPayloadType, nextToken;
				char *nextStep = NULL;

				while(rtpmapAttribute && rtpmapAttribute < sipEnd)
				{
					rtpPayloadType = "";
					GrabTokenSkipLeadingWhitespaces(rtpmapAttribute, sipEnd, rtpPayloadType);
					nextToken.Format("%s ", rtpPayloadType);
					nextStep = memFindAfter((char*)nextToken.c_str(), rtpmapAttribute, sipEnd);

					/* We need our "nextStep" to contain at least the length
					 * of the string "telephone-event", 15 characters */
					if(nextStep && ((sipEnd - nextStep) >= 15))
					{
						if(ACE_OS::strncasecmp(nextStep, "telephone-event", 15) == 0)
						{
							/* Our DTMF packets are indicated using
							 * the payload type rtpPayloadType */
							info->m_telephoneEventPayloadType = rtpPayloadType;
							info->m_telephoneEventPtDefined = true;
							break;
						}
					}

					rtpmapAttribute = memFindAfter("\na=rtpmap:", rtpmapAttribute, sipEnd);
				}
			}
		}

		if((unsigned int)info->m_fromRtpIp.s_addr == 0)
		{
			// In case connection address could not be extracted, use SIP invite sender IP address
			if(DLLCONFIG.m_dahdiIntercept == true)
			{
				info->m_fromRtpIp = ipHeader->ip_dest;
			}
			else
			{
				info->m_fromRtpIp = ipHeader->ip_src;
			}
		}
		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;
		info->m_recvTime = time(NULL);
		memcpy(info->m_senderMac, ethernetHeader->sourceMac, sizeof(info->m_senderMac));
		memcpy(info->m_receiverMac, ethernetHeader->destinationMac, sizeof(info->m_receiverMac));

		if(sipMethod.Equals(SIP_METHOD_INVITE) || info->m_fromRtpPort.size())
		{
			// Only log SIP non-INVITE messages that contain SDP (i.e. with a valid RTP port)
			CStdString logMsg;
			info->ToString(logMsg);
			logMsg = sipMethod + ": " + logMsg;
			LOG4CXX_INFO(s_sipPacketLog, logMsg);
		}

		if(drop == false && info->m_fromRtpPort.size() && info->m_from.size() && info->m_to.size() && info->m_callId.size())
		{
			RtpSessionsSingleton::instance()->ReportSipInvite(info);
		}
		else
		{
			if(drop == false && DLLCONFIG.m_sipUse200OkMediaAddress && info->m_fromRtpPort.size() && info->m_from.size() && info->m_to.size() && info->m_callId.size())
			{
				// Get information from 200 OK
				RtpSessionsSingleton::instance()->ReportSipInvite(info);
			}
			//logMsg.Format("Not logging this INVITE: drop:%s m_fromRtpPort:%s from:%s to:%s callId:%s", (drop ? "true" : "false"), info->m_fromRtpPort, info->m_from, info->m_to, info->m_callId);
			//LOG4CXX_INFO(s_sipPacketLog, logMsg);
		}
	}
	return result;
}

void HandleSkinnyMessage(SkinnyHeaderStruct* skinnyHeader, IpHeaderStruct* ipHeader, u_char* packetEnd, TcpHeaderStruct* tcpHeader)
{
	bool useful = true;
	CStdString logMsg;
	
	SkStartMediaTransmissionStruct* startMedia;
	SkStartMediaTransmissionStruct smtmp;
	SkStopMediaTransmissionStruct* stopMedia;
	SkCallInfoStruct* callInfo;
	SkOpenReceiveChannelAckStruct* openReceiveAck;
	SkOpenReceiveChannelAckStruct orcatmp;
	SkLineStatStruct* lineStat;
	SkCcm5CallInfoStruct* ccm5CallInfo;
	SkSoftKeyEventMessageStruct* softKeyEvent;
	SkSoftKeySetDescriptionStruct* softKeySetDescription;

	char szEndpointIp[16];
	struct in_addr endpointIp = ipHeader->ip_dest;	// most of the interesting skinny messages are CCM -> phone

	memset(&smtmp, 0, sizeof(smtmp));
	memset(&orcatmp, 0, sizeof(orcatmp));

	switch(skinnyHeader->messageType)
	{
	case SkStartMediaTransmission:

		startMedia = (SkStartMediaTransmissionStruct*)skinnyHeader;
		SkCcm7_1StartMediaTransmissionStruct *ccm7_1sm;
		ccm7_1sm = (SkCcm7_1StartMediaTransmissionStruct*)skinnyHeader;

		if(SkinnyValidateStartMediaTransmission(startMedia, packetEnd))
		{
			if(s_skinnyPacketLog->isInfoEnabled())
			{
				char szRemoteIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&startMedia->remoteIpAddr, szRemoteIp, sizeof(szRemoteIp));
				logMsg.Format(" CallId:%u PassThru:%u media address:%s,%u", startMedia->conferenceId, startMedia->passThruPartyId, szRemoteIp, startMedia->remoteTcpPort);
			}
			RtpSessionsSingleton::instance()->ReportSkinnyStartMediaTransmission(startMedia, ipHeader, tcpHeader);	
		}
		else if(SkinnyValidateCcm7_1StartMediaTransmission(ccm7_1sm, packetEnd))
		{
			startMedia = &smtmp;

			memcpy(&startMedia->header, &ccm7_1sm->header, sizeof(startMedia->header));
			startMedia->conferenceId = ccm7_1sm->conferenceId;
			startMedia->passThruPartyId = ccm7_1sm->passThruPartyId;
			memcpy(&startMedia->remoteIpAddr, &ccm7_1sm->remoteIpAddr, sizeof(startMedia->remoteIpAddr));
			startMedia->remoteTcpPort = ccm7_1sm->remoteTcpPort;

			if(s_skinnyPacketLog->isInfoEnabled())
			{
				char szRemoteIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&startMedia->remoteIpAddr, szRemoteIp, sizeof(szRemoteIp));
				logMsg.Format(" (CCM 7.1) CallId:%u PassThru:%u media address:%s,%u", startMedia->conferenceId, startMedia->passThruPartyId, szRemoteIp, startMedia->remoteTcpPort);
			}

			RtpSessionsSingleton::instance()->ReportSkinnyStartMediaTransmission(startMedia, ipHeader, tcpHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid StartMediaTransmission.");
		}
		break;
	case SkStopMediaTransmission:
	case SkCloseReceiveChannel:
		// StopMediaTransmission and CloseReceiveChannel have the same definition, treat them the same for now.
		stopMedia = (SkStopMediaTransmissionStruct*)skinnyHeader;
		if(SkinnyValidateStopMediaTransmission(stopMedia, packetEnd))
		{
			if(s_skinnyPacketLog->isInfoEnabled())
			{
				logMsg.Format(" ConferenceId:%u PassThruPartyId:%u", stopMedia->conferenceId, stopMedia->passThruPartyId);
			}
			RtpSessionsSingleton::instance()->ReportSkinnyStopMediaTransmission(stopMedia, ipHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid StopMediaTransmission or CloseReceiveChannel.");
		}
		break;
	case SkCallInfoMessage:
		callInfo = (SkCallInfoStruct*)skinnyHeader;
		if(SkinnyValidateCallInfo(callInfo, packetEnd))
		{
			if(s_skinnyPacketLog->isInfoEnabled())
			{
				logMsg.Format(" CallId:%u calling:%s called:%s callingname:%s calledname:%s line:%d callType:%d", 
								callInfo->callId, callInfo->callingParty, callInfo->calledParty, 
								callInfo->callingPartyName, callInfo->calledPartyName, callInfo->lineInstance, callInfo->callType);
			}
			RtpSessionsSingleton::instance()->ReportSkinnyCallInfo(callInfo, ipHeader, tcpHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid CallInfoMessage.");
		}
		break;
	case SkCcm5CallInfoMessage:
		ccm5CallInfo = (SkCcm5CallInfoStruct*)skinnyHeader;
		if(SkinnyValidateCcm5CallInfo(ccm5CallInfo, packetEnd))
		{
			// Extract Calling and Called number.
			char* parties = (char*)(&ccm5CallInfo->parties);
			char* partiesPtr = parties;
			long partiesLen = (long)packetEnd - (long)ccm5CallInfo - sizeof(SkCcm5CallInfoStruct);

			CStdString callingParty;
			CStdString calledParty;
			CStdString callingPartyName;
			CStdString calledPartyName;

			// Tokens separated by a single null char. Multiple sequential null chars result in empty tokens.
			// There are 12 tokens:
			// calling general number, called long num, called long num, called long num
			// calling extension (1), called ext num, called ext num, called ext num 
			// calling name, called name (2), called name, called name
			//(1) not always available, in this case, use calling general number
			//(2) not always available
			int tokenNr = 0;
			while(tokenNr < 10 && partiesPtr < parties+partiesLen)
			{
				CStdString party;
				GrabTokenAcceptSpace(partiesPtr, parties+partiesLen, party);
				if(s_skinnyPacketLog->isDebugEnabled())
				{
					logMsg = logMsg + party + ", ";
				}
				partiesPtr += party.size() + 1;
				tokenNr += 1;

				switch(tokenNr)
				{
				case 1:
					// Token 1 can be the general office number for outbound
					// TODO, report this as local entry point
					callingParty = party;
					break;
				case 2:
					calledParty = party;
					break;
				case 3:
					if(DLLCONFIG.m_cucm7_1Mode == true)
					{
						// In CCM 7.1, it appears that each party is
						// named twice
						calledParty = party;
					}
					break;
				case 5:
					if(DLLCONFIG.m_cucm7_1Mode == false)
					{
						// Token 5 is the calling extension, use this if outbound for callingParty instead of general number
						// With CCM 7.1, this appears not to be the case
						if(party.size()>0 && ccm5CallInfo->callType == SKINNY_CALL_TYPE_OUTBOUND)
						{
							callingParty = party;
						}
					}
					break;
				case 6:
					// This is the called party extension, not used for now
					break;
				case 9:
					callingPartyName = party;
					break;
				case 10:
					calledPartyName = party;
					break;
				}
			}
			LOG4CXX_DEBUG(s_skinnyPacketLog, "parties tokens:" + logMsg);

			// Emulate a regular CCM CallInfo message
			SkCallInfoStruct callInfo;
			strcpy(callInfo.calledParty, (PCSTR)calledParty);
			strcpy(callInfo.callingParty, (PCSTR)callingParty);
			callInfo.callId = ccm5CallInfo->callId;
			callInfo.callType = ccm5CallInfo->callType;
			callInfo.lineInstance = 0;
			if(calledPartyName.size())
			{
				strncpy(callInfo.calledPartyName, (PCSTR)calledPartyName, sizeof(callInfo.calledPartyName));
			}
			else
			{
				callInfo.calledPartyName[0] = '\0';
			}
			if(callingPartyName.size())
			{
				strncpy(callInfo.callingPartyName, (PCSTR)callingPartyName, sizeof(callInfo.callingPartyName));
			}
			else
			{
				callInfo.callingPartyName[0] = '\0';
			}

			if(s_skinnyPacketLog->isInfoEnabled())
			{
				logMsg.Format(" CallId:%u calling:%s called:%s callingname:%s calledname:%s callType:%d", callInfo.callId, 
								callInfo.callingParty, callInfo.calledParty, callInfo.callingPartyName, callInfo.calledPartyName, callInfo.callType);
			}
			RtpSessionsSingleton::instance()->ReportSkinnyCallInfo(&callInfo, ipHeader, tcpHeader);
		}
		break;
	case SkOpenReceiveChannelAck:

		openReceiveAck = (SkOpenReceiveChannelAckStruct*)skinnyHeader;
		SkCcm7_1SkOpenReceiveChannelAckStruct *orca;
		orca = (SkCcm7_1SkOpenReceiveChannelAckStruct*)skinnyHeader;
		
		if(SkinnyValidateOpenReceiveChannelAck(openReceiveAck, packetEnd))
		{
			if(s_skinnyPacketLog->isInfoEnabled())
			{
				char szMediaIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&openReceiveAck->endpointIpAddr, szMediaIp, sizeof(szMediaIp));
				logMsg.Format(" PassThru:%u media address:%s,%u", openReceiveAck->passThruPartyId, szMediaIp, openReceiveAck->endpointTcpPort);
			}
			endpointIp = ipHeader->ip_src;	// this skinny message is phone -> CCM
			RtpSessionsSingleton::instance()->ReportSkinnyOpenReceiveChannelAck(openReceiveAck, ipHeader, tcpHeader);
		}
		else if(SkinnyValidateCcm7_1SkOpenReceiveChannelAckStruct(orca, packetEnd))
		{
			openReceiveAck = &orcatmp;

			memcpy(&openReceiveAck->header, &orca->header, sizeof(openReceiveAck->header));
			openReceiveAck->openReceiveChannelStatus = orca->openReceiveChannelStatus;
			memcpy(&openReceiveAck->endpointIpAddr, &orca->endpointIpAddr, sizeof(openReceiveAck->endpointIpAddr));
			openReceiveAck->endpointTcpPort = orca->endpointTcpPort;
			openReceiveAck->passThruPartyId = orca->passThruPartyId;

			if(s_skinnyPacketLog->isInfoEnabled())
			{
				char szMediaIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&openReceiveAck->endpointIpAddr, szMediaIp, sizeof(szMediaIp));
				logMsg.Format(" (CCM 7.1) PassThru:%u media address:%s,%u", openReceiveAck->passThruPartyId, szMediaIp, openReceiveAck->endpointTcpPort);
			}
			endpointIp = ipHeader->ip_src;	// this skinny message is phone -> CCM
			RtpSessionsSingleton::instance()->ReportSkinnyOpenReceiveChannelAck(openReceiveAck, ipHeader, tcpHeader);			
		}
		
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid OpenReceiveChannelAck.");
		}
		break;
	case SkLineStatMessage:
		lineStat = (SkLineStatStruct*)skinnyHeader;
		if(SkinnyValidateLineStat(lineStat, packetEnd))
		{
			if(s_skinnyPacketLog->isInfoEnabled())
			{
				logMsg.Format(" line:%u extension:%s display name:%s", lineStat->lineNumber, lineStat->lineDirNumber, lineStat->displayName);
			}
			endpointIp = ipHeader->ip_dest;	// this skinny message is CCM -> phone
			RtpSessionsSingleton::instance()->ReportSkinnyLineStat(lineStat, ipHeader, tcpHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid LineStatMessage.");
		}

		break;
	case SkSoftKeyEventMessage:
		softKeyEvent = (SkSoftKeyEventMessageStruct*)skinnyHeader;
		if(SkinnyValidateSoftKeyEvent(softKeyEvent, packetEnd))
		{
			useful = true;
			logMsg.Format(" eventString:%s eventNum:%d line:%lu callId:%lu",
					SoftKeyEvent::SoftKeyEventToString(softKeyEvent->softKeyEvent),
					softKeyEvent->softKeyEvent,
					softKeyEvent->lineInstance,
					softKeyEvent->callIdentifier);

			endpointIp = ipHeader->ip_src;  // this skinny message is phone -> CCM

			switch(softKeyEvent->softKeyEvent)
			{
			case SoftKeyEvent::SkSoftKeyHold:
				RtpSessionsSingleton::instance()->ReportSkinnySoftKeyHold(softKeyEvent, ipHeader);
				break;
			case SoftKeyEvent::SkSoftKeyResume:
				RtpSessionsSingleton::instance()->ReportSkinnySoftKeyResume(softKeyEvent, ipHeader);
				break;
			case SoftKeyEvent::SkSoftKeyConfrn:
				if (DLLCONFIG.m_SkinnyTrackConferencesTransfers == true)
				{
					RtpSessionsSingleton::instance()->ReportSkinnySoftKeyConfPressed(endpointIp, tcpHeader);
				}
				break;
			default:
				CStdString logSoftKey;

				logSoftKey.Format("Ignoring unsupported event %s (%d)",
					SoftKeyEvent::SoftKeyEventToString(softKeyEvent->softKeyEvent),
					softKeyEvent->softKeyEvent);
				LOG4CXX_INFO(s_skinnyPacketLog, logSoftKey);
				break;
			}
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid SoftKeyEventMessage.");
		}
		break;
	case SkSoftKeySetDescription:
		softKeySetDescription = (SkSoftKeySetDescriptionStruct*)skinnyHeader;
		if(SkinnyValidateSoftKeySetDescription(softKeySetDescription, packetEnd))
		{
			useful = true;
			logMsg.Format(" eventString:%s eventNum:%d line:%lu callId:%lu",
					SoftKeySetDescription::SoftKeySetDescriptionToString(softKeySetDescription->softKeySetDescription),
					softKeySetDescription->softKeySetDescription,
					softKeySetDescription->lineInstance,
					softKeySetDescription->callIdentifier);

			endpointIp = ipHeader->ip_dest;
			switch(softKeySetDescription->softKeySetDescription)
			{
				case SoftKeySetDescription::SkSoftKeySetConference:
					if (DLLCONFIG.m_SkinnyTrackConferencesTransfers == true)
					{
						RtpSessionsSingleton::instance()->ReportSkinnySoftKeySetConfConnected(endpointIp, tcpHeader);
					}
					break;
			}
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid SoftKeySetDescription.");
		}
		break;
	default:
		useful = false;
	}
	if(useful && s_skinnyPacketLog->isInfoEnabled())
	{
		CStdString msg = SkinnyMessageToString(skinnyHeader->messageType);
		ACE_OS::inet_ntop(AF_INET, (void*)&endpointIp, szEndpointIp, sizeof(szEndpointIp));
		logMsg = "processed " + msg + logMsg + " endpoint:" + szEndpointIp;
		LOG4CXX_INFO(s_skinnyPacketLog, logMsg);
	}
}

void HandlePacket(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	time_t now = time(NULL);

	s_numPackets++;
	s_numPacketsPerSecond++;
	if(s_lastPacketsPerSecondTime != now)
	{
		s_lastPacketsPerSecondTime = now;
		if(s_numPacketsPerSecond > s_maxPacketsPerSecond)
		{
			s_maxPacketsPerSecond = s_numPacketsPerSecond;
		}
		if(s_minPacketsPerSecond == 0)
		{
			s_minPacketsPerSecond = s_numPacketsPerSecond;
		}
		if(s_numPacketsPerSecond < s_minPacketsPerSecond)
		{
			s_minPacketsPerSecond = s_numPacketsPerSecond;
		}
		s_numPacketsPerSecond = 0;
	}

	if(s_liveCapture && (now - s_lastPcapStatsReportingTime) > 10)
	{
		MutexSentinel mutexSentinel(s_mutex);		// serialize access for competing pcap threads
		s_lastPcapStatsReportingTime = now;
		VoIpSingleton::instance()->ReportPcapStats();

		CStdString logMsg;
		logMsg.Format("numPackets:%u maxPPS:%u minPPS:%u", s_numPackets, s_maxPacketsPerSecond, s_minPacketsPerSecond);
		LOG4CXX_INFO(s_packetStatsLog, logMsg)
		s_numPackets = 0;
		s_maxPacketsPerSecond = 0;
		s_minPacketsPerSecond = 0;
	}
	if(DLLCONFIG.m_pcapTest)
	{
		return;
	}

	EthernetHeaderStruct* ethernetHeader = (EthernetHeaderStruct *)pkt_data;
	IpHeaderStruct* ipHeader = NULL;

	if(ntohs(ethernetHeader->type) == 0x8100)
	{
		ipHeader = (IpHeaderStruct*)((char*)ethernetHeader + sizeof(EthernetHeaderStruct) + 4);
	}
	else
	{
		ipHeader = (IpHeaderStruct*)((char*)ethernetHeader + sizeof(EthernetHeaderStruct));
	}

	if(ipHeader->ip_v != 4)	// sanity check, is it an IP packet v4
	{
		// If not, the IP packet might have been captured from multiple interfaces using the tcpdump -i switch
		ipHeader = (IpHeaderStruct*)((u_char*)ipHeader+2);
		if(ipHeader->ip_v != 4)
		{
			// If not, the IP packet might be wrapped into a 802.1Q VLAN or MPLS header (add 4 bytes, ie 2 bytes on top of previous 2)
			ipHeader = (IpHeaderStruct*)((u_char*)ipHeader+2);
			if(ipHeader->ip_v != 4)
			{
				// If not, the IP packet might be tcpdump -i as well as VLAN, add another 2 bytes
				ipHeader = (IpHeaderStruct*)((u_char*)ipHeader+2);
				if(ipHeader->ip_v != 4)
				{
					// If not, the IP packet might be on 802.11
					ipHeader = (IpHeaderStruct*)((u_char*)ipHeader+12);
					if(ipHeader->ip_v != 4)
					{
						// Still not an IP packet V4, drop it
						return;
					}
				}
			}
		}
	}
	int ipHeaderLength = ipHeader->ip_hl*4;
	u_char* ipPacketEnd = (u_char*)ipHeader + ntohs(ipHeader->ip_len);
	u_char* captureEnd = (u_char*)pkt_data + header->caplen; 
	if( captureEnd < (u_char*)ipPacketEnd  || (u_char*)ipPacketEnd <= ((u_char*)ipHeader + ipHeaderLength + TCP_HEADER_LENGTH))
	{
		// The packet has been snipped or has not enough payload, drop it,
		return;
	}

//#ifdef WIN32
	if(!s_liveCapture)
	{
		// This is a pcap file replay
		if(DLLCONFIG.m_pcapFastReplay)
		{
			if((now - s_lastPause) > 1)
			{
				if(DLLCONFIG.m_pcapFastReplaySleepUsPerSec > 0)
				{
					ACE_Time_Value yield;
					yield.set(0,DLLCONFIG.m_pcapFastReplaySleepUsPerSec * 1000);
					ACE_OS::sleep(yield);
				}
				s_lastPause = now;
			}
			else
			{
				// Make sure Orkaudio won't be flooded by too many
				// packets at a time by yielding control to other threads.
				struct timespec ts;
				ts.tv_sec = 0;
				ts.tv_nsec = 1;
				ACE_OS::nanosleep (&ts, NULL);
			}
		}
		else
		{
			// Simulate normal ("real-time") replay speed:
			// Every capture-second, wait for the local clock to elapse a second.
			if(header->ts.tv_sec != s_lastPacketTimestamp)
			{
				while(now == time(NULL))
				{
					struct timespec ts;
					ts.tv_sec = 0;
					ts.tv_nsec = 5000000;	// 5 ms
					ACE_OS::nanosleep (&ts, NULL);
				}
				s_lastPacketTimestamp = header->ts.tv_sec;
			}
		}
	}
//#endif

	if(DLLCONFIG.IsPacketWanted(ipHeader) == false)
	{
		return;
	}

	if(ipHeader->ip_p == IPPROTO_UDP)
	{
		UdpHeaderStruct* udpHeader = (UdpHeaderStruct*)((char *)ipHeader + ipHeaderLength);

		if(ntohs(udpHeader->source) >= 1024 && ntohs(udpHeader->dest) >= 1024) {
			bool detectedUsefulPacket = false;
			u_char* udpPayload = (u_char *)udpHeader + sizeof(UdpHeaderStruct);

			MutexSentinel mutexSentinel(s_mutex); // serialize access for competing pcap threads

			detectedUsefulPacket = TryRtp(ethernetHeader, ipHeader, udpHeader, udpPayload);

			if(!detectedUsefulPacket) {
				detectedUsefulPacket= TrySipInvite(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket= TrySip200Ok(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
			}
			

			if(DLLCONFIG.m_sipNotifySupport == true){
				if(!detectedUsefulPacket) {
					detectedUsefulPacket= TrySipNotify(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
				}
			}
			if(!detectedUsefulPacket) {
				if(DLLCONFIG.m_sipDetectSessionProgress == true)
				{
					detectedUsefulPacket = TrySipSessionProgress(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
				}
			}

			if(!detectedUsefulPacket) {
				if(DLLCONFIG.m_sip302MovedTemporarilySupport == true)
				{
					detectedUsefulPacket = TrySip302MovedTemporarily(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
				}
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TrySipBye(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TryLogFailedSip(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
			}

			if(!detectedUsefulPacket) {
				if(DLLCONFIG.m_rtcpDetect == true)
				{
					detectedUsefulPacket = TryRtcp(ethernetHeader, ipHeader, udpHeader, udpPayload);
				}
			}

			if(DLLCONFIG.m_iax2Support == false)
			{
				detectedUsefulPacket = true;	// Stop trying to detect if this UDP packet could be of interest
			}

			if(!detectedUsefulPacket) {
				 detectedUsefulPacket = TryIax2New(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TryIax2Accept(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TryIax2Authreq(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TryIax2Hangup(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TryIax2ControlHangup(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TryIax2Reject(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TryIax2FullVoiceFrame(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TryIax2MetaTrunkFrame(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}

			if(!detectedUsefulPacket) {
				detectedUsefulPacket = TryIax2MiniVoiceFrame(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}
		}
	}
	else if(ipHeader->ip_p == IPPROTO_TCP)
	{
		TcpHeaderStruct* tcpHeader = (TcpHeaderStruct*)((char *)ipHeader + ipHeaderLength);

		if(ntohs(tcpHeader->source) == DLLCONFIG.m_skinnyTcpPort || ntohs(tcpHeader->dest) == DLLCONFIG.m_skinnyTcpPort)
		{
			u_char* startTcpPayload = (u_char*)tcpHeader + (tcpHeader->off * 4);
			SkinnyHeaderStruct* skinnyHeader = (SkinnyHeaderStruct*)(startTcpPayload);

			// Scan all skinny messages in this TCP packet
			while(	ipPacketEnd > (u_char*)skinnyHeader && 
					(u_char*)skinnyHeader>=startTcpPayload &&
					(ipPacketEnd - (u_char*)skinnyHeader) > SKINNY_MIN_MESSAGE_SIZE	&&
					skinnyHeader->len > 1 && skinnyHeader->len < 2048 &&
					skinnyHeader->messageType >= 0x0 && skinnyHeader->messageType <= 0x200 )	// Last known skinny message by ethereal is 0x13F, but seen higher message ids in the field.
			{
				if(s_skinnyPacketLog->isDebugEnabled())
				{
					CStdString dbg;
					unsigned int offset = (u_char*)skinnyHeader - startTcpPayload;
					dbg.Format("Offset:%x Len:%u Type:%x %s", offset, skinnyHeader->len, skinnyHeader->messageType, SkinnyMessageToString(skinnyHeader->messageType));
					LOG4CXX_DEBUG(s_skinnyPacketLog, dbg);
				}
				MutexSentinel mutexSentinel(s_mutex);		// serialize access for competing pcap threads

				HandleSkinnyMessage(skinnyHeader, ipHeader, ipPacketEnd, tcpHeader);

				// Point to next skinny message within this TCP packet
				skinnyHeader = (SkinnyHeaderStruct*)((u_char*)skinnyHeader + SKINNY_HEADER_LENGTH + skinnyHeader->len);
			}
		}
		else if(DLLCONFIG.m_sipOverTcpSupport) 
		{
			//CStdString tcpSeq;
			//memToHex((unsigned char *)&tcpHeader->seq, 4, tcpSeq);
			TrySipTcp(ethernetHeader, ipHeader, tcpHeader);
		}
	}

	if((now - s_lastHooveringTime) > 5)
	{
		MutexSentinel mutexSentinel(s_mutex);		// serialize access for competing pcap threads
		s_lastHooveringTime = now;
		RtpSessionsSingleton::instance()->Hoover(now);
		Iax2SessionsSingleton::instance()->Hoover(now);
	}
}

void SingleDeviceCaptureThreadHandler(pcap_t* pcapHandle)
{
	bool repeat = false;
	if(!s_liveCapture)
	{
		// File replay, make sure that only one file is replayed at a time
		s_replaySemaphore.acquire();
		s_replayThreadCounter--;
		if(s_replayThreadCounter == 0 && DLLCONFIG.m_pcapRepeat)
		{
			repeat = true;
		}
	}
	if(pcapHandle)
	{
		CStdString log;
		log.Format("Start Capturing: pcap handle:%x", pcapHandle);
		LOG4CXX_INFO(s_packetLog, log);
		pcap_loop(pcapHandle, 0, HandlePacket, NULL);
		if(!s_liveCapture)
		{
			// This is a pcap file replay, stop all sessions before exiting
			RtpSessionsSingleton::instance()->StopAll();
			Iax2SessionsSingleton::instance()->StopAll();
			pcap_close(pcapHandle);
		}
		log.Format("Stop Capturing: pcap handle:%x", pcapHandle);
		LOG4CXX_INFO(s_packetLog, log);

		if(s_liveCapture == true)
		{
			CStdString deviceName;
			pcap_t* oldHandle = NULL;

			deviceName = VoIpSingleton::instance()->GetPcapDeviceName(pcapHandle);
			if(deviceName.size())
			{
				oldHandle = pcapHandle;
				VoIpSingleton::instance()->RemovePcapDeviceFromMap(pcapHandle);
				pcap_close(pcapHandle); // XXX this can cause a crash if later other code is added to close all handles in the m_pcapHandles list

				while(1)
				{
					struct timespec ts;

					ts.tv_sec = 60; // Try re-open after a minute
					ts.tv_nsec = 0;
					ACE_OS::nanosleep (&ts, NULL);

					log.Format("Attempting to re-open device:%s - old handle:%x was closed", deviceName, oldHandle);
					LOG4CXX_INFO(s_packetLog, log);

					pcapHandle = VoIpSingleton::instance()->OpenDevice(deviceName);
					if(pcapHandle != NULL)
					{
						VoIpSingleton::instance()->AddPcapDeviceToMap(deviceName, pcapHandle);

						log.Format("Start Capturing: pcap handle:%x", pcapHandle);
						LOG4CXX_INFO(s_packetLog, log);

						pcap_loop(pcapHandle, 0, HandlePacket, NULL);

						log.Format("Stop Capturing: pcap handle:%x", pcapHandle);
						LOG4CXX_INFO(s_packetLog, log);

						oldHandle = pcapHandle;
						VoIpSingleton::instance()->RemovePcapDeviceFromMap(pcapHandle);
						pcap_close(pcapHandle);
					}
				}
			}
			else
			{
				log.Format("Running in live capture mode but unable to determine which device handle:%x belongs to. Will not restart capture", pcapHandle);
				LOG4CXX_INFO(s_packetLog, log);
				pcap_close(pcapHandle); // XXX this can cause a crash if later other code is added to close all handles in the m_pcapHandles list
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(s_packetLog, "Cannot start capturing, pcap handle is null");
	}
	if(!s_liveCapture)
	{
		// Pass token to for next file replay
		s_replaySemaphore.release();
	}
	if(repeat == true)
	{
		// Reinitialize for another file replay cycle.
		VoIpSingleton::instance()->Initialize();
		VoIpSingleton::instance()->Run();
	}
}


//=======================================================
VoIp::VoIp()
{
	m_pcapHandle = NULL;
}

void Configure(DOMNode* node)
{
	s_voipPluginLog =  Logger::getLogger("voipplugin");

	if (node)
	{
		VoIpConfigTopObjectRef VoIpConfigTopObjectRef(new VoIpConfigTopObject);
		try
		{
			VoIpConfigTopObjectRef.get()->DeSerializeDom(node);
			g_VoIpConfigTopObjectRef = VoIpConfigTopObjectRef;
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(s_voipPluginLog, e + " - check your config.xml");
		}
	}
	else
	{
		LOG4CXX_ERROR(s_voipPluginLog, "Got empty DOM tree");
	}
}

void VoIp::OpenPcapDirectory(CStdString& path)
{
	CStdString logMsg;

	// Iterate over folder
	ACE_DIR* dir = ACE_OS::opendir((PCSTR)path);
	if (!dir)
	{
		LOG4CXX_ERROR(s_packetLog, CStdString("pcap traces directory could not be found:" + path + " please correct this in config.xml"));
	}
	else
	{
		dirent* dirEntry = NULL;
		while((dirEntry = ACE_OS::readdir(dir)))
		{	
			CStdString dirEntryFilename = dirEntry->d_name;
			CStdString pcapExtension = ".pcap";
			int extensionPos = dirEntryFilename.Find(pcapExtension);
			if(extensionPos == -1)
			{
				pcapExtension = ".cap";
				extensionPos = dirEntryFilename.Find(pcapExtension);
			}

			if ( extensionPos != -1 && (dirEntryFilename.size() - extensionPos) == pcapExtension.size() )
			{
				CStdString pcapFilePath = path + "/" + dirEntry->d_name;
				if(FileCanOpen(pcapFilePath))
				{
					OpenPcapFile(pcapFilePath);
				}
			}
		}
		ACE_OS::closedir(dir);
	}

}

char* VoIp::ApplyPcapFilter()
{
	struct bpf_program fp;
	char* error = NULL;
	CStdString logMsg;

	if(DLLCONFIG.m_pcapFilter.size())
	{
		if(pcap_compile(m_pcapHandle,&fp, (PSTR)(PCSTR)DLLCONFIG.m_pcapFilter,1,0) == -1)
		{
			error = pcap_geterr(m_pcapHandle);
			logMsg.Format("pcap_compile: Please check your PcapFilter in config.xml; pcap handle:%x", m_pcapHandle);
			LOG4CXX_ERROR(s_packetLog, logMsg);

		} 
		if(error == NULL && pcap_setfilter(m_pcapHandle,&fp) == -1)
		{ 
			error = pcap_geterr(m_pcapHandle);
			logMsg.Format("pcap_setfilter: Please check your PcapFilter in config.xml; pcap handle:%x", m_pcapHandle);
			LOG4CXX_ERROR(s_packetLog, logMsg);
		}
	}
	return error;
}

void VoIp::OpenPcapFile(CStdString& filename)
{
	CStdString logMsg;

	LOG4CXX_INFO(s_packetLog, CStdString("Adding pcap capture file to replay list:") + filename);

	// Open device
	char * error = NULL;

	m_pcapHandle = pcap_open_offline((PCSTR)filename , error);

	if(error == NULL)
	{
		error = ApplyPcapFilter();
	}
	if(error)
	{
		LOG4CXX_ERROR(s_packetLog, "pcap error when opening file:" + filename + "; pcap error:" + error);
	}
	else
	{
		logMsg.Format("Successfully opened file. pcap handle:%x", m_pcapHandle);
		LOG4CXX_INFO(s_packetLog, logMsg);
		m_pcapHandles.push_back(m_pcapHandle);
	}
}

void VoIp::SetPcapSocketBufferSize(pcap_t* pcapHandle)
{
#ifndef WIN32
	CStdString logMsg = "failure";
	int pcapFileno = pcap_fileno(m_pcapHandle);
	size_t bufSize = DLLCONFIG.m_pcapSocketBufferSize;
	if(bufSize < 1)
	{
		return;
	}
	if(pcapFileno)
	{
		if(setsockopt(pcapFileno, SOL_SOCKET, SO_RCVBUF, &bufSize, sizeof(bufSize)) == 0)
		{
			logMsg = "success";		
		}
	}
	logMsg.Format("Setting pcap socket buffer size:%u bytes ... %s", bufSize, logMsg);
	LOG4CXX_INFO(s_packetLog, logMsg);
#endif
}

void VoIp::AddPcapDeviceToMap(CStdString& deviceName, pcap_t* pcapHandle)
{
	MutexSentinel mutexSentinel(m_pcapDeviceMapMutex);

	m_pcapDeviceMap.insert(std::make_pair(pcapHandle, deviceName));
}

void VoIp::RemovePcapDeviceFromMap(pcap_t* pcapHandle)
{
	MutexSentinel mutexSentinel(m_pcapDeviceMapMutex);

	m_pcapDeviceMap.erase(pcapHandle);
}

CStdString VoIp::GetPcapDeviceName(pcap_t* pcapHandle)
{
	MutexSentinel mutexSentinel(m_pcapDeviceMapMutex);
	std::map<pcap_t*, CStdString>::iterator pair;
	CStdString deviceName;

	pair = m_pcapDeviceMap.find(pcapHandle);
	if(pair != m_pcapDeviceMap.end())
	{
		deviceName = pair->second;
	}

	return deviceName;
}

pcap_t* VoIp::OpenDevice(CStdString& name)
{
	char errorBuf[PCAP_ERRBUF_SIZE];
	memset(errorBuf, 0, sizeof(errorBuf));
	char * error = errorBuf;

	MutexSentinel mutexSentinel(m_pcapDeviceMapMutex);
	CStdString logMsg;

	m_pcapHandle = NULL;
	m_pcapHandle = pcap_open_live((char*)name.c_str(), 1500, PROMISCUOUS, 500, errorBuf);

	if(m_pcapHandle)
	{
		error = ApplyPcapFilter();
		if(error == NULL)
		{
			error = errorBuf;
		}
	}
	if(m_pcapHandle == NULL)
	{
		LOG4CXX_ERROR(s_packetLog, CStdString("pcap error when opening device; error message:") + error);
	}
	else
	{
		logMsg.Format("Successfully opened device. pcap handle:%x message:%s", m_pcapHandle, error);
		LOG4CXX_INFO(s_packetLog, logMsg);
		SetPcapSocketBufferSize(m_pcapHandle);
	}

	return m_pcapHandle;
}

void VoIp::OpenDevices()
{
	pcap_if_t* devices = NULL;
	pcap_if_t* defaultDevice = NULL;
	s_lastHooveringTime = time(NULL);
	s_lastPause = time(NULL);
	s_lastPcapStatsReportingTime = time(NULL);
	s_lastPacketsPerSecondTime = time(NULL);
	s_numPackets = 0;
	s_numPacketsPerSecond = 0;
	s_minPacketsPerSecond = 0;
	s_maxPacketsPerSecond = 0;
	m_pcapHandle = NULL;

	CStdString logMsg;

	char errorBuf[PCAP_ERRBUF_SIZE];
	memset(errorBuf, 0, sizeof(errorBuf));
	char * error = errorBuf;

	if (pcap_findalldevs(&devices, errorBuf) == -1)
	{
		LOG4CXX_ERROR(s_packetLog, CStdString("pcap error when discovering devices: ") + error);
	}
	else
	{
		if(devices)
		{
			LOG4CXX_INFO(s_packetLog, CStdString("Available pcap devices:"));

			for (pcap_if_t* device = devices; device != NULL; device = device->next)
			{
				if(!device){break;}

				CStdString description = device->description;
				LOG4CXX_INFO(s_packetLog, CStdString("* ") + device->name + " - " + description);
				CStdString deviceName(device->name);
				deviceName.ToLower();
				if(	deviceName.Find("dialup") == -1		&&			// Don't want Windows dialup devices (still possible to force them using the configuration file)
					deviceName.Find("lo") == -1			&&			// Don't want Unix loopback device
					deviceName.Find("any") == -1			)		// Don't want Unix "any" device
				{
					defaultDevice =  device;
				}
				if(DLLCONFIG.IsDeviceWanted(device->name))
				{
					// Open device
					m_pcapHandle = pcap_open_live(device->name, 1500, PROMISCUOUS, 500, errorBuf);
					
					if(m_pcapHandle)
					{
						error = ApplyPcapFilter();
						if(error == NULL)
						{
							error = errorBuf;
						}
					}
					if(m_pcapHandle == NULL)
					{
						LOG4CXX_ERROR(s_packetLog, CStdString("pcap error when opening device; error message:") + error);
					}
					else
					{
						CStdString logMsg, deviceName;

						deviceName = device->name;
						logMsg.Format("Successfully opened device. pcap handle:%x message:%s", m_pcapHandle, error);
						LOG4CXX_INFO(s_packetLog, logMsg);
						SetPcapSocketBufferSize(m_pcapHandle);

						m_pcapHandles.push_back(m_pcapHandle);
						AddPcapDeviceToMap(deviceName, m_pcapHandle);
					}
				}
			}
			if(m_pcapHandles.size() == 0)
			{
				if(DLLCONFIG.m_devices.size() > 0)
				{
					LOG4CXX_ERROR(s_packetLog, "Could not find any of the devices listed in config file or error, trying default device...");
				}

				// Let's open the default device
				if(defaultDevice)
				{
					m_pcapHandle = pcap_open_live(defaultDevice->name, 1500, PROMISCUOUS, 500, errorBuf);

					if(m_pcapHandle)
					{
						error = ApplyPcapFilter();
						if(error == NULL)
						{
							error = errorBuf;
						}
					}
					if(m_pcapHandle == NULL)
					{
						logMsg.Format("pcap error when opening default device:%s error message:", defaultDevice->name, error);
						LOG4CXX_ERROR(s_packetLog, logMsg);
					}
					else
					{
						CStdString deviceName;

						logMsg.Format("Successfully opened default device:%s pcap handle:%x message:%s", defaultDevice->name, m_pcapHandle, error);
						LOG4CXX_INFO(s_packetLog, logMsg);
						SetPcapSocketBufferSize(m_pcapHandle);

						m_pcapHandles.push_back(m_pcapHandle);
						deviceName = defaultDevice->name;
						AddPcapDeviceToMap(deviceName, m_pcapHandle);
					}
				}
				else
				{
					LOG4CXX_ERROR(s_packetLog, "Could not determine the default device to monitor. If you want a specific device listed above, please specify it in your config file");
				}
			}
		}
		else
		{
			LOG4CXX_ERROR(s_packetLog, CStdString("pcap could not find any device"));
		}
	}
}

void VoIp::ProcessLocalPartyMap(char *line, int ln)
{
	char *oldparty = NULL;
	char *newparty = NULL;
	CStdString logMsg;

	oldparty = line;
	newparty = strchr(line, ',');

	if(!newparty || !oldparty)
	{
		logMsg.Format("ProcessLocalPartyMap: invalid format of line:%d in the local party maps file", ln);
		LOG4CXX_WARN(s_packetLog, logMsg);

		return;
	}

	*(newparty++) = '\0';

	CStdString oldpty, newpty;

	oldpty = oldparty;
	newpty = newparty;

	oldpty.Trim();
	newpty.Trim();

	RtpSessionsSingleton::instance()->SaveLocalPartyMap(oldpty, newpty);
}

void VoIp::LoadPartyMaps()
{
	FILE *maps = NULL;
	char buf[1024];
	int ln = 0;
	CStdString logMsg;

	memset(buf, 0, sizeof(buf));
	maps = fopen(LOCAL_PARTY_MAP_FILE, "r");
	if(!maps)
	{
		logMsg.Format("LoadPartyMaps: Could not open file:%s -- trying:%s now", LOCAL_PARTY_MAP_FILE, ETC_LOCAL_PARTY_MAP_FILE);
		LOG4CXX_INFO(s_packetLog, logMsg);

		maps = fopen(ETC_LOCAL_PARTY_MAP_FILE, "r");
		if(!maps)
		{
			logMsg.Format("LoadPartyMaps: Could not open file:%s either -- giving up", ETC_LOCAL_PARTY_MAP_FILE);
			LOG4CXX_INFO(s_packetLog, logMsg);

			return;
		}
	}

	while(fgets(buf, sizeof(buf), maps))
	{
		ln += 1;

		// Minimum line of x,y\n
		if(strlen(buf) > 4)
		{
			if(buf[strlen(buf)-1] == '\n')
			{
				buf[strlen(buf)-1] = '\0';
			}

			ProcessLocalPartyMap(buf, ln);
		}
	}

	fclose(maps);

	return;
}

void VoIp::Initialize()
{
	m_pcapHandles.clear();
	s_SipTcpStreams.clear();

	s_packetLog = Logger::getLogger("packet");
	s_packetStatsLog = Logger::getLogger("packet.pcapstats");
	s_rtpPacketLog = Logger::getLogger("packet.rtp");
	s_rtcpPacketLog = Logger::getLogger("packet.rtcp");
	s_sipPacketLog = Logger::getLogger("packet.sip");
	s_sipTcpPacketLog = Logger::getLogger("packet.tcpsip");
	s_skinnyPacketLog = Logger::getLogger("packet.skinny");
	s_sipExtractionLog = Logger::getLogger("sipextraction");

	LOG4CXX_INFO(s_packetLog, "Initializing VoIP plugin");

	// create a default config object in case it was not properly initialized by Configure
	if(!g_VoIpConfigTopObjectRef.get())
	{
		g_VoIpConfigTopObjectRef.reset(new VoIpConfigTopObject);
	}

	if(DLLCONFIG.m_pcapFile.size() > 0)
	{
		if(FileCanOpen(DLLCONFIG.m_pcapFile))
		{
			OpenPcapFile(DLLCONFIG.m_pcapFile);
			s_liveCapture = false;
		}
		else
		{
			LOG4CXX_ERROR(s_packetLog, "Could not open pcap file: " + DLLCONFIG.m_pcapFile);
		}
	}
	else if(DLLCONFIG.m_pcapDirectory.size() > 0)
	{
		OpenPcapDirectory(DLLCONFIG.m_pcapDirectory);
	}
	else
	{
		OpenDevices();
		s_liveCapture = true;
	}

	LoadPartyMaps();
}

void VoIp::ReportPcapStats()
{
	for(std::list<pcap_t*>::iterator it = m_pcapHandles.begin(); it != m_pcapHandles.end(); it++)
	{
		struct pcap_stat stats;
		if(*it)
		{
			pcap_stats(*it, &stats);
			CStdString logMsg;
			logMsg.Format("handle:%x received:%u dropped:%u", *it, stats.ps_recv, stats.ps_drop);
			LOG4CXX_INFO(s_packetStatsLog, logMsg)
		}
	}
}


void VoIp::Run()
{
	s_replayThreadCounter = m_pcapHandles.size();

	for(std::list<pcap_t*>::iterator it = m_pcapHandles.begin(); it != m_pcapHandles.end(); it++)
	{
		if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(SingleDeviceCaptureThreadHandler), *it, THR_DETACHED))
		{
			LOG4CXX_INFO(s_packetLog, CStdString("Failed to create pcap capture thread"));
		}
	}
}

void VoIp::Shutdown()
{
	LOG4CXX_INFO(s_packetLog, "Shutting down VoIp.dll");
#ifdef WIN32
	pcap_breakloop(m_pcapHandle);
#endif
}

void VoIp::StartCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid, CStdString& side)
{
	CStdString logMsg;

	logMsg.Format("StartCapture: party:%s orkuid:%s nativecallid:%s side:%s", party, orkuid, nativecallid, side);
	LOG4CXX_INFO(s_voipPluginLog, logMsg);

	MutexSentinel mutexSentinel(s_mutex);

	if(orkuid.size())
	{
		RtpSessionsSingleton::instance()->StartCaptureOrkuid(orkuid, side);
	}
	else if(party.size())
	{
		orkuid = RtpSessionsSingleton::instance()->StartCapture(party, side);
	}
	else if(nativecallid.size())
	{
		orkuid = RtpSessionsSingleton::instance()->StartCaptureNativeCallId(nativecallid, side);
	}
}

void VoIp::PauseCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid)
{
	CStdString logMsg;

	logMsg.Format("PauseCapture: party:%s orkuid:%s nativecallid:%s", party, orkuid, nativecallid);
	LOG4CXX_INFO(s_voipPluginLog, logMsg);

	MutexSentinel mutexSentinel(s_mutex);

	if(orkuid.size())
	{
		RtpSessionsSingleton::instance()->PauseCaptureOrkuid(orkuid);
	}
	else if(party.size())
	{
		orkuid = RtpSessionsSingleton::instance()->PauseCapture(party);
	}
	else if(nativecallid.size())
	{
		orkuid = RtpSessionsSingleton::instance()->PauseCaptureNativeCallId(nativecallid);
	}
}

void VoIp::StopCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid, CStdString& qos)
{
	CStdString logMsg;

	logMsg.Format("StopCapture: party:%s orkuid:%s nativecallid:%s", party, orkuid, nativecallid);
	LOG4CXX_INFO(s_voipPluginLog, logMsg);

	MutexSentinel mutexSentinel(s_mutex);

	if(orkuid.size())
	{
		RtpSessionsSingleton::instance()->StopCaptureOrkuid(orkuid, qos);
	}
	else if(party.size())
	{
		orkuid = RtpSessionsSingleton::instance()->StopCapture(party, qos);
	}
	else if(nativecallid.size())
	{
		orkuid = RtpSessionsSingleton::instance()->StopCaptureNativeCallId(nativecallid, qos);
	}

	logMsg.Format("StopCapture: party:%s orkuid:%s nativecallid:%s qos:%s", party, orkuid, nativecallid, qos);
	LOG4CXX_INFO(s_voipPluginLog, logMsg);
}

void VoIp::SetOnHold(CStdString& port, CStdString& orkuid)
{
	;
}

void VoIp::SetOffHold(CStdString& port, CStdString& orkuid)
{
	;
}

void VoIp::GetConnectionStatus(CStdString& msg)
{
	msg = "unknown";
}

void __CDECL__ Initialize()
{
	VoIpSingleton::instance()->Initialize();
}

void __CDECL__ Run()
{
	VoIpSingleton::instance()->Run();
}

void __CDECL__ Shutdown()
{
	VoIpSingleton::instance()->Shutdown();
}

void __CDECL__ StartCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid, CStdString& side)
{
	VoIpSingleton::instance()->StartCapture(party, orkuid, nativecallid, side);
}

void __CDECL__ PauseCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid)
{
	VoIpSingleton::instance()->PauseCapture(party, orkuid, nativecallid);
}

void __CDECL__ StopCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid, CStdString& qos)
{
	VoIpSingleton::instance()->StopCapture(party, orkuid, nativecallid, qos);
}

void __CDECL__ SetOnHold(CStdString& port, CStdString& orkuid)
{
	;
}

void __CDECL__ SetOffHold(CStdString& port, CStdString& orkuid)
{
	;
}

void __CDECL__  GetConnectionStatus(CStdString& msg)
{
	VoIpSingleton::instance()->GetConnectionStatus(msg);
}

