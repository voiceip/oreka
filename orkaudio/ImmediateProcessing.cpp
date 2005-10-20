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

#include "ImmediateProcessing.h"
#include "LogManager.h"
#include "ace/OS_NS_unistd.h"
#include "BatchProcessing.h"

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

void ImmediateProcessing::ThreadHandler(void *args)
{
	ImmediateProcessing* pImmediateProcessing = ImmediateProcessing::GetInstance();

	for(;;)
	{
		try
		{
			AudioTapeRef audioTapeRef = pImmediateProcessing->m_audioTapeQueue.pop();
			//LOG4CXX_DEBUG(LOG.immediateProcessingLog, CStdString("Got chunk"));
		
			audioTapeRef->Write();

			if (audioTapeRef->IsStoppedAndValid())
			{
				// Forward to batch processing thread
				BatchProcessing::GetInstance()->AddAudioTape(audioTapeRef);
			}
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.immediateProcessingLog, CStdString("ImmediateProcessing: ") + e);
		}
	}
}


