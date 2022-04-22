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
#include <unistd.h>
#endif

#ifdef WIN32
#define snprintf _snprintf
#endif


#include <list>
#include <stdio.h>
#include <string.h>
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "Utils.h"
#include "VoIpConfig.h"
#include "pcap.h"
#include "PacketHeaderDefs.h"
#include "Rtp.h"
#include "VoIpSession.h"
#include "Iax2Session.h"
#include "SipTcp.h"
#include "Win1251.h"
#include "LogManager.h"
#include "ParsingUtils.h"
#include "SkinnyParsers.h"
#include "SipParsers.h"
#include "Iax2Parsers.h"
#include "SizedBuffer.h"

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;
extern OrkLogManager* g_logManager;

void HandleTcpConnection(int clientSock);
void TcpListenerThread();

static LoggerPtr s_packetLog;
static LoggerPtr s_packetStatsLog;
static LoggerPtr s_rtpPacketLog;
static LoggerPtr s_voipPluginLog;
static LoggerPtr s_rtcpPacketLog;
static time_t s_lastHooveringTime;
static time_t s_lastPause;
static time_t s_lastPacketTimestamp;
static std::mutex s_mutex;
static OrkSemaphore s_replaySemaphore;
int s_replayThreadCounter;
static bool s_liveCapture;
static time_t s_lastPcapStatsReportingTime;
static time_t s_lastPacketsPerSecondTime;
static unsigned int s_numPackets;
static unsigned int s_numPacketsPerSecond;
static unsigned int s_minPacketsPerSecond;
static unsigned int s_maxPacketsPerSecond;
unsigned short utf[256];		//UTF-8 encoding table (partial)
static unsigned int s_udpCounter;
static unsigned int s_numUdpPacketsInUdpMode;
static unsigned int s_numUdpPacketsInUdpMode10s;
static unsigned int s_numLostUdpPacketsInUdpMode;
static unsigned int s_numLostUdpPacketsInUdpMode10s;
static unsigned int s_tcpCounter;
static unsigned int s_numLostTcpPacketsInUdpMode;
static int s_mtuMaxSize;
static int s_tcpListenerSock;

SizedBufferRef HandleIpFragment(IpHeaderStruct* ipHeader);

const int pcap_live_snaplen = 65535;

#define PROMISCUOUS 1
#define LOCAL_PARTY_MAP_FILE	"localpartymap.csv"
#define ETC_LOCAL_PARTY_MAP_FILE "/etc/orkaudio/" LOCAL_PARTY_MAP_FILE
#define SKINNY_GLOBAL_NUMBERS_FILE	"skinnyglobalnumbers.csv"
#define ETC_SKINNY_GLOBAL_NUMBERS_FILE	"/etc/orkaudio/skinnyglobalnumbers.csv"
#define PROT_ERSPAN 0x88be
#define IPPROTO_GRE 47
#define PROT_TRP 0x6558	//Transparent Ethernet Bridging

//========================================================
class PcapHandleData
{
public:
	PcapHandleData(pcap_t* pcaphandle, const char *name);
	CStdString ifName;
	pcap_t* m_pcapHandle;
	time_t m_lastReportTs;
	unsigned int m_numReceived;
	unsigned int m_numReceived10s;
	unsigned int m_numDropped;
	unsigned int m_numDropped10s;
	unsigned int m_numIfDropped;
	unsigned int m_numIfDropped10s;
};
typedef oreka::shared_ptr<PcapHandleData> PcapHandleDataRef;
//========================================================
class VoIp : public OrkSingleton<VoIp>
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
	void ProcessSkinnyGlobalNumbers(char *line, int ln);
	void LoadSkinnyGlobalNumbers();
	void GetConnectionStatus(CStdString& msg);
	void ProcessMetadataMsg(SyncMessage* msg);

private:
	pcap_t* OpenPcapDeviceLive(CStdString name);
	bool ActivatePcapHandle(pcap_t* pcapHandle);
	void OpenDevices();
	void OpenPcapFile(CStdString& filename);
	void OpenPcapDirectory(CStdString& path);
	bool SetPcapSocketBufferSize(pcap_t* pcapHandle);
	char* ApplyPcapFilter(pcap_t* pcapHandle);

	std::map<pcap_t*, PcapHandleDataRef> m_pcapDeviceMap;
	std::mutex m_pcapDeviceMapMutex;
	int64_t m_lastModLocalPartyMapTs;
};

#define VoIpSingleton VoIp
//=========================================================
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
	inet_ntopV4(AF_INET, (void*)&ipHeader->ip_src, sourceIp, sizeof(sourceIp));
	inet_ntopV4(AF_INET, (void*)&ipHeader->ip_dest, destIp, sizeof(destIp));

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

	if(csrcItem->length == 0 || strncasecmp(cname, "ext", ((3 > csrcItem->length) ? csrcItem->length : 3)))
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
	y = strchr(cname, '@');
	if(!y)
	{
		// CNAME is in the "host" or "host:port" format only, no user
		y = strchr(cname, ':');
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
			z = strchr(y, ':');
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

	VoIpSessionsSingleton::instance()->ReportRtcpSrcDescription(info);

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

	if (rtpHeader->version == 2 && rtpHeader->cc == 0 && rtpHeader->p == 0)
	{
		if((!(ntohs(udpHeader->source)%2) && !(ntohs(udpHeader->dest)%2)) || DLLCONFIG.m_rtpDetectOnOddPorts)	// udp ports usually even 
		{
			pair = DLLCONFIG.m_rtpPayloadTypeBlockList.find(rtpHeader->pt);
			if(pair != DLLCONFIG.m_rtpPayloadTypeBlockList.end())
			{
				if(s_rtpPacketLog->isDebugEnabled())
				{
					u_char* payload = rtpHeader->getFirstPayloadByte();
					u_char* packetEnd = (u_char *)ipHeader + ntohs(ipHeader->ip_len);
					if (packetEnd <= payload) return false;
					RtpPacketInfoRef rtpInfo(new RtpPacketInfo());
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
					rtpInfo->m_ssrc = ntohl(rtpHeader->ssrc);
					rtpInfo->m_payload = payload;
					rtpInfo->m_arrivalTimestamp = time(NULL);
					memcpy(rtpInfo->m_sourceMac, ethernetHeader->sourceMac, sizeof(rtpInfo->m_sourceMac));
					memcpy(rtpInfo->m_destMac, ethernetHeader->destinationMac, sizeof(rtpInfo->m_destMac));

					rtpInfo->ToString(logMsg);
					LOG4CXX_DEBUG(s_rtpPacketLog, "Dropped RTP packet with payload type:" + IntToString(rtpHeader->pt) + " " + logMsg);
				}

				return true;
			}

			if((rtpHeader->pt <= 34 &&  rtpHeader->pt != 13) || (rtpHeader->pt >= 96 && rtpHeader->pt < 128) )         
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
						inet_ntopV4(AF_INET, (void*)&ipHeader->ip_src, sourceIp, sizeof(sourceIp));
						char destIp[16];
						inet_ntopV4(AF_INET, (void*)&ipHeader->ip_dest, destIp, sizeof(destIp));
						logMsg.Format("RTP packet filtered by rtpBlockedIpRanges: src:%s dst:%s", sourceIp, destIp);
						LOG4CXX_DEBUG(s_rtpPacketLog, logMsg);
					}
				}
				else
				{
					result = true;
					u_char* payload = (u_char *)rtpHeader->getFirstPayloadByte();
					u_char* packetEnd = (u_char *)ipHeader + ntohs(ipHeader->ip_len);
					if (packetEnd <= payload) return false;
					u_int payloadLength = packetEnd - payload;

					RtpPacketInfoRef rtpInfo(new RtpPacketInfo());
					rtpInfo->m_sourceIp = ipHeader->ip_src;
					rtpInfo->m_destIp =  ipHeader->ip_dest;
					rtpInfo->m_sourcePort = ntohs(udpHeader->source);
					rtpInfo->m_destPort = ntohs(udpHeader->dest);
					rtpInfo->m_payloadType = rtpHeader->pt;
					rtpInfo->m_seqNum = ntohs(rtpHeader->seq);
					rtpInfo->m_timestamp = ntohl(rtpHeader->ts);
					rtpInfo->m_ssrc = ntohl(rtpHeader->ssrc);
					rtpInfo->m_arrivalTimestamp = time(NULL);
					memcpy(rtpInfo->m_sourceMac, ethernetHeader->sourceMac, sizeof(rtpInfo->m_sourceMac));
					memcpy(rtpInfo->m_destMac, ethernetHeader->destinationMac, sizeof(rtpInfo->m_destMac));


                    rtpInfo->m_payloadSize = payloadLength;
                    rtpInfo->m_payload = payload;

					if(s_rtpPacketLog->isDebugEnabled())
					{
						CStdString logMsg;
						rtpInfo->ToString(logMsg);
						LOG4CXX_DEBUG(s_rtpPacketLog, logMsg);
					}
					if(payloadLength < 900)		// sanity check, speech RTP payload should always be smaller
					{
						VoIpSessionsSingleton::instance()->ReportRtpPacket(rtpInfo);
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
					inet_ntopV4(AF_INET, (void*)&ipHeader->ip_src, sourceIp, sizeof(sourceIp));
					char destIp[16];
					inet_ntopV4(AF_INET, (void*)&ipHeader->ip_dest, destIp, sizeof(destIp));
					logMsg.Format("Unsupported codec:%x  src:%s dst:%s", rtpHeader->pt, sourceIp, destIp);
					LOG4CXX_DEBUG(s_rtpPacketLog, logMsg);
				}
			}
		}
	}
	return result;
}

