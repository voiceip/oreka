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

#ifndef __THREADSAFEQUEUE_H__
#define __THREADSAFEQUEUE_H__

#include <queue>
#include "ace/Thread_Mutex.h"
#include "ace/Thread_Semaphore.h"
#include "Utils.h"

/** Thread safe queue holding objects of arbitrary class.
	Enqueuing is never blocking
	Dequeuing is blocking when the queue is empty
 */
template <class T> class ThreadSafeQueue
{
public:
	ThreadSafeQueue(int size = 50000)
	{
		m_size = size;
		m_semaphore.acquire(); // reset count to zero
	};

	bool push(T &);
	T pop();
	int numElements();
	void setSize(int size);

private:
	int m_size;
	ACE_Thread_Mutex m_mutex;
	ACE_Thread_Semaphore m_semaphore;
	std::queue<T> m_queue;
};


/** Push an element onto the queue, returns false if queue full (never blocks) */
template <class T> bool ThreadSafeQueue<T>::push(T &element)
{
	bool result = false;
	MutexSentinel mutexSentinel(m_mutex);

	if (m_queue.size() < (unsigned int)m_size)
	{
		m_queue.push(element);
		result = true;
	}

	m_semaphore.release();
	return result;
}

/** Pop and element from the queue, or blocks until one available */
template <class T> T ThreadSafeQueue<T>::pop()
{
// #### Fixme: when timeout specified under Linux CentOS 4.2, acquire returns immediately instead of waiting for the timeout -> causes CPU to spike at 100% 

//#ifdef WIN32
//	ACE_Time_Value timeout(time(NULL)+2);
//	m_semaphore.acquire(&timeout);
//#else
//	m_semaphore.acquire();
//#endif
	
	// XXX: The semaphore timeout logic above for Windows has
	// the effect of causing tape processors to exit when
	// transcoding

	m_semaphore.acquire();
	MutexSentinel mutexSentinel(m_mutex);

	T element;

	if(m_queue.size() > 0)
	{
		element = m_queue.front();
		m_queue.pop();
	}
	return element;
}

template <class T> int ThreadSafeQueue<T>::numElements()
{
	return m_queue.size();
}

template <class T> void ThreadSafeQueue<T>::setSize(int size)
{
	m_size = size;
}

#endif // __THREADSAFEQUEUE_H__

