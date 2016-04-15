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
#include "VoIpSession.h"
#include "Iax2Session.h"
#include "SipTcp.h"
#include "shared_ptr.h"
#include "StdString.h"
#include "SipTcp.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

SafeBuffer::SafeBuffer()
{
	m_pBuffer = NULL;
	m_size = 0;
}

SafeBuffer::~SafeBuffer()
{
	if(m_size) {
		free(m_pBuffer);
		m_size = 0;
	}

	m_pBuffer = NULL;
}

int SafeBuffer::Size()
{
	return m_size;
}

void SafeBuffer::Store(u_char *buf, int len)
{
	if(m_size) {
		free(m_pBuffer);
		m_size = 0;
		m_pBuffer = NULL;
	}

	m_pBuffer = (u_char *)calloc(len+1, 1);
	m_size = len;

	if(!m_pBuffer) {
		char tmp[80];
		snprintf(tmp, sizeof(tmp), "%d", len);

		CStdString numBytes = CStdString(tmp);
                throw("SafeBuffer::Store could not malloc a buffer of size:" + numBytes);
	}

	memcpy(m_pBuffer, buf, len);
}

void SafeBuffer::Add(u_char *buf, int len)
{
        u_char *newBuf = NULL;

        if(!m_size) {
                Store(buf, len);
                return;
        }

        newBuf = (u_char*)realloc(m_pBuffer, m_size+len+1);
        if(!newBuf) {
                char tmp[80];
                snprintf(tmp, sizeof(tmp), "%d", len+m_size);

                CStdString numBytes = CStdString(tmp);
                throw("SafeBuffer::Add failed to realloc buffer to " + numBytes);
        }

        m_pBuffer = newBuf;
        memcpy(m_pBuffer+m_size, buf, len);
        m_size += len;
        *(m_pBuffer+m_size) = 0;
}

u_char *SafeBuffer::GetBuffer()
{
	return m_pBuffer;
}

// ============================================================

static char* memFindAfter(const char* toFind, char* start, char* stop)
{
        for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memchr(ptr+1, toFind[0],(stop - ptr - 1)))
        {
                if(ACE_OS::strncasecmp(toFind, ptr, strlen(toFind)) == 0)
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
                if(ACE_OS::strncasecmp(toFind, ptr, strlen(toFind)) == 0)
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

        ACE_OS::inet_ntop(AF_INET, (void*)&m_senderIp, senderIp, sizeof(senderIp));
        ACE_OS::inet_ntop(AF_INET, (void*)&m_receiverIp, receiverIp, sizeof(receiverIp));

	//string.Format("sender:%s receiver:%s sender-port:%d receiver-port:%d entry-time:%d expecting-seq-no:%s total-bytes:%d last-seqno:%s [[[%s]]]", senderIp, receiverIp, m_senderPort, m_receiverPort, m_entryTime, expSeq, m_tcpBuffer->Size(), lastSeq, m_tcpBuffer->GetBuffer());
	string.Format("sender:%s receiver:%s sender-port:%d receiver-port:%d entry-time:%d expecting-seq-no:%s total-bytes:%d last-seqno:%s", senderIp, receiverIp, m_senderPort, m_receiverPort, m_entryTime, expSeq, m_tcpBuffer->Size(), lastSeq);
}

void SipTcpStream::AddTcpPacket(u_char *pBuffer, int packetLen)
{
	m_tcpBuffer->Add(pBuffer, packetLen);
}

