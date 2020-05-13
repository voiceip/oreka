/*
 * Oreka -- A media capture and retrieval platform
 *
 */
#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "LiveStream.h"
#include <log4cxx/logger.h>
#include "AudioCapture.h"
#include <iostream>
#include <string>
#include <cstring>
#include "Utils.h"

#define BUFFER_SAMPLES  8000

static log4cxx::LoggerPtr s_log = log4cxx::Logger::getLogger("plugin.livestream");

template<class T>
std::string toString(const T &value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

LiveStreamFilter::LiveStreamFilter()
{
   //setup new server
    LiveStreamServer liveStreamServer(9090);
	if(liveStreamServer.Initialize())
	{
		std::thread handler(&LiveStreamServer::Run, &liveStreamServer);
		handler.detach();
	}
}

LiveStreamFilter::~LiveStreamFilter()
{

}

FilterRef LiveStreamFilter::Instanciate()
{
    FilterRef Filter(new LiveStreamFilter());
    return Filter;
}

void LiveStreamFilter::AudioChunkIn(AudioChunkRef& inputAudioChunk)
{
    int16_t pcmdata[BUFFER_SAMPLES];
    int input_size = 0;
    int output_size;
    CStdString logMsg;

    memset(pcmdata, 0, sizeof(pcmdata));
    m_outputAudioChunk.reset();

    if(inputAudioChunk.get() == NULL) {
        return;
    }

    if(inputAudioChunk->GetNumSamples() == 0) {
        return;
    }

    AudioChunkDetails outputDetails = *inputAudioChunk->GetDetails();
    if(SupportsInputRtpPayloadType(outputDetails.m_rtpPayloadType) == false)
    {
        logMsg.Format("Wrong input RTP payload type: %d", outputDetails.m_rtpPayloadType);
        LOG4CXX_DEBUG(s_log, logMsg);
        return;
    }

    unsigned char* inputBuffer = (unsigned char*)inputAudioChunk->m_pBuffer;
    input_size = outputDetails.m_numBytes;

    LOG4CXX_DEBUG(s_log, "LiveStream AudioChunkIn Size : " + toString(input_size));

    m_outputAudioChunk.reset(new AudioChunk());
    outputDetails.m_rtpPayloadType = -1;
    outputDetails.m_encoding = PcmAudio;
    outputDetails.m_numBytes = input_size;

    short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(outputDetails);
    memcpy(outputBuffer, inputBuffer, outputDetails.m_numBytes);
}

void LiveStreamFilter::AudioChunkOut(AudioChunkRef& chunk)
{
    chunk = m_outputAudioChunk;
}

AudioEncodingEnum LiveStreamFilter::GetInputAudioEncoding()
{
    return PcmAudio;
}

AudioEncodingEnum LiveStreamFilter::GetOutputAudioEncoding()
{
    return PcmAudio;
}

CStdString LiveStreamFilter::GetName()
{
    return "LiveStreamFilter";
}

bool LiveStreamFilter::SupportsInputRtpPayloadType(int rtpPayloadType)
{
    return rtpPayloadType == pt_PCMU;
}

void LiveStreamFilter::CaptureEventIn(CaptureEventRef& event)
{
    ;
}

void LiveStreamFilter::CaptureEventOut(CaptureEventRef& event)
{
    ;
}

// =================================================================

LiveStreamServer::LiveStreamServer(int port)
{
	m_port = port;
	OrkAprSingleton* orkAprsingle = OrkAprSingleton::GetInstance();
	apr_pool_create(&m_mp,orkAprsingle->GetAprMp());
}
 
bool LiveStreamServer::Initialize()
{
	apr_status_t ret;

    ret = apr_sockaddr_info_get(&m_sockAddr, NULL, APR_INET, m_port, 0, m_mp);
    if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to get sockaddr for livestreamserver");
		return false;
	}
    ret = apr_socket_create(&m_socket, m_sockAddr->family, SOCK_STREAM,APR_PROTO_TCP, m_mp);
    if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to create a socket livestreamserver");
		return false;
	}
    apr_socket_opt_set(m_socket, APR_SO_REUSEADDR, 1);
	apr_socket_opt_set(m_socket, APR_SO_NONBLOCK, 0);
	apr_socket_timeout_set(m_socket, -1);

    ret = apr_socket_bind(m_socket, m_sockAddr);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to bind livestream server socket");
		return false;
	}
    ret = apr_socket_listen(m_socket, SOMAXCONN);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to have a listening socket for livestreamserver");
		return false;
	}
	CStdString tcpPortString = IntToString(m_port);
    LOG4CXX_INFO(s_log, CStdString("Started LiveStream Server on port: ")+tcpPortString);
	return true;
}

void LiveStreamServer::Run()
{
	SetThreadName("orka:livestreamsv");
	while(true)
	{
        LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop");

		apr_status_t ret;
		apr_socket_t* incomingSocket;
		apr_pool_t* request_pool;
		apr_pool_create(&request_pool, m_mp);
        LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop Stage 1");

		ret = apr_socket_accept(&incomingSocket, m_socket, request_pool);
        LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop Stage 2");
		if (ret != APR_SUCCESS) {
            LOG4CXX_ERROR(s_log, "LiveStreamServer:: Bind Exception");
			apr_pool_destroy(request_pool);
			continue;
		}
		// apr_socket_opt_set(incomingSocket, APR_SO_NONBLOCK, 0);
        // LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop Stage 3");
		// apr_socket_timeout_set(incomingSocket, -1);
        // LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop Stage 4");
		// try{
		// 	std::thread handler(StreamingSvc, incomingSocket, request_pool);
        //     LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop Stage 5");
		// 	handler.detach();
		// } catch(const std::exception &ex){
		// 	CStdString logMsg;
		// 	logMsg.Format("Failed to start StreamingSvc thread reason:%s",  ex.what());
		// 	LOG4CXX_ERROR(s_log, logMsg);
		// 	apr_pool_destroy(request_pool);
		// 	continue;
		// }
	}

}

