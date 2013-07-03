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

#include "G721Codec.h"

G721CodecDecoder::G721CodecDecoder()
{
	// Initialize decoder
	g72x_init_state(&m_decoderState);
}

G721CodecDecoder::~G721CodecDecoder()
{
	memset(&m_decoderState, 0, sizeof(m_decoderState));
}

FilterRef G721CodecDecoder::Instanciate()
{
	FilterRef Filter(new G721CodecDecoder());
	return Filter;
}

void G721CodecDecoder::AudioChunkIn(AudioChunkRef& inputAudioChunk)
{
	unsigned char* inputBuffer;
	short outputSample = 0;
	short pcmdata[8000];
	int input_size = 0;
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
		return;
	}

	inputBuffer = (unsigned char*)inputAudioChunk->m_pBuffer;
	input_size = outputDetails.m_numBytes;
	int j = 0;
	for(int i=0; i<input_size; i++)
	{
		unsigned char inputSample = inputBuffer[i];
		unsigned char lower4bits = inputSample & 0xF;
		unsigned char upper4bits = (inputSample>>4) & 0xF;
		outputSample = g721_decoder(lower4bits, AUDIO_ENCODING_LINEAR, &m_decoderState);
		memcpy(pcmdata + j, &outputSample, sizeof(short));
		outputSample = g721_decoder(upper4bits, AUDIO_ENCODING_LINEAR, &m_decoderState);
		memcpy(pcmdata + j + 1, &outputSample, sizeof(short));
		j += 2;
	}

	m_outputAudioChunk.reset(new AudioChunk());
	outputDetails.m_rtpPayloadType = -1;
	outputDetails.m_encoding = PcmAudio;
	outputDetails.m_numBytes = (input_size * 4);
	short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(outputDetails);
	memcpy(outputBuffer, pcmdata, outputDetails.m_numBytes);
}

void G721CodecDecoder::AudioChunkOut(AudioChunkRef& chunk)
{
	chunk = m_outputAudioChunk;
}

AudioEncodingEnum G721CodecDecoder::GetInputAudioEncoding()
{
	return G721Audio;
}

AudioEncodingEnum G721CodecDecoder::GetOutputAudioEncoding()
{
	return PcmAudio;
}

CStdString G721CodecDecoder::GetName()
{
	return "G721CodecDecoder";
}

bool G721CodecDecoder::SupportsInputRtpPayloadType(int rtpPayloadType)
{
	return rtpPayloadType == 2;
}

void G721CodecDecoder::CaptureEventIn(CaptureEventRef& event)
{
	;
}

void G721CodecDecoder::CaptureEventOut(CaptureEventRef& event)
{
	;
}

