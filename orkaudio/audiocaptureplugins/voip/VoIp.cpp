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

#include <list>
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_string.h"
#include "ace/Singleton.h"
#include "ace/Min_Max.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/OS_NS_ctype.h"
#include "ace/Thread_Manager.h"
#include "ace/Thread_Mutex.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "Utils.h"
#include "VoIpConfig.h"
#include "pcap.h"
#include "PacketHeaderDefs.h"
#include "Rtp.h"
#include "RtpSession.h"

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;
extern OrkLogManager* g_logManager;

#include "LogManager.h"

static LoggerPtr s_packetLog;
static LoggerPtr s_rtpPacketLog;
static LoggerPtr s_sipPacketLog;
static LoggerPtr s_skinnyPacketLog;
static LoggerPtr s_sipExtractionLog;
time_t lastHooveringTime;

static ACE_Thread_Mutex s_mutex;

VoIpConfigTopObjectRef g_VoIpConfigTopObjectRef;
#define DLLCONFIG g_VoIpConfigTopObjectRef.get()->m_config

#define PROMISCUOUS 1

// Convert a piece of memnory to hex string
void memToHex(unsigned char* input, size_t len, CStdString&output)
{
	char byteAsHex[10];
	for(int i=0; i<len; i++)
	{
		sprintf(byteAsHex, "%.2x", input[i]);
		output += byteAsHex;
	}
}

// find the address that follows the given search string between start and stop pointers
char* memFindAfter(char* toFind, char* start, char* stop)
{
	for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memchr(ptr+1, toFind[0],(stop - start)))
	{
		if(memcmp(toFind, ptr, strlen(toFind)) == 0)
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

void GrabToken(char* in, CStdString& out)
{
	for(char* c = in; *c != '\0' && *c != 0x20 && *c != 0x0D && *c != 0x0A; c = c+1)
	{
		out += *c;
	}
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

bool TryRtp(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	bool result = false;
	RtpHeaderStruct* rtpHeader = (RtpHeaderStruct*)udpPayload;

	if (rtpHeader->version == 2)
	{
		u_short source = ntohs(udpHeader->source);
		u_short dest = ntohs(udpHeader->dest);
		if(!(ntohs(udpHeader->source)%2) && !(ntohs(udpHeader->dest)%2))	// udp ports must be even 
		{
			if(rtpHeader->pt == RTP_PT_PCMU || rtpHeader->pt == RTP_PT_PCMA)
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

				if(s_rtpPacketLog->isDebugEnabled())
				{
					CStdString debug;
					rtpInfo->ToString(debug);
					LOG4CXX_DEBUG(s_rtpPacketLog, debug);
				}

				RtpSessionsSingleton::instance()->ReportRtpPacket(rtpInfo);
			}
			else
			{
				// unsupported CODEC
				if(s_rtpPacketLog->isDebugEnabled())
				{
					CStdString debug;
					char sourceIp[16];
					ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_src, sourceIp, sizeof(sourceIp));
					char destIp[16];
					ACE_OS::inet_ntop(AF_INET, (void*)&ipHeader->ip_dest, destIp, sizeof(destIp));
					debug.Format("Unsupported codec:%x  src:%s dst:%s", rtpHeader->pt, sourceIp, destIp);
					LOG4CXX_DEBUG(s_rtpPacketLog, debug);
				}
			}
		}
	}
	return result;
}


bool TrySipBye(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	bool result = false;
	if (memcmp("BYE", (void*)udpPayload, 1) == 0)
	{
		result = true;
		int sipLength = ntohs(udpHeader->len);
		char* sipEnd = (char*)udpPayload + sipLength;
		SipByeInfo info;
		char* callIdField = memFindAfter("Call-ID: ", (char*)udpPayload, sipEnd);
		if(callIdField)
		{
			GrabToken(callIdField, info.m_callId);
			RtpSessionsSingleton::instance()->ReportSipBye(info);
		}
		LOG4CXX_DEBUG(s_sipPacketLog, "SIP BYE");
	}
	return result;
}

