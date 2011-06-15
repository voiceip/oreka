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

#include "GsmFilters.h"

GsmToPcmFilter::GsmToPcmFilter()
{
	CStdString logMsg;

	gsmState = gsm_create();
	if(!gsmState)
	{
		logMsg = "gsm_create() returned memory allocation problem";
		throw(logMsg);
	}
}

GsmToPcmFilter::~GsmToPcmFilter()
{
	gsm_destroy(gsmState);
}

FilterRef GsmToPcmFilter::Instanciate()
{
	FilterRef Filter(new GsmToPcmFilter());
	return Filter;
}

void GsmToPcmFilter::AudioChunkIn(AudioChunkRef& inputAudioChunk)
{
	m_outputAudioChunk.reset();

	if(inputAudioChunk.get() == NULL)
	{
		return;
	}
	if(inputAudioChunk->GetNumSamples() == 0)
	{
		return;
	}
	AudioChunkDetails outputDetails = *inputAudioChunk->GetDetails();

	if(SupportsInputRtpPayloadType(outputDetails.m_rtpPayloadType) == false)
	{
		return;
	}

	// Create output buffer
	m_outputAudioChunk.reset(new AudioChunk());
	outputDetails.m_rtpPayloadType = -1;		//  Override details that this filter changes
	outputDetails.m_encoding = PcmAudio;
	int numSamples = 160;	// standard GSM frame is 33 bytes for 160 PCM samples (20 ms)
	outputDetails.m_numBytes = numSamples*2;
	short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(outputDetails);
	unsigned char* inputBuffer = (unsigned char*)inputAudioChunk->m_pBuffer;
	gsm_decode(gsmState, inputBuffer, outputBuffer);
}

void GsmToPcmFilter::AudioChunkOut(AudioChunkRef& chunk)
{
	chunk = m_outputAudioChunk;
}

AudioEncodingEnum GsmToPcmFilter::GetInputAudioEncoding()
{
	return GsmAudio;
}

AudioEncodingEnum GsmToPcmFilter::GetOutputAudioEncoding()
{
	return PcmAudio;
}

CStdString GsmToPcmFilter::GetName()
{
	return "GsmToPcm";
}

bool GsmToPcmFilter::SupportsInputRtpPayloadType(int rtpPayloadType)
{
	return  rtpPayloadType == 0x3;
}

void GsmToPcmFilter::CaptureEventIn(CaptureEventRef& event)
{
	;
}

void GsmToPcmFilter::CaptureEventOut(CaptureEventRef& event)
{
	;
}

