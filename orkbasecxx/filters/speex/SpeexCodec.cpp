#include "SpeexCodec.h"
#include <stdio.h>
#include <vector>
#include "ConfigManager.h"

static log4cxx::LoggerPtr s_log;

static int s_numPacketsDecoded;

const SpeexMode * mode;

SpeexDecoder::SpeexDecoder()
{
	mode    = speex_lib_get_mode (SPEEX_MODEID_NB);
	m_state = speex_decoder_init(mode);
	
	speex_decoder_ctl(m_state, SPEEX_GET_FRAME_SIZE, &m_frameSize);
	speex_decoder_ctl(m_state, SPEEX_SET_ENH, &m_enh);
	speex_bits_init(&m_bits);

	m_initialized = true;
}

SpeexDecoder::~SpeexDecoder()
{
	speex_bits_destroy(&m_bits);	
	speex_decoder_destroy(m_state);
}

FilterRef SpeexDecoder::Instanciate()
{
	SpeexDecoder* decoder = new SpeexDecoder();
	FilterRef filter(decoder);
	if(decoder->m_initialized)
	{
		return filter;
	}
	return FilterRef();
}

void SpeexDecoder::AudioChunkIn(AudioChunkRef& inputChunk)
{
	CStdString logMsg;

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

	// 16 bit per sample

	int frame_size;
	
	m_outputAudioChunk.reset(new AudioChunk());

	AudioChunkDetails outputDetails = *inputChunk->GetDetails();	// pass through all details
	outputDetails.m_rtpPayloadType = -1;							// and override the ones that this filter changes
	outputDetails.m_encoding = PcmAudio;
	
	speex_bits_read_from(&m_bits,(char *) inputChunk->m_pBuffer, inputChunk->GetNumBytes() );
	
	// rtp payload could contain multiple speex packets, 
	// first store all in a vector than create the final output buffer
	std::vector< short * > memVector;

	int err;
	
	while(1) 
	{
		short * mem =  new short[m_frameSize];

		err = speex_decode_int(m_state, &m_bits, mem);
	   
		s_numPacketsDecoded++;

		if (err==-1)
		{
			delete mem;
			break;
		}
		else
		{
			memVector.push_back( mem );
		}
	}

	int vecSize = memVector.size();

	outputDetails.m_numBytes = m_frameSize * sizeof(short) * vecSize ;
	short* outputBuffer = (short *)m_outputAudioChunk->CreateBuffer(outputDetails);

	for(int i=0;i<vecSize;i++)
	{
		memcpy(&outputBuffer[m_frameSize*i],memVector[i],m_frameSize * sizeof(short));

		delete memVector[i];
	}

	
}

void SpeexDecoder::AudioChunkOut(AudioChunkRef& chunkOut)
{
	chunkOut = m_outputAudioChunk;
}
AudioEncodingEnum SpeexDecoder::GetInputAudioEncoding()
{
	return UnknownAudio;;
}

AudioEncodingEnum SpeexDecoder::GetOutputAudioEncoding()
{
	return PcmAudio;;
}

CStdString SpeexDecoder::GetName()
{
	return CStdString("SpeexDecoder");
}

bool SpeexDecoder::SupportsInputRtpPayloadType(int rtpPayloadType)
{
	return rtpPayloadType == 66;
}

void SpeexDecoder::CaptureEventIn(CaptureEventRef &event)
{
	;
}

void SpeexDecoder::CaptureEventOut(CaptureEventRef &event)
{
	;
}