void DetectUsefulUdpPacket(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, int ipHeaderLength, u_char* ipPacketEnd)
{
	UdpHeaderStruct* udpHeader = (UdpHeaderStruct*)((char *)ipHeader + ipHeaderLength);
	if(ntohs(udpHeader->source) >= DLLCONFIG.m_udpMinPort && ntohs(udpHeader->dest) >= DLLCONFIG.m_udpMinPort)
	{
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
			detectedUsefulPacket = TrySipRefer(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
		}

		if(!detectedUsefulPacket) {
			detectedUsefulPacket = TrySipInfo(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
		}

		if(!detectedUsefulPacket) {
			detectedUsefulPacket = TryLogFailedSip(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
		}

		if(!detectedUsefulPacket) {
			if(DLLCONFIG.m_sipCallPickUpSupport == true)
			{
				detectedUsefulPacket= TrySipSubscribe(ethernetHeader, ipHeader, udpHeader, udpPayload, ipPacketEnd);
			}
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

void DetectUsefulTcpPacket(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, int ipHeaderLength, u_char* ipPacketEnd)
{
	bool detectedUsefulPacket = false;
	TcpHeaderStruct* tcpHeader = (TcpHeaderStruct*)((char *)ipHeader + ipHeaderLength);
	if(ntohs(tcpHeader->source) == DLLCONFIG.m_skinnyTcpPort || ntohs(tcpHeader->dest) == DLLCONFIG.m_skinnyTcpPort)
	{
		detectedUsefulPacket = true;
		MutexSentinel mutexSentinel(s_mutex);		// serialize access for competing pcap threads
		ScanAllSkinnyMessages(ethernetHeader, ipHeader, tcpHeader, ipPacketEnd);
	}
	else if(DLLCONFIG.m_sipOverTcpSupport)
	{
		//CStdString tcpSeq;
		//memToHex((unsigned char *)&tcpHeader->seq, 4, tcpSeq);
		MutexSentinel mutexSentinel(s_mutex);		// serialize access for competing pcap threads
		detectedUsefulPacket = TrySipTcp(ethernetHeader, ipHeader, tcpHeader);
	}
	if(!detectedUsefulPacket && DLLCONFIG.m_urlExtractorEnable == true && ntohs(tcpHeader->dest) == DLLCONFIG.m_urlExtractorPort)
	{
		char* startTcpPayload = (char*)tcpHeader + (tcpHeader->off * 4);
		int payloadLength = ntohs(ipHeader->ip_len) - (ipHeader->ip_hl*4) - TCP_HEADER_LENGTH;
		CStdString urlString;
		for(int i=0; i<payloadLength; i++)
		{
			urlString += *startTcpPayload;
			startTcpPayload++;
		}
		if(DLLCONFIG.m_urlExtractorEndpointIsSender == true)
		{
			VoIpSessionsSingleton::instance()->UrlExtraction(urlString, &ipHeader->ip_src);
		}
		else
		{
			VoIpSessionsSingleton::instance()->UrlExtraction(urlString, &ipHeader->ip_dest);
		}
	}

	if(!detectedUsefulPacket && DLLCONFIG.m_onDemandTcpMarkerKey.length() > 0)
	{
		char* startTcpPayload = (char*)tcpHeader + (tcpHeader->off * 4);
		char* patternKey = memFindAfter(DLLCONFIG.m_onDemandTcpMarkerKey, startTcpPayload, (char*)ipPacketEnd);
		if(patternKey != NULL)
		{
			if(DLLCONFIG.m_onDemandTcpMarkerValue.length() > 0)
			{
				char *patternValue = memFindAfter(DLLCONFIG.m_onDemandTcpMarkerValue, patternKey, (char*)ipPacketEnd);
				if(patternValue != NULL)
				{
					VoIpSessionsSingleton::instance()->ReportOnDemandMarkerByIp(ipHeader->ip_src);
				}
			}
			else
			{
				VoIpSessionsSingleton::instance()->ReportOnDemandMarkerByIp(ipHeader->ip_src);
			}
		}
	}
}

bool TryIpPacketV4(IpHeaderStruct* ipHeader)
{
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
						return false;
					}
				}
			}
		}
	}
	return true;
}

/* Ethernet frame types */
#define ETHER_TYPE_IPV4      0x0800 /**< IPv4 Protocol. */
#define ETHER_TYPE_IPV6      0x86DD /**< IPv6 Protocol. */
#define ETHER_TYPE_ARP       0x0806 /**< Arp Protocol. */
#define ETHER_TYPE_IEEE8021Q 0x8100 /**< IEEE 802.1Q VLAN tagging. */

/* VXLAN Ports */
#define IANA_VXLAN_UDP_PORT     4789
#define IANA_VXLAN_GPE_UDP_PORT 4790

