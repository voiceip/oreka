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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include <log4cxx/logger.h>
#include "CapturePort.h"
#include "Utils.h"
#include "ImmediateProcessing.h"
#include "Reporting.h"
#include "ConfigManager.h"
#include "ace/Thread_Mutex.h"

static LoggerPtr s_log;
static ACE_Thread_Mutex s_mutex;

CapturePort::CapturePort(CStdString& id)
{
	m_id = id;
	m_vadBelowThresholdSec = 0.0;
	m_vadUp = false;
	m_capturing = false;
	m_lastUpdated = 0;
	m_needSendStop = false;
	m_segmentNumber = 0;

	LoadFilters();
}

void CapturePort::LoadFilters()
{
	for(std::list<CStdString>::iterator it = CONFIG.m_capturePortFilters.begin(); it != CONFIG.m_capturePortFilters.end(); it++)
	{
		CStdString filterName = *it;
		FilterRef filter = FilterRegistry::instance()->GetNewFilter(filterName);
		if(filter.get())
		{
			m_filters.push_back(filter);
			LOG4CXX_DEBUG(s_log, CStdString("Adding filter:") + filterName);
		}
		else
		{
			LOG4CXX_ERROR(s_log, CStdString("Filter:") + filterName + " does not exist, please check <CapturePortFilters> in config.xml");
		}
	}
	if(CONFIG.m_vad == true)
	{
		FilterRef decoder;
		for(int pt = 0; pt < RTP_PAYLOAD_TYPE_MAX; pt++)
		{
			decoder = FilterRegistry::instance()->GetNewFilter(pt);
			m_decoders.push_back(decoder);
		}
	}
}


CStdString CapturePort::ToString()
{
	CStdString ret;
	return ret;
}

CStdString CapturePort::GetId()
{
	return m_id;
}

void CapturePort::FilterAudioChunk(AudioChunkRef& chunkRef)
{
	// Iterate through all filters
	std::list<FilterRef>::iterator it;
	for(it = m_filters.begin(); it != m_filters.end(); it++)
	{
		FilterRef filter = *it;
		filter->AudioChunkIn(chunkRef);
		filter->AudioChunkOut(chunkRef);
	}
}

void CapturePort::FilterCaptureEvent(CaptureEventRef& eventRef)
{
	// Iterate through all filters
	std::list<FilterRef>::iterator it;
	for(it = m_filters.begin(); it != m_filters.end(); it++)
	{
		FilterRef filter = *it;
		filter->CaptureEventIn(eventRef);
		filter->CaptureEventOut(eventRef);
	}
}

void CapturePort::QueueCaptureEvent(CaptureEventRef& eventRef)
{
	m_captureEvents.push_back(eventRef);
}

void CapturePort::ClearEventQueue()
{
	m_captureEvents.clear();
}

void CapturePort::ReportEventBacklog(AudioTapeRef& audioTape)
{
	std::list<CaptureEventRef>::iterator it;

	for(it = m_captureEvents.begin(); it != m_captureEvents.end(); it++)
	{
		CaptureEventRef eventRef = *it;

		if(eventRef->m_type == CaptureEvent::EtOrkUid)
		{
			CStdString newOrkUid;
			CaptureEventRef eventRef2(new CaptureEvent());

			m_segmentNumber += 1;
			newOrkUid.Format("%s_%d", eventRef->m_value, m_segmentNumber);

			eventRef2->m_type = CaptureEvent::EtOrkUid;
			eventRef2->m_value = newOrkUid;

			AddCaptureEvent(eventRef2);
		}
		else
		{
			AddCaptureEvent(eventRef);
		}
	}
}

