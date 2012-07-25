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

#include "ConfigManager.h"
#include <log4cxx/logger.h>
#include "TapeProcessor.h"

using namespace log4cxx;

TapeProcessor::TapeProcessor()
{
	;
}

void TapeProcessor::SetNextProcessor(TapeProcessorRef& nextProcessor)
{
	m_nextProcessor = nextProcessor;
}

void TapeProcessor::RunNextProcessor(AudioTapeRef& tape)
{
	if(m_nextProcessor.get())
	{
		if(tape->m_isExternal && tape->m_mediaType != MediaType::AudioType && tape->m_mediaType != MediaType::VideoType && m_nextProcessor->GetName().CompareNoCase("CommandProcessing") == 0)	//Skip CommandProcessing (where transcoding happens in case of external audio) if the tape is Instant message
		{
			m_nextProcessor->RunNextProcessor(tape);
		}
		else
		{
			m_nextProcessor->AddAudioTape(tape);
		}
	}
}


//=====================================================
LoggerPtr s_log;

TapeProcessorRegistry::TapeProcessorRegistry()
{
	s_log = Logger::getLogger("tape.taperegistry");
}


void TapeProcessorRegistry::RegisterTapeProcessor(TapeProcessorRef& tapeProcessor) 
{
	m_TapeProcessors.push_back(tapeProcessor);
	LOG4CXX_INFO(s_log, CStdString("Registered processor: ") + tapeProcessor->GetName());
}


TapeProcessorRef TapeProcessorRegistry::GetNewTapeProcessor(CStdString& TapeProcessorName)
{
	for(std::list<TapeProcessorRef>::iterator it = m_TapeProcessors.begin(); it!=m_TapeProcessors.end(); it++)
	{
		TapeProcessorRef TapeProcessor = *it;

		if(	TapeProcessor->GetName().CompareNoCase(TapeProcessorName) == 0 ) 
		{
			return TapeProcessor->Instanciate();
		}
	}
	return TapeProcessorRef();	// No TapeProcessor found
}

TapeProcessorRegistry* TapeProcessorRegistry::m_singleton = 0;

TapeProcessorRegistry* TapeProcessorRegistry::instance()
{
	if(m_singleton == NULL)
	{
		m_singleton = new TapeProcessorRegistry();
	}
	return m_singleton;
}

void TapeProcessorRegistry::CreateProcessingChain()
{
	TapeProcessorRef previousProcessor;

	//ConfigManager* cm = ConfigManagerSingleton::instance();

	for(std::list<CStdString>::iterator it = CONFIG.m_tapeProcessors.begin(); it != CONFIG.m_tapeProcessors.end(); it++)
	{
		CStdString tapeProcessorName = *it;
		TapeProcessorRef processor = GetNewTapeProcessor(tapeProcessorName);
		if(processor.get())
		{
			if(m_firstTapeProcessor.get() == NULL)
			{
				m_firstTapeProcessor = processor;
			}
			if(previousProcessor.get())
			{
				previousProcessor->SetNextProcessor(processor);
			}
			previousProcessor = processor;
			LOG4CXX_DEBUG(s_log, CStdString("Adding processor to chain:") + tapeProcessorName);
		}
		else
		{
			LOG4CXX_ERROR(s_log, CStdString("Processor:") + tapeProcessorName + " does not exist, please check <TapeProcessors> in config.xml");
		}
	}
}

void TapeProcessorRegistry::RunProcessingChain(AudioTapeRef& tape)
{
	if(m_firstTapeProcessor.get())
	{
		if(!tape->m_isExternal)
		{
			m_firstTapeProcessor->AddAudioTape(tape);
		}
		else	// this is manually imported tape
		{
			if(m_firstTapeProcessor->GetName().CompareNoCase("BatchProcessing") == 0)	//Skip BatchProcessing tape processor for imported tape
			{
				m_firstTapeProcessor->RunNextProcessor(tape);
			}

		}
	}
}
