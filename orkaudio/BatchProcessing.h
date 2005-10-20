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

#ifndef __BATCHPROCESSING_H__
#define __BATCHPROCESSING_H__

#include "ThreadSafeQueue.h"
#include "AudioTape.h"
#include "ace/Thread_Mutex.h"

class BatchProcessing
{
public:
	static BatchProcessing* GetInstance();
	static void ThreadHandler(void *args);

	void AddAudioTape(AudioTapeRef audioTapeRef);
private:
	BatchProcessing();

	static BatchProcessing m_batchProcessingSingleton;
	ThreadSafeQueue<AudioTapeRef> m_audioTapeQueue;

	size_t m_threadCount;
	ACE_Thread_Mutex m_mutex;
};

#endif

