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
#include "TapeProcessor.h"
#include "AudioTape.h"


struct ReportingThreadInfo
{
	char m_serverHostname[256];
	int m_serverPort;
	int m_numTapesToSkip;
	bool m_queueFullError;
	char m_threadId[256];
	//ThreadSafeQueue<AudioTapeRef> m_audioTapeQueue;
	ThreadSafeQueue<MessageRef> m_messageQueue;
	ACE_Thread_Mutex m_mutex;
};
typedef boost::shared_ptr<ReportingThreadInfo> ReportingThreadInfoRef;


class ReportingThread
{
public:
	ReportingThread();
	void Run();

	CStdString m_serverHostname;
	int m_serverPort;

	CStdString m_threadId;
	ReportingThreadInfoRef m_myInfo;
private:
	bool IsSkip();
};

//=======================================================

class DLL_IMPORT_EXPORT_ORKBASE Reporting : public TapeProcessor
{
public:
	static void Initialize();
	static Reporting* Instance();

	CStdString __CDECL__ GetName();
	TapeProcessorRef __CDECL__ Instanciate();
	void __CDECL__ AddAudioTape(AudioTapeRef& audioTapeRef);
	void __CDECL__ AddTapeMessage(MessageRef& messageRef);
	void __CDECL__ SkipTapes(int number, CStdString trackingServer="");

	//static Reporting* GetInstance();
	static void ThreadHandler(void *args);
	static void ReportingThreadEntryPoint(void *args);

private:
	Reporting();
	//bool IsSkip();

	//static Reporting m_reportingSingleton;
	static TapeProcessorRef m_singleton;

	//ThreadSafeQueue<AudioTapeRef> m_audioTapeQueue;
	//bool m_queueFullError;
	//int numTapesToSkip;
	//ACE_Thread_Mutex m_mutex;
};

class DLL_IMPORT_EXPORT_ORKBASE ReportingSkipTapeMsg : public SyncMessage
{
public:
	ReportingSkipTapeMsg();

	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	int m_number;
	CStdString m_tracker;
};


#endif

