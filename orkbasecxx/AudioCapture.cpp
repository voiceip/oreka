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
	m_details.Clear();
	m_pBuffer = NULL;
}

AudioChunk::~AudioChunk()
{
	if(m_pBuffer)
	{
		free(m_pBuffer);
	}
}

void* AudioChunk::CreateBuffer(size_t numBytes, AudioChunkDetails& details)
{
	if(m_pBuffer)
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
		m_details.m_numBytes = 0;
	}
	if(numBytes)
	{
		m_pBuffer = calloc(numBytes, 1);
	}
	if (!m_pBuffer)
	{
		CStdString numBytesString = IntToString(numBytes);
		throw("AudioChunk::AudioChunk: could not calloc a buffer of size:" + numBytesString);
	}
	else
	{
		m_details = details;
		m_details.m_numBytes = numBytes;
	}
	return m_pBuffer;
}

void AudioChunk::SetBuffer(void* pBuffer, size_t numBytes, AudioChunkDetails& details)
{
	if(m_pBuffer)
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
		m_details.m_numBytes = 0;
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
			memcpy(m_pBuffer, pBuffer, numBytes);
			m_details = details;
			m_details.m_numBytes = numBytes;
		}
	}
}

int AudioChunk::GetNumSamples()
{
	switch(m_details.m_encoding)
	{
	case PcmAudio:
		return m_details.m_numBytes/2;
	case AlawAudio: case UlawAudio:
		return m_details.m_numBytes;
	default:
		throw(CStdString("AudioChunk::GetNumSamples: unknown encoding"));
	}
}

double AudioChunk::GetDurationSec()
{
	int i = 0;
	return ((double)GetNumSamples())/((double)m_details.m_sampleRate);
}


double AudioChunk::ComputeRms()
{
	double rmsValue = 0;
	if(m_details.m_encoding == PcmAudio)
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
	if(m_details.m_encoding == PcmAudio)
	{
		rmsDbValue = 10 * log10(ComputeRms()/32768.0);
	}
	return rmsDbValue;
}

AudioEncodingEnum AudioChunk::GetEncoding()
{
	return m_details.m_encoding;
}

int AudioChunk::GetSampleRate()
{
	return m_details.m_sampleRate;
}

AudioChunkDetails* AudioChunk::GetDetails()
{
	return &m_details;
}

void AudioChunk::SetDetails(AudioChunkDetails* details)
{
	m_details = *details;
}


int AudioChunk::GetNumBytes()
{
	return m_details.m_numBytes;
}

//================================
AudioChunkDetails::AudioChunkDetails()
{
	Clear();
}

void AudioChunkDetails::Clear()
{
	m_marker = MEDIA_CHUNK_MARKER;
	m_encoding = UnknownAudio;
	m_timestamp = 0;
	m_arrivalTimestamp = 0;
	m_sequenceNumber = 0;
	m_numBytes = 0;
	m_sampleRate = 8000;
	m_rtpPayloadType = -1;
	m_channel = 0;			// mono by default
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
	case	EtLocalIp:
		return ET_LOCALIP;
	case	EtRemoteIp:
		return ET_REMOTEIP;
	case	EtLocalMac:
		return ET_LOCALMAC;
	case	EtRemoteMac:
		return ET_REMOTEMAC;
	case	EtOrkUid:
		return ET_ORKUID;
	case	EtEndMetadata:
		return ET_ENDMETADATA;
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
	else if (eventTypeString.CompareNoCase(ET_LOCALIP) == 0)
	{
		eventTypeEnum = EtLocalIp;
	}
	else if (eventTypeString.CompareNoCase(ET_REMOTEIP) == 0)
	{
		eventTypeEnum = EtRemoteIp;
	}
	else if (eventTypeString.CompareNoCase(ET_LOCALMAC) == 0)
	{
		eventTypeEnum = EtLocalMac;
	}
	else if (eventTypeString.CompareNoCase(ET_REMOTEMAC) == 0)
	{
		eventTypeEnum = EtRemoteMac;
	}
	else if (eventTypeString.CompareNoCase(ET_ORKUID) == 0)
	{
		eventTypeEnum = EtOrkUid;
	}
	else if (eventTypeString.CompareNoCase(ET_ENDMETADATA) == 0)
	{
		eventTypeEnum = EtEndMetadata;
	}
	return eventTypeEnum;
}

//========================================
// File format related methods

int FileFormatToEnum(CStdString& format)
{
	int formatEnum = FfUnknown;
	if(format.CompareNoCase(FF_NATIVE) == 0)
	{
		formatEnum = FfNative;
	}
	else if (format.CompareNoCase(FF_GSM) == 0)
	{
		formatEnum = FfGsm;
	}
	else if (format.CompareNoCase(FF_ULAW) == 0)
	{
		formatEnum = FfUlaw;
	}
	else if (format.CompareNoCase(FF_ALAW) == 0)
	{
		formatEnum = FfAlaw;
	}
	else if (format.CompareNoCase(FF_PCMWAV) == 0)
	{
		formatEnum = FfPcmWav;
	}
	return formatEnum;
}

CStdString FileFormatToString(int formatEnum)
{
	CStdString formatString;
	switch (formatEnum)
	{
	case FfNative:
		formatString = FF_NATIVE;
		break;
	case FfGsm:
		formatString = FF_GSM;
		break;
	case FfUlaw:
		formatString = FF_ULAW;
		break;
	case FfAlaw:
		formatString = FF_ALAW;
		break;
	case FfPcmWav:
		formatString = FF_PCMWAV;
		break;
	default:
		formatString = FF_UNKNOWN;
	}
	return formatString;
}

CStdString FileFormatGetExtension(FileFormatEnum formatEnum)
{
	CStdString extension;
	switch (formatEnum)
	{
	case FfGsm:
	case FfUlaw:
	case FfAlaw:
	case FfPcmWav:
		extension = ".wav";
		break;
	default:
		CStdString formatEnumString = IntToString(formatEnum);
		throw (CStdString("AudioTape::GetFileFormatExtension: unknown file format:") + formatEnumString);
	}
	return extension;
}