bool TrySipInvite(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	bool result = false;
	if (memcmp("INVITE", (void*)udpPayload, 1) == 0)
	{
		result = true;

		int sipLength = ntohs(udpHeader->len);
		char* sipEnd = (char*)udpPayload + sipLength;

		SipInviteInfoRef info(new SipInviteInfo());

		char* fromField = memFindAfter("From: ", (char*)udpPayload, sipEnd);
		char* toField = NULL;
		char* callIdField = NULL;
		char* audioField = NULL;

		if(fromField)
		{
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString from;
				GrabLine(fromField, sipEnd, from);
				s_sipExtractionLog->forcedLog(Level::DEBUG, "from: " + from);
			}

			char* fromFieldEnd = memFindEOL(fromField, sipEnd);

			char* sipUser = memFindAfter("sip:", fromField, fromFieldEnd);
			if(sipUser)
			{
				GrabAlphaNumToken(sipUser, fromFieldEnd, info->m_from);
			}
			else
			{
				GrabAlphaNumToken(fromField, fromFieldEnd, info->m_from);
			}
			toField = memFindAfter("To: ", fromField, sipEnd);
		}
		if(toField)
		{
			char* toFieldEnd = NULL;
			if(s_sipExtractionLog->isDebugEnabled())
			{
				CStdString to;
				toFieldEnd = GrabLine(toField, sipEnd, to);
				s_sipExtractionLog->forcedLog(Level::DEBUG, "to: " + to);
			}

			char* sipUser = memFindAfter("sip:", toField, toFieldEnd);
			if(sipUser)
			{
				GrabAlphaNumToken(sipUser, toFieldEnd, info->m_to);
			}
			else
			{
				GrabAlphaNumToken(toField, toFieldEnd, info->m_to);
			}
			callIdField = memFindAfter("Call-ID: ", toField, sipEnd);
		}
		if(callIdField)
		{
			GrabToken(callIdField, info->m_callId);
			audioField = memFindAfter("m=audio ", callIdField, sipEnd);
		}
		if(audioField)
		{
			GrabToken(audioField, info->m_fromRtpPort);
			info->m_fromIp = ipHeader->ip_src;
			RtpSessionsSingleton::instance()->ReportSipInvite(info);
		}
		LOG4CXX_DEBUG(s_sipPacketLog, "SIP INVITE");
	}
	return result;
}

void HandleSkinnyMessage(SkinnyHeaderStruct* skinnyHeader)
{
	bool useful = true;
	CStdString debug;
	SkStartMediaTransmissionStruct* startMedia;
	SkStopMediaTransmissionStruct* stopMedia;
	SkCallInfoStruct* callInfo;

	switch(skinnyHeader->messageType)
	{
	case SkStartMediaTransmission:
		startMedia = (SkStartMediaTransmissionStruct*)skinnyHeader;
		if(s_skinnyPacketLog->isDebugEnabled())
		{
			char szRemoteIp[16];
			ACE_OS::inet_ntop(AF_INET, (void*)&startMedia->remoteIpAddr, szRemoteIp, sizeof(szRemoteIp));
			debug.Format(" CallId:%u %s,%u", startMedia->conferenceId, szRemoteIp, startMedia->remoteTcpPort);
		}
		RtpSessionsSingleton::instance()->ReportSkinnyStartMediaTransmission(startMedia);
		break;
	case SkStopMediaTransmission:
		stopMedia = (SkStopMediaTransmissionStruct*)skinnyHeader;
		if(s_skinnyPacketLog->isDebugEnabled())
		{
			debug.Format(" CallId:%u", stopMedia->conferenceId);
		}
		RtpSessionsSingleton::instance()->ReportSkinnyStopMediaTransmission(stopMedia);
		break;
	case SkCallInfoMessage:
		callInfo = (SkCallInfoStruct*)skinnyHeader;
		if(s_skinnyPacketLog->isDebugEnabled())
		{
			debug.Format(" CallId:%u calling:%s called:%s", callInfo->callId, callInfo->callingParty, callInfo->calledParty);
		}
		RtpSessionsSingleton::instance()->ReportSkinnyCallInfo(callInfo);
		break;
	default:
		useful = false;
	}
	if(useful && s_skinnyPacketLog->isDebugEnabled())
	{
		CStdString msg = SkinnyMessageToString(skinnyHeader->messageType);
		debug = msg + debug;
		LOG4CXX_INFO(s_skinnyPacketLog, debug);
	}
}

