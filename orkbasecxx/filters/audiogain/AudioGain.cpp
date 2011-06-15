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

#include "AudioGain.h"
#include "ConfigManager.h"

AudioGainFilter::AudioGainFilter()
{
	m_log = Logger::getLogger("audiogain");
	m_numEncodingErrors = 0;
	LOG4CXX_DEBUG(this->m_log, "Initialized AudioGain filter");
}

AudioGainFilter::~AudioGainFilter()
{
	if(m_numEncodingErrors > 0)
	{
		CStdString logMsg;
		logMsg.Format("Encoding error happened %d time(s)", m_numEncodingErrors);
		LOG4CXX_WARN(m_log, logMsg);
	}
	LOG4CXX_DEBUG(this->m_log, "Decommissioned AudioGain filter");
}

FilterRef AudioGainFilter::Instanciate()
{
	FilterRef Filter(new AudioGainFilter());
	return Filter;
}

void AudioGainFilter::AudioChunkIn(AudioChunkRef& inputAudioChunk)
{
	int r_samples = 0;
	int i = 0;
	AudioChunkDetails outputDetails;

	m_outputAudioChunk.reset();
	if(inputAudioChunk.get() == NULL) {
		return;
	}

	if(inputAudioChunk->GetNumSamples() == 0) {
		return;
	}

	outputDetails = *inputAudioChunk->GetDetails();
	r_samples = inputAudioChunk->GetNumSamples();
	m_outputAudioChunk.reset(new AudioChunk());

	if(inputAudioChunk->GetEncoding() != PcmAudio)
	{
		if(m_numEncodingErrors == 0)
		{
			CStdString logMsg;

			logMsg.Format("Unexpected encoding:%d expected:%d (PcmAudio), gain not applied", inputAudioChunk->GetEncoding(), PcmAudio);
			LOG4CXX_WARN(m_log, logMsg);
		}
		m_numEncodingErrors++;
		m_outputAudioChunk->SetBuffer(inputAudioChunk->m_pBuffer, outputDetails);
		return;
	}

	short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(outputDetails);
	short* inputBuffer = (short*)inputAudioChunk->m_pBuffer;
	int sample = 0;
	double multiplier, multiplier1, multiplier2;

	multiplier = 0.0;
	multiplier1 = 0.0;
	multiplier2 = 0.0;

	multiplier = pow(10, (CONFIG.m_audioGainDb / 20.0));
	multiplier1 = pow(10, (CONFIG.m_audioGainChannel1Db / 20.0));
	multiplier2 = pow(10, (CONFIG.m_audioGainChannel2Db / 20.0));

	for(i = 0; i < r_samples; i++) {
		sample = inputBuffer[i];
		if(CONFIG.m_audioGainDb != 0)
		{
			sample = sample * multiplier;
		}
		if(CONFIG.m_audioGainChannel1Db != 0)
		{
			if(outputDetails.m_channel == 1)
			{
				sample = sample * multiplier1;
			}
		}
		if(CONFIG.m_audioGainChannel2Db != 0)
		{
			if(outputDetails.m_channel == 2)
			{
				sample = sample * multiplier2;
			}
		}

		if(sample < -32768)
		{
			sample = -32768;
		}
		if(sample > 32767)
		{
			sample = 32767;
		}

		outputBuffer[i] = sample;
	}
}

void AudioGainFilter::AudioChunkOut(AudioChunkRef& chunk)
{
	chunk = m_outputAudioChunk;
}

AudioEncodingEnum AudioGainFilter::GetInputAudioEncoding()
{
	return PcmAudio;
}

AudioEncodingEnum AudioGainFilter::GetOutputAudioEncoding()
{
	return PcmAudio;
}

CStdString AudioGainFilter::GetName()
{
	return "AudioGain";
}

void AudioGainFilter::CaptureEventIn(CaptureEventRef& event)
{
	;
}

void AudioGainFilter::CaptureEventOut(CaptureEventRef& event)
{
	;
}

