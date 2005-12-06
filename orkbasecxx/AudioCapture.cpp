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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Utils.h"
#include "math.h"
#include "AudioCapture.h"


AudioChunk::AudioChunk()
{
	m_encoding = UnknownAudio;
	m_timestamp = 0;
	m_sequenceNumber = 0;
	m_numBytes = 0;
	m_pBuffer = NULL;
	m_sampleRate = 8000;
}

AudioChunk::~AudioChunk()
{
	if(m_pBuffer)
	{
		free(m_pBuffer);
	}
}

void AudioChunk::SetBuffer(void* pBuffer, size_t numBytes, AudioEncodingEnum encoding, unsigned int timestamp, unsigned int sequenceNumber, unsigned int sampleRate)
{
	if(m_pBuffer)
	{
		free(m_pBuffer);
		m_numBytes = 0;
	}
	if(numBytes && pBuffer)
	{
		m_pBuffer = malloc(numBytes);
		if (!m_pBuffer)
		{
			CStdString numBytesString = IntToString(numBytes);
			throw("AudioChunk::AudioChunk: could not malloc a buffer of size:" + numBytesString);
		}
		else
		{
			m_numBytes = numBytes;
			memcpy(m_pBuffer, pBuffer, numBytes);
			m_encoding = encoding;
			m_timestamp = timestamp;
			m_sequenceNumber = sequenceNumber;
			m_sampleRate = sampleRate;
		}
	}
}

int AudioChunk::GetNumSamples()
{
	switch(m_encoding)
	{
	case PcmAudio:
		return m_numBytes/2;
	case AlawAudio: case UlawAudio:
		return m_numBytes;
	default:
		throw(CStdString("AudioChunk::GetNumSamples: unknown encoding"));
	}
}

double AudioChunk::GetDurationSec()
{
	int i = 0;
	return ((double)GetNumSamples())/((double)m_sampleRate);
}


double AudioChunk::ComputeRms()
{
	double rmsValue = 0;
	if(m_encoding == PcmAudio)
	{
		for(int i=0; i<GetNumSamples(); i++)
		{
			rmsValue += ((short *)m_pBuffer)[i] * ((short *)m_pBuffer)[i];
		}
		rmsValue = sqrt(rmsValue/GetNumSamples());
	}
	return rmsValue;
}

double AudioChunk::ComputeRmsDb()
{
	double rmsDbValue = 10 * log10(1.0/32768.0);	// default value, the lowest possible
	if(m_encoding == PcmAudio)
	{
		rmsDbValue = 10 * log10(ComputeRms()/32768.0);
	}
	return rmsDbValue;
}



//=================================

CaptureEvent::CaptureEvent()
{
	m_timestamp = 0;
	m_type = EtUnknown;
}

CStdString CaptureEvent::DirectionToString(int direction)
{
	switch(direction)
	{
	case DirIn:
		return DIR_IN;
	case DirOut:
		return DIR_OUT;
	}
	return DIR_UNKN;
}

int CaptureEvent::DirectionToEnum(CStdString& dir)
{
	if(dir.CompareNoCase(DIR_IN) == 0)
	{
		return DirIn;
	}
	else if(dir.CompareNoCase(DIR_OUT) == 0)
	{
		return DirOut;
	}
	return DirUnkn;
}

CStdString CaptureEvent::EventTypeToString(int eventTypeEnum)
{
	switch(eventTypeEnum)
	{
	case	EtUnknown:
		return ET_UNKNOWN;
	case	EtStart:
		return ET_START;
	case	EtStop:
		return ET_STOP;
	case	EtDirection:
		return ET_DIRECTION;
	case	EtRemoteParty:
		return ET_REMOTEPARTY;
	case	EtLocalParty:
		return ET_LOCALPARTY;
	case	EtLocalEntryPoint:
		return ET_LOCALENTRYPOINT;
	case	EtKeyValue:
		return ET_KEYVALUE;
	}
	return ET_INVALID;
}

int CaptureEvent::EventTypeToEnum(CStdString& eventTypeString)
{
	int eventTypeEnum = EtUnknown;
	if(eventTypeString.CompareNoCase(ET_START) == 0)
	{
		eventTypeEnum = EtStart;
	}
	else if (eventTypeString.CompareNoCase(ET_STOP) == 0)
	{
		eventTypeEnum = EtStop;
	}
	else if (eventTypeString.CompareNoCase(ET_DIRECTION) == 0)
	{
		eventTypeEnum = EtDirection;
	}
	else if (eventTypeString.CompareNoCase(ET_REMOTEPARTY) == 0)
	{
		eventTypeEnum = EtRemoteParty;
	}
	else if (eventTypeString.CompareNoCase(ET_LOCALPARTY) == 0)
	{
		eventTypeEnum = EtLocalParty;
	}
	else if (eventTypeString.CompareNoCase(ET_LOCALENTRYPOINT) == 0)
	{
		eventTypeEnum = EtLocalEntryPoint;
	}
	else if (eventTypeString.CompareNoCase(ET_KEYVALUE) == 0)
	{
		eventTypeEnum = EtKeyValue;
	}
	return eventTypeEnum;
}

