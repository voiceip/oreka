/*
 * Oreka -- A media capture and retrieval platform
 *
 */
#pragma warning(disable : 4786) // disables truncated symbols in browse-info warning

#define _WINSOCKAPI_ // prevents the inclusion of winsock.h

#include "LiveStream.h"
#include <log4cxx/logger.h>
#include "AudioCapture.h"
#include <iostream>
#include <string>
#include <cstring>
#include "Utils.h"
#include "srs_librtmp.h"
#include <queue>

#define BUFFER_SAMPLES 8000

static log4cxx::LoggerPtr s_log = log4cxx::Logger::getLogger("plugin.livestream");

template <class T>
std::string toString(const T &value)
{
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

void LiveStreamFilter::AudioChunkIn(AudioChunkRef &inputAudioChunk)
{
	// LOG4CXX_INFO(s_log, "LiveStream AudioChunkIn ");
	m_outputAudioChunk = inputAudioChunk;
	// int16_t pcmdata[BUFFER_SAMPLES];
	int size = 0;
	CStdString logMsg;

	u_int32_t time_delta = 17;

	// memset(pcmdata, 0, sizeof(pcmdata));
	// m_outputAudioChunk.reset();

	if (inputAudioChunk.get() == NULL)
	{
		return;
	}

	if (inputAudioChunk->GetNumSamples() == 0)
	{
		return;
	}

	AudioChunkDetails outputDetails = *inputAudioChunk->GetDetails();
	char *inputBuffer = (char *)inputAudioChunk->m_pBuffer;
	size = outputDetails.m_numBytes * 2;

	logMsg.Format("LiveStreamFilter AudioChunkIn Size: %d, Encoding: %s , RTP payload type: %s", size, toString(outputDetails.m_encoding), RtpPayloadTypeEnumToString(outputDetails.m_rtpPayloadType));
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
	if (outputDetails.m_rtpPayloadType == pt_PCMU)
		sound_format = 8;
	else if (outputDetails.m_rtpPayloadType == pt_PCMA)
		sound_format = 7;

	// @param sound_rate Sampling rate. The following values are defined:
	// 0 = 5.5 kHz
	// 1 = 11 kHz
	// 2 = 22 kHz
	// 3 = 44 kHz
	char sound_rate = 3;

	// @param sound_size Size of each audio sample. This parameter only pertains to
	// uncompressed formats. Compressed formats always decode
	// to 16 bits internally.
	// 0 = 8-bit samples
	// 1 = 16-bit samples
	char sound_size = 1;

	// @param sound_type Mono or stereo sound
	// 0 = Mono sound
	// 1 = Stereo sound
	//char sound_type = outputDetails.m_channel == 0 ? 0 : 1;
	char sound_type = 1;

	timestamp += 160; //Timestamp increment = clock frequency/frame rate
					  //160 byte payload of G.711 has a packetization interval of 20 ms
					  //For 1 second, there will be 1000ms / 20ms = 50 frames
					  //Audio RTP packet timestamp incremental value = 8kHz / 50 = 8000Hz / 50 = 160

	if (outputDetails.m_channel == 1)
	{
		bufferQueue.push(inputBuffer);
	}

	if (rtmp != NULL && status)
	{
		if (outputDetails.m_channel == 2 && bufferQueue.size() > 0)
		{
			char *outputBuffer = (char *)malloc(size);
			char *tempBuffer = bufferQueue.front();
			bufferQueue.pop();

			for (int i = 0; i < 160; ++i)
			{
				outputBuffer[i * 2] = tempBuffer[i];
				outputBuffer[i * 2 + 1] = inputBuffer[i];
			}

			if (srs_audio_write_raw_frame(rtmp, sound_format, sound_rate, sound_size, sound_type, outputBuffer, size, timestamp) != 0)
			{
				srs_human_trace("send audio raw data failed.");
				return;
			}

			srs_human_trace("Accepted Packet: sample_rate=%d, sound_format=%s, m_timestamp=%d, m_arrivalTimestamp=%d,m_sequenceNumber=%d",
							outputDetails.m_sampleRate, outputDetails.m_rtpPayloadType, outputDetails.m_timestamp, outputDetails.m_arrivalTimestamp, outputDetails.m_sequenceNumber);
			srs_human_trace("sent packet: type=%s, time=%d, size=%d, codec=%d, rate=%d, sample=%d, channel=%d",
							srs_human_flv_tag_type2string(SRS_RTMP_TYPE_AUDIO), timestamp, size, sound_format, sound_rate, sound_size, sound_type);
		}
	}
}

void LiveStreamFilter::AudioChunkOut(AudioChunkRef &chunk)
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

void LiveStreamFilter::CaptureEventIn(CaptureEventRef &event)
{
	//Start RTP Stream Open
	auto key = event->EventTypeToString(event->m_type);
	LOG4CXX_INFO(s_log, "LiveStream CaptureEventIn " + key + " : " + event->m_value);
	if (event->m_type == CaptureEvent::EventTypeEnum::EtCallId)
	{
		m_callId = event->m_value;
	}

	if (event->m_type == CaptureEvent::EventTypeEnum::EtKeyValue && event->m_key == "LiveStream" && event->m_value == "start")
	{
		std::string url = "rtmp://172.16.176.65:1935/live/" + m_callId;
		status = true;
		LOG4CXX_INFO(s_log, "LiveStream URL : " + url);
		//open rstp stream
		rtmp = srs_rtmp_create(url.c_str());

		if (srs_rtmp_handshake(rtmp) != 0)
		{
			srs_human_trace("simple handshake failed.");
			return;
		}
		srs_human_trace("simple handshake success");

		if (srs_rtmp_connect_app(rtmp) != 0)
		{
			srs_human_trace("connect vhost/app failed.");
			return;
		}
		srs_human_trace("connect vhost/app success");

		if (srs_rtmp_publish_stream(rtmp) != 0)
		{
			srs_human_trace("publish stream failed.");
			return;
		}
		srs_human_trace("publish stream success");
	}

	if (event->m_type == CaptureEvent::EventTypeEnum::EtStop)
	{
		//close rstp stream
		status = false;
		if (rtmp != NULL)
		{
			srs_human_trace("stream detroying...");
			srs_rtmp_destroy(rtmp);
		}
	}

	if (event->m_type == CaptureEvent::EventTypeEnum::EtKeyValue && event->m_key == "LiveStream" && event->m_value == "end")
	{
		//close rstp stream
		status = false;
	}
}

void LiveStreamFilter::CaptureEventOut(CaptureEventRef &event)
{
	//LOG4CXX_INFO(s_log, "LiveStream CaptureEventOut " + toString(event.get()));
}

void LiveStreamFilter::SetSessionInfo(CStdString &trackingId)
{
	LOG4CXX_INFO(s_log, "LiveStream SetSessionInfo " + trackingId);
}

// =================================================================

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
