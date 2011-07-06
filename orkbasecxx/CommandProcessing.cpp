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
#include "CommandProcessing.h"
#include "ace/OS_NS_unistd.h"
#include "audiofile/LibSndFileFile.h"
#include "Daemon.h"
#include "Filter.h"
#include "Reporting.h"

#ifndef WIN32
#include <pwd.h>
#include <grp.h>
#endif

CStdString processorName("CommandProcessing");

TapeProcessorRef CommandProcessing::m_singleton;

void CommandProcessing::Initialize()
{
	if(m_singleton.get() == NULL)
	{
		m_singleton.reset(new CommandProcessing());
		TapeProcessorRegistry::instance()->RegisterTapeProcessor(m_singleton);
	}
}


CommandProcessing::CommandProcessing()
{
	m_threadCount = 0;

	struct tm date = {0};
	time_t now = time(NULL);
	ACE_OS::localtime_r(&now, &date);
}

CStdString __CDECL__ CommandProcessing::GetName()
{
	return processorName;
}

TapeProcessorRef  CommandProcessing::Instanciate()
{
	return m_singleton;
}

void CommandProcessing::AddAudioTape(AudioTapeRef& audioTapeRef)
{
	if (!m_audioTapeQueue.push(audioTapeRef))
	{
		// Log error
		LOG4CXX_ERROR(LOG.batchProcessingLog, CStdString("Command Processing queue full"));
	}
}

void CommandProcessing::SetQueueSize(int size)
{
	m_audioTapeQueue.setSize(size);
}

#define QUEUE_SIZE 10000

void CommandProcessing::ThreadHandler(void *args)
{
	CStdString debug;
	CStdString logMsg;

	
	TapeProcessorRef commandProcessing = TapeProcessorRegistry::instance()->GetNewTapeProcessor(processorName);
	if(commandProcessing.get() == NULL)
	{
		LOG4CXX_ERROR(LOG.batchProcessingLog, "Could not instanciate CommandProcessing");
		return;
	}
	CommandProcessing* pCommandProcessing = (CommandProcessing*)(commandProcessing->Instanciate().get());

	pCommandProcessing->SetQueueSize(QUEUE_SIZE);

	int threadId = 0;
	{
		MutexSentinel sentinel(pCommandProcessing->m_mutex);
		threadId = pCommandProcessing->m_threadCount++;
	}
	CStdString threadIdString = IntToString(threadId);
	debug.Format("Command Processing thread Th%s starting - queue size:%d", threadIdString, QUEUE_SIZE);
	LOG4CXX_INFO(LOG.batchProcessingLog, debug);

	bool stop = false;

	for(;stop == false;)
	{
		AudioFileRef fileRef;
		AudioTapeRef audioTapeRef;
		CStdString trackingId = "[no-trk]";

		try
		{
			audioTapeRef = pCommandProcessing->m_audioTapeQueue.pop();
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
				CStdString command = CONFIG.m_commandProcessingCommand;
				trackingId = audioTapeRef->m_trackingId;

				if(command.length() == 0)
				{
					LOG4CXX_ERROR(LOG.batchProcessingLog, "Command Processing Warning : Empty command, please check config.xml. Passing on to next Tape Processor");
					pCommandProcessing->RunNextProcessor(audioTapeRef);
					continue;
				}
				
				CStdString audioFileName = CONFIG.m_audioOutputPath + "/" + audioTapeRef->GetPath() + audioTapeRef->GetIdentifier() + ".wav" ;

#ifdef WIN32
				audioFileName.Replace("/","\\");
#endif
				LOG4CXX_DEBUG(LOG.batchProcessingLog, "Command Processing [" + trackingId + "] Th" + IntToString(threadId) + " : Executing command \"" + command + " " + audioFileName + "\"" );
#ifdef WIN32
				if( (int)_spawnl(_P_WAIT, command ,command + " " + audioFileName, NULL) < 0)
				{
					LOG4CXX_ERROR(LOG.tapeLog,"Command Processing [" + trackingId + "] Th" + IntToString(threadId) + " Error Executing command \"" + command + " " + audioFileName + "\"" );
				}
#else
				system(command + " " + audioFileName);
#endif

				LOG4CXX_DEBUG(LOG.batchProcessingLog, "Command Processing [" + trackingId + "] Th" + IntToString(threadId) + " : Command executed successfully, passing on to next tape processor" );
				pCommandProcessing->RunNextProcessor(audioTapeRef);	
			}
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.batchProcessingLog, "Command Processing Exception [" + trackingId + "] Th" + IntToString(threadId));
		}
	}
	LOG4CXX_INFO(LOG.batchProcessingLog, CStdString("Exiting Command Processing thread Th" + threadIdString));
}
