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

class ImmediateProcessing
{
public:
	static ImmediateProcessing* GetInstance();
	static void ThreadHandler(void *args);

	void AddAudioTape(AudioTapeRef audioTapeRef);
	void SetQueueSize(int size);
private:
	static ImmediateProcessing m_immediateProcessingSingleton;
	ThreadSafeQueue<AudioTapeRef> m_audioTapeQueue;

};

#endif

