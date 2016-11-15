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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "ConfigManager.h"
#include "Reporting.h"
#include "LogManager.h"
#include "messages/Message.h"
#include "messages/TapeMsg.h"
#include "OrkClient.h"
#include "Daemon.h"
#include "CapturePluginProxy.h"
#include "ace/Thread_Manager.h"
#include "EventStreaming.h"
#include "messages/InitMsg.h"
#include "OrkTrack.h"
#include <vector>

struct ReportingThreadInfo
{
	int m_numTapesToSkip;
	CStdString m_threadId;
	//ThreadSafeQueue<AudioTapeRef> m_audioTapeQueue;
	ThreadSafeQueue<MessageRef> m_messageQueue;
	ACE_Thread_Mutex m_mutex;
	OrkTrack m_tracker;
};
typedef oreka::shared_ptr<ReportingThreadInfo> ReportingThreadInfoRef;


class ReportingThread
{
public:
	void Run();

	CStdString m_threadId;
	ReportingThreadInfoRef m_myInfo;
	OrkTrack m_tracker;
private:
	bool IsSkip();
};

TapeProcessorRef Reporting::m_singleton;
static std::map<CStdString, ReportingThreadInfoRef> s_reportingThreads;
static std::vector<ReportingThreadInfoRef> s_reportingThreadsVector;

void Reporting::Initialize()
{
	CStdString logMsg;

	if(m_singleton.get() == NULL)
	{
		m_singleton.reset(new Reporting());

		for (std::vector<OrkTrack>::const_iterator it = OrkTrack::getTrackers().begin(); it != OrkTrack::getTrackers().end(); it++) {
			ReportingThreadInfo *rtInfo = new ReportingThreadInfo();
			rtInfo->m_tracker = *it;

			if(!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(ReportingThreadEntryPoint), (void *)rtInfo))
			{
				FLOG_WARN(LOG.reporting,"[%s] failed to start reporting thread", rtInfo->m_tracker.ToString());
				delete rtInfo;
			}
		}

		TapeProcessorRegistry::instance()->RegisterTapeProcessor(m_singleton);
	}
}

void Reporting::ReportingThreadEntryPoint(void *args)
{
	ReportingThreadInfo *rtInfo = (ReportingThreadInfo *)args;

	ReportingThreadInfoRef rtInfoRef(new ReportingThreadInfo());
	rtInfoRef->m_tracker = rtInfo->m_tracker;
	rtInfoRef->m_numTapesToSkip = 0;
	rtInfoRef->m_messageQueue.setSize(CONFIG.m_reportingQueueSize);
	rtInfoRef->m_threadId.Format("%s,%d", rtInfoRef->m_tracker.m_hostname, rtInfoRef->m_tracker.m_port);

	ReportingThread myRunInfo;
	myRunInfo.m_tracker = rtInfo->m_tracker;
	myRunInfo.m_myInfo = rtInfoRef;
	myRunInfo.m_threadId.Format("%s,%d", myRunInfo.m_tracker.m_hostname, myRunInfo.m_tracker.m_port);

	s_reportingThreads.insert(std::make_pair(myRunInfo.m_tracker.m_hostname, rtInfoRef));
	s_reportingThreadsVector.push_back(rtInfoRef);
	delete rtInfo;

	myRunInfo.Run();
}

Reporting* Reporting::Instance()
{
	return (Reporting*)m_singleton.get();
}

Reporting::Reporting()
{
	m_readyToReport = false;
	//m_queueFullError = false;
	//numTapesToSkip = 0;
}

CStdString __CDECL__ Reporting::GetName()
{
	return "Reporting";
}

TapeProcessorRef  Reporting::Instanciate()
{
	return m_singleton;
}

