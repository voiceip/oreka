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

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;
extern OrkLogManager* g_logManager;

#include "LogManager.h"

static LoggerPtr s_packetLog;
static LoggerPtr s_rtpPacketLog;
static LoggerPtr s_sipPacketLog;
static LoggerPtr s_skinnyPacketLog;
static LoggerPtr s_sipExtractionLog;
static LoggerPtr s_voipPluginLog;
static time_t s_lastHooveringTime;
static ACE_Thread_Mutex s_mutex;
static ACE_Thread_Semaphore s_replaySemaphore;
int s_replayThreadCounter;
static bool s_liveCapture;

VoIpConfigTopObjectRef g_VoIpConfigTopObjectRef;
#define DLLCONFIG g_VoIpConfigTopObjectRef.get()->m_config

#define PROMISCUOUS 1

//========================================================
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
	void OpenDevices();
	void OpenPcapFile(CStdString& filename);
	void VoIp::OpenPcapDirectory(CStdString& path);

	pcap_t* m_pcapHandle;
	std::list<pcap_t*> m_pcapHandles;
};

typedef ACE_Singleton<VoIp, ACE_Thread_Mutex> VoIpSingleton;

//=========================================================
// Convert a piece of memory to hex string
void memToHex(unsigned char* input, size_t len, CStdString&output)
{
	char byteAsHex[10];
	for(int i=0; i<len; i++)
	{
		sprintf(byteAsHex, "%.2x", input[i]);
		output += byteAsHex;
	}
}

// find the address that follows the given search string between start and stop pointers - case insensitive
char* memFindAfter(char* toFind, char* start, char* stop)
{
	for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memchr(ptr+1, toFind[0],(stop - start)))
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
			if(rtpHeader->pt <= 34 &&  rtpHeader->pt != 13)		// pt=34 is H263 and is the last possible valid codec 
			{													// pt=13 is CN (Comfort Noise)
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
					CStdString logMsg;
					rtpInfo->ToString(logMsg);
					LOG4CXX_DEBUG(s_rtpPacketLog, logMsg);
				}
				if(payloadLength < 800)		// sanity check, speech RTP payload should always be smaller
				{
					RtpSessionsSingleton::instance()->ReportRtpPacket(rtpInfo);
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


bool TrySipBye(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	bool result = false;
	if (memcmp("BYE", (void*)udpPayload, 3) == 0)
	{
		result = true;
		int sipLength = ntohs(udpHeader->len);
		char* sipEnd = (char*)udpPayload + sipLength;
		SipByeInfo info;
		char* callIdField = memFindAfter("Call-ID: ", (char*)udpPayload, sipEnd);
		if(callIdField)
		{
			GrabToken(callIdField, info.m_callId);
		}
		LOG4CXX_INFO(s_sipPacketLog, "BYE: callid:" + info.m_callId);
		if(callIdField)
		{
			RtpSessionsSingleton::instance()->ReportSipBye(info);
		}
	}
	return result;
}

bool TrySipInvite(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload)
{
	bool result = false;
	bool drop = false;
	if (memcmp("INVITE", (void*)udpPayload, 6) == 0)
	{
		result = true;

		int sipLength = ntohs(udpHeader->len);
		char* sipEnd = (char*)udpPayload + sipLength;

		SipInviteInfoRef info(new SipInviteInfo());

		char* fromField = memFindAfter("From: ", (char*)udpPayload, sipEnd);
		char* toField = memFindAfter("To: ", (char*)udpPayload, sipEnd);
		char* callIdField = memFindAfter("Call-ID: ", (char*)udpPayload, sipEnd);
		char* audioField = NULL;
		char* connectionAddressField = NULL;

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
				GrabAlphaNumToken(sipUser, fromFieldEnd, info->m_from);
			}
			else
			{
				GrabAlphaNumToken(fromField, fromFieldEnd, info->m_from);
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
				GrabAlphaNumToken(sipUser, toFieldEnd, info->m_to);
			}
			else
			{
				GrabAlphaNumToken(toField, toFieldEnd, info->m_to);
			}
		}
		if(callIdField)
		{
			GrabToken(callIdField, info->m_callId);
			audioField = memFindAfter("m=audio ", callIdField, sipEnd);
			connectionAddressField = memFindAfter("c=IN IP4 ", callIdField, sipEnd);
		}
		if(audioField)
		{
			GrabToken(audioField, info->m_fromRtpPort);
		}
		if(connectionAddressField)
		{
			CStdString connectionAddress;
			GrabToken(connectionAddressField, connectionAddress);
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
		if((unsigned int)info->m_fromRtpIp.s_addr == 0)
		{
			// In case connection address could not be extracted, use SIP invite sender IP address
			info->m_fromRtpIp = ipHeader->ip_src;
		}
		info->m_senderIp = ipHeader->ip_src;
		info->m_receiverIp = ipHeader->ip_dest;

		CStdString logMsg;
		info->ToString(logMsg);
		logMsg = "INVITE: " + logMsg;
		LOG4CXX_INFO(s_sipPacketLog, logMsg);

		if(drop == false && info->m_fromRtpPort.size() && info->m_from.size() && info->m_to.size() && info->m_callId.size())
		{
			RtpSessionsSingleton::instance()->ReportSipInvite(info);
		}
	}
	return result;
}

