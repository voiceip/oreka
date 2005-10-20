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
#include "OrkClient.h"

Reporting Reporting::m_reportingSingleton;

Reporting* Reporting::GetInstance()
{
	return &m_reportingSingleton;
}

void Reporting::AddAudioTape(AudioTapeRef audioTapeRef)
{
	if (!m_audioTapeQueue.push(audioTapeRef))
	{
		LOG4CXX_ERROR(LOG.reportingLog, CStdString("Reporting: queue full"));
	}
}

void Reporting::ThreadHandler(void *args)
{
	Reporting* pReporting = Reporting::GetInstance();

	for(;;)
	{
		AudioTapeRef audioTapeRef = pReporting->m_audioTapeQueue.pop();

		MessageRef msgRef;
		audioTapeRef->GetMessage(msgRef);
		if(msgRef.get() && CONFIG.m_enableReporting)
		{
			CStdString msgAsSingleLineString = msgRef->SerializeSingleLine();
			LOG4CXX_INFO(LOG.reportingLog, msgAsSingleLineString);

			OrkHttpSingleLineClient c;
			SimpleResponseMsg srm;
			while (!c.Execute((SyncMessage&)(*msgRef.get()), srm, CONFIG.m_trackerHostname, CONFIG.m_trackerTcpPort, CONFIG.m_trackerServicename, CONFIG.m_clientTimeout))
			{
				ACE_OS::sleep(5);
			}
			//CStdString host("foo");
			//while (!msgRef->InvokeXmlRpc(host, 10000))
			//{
			//	ACE_OS::sleep(5);
			//}
		}
	}
}