void LiveStreamServer::StreamingSvc(apr_socket_t* sock, apr_pool_t* pool)
{
	char buf[2048];
	CStdString logMsg;
	CStdString sessionId;
	int messagesSent = 0;
	SetThreadName("orka:livestream-client");
	OrkAprSubPool subPool(pool); //change lifetime of pool to this request.

	while(true)
	{
		apr_size_t size = 2048;
		apr_status_t ret = apr_socket_recv(sock, buf, &size);

        CStdString notFound("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nNot found\r\n");
        apr_size_t leng = notFound.GetLength();
        ret = apr_socket_send(sock, notFound, &leng);

		// if(size <= 5)
		// {
		// 	CStdString notFound("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nNot found\r\n");
		// 	apr_size_t leng = notFound.GetLength();
		// 	ret = apr_socket_send(sock, notFound, &leng);
		// 	break;
		// }

		// try
		// {
		// 	int startUrlOffset = 5;
		// 	char* stopUrl = strstr(buf+startUrlOffset, " HTTP");

		// 	if(!stopUrl)
		// 	{
		// 		throw (CStdString("Malformed http request"));
		// 	}

		// 	CStdString header;
		// 	struct tm date = {0};
		// 	time_t now = time(NULL);
		// 	CStdString rfc822Date;

		// 	apr_time_t tn = apr_time_now();
		// 	apr_time_exp_t texp;
		// 	apr_time_exp_gmt(&texp, tn);
		// 	rfc822Date.Format("Tue, %.2d Nov %.4d %.2d:%.2d:%.2d GMT", texp.tm_mday, (texp.tm_year+1900), texp.tm_hour, texp.tm_min, texp.tm_sec);
		// 	header.Format("HTTP/1.1 200 OK\r\nLast-Modified:%s\r\nContent-Type:text/plain\r\n\r\n", rfc822Date);

		// 	apr_size_t leng = header.GetLength();
		// 	ret = apr_socket_send(sock, header, &leng);

		// 	time_t startTime = time(NULL);

		// 	sessionId = EventStreamingSingleton::instance()->GetNewSessionId() + " -";
		// 	logMsg.Format("%s Event streaming start", sessionId);
		// 	LOG4CXX_INFO(s_log, logMsg);

		// 	EventStreamingSessionRef session(new EventStreamingSession());
		// 	EventStreamingSingleton::instance()->AddSession(session);
		// 	if(EventStreamingSingleton::instance()->GetNumSessions() > 100)
		// 	{
		// 		EventStreamingSingleton::instance()->RemoveSession(session);
		// 		logMsg.Format("Event streaming number of session exceeds 100. Stopping %s", sessionId);
		// 		LOG4CXX_ERROR(s_log, logMsg);
		// 		break;
		// 	}

		// 	ret=APR_SUCCESS;
		// 	while(ret == APR_SUCCESS)
		// 	{
		// 		session->WaitForMessages();

		// 		while(session->GetNumMessages() && ret == APR_SUCCESS)
		// 		{
		// 			MessageRef message;

		// 			session->GetTapeMessage(message);
		// 			if(message.get())
		// 			{
		// 				CStdString msgAsSingleLineString;
		// 				msgAsSingleLineString = message->SerializeUrl() + "\r\n";

		// 				apr_size_t leng = msgAsSingleLineString.GetLength();
		// 				ret = apr_socket_send(sock, msgAsSingleLineString, &leng);
		// 				if (ret == APR_SUCCESS)
		// 				{
		// 					messagesSent += 1;
		// 				}
		// 				else
		// 				{
		// 					char errstr[256];
		// 					apr_strerror(ret, errstr, sizeof(errstr));

		// 					logMsg.Format("%s Event streaming session error %d: %s", sessionId, ret, errstr);
		// 					LOG4CXX_WARN(s_log, logMsg);
		// 				}
		// 			}
		// 		}
		// 	}

		// 	EventStreamingSingleton::instance()->RemoveSession(session);
		// 	logMsg.Format("%s Stream client stop - sent %d messages in %d sec", sessionId, messagesSent, (time(NULL) - startTime));
		// 	LOG4CXX_INFO(s_log, logMsg);
		// }
		// catch (CStdString& e)
		// {
		// 	CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nError\r\n");
		// 	error = error + e + "\r\n";
		// 	LOG4CXX_ERROR(s_log, e);
		// 	apr_size_t leng = error.GetLength();
		// 	ret = apr_socket_send(sock, error, &leng);
		// }
        break;
    }
    apr_socket_close(sock);
}

//----------------------------------------------------------------




extern "C"
{
    DLL_EXPORT void __CDECL__ OrkInitialize()
    {
        LOG4CXX_INFO(s_log, "LiveStream  filter starting.");
        FilterRef filter(new LiveStreamFilter());
        FilterRegistry::instance()->RegisterFilter(filter);
        LOG4CXX_INFO(s_log, "LiveStream  filter initialized.");
    }
}
