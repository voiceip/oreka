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
#include "srs_librtmp.h"

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
	LOG4CXX_INFO(s_log, "LiveStream New Instance Created");
}

LiveStreamFilter::~LiveStreamFilter()
{
	LOG4CXX_INFO(s_log, "LiveStream Instance Destroying");
}

FilterRef LiveStreamFilter::Instanciate()
{
    FilterRef Filter(new LiveStreamFilter());
    return Filter;
}

void LiveStreamFilter::AudioChunkIn(AudioChunkRef& inputAudioChunk)
{
	// LOG4CXX_INFO(s_log, "LiveStream AudioChunkIn ");
	m_outputAudioChunk = inputAudioChunk;
    // int16_t pcmdata[BUFFER_SAMPLES];
    int size = 0;
    CStdString logMsg;

	u_int32_t time_delta = 17;

    // memset(pcmdata, 0, sizeof(pcmdata));
    // m_outputAudioChunk.reset();

    if(inputAudioChunk.get() == NULL) {
        return;
    }

    if(inputAudioChunk->GetNumSamples() == 0) {
        return;
    }

    AudioChunkDetails outputDetails = *inputAudioChunk->GetDetails();
    char* inputBuffer = (char*)inputAudioChunk->m_pBuffer;
    size = outputDetails.m_numBytes;

	logMsg.Format("LiveStreamFilter AudioChunkIn Size: %d, Encoding: %s , RTP payload type: %s",size ,toString(outputDetails.m_encoding) , RtpPayloadTypeEnumToString(outputDetails.m_rtpPayloadType));
    LOG4CXX_INFO(s_log, logMsg);

	// @param sound_format Format of SoundData. The following values are defined:
	// 0 = Linear PCM, platform endian
	// 1 = ADPCM
	// 2 = MP3
	// 3 = Linear PCM, little endian
	// 4 = Nellymoser 16 kHz mono
	// 5 = Nellymoser 8 kHz mono
	// 6 = Nellymoser
	// 7 = G.711 A-law logarithmic PCM
	// 8 = G.711 mu-law logarithmic PCM
	// 9 = reserved
	// 10 = AAC
	// 11 = Speex
	// 14 = MP3 8 kHz
	// 15 = Device-specific sound
	// Formats 7, 8, 14, and 15 are reserved.
	// AAC is supported in Flash Player 9,0,115,0 and higher.
	// Speex is supported in Flash Player 10 and higher.

	char sound_format = 9;
	if (outputDetails.m_rtpPayloadType ==  pt_PCMU)
		sound_format = 8;
	else if (outputDetails.m_rtpPayloadType ==  pt_PCMA)
		sound_format = 7;

	// @param sound_rate Sampling rate. The following values are defined:
	// 0 = 5.5 kHz
	// 1 = 11 kHz
	// 2 = 22 kHz
	// 3 = 44 kHz
	char sound_rate = 8;

	// @param sound_size Size of each audio sample. This parameter only pertains to
	// uncompressed formats. Compressed formats always decode
	// to 16 bits internally.
	// 0 = 8-bit samples
	// 1 = 16-bit samples
	char sound_size = 0;

	// @param sound_type Mono or stereo sound
	// 0 = Mono sound
	// 1 = Stereo sound
	//char sound_type = outputDetails.m_channel == 0 ? 0 : 1;
	char sound_type = 0;

	timestamp += 160; //Timestamp increment = clock frequency/frame rate
					  //160 byte payload of G.711 has a packetization interval of 20 ms
					  //For 1 second, there will be 1000ms / 20ms = 50 frames
					  //Audio RTP packet timestamp incremental value = 8kHz / 50 = 8000Hz / 50 = 160

	if (rtmp != NULL)
	{ //Filter for Channel 1
		if (sequenceNumber == 0 || (sequenceNumber == outputDetails.m_sequenceNumber - 1))
		{
			sequenceNumber = outputDetails.m_sequenceNumber;

			if (srs_audio_write_raw_frame(rtmp, sound_format, sound_rate, sound_size, sound_type, inputBuffer, size, timestamp) != 0)
			{
				srs_human_trace("send audio raw data failed.");
				return;
			}

			srs_human_trace("Accepted Packet: sample_rate=%d, sound_format=%s, m_timestamp=%d, m_arrivalTimestamp=%d,m_sequenceNumber=%d",
							outputDetails.m_sampleRate, outputDetails.m_rtpPayloadType, outputDetails.m_timestamp, outputDetails.m_arrivalTimestamp, outputDetails.m_sequenceNumber);
			srs_human_trace("sent packet: type=%s, time=%d, size=%d, codec=%d, rate=%d, sample=%d, channel=%d",
							srs_human_flv_tag_type2string(SRS_RTMP_TYPE_AUDIO), timestamp, size, sound_format, sound_rate, sound_size, sound_type);
		}
		else
		{
			srs_human_trace("Dropped Packet: sample_rate=%d, sound_format=%s, m_timestamp=%d, m_arrivalTimestamp=%d,m_sequenceNumber=%d",
							outputDetails.m_sampleRate, outputDetails.m_rtpPayloadType, outputDetails.m_timestamp, outputDetails.m_arrivalTimestamp, outputDetails.m_sequenceNumber);
		}
	}
}

void LiveStreamFilter::AudioChunkOut(AudioChunkRef& chunk)
{
    chunk = m_outputAudioChunk;
}