void HandleSkinnyMessage(SkinnyHeaderStruct* skinnyHeader, IpHeaderStruct* ipHeader)
{
	bool useful = true;
	CStdString logMsg;
	
	SkStartMediaTransmissionStruct* startMedia;
	SkStopMediaTransmissionStruct* stopMedia;
	SkCallInfoStruct* callInfo;
	SkOpenReceiveChannelAckStruct* openReceiveAck;
	SkLineStatStruct* lineStat;

	char szEndpointIp[16];
	struct in_addr endpointIp = ipHeader->ip_dest;	// most of the interesting skinny messages are CCM -> phone

	switch(skinnyHeader->messageType)
	{
	case SkStartMediaTransmission:
		startMedia = (SkStartMediaTransmissionStruct*)skinnyHeader;
		if(SkinnyValidateStartMediaTransmission(startMedia))
		{
			if(s_skinnyPacketLog->isInfoEnabled())
			{
				char szRemoteIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&startMedia->remoteIpAddr, szRemoteIp, sizeof(szRemoteIp));
				logMsg.Format(" CallId:%u PassThru:%u media address:%s,%u", startMedia->conferenceId, startMedia->passThruPartyId, szRemoteIp, startMedia->remoteTcpPort);
			}
			RtpSessionsSingleton::instance()->ReportSkinnyStartMediaTransmission(startMedia, ipHeader);
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
		if(s_skinnyPacketLog->isInfoEnabled())
		{
			logMsg.Format(" ConferenceId:%u PassThruPartyId:%u", stopMedia->conferenceId, stopMedia->passThruPartyId);
		}
		RtpSessionsSingleton::instance()->ReportSkinnyStopMediaTransmission(stopMedia, ipHeader);
		break;
	case SkCallInfoMessage:
		callInfo = (SkCallInfoStruct*)skinnyHeader;
		if(SkinnyValidateCallInfo(callInfo))
		{
			if(s_skinnyPacketLog->isInfoEnabled())
			{
				logMsg.Format(" CallId:%u calling:%s called:%s", callInfo->callId, callInfo->callingParty, callInfo->calledParty);
			}
			RtpSessionsSingleton::instance()->ReportSkinnyCallInfo(callInfo, ipHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid CallInfoMessage.");
		}
		break;
	case SkOpenReceiveChannelAck:
		openReceiveAck = (SkOpenReceiveChannelAckStruct*)skinnyHeader;
		if(SkinnyValidateOpenReceiveChannelAck(openReceiveAck))
		{
			if(s_skinnyPacketLog->isInfoEnabled())
			{
				char szMediaIp[16];
				ACE_OS::inet_ntop(AF_INET, (void*)&openReceiveAck->endpointIpAddr, szMediaIp, sizeof(szMediaIp));
				logMsg.Format(" PassThru:%u media address:%s,%u", openReceiveAck->passThruPartyId, szMediaIp, openReceiveAck->endpointTcpPort);
			}
			endpointIp = ipHeader->ip_src;	// this skinny message is phone -> CCM
			RtpSessionsSingleton::instance()->ReportSkinnyOpenReceiveChannelAck(openReceiveAck);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid OpenReceiveChannelAck.");
		}
		break;
	case SkLineStatMessage:
		lineStat = (SkLineStatStruct*)skinnyHeader;
		if(SkinnyValidateLineStat(lineStat))
		{
			if(s_skinnyPacketLog->isInfoEnabled())
			{
				logMsg.Format(" line:%u extension:%s display name:%s", lineStat->lineNumber, lineStat->lineDirNumber, lineStat->displayName);
			}
			endpointIp = ipHeader->ip_dest;	// this skinny message is CCM -> phone
			RtpSessionsSingleton::instance()->ReportSkinnyLineStat(lineStat, ipHeader);
		}
		else
		{
			useful = false;
			LOG4CXX_WARN(s_skinnyPacketLog, "Invalid LineStatMessage.");
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
	EthernetHeaderStruct* ethernetHeader = (EthernetHeaderStruct *)pkt_data;
	IpHeaderStruct* ipHeader = (IpHeaderStruct*)((char*)ethernetHeader + sizeof(EthernetHeaderStruct));
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
					// Still not an IP packet V4, drop it
					return;
				}
			}
		}
	}
	int ipHeaderLength = ipHeader->ip_hl*4;
	u_char* ipPacketEnd = (u_char*)ipHeader + ntohs(ipHeader->ip_len);

