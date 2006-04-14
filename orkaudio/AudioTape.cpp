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

#include "ConfigManager.h"
#include "AudioTape.h"
#include "ace/OS_NS_time.h"
#include "Utils.h"
#include "ThreadSafeQueue.h"
#include "LogManager.h"
#include "audiofile/PcmFile.h"
#include "audiofile/LibSndFileFile.h"
#include "audiofile/MediaChunkFile.h"
#include "messages/TapeMsg.h"

AudioTapeDescription::AudioTapeDescription()
{
	m_direction = CaptureEvent::DirUnkn;
	m_duration = 0;
	m_beginDate = 0;
}

void AudioTapeDescription::Define(Serializer* s)
{
	s->DateValue("date", m_beginDate);
	s->IntValue("duration", m_duration);
	s->EnumValue("direction", (int&)m_direction, CaptureEvent::DirectionToEnum, CaptureEvent::DirectionToString);
	s->StringValue("capturePort", m_capturePort);
	s->StringValue("localParty", m_localParty);
	s->StringValue("remoteParty", m_remoteParty);
	s->StringValue("localEntryPoint", m_localEntryPoint);
	s->StringValue("localIp", m_localIp);
	s->StringValue("remoteIp", m_remoteIp);
}

void AudioTapeDescription::Validate(){}

CStdString AudioTapeDescription::GetClassName()
{
	return "tapedescription";
}

ObjectRef AudioTapeDescription::NewInstance()
{
	ObjectRef ref(new AudioTapeDescription());
	return ref;
}

ObjectRef AudioTapeDescription::Process() 
{
	ObjectRef ref;
	return ref;
}


//===================================================
AudioTape::AudioTape(CStdString &portId)
{
	m_portId = portId;
	m_state = StateCreated;
	m_beginDate = ACE_OS::time(NULL);
	m_endDate = 0;
	m_duration = 0;
	m_direction = CaptureEvent::DirUnkn;
	m_shouldStop = false;
	m_readyForBatchProcessing = false;

	GenerateFilePathAndIdentifier();
}


void AudioTape::AddAudioChunk(AudioChunkRef chunkRef)
{
	// Add the chunk to the local queue
	if(m_state == StateCreated || m_state == StateActive)
	{
		MutexSentinel sentinel(m_mutex);
		m_chunkQueue.push(chunkRef);
	}
}

void AudioTape::Write()
{
	// Get the latest audio chunks and write them to disk
	bool done = false;
	while(!done && m_state != StateStopped && m_state != StateError)
	{
		// Get the oldest audio chunk
		AudioChunkRef chunkRef;
		{
			MutexSentinel sentinel(m_mutex);
			if (m_chunkQueue.size() > 0)
			{
				chunkRef = m_chunkQueue.front();
				m_chunkQueue.pop();
			}
			else
			{
				done = true;
			}
		}
		if(!done)
		{
			try
			{
				// Need to create file appender when receiving first audio chunk
				if (m_state == StateCreated)
				{
					m_state = StateActive;

					switch(chunkRef->GetEncoding())
					{
					case PcmAudio:
						m_audioFileRef.reset(new LibSndFileFile(SF_FORMAT_PCM_16 | SF_FORMAT_WAV));
						break;
					default:					
						// All other encodings: output as a media chunk file
						m_audioFileRef.reset(new MediaChunkFile());
					}
					if (m_state == StateActive)
					{
						// A file format was successfully added to the tape, open it
						CStdString file = CONFIG.m_audioOutputPath + "/" + m_filePath + m_fileIdentifier;
						m_audioFileRef->Open(file, AudioFile::WRITE, false, chunkRef->GetSampleRate());

						// determine what final extension the file will have after optional compression
						if(CONFIG.m_storageAudioFormat == FfNative)
						{
							m_fileExtension = m_audioFileRef->GetExtension();
						}
						else
						{
							m_fileExtension = GetFileFormatExtension(CONFIG.m_storageAudioFormat);
						}
					}
				}
				if (m_state == StateActive)
				{
					m_audioFileRef->WriteChunk(chunkRef);

					if (CONFIG.m_logRms)
					{
						// Compute RMS, RMS dB and log
						CStdString rmsString;
						rmsString.Format("%.1f dB:%.1f", chunkRef.get()->ComputeRms(), chunkRef.get()->ComputeRmsDb());
						LOG4CXX_INFO(LOG.portLog, m_portId + " RMS: " + rmsString);
					}
				}
			}
			catch (CStdString& e)
			{
				LOG4CXX_INFO(LOG.portLog, "#" + m_portId + ": " + e);
				m_state = StateError;
			}
		}
	}

	if ( (m_shouldStop && m_state != StateStopped) || m_state == StateError)
	{
		m_state = StateStopped;
		if(m_audioFileRef.get())
		{
			m_audioFileRef->Close();
			m_readyForBatchProcessing = true;
		}
	}


}

