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

#include <vector>
#include <bitset>

#include "ConfigManager.h"
#include "DirectionSelector.h"
#include "ace/OS_NS_unistd.h"
#include "audiofile/LibSndFileFile.h"
#include "Daemon.h"
#include "Filter.h"
#include "Reporting.h"
#include "Utils.h"
#include "audiofile/MediaChunkFile.h"
#ifndef WIN32
#include <pwd.h>
#include <grp.h>
#endif

TapeProcessorRef DirectionSelector::m_singleton;

void DirectionSelector::Initialize()
{
	if(m_singleton.get() == NULL)
	{
		m_singleton.reset(new DirectionSelector());
		TapeProcessorRegistry::instance()->RegisterTapeProcessor(m_singleton);


	}
}

DirectionSelector::DirectionSelector()
{
	m_threadCount = 0;
	struct tm date = {0};
	time_t now = time(NULL);
	ACE_OS::localtime_r(&now, &date);
	m_currentDay = date.tm_mday;
}

CStdString __CDECL__ DirectionSelector::GetName()
{
	return "DirectionSelector";
}

TapeProcessorRef  DirectionSelector::Instanciate()
{
	return m_singleton;
}

void DirectionSelector::AddAudioTape(AudioTapeRef& audioTapeRef)
{
	if (!m_audioTapeQueue.push(audioTapeRef))
	{
		// Log error
		LOG4CXX_ERROR(LOG.directionSelectorLog, CStdString("queue full"));
	}
}

void DirectionSelector::SetQueueSize(int size)
{
	m_audioTapeQueue.setSize(size);
}

void DirectionSelector::ProcessAreaCodesMap(char *line, int ln)
{
	char *areaCode = NULL;
	char *side = NULL;
	CStdString logMsg;

	areaCode = line;
	side = strchr(line, ':');

	if(!side || !areaCode)
	{

		logMsg.Format("ProcessAreaCodesMap: invalid format of line:%d in the area codes maps file", ln);
		LOG4CXX_WARN(LOG.directionSelectorLog, logMsg);
		return;
	}

	*(side++) = '\0';

	CStdString code, sideKept;
	code = areaCode;
	sideKept = side;

	code.Trim();
	sideKept.Trim();

	if((!StringIsPhoneNumber(code)) || (CaptureEvent::AudioKeepDirectionEnum)CaptureEvent::AudioKeepDirectionToEnum(sideKept) == CaptureEvent::AudioKeepDirectionInvalid)
	{
		logMsg.Format("ProcessAreaCodesMap: invalid format of line:%d in the area codes maps file", ln);
		LOG4CXX_WARN(LOG.directionSelectorLog, logMsg);
		return;
	}

	m_areaCodesMap.insert(std::make_pair(code, side));
	LOG4CXX_DEBUG(LOG.directionSelectorLog,  "AreaCodesMap codes:" + code + " side:" + sideKept);
}

void DirectionSelector::LoadAreaCodesMap()
{
	FILE *maps = NULL;
	char buf[1024];
	int i = 0;
	int ln = 0;
	CStdString logMsg;

	memset(buf, 0, sizeof(buf));
	maps = fopen(LOCAL_AREA_CODES_MAP_FILE, "r");
	if(!maps)
	{
		logMsg.Format("LoadAreaCodesMaps: Could not open file:%s -- trying:%s now", LOCAL_AREA_CODES_MAP_FILE, ETC_LOCAL_AREA_CODES_MAP_FILE);
		LOG4CXX_INFO(LOG.directionSelectorLog, logMsg);

		maps = fopen(ETC_LOCAL_AREA_CODES_MAP_FILE, "r");
		if(!maps)
		{
			logMsg.Format("LoadAreaCodesMaps: Could not open file:%s either -- giving up", ETC_LOCAL_AREA_CODES_MAP_FILE);
			LOG4CXX_INFO(LOG.directionSelectorLog, logMsg);
			return;
		}
	}

	while(fgets(buf, sizeof(buf), maps))
	{
		ln += 1;
		// Minimum line of x,y\n
		if(strlen(buf) > 4)
		{
			if(buf[strlen(buf)-1] == '\n')
			{
				buf[strlen(buf)-1] = '\0';
			}

			ProcessAreaCodesMap(buf, ln);
		}
	}

	fclose(maps);
	return;
}

