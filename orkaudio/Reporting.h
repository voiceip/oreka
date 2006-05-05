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

#ifndef __REPORTING_H__
#define __REPORTING_H__

#include "ThreadSafeQueue.h"
#include "AudioTape.h"

class Reporting
{
public:
	Reporting();
	static Reporting* GetInstance();
	static void ThreadHandler(void *args);

	void AddAudioTape(AudioTapeRef audioTapeRef);
private:
	static Reporting m_reportingSingleton;
	ThreadSafeQueue<AudioTapeRef> m_audioTapeQueue;
	bool m_queueFullError;
};

#endif

