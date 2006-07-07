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
#include "Reporting.h"
#include "LogManager.h"
#include "messages/Message.h"
#include "messages/TapeMsg.h"
#include "OrkClient.h"
#include "Daemon.h"


TapeProcessorRef Reporting::m_singleton;

void Reporting::Initialize()
{
	m_singleton.reset(new Reporting());
	TapeProcessorRegistry::instance()->RegisterTapeProcessor(m_singleton);
}

Reporting* Reporting::Instance()
{
	return (Reporting*)m_singleton.get();
}

Reporting::Reporting()
{
	m_queueFullError = false;
}

CStdString __CDECL__ Reporting::GetName()
{
	return "Reporting";
}

TapeProcessorRef  Reporting::Instanciate()
{
	return m_singleton;
}

void Reporting::AddAudioTape(AudioTapeRef& audioTapeRef)
{
	if (m_audioTapeQueue.push(audioTapeRef))
	{
		LOG4CXX_DEBUG(LOG.reportingLog, CStdString("added audiotape to queue:") + audioTapeRef->GetIdentifier());
		m_queueFullError = false;
	}
	else
	{
		if(m_queueFullError == false)
		{
			m_queueFullError = true;
			LOG4CXX_ERROR(LOG.reportingLog, CStdString("queue full"));
		}
	}
}

void Reporting::ThreadHandler(void *args)
{
	TapeProcessorRef reporting = TapeProcessorRegistry::instance()->GetNewTapeProcessor(CStdString("Reporting"));
	if(reporting.get() == NULL)
	{
		LOG4CXX_ERROR(LOG.reportingLog, "Could not instanciate Reporting");
		return;
	}
	Reporting* pReporting = (Reporting*)(reporting->Instanciate().get());

	bool stop = false;

	for(;stop == false;)
	{
		try
		{
			AudioTapeRef audioTapeRef = pReporting->m_audioTapeQueue.pop();

			if(audioTapeRef.get() == NULL)
			{
				if(DaemonSingleton::instance()->IsStopping())
				{
					stop = true;
				}
			}
			else
			{

				MessageRef msgRef;
				audioTapeRef->GetMessage(msgRef);
				TapeMsg* ptapeMsg = (TapeMsg*)msgRef.get();
				bool startMsg = false;

				if(msgRef.get() /*&& CONFIG.m_enableReporting*/)
				{
					if(ptapeMsg->m_stage.Equals("START"))
					{
						startMsg = true;
					}

					CStdString msgAsSingleLineString = msgRef->SerializeSingleLine();
					LOG4CXX_INFO(LOG.reportingLog, msgAsSingleLineString);

					OrkHttpSingleLineClient c;
					TapeResponseRef tr(new TapeResponse());
					audioTapeRef->m_tapeResponse = tr;

					bool success = false;
					bool firstError = true;

					while (!success)
					{
						if (c.Execute((SyncMessage&)(*msgRef.get()), (AsyncMessage&)(*tr.get()), CONFIG.m_trackerHostname, CONFIG.m_trackerTcpPort, CONFIG.m_trackerServicename, CONFIG.m_clientTimeout))
						{
							success = true;
							if(tr->m_deleteTape)
							{
								CStdString tapeFilename = audioTapeRef->GetFilename();

								CStdString absoluteFilename = CONFIG.m_audioOutputPath + "/" + tapeFilename;
								if (ACE_OS::unlink((PCSTR)absoluteFilename) == 0)
								{
									LOG4CXX_INFO(LOG.reportingLog, "Deleted tape: " + tapeFilename);
								}
								else
								{
									LOG4CXX_DEBUG(LOG.reportingLog, "Could not delete tape: " + tapeFilename);
								}

							}
							else
							{
								if(!startMsg)
								{
									// Pass the tape to the next processor
									pReporting->RunNextProcessor(audioTapeRef);
								}
							}
						}
						else
						{
							if(firstError)
							{
								firstError = false;
								LOG4CXX_ERROR(LOG.reportingLog, CStdString("Could not contact orktrack"));
							}
							ACE_OS::sleep(5);
						}
					}
				}
			}
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.reportingLog, CStdString("Exception: ") + e);
		}
	}
	LOG4CXX_INFO(LOG.reportingLog, CStdString("Exiting thread"));
}



