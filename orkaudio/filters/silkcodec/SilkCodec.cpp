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

#include "SilkCodec.h"
#include <log4cxx/logger.h>
#include "AudioCapture.h"
#include "Utils.h"
#include "dll.h"

static log4cxx::LoggerPtr s_log = log4cxx::Logger::getLogger("codec.silk");

SilkCodecDecoder::SilkCodecDecoder()
{
   
    // Initialize decoder
    m_initialized = true;
    m_decControl.API_sampleRate = 8000;
    m_decControl.framesPerPacket = 1;
    /* Create decoder */
    int ret = SKP_Silk_SDK_Get_Decoder_Size( &m_decSizeBytes );
    if(ret) 
    {
//        LOG4CXX_ERROR(s_log, "SKP_Silk_SDK_Get_Decoder_Size failed");
        m_initialized = false;
    }
    m_psDec = malloc( m_decSizeBytes );

    /* Reset decoder */
    ret = SKP_Silk_SDK_InitDecoder( m_psDec );
    if(ret)
    {
 //       LOG4CXX_ERROR(s_log, "SKP_Silk_InitDecoder falied");
    	m_initialized = false;
    }
    m_sampleRate8KhzMultiplier = -1;	//invalid
    m_lastRtpSeq = 0;
    m_lastRtpTs = 0;
}

SilkCodecDecoder::~SilkCodecDecoder()
{
    /* Free decoder */
    free(m_psDec);
}

FilterRef SilkCodecDecoder::Instanciate()
{
	FilterRef Filter(new SilkCodecDecoder());
	return Filter;
}

void SilkCodecDecoder::AudioChunkIn(AudioChunkRef& inputAudioChunk)
{
	if(m_initialized != true)
	{
		return;
	}

	int in_samples = 0;
	int out_samples = 0;
	short pcmdata[8000];
	CStdString logMsg;

	memset(pcmdata, 0, sizeof(pcmdata));

	if(inputAudioChunk.get() == NULL) {
		return;
	}

	if(inputAudioChunk->GetNumSamples() == 0) {
		return;
	}
	AudioChunkDetails* details = inputAudioChunk->GetDetails();

	if(SupportsInputRtpPayloadType(details->m_rtpPayloadType) == false)
	{
        logMsg.Format("Wrong input RTP payload type:%d", details->m_rtpPayloadType);
		LOG4CXX_DEBUG(s_log, logMsg);
		return;
	}
        
	AudioChunkDetails outputDetails = *inputAudioChunk->GetDetails();
	outputDetails.m_rtpPayloadType = -1;
	outputDetails.m_encoding = PcmAudio;
	outputDetails.m_numBytes = 320;
	outputDetails.m_timestamp = details->m_timestamp;
        int sampleRate = 0;
	if(m_lastRtpSeq == 0 || m_lastRtpTs == 0)	//very first chunk, not known sampling rate yet
	{
		m_lastRtpSeq = outputDetails.m_sequenceNumber;
		m_lastRtpTs = outputDetails.m_timestamp;
		m_outputChunk.reset();
		return;
	}
	else if((outputDetails.m_sequenceNumber - m_lastRtpSeq) == 1)
	{
		if((outputDetails.m_timestamp - m_lastRtpTs) == 160 || (outputDetails.m_timestamp - m_lastRtpTs) == 320)
		{
			//only support 8MHZ and 16MHZ for now
			//If this happen, it would mean silk stream has changed from 8MHZ to 16MHZ or vice versal in the same ssrc stream(consecutive sequence, but not timestamp)
			sampleRate = (outputDetails.m_timestamp - m_lastRtpTs)/160;
			if(sampleRate != m_sampleRate8KhzMultiplier && (sampleRate == 1 || sampleRate == 2 || sampleRate == 3))
			{
				m_sampleRate8KhzMultiplier = sampleRate;
			}
		}
		else
		{
			m_lastRtpSeq = outputDetails.m_sequenceNumber;
			m_lastRtpTs = outputDetails.m_timestamp;
			m_sampleRate8KhzMultiplier = -1;
			return;
		}

	}
	// cast timestamps to long so that we can take difference of unsigned ints
	else if(abs((long)outputDetails.m_timestamp - (long)m_lastRtpTs) !=
		   (abs((long)outputDetails.m_sequenceNumber - (long)m_lastRtpSeq) *m_sampleRate8KhzMultiplier*160))
	{
		// sequence number delta is not coherent with timestamp delta, recalculating m_sampleRate8KhzMultiplier.
		//Using abs() in the condition to make sure that out of order packets will not trigger this and cause one additional unecessary lost packet
		m_lastRtpSeq = outputDetails.m_sequenceNumber;
		m_lastRtpTs = outputDetails.m_timestamp;
		m_sampleRate8KhzMultiplier = -1;
		return;
	}
	//sanity check/ Currently not yet support 24Khz
	if(m_sampleRate8KhzMultiplier != 1 && m_sampleRate8KhzMultiplier != 2)
	{
			return;
	}

	m_decControl.API_sampleRate = m_sampleRate8KhzMultiplier*8000;

	m_lastRtpSeq = outputDetails.m_sequenceNumber;
	m_lastRtpTs = outputDetails.m_timestamp;
	m_outputChunk.reset(new AudioChunk());
	//decoder
	SKP_int16 decodedLen = 0;
	SKP_uint8* inputBuffer = (SKP_uint8*)inputAudioChunk->m_pBuffer;
	//Update correct timestamp step for output chunk
	outputDetails.m_timestamp = details->m_timestamp/m_sampleRate8KhzMultiplier;

	if(outputDetails.m_numBytes == 0)
	{
		m_outputChunk.reset();
		return;
	}

	int numBytes = inputAudioChunk->GetNumBytes();
	SKP_int16* outputBuffer = (SKP_int16*)m_outputChunk->CreateBuffer(outputDetails);
	switch (m_sampleRate8KhzMultiplier){
		case 1: Decode8KhzChunk(inputBuffer, outputBuffer, numBytes);
			break;
		case 2: Decode16KhzChunk(inputBuffer, outputBuffer, numBytes);
			break;
	}

}

