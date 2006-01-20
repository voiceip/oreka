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

#include "CapturePort.h"
#include "Utils.h"
#include "ImmediateProcessing.h"
#include "LogManager.h"
#include "Reporting.h"
#include "ConfigManager.h"

CapturePort::CapturePort(CStdString& Id)
{
	m_Id = Id;
	m_vadBelowThresholdSec = 0.0;
	m_vadUp = false;
	m_capturing = false;
}

CStdString CapturePort::ToString()
{
	CStdString ret;
	return ret;
}

void CapturePort::AddAudioChunk(AudioChunkRef chunkRef, bool remote)
{
	time_t now = time(NULL);

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
				m_audioTapeRef.reset(new AudioTape(m_Id));

				// signal new tape start event
				eventRef.reset(new CaptureEvent);
				eventRef->m_type = CaptureEvent::EtStart;
				eventRef->m_timestamp = now;
				AddCaptureEvent(eventRef);
			}
		}	
		else
		{
			// create new tape
			m_audioTapeRef.reset(new AudioTape(m_Id));

			// signal new tape start event
			CaptureEventRef eventRef(new CaptureEvent);
			eventRef->m_type = CaptureEvent::EtStart;
			eventRef->m_timestamp = now;
			AddCaptureEvent(eventRef);
		}
	}
	else if (CONFIG.m_vad)
	{
		if(chunkRef->GetEncoding() == PcmAudio)
		{
			if(m_vadUp)
			{
				// There is an ongoing capture
				if (chunkRef->ComputeRmsDb() < CONFIG.m_vadLowThresholdDb)
				{
					// Level has gone below low threshold, increase holdon counter
					m_vadBelowThresholdSec += chunkRef->GetDurationSec();
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
				}
			}
			else
			{
				// No capture is taking place yet
				if (chunkRef->ComputeRmsDb() > CONFIG.m_vadHighThresholdDb)
				{
					// Voice detected, start a new capture
					m_vadBelowThresholdSec = 0.0;
					m_vadUp = true;

					// create new tape
					m_audioTapeRef.reset(new AudioTape(m_Id));

					// signal new tape start event
					CaptureEventRef eventRef(new CaptureEvent);
					eventRef->m_type = CaptureEvent::EtStart;
					eventRef->m_timestamp = now;
					AddCaptureEvent(eventRef);
				}
			}
		}
		else
		{
			LOG4CXX_ERROR(LOG.portLog, CStdString("Voice activity detection cannot be used on non PCM audio"));
		}
	}

	// ############ 
	//if (!m_audioTapeRef.get())
	//{
	//	m_audioTapeRef.reset(new AudioTape(m_Id));
	//	LOG4CXX_WARN(LOG.portLog, CStdString("Got impromptu audio"));
	//}

	if (m_audioTapeRef.get() && m_capturing)
	{
		m_audioTapeRef->AddAudioChunk(chunkRef, remote);

		// Signal to immediate processing thread that tape has new stuff
		ImmediateProcessing::GetInstance()->AddAudioTape(m_audioTapeRef);
	}
}

void CapturePort::AddCaptureEvent(CaptureEventRef eventRef)
{
	AudioTapeRef audioTapeRef = m_audioTapeRef;

	// First of all, handle tape start
	if (eventRef->m_type == CaptureEvent::EtStart)
	{
		m_capturing = true;
		if (audioTapeRef.get())
		{
			audioTapeRef->SetShouldStop();	// force stop of previous tape
		}
		audioTapeRef.reset(new AudioTape(m_Id));	// Create a new tape
		audioTapeRef->AddCaptureEvent(eventRef, true);
		Reporting::GetInstance()->AddAudioTape(audioTapeRef);
		m_audioTapeRef = audioTapeRef;
		LOG4CXX_INFO(LOG.portLog, "#" + m_Id + ": start");
	}

	if (!audioTapeRef.get())
	{
		LOG4CXX_WARN(LOG.portLog, "#" + m_Id + ": received unexpected capture event:" 
			+ CaptureEvent::EventTypeToString(eventRef->m_type));
	}
	else
	{
		// Ok, at this point, we know we have a valid audio tape
		switch(eventRef->m_type)
		{
		case CaptureEvent::EtStop:

			m_capturing = false;
			LOG4CXX_INFO(LOG.portLog, "#" + m_Id + ": stop");
			audioTapeRef->AddCaptureEvent(eventRef, true);

			if (m_audioTapeRef->GetAudioFileRef().get())
			{
				// Notify immediate processing that tape has stopped
				ImmediateProcessing::GetInstance()->AddAudioTape(m_audioTapeRef);
				// Reporting needs to send a stop message
				Reporting::GetInstance()->AddAudioTape(audioTapeRef);
			}
			else
			{
				// Received a stop but there is no valid audio file associated with the tape
				LOG4CXX_WARN(LOG.portLog, "#" + m_Id + ": no audio reported between last start and stop");
			}
			break;
		case CaptureEvent::EtDirection:
		case CaptureEvent::EtRemoteParty:
		case CaptureEvent::EtLocalParty:
		case CaptureEvent::EtLocalEntryPoint:
		default:
			audioTapeRef->AddCaptureEvent(eventRef, false);
		}
	}
}


//=======================================

void CapturePorts::Initialize()
{
	m_ports.clear();
}

CapturePortRef CapturePorts::GetPort(CStdString & portId)
{
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
	//MutexGuard mutexGuard(m_mutex);		// To make sure a channel cannot be created twice

	CapturePortRef portRef = GetPort(portId);
	if (portRef.get() == NULL)
	{
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