AudioEncodingEnum LiveStreamFilter::GetInputAudioEncoding()
{
    return UnknownAudio;
}

AudioEncodingEnum LiveStreamFilter::GetOutputAudioEncoding()
{
    return UnknownAudio;
}

CStdString LiveStreamFilter::GetName()
{
    return "LiveStreamFilter";
}

bool LiveStreamFilter::SupportsInputRtpPayloadType(int rtpPayloadType)
{
	//so that BatchProcessing doesn't pick this filter.
    return rtpPayloadType == pt_Unknown;
}


std::string url = "rtmp://172.16.176.65:1935/live/sipcall";

void LiveStreamFilter::CaptureEventIn(CaptureEventRef& event)
{
	//Start RTP Stream Open
	auto key = event->EventTypeToString(event->m_type);
	LOG4CXX_INFO(s_log, "LiveStream CaptureEventIn " + key + " : " + event->m_value);
	if(event->m_type == CaptureEvent::EventTypeEnum::EtCallId){
		m_callId = event->m_value;
	}

	if(event->m_type == CaptureEvent::EventTypeEnum::EtStart){
		//open rstp stream
		rtmp = srs_rtmp_create(url.c_str());

		if (srs_rtmp_handshake(rtmp) != 0) {
			srs_human_trace("simple handshake failed.");
			return;
		}
		srs_human_trace("simple handshake success");

		if (srs_rtmp_connect_app(rtmp) != 0) {
			srs_human_trace("connect vhost/app failed.");
			return;
		}
		srs_human_trace("connect vhost/app success");

		if (srs_rtmp_publish_stream(rtmp) != 0) {
			srs_human_trace("publish stream failed.");
			return;
		}
		srs_human_trace("publish stream success");
	}

	if(event->m_type == CaptureEvent::EventTypeEnum::EtStop){
		//close rstp stream
		if (rtmp != NULL) {
			srs_human_trace("stream detroying...");
			srs_rtmp_destroy(rtmp);
		}
	}
}

void LiveStreamFilter::CaptureEventOut(CaptureEventRef& event)
{
	//LOG4CXX_INFO(s_log, "LiveStream CaptureEventOut " + toString(event.get()));
}

void LiveStreamFilter::SetSessionInfo(CStdString& trackingId){
	LOG4CXX_INFO(s_log, "LiveStream SetSessionInfo " + trackingId);
}

// =================================================================

LiveStreamServer::LiveStreamServer(int port)
{
	m_port = port;
	//OrkAprSingleton* orkAprsingle = OrkAprSingleton::GetInstance();
	//apr_pool_create(&m_mp,orkAprsingle->GetAprMp());
    apr_pool_create(&m_mp,NULL);

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

		ret = apr_socket_accept(&incomingSocket, m_socket, request_pool);
        LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop Stage 2");
		if (ret != APR_SUCCESS) {
            LOG4CXX_ERROR(s_log, "LiveStreamServer:: Bind Exception");
			apr_pool_destroy(request_pool);
			continue;
		}
		apr_socket_opt_set(incomingSocket, APR_SO_NONBLOCK, 0);
        LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop Stage 3");
		apr_socket_timeout_set(incomingSocket, -1);
        LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop Stage 4");
		try{
			std::thread handler(StreamingSvc, incomingSocket, request_pool);
            LOG4CXX_INFO(s_log, "LiveStreamServer::Run loop Stage 5");
			handler.detach();
		} catch(const std::exception &ex){
			CStdString logMsg;
			logMsg.Format("Failed to start StreamingSvc thread reason:%s",  ex.what());
			LOG4CXX_ERROR(s_log, logMsg);
			apr_pool_destroy(request_pool);
			continue;
		}
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

		try
		{
			int startUrlOffset = 5;
			char* stopUrl = strstr(buf+startUrlOffset, " HTTP");

			if(!stopUrl)
			{
				throw (CStdString("Malformed http request"));
			}

			CStdString header;
			struct tm date = {0};
			time_t now = time(NULL);
			CStdString rfc822Date;

			apr_time_t tn = apr_time_now();
			apr_time_exp_t texp;
			apr_time_exp_gmt(&texp, tn);
			rfc822Date.Format("Tue, %.2d Nov %.4d %.2d:%.2d:%.2d GMT", texp.tm_mday, (texp.tm_year+1900), texp.tm_hour, texp.tm_min, texp.tm_sec);
			header.Format("HTTP/1.1 200 OK\r\nLast-Modified:%s\r\nContent-Type:text/plain\r\n\r\n", rfc822Date);

		// 	apr_size_t leng = header.GetLength();
		// 	ret = apr_socket_send(sock, header, &leng);

		// 	time_t startTime = time(NULL);

		// 	sessionId = EventStreamingSingleton::instance()->GetNewSessionId() + " -";
		// 	logMsg.Format("%s Event streaming start", sessionId);
		// 	LOG4CXX_INFO(s_log, logMsg);

			// EventStreamingSessionRef session(new EventStreamingSession());
			// EventStreamingSingleton::instance()->AddSession(session);
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
		}
		catch (CStdString& e)
		{
			CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nError\r\n");
			error = error + e + "\r\n";
			LOG4CXX_ERROR(s_log, e);
			apr_size_t leng = error.GetLength();
			ret = apr_socket_send(sock, error, &leng);
		}
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
