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
#pragma warning( disable: 4786 )

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h


#include "ace/OS_NS_time.h"
#include "Utils.h"
#include "ThreadSafeQueue.h"
#include "LogManager.h"
#include "audiofile/PcmFile.h"
#include "audiofile/LibSndFileFile.h"
#include "audiofile/MediaChunkFile.h"
#include "messages/TapeMsg.h"
#include "AudioTape.h"
#include "ConfigManager.h"



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
	m_trackingId = portId;	// to make sure this has a value before we get the capture tracking Id.

	GenerateFilePathAndIdentifier();
}

AudioTape::AudioTape(CStdString &portId, CStdString& file)
{
	m_portId = portId;

	// Extract Path and Identifier
	m_filePath = FilePath(file);
	CStdString basename = FileBaseName(file);
	m_fileIdentifier = FileStripExtension(basename);

	// Create the audiofile
	m_audioFileRef.reset(new MediaChunkFile());
	m_audioFileRef->SetFilename(file);
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
							m_fileExtension = FileFormatGetExtension(CONFIG.m_storageAudioFormat);
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
						LOG4CXX_INFO(LOG.portLog, "[" + m_trackingId + "] RMS: " + rmsString);
					}
				}
			}
			catch (CStdString& e)
			{
				LOG4CXX_INFO(LOG.portLog, "[" + m_trackingId + "] " + e);
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
	case CaptureEvent::EtStart:
		m_trackingId = eventRef->m_value;
		if (m_state == StateCreated)
		{
			// Media chunk stream not yet started, we can update begin date with the actual capture begin date
			m_beginDate = eventRef->m_timestamp;
			GenerateFilePathAndIdentifier();
		}
		break;
	case CaptureEvent::EtStop:
		m_shouldStop = true;

		m_duration = eventRef->m_timestamp - m_beginDate;

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
	case CaptureEvent::EtOrkUid:
		m_orkUid = eventRef->m_value;
		if (m_state == StateCreated)
		{
			// Media chunk stream not yet started, we can set the mcf file name to be the Oreka Unique ID
			m_fileIdentifier = m_orkUid;
		}
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
		if(m_toSendEventQueue.size() > 0)
		{
			captureEventRef = m_toSendEventQueue.front();
			m_toSendEventQueue.pop();
		}
	}

	msgRef.reset(new TapeMsg);
	TapeMsg* pTapeMsg = (TapeMsg*)msgRef.get();
	if(captureEventRef->m_type == CaptureEvent::EtStop || captureEventRef->m_type == CaptureEvent::EtStart)
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
		// This should be a key-value pair message
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

CStdString AudioTape::GetExtension()
{
	return m_fileExtension;
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