void __CDECL__ Reporting::SkipTapes(int number, CStdString trackingServer)
{
	std::map<CStdString, ReportingThreadInfoRef>::iterator pair;

	if(!trackingServer.size())
	{
		if(s_reportingThreads.size() == 1)
		{
			pair = s_reportingThreads.begin();
			ReportingThreadInfoRef reportingThread = pair->second;
			{
				MutexSentinel sentinel(reportingThread->m_mutex);
				reportingThread->m_numTapesToSkip += number;

				return;
			}
		}
		else
		{
			return;
		}
	}

	pair = s_reportingThreads.find(trackingServer);
	if(pair != s_reportingThreads.end())
	{
		ReportingThreadInfoRef reportingThread = pair->second;
		{
			MutexSentinel sentinel(reportingThread->m_mutex);
			reportingThread->m_numTapesToSkip += number;
		}
	}
}

bool Reporting::AddMessage(MessageRef messageRef)
{
	CStdString logMsg;
	const CStdString msgAsSingleLineString = messageRef->SerializeSingleLine();

	IReportable* reportable = dynamic_cast<IReportable*>(messageRef.get());
	if (!reportable) {
		FLOG_WARN(LOG.reporting,"Discarding message that is not reportable: %s", msgAsSingleLineString);
		return false;
	}

	// According to the legend once upon a time
	// there was an unidentifed bug that was causing 
	// a crash when the messages were not cloned 
	// therefore we still clone the messages before 
	// passing them down the reporting pipeline 
	
	bool ret = true;
	std::vector<ReportingThreadInfoRef>::iterator it;
	
	for(it = s_reportingThreadsVector.begin(); it != s_reportingThreadsVector.end(); it++)
	{
		ReportingThreadInfoRef reportingThread = *it;
		MessageRef reportingMsgRef = reportable->Clone();

		ret = reportingThread->m_messageQueue.push(reportingMsgRef);
		FLOG_INFO(LOG.reporting,"[%s] %s: %s",reportingThread->m_tracker.ToString(),ret?"enqueued":"queue full, rejected",msgAsSingleLineString);
	}

	EventStreamingSingleton::instance()->AddMessage(reportable->Clone());
	return ret;
}

void Reporting::AddAudioTape(AudioTapeRef& audioTapeRef)
{
	audioTapeRef->m_isDoneProcessed = true; 	//to notify API caller the importing is good so far
	MessageRef msgRef;
	audioTapeRef->GetMessage(msgRef);
	AddMessage(msgRef);
}

void Reporting::ThreadHandler(void *args)
{
	return;
}

//=======================================================
#define REPORTING_SKIP_TAPE_CLASS "reportingskiptape"

ReportingSkipTapeMsg::ReportingSkipTapeMsg()
{
	m_number = 1;
}

void ReportingSkipTapeMsg::Define(Serializer* s)
{
	CStdString thisClass(REPORTING_SKIP_TAPE_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, thisClass, true);
	s->IntValue("num", (int&)m_number, false);
	s->StringValue("tracker", m_tracker, false);
}


CStdString ReportingSkipTapeMsg::GetClassName()
{
	return  CStdString(REPORTING_SKIP_TAPE_CLASS);
}

ObjectRef ReportingSkipTapeMsg::NewInstance()
{
	return ObjectRef(new ReportingSkipTapeMsg);
}

ObjectRef ReportingSkipTapeMsg::Process()
{
	bool success = true;
	CStdString logMsg;

	Reporting* reporting = Reporting::Instance();
	if(reporting)
	{
		reporting->SkipTapes(m_number, m_tracker);
	}

	SimpleResponseMsg* msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	msg->m_success = success;
	msg->m_comment = logMsg;
	return ref;
}


//=======================================================

bool ReportingThread::IsSkip()
{
	MutexSentinel sentinel(m_myInfo->m_mutex);

	if(m_myInfo->m_numTapesToSkip > 0)
	{
		m_myInfo->m_numTapesToSkip--;
		return true;
	}

	return false;
}

