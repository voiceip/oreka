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
	m_numChannels = 0;
	m_pChannelAudio = NULL;
}

AudioChunk::AudioChunk(int numChannels)
{
	m_details.Clear();
	m_pBuffer = NULL;
	m_pChannelAudio = NULL;
	if(numChannels <= 0)
	{
		// Force at least one channel to resolve this possible
		// error condition
		numChannels = 1;
	}

	m_numChannels = numChannels;
	m_pChannelAudio = (void**)calloc(m_numChannels, sizeof(void*));
	m_details.m_channel = 100;

	if(!m_pChannelAudio)
	{
		CStdString numBytesString;

		numBytesString.Format("%d", m_numChannels*sizeof(void*));
		throw("AudioChunk::AudioChunk(numChannels) could not allocate a buffer of size " + numBytesString);
	}
}

AudioChunk::~AudioChunk()
{
	FreeAll();
}

void AudioChunk::ToString(CStdString& string)
{
	if(!m_numChannels)
	{
		string.Format("encoding:%d numBytes:%u ts:%u ats:%u seq:%u rtp-pt:%d ch:%u", 
			m_details.m_encoding, m_details.m_numBytes, m_details.m_timestamp, m_details.m_arrivalTimestamp, 
			m_details.m_sequenceNumber, m_details.m_rtpPayloadType, m_details.m_channel);
	}
	else
	{
		string.Format("encoding:%d numBytesPerChannel:%u numChannels:%d ts:%u ats:%u seq:%u rtp-pt:%d",
			m_details.m_encoding, m_details.m_numBytes, m_numChannels, m_details.m_timestamp,
			m_details.m_arrivalTimestamp, m_details.m_sequenceNumber, m_details.m_rtpPayloadType);
	}
}

void AudioChunk::FreeAll()
{
	if(m_pBuffer)
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
	}
	if(m_numChannels)
	{
		for(int i = 0; i < m_numChannels; i++)
		{
			if(m_pChannelAudio[i])
			{
				free(m_pChannelAudio[i]);
			}
		}

		free(m_pChannelAudio);
		m_pChannelAudio = NULL;
	}
}

void AudioChunk::CreateMultiChannelBuffers(AudioChunkDetails& details)
{
	if(!m_numChannels)
	{
		return;
	}

	for(int i = 0; i < m_numChannels; i++)
	{
		m_pChannelAudio[i] = calloc(details.m_numBytes, 1);
		if(!m_pChannelAudio[i])
		{
			CStdString exception;

			exception.Format("AudioChunk::CreateMultiChannelBuffers failed to calloc buffer of size:%d for channel:%d", details.m_numBytes, i);
			throw(exception);
		}
	}
}

void* AudioChunk::CreateBuffer(AudioChunkDetails& details)
{
	FreeAll();
	if(details.m_numBytes)
	{
		m_pBuffer = calloc(details.m_numBytes, 1);
		CreateMultiChannelBuffers(details);
	}
	if (!m_pBuffer)
	{
		CStdString numBytesString = IntToString(details.m_numBytes);
		throw("AudioChunk::AudioChunk: could not calloc a buffer of size:" + numBytesString);
	}
	else
	{
		m_details = details;
	}
	return m_pBuffer;
}

void AudioChunk::SetBuffer(void* pBuffer, AudioChunkDetails& details)
{
	if(m_pBuffer)
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
		m_details.m_numBytes = 0;
	}
	if(details.m_numBytes && pBuffer)
	{
		m_pBuffer = malloc(details.m_numBytes);
		if (!m_pBuffer)
		{
			CStdString numBytesString = IntToString(details.m_numBytes);
			throw("AudioChunk::AudioChunk: could not malloc a buffer of size:" + numBytesString);
		}
		else
		{
			memcpy(m_pBuffer, pBuffer, details.m_numBytes);
			m_details = details;
		}
	}
}

