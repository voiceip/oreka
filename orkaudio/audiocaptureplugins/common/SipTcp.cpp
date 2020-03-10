
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#define _WINSOCKAPI_// prevents the inclusion of winsock.h
#endif

#include <list>
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "Utils.h"
#include "pcap.h"
#include "PacketHeaderDefs.h"
#include "Rtp.h"
#include "SipTcp.h"
#include "shared_ptr.h"
#include "StdString.h"
#include "SipTcp.h"
#include <boost/algorithm/string/predicate.hpp>


static LoggerPtr s_sipTcpPacketLog = Logger::getLogger("packet.tcpsip");

// ============================================================

static char* memFindAfter(const char* toFind, char* start, char* stop)
{
        for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memchr(ptr+1, toFind[0],(stop - ptr - 1)))
        {
                if(strncasecmp(toFind, ptr, strlen(toFind)) == 0)
                {
                        return (ptr+strlen(toFind));
                }
        }
        return NULL;
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

// Implementation of strcasestr() - works like strstr() but
// is case insensitive
char* memFindStr(const char* toFind, char* start, char* stop)
{
        for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memnchr(ptr+1, toFind[0],(stop - ptr - 1)))
        {
                if(strncasecmp(toFind, ptr, strlen(toFind)) == 0)
                {
                        return (ptr);
                }
        }
        return NULL;
}

static char* memFindEOL(char* start, char* limit)
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

static void memToHex(unsigned char* input, size_t len, CStdString&output)
{
        char byteAsHex[10];
        for(unsigned int i=0; i<len; i++)
        {
                sprintf(byteAsHex, "%.2x", input[i]);
                output += byteAsHex;
        }
}

SipTcpStream::SipTcpStream() : m_offset(0)
{
	m_expectingSeqNo = 0;
	m_senderIp.s_addr = 0;
	m_receiverIp.s_addr = 0;
	m_senderPort = 0;
	m_receiverPort = 0;
	m_entryTime = time(NULL);
	m_tcpBuffer = SafeBufferRef(new SafeBuffer());
}

SipTcpStream::~SipTcpStream()
{
}

