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

Filter::Filter()
{
	m_numOutputChannels = 1;
}

void Filter::SetSessionInfo(CStdString& trackingId)
{
	m_trackingId = trackingId;
}

bool Filter::SupportsInputRtpPayloadType( int rtpPayloadType )
{
	return false;
}

void Filter::Configure(FilterConfigurationParametersRef configParams)
{
;
}

void Filter::SetNumOutputChannels(int numChan)
{
	m_numOutputChannels = numChan;
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

		if(Filter->SupportsInputRtpPayloadType(rtpPayloadType) == true) 
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

	if(SupportsInputRtpPayloadType(outputDetails.m_rtpPayloadType) == false)
	{
		return;
	}

	// Create output buffer
	m_outputAudioChunk.reset(new AudioChunk());
	outputDetails.m_rtpPayloadType = -1;		//  Override details that this filter changes
	outputDetails.m_encoding = PcmAudio;
	int numSamples = inputAudioChunk->GetNumSamples();
	outputDetails.m_numBytes = numSamples*2;
	short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(outputDetails);
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

bool AlawToPcmFilter::SupportsInputRtpPayloadType(int rtpPayloadType)
{
	return rtpPayloadType == 0x8;
}

void AlawToPcmFilter::CaptureEventIn(CaptureEventRef& event)
{
	;
}

void AlawToPcmFilter::CaptureEventOut(CaptureEventRef& event)
{
	;
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
	if(SupportsInputRtpPayloadType(outputDetails.m_rtpPayloadType) == false)
	{
		return;
	}

	// Create output buffer
	m_outputAudioChunk.reset(new AudioChunk());
	outputDetails.m_rtpPayloadType = -1;		//  Override details that this filter changes
	outputDetails.m_encoding = PcmAudio;
	int numSamples = inputAudioChunk->GetNumSamples();
	outputDetails.m_numBytes = numSamples*2;
	short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(outputDetails);
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

bool UlawToPcmFilter::SupportsInputRtpPayloadType(int rtpPayloadType)
{
	return rtpPayloadType == 0x0;
}

void UlawToPcmFilter::CaptureEventIn(CaptureEventRef& event)
{
	;
}

void UlawToPcmFilter::CaptureEventOut(CaptureEventRef& event)
{
	;
}
