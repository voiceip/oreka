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
#include "boost/shared_ptr.hpp"
#include "StdString.h"
#include "SipTcp.h"


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

static char* memFindAfter(char* toFind, char* start, char* stop)
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
        for(int i=0; i<len; i++)
        {
                sprintf(byteAsHex, "%.2x", input[i]);
                output += byteAsHex;
        }
}

SipTcpStream::SipTcpStream()
{
	m_expectingSeqNo = 0;
	m_senderIp.s_addr = 0;
	m_receiverIp.s_addr = 0;
	m_senderPort = 0;
	m_receiverPort = 0;
	m_entryTime = time(NULL);
	m_sipRequest = SafeBufferRef(new SafeBuffer());
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

	//string.Format("sender:%s receiver:%s sender-port:%d receiver-port:%d entry-time:%d expecting-seq-no:%s total-bytes:%d last-seqno:%s [[[%s]]]", senderIp, receiverIp, m_senderPort, m_receiverPort, m_entryTime, expSeq, m_sipRequest->Size(), lastSeq, m_sipRequest->GetBuffer());
	string.Format("sender:%s receiver:%s sender-port:%d receiver-port:%d entry-time:%d expecting-seq-no:%s total-bytes:%d last-seqno:%s", senderIp, receiverIp, m_senderPort, m_receiverPort, m_entryTime, expSeq, m_sipRequest->Size(), lastSeq);
}

void SipTcpStream::AddTcpPacket(u_char *pBuffer, int packetLen)
{
	m_sipRequest->Add(pBuffer, packetLen);
}

/*
 * How we know the SIP request is complete:  Small excerpt from
 * RFC3261
 * 
 * ---8<---
 * ...
 * 20.14 Content-Length
 *
 * The Content-Length header field indicates the size of the message-
 * body, in decimal number of octets, sent to the recipient.
 * Applications SHOULD use this field to indicate the size of the
 * message-body to be transferred, regardless of the media type of the
 * entity.  If a stream-based protocol (such as TCP) is used as
 * transport, the header field MUST be used.
 *
 * The size of the message-body does not include the CRLF separating
 * header fields and body.  Any Content-Length greater than or equal to
 * zero is a valid value.  If no body is present in a message, then the
 * Content-Length header field value MUST be set to zero.
 * ...
 * --->8---
 *
 */
bool SipTcpStream::SipRequestIsComplete()
{
	if(!m_sipRequest->Size())
		return false;
		
	char *pBufStart = (char*)m_sipRequest->GetBuffer();
	char *pBufEnd = pBufStart+m_sipRequest->Size();
	char *contentLengthHeader = ACE_OS::strstr(pBufStart, "Content-Length: ");
	char *contentLength = memFindAfter("Content-Length: ", pBufStart, pBufEnd);
	int cLength = 0;

	if(!contentLength || !contentLengthHeader)
		return false;

        char *eol = memFindEOL(contentLengthHeader, pBufEnd);
	if(eol == contentLengthHeader)
		return false;

	cLength = ACE_OS::atoi(contentLength);
	if(!cLength)
		return true;

	/* Step over newlines */
	while(*eol && (*eol == '\r' || *eol == '\n'))
		eol++;

	if(!*eol)
		return false;

	if(strlen(eol) == cLength)
		return true;
}

SafeBufferRef SipTcpStream::GetCompleteSipRequest()
{
	SafeBufferRef buf(new SafeBuffer());

	buf->Store(m_sipRequest->GetBuffer(), m_sipRequest->Size());

	return buf;
}

