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

#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_string.h"
#include "ace/Singleton.h"
#include "ace/Min_Max.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/OS_NS_ctype.h"
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
extern LogManager* g_logManager;

#include "LogManager.h"

static LoggerPtr s_log;
static LoggerPtr s_sipExtractionLog;
time_t lastHooveringTime;

VoIpConfigTopObjectRef g_VoIpConfigTopObjectRef;
#define DLLCONFIG g_VoIpConfigTopObjectRef.get()->m_config

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

				CStdString debug;
				debug.Format("%s,%d seq:%u ts:%u len:%d", ACE_OS::inet_ntoa(rtpInfo->m_sourceIp), rtpInfo->m_sourcePort, ntohs(rtpHeader->seq), ntohl(rtpHeader->ts), payloadLength);
				LOG4CXX_DEBUG(s_log, debug);

				RtpSessionsSingleton::instance()->ReportRtpPacket(rtpInfo);
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
		LOG4CXX_DEBUG(s_log, "SIP BYE");
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
		LOG4CXX_DEBUG(s_log, "SIP INVITE");
	}
	return result;
}

void HandlePacket(u_char *param, const struct pcap_pkthdr *header, const u_char *pkt_data)
{
	EthernetHeaderStruct* ethernetHeader = (EthernetHeaderStruct *)pkt_data;
	IpHeaderStruct* ipHeader = (IpHeaderStruct*)((char*)ethernetHeader + sizeof(EthernetHeaderStruct));
	int ipHeaderLength = ipHeader->ip_hl*4;

	//CStdString source = ACE_OS::inet_ntoa(ipHeader->ip_src);
	//CStdString dest = ACE_OS::inet_ntoa(ipHeader->ip_dest);
	//CStdString debug;
	//debug.Format("%x, %x", ethernetHeader, ipHeader);
	//LOG4CXX_INFO(s_log, source + "-" + dest);

	//LOG4CXX_DEBUG(s_log, "*");

	if(ipHeader->ip_p == IPPROTO_UDP)
	{
		UdpHeaderStruct* udpHeader = (UdpHeaderStruct*)((char *)ipHeader + ipHeaderLength);

		if(	ntohs(udpHeader->source) > 5000 && ntohs(udpHeader->dest) > 5000 )
		{
			u_char* udpPayload = (u_char *)udpHeader + sizeof(UdpHeaderStruct);


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

	time_t now = time(NULL);
	if((now - lastHooveringTime) > 5)
	{
		lastHooveringTime = now;
		RtpSessionsSingleton::instance()->Hoover(now);
	}
}


class VoIp
{
public:
	VoIp();
	void Initialize();
	void Run();
	void StartCapture(CStdString& port);
	void StopCapture(CStdString& port);
private:
	pcap_t* m_pcapHandle;
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
	s_log = Logger::getLogger("voip");
	s_sipExtractionLog = Logger::getLogger("sipextraction");
	LOG4CXX_INFO(s_log, "Initializing VoIP plugin");

	// create a default config object in case it was not properly initialized by Configure
	if(!g_VoIpConfigTopObjectRef.get())
	{
		g_VoIpConfigTopObjectRef.reset(new VoIpConfigTopObject);
	}

/*
	char addr1[] = "10.1.2.3";
	char addr2[] = "192.168.5.6";
	char addr3[] = "45.168.5.6";
	struct in_addr addr;
	ACE_OS::inet_aton(addr1, &addr);
	bool result = DLLCONFIG.IsPartOfLan(addr);
	result = DLLCONFIG.IsMediaGateway(addr);
	ACE_OS::inet_aton(addr2, &addr);
	result = DLLCONFIG.IsPartOfLan(addr);
	result = DLLCONFIG.IsMediaGateway(addr);
	ACE_OS::inet_aton(addr3, &addr);
	result = DLLCONFIG.IsPartOfLan(addr);
	result = DLLCONFIG.IsMediaGateway(addr);
*/

	pcap_if_t* devices = NULL;
	pcap_if_t* lastDevice = NULL;
	pcap_if_t* deviceToSniff = NULL;
	lastHooveringTime = time(NULL);

	char * error = NULL;
	if (pcap_findalldevs(&devices, error) == -1)
	{
		LOG4CXX_ERROR(s_log, CStdString("pcap error when discovering devices: ") + error);
	}
	else
	{
		if(devices)
		{
			LOG4CXX_INFO(s_log, CStdString("Available pcap devices:"));

			for (pcap_if_t* device = devices; device != NULL; device = device->next)
			{
				LOG4CXX_INFO(s_log, CStdString("\t* ") + device->description + " " + device->name );
				if(DLLCONFIG.m_device.Equals(device->name))
				{
					deviceToSniff = device;
				}
				if(device)
				{
					lastDevice = device;
				}
			}
			if (!deviceToSniff)
			{
				if(!DLLCONFIG.m_device.IsEmpty())
				{
					LOG4CXX_ERROR(s_log, CStdString("pcap could not find wanted device: ") + DLLCONFIG.m_device + " please check your config file");
				}
				if(lastDevice)
				{
					LOG4CXX_INFO(s_log, CStdString("Defaulting to device: ") + lastDevice->name);
					deviceToSniff = lastDevice;
				}
			}
			if (deviceToSniff)
			{
				#define PROMISCUOUS 1
				if ((m_pcapHandle = pcap_open_live(deviceToSniff->name, 1500, PROMISCUOUS,
					 500, error)) == NULL)
				{
					LOG4CXX_ERROR(s_log, CStdString("pcap error when opening device: ") + deviceToSniff->name);
				}
				else
				{
					LOG4CXX_INFO(s_log, CStdString("successfully opened device: ") + deviceToSniff->name);
				}
			}
		}
		else
		{
			LOG4CXX_ERROR(s_log, CStdString("pcap could not find any device"));
		}
	}

}

void VoIp::Run()
{
	if(m_pcapHandle)
	{
		LOG4CXX_INFO(s_log, "Running VoIp.dll");
		pcap_loop(m_pcapHandle, 0, HandlePacket, NULL);
	}
	else
	{
		LOG4CXX_INFO(s_log, "No network device opened - VoIp.dll not starting");
	}
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

void __CDECL__ StartCapture(CStdString& capturePort)
{
	;
}

void __CDECL__ StopCapture(CStdString& capturePort)
{
	;
}

