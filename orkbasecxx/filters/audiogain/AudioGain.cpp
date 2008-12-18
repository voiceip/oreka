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
	LOG4CXX_INFO(this->m_log, "Initialized AudioGain filter");
}

AudioGainFilter::~AudioGainFilter()
{
	if(m_numEncodingErrors > 0)
	{
		CStdString logMsg;
		logMsg.Format("Encoding error happened %d time(s)", m_numEncodingErrors);
		LOG4CXX_WARN(m_log, logMsg);
	}
	LOG4CXX_INFO(this->m_log, "Decommissioned AudioGain filter");
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

	for(i = 0; i < r_samples; i++) {
		sample = inputBuffer[i];
		if(CONFIG.m_audioGain != 0)
		{
			if(CONFIG.m_audioGain < 0)
			{
				sample = (int)((double)sample / sqrt(fabs(CONFIG.m_audioGain)));
			}
			else
			{
				sample = (int)((double)sample * sqrt(fabs(CONFIG.m_audioGain)));
			}
		}
		if(CONFIG.m_audioGainChannel1 != 0)
		{
			if(outputDetails.m_channel == 1)
			{
				if(CONFIG.m_audioGainChannel1 < 0)
				{
					sample = (int)((double)sample / sqrt(fabs(CONFIG.m_audioGainChannel1)));
				}
				else
				{
					sample = (int)((double)sample * sqrt(fabs(CONFIG.m_audioGainChannel1)));
				}
			}
		}
		if(CONFIG.m_audioGainChannel2 != 0)
		{
			if(outputDetails.m_channel == 2)
			{
				if(CONFIG.m_audioGainChannel2 < 0)
				{
					sample = (int)((double)sample / sqrt(fabs(CONFIG.m_audioGainChannel2)));
				}
				else
				{
					sample = (int)((double)sample / sqrt(fabs(CONFIG.m_audioGainChannel2)));
				}
			}
		}

		if(sample < -32768)
		{
			sample = -32768;
		}
		if(sample > 32768)
		{
			sample = 32768;
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

int AudioGainFilter::GetInputRtpPayloadType()
{
	return -1;
}

void AudioGainFilter::CaptureEventIn(CaptureEventRef& event)
{
	;
}

void AudioGainFilter::CaptureEventOut(CaptureEventRef& event)
{
	;
}

