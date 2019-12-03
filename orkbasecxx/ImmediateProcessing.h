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

#ifndef __IMMEDIATEPROCESSING_H__
#define __IMMEDIATEPROCESSING_H__

#include "ThreadSafeQueue.h"
#include "AudioTape.h"
#include <mutex>
class DLL_IMPORT_EXPORT_ORKBASE ImmediateProcessing : public OrkSingleton<ImmediateProcessing>
{
public:
	ImmediateProcessing();
	static void ThreadHandler();

	void AddAudioTape(AudioTapeRef audioTapeRef);

	AudioTapeRef Pop(CStdString& after);
	AudioTapeRef Pop();
	void Push(AudioTapeRef& audioTapeRef);

private:
	std::deque<AudioTapeRef> m_audioTapeQueue;
	std::mutex m_mutex;
	OrkSemaphore m_semaphore;

	time_t m_lastQueueFullTime;
};

#endif

