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

	GenerateCaptureFilePathAndIdentifier();
}

AudioTape::AudioTape(CStdString &portId, CStdString& file)
{
	m_portId = portId;

	// Extract Path and Identifier
	m_filePath = FilePath(file);
	if(m_filePath.Find(CONFIG.m_audioOutputPath) >= 0) {
		m_filePath = (m_filePath.Right(m_filePath.size() - 1 - CONFIG.m_audioOutputPath.size()));
	}

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

						// Prevent identifier collision
						CStdString path = CONFIG.m_audioOutputPath + "/" + m_filePath;
						CStdString extension = m_audioFileRef->GetExtension();
						PreventFileIdentifierCollision(path, m_fileIdentifier , extension);

						// Open the capture file
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
			/*
			 * This function is now called in the TapeFileNaming
			 * tape processor.
			 */
			//GenerateFinalFilePathAndIdentifier();
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
			GenerateCaptureFilePathAndIdentifier();
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
	case CaptureEvent::EtCallId:
		m_nativeCallId = eventRef->m_value;
		break;
	}

	{
		MutexSentinel sentinel(m_mutex);
		// Store the capture event locally
		m_eventQueue.push(eventRef);
		if (send)
		{
			m_toSendEventQueue.push(eventRef);
		}
		// Store the tags
		if(eventRef->m_type == CaptureEvent::EtKeyValue && eventRef->m_value.size() > 0 && eventRef->m_key.size() > 0)
		{
			m_tags.insert(std::make_pair(eventRef->m_key, eventRef->m_value));
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
	if(captureEventRef->m_type == CaptureEvent::EtStop || captureEventRef->m_type == CaptureEvent::EtStart || captureEventRef->m_type == CaptureEvent::EtReady || captureEventRef->m_type == CaptureEvent::EtUpdate)
	{
		PopulateTapeMessage(pTapeMsg, captureEventRef->m_type);
	}
	else
	{
		// This should be a key-value pair message
	}
}

void AudioTape::GetDetails(TapeMsg* msg)
{
	PopulateTapeMessage(msg, CaptureEvent::EtStop);
}


void AudioTape::PopulateTapeMessage(TapeMsg* msg, CaptureEvent::EventTypeEnum eventType)
{
	msg->m_recId = m_orkUid;
	msg->m_fileName = m_filePath + m_fileIdentifier + m_fileExtension;
	msg->m_stage = CaptureEvent::EventTypeToString(eventType);
	msg->m_capturePort = m_portId;
	msg->m_localParty = m_localParty;
	msg->m_localEntryPoint = m_localEntryPoint;
	msg->m_remoteParty = m_remoteParty;
	msg->m_direction = CaptureEvent::DirectionToString(m_direction);
	msg->m_duration = m_duration;
	msg->m_timestamp = m_beginDate;
	msg->m_localIp = m_localIp;
	msg->m_remoteIp = m_remoteIp;

	MutexSentinel sentinel(m_mutex);;
	std::copy(m_tags.begin(), m_tags.end(), std::inserter(msg->m_tags, msg->m_tags.begin()));
}

void AudioTape::GenerateCaptureFilePathAndIdentifier()
{
	struct tm date = {0};

	ACE_OS::localtime_r(&m_beginDate, &date);
	int month = date.tm_mon + 1;				// january=0, decembre=11
	int year = date.tm_year + 1900;

	m_filePath.Format("%.4d/%.2d/%.2d/%.2d/", year, month, date.tm_mday, date.tm_hour);

	m_fileIdentifier.Format("%.4d%.2d%.2d_%.2d%.2d%.2d_%s", year, month, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec, m_portId);

	m_year.Format("%.4d", year);
	m_day.Format("%.2d", date.tm_mday);
	m_month.Format("%.2d", month);
	m_hour.Format("%.2d", date.tm_hour);
	m_min.Format("%.2d", date.tm_min);
	m_sec.Format("%.2d", date.tm_sec);
}

void AudioTape::GenerateFinalFilePath()
{
	if(CONFIG.m_tapePathNaming.size() > 0)
	{
		CStdString pathIdentifier;
		std::list<CStdString>::iterator it;

		for(it = CONFIG.m_tapePathNaming.begin(); it != CONFIG.m_tapePathNaming.end(); it++)
                {
			CStdString element = *it;
			int tapeAttributeEnum = TapeAttributes::TapeAttributeToEnum(element);

			switch(tapeAttributeEnum) {
                        case TapeAttributes::TaNativeCallId:
                        {
                                if(m_nativeCallId.size() > 0)
                                {
                                        pathIdentifier += m_nativeCallId;
                                }
                                else
                                {
                                        pathIdentifier += "nonativecallid";
                                }
                                break;
                        }
                        case TapeAttributes::TaTrackingId:
                        {
                                if(m_trackingId.size() > 0)
                                {
                                        pathIdentifier += m_trackingId;
                                }
                                else
                                {
                                        pathIdentifier += "notrackingid";
                                }
                                break;
                        }
                        case TapeAttributes::TaDirection:
                        {
                                pathIdentifier += CaptureEvent::DirectionToString(m_direction);
                                break;
                        }
                        case TapeAttributes::TaShortDirection:
                        {
                                pathIdentifier += CaptureEvent::DirectionToShortString(m_direction);
                                break;
                        }
                        case TapeAttributes::TaRemoteParty:
                        {
                                if(m_remoteParty.size() > 0)
                                {
                                        pathIdentifier += m_remoteParty;
                                }
                                else
                                {
                                        pathIdentifier += "noremoteparty";
                                }
                                break;
                        }
                        case TapeAttributes::TaLocalParty:
                        {
                                if(m_localParty.size() > 0)
                                {
                                        pathIdentifier += m_localParty;
                                }
                                else
                                {
                                        pathIdentifier += "nolocalparty";
                                }
                                break;
                        }
                        case TapeAttributes::TaLocalEntryPoint:
                        {
                                if(m_localEntryPoint.size() > 0)
                                {
                                        pathIdentifier += m_localEntryPoint;
                                }
                                else
                                {
                                        pathIdentifier += "nolocalentrypoint";
                                }
                                break;
                        }
                        case TapeAttributes::TaLocalIp:
                        {
                                if(m_localIp.size() > 0)
                                {
                                        pathIdentifier += m_localIp;
                                }
                                else
                                {
                                        pathIdentifier += "nolocalip";
                                }
                                break;
                        }
                        case TapeAttributes::TaRemoteIp:
                        {
                                if(m_remoteIp.size() > 0)
                                {
                                        pathIdentifier += m_remoteIp;
                                }
                                else
                                {
                                        pathIdentifier += "noremoteip";
                                }
                                break;
                        }
                        case TapeAttributes::TaHostname:
                        {
                                char host_name[255];

                                memset(host_name, 0, sizeof(host_name));
                                ACE_OS::hostname(host_name, sizeof(host_name));

                                if(strlen(host_name))
                                {
                                        pathIdentifier += host_name;
                                }
                                else
                                {
                                        pathIdentifier += "nohostname";
                                }

                                break;
                        }
                        case TapeAttributes::TaYear:
                        {
                                pathIdentifier += m_year;
                                break;
                        }
                        case TapeAttributes::TaDay:
                        {
                                pathIdentifier += m_day;
                                break;
                        }
                        case TapeAttributes::TaMonth:
                        {
                                pathIdentifier += m_month;
                                break;
                        }
                        case TapeAttributes::TaHour:
                        {
                                pathIdentifier += m_hour;
                                break;
                        }
                        case TapeAttributes::TaMin:
                        {
                                pathIdentifier += m_min;
                                break;
                        }
                        case TapeAttributes::TaSec:
                        {
                                pathIdentifier += m_sec;
                                break;
                        }
                        case TapeAttributes::TaUnknown:
                        {
				std::map<CStdString, CStdString>::iterator pair;
				CStdString correctKey, mTagsValue;

				// Remove the []
				correctKey = element.substr(1, element.size()-2);
				pair = m_tags.find(correctKey);

				if(pair != m_tags.end())
				{
					mTagsValue = pair->second;
					pathIdentifier += mTagsValue;
				}
				else
				{
					pathIdentifier += element;
				}

				break;
			}
			}
		}

        if(pathIdentifier.size() > 0)
        {
		m_filePath = pathIdentifier;

		CStdString mkdirPath;

		mkdirPath.Format("%s/%s", CONFIG.m_audioOutputPath, m_filePath);
		FileRecursiveMkdir(mkdirPath, CONFIG.m_audioFilePermissions, CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup, CONFIG.m_audioOutputPath);
        }
	}
}

void AudioTape::GenerateFinalFilePathAndIdentifier()
{
	if(CONFIG.m_tapeFileNaming.size() > 0)
	{
		// The config file specifies a naming scheme for the recordings, build the final file identifier
		CStdString fileIdentifier;
		std::list<CStdString>::iterator it;

		for(it = CONFIG.m_tapeFileNaming.begin(); it != CONFIG.m_tapeFileNaming.end(); it++)
		{
			CStdString element = *it;
			int tapeAttributeEnum = TapeAttributes::TapeAttributeToEnum(element);

			switch(tapeAttributeEnum) {
			case TapeAttributes::TaNativeCallId:
			{
				if(m_nativeCallId.size() > 0)
                                {
                                        fileIdentifier += m_nativeCallId;
                                }
                                else
                                {
                                        fileIdentifier += "nonativecallid";
                                }
				break;
			}
			case TapeAttributes::TaTrackingId:
			{
				if(m_trackingId.size() > 0)
				{
					fileIdentifier += m_trackingId;
				}
				else
				{
					fileIdentifier += "notrackingid";
				}
				break;
			}
			case TapeAttributes::TaDirection:
                        {
				fileIdentifier += CaptureEvent::DirectionToString(m_direction);
				break;
			}
			case TapeAttributes::TaShortDirection:
			{
				fileIdentifier += CaptureEvent::DirectionToShortString(m_direction);
				break;
			}
			case TapeAttributes::TaRemoteParty:
			{
				if(m_remoteParty.size() > 0)
				{
					fileIdentifier += m_remoteParty;
				}
				else
				{
					fileIdentifier += "noremoteparty";
				}
				break;
			}
			case TapeAttributes::TaLocalParty:
			{
				if(m_localParty.size() > 0)
                                {
                                        fileIdentifier += m_localParty;
                                }
                                else
                                {
                                        fileIdentifier += "nolocalparty";
                                }
                                break;
                        }
                        case TapeAttributes::TaLocalEntryPoint:
                        {
                                if(m_localEntryPoint.size() > 0)
                                {
                                        fileIdentifier += m_localEntryPoint;
                                }
                                else
                                {
                                        fileIdentifier += "nolocalentrypoint";
                                }
                                break;
                        }
                        case TapeAttributes::TaLocalIp:
                        {
                                if(m_localIp.size() > 0)
                                {
                                        fileIdentifier += m_localIp;
                                }
                                else
                                {
                                        fileIdentifier += "nolocalip";
                                }
                                break;
                        }
                        case TapeAttributes::TaRemoteIp:
                        {
                                if(m_remoteIp.size() > 0)
                                {
                                        fileIdentifier += m_remoteIp;
                                }
                                else
                                {
                                        fileIdentifier += "noremoteip";
                                }
                                break;
                        }
                        case TapeAttributes::TaHostname:
			{
				char host_name[255];

				memset(host_name, 0, sizeof(host_name));
				ACE_OS::hostname(host_name, sizeof(host_name));

				if(strlen(host_name))
				{
					fileIdentifier += host_name;
				}
				else
				{
					fileIdentifier += "nohostname";
				}

				break;
			}
			case TapeAttributes::TaYear:
			{
				fileIdentifier += m_year;
				break;
			}
			case TapeAttributes::TaDay:
                        {
                                fileIdentifier += m_day;
				break;
			}
			case TapeAttributes::TaMonth:
                        {
                                fileIdentifier += m_month;
                                break;
                        }
                        case TapeAttributes::TaHour:
                        {
                                fileIdentifier += m_hour;
				break;
                        }
                        case TapeAttributes::TaMin:
                        {
                                fileIdentifier += m_min;
                                break;
                        }
                        case TapeAttributes::TaSec:
                        {
                                fileIdentifier += m_sec;
                                break;
                        }
			case TapeAttributes::TaUnknown:
			{
				std::map<CStdString, CStdString>::iterator pair;
				CStdString correctKey, mTagsValue;

				// Remove the []
				correctKey = element.substr(1, element.size()-2);
				pair = m_tags.find(correctKey);

				if(pair != m_tags.end())
				{
					mTagsValue = pair->second;
					fileIdentifier += mTagsValue;
				}
				else
				{
					fileIdentifier += element;
				}

				break;
			}
			}
		}

		if(fileIdentifier.size() > 0)
		{
			m_fileIdentifier = fileIdentifier;
		}

		GenerateFinalFilePath();

		CStdString path = CONFIG.m_audioOutputPath + "/" + m_filePath + "/";
		PreventFileIdentifierCollision(path, m_fileIdentifier , m_fileExtension);
	}
}

void AudioTape::PreventFileIdentifierCollision(CStdString& path, CStdString& identifier, CStdString& extension)
{
	int fileIndex = 0;
	CStdString identifierWithIndex;
	CStdString file = path + identifier + extension;
	while(FileCanOpen(file) == true)
	{
		fileIndex++;
		identifierWithIndex.Format("%s-%d", identifier, fileIndex);
		file = path + identifierWithIndex + extension;
	}
	if(fileIndex)
	{
		//identifier = identifierWithIndex;
		identifier += "-" + m_orkUid;
	}
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

//=================================

CStdString TapeAttributes::TapeAttributeToString(int ta)
{
	switch(ta) {
	case TaNativeCallId:
		return TA_NATIVECALLID;
	case TaTrackingId:
		return TA_TRACKINGID;
	case TaDirection:
		return TA_DIRECTION;
	case TaShortDirection:
		return TA_SHORTDIRECTION;
	case TaRemoteParty:
		return TA_REMOTEPARTY;
	case TaLocalParty:
		return TA_LOCALPARTY;
	case TaLocalEntryPoint:
		return TA_LOCALENTRYPOINT;
	case TaLocalIp:
		return TA_LOCALIP;
	case TaRemoteIp:
		return TA_REMOTEIP;
	case TaHostname:
		return TA_HOSTNAME;
	case TaYear:
		return TA_YEAR;
	case TaDay:
                return TA_DAY;
        case TaMonth:
                return TA_MONTH;
        case TaHour:
                return TA_HOUR;
        case TaMin:
                return TA_MIN;
        case TaSec:
                return TA_SEC;
	}

	return "[UnknownAttribute]";
}

int TapeAttributes::TapeAttributeToEnum(CStdString& ta)
{
	if(ta.CompareNoCase(TA_NATIVECALLID) == 0)
        {
		return TaNativeCallId;
	}

	if(ta.CompareNoCase(TA_TRACKINGID) == 0)
        {
                return TaTrackingId;
        }

        if(ta.CompareNoCase(TA_DIRECTION) == 0)
        {
                return TaDirection;
        }

        if(ta.CompareNoCase(TA_SHORTDIRECTION) == 0)
        {
                return TaShortDirection;
        }

        if(ta.CompareNoCase(TA_REMOTEPARTY) == 0)
        {
                return TaRemoteParty;
        }

        if(ta.CompareNoCase(TA_LOCALPARTY) == 0)
        {
                return TaLocalParty;
        }

        if(ta.CompareNoCase(TA_LOCALENTRYPOINT) == 0)
        {
                return TaLocalEntryPoint;
        }

        if(ta.CompareNoCase(TA_LOCALIP) == 0)
        {
                return TaLocalIp;
        }

        if(ta.CompareNoCase(TA_REMOTEIP) == 0)
        {
                return TaRemoteIp;
        }

        if(ta.CompareNoCase(TA_HOSTNAME) == 0)
        {
                return TaHostname;
        }

	if(ta.CompareNoCase(TA_YEAR) == 0)
        {
                return TaYear;
        }

        if(ta.CompareNoCase(TA_DAY) == 0)
        {
                return TaDay;
        }

        if(ta.CompareNoCase(TA_MONTH) == 0)
        {
                return TaMonth;
        }

        if(ta.CompareNoCase(TA_HOUR) == 0)
        {
                return TaHour;
        }

        if(ta.CompareNoCase(TA_MIN) == 0)
        {
		return TaMin;
        }

        if(ta.CompareNoCase(TA_SEC) == 0)
        {
                return TaSec;
        }

	return TaUnknown;
}