void ReportingThread::Run()
{
	CStdString logMsg;

	FLOG_INFO(LOG.reporting,"[%s] reporting thread started", m_tracker.ToString());

	bool stop = false;
	bool reportError = true;
	time_t reportErrorLastTime = 0;
	bool error = false;

	InitMsgRef initMsgRef(new InitMsg());
	if(CONFIG.m_hostnameReportFqdn == false)
	{
		char szLocalHostname[255];
		ACE_OS::hostname(szLocalHostname, sizeof(szLocalHostname));
		initMsgRef->m_hostname = szLocalHostname;
	}
	else
	{
		GetHostFqdn(initMsgRef->m_hostname, 255);
	}
	initMsgRef->m_name = CONFIG.m_serviceName;	
	initMsgRef->m_type = "A";
	initMsgRef->m_tcpPort = 59140;
	initMsgRef->m_contextPath = "/audio";
	initMsgRef->m_absolutePath = CONFIG.m_audioOutputPath;

	OrkHttpSingleLineClient c;
	SimpleResponseMsg response;

	do {
		bool conn = c.Execute((SyncMessage&)(*initMsgRef.get()), 
				              (AsyncMessage&)response, 
							  m_tracker.m_hostname, 
							  m_tracker.m_port,
							  m_tracker.m_servicename, 
							  CONFIG.m_clientTimeout);

		if (!response.m_success) {
			if (time(NULL) - reportErrorLastTime > 60) {
				FLOG_WARN(LOG.reporting,"[%s] init connection:%s success:false comment:%s ", m_tracker.ToString(), conn?"true":"false", response.m_comment);
				reportErrorLastTime = time(NULL);
			}
			ACE_OS::sleep(CONFIG.m_clientTimeout + 10);
		}
	} while (!response.m_success);

	FLOG_INFO(LOG.reporting,"[%s] init success:true comment:%s", m_tracker.ToString(),  response.m_comment);

	for(;stop == false;)
	{
		try
		{
			MessageRef msgRef = m_myInfo->m_messageQueue.pop();

			if(msgRef.get() == NULL)
			{
				if(Daemon::Singleton()->IsStopping())
				{
					stop = true;
				}
			}
			else
			{
				if ( msgRef->IsValid() == false ) {
					continue;
				}

				CStdString msgAsSingleLineString = msgRef->SerializeSingleLine();

				IReportable* reportable = dynamic_cast<IReportable*>(msgRef.get());
				if (!reportable) {
					FLOG_WARN(LOG.reporting,"[%s] Discarding message that is not reportable:%s", m_tracker.ToString(), msgAsSingleLineString);
					continue;
				}

				if( CONFIG.m_enableReporting)
				{
					FLOG_INFO(LOG.reporting,"[%s] sending: %s", m_tracker.ToString(), msgAsSingleLineString);

					OrkHttpSingleLineClient c;

					MessageRef tr = reportable->CreateResponse();

					bool success = false;

					while (!success && !IsSkip())
					{
						if (c.Execute((SyncMessage&)(*msgRef.get()), (AsyncMessage&)(*tr.get()), m_tracker.m_hostname, m_tracker.m_port, m_tracker.m_servicename, CONFIG.m_clientTimeout))
						{
							success = true;
							reportError = true; // reenable error reporting
							if(error)
							{
								error = false;
								FLOG_INFO(LOG.reporting,"[%s] successfully reconnected to the tracker after error", m_tracker.ToString());
							}

							reportable->HandleResponse(tr);
						}
						else
						{
							error = true;

							if( reportError || ((time(NULL) - reportErrorLastTime) > 60) )	// at worst, one error is reported every minute
							{
								reportError = false;
								reportErrorLastTime = time(NULL);
								FLOG_ERROR(LOG.reporting,"[%s] could not connect to tracker", m_tracker.ToString());
							}

							if (reportable->IsRealtime()) {
								success = true;		// No need to resend realtime messages
							}
							else
							{
								ACE_OS::sleep(CONFIG.m_clientRetryPeriodSec);	// Make sure orktrack is not flooded in case of a problem
							}
						}
					}
				}
			}
		}
		catch (CStdString& e)
		{
			FLOG_ERROR(LOG.reporting,"[%s] exception:", m_tracker.ToString(), e);
		}
	}
	FLOG_INFO(LOG.reporting,"[%s] gracefully terminating the reporting thread", m_tracker.ToString());
}

