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
#include "LiveStreamServerProxy.h"
#include "ConfigManager.h"
#include "CapturePort.h"
#include <set>

LiveStreamServerProxy *LiveStreamServerProxy::m_singleton;

LiveStreamServerProxy::LiveStreamServerProxy()
{
	m_runFunction = NULL;
	m_startStreamFunction = NULL;
	m_endStreamFunction = NULL;
	m_getStreamFunction = NULL;
	m_loaded = false;
}

LiveStreamServerProxy *LiveStreamServerProxy::Singleton()
{
	return m_singleton;
}

bool LiveStreamServerProxy::Initialize()
{
	m_singleton = new LiveStreamServerProxy();
	return m_singleton->Init();
}

bool LiveStreamServerProxy::Init()
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
		apr_dir_t *dir;
		ret = apr_dir_open(&dir, (PCSTR)pluginDirectory, AprLp);
		if (ret != APR_SUCCESS)
		{
			LOG4CXX_ERROR(LOG.rootLog, CStdString("Capture plugin directory could not be found:" + pluginDirectory));
		}
		else
		{
			apr_finfo_t finfo;
			apr_int32_t wanted = APR_FINFO_NAME | APR_FINFO_SIZE;
			while ((ret = apr_dir_read(&finfo, wanted, dir)) == APR_SUCCESS)
			{
				CStdString fileName;
				fileName.Format("%s", finfo.name);
				if (fileName.find(".dll") != std::string::npos)
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
		if (ret != APR_SUCCESS)
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
			ret = apr_dso_sym((apr_dso_handle_sym_t *)&m_runFunction, m_dsoHandle, "Run");
			if (ret == APR_SUCCESS)
			{
				ret = apr_dso_sym((apr_dso_handle_sym_t *)&m_startStreamFunction, m_dsoHandle, "StartStream");
				if (ret == APR_SUCCESS)
				{
					ret = apr_dso_sym((apr_dso_handle_sym_t *)&m_endStreamFunction, m_dsoHandle, "EndStream");
					if (ret == APR_SUCCESS)
					{
						ret = apr_dso_sym((apr_dso_handle_sym_t *)&m_getStreamFunction, m_dsoHandle, "GetStream");
						if (ret == APR_SUCCESS)
						{
							m_loaded = true;
						}
						else
						{
							LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find GetStream function in ") + pluginPath);
						}
					}
					else
					{
						LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find EndStream function in ") + pluginPath);
					}
				}
				else
				{
					LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find StartStream function in ") + pluginPath);
				}
			}
			else
			{
				LOG4CXX_ERROR(LOG.rootLog, CStdString("Could not find Run function in ") + pluginPath);
			}
		}
	}
	else
	{
		LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to find any capture plugin in: ") + pluginDirectory);
	}

	return m_loaded;
}

void LiveStreamServerProxy::Run()
{
	try
	{
		std::thread handler(m_runFunction);
		handler.detach();
	}
	catch (const std::exception &ex)
	{
		CStdString logMsg;
		logMsg.Format("Failed to create capture thread reason:%s", ex.what());
		LOG4CXX_ERROR(LOG.rootLog, logMsg);
	}
}

void LiveStreamServerProxy::Shutdown()
{
	ShutdownFunction shutdownFunction;
	apr_status_t ret;
	ret = apr_dso_sym((apr_dso_handle_sym_t *)&shutdownFunction, m_dsoHandle, "Shutdown");
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

void LiveStreamServerProxy::StartStream(CStdString &party, CStdString &orkuid, CStdString &nativecallid)
{
	if (m_loaded)
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Shutting down"));
		m_startStreamFunction(party, orkuid, nativecallid);
	}
	else
	{
		throw(CStdString("StartStream: Capture plugin not yet loaded"));
	}
}

void LiveStreamServerProxy::EndStream(CStdString &party, CStdString &orkuid, CStdString &nativecallid)
{
	if (m_loaded)
	{
		m_endStreamFunction(party, orkuid, nativecallid);
	}
	else
	{
		throw(CStdString("EndStream: Capture plugin not yet loaded"));
	}
}

std::set<std::string> LiveStreamServerProxy::GetStream()
{
	if (m_loaded)
	{
		return m_getStreamFunction();
	}
	else
	{
		throw(CStdString("GetStream: Capture plugin not yet loaded"));
	}
}
