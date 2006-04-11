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

#include "Filter.h"
extern "C"
{
#include "g711.h"
}
void FilterRegistry::RegisterFilter(FilterRef& Filter) 
{
	m_Filters.push_back(Filter);
}


FilterRef FilterRegistry::GetNewFilter(AudioEncodingEnum inputEncoding, AudioEncodingEnum outputEncoding)
{
	for(std::list<FilterRef>::iterator it = m_Filters.begin(); it!=m_Filters.end(); it++)
	{
		FilterRef Filter = *it;

		if(	Filter->GetInputAudioEncoding() == inputEncoding &&
			Filter->GetOutputAudioEncoding() == outputEncoding ) 
		{
			return Filter->Instanciate();
		}
	}
	return FilterRef();	// No filter found
}


FilterRef FilterRegistry::GetNewFilter(int rtpPayloadType)
{
	for(std::list<FilterRef>::iterator it = m_Filters.begin(); it!=m_Filters.end(); it++)
	{
		FilterRef Filter = *it;

		if(	Filter->GetInputRtpPayloadType() == rtpPayloadType ) 
		{
			return Filter->Instanciate();
		}
	}
	return FilterRef();	// No filter found
}


FilterRef FilterRegistry::GetNewFilter(CStdString& filterName)
{
	for(std::list<FilterRef>::iterator it = m_Filters.begin(); it!=m_Filters.end(); it++)
	{
		FilterRef Filter = *it;

		if(	Filter->GetName().CompareNoCase(filterName) == 0 ) 
		{
			return Filter->Instanciate();
		}
	}
	return FilterRef();	// No filter found
}

FilterRegistry* FilterRegistry::m_singleton = 0;

FilterRegistry* FilterRegistry::instance()
{
	if(m_singleton == NULL)
	{
		m_singleton = new FilterRegistry();
	}
	return m_singleton;
}

//====================================================================

int Filter::GetInputRtpPayloadType()
{
	// default: the filter does not accept any RTP payload type
	return -1;
}


//====================================================================


FilterRef AlawToPcmFilter::Instanciate()
{
	FilterRef Filter(new AlawToPcmFilter());
	return Filter;
}

void AlawToPcmFilter::AudioChunkIn(AudioChunkRef& inputAudioChunk)
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
	if(outputDetails.m_rtpPayloadType != GetInputRtpPayloadType())
	{
		return;
	}

	// Create output buffer
	m_outputAudioChunk.reset(new AudioChunk());
	outputDetails.m_rtpPayloadType = -1;		//  Override details that this filter changes
	outputDetails.m_encoding = PcmAudio;

	int numSamples = inputAudioChunk->GetNumSamples();
	short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(numSamples*2, outputDetails);
	char* inputBuffer = (char*)inputAudioChunk->m_pBuffer;
	

	for(int i=0; i<numSamples; i++)
	{
		outputBuffer[i] = (short)alaw2linear(inputBuffer[i]);
	}
}

void AlawToPcmFilter::AudioChunkOut(AudioChunkRef& chunk)
{
	chunk = m_outputAudioChunk;
}

AudioEncodingEnum AlawToPcmFilter::GetInputAudioEncoding()
{
	return AlawAudio;
}

AudioEncodingEnum AlawToPcmFilter::GetOutputAudioEncoding()
{
	return PcmAudio;
}

CStdString AlawToPcmFilter::GetName()
{
	return "ALawToPcm";
}

int AlawToPcmFilter::GetInputRtpPayloadType()
{
	return 0x8;
}


//====================================================================


FilterRef UlawToPcmFilter::Instanciate()
{
	FilterRef Filter(new UlawToPcmFilter());
	return Filter;
}

void UlawToPcmFilter::AudioChunkIn(AudioChunkRef& inputAudioChunk)
{
	m_outputAudioChunk.reset();

	if(inputAudioChunk.get() == NULL)
	{
		return;
	}
	else if(inputAudioChunk->GetNumSamples() == 0)
	{
		return;
	}

	AudioChunkDetails outputDetails = *inputAudioChunk->GetDetails();
	if(outputDetails.m_rtpPayloadType != GetInputRtpPayloadType())
	{
		return;
	}

	// Create output buffer
	m_outputAudioChunk.reset(new AudioChunk());
	outputDetails.m_rtpPayloadType = -1;		//  Override details that this filter changes
	outputDetails.m_encoding = PcmAudio;

	int numSamples = inputAudioChunk->GetNumSamples();
	short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(numSamples*2, outputDetails);
	char* inputBuffer = (char*)inputAudioChunk->m_pBuffer;
	

	for(int i=0; i<numSamples; i++)
	{
		outputBuffer[i] = (short)ulaw2linear(inputBuffer[i]);
	}	
}

void UlawToPcmFilter::AudioChunkOut(AudioChunkRef& chunk)
{
	chunk = m_outputAudioChunk;
}

AudioEncodingEnum UlawToPcmFilter::GetInputAudioEncoding()
{
	return UlawAudio;
}

AudioEncodingEnum UlawToPcmFilter::GetOutputAudioEncoding()
{
	return PcmAudio;
}

CStdString UlawToPcmFilter::GetName()
{
	return "UlawToPcm";
}

int UlawToPcmFilter::GetInputRtpPayloadType()
{
	return 0x0;
}
