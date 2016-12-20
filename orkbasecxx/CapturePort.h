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

#ifndef __PORT_H__
#define __PORT_H__

#include <map>
#include <list>
#include "shared_ptr.h"
#include "ace/Thread_Mutex.h"
#include "ace/Singleton.h"

#include "StdString.h"

#include "AudioCapture.h"
#include "AudioTape.h"
#include "Filter.h"

#define HOSTNAME_BUF_LEN 255
/** Base class for all types of capture ports. */ 
class DLL_IMPORT_EXPORT_ORKBASE CapturePort
{
public:
	CapturePort(CStdString& Id);
	CStdString ToString();
	CStdString GetId();

	void AddAudioChunk(AudioChunkRef chunkRef);
	void AddCaptureEvent(CaptureEventRef eventRef);
	bool IsExpired(time_t now);
	void Finalize();
	void QueueCaptureEvent(CaptureEventRef& eventRef);
	void ClearEventQueue();

private:
	void LoadFilters();
	void FilterAudioChunk(AudioChunkRef& chunkRef);
	void FilterCaptureEvent(CaptureEventRef& eventRef);
	void ReportEventBacklog(AudioTapeRef& audioTape);

	CStdString m_id;
	AudioTapeRef m_audioTapeRef;
	ACE_Thread_Mutex m_mutex;
	bool m_capturing;
	double m_vadBelowThresholdSec;
	bool m_vadUp;
	time_t m_lastUpdated;
	std::list<FilterRef> m_filters;
	std::vector<FilterRef> m_decoders;
	std::list<CaptureEventRef> m_captureEvents;
	bool m_needSendStop;
	int m_segmentNumber;
};

typedef oreka::shared_ptr<CapturePort> CapturePortRef;

/** This singleton holds all dynamically created capture ports and allows convenient access. */
class DLL_IMPORT_EXPORT_ORKBASE CapturePorts
{
public:
	CapturePorts();
	CapturePortRef GetPort(CStdString & portId);
	/** Tries to find a capture port from its ID. If unsuccessful, creates a new one and returns it */
	CapturePortRef AddAndReturnPort(CStdString & portId);
	CStdString GetHostName();
	void Hoover();
private:
	std::map<CStdString, CapturePortRef> m_ports;
	ACE_Thread_Mutex m_mutex;
	time_t m_lastHooveringTime;
	CStdString m_hostname;
};

typedef ACE_Singleton<CapturePorts, ACE_Thread_Mutex> CapturePortsSingleton;

#endif