void SilkCodecDecoder::Decode8KhzChunk(SKP_uint8* inputBuffer, SKP_int16* outputBuffer, int numBytes)
{
	SKP_int16 decodedLen = 0;
	int ret = 0;
	do{
		ret = SKP_Silk_SDK_Decode( m_psDec, &m_decControl, 0, inputBuffer, numBytes, outputBuffer, &decodedLen );
		outputBuffer += decodedLen;
	} while( m_decControl.moreInternalDecoderFrames );
}

void SilkCodecDecoder::Decode16KhzChunk(SKP_uint8* inputBuffer, SKP_int16* outputBuffer, int numBytes)
{
	SKP_int16 tmpBuf[320];
	SKP_int16 decodedLen = 0;
	SKP_int16 bufOffset = 0;
	int ret = 0;
	do {
		ret = SKP_Silk_SDK_Decode( m_psDec, &m_decControl, 0, inputBuffer, numBytes, &tmpBuf[bufOffset], &decodedLen );
		bufOffset += decodedLen;
	} while( m_decControl.moreInternalDecoderFrames );
	for(int i=0; i<320; i+=2)
	{
		outputBuffer[i/2] = tmpBuf[i];
	}
}

void SilkCodecDecoder::AudioChunkOut(AudioChunkRef& chunk)
{
	chunk = m_outputChunk;
}

AudioEncodingEnum SilkCodecDecoder::GetInputAudioEncoding()
{
	return SilkAudio;

}

AudioEncodingEnum SilkCodecDecoder::GetOutputAudioEncoding()
{
	return PcmAudio;
}

CStdString SilkCodecDecoder::GetName()
{
	return "SilkCodecDecoder";

}

bool SilkCodecDecoder::SupportsInputRtpPayloadType(int rtpPayloadType)
{
	return rtpPayloadType == pt_SILK;
}

void SilkCodecDecoder::CaptureEventIn(CaptureEventRef& event)
{
	;
}

void SilkCodecDecoder::CaptureEventOut(CaptureEventRef& event)
{
	;
}

extern "C"
{
	DLL_EXPORT void __CDECL__ OrkInitialize()
	{
		FilterRef filter(new SilkCodecDecoder());
		FilterRegistry::instance()->RegisterFilter(filter);
		LOG4CXX_INFO(s_log, "SILK codec filter initialized.");
	}
}