void HandlePacket(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	EthernetHeaderStruct* ethernetHeader = (EthernetHeaderStruct *)pkt_data;
	IpHeaderStruct* ipHeader = (IpHeaderStruct*)((char*)ethernetHeader + sizeof(EthernetHeaderStruct));
	int ipHeaderLength = ipHeader->ip_hl*4;
	u_char* ipPacketEnd = (u_char*)ipHeader + ipHeader->ip_len;

	if(ipHeader->ip_p == IPPROTO_UDP)
	{
		UdpHeaderStruct* udpHeader = (UdpHeaderStruct*)((char *)ipHeader + ipHeaderLength);

		if(	ntohs(udpHeader->source) > 5000 && ntohs(udpHeader->dest) > 5000 )
		{
			u_char* udpPayload = (u_char *)udpHeader + sizeof(UdpHeaderStruct);

			MutexSentinel mutexSentinel(s_mutex);		// serialize access for competing pcap threads

			bool detectedUsefulPacket = TryRtp(ethernetHeader, ipHeader, udpHeader, udpPayload);
			
			if(!detectedUsefulPacket)
			{
				detectedUsefulPacket= TrySipInvite(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}
			if(!detectedUsefulPacket)
			{
				detectedUsefulPacket = TrySipBye(ethernetHeader, ipHeader, udpHeader, udpPayload);
			}
		}
	}
	else if(ipHeader->ip_p == IPPROTO_TCP)
	{
		TcpHeaderStruct* tcpHeader = (TcpHeaderStruct*)((char *)ipHeader + ipHeaderLength);
		
		if(ntohs(tcpHeader->source) == SKINNY_CTRL_PORT || ntohs(tcpHeader->dest) == SKINNY_CTRL_PORT)
		{
			u_char* startTcpPayload = (u_char*)tcpHeader + TCP_HEADER_LENGTH;
			SkinnyHeaderStruct* skinnyHeader = (SkinnyHeaderStruct*)(startTcpPayload);

			// Scan all skinny messages in this TCP packet
			while(	ipPacketEnd > (u_char*)skinnyHeader && 
					(u_char*)skinnyHeader>=((u_char*)tcpHeader + TCP_HEADER_LENGTH) &&
					(ipPacketEnd - (u_char*)skinnyHeader) > SKINNY_MIN_MESSAGE_SIZE	&&
					skinnyHeader->len > 1 && skinnyHeader->len < 2048 &&
					skinnyHeader->messageType >= 0x0 && skinnyHeader->messageType <= 0x13F )
			{
				if(s_skinnyPacketLog->isDebugEnabled())
				{
					CStdString dbg;
					unsigned int offset = (u_char*)skinnyHeader - startTcpPayload;
					dbg.Format("Offset:%x Len:%u Type:%x %s", offset, skinnyHeader->len, skinnyHeader->messageType, SkinnyMessageToString(skinnyHeader->messageType));
					LOG4CXX_DEBUG(s_skinnyPacketLog, dbg);
				}
				MutexSentinel mutexSentinel(s_mutex);		// serialize access for competing pcap threads

				HandleSkinnyMessage(skinnyHeader);

				// Point to next skinny message within this TCP packet
				skinnyHeader = (SkinnyHeaderStruct*)((u_char*)skinnyHeader + SKINNY_HEADER_LENGTH + skinnyHeader->len);
			}
		}
	}

	time_t now = time(NULL);
	if((now - lastHooveringTime) > 5)
	{
		lastHooveringTime = now;
		RtpSessionsSingleton::instance()->Hoover(now);
	}
}


void SingleDeviceCaptureThreadHandler(pcap_t* pcapHandle)
{
	if(pcapHandle)
	{
		CStdString log;
		log.Format("Start Capturing: pcap handle:%x", pcapHandle);
		LOG4CXX_INFO(s_packetLog, log);
		pcap_loop(pcapHandle, 0, HandlePacket, NULL);
		log.Format("Stop Capturing: pcap handle:%x", pcapHandle);
		LOG4CXX_INFO(s_packetLog, log);
	}
	else
	{
		LOG4CXX_ERROR(s_packetLog, "Cannot start capturing, pcap handle is null");
	}
}

class VoIp
{
public:
	VoIp();
	void Initialize();
	void Run();
	void Shutdown();
	void StartCapture(CStdString& port);
	void StopCapture(CStdString& port);
private:
	pcap_t* m_pcapHandle;
	std::list<pcap_t*> m_pcapHandles;
};

typedef ACE_Singleton<VoIp, ACE_Thread_Mutex> VoIpSingleton;

VoIp::VoIp()
{
	m_pcapHandle = NULL;
}

void Configure(DOMNode* node)
{
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
			LOG4CXX_WARN(g_logManager->rootLog, "VoIp.dll: " + e);
		}
	}
	else
	{
		LOG4CXX_WARN(g_logManager->rootLog, "VoIp.dll: got empty DOM tree");
	}
}