void ProcessVXLANPacket(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, int ipHeaderLength, u_char* ipPacketEnd)
{
	//extract inner frame
	u_char* udpPayload = (u_char *)ipHeader + ipHeaderLength + sizeof(UdpHeaderStruct);
	VXLanHeaderStruct* vxlanHeader = (VXLanHeaderStruct*)(udpPayload);

	//Now there would be the ethernet frame
	u_char* framePayload = (u_char *)udpPayload + sizeof(VXLanHeaderStruct);
	EthernetHeaderStruct* encapsulatedEthernetHeader = (EthernetHeaderStruct *)framePayload;
	IpHeaderStruct* encapsulatedIpHeader = NULL;

	if(ntohs(encapsulatedEthernetHeader->type) == ETHER_TYPE_IEEE8021Q)
	{
		encapsulatedIpHeader = (IpHeaderStruct*)((char*)encapsulatedEthernetHeader + sizeof(EthernetHeaderStruct) + 4);
	}
	else if(ntohs(encapsulatedEthernetHeader->type) == ETHER_TYPE_IPV4 || ntohs(encapsulatedEthernetHeader->type) == ETHER_TYPE_ARP)
	{
		encapsulatedIpHeader = (IpHeaderStruct*)((char*)encapsulatedEthernetHeader + sizeof(EthernetHeaderStruct));
	}
	else 
	{   //Maybe corrupted pcap
		return; 
	}

	if(TryIpPacketV4(encapsulatedIpHeader) != true)
	{
		return;
	}

	int encapsulatedIpHeaderLength = encapsulatedIpHeader->headerLen();
	u_char* encapsulatedIpPacketEnd    = reinterpret_cast<unsigned char*>(encapsulatedIpHeader) + encapsulatedIpHeader->packetLen();
	//u_char* encapsulatedIpPacketEnd = (u_char*)encapsulatedIpHeader + ntohs(encapsulatedIpHeader->ip_len);

	if(encapsulatedIpHeader->ip_p == IPPROTO_UDP)
	{
		DetectUsefulUdpPacket(encapsulatedEthernetHeader, encapsulatedIpHeader, encapsulatedIpHeaderLength, encapsulatedIpPacketEnd);
	}
	else if(encapsulatedIpHeader->ip_p == IPPROTO_TCP)
	{
		DetectUsefulTcpPacket(encapsulatedEthernetHeader, encapsulatedIpHeader, encapsulatedIpHeaderLength, encapsulatedIpPacketEnd);
	}
}

