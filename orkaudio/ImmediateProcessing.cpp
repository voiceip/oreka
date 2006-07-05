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
#include "ace/OS_NS_unistd.h"
#include "BatchProcessing.h"
#include "Daemon.h"
#include "ConfigManager.h"
#include "TapeProcessor.h"


ImmediateProcessing ImmediateProcessing::m_immediateProcessingSingleton;

ImmediateProcessing* ImmediateProcessing::GetInstance()
{
	return &m_immediateProcessingSingleton;
}

void ImmediateProcessing::AddAudioTape(AudioTapeRef audioTapeRef)
{
	if (!m_audioTapeQueue.push(audioTapeRef))
	{
		// Log error
		LOG4CXX_ERROR(LOG.immediateProcessingLog, CStdString("ImmediateProcessing: queue full"));
	}
}

void ImmediateProcessing::SetQueueSize(int size)
{
	m_audioTapeQueue.setSize(size);
}


void ImmediateProcessing::ThreadHandler(void *args)
{
	CStdString logMsg;

	ImmediateProcessing* pImmediateProcessing = ImmediateProcessing::GetInstance();
	pImmediateProcessing->SetQueueSize(CONFIG.m_immediateProcessingQueueSize);

	logMsg.Format("thread starting - queue size:%d", CONFIG.m_immediateProcessingQueueSize);
	LOG4CXX_INFO(LOG.immediateProcessingLog, logMsg);

	bool stop = false;

	for(;stop == false;)
	{
		try
		{
			AudioTapeRef audioTapeRef = pImmediateProcessing->m_audioTapeQueue.pop();

			if(audioTapeRef.get() == NULL)
			{
				if(DaemonSingleton::instance()->IsStopping())
				{
					stop = true;
				}
			}
			else
			{
				//LOG4CXX_DEBUG(LOG.immediateProcessingLog, CStdString("Got chunk"));
			
				audioTapeRef->Write();

				if (audioTapeRef->IsReadyForBatchProcessing())
				{
					// Pass the tape to the tape processor chain
					TapeProcessorRegistry::instance()->RunProcessingChain(audioTapeRef);
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

