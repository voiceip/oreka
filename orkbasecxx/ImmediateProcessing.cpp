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

#include "ImmediateProcessing.h"
#include "LogManager.h"
#include "BatchProcessing.h"
#include "Daemon.h"
#include "ConfigManager.h"
#include "TapeProcessor.h"

ImmediateProcessing::ImmediateProcessing()
{
	m_lastQueueFullTime = time(NULL);
	m_semaphore.acquire();
}

void ImmediateProcessing::AddAudioTape(AudioTapeRef audioTapeRef)
{
	Push(audioTapeRef);
}

AudioTapeRef ImmediateProcessing::Pop(CStdString& after)
{
	m_semaphore.acquire();
	MutexSentinel mutexSentinel(m_mutex);

	AudioTapeRef audioTapeRef;

	if(m_audioTapeQueue.size() > 0)
	{
		audioTapeRef = m_audioTapeQueue.front();
		m_audioTapeQueue.pop_front();
		if (audioTapeRef.get() != NULL)
		{
			audioTapeRef->m_isQueued = false;
		}
		else
		{
			LOG4CXX_WARN(LOG.immediateProcessingLog, "Popped NULL tapeRef");
		}
	}

	return audioTapeRef;
}

void ImmediateProcessing::Push(AudioTapeRef& audioTapeRef)
{
	MutexSentinel mutexSentinel(m_mutex);
	CStdString logMsg;

	if (audioTapeRef.get() == NULL)
	{
		LOG4CXX_WARN(LOG.immediateProcessingLog,"Trying to Q a NULL tape");
		return;
	}

	// queue the tape if it isn't already queued
	if (audioTapeRef->m_isQueued == false)
	{
		audioTapeRef->m_isQueued = true;
		m_audioTapeQueue.push_back(audioTapeRef);
	}

	m_semaphore.release();
}

void ImmediateProcessing::ThreadHandler()
{
	SetThreadName("orka:imm");

	CStdString logMsg;
	CStdString lastHandled;

	ImmediateProcessing* pImmediateProcessing = ImmediateProcessing::GetInstance();

	logMsg.Format("thread starting - queue size:%d", CONFIG.m_immediateProcessingQueueSize);
	LOG4CXX_INFO(LOG.immediateProcessingLog, logMsg);

	bool stop = false;

	for(;stop == false;)
	{
		try
		{
			AudioTapeRef audioTapeRef = pImmediateProcessing->Pop(lastHandled);

			if(audioTapeRef.get() == NULL)
			{
				lastHandled = "NULL";
				if(Daemon::Singleton()->IsStopping())
				{
					stop = true;
				}
			}
			else
			{
				//LOG4CXX_INFO(LOG.immediateProcessingLog, "Previous:" + lastHandled + " Current:" + audioTapeRef->m_portId);
				lastHandled = audioTapeRef->m_portId;
			
				audioTapeRef->Write();

				if (audioTapeRef->IsReadyForBatchProcessing())
				{

					if(CONFIG.m_tapeDurationMinimumSec>0 && audioTapeRef->m_duration<CONFIG.m_tapeDurationMinimumSec)
					{
						audioTapeRef->GetAudioFileRef()->Delete();

						CStdString logMsg;
						logMsg.Format("[%s] is less than %d sec, discarding", audioTapeRef->m_trackingId, CONFIG.m_tapeDurationMinimumSec);
						LOG4CXX_INFO(LOG.immediateProcessingLog, logMsg);
					}
					else if(audioTapeRef->m_keep == false)
					{
						audioTapeRef->GetAudioFileRef()->Delete();
						CStdString logMsg;
						logMsg.Format("[%s] Do Not Keep detected , deleting", audioTapeRef->m_trackingId, CONFIG.m_tapeDurationMinimumSec);
						LOG4CXX_INFO(LOG.immediateProcessingLog, logMsg);
					}
					else
					{
						// Pass the tape to the tape processor chain
						TapeProcessorRegistry::instance()->RunProcessingChain(audioTapeRef);
					}
				}
			}
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.immediateProcessingLog, CStdString("ImmediateProcessing: ") + e);
		}
	}
	LOG4CXX_INFO(LOG.immediateProcessingLog, CStdString("Exiting thread"));
}