void ProcessTransportLayer(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader) {
	size_t ipHeaderLength = ipHeader->headerLen();
	u_char* ipPacketEnd    = reinterpret_cast<unsigned char*>(ipHeader) + ipHeader->packetLen();

	if(ipHeader->ip_p == IPPROTO_UDP)
	{
		UdpHeaderStruct* udpHeader = (UdpHeaderStruct*)((char *)ipHeader + ipHeaderLength);
		if(ntohs(udpHeader->dest) == IANA_VXLAN_UDP_PORT)
		{
			ProcessVXLANPacket(ethernetHeader, ipHeader, ipHeaderLength, ipPacketEnd);
		}
		else 
		{
			DetectUsefulUdpPacket(ethernetHeader, ipHeader, ipHeaderLength, ipPacketEnd);
		}
	}
	else if(ipHeader->ip_p == IPPROTO_TCP)
	{
		DetectUsefulTcpPacket(ethernetHeader, ipHeader, ipHeaderLength, ipPacketEnd);
	}
	else if(ipHeader->ip_p == IPPROTO_GRE)
	{
		//Check if its ESPAN
		GreHeaderStruct *greHeader = (GreHeaderStruct*)((char *)ipHeader + ipHeaderLength);
		if((ntohs(greHeader->flagVersion) == 0x1000 && ntohs(greHeader->protocolType) == PROT_ERSPAN)
		  || (ntohs(greHeader->flagVersion) == 0x2000 && ntohs(greHeader->protocolType) == PROT_TRP) )
		{
			//temporary ignore Erspan payload, flag ...
			//Follow is the real headers got encapsulated
			EthernetHeaderStruct* encapsulatedEthernetHeader;
			if(ntohs(greHeader->protocolType) == PROT_ERSPAN)
			{
				encapsulatedEthernetHeader = (EthernetHeaderStruct *)((char *)ipHeader + ipHeaderLength +  sizeof(GreHeaderStruct) + sizeof(ErspanHeaderStruct));
			}
			else
			{
				encapsulatedEthernetHeader = (EthernetHeaderStruct *)((char *)ipHeader + ipHeaderLength +  sizeof(GreHeaderStruct));
			}
			
			IpHeaderStruct* encapsulatedIpHeader = NULL;

			if(ntohs(encapsulatedEthernetHeader->type) == ETHER_TYPE_IEEE8021Q)
			{
				encapsulatedIpHeader = (IpHeaderStruct*)((char*)encapsulatedEthernetHeader + sizeof(EthernetHeaderStruct) + 4);
			}
			else
			{
				encapsulatedIpHeader = (IpHeaderStruct*)((char*)encapsulatedEthernetHeader + sizeof(EthernetHeaderStruct));
			}

			if(TryIpPacketV4(encapsulatedIpHeader) != true)
			{
				return;
			}

			int encapsulatedIpHeaderLength = ipHeader->ip_hl*4;
			u_char* encapsulatedIpPacketEnd = (u_char*)encapsulatedIpHeader + ntohs(encapsulatedIpHeader->ip_len);

			if(encapsulatedIpHeader->ip_p == IPPROTO_UDP)
			{
				DetectUsefulUdpPacket(encapsulatedEthernetHeader, encapsulatedIpHeader, encapsulatedIpHeaderLength, encapsulatedIpPacketEnd);
			}
			else if(encapsulatedIpHeader->ip_p == IPPROTO_TCP)
			{
				DetectUsefulTcpPacket(encapsulatedEthernetHeader, encapsulatedIpHeader, encapsulatedIpHeaderLength, encapsulatedIpPacketEnd);
			}
		}
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

	if(s_liveCapture && (now - s_lastPcapStatsReportingTime) >= 10)
	{
		MutexSentinel mutexSentinel(s_mutex);		// serialize access for competing pcap threads
		s_lastPcapStatsReportingTime = now;
		VoIpSingleton::instance()->ReportPcapStats();

		CStdString logMsg;
		logMsg.Format("numPackets:%u maxPPS:%u minPPS:%u", s_numPackets, s_maxPacketsPerSecond, s_minPacketsPerSecond);
		LOG4CXX_INFO(s_packetStatsLog, logMsg);
		if(DLLCONFIG.m_orekaEncapsulationMode == true)
		{
			logMsg.Format("udplistener-received:%u received10s:%u dropped:%u dropped10s:%u tcplistener-dropped:%d",
						  s_numUdpPacketsInUdpMode, s_numUdpPacketsInUdpMode10s,
						  s_numLostUdpPacketsInUdpMode, s_numLostUdpPacketsInUdpMode10s,
						  s_numLostTcpPacketsInUdpMode);
			LOG4CXX_INFO(s_packetStatsLog, logMsg);
			s_numUdpPacketsInUdpMode10s = 0;
			s_numLostUdpPacketsInUdpMode10s = 0;
//			s_numLostTcpPacketsInUdpMode = 0;
		}
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

	if(ntohs(ethernetHeader->type) == ETHER_TYPE_IEEE8021Q)
	{
		ipHeader = (IpHeaderStruct*)((char*)ethernetHeader + sizeof(EthernetHeaderStruct) + 4);
	}
	else if(ntohs(ethernetHeader->type) == ETHER_TYPE_IPV4 || ntohs(ethernetHeader->type) == ETHER_TYPE_ARP)
	{
		ipHeader = (IpHeaderStruct*)((char*)ethernetHeader + sizeof(EthernetHeaderStruct));
	}
	else if(ntohs(ethernetHeader->type) == ETHER_TYPE_IPV6)
	{
		return;
	}
	else	//Maybe linux cooked pcap
	{
		//If Linux cooked capture, we arbitrarily align the Ethernet header pointer so that its ETHER_TYPE is aligned with the ETHER_TYPE field of the Linux Cooked header.
		//This means that the source and destination MAC addresses of the obtained Ethernet header are totally wrong, but this is fine, as long as we are aware of this limitation
		ethernetHeader = (EthernetHeaderStruct *)(pkt_data + 2);
		if(ntohs(ethernetHeader->type) == ETHER_TYPE_IEEE8021Q)
		{
			ipHeader = (IpHeaderStruct*)((char*)ethernetHeader + sizeof(EthernetHeaderStruct) + 4);
		}
		else if(ntohs(ethernetHeader->type) == ETHER_TYPE_IPV6)
		{
			return;
		}
		else
		{
			ipHeader = (IpHeaderStruct*)((char*)ethernetHeader + sizeof(EthernetHeaderStruct));
		}
	}

	if(TryIpPacketV4(ipHeader) != true)
	{
		return;
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
					OrkSleepMicrSec(DLLCONFIG.m_pcapFastReplaySleepUsPerSec);
				}
				s_lastPause = now;
			}
			else
			{
				// Make sure Orkaudio won't be flooded by too many
				// packets at a time by yielding control to other threads.
				OrkSleepMicrSec(1);
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
					OrkSleepMs(5);
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

	if (DLLCONFIG.m_ipFragmentsReassemble && ipHeader->isFragmented()) {
		SizedBufferRef packetData = HandleIpFragment(ipHeader);	
		if (packetData) { // Packet data will return non-empty when the packet is complete
			ProcessTransportLayer(ethernetHeader,reinterpret_cast<IpHeaderStruct*>(packetData->get()) );
		}
	}
	else {
		ProcessTransportLayer(ethernetHeader,ipHeader);
	}

	if((now - s_lastHooveringTime) > 5)
	{
		MutexSentinel mutexSentinel(s_mutex);		// serialize access for competing pcap threads
		s_lastHooveringTime = now;
		VoIpSessionsSingleton::instance()->Hoover(now);
		Iax2SessionsSingleton::instance()->Hoover(now);
		VoIpSingleton::instance()->LoadPartyMaps();
	}
}

void SingleDeviceCaptureThreadHandler(pcap_t* pcapHandle)
{
	SetThreadName("orka:v:pcap");

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
			VoIpSessionsSingleton::instance()->StopAll();
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
				pcap_close(pcapHandle);
				while(1)
				{
					OrkSleepSec(60);

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
				pcap_close(pcapHandle);
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


void UdpListenerThread()
{
	OrkAprSubPool locPool;

	SetThreadName("orka:v:udpl");

	CStdString logMsg;

	char frameBuffer[65000];
	apr_int32_t bufSize = DLLCONFIG.m_udpListenerSocketBufferSize;
	apr_status_t ret;
	apr_sockaddr_t* sa;
    apr_socket_t* socket;
	
	apr_sockaddr_info_get(&sa, NULL, APR_INET, DLLCONFIG.m_orekaEncapsulationPort, 0, AprLp);
    apr_socket_create(&socket, sa->family, SOCK_DGRAM, APR_PROTO_UDP, AprLp);

    ret = apr_socket_bind(socket, sa);
    if(ret != APR_SUCCESS){
		logMsg.Format("UdpListenerThread failed to bind to udp port:%d", DLLCONFIG.m_orekaEncapsulationPort);
		LOG4CXX_ERROR(s_packetLog, logMsg);
    }

   	apr_socket_opt_set(socket, APR_SO_NONBLOCK, 0);
	apr_interval_time_t timeout = 100*1000;
    apr_socket_timeout_set(socket, timeout);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_packetLog, "UdpListenerThread failed to set timeout");
	}
	set_socket_buffer_size(s_packetLog, "UdpListenerThread", socket,  bufSize);

	struct pcap_pkthdr pcap_headerPtr ;
	u_char param;
	
	bool stop = false;
	while(stop != true)
	{
		apr_size_t recv_bytes = 65000;
		memset(frameBuffer, 0, 65000);
		apr_socket_recv(socket, frameBuffer, &recv_bytes);
		if(recv_bytes > 0)
		{
			//Need to deencapsulate each packets in this packet
			OrkEncapsulationStruct* orkEncapsulationStruct = (OrkEncapsulationStruct*)frameBuffer;
			if(ntohs(orkEncapsulationStruct->protocol) == 0xbeef && orkEncapsulationStruct->version == 0x01)
			{
				int frameOffset = 0;
				while(frameOffset < (recv_bytes - sizeof(OrkEncapsulationStruct)))
				{
					//deencapsulated this packet
					OrkEncapsulationStruct* orkEncapsulationStruct = (OrkEncapsulationStruct*)(frameBuffer + frameOffset);
					if(ntohs(orkEncapsulationStruct->protocol) == 0xbeef && orkEncapsulationStruct->version == 0x01)
					{

						unsigned int counter = ntohl(orkEncapsulationStruct->seq);
						unsigned short packetLen = ntohs(orkEncapsulationStruct->totalPacketLength);
						if(counter > s_udpCounter)
						{
							s_numLostUdpPacketsInUdpMode    += counter - s_udpCounter - 1;
							s_numLostUdpPacketsInUdpMode10s += counter - s_udpCounter - 1;
						}
						s_numUdpPacketsInUdpMode++;
						s_numUdpPacketsInUdpMode10s++;
						s_udpCounter = counter;
						pcap_headerPtr.caplen = packetLen;

						 if(pcap_headerPtr.caplen > (recv_bytes - frameOffset - sizeof(OrkEncapsulationStruct) + 1))
						{
							break;
						}

						HandlePacket(&param, &pcap_headerPtr, (unsigned char*)frameBuffer + frameOffset + sizeof(OrkEncapsulationStruct));
						frameOffset = frameOffset + packetLen + sizeof(OrkEncapsulationStruct);
					}
					else
					{
						LOG4CXX_ERROR(s_packetLog, "udplistener got alien packet");
						break;
					}

				}

			}
		}

		//In case there is no traffic coming into the socket, Hoover is not going to be called, try to avoid it
		time_t now = time(NULL);
		if((now - s_lastHooveringTime) > 5)
		{
			MutexSentinel mutexSentinel(s_mutex);
			s_lastHooveringTime = now;
			Iax2SessionsSingleton::instance()->Hoover(now);
			VoIpSessionsSingleton::instance()->Hoover(now);
		}

	}
}
//=======================================================
PcapHandleData::PcapHandleData(pcap_t* pcaphandle ,const char *name): ifName(name)
{
	m_pcapHandle = pcaphandle;
	m_lastReportTs = time(NULL);
	m_numReceived = 0;
	m_numReceived10s = 0;
	m_numDropped = 0;
	m_numDropped10s = 0;
	m_numIfDropped = 0;
	m_numIfDropped10s = 0;
}
//=======================================================
VoIp::VoIp()
{
	m_lastModLocalPartyMapTs = 0;

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
	OrkAprSubPool locPool;

	CStdString logMsg;
	std::map<CStdString, CStdString> pcapsMap;
	// Iterate over folder
	apr_status_t ret;
	apr_dir_t* dir;
	ret = apr_dir_open(&dir, (PCSTR)path, AprLp);
	if (ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_packetLog, CStdString("pcap traces directory could not be found:" + path + " please correct this in config.xml"));
	}
	else
	{
		apr_finfo_t finfo;
		apr_int32_t wanted = APR_FINFO_NAME | APR_FINFO_SIZE;
		while((ret = apr_dir_read(&finfo, wanted, dir)) == APR_SUCCESS)
		{	
			CStdString dirEntryFilename;
			dirEntryFilename.Format("%s",finfo.name);
			CStdString pcapExtension = ".pcap";
			int extensionPos = dirEntryFilename.Find(pcapExtension);
			if(extensionPos == -1)
			{
				pcapExtension = ".cap";
				extensionPos = dirEntryFilename.Find(pcapExtension);
			}

			if ( extensionPos != -1 && (dirEntryFilename.size() - extensionPos) == pcapExtension.size() )
			{
				CStdString pcapFilePath = path + "/" + finfo.name;
				if(FileCanOpen(pcapFilePath))
				{
					pcapsMap.insert(std::make_pair(finfo.name, pcapFilePath));
				}
			}
		}

		std::map<CStdString, CStdString>::iterator it;
		for(it = pcapsMap.begin(); it != pcapsMap.end(); it++)
		{
			OpenPcapFile(it->second);
		}

		apr_dir_close(dir);
	}

}

char* VoIp::ApplyPcapFilter(pcap_t* pcapHandle)
{
	struct bpf_program fp;
	char* error = NULL;
	CStdString logMsg;

	if(DLLCONFIG.m_pcapFilter.size())
	{
		if(pcap_compile(pcapHandle,&fp, (PSTR)(PCSTR)DLLCONFIG.m_pcapFilter,1,0) == -1)
		{
			error = pcap_geterr(pcapHandle);
			logMsg.Format("pcap_compile: Please check your PcapFilter in config.xml; pcap handle:%x", pcapHandle);
			LOG4CXX_ERROR(s_packetLog, logMsg);

		} 
		if(error == NULL && pcap_setfilter(pcapHandle,&fp) == -1)
		{ 
			error = pcap_geterr(pcapHandle);
			logMsg.Format("pcap_setfilter: Please check your PcapFilter in config.xml; pcap handle:%x", pcapHandle);
			LOG4CXX_ERROR(s_packetLog, logMsg);
		}
	}
	return error;
}

void VoIp::OpenPcapFile(CStdString& filename)
{
	CStdString logMsg;

	LOG4CXX_INFO(s_packetLog, CStdString("Adding pcap capture file to replay list:") + filename);
	pcap_t* pcapHandle = NULL;
	// Open device
	char * error = NULL;

	pcapHandle = pcap_open_offline((PCSTR)filename , error);

	if(error == NULL)
	{
		error = ApplyPcapFilter(pcapHandle);
	}
	if(error)
	{
		LOG4CXX_ERROR(s_packetLog, "pcap error when opening file:" + filename + "; pcap error:" + error);
	}
	else
	{
		logMsg.Format("Successfully opened file. pcap handle:%x", pcapHandle);
		LOG4CXX_INFO(s_packetLog, logMsg);
		CStdString pcapName = "file";
		AddPcapDeviceToMap(pcapName, pcapHandle);
	}
}

#ifdef CENTOS_5

bool VoIp::SetPcapSocketBufferSize(pcap_t* pcapHandle)
{
	CStdString logMsg = "failure";
	size_t bufSize = 0;
#ifndef WIN32
	int pcapFileno = pcap_fileno(pcapHandle);
	bufSize = DLLCONFIG.m_pcapSocketBufferSize;
	if(bufSize < 1)
	{
		return false;
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
#elif WIN32
	bufSize = DLLCONFIG.m_pcapSocketBufferSize;
	if(bufSize > 0)
	{
		if(pcap_setbuff(pcapHandle, bufSize) == 0)
		{
			logMsg = "success";	
		}
		logMsg.Format("Setting pcap socket buffer size:%u bytes ... %s", bufSize, logMsg);
		LOG4CXX_INFO(s_packetLog, logMsg);
	}
#endif
}

#else

bool VoIp::SetPcapSocketBufferSize(pcap_t* pcapHandle)
{
	bool ret = true;
	CStdString logMsg;
	size_t bufSize = 0;

	int status = 0;
	bufSize = DLLCONFIG.m_pcapSocketBufferSize;
	if(bufSize < 1)
	{
		return ret;
	}

	status = pcap_set_buffer_size(pcapHandle, bufSize);	//66584576 ~64Mb
	if(status == 0)
	{
		logMsg.Format("Setting pcap socket buffer size:%u bytes successful", bufSize);
		LOG4CXX_INFO(s_packetLog, logMsg);
		return ret;

	}
	else
	{
		logMsg.Format("Setting pcap buffer size on pcaphandle:%x failed error:%d", pcapHandle, status);
		LOG4CXX_ERROR(s_packetLog, logMsg);
		return false;
	}
}

bool VoIp::ActivatePcapHandle(pcap_t* pcapHandle)
{
	CStdString logMsg;
	int status = 0;
	status = pcap_activate(pcapHandle);
	if(status < 0)
	{
		logMsg.Format("Activating pcaphandle:%x failed error:%d", pcapHandle, status);
		LOG4CXX_ERROR(s_packetLog, logMsg);
		return false;
	}
	logMsg.Format("Activating pcaphandle:%x successfully", pcapHandle);
	LOG4CXX_INFO(s_packetLog, logMsg);
	#ifndef WIN32
	//Setting SO_RCVBUF size - this proved to help under CentOS6. This is probably not necessary under CentOS7 but does no harm.
		int pcapFileno = pcap_fileno(pcapHandle);
		if(pcapFileno < 0)
		{
			logMsg.Format("Getting pcaphandle:%x descriptor failed", pcapHandle);
			LOG4CXX_ERROR(s_packetLog, logMsg);
			return false;
		}
		size_t bufSize = 0;
		bufSize = 8388608;
		if(setsockopt(pcapFileno, SOL_SOCKET, SO_RCVBUF, &bufSize, sizeof(bufSize)) == 0)
		{
			logMsg.Format("Setting setsockopt with bufsize:%d successfully", bufSize);
			LOG4CXX_INFO(s_packetLog, logMsg);
                	return true;
		}
		else
		{
			logMsg.Format("Setting setsockopt with bufsize:%d failed", bufSize);
			LOG4CXX_ERROR(s_packetLog, logMsg);
                	return false;
		}
	#endif
	return true;
}

#endif
void VoIp::AddPcapDeviceToMap(CStdString& deviceName, pcap_t* pcapHandle)
{
	MutexSentinel mutexSentinel(m_pcapDeviceMapMutex);
	PcapHandleDataRef pcapHandleData(new PcapHandleData(pcapHandle, deviceName));
	m_pcapDeviceMap.insert(std::make_pair(pcapHandle, pcapHandleData));
}

void VoIp::RemovePcapDeviceFromMap(pcap_t* pcapHandle)
{
	MutexSentinel mutexSentinel(m_pcapDeviceMapMutex);

	m_pcapDeviceMap.erase(pcapHandle);
}

CStdString VoIp::GetPcapDeviceName(pcap_t* pcapHandle)
{
	MutexSentinel mutexSentinel(m_pcapDeviceMapMutex);
	CStdString deviceName;

	auto pair = m_pcapDeviceMap.find(pcapHandle);
	if(pair != m_pcapDeviceMap.end())
	{
		deviceName = pair->second->ifName;
	}

	return deviceName;
}

#ifndef CENTOS_5

pcap_t* VoIp::OpenPcapDeviceLive(CStdString name)
{
	char errorBuf[PCAP_ERRBUF_SIZE];
	memset(errorBuf, 0, sizeof(errorBuf));
	char * error = errorBuf;
	CStdString logMsg;
	pcap_t* pcapHandle = NULL;
	int status = 0;

	check_pcap_capabilities(s_packetLog); // verify we have access to the device.
	pcapHandle = pcap_create((char*)name.c_str(), errorBuf);
	if(pcapHandle == NULL)
	{
		error = errorBuf;
		LOG4CXX_ERROR(s_packetLog, CStdString("pcap error when creating pcaphandle; error message:") + error);
		return NULL;
	}
	status = pcap_set_snaplen(pcapHandle, pcap_live_snaplen);
        if(status < 0)
	{
    		logMsg.Format("pcap error when setting snaplen %d; error return:%d", pcap_live_snaplen, status);
		LOG4CXX_ERROR(s_packetLog, logMsg);
		return NULL;
	}
	status = pcap_set_promisc(pcapHandle, PROMISCUOUS);
        if(status < 0)
	{
    		logMsg.Format("pcap error when setting promiscous mode; error return:%d", status);
		LOG4CXX_ERROR(s_packetLog, logMsg);
		return NULL;
	}
	status = pcap_set_timeout(pcapHandle, 500);
    	if(status < 0)
	{
    		logMsg.Format("pcap error when setting timeout; error return:%d", status);
		LOG4CXX_ERROR(s_packetLog, logMsg);
		return NULL;
	}

	if(SetPcapSocketBufferSize(pcapHandle) == false)
	{
		return NULL;
	}
	if(ActivatePcapHandle(pcapHandle) == false)
	{
		return NULL;
	}
        logMsg.Format("Successfully opened device. pcap handle:%x message:%s", pcapHandle, error);
        LOG4CXX_INFO(s_packetLog, logMsg);

	return pcapHandle;
}

#endif

pcap_t* VoIp::OpenDevice(CStdString& name)
{
	CStdString logMsg;
	char errorBuf[PCAP_ERRBUF_SIZE];
	memset(errorBuf, 0, sizeof(errorBuf));
	char * error = errorBuf;
	MutexSentinel mutexSentinel(m_pcapDeviceMapMutex);
	pcap_t* pcapHandle = NULL;
	pcapHandle = OpenPcapDeviceLive(name);

	if(pcapHandle)
	{
		error = ApplyPcapFilter(pcapHandle);
		if(error == NULL)
		{
			error = errorBuf;
		}
		else
		{
			return NULL;
		}
	}
	if(pcapHandle == NULL)
	{
		LOG4CXX_ERROR(s_packetLog, CStdString("pcap error when opening device; error message:") + error);
	}
	else
	{
		logMsg.Format("Successfully opened device. pcap handle:%x message:%s", pcapHandle, error);
		LOG4CXX_INFO(s_packetLog, logMsg);
	}

	return pcapHandle;
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
	pcap_t* pcapHandle = NULL;

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
				if((DLLCONFIG.m_devices.size() > 0 && (*DLLCONFIG.m_devices.begin()).CompareNoCase("all") == 0) || DLLCONFIG.IsDeviceWanted(device->name))
				{
					// Open device
#ifdef CENTOS_5
					pcapHandle = pcap_open_live(device->name, pcap_live_snaplen, PROMISCUOUS, 500, errorBuf);
#else
					pcapHandle = OpenPcapDeviceLive(device->name);
#endif
					
					if(pcapHandle)
					{
						error = ApplyPcapFilter(pcapHandle);
						if(error == NULL)
						{
							error = errorBuf;
						}
						else
						{
							pcapHandle = NULL;
						}
					}
					if(pcapHandle == NULL)
					{
						LOG4CXX_ERROR(s_packetLog, CStdString("pcap error when opening device; error message:") + error);
					}
					else
					{
#ifdef CENTOS_5
						CStdString logMsg, deviceName;

						deviceName = device->name;
						logMsg.Format("Successfully opened device. pcap handle:%x message:%s", pcapHandle, error);
						LOG4CXX_INFO(s_packetLog, logMsg);
						SetPcapSocketBufferSize(pcapHandle);
#endif
						AddPcapDeviceToMap(deviceName, pcapHandle);
					}
				}
			}
			if(m_pcapDeviceMap.size() == 0 && DLLCONFIG.m_orekaEncapsulationMode == false)
			{
				// only use the default handle if we aren't configured for encapsulated UDP traffic
				if(DLLCONFIG.m_devices.size() > 0)
				{
					LOG4CXX_ERROR(s_packetLog, "Could not find any of the devices listed in config file or error, trying default device...");
				}

				// Let's open the default device
				if(defaultDevice)
				{
#ifdef CENTOS_5
					pcapHandle = pcap_open_live((char*)defaultDevice->name, pcap_live_snaplen, PROMISCUOUS, 500, errorBuf);
#else
					pcapHandle = OpenPcapDeviceLive(defaultDevice->name);
#endif

					if(pcapHandle)
					{
						error = ApplyPcapFilter(pcapHandle);
						if(error == NULL)
						{
							error = errorBuf;
						}
						else
						{
							pcapHandle = NULL;
						}
					}
					if(pcapHandle == NULL)
					{
						logMsg.Format("pcap error when opening default device:%s error message:", defaultDevice->name, error);
						LOG4CXX_ERROR(s_packetLog, logMsg);
					}
					else
					{
						CStdString deviceName;

						logMsg.Format("Successfully opened default device:%s pcap handle:%x message:%s", defaultDevice->name, pcapHandle, error);
						LOG4CXX_INFO(s_packetLog, logMsg);
#ifdef CENTOS_5
						SetPcapSocketBufferSize(pcapHandle);
#endif

						deviceName = defaultDevice->name;
						AddPcapDeviceToMap(deviceName, pcapHandle);
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

	VoIpSessionsSingleton::instance()->SaveLocalPartyMap(oldpty, newpty);
}

#ifdef WIN32
#define stat _stat
#endif

void VoIp::LoadPartyMaps()
{
	CStdString logMsg;
	//Separate file operation for file stat, just to not affect the current working stuff
	struct stat finfo;
	CStdString filename = LOCAL_PARTY_MAP_FILE;
	int rt = -1;
	if(stat((PCSTR)filename.c_str(), &finfo) != 0)
	{
		filename = ETC_LOCAL_PARTY_MAP_FILE;
		rt = stat((PCSTR)filename.c_str(), &finfo);
	}
	if(rt == 0)
	{
		if(finfo.st_mtime <= m_lastModLocalPartyMapTs)
		{
			//the file was already loaded and there has been no change since then
			return;
		}		
		else
		{
			logMsg.Format("Detected %s modification, timestamp:%lu oldtimestamp:%lu", LOCAL_PARTY_MAP_FILE, finfo.st_mtime, m_lastModLocalPartyMapTs);
			LOG4CXX_INFO(s_packetLog, logMsg);
			m_lastModLocalPartyMapTs = finfo.st_mtime;
			VoIpSessionsSingleton::instance()->ClearLocalPartyMap();
		}		
	}

	FILE *maps = NULL;
	char buf[1024];
	int ln = 0;

	memset(buf, 0, sizeof(buf));
	maps = fopen(LOCAL_PARTY_MAP_FILE, "r");
	if(!maps)
	{
		maps = fopen(ETC_LOCAL_PARTY_MAP_FILE, "r");
		if(!maps)
		{
			logMsg.Format("No %s supplied, either locally or at %s", LOCAL_PARTY_MAP_FILE, ETC_LOCAL_PARTY_MAP_FILE);
			LOG4CXX_INFO(s_packetLog, logMsg);

			return;
		}
	}
	logMsg.Format("Loading %s", LOCAL_PARTY_MAP_FILE);
	LOG4CXX_INFO(s_packetLog, logMsg);

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

void VoIp::ProcessSkinnyGlobalNumbers(char *line, int ln)
{
	char *number = NULL;
	CStdString logMsg;

	number = line;

	if(!number)
	{
		logMsg.Format("ProcessSkinnyGlobalNumbers: invalid format of line:%d in the skinny global numbers file", ln);
		LOG4CXX_WARN(s_packetLog, logMsg);
		return;
	}

	CStdString num;
	num = number;
	num.Trim();
	VoIpSessionsSingleton::instance()->SaveSkinnyGlobalNumbersList(num);
}

void VoIp::LoadSkinnyGlobalNumbers()
{
	FILE *maps = NULL;
	char buf[128];
	int ln = 0;
	CStdString logMsg;

	memset(buf, 0, sizeof(buf));
	maps = fopen(SKINNY_GLOBAL_NUMBERS_FILE, "r");
	if(!maps)
	{
		logMsg.Format("LoadSkinnyGlobalNumbersList: Could not open file:%s -- trying:%s now", SKINNY_GLOBAL_NUMBERS_FILE, ETC_SKINNY_GLOBAL_NUMBERS_FILE);
		LOG4CXX_INFO(s_packetLog, logMsg);

		maps = fopen(ETC_SKINNY_GLOBAL_NUMBERS_FILE, "r");
		if(!maps)
		{
			logMsg.Format("LoadPartyMaps: Could not open file:%s either -- giving up", ETC_SKINNY_GLOBAL_NUMBERS_FILE);
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

			ProcessSkinnyGlobalNumbers(buf, ln);
		}
	}

	fclose(maps);

	return;
}

void VoIp::Initialize()
{
	m_pcapDeviceMap.clear();

	s_packetLog = Logger::getLogger("packet");
	s_packetStatsLog = Logger::getLogger("packet.pcapstats");
	s_rtpPacketLog = Logger::getLogger("packet.rtp");
	s_rtcpPacketLog = Logger::getLogger("packet.rtcp");

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

	InitializeWin1251Table(utf);
	LoadPartyMaps();
	LoadSkinnyGlobalNumbers();
	s_mtuMaxSize = DLLCONFIG.m_mtuMaxSize;
}

void VoIp::ReportPcapStats()
{
	MutexSentinel mutexSentinel(m_pcapDeviceMapMutex);
	for(auto it = m_pcapDeviceMap.begin(); it != m_pcapDeviceMap.end(); it++)
	{
		struct pcap_stat stats;
		if((it->second).get())
		{
			PcapHandleDataRef pcapHandleData = it->second;
			pcap_stats(pcapHandleData->m_pcapHandle, &stats);
			CStdString logMsg;

			if((time(NULL) - pcapHandleData->m_lastReportTs) >= 10)
			{
				pcapHandleData->m_numReceived10s = stats.ps_recv - pcapHandleData->m_numReceived;
				pcapHandleData->m_numDropped10s = stats.ps_drop - pcapHandleData->m_numDropped;
				pcapHandleData->m_numIfDropped10s = stats.ps_ifdrop - pcapHandleData->m_numIfDropped;
				pcapHandleData->m_numReceived = stats.ps_recv;
				pcapHandleData->m_numDropped = stats.ps_drop;
				pcapHandleData->m_numIfDropped = stats.ps_ifdrop;
				logMsg.Format("%s: handle:%x received:%u received10s:%u dropped:%u dropped10s:%u ifdropped:%u ifdropped10s:%u",
							   pcapHandleData->ifName,pcapHandleData->m_pcapHandle, stats.ps_recv, pcapHandleData->m_numReceived10s,
							   stats.ps_drop, pcapHandleData->m_numDropped10s, pcapHandleData->m_numIfDropped, pcapHandleData->m_numIfDropped10s);
				 LOG4CXX_INFO(s_packetStatsLog, logMsg);
			} else {
				logMsg.Format("handle:%x received:%u dropped:%u", pcapHandleData->m_pcapHandle, stats.ps_recv, stats.ps_drop);
				LOG4CXX_INFO(s_packetStatsLog, logMsg);
			}
		}
	}
}


void VoIp::Run()
{
	CStdString logMsg;
	s_replayThreadCounter = m_pcapDeviceMap.size();

	if(DLLCONFIG.m_orekaEncapsulationMode == true)
	{
		try{
			std::thread handler(UdpListenerThread);
			handler.detach();
		} catch(const std::exception &ex){
			logMsg.Format("Failed to start UdpListenerThread thread reason:%s",  ex.what());
			LOG4CXX_ERROR(s_packetLog, logMsg);	
		}
#ifndef WIN32
		try{
			std::thread handler(TcpListenerThread);
			handler.detach();
		} catch(const std::exception &ex){
			logMsg.Format("Failed to start TcpListenerThread thread reason:%s",  ex.what());
			LOG4CXX_ERROR(s_packetLog, logMsg);	
		}
#endif
	}

	for(auto it = m_pcapDeviceMap.begin(); it != m_pcapDeviceMap.end(); it++)
	{
		try{
			std::thread handler(SingleDeviceCaptureThreadHandler, it->first);
			handler.detach();
		} catch(const std::exception &ex){
			logMsg.Format("Failed to start SingleDeviceCaptureThreadHandler thread reason:%s",  ex.what());
			LOG4CXX_ERROR(s_packetLog, logMsg);	
		}
	}

}

void VoIp::Shutdown()
{
	LOG4CXX_INFO(s_packetLog, "Shutting down VoIp.dll");
#ifdef WIN32
	for (auto it = m_pcapDeviceMap.begin(); it != m_pcapDeviceMap.end(); it++) {
		if (it->first) {
			pcap_breakloop(it->first);
		}
	}
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
		VoIpSessionsSingleton::instance()->StartCaptureOrkuid(orkuid, side);
	}
	else if(party.size())
	{
		orkuid = VoIpSessionsSingleton::instance()->StartCapture(party, side);
	}
	else if(nativecallid.size())
	{
		orkuid = VoIpSessionsSingleton::instance()->StartCaptureNativeCallId(nativecallid, side);
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
		VoIpSessionsSingleton::instance()->PauseCaptureOrkuid(orkuid);
	}
	else if(party.size())
	{
		orkuid = VoIpSessionsSingleton::instance()->PauseCapture(party);
	}
	else if(nativecallid.size())
	{
		orkuid = VoIpSessionsSingleton::instance()->PauseCaptureNativeCallId(nativecallid);
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
		VoIpSessionsSingleton::instance()->StopCaptureOrkuid(orkuid, qos);
	}
	else if(party.size())
	{
		orkuid = VoIpSessionsSingleton::instance()->StopCapture(party, qos);
	}
	else if(nativecallid.size())
	{
		orkuid = VoIpSessionsSingleton::instance()->StopCaptureNativeCallId(nativecallid, qos);
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

void VoIp::ProcessMetadataMsg(SyncMessage* msg)
{
	;
}
//================================================================================
#ifndef WIN32
void HandleTcpConnection(int clientSock)
{
	SetThreadName("orka:v:tcp");

	CStdString logMsg;
	unsigned char keepAliveBuf[4];
	time_t keepAliveTimer = time(NULL);
	unsigned char buf[s_mtuMaxSize];
	buf[s_mtuMaxSize] = '\0';	// security
	bool stop =false;
	unsigned char prot[3] = {0xbe, 0xef, 0x1};
	OrkEncapsulationStruct* orkEncapsulationHeader;

	while(stop == false)
	{
		int wantedLen = 0;
		int recvLen = 0;
		bool isFullLen =  true;
		int bufferPos = 0;
		memset(buf, 0, s_mtuMaxSize);
		//first try to see if this packet has our own OrkEncapsulation header
		ssize_t size = recv(clientSock, buf, sizeof(OrkEncapsulationStruct), 0);
		bufferPos = bufferPos + size;
		wantedLen = sizeof(OrkEncapsulationStruct);
		isFullLen = true;
		
		while(size != wantedLen)		// while() read header
		{
			if(size > wantedLen)
			{
				logMsg.Format("received %d bytes but was expecting %d", size, wantedLen);
				LOG4CXX_ERROR(s_packetLog, logMsg);
				stop = true; //terminate thread to reconnect
				break;
			}
			else if(size == 0)
			{
				logMsg.Format("remote socket was close");
				LOG4CXX_ERROR(s_packetLog, logMsg);
				stop = true;
				break;
			}
			else if(size < 0)
			{
				logMsg.Format("tcplistener returned error:%s ", strerror(errno));
				LOG4CXX_ERROR(s_packetLog, logMsg);
				stop = true;
				break;
			}

			recvLen = size;
			wantedLen = wantedLen - recvLen;

			size = recv(clientSock, &buf[bufferPos], wantedLen, 0);
			bufferPos = bufferPos + size;
		}	// while() read header
		//Now validate the header. At here, we must have get sizeof(OrkEncapsulationStruct) length buffer

		if(bufferPos != sizeof(OrkEncapsulationStruct))
		{
			logMsg.Format("tcplistener bufferPos != sizeof oreka header ");
			LOG4CXX_ERROR(s_packetLog, logMsg);
			break;
		}
		bufferPos = sizeof(OrkEncapsulationStruct);

		orkEncapsulationHeader = (OrkEncapsulationStruct*)buf;
		if((ntohs(orkEncapsulationHeader->protocol) != 0xbeef) || (orkEncapsulationHeader->version != 0x1))
		{
			logMsg.Format("tcplistener invalid header");
			LOG4CXX_ERROR(s_packetLog, logMsg);
			stop = true;
			break; //header is invalid,
		}
		if(ntohs(orkEncapsulationHeader->totalPacketLength) > (s_mtuMaxSize -sizeof(OrkEncapsulationStruct)))
		{
			logMsg.Format("tcplistener skips overlimit payload length:%d", ntohs(orkEncapsulationHeader->totalPacketLength));
			LOG4CXX_ERROR(s_packetLog, logMsg);
			stop = true; //terminate thread to reconnect
			break;
		}
		//Now try to read actualy payload
		//at this point: bufferPos must equal sizeof(OrkEncapsulationStruct)
		size = recv(clientSock, &buf[bufferPos], ntohs(orkEncapsulationHeader->totalPacketLength), 0);
		bufferPos = bufferPos + size;
		wantedLen = ntohs(orkEncapsulationHeader->totalPacketLength);
		isFullLen = true;
		while(size != wantedLen)		//while() payload read
		{
			if(size > wantedLen)
			{
				logMsg.Format("received %d bytes but was expecting %d", size, wantedLen);
				LOG4CXX_ERROR(s_packetLog, logMsg);
				stop = true; //terminate thread to reconnect
				break;
			}
			else if(size == 0)
			{
				logMsg.Format("remote socket was close");
				LOG4CXX_ERROR(s_packetLog, logMsg);
				stop = true;
				break;
			}
			else if(size < 0)
			{
				logMsg.Format("tcplistener returned error:%s ", strerror(errno));
				LOG4CXX_ERROR(s_packetLog, logMsg);
				stop = true;
				break;
			}

			recvLen = size;
			wantedLen = wantedLen - recvLen;

			size = recv(clientSock, &buf[bufferPos], wantedLen, 0);
			bufferPos = bufferPos + size;

		}	//while() payload read

		if(bufferPos != (sizeof(OrkEncapsulationStruct) + ntohs(orkEncapsulationHeader->totalPacketLength)))
		{
	        logMsg.Format("tcplistener bufferPos:%d expected:%d", bufferPos, sizeof(OrkEncapsulationStruct) + ntohs(orkEncapsulationHeader->totalPacketLength));
			LOG4CXX_ERROR(s_packetLog, logMsg);
			break;
		}

		//at this point, we must have full payload
		struct pcap_pkthdr pcap_headerPtr ;
		pcap_headerPtr.caplen = ntohs(orkEncapsulationHeader->totalPacketLength);
		u_char param;
		HandlePacket(&param, &pcap_headerPtr, &buf[sizeof(OrkEncapsulationStruct)]);

		//Keep alive: just to kill the thread, longer timer is not a problem
		if((time(NULL) - keepAliveTimer) > 5)
		{
			int sendRet;
			sendRet = send(clientSock, keepAliveBuf, 4, 0);
			if(sendRet != 4)
			{
				logMsg.Format("HandleTcpConnectionThread is terminated due to tcp client disconnection on socket:%d", clientSock);
				LOG4CXX_ERROR(s_packetLog, logMsg);
				break;
			}
			else
			{
				keepAliveTimer = time(NULL);
			}
		}
	}//Outer while()
	close(clientSock);
}

void TcpListenerThread()
{
	SetThreadName("orka:v:tcpl");

	static struct sockaddr_in hostTcpAddr;
	CStdString logMsg;
	LOG4CXX_INFO(s_packetLog, "TcpListenerThread is initialized successfully");
	struct sockaddr_in remoteAddr;
	memset(&hostTcpAddr, 0, sizeof(hostTcpAddr));
	hostTcpAddr.sin_family = AF_INET;
	CStdString localAddr = DLLCONFIG.m_orekaEncapsulationHost;
	int localPort = DLLCONFIG.m_orekaEncapsulationPort + 1;
	inet_pton(AF_INET, localAddr, &(hostTcpAddr.sin_addr));
	hostTcpAddr.sin_port = htons(localPort);
	s_tcpListenerSock = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(s_tcpListenerSock < 0)
	{
		LOG4CXX_ERROR(s_packetLog, "Failed to created tcp listener socket");
		return;
	}

	int bindRet, listenRet, clientSock;
	bindRet = bind(s_tcpListenerSock, (struct sockaddr*)&hostTcpAddr, sizeof(hostTcpAddr));
	while(bindRet < 0)
	{
		LOG4CXX_ERROR(s_packetLog, "Failed to bind tcp listener socket. Retrying ...");
		bindRet = bind(s_tcpListenerSock, (struct sockaddr*)&hostTcpAddr, sizeof(hostTcpAddr));
		sleep(5);
	}

	listenRet = listen(s_tcpListenerSock, 5);
	while(listenRet < 0)
	{
		LOG4CXX_ERROR(s_packetLog, "Failed to listen tcp listener socket. Retrying ...");
		listenRet = listen(s_tcpListenerSock, 5);
		sleep(5);
	}
	for(;;)
	{
		unsigned int remoteLen = sizeof(remoteAddr);
		clientSock = accept(s_tcpListenerSock, (struct sockaddr*)&remoteAddr, &remoteLen);
		while(clientSock < 0)
		{
			LOG4CXX_ERROR(s_packetLog, "Failed to accept tcp connection. Retrying ...");
			clientSock = accept(s_tcpListenerSock, (struct sockaddr*)&remoteAddr, &remoteLen);
			sleep(5);
		}
		logMsg.Format("Accepted incomming connection from:%s on socket:%d",inet_ntoa(remoteAddr.sin_addr), clientSock);
		LOG4CXX_INFO(s_packetLog, logMsg);
		try{
			std::thread handler(HandleTcpConnection, clientSock);
			handler.detach();
		} catch(const std::exception &ex){
			logMsg.Format("Failed to start HandleTcpConnection thread reason:%s",  ex.what());
			LOG4CXX_ERROR(LOG.rootLog, logMsg);	
		}

	}

}

#endif
//================================================================================
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

void __CDECL__  ProcessMetadataMsg(SyncMessage* msg)
{
	VoIpSingleton::instance()->ProcessMetadataMsg(msg);
}
