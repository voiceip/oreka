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


class DLL_IMPORT_EXPORT_ORKBASE Reporting : public TapeProcessor
{
public:
	static void Initialize();
	static Reporting* Instance();

	CStdString __CDECL__ GetName();
	TapeProcessorRef __CDECL__ Instanciate();
	void __CDECL__ AddAudioTape(AudioTapeRef& audioTapeRef);
	bool __CDECL__ AddMessage(MessageRef messageRef);
	void __CDECL__ SkipTapes(int number, CStdString trackingServer="");

	//static Reporting* GetInstance();
	static void ThreadHandler(void *args);
	static void ReportingThreadEntryPoint(void *args);

private:
	Reporting();
	bool m_readyToReport;
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