#ifdef WIN32
	if(!s_liveCapture)
	{
		// This is a pcap file replay, make sure Orkaudio won't be flooded by too many
		// packets at a time by yielding control to other threads.
		//ACE_Time_Value yield;
		//yield.set(0,1);	// 1 us
		//ACE_OS::sleep(yield);

		// Use nanosleep instead
		struct timespec ts;
		ts.tv_sec = 0;
        ts.tv_nsec = 1;
		ACE_OS::nanosleep (&ts, NULL);
	}
#endif

	if(DLLCONFIG.IsPacketWanted(ipHeader) == false)
	{
		return;
	}

	if(ipHeader->ip_p == IPPROTO_UDP)
	{
		UdpHeaderStruct* udpHeader = (UdpHeaderStruct*)((char *)ipHeader + ipHeaderLength);

		if(	ntohs(udpHeader->source) > 1024 && ntohs(udpHeader->dest) > 1024 )
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

				HandleSkinnyMessage(skinnyHeader, ipHeader);

				// Point to next skinny message within this TCP packet
				skinnyHeader = (SkinnyHeaderStruct*)((u_char*)skinnyHeader + SKINNY_HEADER_LENGTH + skinnyHeader->len);
			}
		}
	}

	time_t now = time(NULL);
	if((now - s_lastHooveringTime) > 5)
	{
		s_lastHooveringTime = now;
		RtpSessionsSingleton::instance()->Hoover(now);
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
			pcap_close(pcapHandle);
		}
		log.Format("Stop Capturing: pcap handle:%x", pcapHandle);
		LOG4CXX_INFO(s_packetLog, log);
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
			LOG4CXX_ERROR(s_voipPluginLog, e);
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
		while(dirEntry = ACE_OS::readdir(dir))
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

void VoIp::OpenPcapFile(CStdString& filename)
{
	CStdString logMsg;

	LOG4CXX_INFO(s_packetLog, CStdString("Adding pcap capture file to replay list:") + filename);

	// Open device
	char * error = NULL;

	if ((m_pcapHandle = pcap_open_offline((PCSTR)filename , error)) == NULL)
	{
		LOG4CXX_ERROR(s_packetLog, "pcap error when opening file:" + filename + "; error message:" + error);
	}
	else
	{
		logMsg.Format("Successfully opened file. pcap handle:%x", m_pcapHandle);
		LOG4CXX_INFO(s_packetLog, logMsg);

		m_pcapHandles.push_back(m_pcapHandle);
	}
}


void VoIp::OpenDevices()
{
	pcap_if_t* devices = NULL;
	pcap_if_t* defaultDevice = NULL;
	s_lastHooveringTime = time(NULL);

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
					if ((m_pcapHandle = pcap_open_live(device->name, 1500, PROMISCUOUS,
							500, error)) == NULL)
					{
						LOG4CXX_ERROR(s_packetLog, CStdString("pcap error when opening device; error message:") + error);
					}
					else
					{
						CStdString logMsg;
						logMsg.Format("Successfully opened device. pcap handle:%x", m_pcapHandle);
						LOG4CXX_INFO(s_packetLog, logMsg);

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


void VoIp::Initialize()
{
	m_pcapHandles.clear();

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
}


void VoIp::Run()
{
	s_replayThreadCounter = m_pcapHandles.size();

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