void CapturePort::AddAudioChunk(AudioChunkRef chunkRef)
{
	FilterAudioChunk(chunkRef);

	time_t now = time(NULL);
	m_lastUpdated = now;

	if(CONFIG.m_audioSegmentation)
	{
		if (m_audioTapeRef.get())
		{
			if ((now - m_audioTapeRef->m_beginDate) >= CONFIG.m_audioSegmentDuration)
			{
				// signal current tape stop event
				CaptureEventRef eventRef(new CaptureEvent);
				eventRef->m_type = CaptureEvent::EtStop;
				eventRef->m_timestamp = now;
				AddCaptureEvent(eventRef);

				// create new tape
				m_audioTapeRef.reset(new AudioTape(m_id));

				// signal new tape start event
				eventRef.reset(new CaptureEvent);
				eventRef->m_type = CaptureEvent::EtStart;
				eventRef->m_value = m_id;
				eventRef->m_timestamp = now;
				AddCaptureEvent(eventRef);
				ReportEventBacklog(m_audioTapeRef);
			}
		}	
		else
		{
			// create new tape
			m_audioTapeRef.reset(new AudioTape(m_id));

			// signal new tape start event
			CaptureEventRef eventRef(new CaptureEvent);
			eventRef->m_type = CaptureEvent::EtStart;
			eventRef->m_timestamp = now;
			eventRef->m_value = m_id;
			AddCaptureEvent(eventRef);
			ReportEventBacklog(m_audioTapeRef);
		}
	}
	else if (CONFIG.m_vad)
	{

		AudioChunkRef tmpChunkRef;
		AudioChunkDetails details = *chunkRef->GetDetails();

		if(chunkRef->GetEncoding() != PcmAudio)
		{
			FilterRef decoder;

			decoder = m_decoders.at(details.m_rtpPayloadType);
			if(decoder.get() != NULL)
			{
				decoder->AudioChunkIn(chunkRef);
				decoder->AudioChunkOut(tmpChunkRef);
			}
		}
		else
		{
			tmpChunkRef = chunkRef;
		}

		if(tmpChunkRef.get())
		{
			if(m_vadUp)
			{
				// There is an ongoing capture
				if (tmpChunkRef->ComputeRmsDb() < CONFIG.m_vadLowThresholdDb)
				{
					// Level has gone below low threshold, increase holdon counter
					m_vadBelowThresholdSec += tmpChunkRef->GetDurationSec();
				}
				else
				{
					// Level has gone above low threshold, reset holdon counter
					m_vadBelowThresholdSec = 0.0;
				}

				if (m_vadBelowThresholdSec > CONFIG.m_vadHoldOnSec)
				{
					// no activity detected for more than hold on time
					m_vadUp = false;

					// signal current tape stop event
					CaptureEventRef eventRef(new CaptureEvent);
					eventRef->m_type = CaptureEvent::EtStop;
					eventRef->m_timestamp = now;
					AddCaptureEvent(eventRef);

					LOG4CXX_DEBUG(s_log, "[" + m_audioTapeRef->m_trackingId + "] VAD triggered Stop");
					m_audioTapeRef.reset();
				}
			}
			else
			{
				// No capture is taking place yet
				if (tmpChunkRef->ComputeRmsDb() > CONFIG.m_vadHighThresholdDb)
				{
					// Voice detected, start a new capture
					m_vadBelowThresholdSec = 0.0;
					m_vadUp = true;

					// create new tape
					if(!m_audioTapeRef.get())
					{
						m_audioTapeRef.reset(new AudioTape(m_id));

						LOG4CXX_DEBUG(s_log, "[" + m_audioTapeRef->m_trackingId + "] VAD triggered Start");
						// signal new tape start event
						CaptureEventRef eventRef(new CaptureEvent);
						eventRef->m_type = CaptureEvent::EtStart;
						eventRef->m_timestamp = now;
						eventRef->m_value = m_id;
						AddCaptureEvent(eventRef);
						ReportEventBacklog(m_audioTapeRef);
					}
				}
			}
		}
		else
		{
			CStdString logMsg;

			logMsg.Format("Voice activity detection: unsupported RTP payload type:%d", details.m_rtpPayloadType);
			LOG4CXX_ERROR(s_log, logMsg);
		}
	}

	if (m_audioTapeRef.get() && m_capturing)
	{
		m_audioTapeRef->AddAudioChunk(chunkRef);

		// Signal to immediate processing thread that tape has new stuff
		ImmediateProcessing::GetInstance()->AddAudioTape(m_audioTapeRef);
	}
}

void CapturePort::AddCaptureEvent(CaptureEventRef eventRef)
{
	FilterCaptureEvent(eventRef);

	m_lastUpdated = time(NULL);

	AudioTapeRef audioTapeRef = m_audioTapeRef;

	// First of all, handle tape start
	if (eventRef->m_type == CaptureEvent::EtStart)
	{
		m_capturing = true;
		if (audioTapeRef.get())
		{
			audioTapeRef->SetShouldStop();	// force stop of previous tape
		}
		audioTapeRef.reset(new AudioTape(m_id));	// Create a new tape
		audioTapeRef->AddCaptureEvent(eventRef, true);

		m_audioTapeRef = audioTapeRef;
		LOG4CXX_INFO(s_log, "[" + m_audioTapeRef->m_trackingId + "] #" + m_id + " start");
	}

	if (!audioTapeRef.get())
	{
		if(!CONFIG.m_vad && !CONFIG.m_audioSegmentation)
		{
			// These are queued for VAD & Audio Segmentation
			LOG4CXX_WARN(s_log, "#" + m_id + ": received unexpected capture event:" 
				+ CaptureEvent::EventTypeToString(eventRef->m_type));
		}
	}
	else
	{
		// Ok, at this point, we know we have a valid audio tape
		switch(eventRef->m_type)
		{
		case CaptureEvent::EtStart:
			break;
		case CaptureEvent::EtStop:
		{
			m_capturing = false;
			LOG4CXX_INFO(s_log, "[" + audioTapeRef->m_trackingId + "] #" + m_id + " stop");
			audioTapeRef->AddCaptureEvent(eventRef, true);
			
			MessageRef msgRef;
			audioTapeRef->GetMessage(msgRef);
			Reporting::Instance()->AddMessage(msgRef);
			m_needSendStop = false;

			if (m_audioTapeRef->GetAudioFileRef().get())
			{
				// Notify immediate processing that tape has stopped
				ImmediateProcessing::GetInstance()->AddAudioTape(m_audioTapeRef);
			}
			else
			{
				// Received a stop but there is no valid audio file associated with the tape
				audioTapeRef->m_noAudio = true;
				LOG4CXX_WARN(s_log, "[" + audioTapeRef->m_trackingId + "] #" + m_id + " no audio reported between last start and stop");
			}
			break;
		}
		case CaptureEvent::EtEndMetadata:
		{
			// Now that all metadata has been acquired, we can generate the tape start message

			MessageRef msgRef;
			audioTapeRef->GetMessage(msgRef);
			Reporting::Instance()->AddMessage(msgRef);
			m_needSendStop = true;

			break;
		}
		case CaptureEvent::EtUpdate:
		{
			audioTapeRef->AddCaptureEvent(eventRef, true);
			// Generate tape update message
			MessageRef msgRef;
			audioTapeRef->GetMessage(msgRef);
			Reporting::Instance()->AddMessage(msgRef);
			break;
		}
		case CaptureEvent::EtLocalSide:
		case CaptureEvent::EtAudioKeepDirection:
		case CaptureEvent::EtDirection:
		case CaptureEvent::EtRemoteParty:
		case CaptureEvent::EtLocalParty:
		case CaptureEvent::EtLocalEntryPoint:
		default:
			audioTapeRef->AddCaptureEvent(eventRef, false);
		}
	}
}