void VoIp::Initialize()
{
	s_packetLog = Logger::getLogger("packet");
	s_rtpPacketLog = Logger::getLogger("packet.rtp");
	s_sipPacketLog = Logger::getLogger("packet.sip");
	s_skinnyPacketLog = Logger::getLogger("packet.skinny");

	s_sipExtractionLog = Logger::getLogger("sipextraction");
	LOG4CXX_INFO(s_packetLog, "Initializing VoIP plugin");

	// create a default config object in case it was not properly initialized by Configure
	if(!g_VoIpConfigTopObjectRef.get())
	{
		g_VoIpConfigTopObjectRef.reset(new VoIpConfigTopObject);
	}

	pcap_if_t* devices = NULL;
	pcap_if_t* defaultDevice = NULL;
	lastHooveringTime = time(NULL);

	CStdString logMsg;

	char * error = NULL;
	if (pcap_findalldevs(&devices, error) == -1)
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

				LOG4CXX_INFO(s_packetLog, CStdString("* ") + device->description + " " + device->name );

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
					if ((m_pcapHandle = pcap_open_live(device->name, 1500, PROMISCUOUS,
							500, error)) == NULL)
					{
						LOG4CXX_ERROR(s_packetLog, "pcap error when opening device");
					}
					else
					{
						CStdString logMsg;
						logMsg.Format("Successfully opened device. pcap handle:%x", m_pcapHandle);
						LOG4CXX_INFO(s_packetLog, "Successfully opened device");

						m_pcapHandles.push_back(m_pcapHandle);
					}
				}
			}
			if(m_pcapHandles.size() == 0)
			{
				if(DLLCONFIG.m_devices.size() > 0)
				{
					LOG4CXX_ERROR(s_packetLog, "Could not find any of the devices listed in config file");
				}

				// Let's open the default device
				if(defaultDevice)
				{
					if ((m_pcapHandle = pcap_open_live(defaultDevice->name, 1500, PROMISCUOUS,
							500, error)) == NULL)
					{
						logMsg.Format("pcap error when opening default device:%s", defaultDevice->name);
						LOG4CXX_ERROR(s_packetLog, logMsg);
					}
					else
					{
						logMsg.Format("Successfully opened default device:%s pcap handle:%x", defaultDevice->name, m_pcapHandle);
						LOG4CXX_INFO(s_packetLog, logMsg);

						m_pcapHandles.push_back(m_pcapHandle);
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

void VoIp::Run()
{
	for(std::list<pcap_t*>::iterator it = m_pcapHandles.begin(); it != m_pcapHandles.end(); it++)
	{
		if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(SingleDeviceCaptureThreadHandler), *it))
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

void VoIp::StartCapture(CStdString& port)
{
	;
}

void VoIp::StopCapture(CStdString& port)
{
	;
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

void __CDECL__ StartCapture(CStdString& capturePort)
{
	;
}

void __CDECL__ StopCapture(CStdString& capturePort)
{
	;
}