void AudioTape::SetShouldStop()
{
	m_shouldStop = true;
}

void AudioTape::AddCaptureEvent(CaptureEventRef eventRef, bool send)
{
	CStdString logMsg;

	// Extract useful info from well known events
	switch(eventRef->m_type)
	{
	case CaptureEvent::EtStop:
		m_shouldStop = true;

		m_duration = time(NULL) - m_beginDate;

		{
			// Log the call details
			AudioTapeDescription atd;
			atd.m_beginDate = m_beginDate;
			atd.m_capturePort = m_portId;
			atd.m_direction = m_direction;
			atd.m_duration = m_duration;
			atd.m_localEntryPoint = m_localEntryPoint;
			atd.m_localParty = m_localParty;
			atd.m_remoteParty = m_remoteParty;
			atd.m_localIp = m_localIp;
			atd.m_remoteIp = m_remoteIp;
			CStdString description = atd.SerializeSingleLine();
			LOG4CXX_INFO(LOG.tapelistLog, description);
		}
		break;
	case CaptureEvent::EtDirection:
		m_direction = (CaptureEvent::DirectionEnum)CaptureEvent::DirectionToEnum(eventRef->m_value);
		break;
	case CaptureEvent::EtRemoteParty:
		m_remoteParty = eventRef->m_value;
		break;
	case CaptureEvent::EtLocalParty:
		m_localParty = eventRef->m_value;
		break;
	case CaptureEvent::EtLocalEntryPoint:
		m_localEntryPoint = eventRef->m_value;
		break;
	case CaptureEvent::EtLocalIp:
		m_localIp = eventRef->m_value;
		break;
	case CaptureEvent::EtRemoteIp:
		m_remoteIp = eventRef->m_value;
		break;
	}

	// Store the capture event locally
	{
		MutexSentinel sentinel(m_mutex);
		m_eventQueue.push(eventRef);
		if (send)
		{
			m_toSendEventQueue.push(eventRef);
		}
	}
}

void AudioTape::GetMessage(MessageRef& msgRef)
{
	CaptureEventRef captureEventRef;
	{
		MutexSentinel sentinel(m_mutex);
		captureEventRef = m_toSendEventQueue.front();
		m_toSendEventQueue.pop();
	}

	msgRef.reset(new TapeMsg);
	TapeMsg* pTapeMsg = (TapeMsg*)msgRef.get();
	if(captureEventRef->m_type == CaptureEvent::EtStop)
	{
		pTapeMsg->m_recId = m_fileIdentifier;
		pTapeMsg->m_fileName = m_filePath + m_fileIdentifier + m_fileExtension;
		pTapeMsg->m_stage = CaptureEvent::EventTypeToString(captureEventRef->m_type);
		pTapeMsg->m_capturePort = m_portId;
		pTapeMsg->m_localParty = m_localParty;
		pTapeMsg->m_localEntryPoint = m_localEntryPoint;
		pTapeMsg->m_remoteParty = m_remoteParty;
		pTapeMsg->m_direction = CaptureEvent::DirectionToString(m_direction);
		pTapeMsg->m_duration = m_duration;
		pTapeMsg->m_timestamp = m_beginDate;
		pTapeMsg->m_localIp = m_localIp;
		pTapeMsg->m_remoteIp = m_remoteIp;
	}
	else
	{
		// This should be a key-value pair message or a start event
	}
}

void AudioTape::GenerateFilePathAndIdentifier()
{
	struct tm date = {0};
	ACE_OS::localtime_r(&m_beginDate, &date);
	int month = date.tm_mon + 1;				// january=0, decembre=11
	int year = date.tm_year + 1900;
	m_filePath.Format("%.4d/%.2d/%.2d/%.2d/", year, month, date.tm_mday, date.tm_hour);
	m_fileIdentifier.Format("%.4d%.2d%.2d_%.2d%.2d%.2d_%s", year, month, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec, m_portId);
}


CStdString AudioTape::GetIdentifier()
{
	return m_fileIdentifier;
}

CStdString AudioTape::GetFilename()
{
	return m_filePath + m_fileIdentifier + m_fileExtension;
}


CStdString AudioTape::GetPath()
{
	return m_filePath;
}

bool AudioTape::IsStoppedAndValid()
{
	if (m_state == StateStopped)
	{
		return true;
	}
	else
	{
		return false;
	}
}

AudioFileRef AudioTape::GetAudioFileRef()
{
	return m_audioFileRef;
}

bool AudioTape::IsReadyForBatchProcessing()
{
	if(m_readyForBatchProcessing)
	{
		m_readyForBatchProcessing = false;	// toggle to ensure not processed twice
		return true;
	}
	return false;
}


//========================================
// File format related methods

int AudioTape::FileFormatToEnum(CStdString& format)
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

CStdString AudioTape::FileFormatToString(int formatEnum)
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

CStdString AudioTape::GetFileFormatExtension(FileFormatEnum formatEnum)
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



