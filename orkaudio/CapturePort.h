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
#include "boost/shared_ptr.hpp"
#include "ace/Thread_Mutex.h"
#include "ace/Singleton.h"

#include "StdString.h"

#include "AudioCapture.h"
#include "AudioTape.h"


/** Base class for all types of capture ports. */ 
class CapturePort
{
public:
	CapturePort(CStdString& Id);
	CStdString ToString();
	CStdString GetId();

	void AddAudioChunk(AudioChunkRef chunkRef);
	void AddCaptureEvent(CaptureEventRef eventRef);
	bool IsExpired(time_t now);
private:
	CStdString m_id;
	AudioTapeRef m_audioTapeRef;
	ACE_Thread_Mutex m_mutex;
	bool m_capturing;
	double m_vadBelowThresholdSec;
	bool m_vadUp;
	time_t m_lastUpdated;
};

typedef boost::shared_ptr<CapturePort> CapturePortRef;

/** This singleton holds all dynamically created capture ports and allows convenient access. */
class CapturePorts
{
public:
	CapturePorts();
	CapturePortRef GetPort(CStdString & portId);
	/** Tries to find a capture port from its ID. If unsuccessful, creates a new one and returns it */
	CapturePortRef AddAndReturnPort(CStdString & portId);
	void Hoover();
private:
	std::map<CStdString, CapturePortRef> m_ports;
	ACE_Thread_Mutex m_mutex;
	time_t m_lastHooveringTime;
};

typedef ACE_Singleton<CapturePorts, ACE_Thread_Mutex> CapturePortsSingleton;

#endif

