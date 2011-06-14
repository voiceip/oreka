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

#include "CapturePluginProxy.h"
#include "ace/OS_NS_dirent.h"
#include "ace/OS_NS_string.h"
#include "ace/Thread_Manager.h"
#include "ConfigManager.h"
#include "CapturePort.h"

CapturePluginProxy* CapturePluginProxy::m_singleton;	

CapturePluginProxy::CapturePluginProxy()
{
	m_configureFunction = NULL;
	m_registerCallBacksFunction = NULL;
	m_initializeFunction = NULL;
	m_runFunction = NULL;
	m_startCaptureFunction = NULL;
	m_stopCaptureFunction = NULL;
	m_GetConnectionStatusFunction = NULL;

	m_loaded = false;
}

CapturePluginProxy* CapturePluginProxy::Singleton()
{
	return m_singleton;
}

bool CapturePluginProxy::Initialize()
{
	m_singleton = new CapturePluginProxy();
	return m_singleton->Init();
}

bool CapturePluginProxy::Init()
{
	// Get the desired capture plugin from the config file, or else, use the first dll encountered.
	CStdString pluginDirectory = CONFIG.m_capturePluginPath + "/";
	CStdString pluginPath;
	if (!CONFIG.m_capturePlugin.IsEmpty())
	{
		// A specific plugin was specified in the config file
		pluginPath = pluginDirectory + CONFIG.m_capturePlugin;
	}
	else
	{
		// No plugin specified, find the first one in the plugin directory
		ACE_DIR* dir = ACE_OS::opendir((PCSTR)pluginDirectory);
		if (!dir)
		{
			LOG4CXX_ERROR(LOG.rootLog, CStdString("Capture plugin directory could not be found:" + pluginDirectory));
		}
		else
		{
			dirent* dirEntry = NULL;
			bool found = false;
			bool done = false;
			while(!found && !done)
			{	
				dirEntry = ACE_OS::readdir(dir);
				if(dirEntry)
				{
					if (ACE_OS::strstr(dirEntry->d_name, ".dll"))
					{
						found = true;
						done = true;
						pluginPath = pluginDirectory + dirEntry->d_name;
					}
				}
				else
				{
					done = true;
				}
			}
			ACE_OS::closedir(dir);
		}
	}
	if (!pluginPath.IsEmpty())
	{
		m_dll.open((PCSTR)pluginPath);
		ACE_TCHAR* error = m_dll.error();
#ifndef WIN32
		char *errorstr;
#endif
		if(error)
		{
#ifndef WIN32
			errorstr = dlerror();
			CStdString errorcstds(errorstr);
			LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to load the following plugin: ") + pluginPath + " " + errorcstds + CStdString(", could be missing dependency, Try running in cmd box (orkaudio debug)"));
#else
			LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to load the following plugin: ") + pluginPath + CStdString(", could be missing dependency. Try running ldd on it."));
#endif
		}
		else
		{
			// Ok, the dll has been successfully loaded
			LOG4CXX_INFO(LOG.rootLog, CStdString("Loaded plugin: ") + pluginPath);

			RegisterCallBacksFunction registerCallBacks;
			registerCallBacks = (RegisterCallBacksFunction)m_dll.symbol("RegisterCallBacks");
			registerCallBacks(AudioChunkCallBack, CaptureEventCallBack, OrkLogManager::Instance());

			m_configureFunction = (ConfigureFunction)m_dll.symbol("Configure");
			if (m_configureFunction)
			{
				ConfigManager::Instance()->AddConfigureFunction(m_configureFunction);

				m_initializeFunction = (InitializeFunction)m_dll.symbol("Initialize");
				if (m_initializeFunction)
				{
					m_initializeFunction();

					m_runFunction = (RunFunction)m_dll.symbol("Run");
					if (m_runFunction)
					{
						m_startCaptureFunction = (StartCaptureFunction)m_dll.symbol("StartCapture");
						if (m_startCaptureFunction)
						{
							m_stopCaptureFunction = (StopCaptureFunction)m_dll.symbol("StopCapture");
							if (m_stopCaptureFunction)
							{
								m_pauseCaptureFunction = (PauseCaptureFunction)m_dll.symbol("PauseCapture");
								if(m_pauseCaptureFunction)
								{
									m_setOnHoldFunction = (SetOnHoldFunction)m_dll.symbol("SetOnHold");
									if(m_setOnHoldFunction)
									{
										m_setOffHoldFunction = (SetOffHoldFunction)m_dll.symbol("SetOffHold");
										if(m_setOffHoldFunction)
										{
											m_GetConnectionStatusFunction = (GetConnectionStatusFunction)m_dll.symbol("GetConnectionStatus");
											if(m_GetConnectionStatusFunction)
											{
												m_loaded = true;
											}
											else
											{
												LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find GetConnectionStatus function in ") + pluginPath);
											}
										}
										else
										{
											LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find SetOffHold function in ") + pluginPath);
										}
									}
									else
									{
										LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find SetOnHold function in ") + pluginPath);
									}
								}
								else
								{
									LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find PauseCapture function in ") + pluginPath);
								}
							}
							else
							{
								LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find StopCapture function in ") + pluginPath);
							}
						}
						else
						{
							LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find StartCapture function in ") + pluginPath);
						}
					}
					else
					{
						LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find Run function in ") + pluginPath);
					}
				}
				else
				{
					LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find Initialize function in ") + pluginPath);
				}
			}
			else
			{
				LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find Configure function in ") + pluginPath);
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to find any capture plugin in: ") + pluginDirectory);
	}

	return m_loaded;
}

void CapturePluginProxy::Run()
{
	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(m_runFunction)))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create capture thread"));
	}
}

