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
#include "Utils.h"

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
				
				//All the file name below include full absolute path
				CStdString audioFileName;
				if(!audioTapeRef->m_isExternal)
				{
					audioFileName = CONFIG.m_audioOutputPath + "/" + audioTapeRef->GetPath() + audioTapeRef->GetIdentifier() + ".wav" ;		//as usual
				}
				else
				{
					audioFileName = CONFIG.m_audioOutputPath + "/" + audioTapeRef->GetPath() + audioTapeRef->m_externalFileName;	//conventional orkaudio name + orginal name (include extension)
				}

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
				CStdString tmpName = audioFileName + ".tmp";
				if(ACE_OS::rename((PCSTR)audioFileName, (PCSTR)tmpName) != 0)
				{
					logMsg.Format("Can not rename the audio file name for CommandProcessing");
					audioTapeRef->m_isSuccessfulImported = false;
					audioTapeRef->m_isDoneProcessed = true;		//The processed in this tape is done, even thought it failed
					LOG4CXX_ERROR(LOG.batchProcessingLog, logMsg);
					continue;
				}
				CStdString inArg = "[IN]";
				CStdString outArg = "[OUT]";
				CStdString tmp1, tmp2;
				int pos_arg = 0;
				if(pos_arg = command.Find(inArg) == -1)
				{
					system(command);
				}
				else
				{
					if((pos_arg = command.Find(inArg)) != -1)
					{
						tmp1 = command.substr(0, pos_arg);

						if(pos_arg < (command.length() - inArg.length()))
						{
							tmp2 = command.substr(pos_arg + inArg.length());
						}
						else
						{
							tmp2 = "";
						}
						command = tmp1 + " "+ tmpName + " " + tmp2;

					}
					if((pos_arg = command.Find(outArg)) != -1)
					{
						tmp1 = command.substr(0, pos_arg);
						if(pos_arg < (command.length() - outArg.length()))
						{
							tmp2 = command.substr(pos_arg + outArg.length());
						}
						else
						{
							tmp2 = "";
						}
						command = tmp1 + " "+ audioFileName + " " + tmp2;
						LOG4CXX_INFO(LOG.batchProcessingLog, command);
					}
					system(command);
					ACE_OS::unlink((PCSTR)tmpName);

					CStdString fileNameWoExt, newFileName;
					int lastDotPos;
					lastDotPos = audioFileName.ReverseFind('.');
					if(lastDotPos != -1);
					{
						fileNameWoExt = audioFileName.substr(0, lastDotPos +1);			//strip the file name extension in order to replace by ext gsm.wav
					}

					newFileName = fileNameWoExt + "gsm.wav";		//This is final name of audiofile = convetional orkaudio name + original name + .gsm.wav ext
					if(ACE_OS::rename((PCSTR)audioFileName, (PCSTR)newFileName) != 0)
					{
						audioTapeRef->m_isSuccessfulImported = false;
						audioTapeRef->m_isDoneProcessed = true;
						logMsg.Format("Can not rename the audio file name for CommandProcessing");
						LOG4CXX_ERROR(LOG.batchProcessingLog, logMsg);
						continue;
					}

					//Verify the output of transcoding and proceed to next processing chain
					if(FileSizeInKb(newFileName) < 1)
					{
						logMsg.Format("The output file %s is is too small", newFileName);
						audioTapeRef->m_isSuccessfulImported = false;
						audioTapeRef->m_isDoneProcessed = true;
						LOG4CXX_ERROR(LOG.batchProcessingLog, logMsg);
						continue;
					}
					//Not really extension, just make it easier to append to the end of file name
					CStdString finalExt = fileNameWoExt + "gsm.wav";
					audioTapeRef->SetExtension(finalExt);
				}

#endif
				//Here we can set the CommandProcessing/Transcoding to DONE status
				audioTapeRef->m_isDoneProcessed = true;
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
