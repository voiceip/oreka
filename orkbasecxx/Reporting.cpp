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

#ifdef WIN32
# ifndef snprintf
#  define snprintf _snprintf
# endif
#endif

TapeProcessorRef Reporting::m_singleton;
static std::map<CStdString, ReportingThreadInfoRef> s_reportingThreads;

void Reporting::Initialize()
{
	CStdString logMsg;

	if(m_singleton.get() == NULL)
	{
		m_singleton.reset(new Reporting());

		for(std::list<CStdString>::iterator it = CONFIG.m_trackerHostname.begin(); it != CONFIG.m_trackerHostname.end(); it++)
		{
			CStdString trackerHostname = *it;
			ReportingThreadInfo *rtInfo = (ReportingThreadInfo *)malloc(sizeof(ReportingThreadInfo));

			memset(rtInfo, 0, sizeof(ReportingThreadInfo));
			snprintf(rtInfo->m_serverHostname, sizeof(rtInfo->m_serverHostname), "%s", trackerHostname.c_str());
			rtInfo->m_serverPort = CONFIG.m_trackerTcpPort;

			if(!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(ReportingThreadEntryPoint), (void *)rtInfo))
			{
				logMsg.Format("Failed to start thread reporting to %s,%d", rtInfo->m_serverHostname, rtInfo->m_serverPort);
				LOG4CXX_WARN(LOG.reportingLog, logMsg);
				free(rtInfo);
			}
		}

		TapeProcessorRegistry::instance()->RegisterTapeProcessor(m_singleton);
	}
}

void Reporting::ReportingThreadEntryPoint(void *args)
{
	ReportingThreadInfo *rtInfo = (ReportingThreadInfo *)args;
	ReportingThreadInfoRef rtInfoRef(new ReportingThreadInfo());
	ReportingThread myRunInfo;

	myRunInfo.m_serverHostname.Format("%s", rtInfo->m_serverHostname);
	myRunInfo.m_serverPort = rtInfo->m_serverPort;
	myRunInfo.m_threadId.Format("%s,%d", myRunInfo.m_serverHostname, myRunInfo.m_serverPort);

	snprintf(rtInfoRef->m_serverHostname, sizeof(rtInfoRef->m_serverHostname), "%s", rtInfo->m_serverHostname);
	rtInfoRef->m_serverPort = rtInfo->m_serverPort;
	rtInfoRef->m_numTapesToSkip = 0;
	rtInfoRef->m_queueFullError = false;
	snprintf(rtInfoRef->m_threadId, sizeof(rtInfoRef->m_threadId), "%s,%d", rtInfoRef->m_serverHostname, rtInfoRef->m_serverPort);
	myRunInfo.m_myInfo = rtInfoRef;

	s_reportingThreads.insert(std::make_pair(myRunInfo.m_serverHostname, rtInfoRef));
	free(rtInfo);

	myRunInfo.Run();
}

Reporting* Reporting::Instance()
{
	return (Reporting*)m_singleton.get();
}