void CapturePluginProxy::Shutdown()
{
	ShutdownFunction shutdownFunction = (ShutdownFunction)m_dll.symbol("Shutdown");
	if (shutdownFunction)
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Shutting down"));
		shutdownFunction();
	}
	else
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Could not find DLL Shutdown function"));
	}
}

void CapturePluginProxy::StartCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid, CStdString& side)
{
	if(m_loaded)
	{
		m_startCaptureFunction(party, orkuid, nativecallid, side);
	}
	else
	{
		throw(CStdString("StartCapture: Capture plugin not yet loaded"));
	}
}

void CapturePluginProxy::StopCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid, CStdString& qos)
{
	if(m_loaded)
	{
		m_stopCaptureFunction(party, orkuid, nativecallid, qos);
	}
	else
	{
		throw(CStdString("StopCapture: Capture plugin not yet loaded"));
	}
}

void CapturePluginProxy::SetOnHold(CStdString& party, CStdString& orkuid)
{
	if(m_loaded)
	{
		m_setOnHoldFunction(party, orkuid);
	}
	else
	{
		throw(CStdString("SetOnHold: Capture plugin not yet loaded"));
	}
}

void CapturePluginProxy::SetOffHold(CStdString& party, CStdString& orkuid)
{
	if(m_loaded)
	{
		m_setOffHoldFunction(party, orkuid);
	}
	else
	{
		throw(CStdString("SetOffHold: Capture plugin not yet loaded"));
	}
}

void CapturePluginProxy::PauseCapture(CStdString& party, CStdString& orkuid, CStdString& nativecallid)
{
	if(m_loaded)
	{
		m_pauseCaptureFunction(party, orkuid, nativecallid);
	}
	else
	{
		throw(CStdString("PauseCapture: Capture plugin not yet loaded"));
	}
}

void CapturePluginProxy::GetConnectionStatus(CStdString& msg)
{
	if(m_loaded)
	{
		m_GetConnectionStatusFunction(msg);
	}
	else
	{
		throw(CStdString("Check Health: plugin not yet loaded"));
	}
}

void __CDECL__  CapturePluginProxy::AudioChunkCallBack(AudioChunkRef chunkRef, CStdString& capturePort)
{
	// find the right port and give it the audio chunk
	CapturePortRef portRef = CapturePortsSingleton::instance()->AddAndReturnPort(capturePort);
	portRef->AddAudioChunk(chunkRef);
}

void __CDECL__ CapturePluginProxy::CaptureEventCallBack(CaptureEventRef eventRef, CStdString& capturePort)
{
	if(CONFIG.m_vad || CONFIG.m_audioSegmentation)
	{
		if (eventRef->m_type == CaptureEvent::EtStart || eventRef->m_type == CaptureEvent::EtStop)
		{
			// find the right port and give it the event
			// If this is EtStop, we clear the event cache
			CapturePortRef portRef = CapturePortsSingleton::instance()->AddAndReturnPort(capturePort);
			if(eventRef->m_type == CaptureEvent::EtStop)
			{
				portRef->ClearEventQueue();
				portRef->AddCaptureEvent(eventRef);
			}
		}
		else
		{
			// find the right port and give it the event
			CapturePortRef portRef = CapturePortsSingleton::instance()->AddAndReturnPort(capturePort);
			portRef->QueueCaptureEvent(eventRef);
			portRef->AddCaptureEvent(eventRef);
		}
	}
	else
	{
		// find the right port and give it the event
		CapturePortRef portRef = CapturePortsSingleton::instance()->AddAndReturnPort(capturePort);
		portRef->AddCaptureEvent(eventRef);
	}
}



