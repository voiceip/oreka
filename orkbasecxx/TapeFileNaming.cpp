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
#include "Daemon.h"
#include "Filter.h"
#include "Reporting.h"
#include "TapeFileNaming.h"

TapeProcessorRef TapeFileNaming::m_singleton;

void TapeFileNaming::Initialize()
{
	if(m_singleton.get() == NULL)
	{
		m_singleton.reset(new TapeFileNaming());
		TapeProcessorRegistry::instance()->RegisterTapeProcessor(m_singleton);
	}
}

TapeFileNaming::TapeFileNaming()
{
	m_threadCount = 0;
	apr_time_t tn = apr_time_now();
	apr_time_exp_t texp;
   	apr_time_exp_lt(&texp, tn);
	m_currentDay = texp.tm_mday;
}

CStdString __CDECL__ TapeFileNaming::GetName()
{
	return "TapeFileNaming";
}

TapeProcessorRef TapeFileNaming::Instanciate()
{
	return m_singleton;
}

void TapeFileNaming::AddAudioTape(AudioTapeRef& audioTapeRef)
{
	if (!m_audioTapeQueue.push(audioTapeRef))
	{
		LOG4CXX_ERROR(LOG.tapeFileNamingLog, CStdString("queue full"));
	}
}

void TapeFileNaming::SetQueueSize(int size)
{
	m_audioTapeQueue.setSize(size);
}

void TapeFileNaming::ThreadHandler()
{
	SetThreadName("orka:tapename");

	CStdString logMsg;

	CStdString processorName("TapeFileNaming");
	TapeProcessorRef tapeFileNaming = TapeProcessorRegistry::instance()->GetNewTapeProcessor(processorName);
	if(tapeFileNaming.get() == NULL)
	{
		LOG4CXX_ERROR(LOG.tapeFileNamingLog, "Could not instanciate TapeFileNaming");
		return;
	}

	TapeFileNaming *pTapeFileNaming = (TapeFileNaming*)(tapeFileNaming->Instanciate().get());

	pTapeFileNaming->SetQueueSize(20000);
	LOG4CXX_INFO(LOG.tapeFileNamingLog, "Started");

	bool stop = false;

	for(;stop == false;)
	{
		AudioTapeRef audioTapeRef;
		CStdString trackingId = "[no-trk]";

		try
		{
			audioTapeRef = pTapeFileNaming->m_audioTapeQueue.pop();
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
				CStdString originalFilename, newFilename;

				trackingId = audioTapeRef->m_trackingId;
	
				originalFilename = audioTapeRef->m_audioOutputPath + "/" + audioTapeRef->GetFilename();
				audioTapeRef->GenerateFinalFilePathAndIdentifier();
				newFilename = audioTapeRef->m_audioOutputPath + "/" + audioTapeRef->GetFilename();

				if(originalFilename.Compare(newFilename) != 0)
				{
					if(std::rename((PCSTR)originalFilename.c_str(), (PCSTR)newFilename.c_str()) != 0)
					{
						logMsg.Format("[%s] error renaming file from %s to %s: %s", trackingId, originalFilename, newFilename, strerror(errno));
						LOG4CXX_ERROR(LOG.tapeFileNamingLog, logMsg);
					}
				}

				pTapeFileNaming->RunNextProcessor(audioTapeRef);
			}
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.tapeFileNamingLog, e);
		}
	}

	LOG4CXX_INFO(LOG.tapeFileNamingLog, CStdString("Exited"));
}