void AudioChunk::SetBuffer(void* pBuffer, AudioChunkDetails& details, int chan)
{
	CStdString exception;
	int chanIdx = 0;

	if(chan > m_numChannels || chan < 1)
	{
		exception.Format("AudioChunk::SetBuffer: invalid channel %d", chan);
		throw(exception);
	}

	chanIdx = chan - 1;
	if(m_pChannelAudio[chanIdx])
	{
		free(m_pChannelAudio[chanIdx]);
		m_pChannelAudio[chanIdx] = NULL;
	}
	if(details.m_numBytes && pBuffer)
	{
		m_pChannelAudio[chanIdx] = malloc(details.m_numBytes);
		if(!m_pChannelAudio[chanIdx])
		{
			exception.Format("AudioChunk::SetBuffer: failed to allocate buffer of size:%d for channel:%d channelidx:%d", details.m_numBytes, chan, chanIdx);
			throw(exception);
		}
		else
		{
			memcpy(m_pChannelAudio[chanIdx], pBuffer, details.m_numBytes);
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
		CStdString msg;
		ToString(msg);
		throw(CStdString("AudioChunk::GetNumSamples(): unknown encoding. Chunk:") + msg);
	}
}

double AudioChunk::GetDurationSec()
{
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
	m_offsetMs = 0;
}

CStdString CaptureEvent::DirectionToShortString(int direction)
{
        switch(direction)
        {
        case DirIn:
                return DIR_IN_SHORT;
        case DirOut:
                return DIR_OUT_SHORT;
        }
        return DIR_UNKN_SHORT;
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

CStdString CaptureEvent::LocalSideToString(int localSideEnum)
{
	switch(localSideEnum)
	{
	case	LocalSideUnkn:
		return LOCALSIDE_UNKN;
	case	LocalSideSide1:
		return LOCALSIDE_SIDE1;
	case	LocalSideSide2:
		return LOCALSIDE_SIDE2;
	case	LocalSideBoth:
		return LOCALSIDE_BOTH;
	}
	return LOCALSIDE_INVALID;
}

int CaptureEvent::LocalSideToEnum(CStdString& localSideString)
{
	if(localSideString.CompareNoCase(LOCALSIDE_UNKN) == 0)
	{
		return LocalSideUnkn;
	}
	else if (localSideString.CompareNoCase(LOCALSIDE_SIDE1) == 0)
	{
		return LocalSideSide1;
	}
	else if (localSideString.CompareNoCase(LOCALSIDE_SIDE2) == 0)
	{
		return LocalSideSide2;
	}
	else if (localSideString.CompareNoCase(LOCALSIDE_BOTH) == 0)
	{
		return LocalSideBoth;
	}
	return LocalSideInvalid;
}

CStdString CaptureEvent::AudioKeepDirectionToString(int audioKeepDirectionEnum)
{
	switch(audioKeepDirectionEnum)
	{
	case AudioKeepDirectionBoth:
		return AUDIOKEEPDIRECTION_BOTH;
	case AudioKeepDirectionLocal:
		return AUDIOKEEPDIRECTION_LOCAL;
	case AudioKeepDirectionRemote:
		return AUDIOKEEPDIRECTION_REMOTE;
	case AudioKeepDirectionNone:
		return AUDIOKEEPDIRECTION_NONE;
	}
	return AUDIOKEEPDIRECTION_INVALID;
}

int CaptureEvent::AudioKeepDirectionToEnum(CStdString& audioKeepDirectionString)
{
	if(audioKeepDirectionString.CompareNoCase(AUDIOKEEPDIRECTION_BOTH) == 0)
	{
		return AudioKeepDirectionBoth;
	}
	else if (audioKeepDirectionString.CompareNoCase(AUDIOKEEPDIRECTION_LOCAL) == 0)
	{
		return AudioKeepDirectionLocal;
	}
	else if (audioKeepDirectionString.CompareNoCase(AUDIOKEEPDIRECTION_REMOTE) == 0)
	{
		return AudioKeepDirectionRemote;
	}
	else if (audioKeepDirectionString.CompareNoCase(AUDIOKEEPDIRECTION_NONE) == 0)
	{
		return AudioKeepDirectionNone;
	}
	return AudioKeepDirectionInvalid;
}

bool CaptureEvent::AudioKeepDirectionIsDefault(CStdString& audioKeepDirectionString)
{
	bool result = false;

	if(audioKeepDirectionString.CompareNoCase(AUDIOKEEPDIRECTION_BOTH) == 0)
	{
		result = true;
	}

	return result;
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
	case	EtReady:
		return ET_READY;
	case	EtUpdate:
		return ET_UPDATE;
	case	EtCallId:
		return ET_CALLID;
	case	EtLocalSide:
		return ET_LOCALSIDE;
	case	EtAudioKeepDirection:
		return ET_AUDIOKEEPDIRECTION;
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
	else if (eventTypeString.CompareNoCase(ET_READY) == 0)
	{
		eventTypeEnum = EtReady;
	}
	else if (eventTypeString.CompareNoCase(ET_UPDATE) == 0)
	{
		eventTypeEnum = EtUpdate;
	}
	else if (eventTypeString.CompareNoCase(ET_CALLID) == 0)
	{
		eventTypeEnum = EtCallId;
	}
	else if (eventTypeString.CompareNoCase(ET_LOCALSIDE) == 0)
	{
		eventTypeEnum = EtLocalSide;
	}
	else if (eventTypeString.CompareNoCase(ET_AUDIOKEEPDIRECTION) == 0)
	{
		eventTypeEnum = EtAudioKeepDirection;
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



