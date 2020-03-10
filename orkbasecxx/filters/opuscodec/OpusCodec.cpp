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
   
#include "OpusCodec.h"
#include "AudioCapture.h"
#include <log4cxx/logger.h>
#include "Utils.h"

static log4cxx::LoggerPtr s_log = log4cxx::Logger::getLogger("codec.opus");

OpusCodecDecoder::OpusCodecDecoder()
{
	m_initialized = false;
	m_lastRtpSeq = 0;
	m_lastRtpTs = 0;
	m_sampleRate8KhzMultiplier = 1;	//default is 8khz
	// Initialize decoder
	m_channels = 1;
	m_max_frame_size = 960*6;
	int err = OPUS_OK;
	//Creating the decoder with 8 KHz output sampling rate, no matter what the input sampling rate is. The Opus decoder itself takes care of the downsampling
	m_decoder = opus_decoder_create(8000, 1, &err);
	if(err != OPUS_OK)
	{
		LOG4CXX_ERROR(s_log, "Failed to create the decoder");
		m_initialized = true;
	}
	m_errorLogged = false;
	m_warnLogged = false;
}

OpusCodecDecoder::~OpusCodecDecoder()
{
	opus_decoder_destroy(m_decoder);
}

FilterRef OpusCodecDecoder::Instanciate()
{
	FilterRef Filter(new OpusCodecDecoder());
	return Filter;
}

void OpusCodecDecoder::AudioChunkIn(AudioChunkRef& inputChunk)
{
	CStdString logMsg;
	m_outputChunk.reset();

	if(m_initialized == true)
	{
		return;
	}

	if(inputChunk.get() == NULL)
	{
		logMsg.Format("Null input chunk");
		LOG4CXX_DEBUG(s_log, logMsg);
		return;
	}
	if(inputChunk->GetNumSamples() == 0)
	{
		logMsg.Format("Empty input chunk");
		LOG4CXX_DEBUG(s_log, logMsg);
		return;
	}
	AudioChunkDetails* details = inputChunk->GetDetails();

	if(SupportsInputRtpPayloadType(details->m_rtpPayloadType) == false)
	{
		logMsg.Format("Wrong input RTP payload type:%i", details->m_rtpPayloadType);
		LOG4CXX_DEBUG(s_log, logMsg);
		return;
	}

	int numBytes = inputChunk->GetNumBytes();
	m_outputChunk.reset(new AudioChunk());
	AudioChunkDetails outputDetails = *inputChunk->GetDetails();	// pass through all details
	outputDetails.m_rtpPayloadType = -1;							// and override the ones that this filter changes
	outputDetails.m_encoding = PcmAudio;
	outputDetails.m_numBytes = m_max_frame_size;	//we will need to reset this to the proper number after the chunk got decoded, since the decoder may use up to 5760 bytes on the buffer
	outputDetails.m_timestamp = details->m_timestamp;	//will need downsampling to 8khz
	if(m_lastRtpSeq == 0 || m_lastRtpTs == 0)	//very first chunk, not known sampling rate yet
	{
		m_lastRtpSeq = outputDetails.m_sequenceNumber;
		m_lastRtpTs = outputDetails.m_timestamp;
		m_outputChunk.reset();
		return;
	}
	else if((outputDetails.m_sequenceNumber - m_lastRtpSeq) == 1)
	{
		int sampleRate = (outputDetails.m_timestamp - m_lastRtpTs)/160;
		if(sampleRate != m_sampleRate8KhzMultiplier && (sampleRate == 1 || sampleRate == 2 || sampleRate == 4 || sampleRate == 6))
		{
			m_sampleRate8KhzMultiplier = sampleRate;
		}
	}

	m_lastRtpSeq = outputDetails.m_sequenceNumber;
	m_lastRtpTs = outputDetails.m_timestamp;

	//Decoding
	unsigned char* pBuffer = (unsigned char*)inputChunk->m_pBuffer;
    int output_samples;
    //We may need to run opus_decoder_ctl if we have missing frames, i.e lost packets
    //However, testing shows that it may not be necessary, but I still have the code here for future reference
//    const bool lost = (numBytes == 0);
//    if(lost)
//    {
//        opus_decoder_ctl(m_decoder, OPUS_GET_LAST_PACKET_DURATION(&output_samples));
//    }
//    else
//    {
//        output_samples = m_max_frame_size;
//    }

    output_samples = m_max_frame_size;
    outputDetails.m_timestamp = details->m_timestamp/m_sampleRate8KhzMultiplier;
    short* outputBuffer = (short*)m_outputChunk->CreateBuffer(outputDetails);
    output_samples = opus_decode(m_decoder,
    							pBuffer,
    							numBytes,
								&outputBuffer[0],
								output_samples,
								0);
	if(output_samples > 0)
	{
		if(output_samples > 160 && m_warnLogged == false){
			logMsg.Format("opus_decode audiochunk seq:%d numSamples:%d is over 160", outputDetails.m_sequenceNumber, output_samples);
			LOG4CXX_WARN(s_log, logMsg);
			m_warnLogged = true;
		}
		outputDetails.m_numBytes = output_samples*2;
		m_outputChunk->SetDetails(&outputDetails);
	}
	else
	{
		if(m_errorLogged == false){
			logMsg.Format("opus_decode audiochunk seq:%d error:%d", outputDetails.m_sequenceNumber, output_samples);
			LOG4CXX_ERROR(s_log, logMsg);
			m_errorLogged = true;
		}
		outputDetails.m_numBytes = 320;
		m_outputChunk->SetDetails(&outputDetails);
		memset(outputBuffer, 0, 320);
	}
}

void OpusCodecDecoder::AudioChunkOut(AudioChunkRef& chunk)
{
	chunk = m_outputChunk;
}

AudioEncodingEnum OpusCodecDecoder::GetInputAudioEncoding()
{
	return OpusAudio;
}

AudioEncodingEnum OpusCodecDecoder::GetOutputAudioEncoding()
{
	return PcmAudio;
}

CStdString OpusCodecDecoder::GetName()
{
	return "OpusCodecDecoder";
}

bool OpusCodecDecoder::SupportsInputRtpPayloadType(int rtpPayloadType)
{
	return rtpPayloadType == pt_OPUS;
}

void OpusCodecDecoder::CaptureEventIn(CaptureEventRef& event)
{
	;
}

void OpusCodecDecoder::CaptureEventOut(CaptureEventRef& event)
{
	;
}

