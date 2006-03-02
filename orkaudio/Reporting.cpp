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

#include "ConfigManager.h"
#include "Reporting.h"
#include "LogManager.h"
#include "messages/Message.h"
#include "messages/TapeMsg.h"
#include "OrkClient.h"
#include "Daemon.h"
#include "BatchProcessing.h"


Reporting Reporting::m_reportingSingleton;

Reporting* Reporting::GetInstance()
{
	return &m_reportingSingleton;
}

void Reporting::AddAudioTape(AudioTapeRef audioTapeRef)
{
	if (m_audioTapeQueue.push(audioTapeRef))
	{
		LOG4CXX_DEBUG(LOG.reportingLog, CStdString("added audiotape to queue:") + audioTapeRef->GetIdentifier());
	}
	else
	{
		LOG4CXX_ERROR(LOG.reportingLog, CStdString("Reporting: queue full"));
	}
}

void Reporting::ThreadHandler(void *args)
{
	Reporting* pReporting = Reporting::GetInstance();
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
				if(msgRef.get() && CONFIG.m_enableReporting)
				{
					CStdString msgAsSingleLineString = msgRef->SerializeSingleLine();
					LOG4CXX_INFO(LOG.reportingLog, msgAsSingleLineString);

					OrkHttpSingleLineClient c;
					TapeResponse tr;

					bool success = false;
					bool firstError = true;

					while (!success)
					{
						if (c.Execute((SyncMessage&)(*msgRef.get()), tr, CONFIG.m_trackerHostname, CONFIG.m_trackerTcpPort, CONFIG.m_trackerServicename, CONFIG.m_clientTimeout))
						{
							success = true;
							if(tr.m_deleteTape)
							{
								LOG4CXX_INFO(LOG.reportingLog, "Registered tape for removal: " + audioTapeRef->GetIdentifier());
								CStdString tapeFilename = audioTapeRef->GetFilename();
								BatchProcessing::GetInstance()->TapeDropRegistration(tapeFilename);
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