bool CapturePort::IsExpired(time_t now)
{
	if((now - m_lastUpdated) > (10*60))	// 10 minutes
	{
		if(m_audioTapeRef.get())
		{
			if(m_audioTapeRef->m_state != AudioTape::StateActive)
			{
				return true;
			}
		}
		else
		{
			return true;
		}
	}
	return false;
}

void CapturePort::Finalize()
{
	if(m_needSendStop)
	{
		// Stop message not sent yet for some reason, force it
		CaptureEventRef stopEvent(new CaptureEvent);
		stopEvent->m_type = CaptureEvent::EtStop;
		stopEvent->m_timestamp = time(NULL);
		AddCaptureEvent(stopEvent);

		MessageRef msgRef;
		m_audioTapeRef->GetMessage(msgRef);
		Reporting::Instance()->AddMessage(msgRef);
		m_needSendStop = false;
	}
}

//=======================================
CapturePorts::CapturePorts()
{
	m_ports.clear();
	m_lastHooveringTime = time(NULL);
	char hostname[HOSTNAME_BUF_LEN];
	if(CONFIG.m_hostnameReportFqdn == false)
	{
		ACE_OS::hostname(hostname, HOSTNAME_BUF_LEN);
		m_hostname = hostname;
	}
	else
	{
		GetHostFqdn(m_hostname, HOSTNAME_BUF_LEN);
	}
	s_log = Logger::getLogger("port");
}

CapturePortRef CapturePorts::GetPort(CStdString & portId)
{
	MutexSentinel mutexSentinel(s_mutex);
	Hoover();
	std::map<CStdString, CapturePortRef>::iterator pair;

	pair = m_ports.find(portId);

	if (pair == m_ports.end())
	{
		CapturePortRef nullPortRef;
		return nullPortRef;
	}
	else
	{
		return pair->second;
	}
}

CapturePortRef CapturePorts::AddAndReturnPort(CStdString & portId)
{
	CapturePortRef portRef = GetPort(portId);
	if (portRef.get() == NULL)
	{
		MutexSentinel mutexSentinel(s_mutex);
		// The port does not already exist, create it.
		CapturePortRef newPortRef(new CapturePort(portId));
		m_ports.insert(std::make_pair(portId, newPortRef));
		return newPortRef;
	}
	else
	{
		return portRef;
	}
}

CStdString CapturePorts::GetHostName()
{
	return m_hostname;
}

void CapturePorts::Hoover()
{
	CStdString logMsg;
	time_t now = time(NULL);
	if( (now - m_lastHooveringTime) > 10)		// Hoover every 10 seconds
	{
		m_lastHooveringTime = now;
		int numPorts = m_ports.size();

		// Go round and detect inactive ports
		std::map<CStdString, CapturePortRef>::iterator pair;
		std::list<CapturePortRef> toDismiss;

		for(pair = m_ports.begin(); pair != m_ports.end(); pair++)
		{
			CapturePortRef port = pair->second;
			if(port->IsExpired(now))
			{
				toDismiss.push_back(port);
			}
		}

		// Discard inactive ports
		for (std::list<CapturePortRef>::iterator it = toDismiss.begin(); it != toDismiss.end() ; it++)
		{
			CapturePortRef port = *it;
			port->Finalize();
			m_ports.erase(port->GetId());
			LOG4CXX_DEBUG(s_log,  port->GetId() + ": Expired");
		}
		logMsg.Format("Hoovered %d ports. New number:%d", (numPorts - m_ports.size()), m_ports.size());
		LOG4CXX_DEBUG(s_log,  logMsg);
	}
}


