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

class DLL_IMPORT_EXPORT_ORKBASE ImmediateProcessing
{
public:
	ImmediateProcessing();
	static ImmediateProcessing* GetInstance();
	static void ThreadHandler(void *args);

	void AddAudioTape(AudioTapeRef audioTapeRef);

	AudioTapeRef Pop(CStdString& after);
	AudioTapeRef Pop();
	void Push(AudioTapeRef& audioTapeRef);

private:
	static ImmediateProcessing m_immediateProcessingSingleton;

	std::map<CStdString, AudioTapeRef> m_audioTapeQueue;
	ACE_Thread_Mutex m_mutex;
	ACE_Thread_Semaphore m_semaphore;

	time_t m_lastQueueFullTime;
};

#endif

