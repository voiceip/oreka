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
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
//#include "winsock2.h"
#endif
#include "CapturePluginProxy.h"
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
	m_ProcessMetadataMsgFunction = NULL;

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
	OrkAprSubPool locPool;

	// Get the desired capture plugin from the config file, or else, use the first dll encountered.
	CStdString pluginDirectory = CONFIG.m_capturePluginPath + "/";
	CStdString pluginPath;
	apr_status_t ret;
	if (!CONFIG.m_capturePlugin.IsEmpty())
	{
		// A specific plugin was specified in the config file
		pluginPath = pluginDirectory + CONFIG.m_capturePlugin;
	}
	else
	{
		// No plugin specified, find the first one in the plugin directory
		apr_dir_t* dir;
		ret = apr_dir_open(&dir, (PCSTR)pluginDirectory, AprLp);
		if(ret != APR_SUCCESS)
		{
			LOG4CXX_ERROR(LOG.rootLog, CStdString("Capture plugin directory could not be found:" + pluginDirectory));
		}
		else
		{
			apr_finfo_t finfo;
    		apr_int32_t wanted = APR_FINFO_NAME | APR_FINFO_SIZE;
    		while((ret = apr_dir_read(&finfo, wanted, dir)) == APR_SUCCESS) 
			{
				CStdString fileName;
				fileName.Format("%s", finfo.name);
				if(fileName.find(".dll") != std::string::npos)
				{
					pluginPath = pluginDirectory + finfo.name;
					break;
				}
   			}
			apr_dir_close(dir);
		}
	}
	if (!pluginPath.IsEmpty())
	{
    	apr_status_t ret;
		char errstr[256];
		// dso handle needs to persist so it must me allocated using a pool that
		// will also persist. 
		ret = apr_dso_load(&m_dsoHandle, (PCSTR)pluginPath, OrkAprSingleton::GetInstance()->GetAprMp());
		if(ret != APR_SUCCESS)
		{
			apr_strerror(ret, errstr, sizeof(errstr));
			apr_dso_error(m_dsoHandle, errstr, sizeof(errstr));
#ifndef WIN32
			LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to load the following plugin: ") + pluginPath + " " + errstr + CStdString(", could be missing dependency, Try running in cmd box (orkaudio debug)"));
#else
			LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to load the following plugin: ") + pluginPath + CStdString(", could be missing dependency. Try running ldd on it."));
#endif
		}
		else
		{
			// Ok, the dll has been successfully loaded
			LOG4CXX_INFO(LOG.rootLog, CStdString("Loaded plugin: ") + pluginPath);

			RegisterCallBacksFunction registerCallBacks;
			if((ret = apr_dso_sym((apr_dso_handle_sym_t*)&registerCallBacks, m_dsoHandle, "RegisterCallBacks")) != APR_SUCCESS)
			{
				LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find registerCallBacksFunction"));
			}
			registerCallBacks(AudioChunkCallBack, CaptureEventCallBack, OrkLogManager::Instance());

			ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_configureFunction, m_dsoHandle, "Configure");
			if (ret == APR_SUCCESS)
			{
				ConfigManager::Instance()->AddConfigureFunction(m_configureFunction);

				ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_initializeFunction, m_dsoHandle, "Initialize");
				if (ret == APR_SUCCESS)
				{
					m_initializeFunction();
					ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_runFunction, m_dsoHandle, "Run");
					if (ret == APR_SUCCESS)
					{
						ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_startCaptureFunction, m_dsoHandle, "StartCapture");
						if (ret == APR_SUCCESS)
						{
							ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_stopCaptureFunction, m_dsoHandle, "StopCapture");
							if (ret == APR_SUCCESS)
							{
								ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_pauseCaptureFunction, m_dsoHandle, "PauseCapture");
								if(ret == APR_SUCCESS)
								{
									ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_setOnHoldFunction, m_dsoHandle, "SetOnHold");
									if(ret == APR_SUCCESS)
									{
										ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_setOffHoldFunction, m_dsoHandle, "SetOffHold");
										if(ret == APR_SUCCESS)
										{
											ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_GetConnectionStatusFunction, m_dsoHandle, "GetConnectionStatus");
											if(ret == APR_SUCCESS)
											{
												ret = apr_dso_sym((apr_dso_handle_sym_t*)&m_ProcessMetadataMsgFunction, m_dsoHandle, "ProcessMetadataMsg");
												if(ret == APR_SUCCESS)
												{
													m_loaded = true;
												}
												else
												{
													LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find ProcessMetadataMsg function in ") + pluginPath);
												}												
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
	try{
		std::thread handler(m_runFunction);
		handler.detach();
	} catch(const std::exception &ex){
		CStdString logMsg;
		logMsg.Format("Failed to create capture thread reason:%s",  ex.what());
		LOG4CXX_ERROR(LOG.rootLog, logMsg);	
	}
}

void CapturePluginProxy::Shutdown()
{
	ShutdownFunction shutdownFunction;
	apr_status_t ret;
	ret = apr_dso_sym((apr_dso_handle_sym_t*)&shutdownFunction, m_dsoHandle, "Shutdown");
	if (ret == APR_SUCCESS)
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

void CapturePluginProxy::ProcessMetadataMsg(SyncMessage* msg)
{
	if(m_loaded)
	{
		m_ProcessMetadataMsgFunction(msg);
	}
	else
	{
		throw(CStdString("ProcessMetadataMsg: plugin not yet loaded"));
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