void SipTcpStream::ToString(CStdString& string)
{
        char senderIp[16], receiverIp[16];
	CStdString expSeq, lastSeq;

	memToHex((unsigned char *)&m_expectingSeqNo, sizeof(m_expectingSeqNo), expSeq);
	memToHex((unsigned char *)&m_lastSeqNo, sizeof(m_lastSeqNo), lastSeq);

        inet_ntopV4(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
        inet_ntopV4(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	//string.Format("sender:%s receiver:%s sender-port:%d receiver-port:%d entry-time:%d expecting-seq-no:%s total-bytes:%d last-seqno:%s [[[%s]]]", senderIp, receiverIp, m_senderPort, m_receiverPort, m_entryTime, expSeq, m_tcpBuffer->Size(), lastSeq, m_tcpBuffer->GetBuffer());
	string.Format("sender:%s receiver:%s sender-port:%d receiver-port:%d entry-time:%d expecting-seq-no:%s total-bytes:%d last-seqno:%s", senderIp, receiverIp, m_senderPort, m_receiverPort, m_entryTime, expSeq, m_tcpBuffer->Size(), lastSeq);
}

void SipTcpStream::AddTcpPacket(u_char *pBuffer, int packetLen)
{
	m_tcpBuffer->Add(pBuffer, packetLen);
}


extern bool TrySipInvite(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);
extern bool TrySip200Ok(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);
extern bool TrySipNotify(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);
extern bool TrySipSessionProgress(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);
extern bool TrySip302MovedTemporarily(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);
extern bool TrySipBye(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);
extern bool TrySipRefer(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);
extern bool TrySipInfo(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);
extern bool TrySipSubscribe(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);
extern bool TryLogFailedSip(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct *udpHeader, u_char* payload, u_char* packetEnd);

bool ParseSipMessage(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,TcpHeaderStruct *tcpHeader, u_char* payload, u_char* packetEnd) {

		UdpHeaderStruct udpHeader;

		udpHeader.source = tcpHeader->source;
		udpHeader.dest = tcpHeader->dest;

		udpHeader.len = htons((packetEnd-payload)+sizeof(UdpHeaderStruct));

		bool usefulPacket = TrySipInvite(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);

		if(!usefulPacket)
		{
				usefulPacket = TrySip200Ok(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);
		}

		if(!usefulPacket)
		{
				usefulPacket = TrySipNotify(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);
		}

		if(!usefulPacket)
		{
				usefulPacket = TrySipSessionProgress(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);
		}

		if(!usefulPacket)
		{
				usefulPacket = TrySip302MovedTemporarily(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);
		}

		if(!usefulPacket)
		{
				usefulPacket = TrySipBye(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);
		}

		if(!usefulPacket)
		{
				usefulPacket = TrySipRefer(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);
		}

		if(!usefulPacket)
		{
				usefulPacket = TrySipInfo(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);
		}

		if(!usefulPacket)
		{
				usefulPacket = TrySipSubscribe(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);
		}

		if(!usefulPacket)
		{
				usefulPacket = TryLogFailedSip(ethernetHeader, ipHeader, &udpHeader, payload, packetEnd);
		}

		return usefulPacket;
}

//Returns the total number of bytes consumed corresponding to one or more successfully parsed complete SIP messages in the input byte stream. If no complete SIP message could be found, returns zero
size_t ParseSipStream(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader, char* bufferStart, char *bufferEnd) 
{
	size_t offset=0;
	char * sipMessageStart = bufferStart;

	while(1) {
		char *contentLenStart = memFindAfter("Content-Length: ", sipMessageStart, bufferEnd);
		if (!contentLenStart) {
			return offset;
		}

		int contentLen;
		sscanf(contentLenStart,"%d\r\n",&contentLen);

		char *sipHeaderEnd = memFindAfter("\r\n\r\n", contentLenStart, bufferEnd);
		if (!sipHeaderEnd) {
			return offset;
		}
		
		char* sipMessageEnd = sipHeaderEnd + contentLen;
	 	if (sipMessageEnd > bufferEnd) {
			return offset;
		}	
			
		// We have a complete SIP message, try to parse it 
		ParseSipMessage(ethernetHeader, ipHeader, tcpHeader, (u_char*) sipMessageStart, (u_char*) sipMessageEnd) ;

		offset = sipMessageEnd - bufferStart;
		sipMessageStart = sipMessageEnd;
	}

	return offset;
}

bool TrySipTcp(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader)
{
	static std::list<SipTcpStreamRef> s_SipTcpStreams;
	CStdString logMsg;
	int tcpPayloadLen = 0;
	u_char* tcpPayload;

	tcpPayload = (u_char*)tcpHeader + (tcpHeader->off * 4);
	tcpPayloadLen = ((u_char*)ipHeader+ipHeader->packetLen()) - tcpPayload;
	
	// Ensure this is not a duplicate
	for(std::list<SipTcpStreamRef>::iterator it = s_SipTcpStreams.begin(); it != s_SipTcpStreams.end(); it++)
	{
		SipTcpStreamRef streamRef = *it;

		if(((unsigned int)(streamRef->m_senderIp.s_addr) == (unsigned int)(ipHeader->ip_src.s_addr)) &&
			((unsigned int)(streamRef->m_receiverIp.s_addr) == (unsigned int)(ipHeader->ip_dest.s_addr)) &&
			(streamRef->m_senderPort == ntohs(tcpHeader->source)) &&
			(streamRef->m_receiverPort == ntohs(tcpHeader->dest)) &&
			(streamRef->m_lastSeqNo >= ntohl(tcpHeader->seq)) ) 
		{
			// TODO log packet id
			LOG4CXX_DEBUG(s_sipTcpPacketLog, "Dropped duplicate TCP packet");
			return true;
		}
	}

    if( (tcpPayloadLen >= SIP_RESPONSE_SESSION_PROGRESS_SIZE+3) &&	// payload must be longer than the longest method name
		  (boost::starts_with((char*)tcpPayload,SIP_METHOD_INVITE) ||
		   boost::starts_with((char*)tcpPayload,SIP_METHOD_ACK) ||
		   boost::starts_with((char*)tcpPayload,SIP_METHOD_BYE) ||
		   boost::starts_with((char*)tcpPayload,SIP_RESPONSE_200_OK) ||
		   boost::starts_with((char*)tcpPayload,SIP_RESPONSE_SESSION_PROGRESS) ||
		   boost::starts_with((char*)tcpPayload,SIP_METHOD_REFER) ||
		   boost::starts_with((char*)tcpPayload,SIP_METHOD_INFO) ||
		   boost::starts_with((char*)tcpPayload,SIP_METHOD_SUBSCRIBE) ||
		   boost::starts_with((char*)tcpPayload,SIP_RESPONSE_302_MOVED_TEMPORARILY) ||
		   boost::starts_with((char*)tcpPayload,SIP_METHOD_NOTIFY) ||
		   boost::starts_with((char*)tcpPayload,"SIP/2.0 4") ||
		   boost::starts_with((char*)tcpPayload,"SIP/2.0 5") ||
		   boost::starts_with((char*)tcpPayload,"SIP/2.0 6") ||
		   boost::starts_with((char*)tcpPayload,"CANCEL ")) )
	{
		size_t tcpSipStreamOffset = ParseSipStream(ethernetHeader, ipHeader, tcpHeader, (char*)tcpPayload, (char*)tcpPayload + tcpPayloadLen);
		
		if(tcpSipStreamOffset != tcpPayloadLen)
		{
			SipTcpStreamRef streamRef(new SipTcpStream());
			streamRef->m_senderIp = ipHeader->ip_src;
			streamRef->m_receiverIp = ipHeader->ip_dest;
			streamRef->m_senderPort = ntohs(tcpHeader->source);
			streamRef->m_receiverPort = ntohs(tcpHeader->dest);
			streamRef->m_expectingSeqNo = ntohl(tcpHeader->seq)+tcpPayloadLen;
			streamRef->m_lastSeqNo = ntohl(tcpHeader->seq);
			streamRef->AddTcpPacket(tcpPayload, tcpPayloadLen);
			streamRef->m_offset += tcpSipStreamOffset;
			
			s_SipTcpStreams.push_back(streamRef);
			streamRef->ToString(logMsg);
			LOG4CXX_DEBUG(s_sipTcpPacketLog, "Obtained incomplete TCP Stream: " + logMsg);
		}


//		streamRef->m_offset += ParseSipStream(ethernetHeader, ipHeader, tcpHeader, (char*)streamRef->GetBufferWithOffset(), (char*)streamRef->GetBufferEnd());

//		if (streamRef->GetBufferWithOffset() != streamRef->GetBufferEnd()) {
//			s_SipTcpStreams.push_back(streamRef);
//			streamRef->ToString(logMsg);
//			LOG4CXX_DEBUG(s_sipTcpPacketLog, "Obtained incomplete TCP Stream: " + logMsg);
//		}
		
		return true;
	}

	LOG4CXX_DEBUG(s_sipTcpPacketLog,"Short payload, will look if it belongs to a previous TCP stream");

	bool result = false;
	std::list<SipTcpStreamRef>::iterator it = s_SipTcpStreams.begin(); 
	while (it != s_SipTcpStreams.end())
	{
		SipTcpStreamRef streamRef = *it;

		if(((unsigned int)(streamRef->m_senderIp.s_addr) == (unsigned int)(ipHeader->ip_src.s_addr)) &&
		   ((unsigned int)(streamRef->m_receiverIp.s_addr) == (unsigned int)(ipHeader->ip_dest.s_addr)) &&
		   (streamRef->m_senderPort == ntohs(tcpHeader->source)) &&
		   (streamRef->m_receiverPort == ntohs(tcpHeader->dest)) &&
		   (streamRef->m_expectingSeqNo == ntohl(tcpHeader->seq)) ) 
		{
			streamRef->AddTcpPacket(tcpPayload, tcpPayloadLen);
			streamRef->m_offset += ParseSipStream(ethernetHeader, ipHeader, tcpHeader, (char*)streamRef->GetBufferWithOffset(), (char*)streamRef->GetBufferEnd());
			streamRef->m_expectingSeqNo += tcpPayloadLen;
			result = true;
		} 
		
		if(streamRef->GetBufferWithOffset() == streamRef->GetBufferEnd() || 
		  (time(NULL) - streamRef->m_entryTime) >= 60) {
			it = s_SipTcpStreams.erase(it);
		}
		else {
			it++;
		}
	}

	return result;
}