void DirectionSelector::ThreadHandler(void *args)
{
	CStdString debug;
	CStdString logMsg;

	CStdString processorName("DirectionSelector");
	TapeProcessorRef directionSelector = TapeProcessorRegistry::instance()->GetNewTapeProcessor(processorName);
	if(directionSelector.get() == NULL)
	{
		LOG4CXX_ERROR(LOG.directionSelectorLog, "Could not instanciate DirectionSelector");
		return;
	}
	DirectionSelector* pDirectionSelector = (DirectionSelector*)(directionSelector->Instanciate().get());

	pDirectionSelector->SetQueueSize(CONFIG.m_directionSelectorQueueSize);

	int threadId = 0;
	{
		MutexSentinel sentinel(pDirectionSelector->m_mutex);
		threadId = pDirectionSelector->m_threadCount++;
	}
	CStdString threadIdString = IntToString(threadId);
	debug.Format("thread Th%s starting - queue size:%d", threadIdString, CONFIG.m_directionSelectorQueueSize);
	LOG4CXX_INFO(LOG.directionSelectorLog, debug);

	pDirectionSelector->LoadAreaCodesMap();

	bool stop = false;

	for(;stop == false;)
	{
		AudioFileRef fileRef;
		oreka::shared_ptr<MediaChunkFile> outFile (new MediaChunkFile());
		AudioTapeRef audioTapeRef;
		CStdString trackingId = "[no-trk]";
		int numSamplesOutv = 0;

		AudioChunkRef chunkRef;

		try
		{
			audioTapeRef = pDirectionSelector->m_audioTapeQueue.pop();
			if(audioTapeRef.get() == NULL)
			{
				if(Daemon::Singleton()->IsStopping())
				{
					stop = true;
				}
				if(Daemon::Singleton()->GetShortLived())
				{
					Daemon::Singleton()->Stop();
				}
			}
			else
			{
			//Iterating through area codes map to check which side will be retain
				bool found = false;
				int foundPos = -1;
				CStdString side;
				std::map<CStdString, CStdString>::iterator it;
				for(it = pDirectionSelector->m_areaCodesMap.begin(); it!= pDirectionSelector->m_areaCodesMap.end() && found == false; it++)
				{
					//For now, we dont consider local party which has nothing to do with area codes
//					foundPos = audioTapeRef->m_localParty.find(it->first);
//					if(foundPos == 0)
//					{
//						side = it->second;
//						found = true;
//						break;
//					}

					foundPos = audioTapeRef->m_remoteParty.find(it->first);
					if(foundPos == 0)
					{
						side = it->second;
						found = true;
						break;
					}
				}

				if(found == true)
				{
					AudioDirectionMarksRef defaultKeptSide(new AudioDirectionMarks());
					defaultKeptSide->m_timestamp = 1;		//make sure it the first event in the chain of event in term of timestamp
					if(audioTapeRef->m_audioDirectionMarks.size() > 0)
					{
						std::vector<AudioDirectionMarksRef>::iterator it;
						it = audioTapeRef->m_audioDirectionMarks.begin();
						defaultKeptSide->m_nextTimestampMark = (*it)->m_timestamp;	//next mark, will be the first api called, if any
					}

					defaultKeptSide->m_audioKeepDirectionEnum = (CaptureEvent::AudioKeepDirectionEnum)CaptureEvent::AudioKeepDirectionToEnum(side);
					audioTapeRef->m_audioDirectionMarks.insert(audioTapeRef->m_audioDirectionMarks.begin(),defaultKeptSide);

				}

				CStdString mcfExt, tmpExt, tmpFileName, origFileName, origFileNameWoExt;
				mcfExt = ".mcf";
				tmpExt = ".tmp";

				audioTapeRef->SetExtension(mcfExt); 		//the real extension at this point
				origFileName = CONFIG.m_audioOutputPath + "/"+ audioTapeRef->GetFilename();
				origFileNameWoExt = CONFIG.m_audioOutputPath + "/" + audioTapeRef->GetPath() + audioTapeRef->GetIdentifier();
				//copy a temporary file for processing
				audioTapeRef->SetExtension(tmpExt);
				tmpFileName = CONFIG.m_audioOutputPath + "/"+ audioTapeRef->GetFilename();

				if(ACE_OS::rename((PCSTR)origFileName, (PCSTR)tmpFileName) != 0){
					LOG4CXX_ERROR(LOG.directionSelectorLog, "Can not rename audio file for processing");
				}

				fileRef = audioTapeRef->GetAudioFileRef();
				trackingId = audioTapeRef->m_trackingId;
				fileRef->SetFilename(tmpFileName);			//audioTapeRef->SetExtension(fullfilename) does not take affect on audiofileRef,

				fileRef->Open(AudioFile::READ);

				outFile->Open(origFileNameWoExt, AudioFile::WRITE);
				while(fileRef->ReadChunkMono(chunkRef))
				{
					AudioChunkDetails details = *chunkRef->GetDetails();

					std::vector<AudioDirectionMarksRef>::iterator it;
					for(it = audioTapeRef->m_audioDirectionMarks.begin(); it != audioTapeRef->m_audioDirectionMarks.end(); it++)
					{
						if(((*it)->m_timestamp == 0))
						{
							continue;
						}

						if((details.m_arrivalTimestamp >= (*it)->m_timestamp) && ((details.m_arrivalTimestamp < (*it)->m_nextTimestampMark) || ((*it)->m_nextTimestampMark == 0)))	//this audio chunk is in between 2 kept-direction reports marks
						{
							if(audioTapeRef->m_localSide == CaptureEvent::LocalSideSide1)
							{
								if(((*it)->m_audioKeepDirectionEnum == CaptureEvent::AudioKeepDirectionLocal) && (details.m_channel == 2))
								{
									memset(chunkRef->m_pBuffer, 0, details.m_numBytes);		//blank side 2
								}
								else if(((*it)->m_audioKeepDirectionEnum == CaptureEvent::AudioKeepDirectionRemote) && (details.m_channel == 1))
								{
									memset(chunkRef->m_pBuffer, 0, details.m_numBytes);		//blank side 1
								}
							}
							else if(audioTapeRef->m_localSide == CaptureEvent::LocalSideSide2)
							{
								if(((*it)->m_audioKeepDirectionEnum == CaptureEvent::AudioKeepDirectionLocal) && (details.m_channel == 1))
								{
									memset(chunkRef->m_pBuffer, 0, details.m_numBytes);
								}
								else if(((*it)->m_audioKeepDirectionEnum == CaptureEvent::AudioKeepDirectionRemote) && (details.m_channel == 2))
								{
									memset(chunkRef->m_pBuffer, 0, details.m_numBytes);
								}

							}

						}
					}

					outFile->WriteChunk(chunkRef);
				}
				outFile->Close();

				ACE_OS::unlink((PCSTR)tmpFileName);
				fileRef->Close();

				audioTapeRef->SetExtension(mcfExt);		//return back to mcf ext

				fileRef->SetFilename(origFileName);		// weird here, but it needs to be done, otherwise audioTapeRef will associate with tmp File
				pDirectionSelector->RunNextProcessor(audioTapeRef);
			}
		}

		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.directionSelectorLog, "[" + trackingId + "] Th" + threadIdString + " " + e);
			if(fileRef.get()) {fileRef->Close();}
		}
	}
	LOG4CXX_INFO(LOG.directionSelectorLog, CStdString("Exiting thread Th" + threadIdString));
}