Reporting::Reporting()
{
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


void Reporting::AddAudioTape(AudioTapeRef& audioTapeRef)
{
	std::map<CStdString, ReportingThreadInfoRef>::iterator pair;
	CStdString logMsg;

	for(pair = s_reportingThreads.begin(); pair != s_reportingThreads.end(); pair++)
	{
		ReportingThreadInfoRef reportingThread = pair->second;

		if(reportingThread->m_audioTapeQueue.push(audioTapeRef))
		{
			reportingThread->m_queueFullError = false;
			logMsg.Format("[%s] added audiotape to queue: %s", reportingThread->m_threadId, audioTapeRef->GetIdentifier());
			LOG4CXX_INFO(LOG.reportingLog, logMsg);
		}
		else
		{
			if(reportingThread->m_queueFullError == false)
			{
				logMsg.Format("[%s] queue full, could not add audiotape %s", reportingThread->m_threadId, audioTapeRef->GetIdentifier());
				LOG4CXX_WARN(LOG.reportingLog, logMsg);
				reportingThread->m_queueFullError = true;
			}
		}
	}
}

void Reporting::ThreadHandler(void *args)
{
	int humptyDumptySatOnAWall;

	humptyDumptySatOnAWall = 0;

	return;
}

#if 0
void Reporting::ThreadHandler(void *args)
{
	CStdString processorName("Reporting");
	TapeProcessorRef reporting = TapeProcessorRegistry::instance()->GetNewTapeProcessor(processorName);
	if(reporting.get() == NULL)
	{
		LOG4CXX_ERROR(LOG.reportingLog, "Could not instanciate Reporting");
		return;
	}
	Reporting* pReporting = (Reporting*)(reporting->Instanciate().get());

	bool stop = false;
	bool reportError = true;
	time_t reportErrorLastTime = 0;
	bool error = false;

	for(;stop == false;)
	{
		try
		{
			AudioTapeRef audioTapeRef = pReporting->m_audioTapeQueue.pop();

			if(audioTapeRef.get() == NULL)
			{
				if(Daemon::Singleton()->IsStopping())
				{
					stop = true;
				}
			}
			else
			{

				MessageRef msgRef;
				audioTapeRef->GetMessage(msgRef);
				TapeMsg* ptapeMsg = (TapeMsg*)msgRef.get();
				//bool startMsg = false;
				bool realtimeMessage = false;

				if(msgRef.get() && CONFIG.m_enableReporting)
				{
					//if(ptapeMsg->m_stage.Equals("START"))
					//{
					//	startMsg = true;
					//}
					if(ptapeMsg->m_stage.Equals("start") || ptapeMsg->m_stage.Equals("stop"))
					{
						realtimeMessage = true;
					}

					CStdString msgAsSingleLineString = msgRef->SerializeSingleLine();
					LOG4CXX_INFO(LOG.reportingLog, msgAsSingleLineString);

					OrkHttpSingleLineClient c;
					TapeResponseRef tr(new TapeResponse());
					audioTapeRef->m_tapeResponse = tr;

					bool success = false;

					while (!success && !pReporting->IsSkip())
					{
						if (c.Execute((SyncMessage&)(*msgRef.get()), (AsyncMessage&)(*tr.get()), CONFIG.m_trackerHostname, CONFIG.m_trackerTcpPort, CONFIG.m_trackerServicename, CONFIG.m_clientTimeout))
						{
							success = true;
							reportError = true; // reenable error reporting
							if(error)
							{
								error = false;
								LOG4CXX_ERROR(LOG.reportingLog, CStdString("Orktrack successfully contacted"));
							}

							if(tr->m_deleteTape)
							{
								CStdString tapeFilename = audioTapeRef->GetFilename();

								CStdString absoluteFilename = CONFIG.m_audioOutputPath + "/" + tapeFilename;
								if (ACE_OS::unlink((PCSTR)absoluteFilename) == 0)
								{
									LOG4CXX_INFO(LOG.reportingLog, "Deleted tape: " + tapeFilename);
								}
								else
								{
									LOG4CXX_DEBUG(LOG.reportingLog, "Could not delete tape: " + tapeFilename);
								}

							}
							else 
							{
								// Tape is wanted
								if(CONFIG.m_lookBackRecording == false && CONFIG.m_allowAutomaticRecording && ptapeMsg->m_stage.Equals("start"))
								{
									CapturePluginProxy::Singleton()->StartCapture(ptapeMsg->m_localParty);
									CapturePluginProxy::Singleton()->StartCapture(ptapeMsg->m_remoteParty);
								}
							}
							//else
							//{
							//	if(!startMsg)
							//	{
							//		// Pass the tape to the next processor
							//		pReporting->Runsftp NextProcessor(audioTapeRef);
							//	}
							//}
						}
						else
						{
							error = true;

							if( reportError || ((time(NULL) - reportErrorLastTime) > 60) )	// at worst, one error is reported every minute
							{
								reportError = false;
								reportErrorLastTime = time(NULL);
								LOG4CXX_ERROR(LOG.reportingLog, CStdString("Could not contact orktrack"));
							}
							if(realtimeMessage)
							{
								success = true;		// No need to resend realtime messages
							}
							else
							{
								ACE_OS::sleep(2);	// Make sure orktrack is not flooded in case of a problem
							}
						}
					}
				}
			}
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.reportingLog, CStdString("Exception: ") + e);
		}
	}
	LOG4CXX_INFO(LOG.reportingLog, CStdString("Exiting thread"));
}
#endif

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
ReportingThread::ReportingThread()
{
	m_serverPort = 0;
	m_serverHostname = "0.0.0.0";
}

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

	logMsg.Format("Thread reporting to %s started", m_threadId);
	LOG4CXX_INFO(LOG.reportingLog, logMsg);

	bool stop = false;
	bool reportError = true;
	time_t reportErrorLastTime = 0;
	bool error = false;

	for(;stop == false;)
	{
		try
		{
			AudioTapeRef audioTapeRef = m_myInfo->m_audioTapeQueue.pop();

			if(audioTapeRef.get() == NULL)
			{
				if(Daemon::Singleton()->IsStopping())
				{
					stop = true;
				}
			}
			else
			{

				MessageRef msgRef;
				audioTapeRef->GetMessage(msgRef);
				TapeMsg* ptapeMsg = (TapeMsg*)msgRef.get();
				//bool startMsg = false;
				bool realtimeMessage = false;

				if(msgRef.get() && CONFIG.m_enableReporting)
				{
					//if(ptapeMsg->m_stage.Equals("START"))
					//{
					//	startMsg = true;
					//}
					if(ptapeMsg->m_stage.Equals("start") || ptapeMsg->m_stage.Equals("stop"))
					{
						realtimeMessage = true;
					}

					CStdString msgAsSingleLineString = msgRef->SerializeSingleLine();
					LOG4CXX_INFO(LOG.reportingLog, "[" + m_threadId + "] " + msgAsSingleLineString);

					OrkHttpSingleLineClient c;
					TapeResponseRef tr(new TapeResponse());
					audioTapeRef->m_tapeResponse = tr;

					bool success = false;

					while (!success && !IsSkip())
					{
						if (c.Execute((SyncMessage&)(*msgRef.get()), (AsyncMessage&)(*tr.get()), m_serverHostname, m_serverPort, CONFIG.m_trackerServicename, CONFIG.m_clientTimeout))
						{
							success = true;
							reportError = true; // reenable error reporting
							if(error)
							{
								error = false;
								LOG4CXX_ERROR(LOG.reportingLog, "[" + m_threadId + "] " + CStdString("Orktrack successfully contacted"));
							}

							if(tr->m_deleteTape)
							{
								CStdString tapeFilename = audioTapeRef->GetFilename();

								CStdString absoluteFilename = CONFIG.m_audioOutputPath + "/" + tapeFilename;
								if (ACE_OS::unlink((PCSTR)absoluteFilename) == 0)
								{
									LOG4CXX_INFO(LOG.reportingLog, "[" + m_threadId + "] " + "Deleted tape: " + tapeFilename);
								}
								else
								{
									LOG4CXX_DEBUG(LOG.reportingLog, "[" + m_threadId + "] " + "Could not delete tape: " + tapeFilename);
								}

							}
							else 
							{
								// Tape is wanted
								if(CONFIG.m_lookBackRecording == false && CONFIG.m_allowAutomaticRecording && ptapeMsg->m_stage.Equals("start"))
								{
									CapturePluginProxy::Singleton()->StartCapture(ptapeMsg->m_localParty);
									CapturePluginProxy::Singleton()->StartCapture(ptapeMsg->m_remoteParty);
								}
							}
							//else
							//{
							//	if(!startMsg)
							//	{
							//		// Pass the tape to the next processor
							//		pReporting->Runsftp NextProcessor(audioTapeRef);
							//	}
							//}
						}
						else
						{
							error = true;

							if( reportError || ((time(NULL) - reportErrorLastTime) > 60) )	// at worst, one error is reported every minute
							{
								reportError = false;
								reportErrorLastTime = time(NULL);
								LOG4CXX_ERROR(LOG.reportingLog, "[" + m_threadId + "] " + CStdString("Could not contact orktrack"));
							}
							if(realtimeMessage)
							{
								success = true;		// No need to resend realtime messages
							}
							else
							{
								ACE_OS::sleep(2);	// Make sure orktrack is not flooded in case of a problem
							}
						}
					}
				}
			}
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.reportingLog, "[" + m_threadId + "] " + CStdString("Exception: ") + e);
		}
	}
	LOG4CXX_INFO(LOG.reportingLog, "[" + m_threadId + "] " + CStdString("Exiting thread"));
}

