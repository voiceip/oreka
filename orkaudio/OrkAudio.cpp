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


#include "stdio.h"

#include "MultiThreadedServer.h"
#include "ace/Thread_Manager.h"
#include "ace/DLL.h"
#include "ace/OS_NS_dirent.h"
#include "ace/OS_NS_string.h"
#include "OrkAudio.h"
#include "Utils.h"
#include "messages/TapeMsg.h"
#include "messages/PingMsg.h"
#include "messages/DeleteTapeMsg.h"
#include "messages/CaptureMsg.h"
#include "messages/TestMsg.h"
#include "Config.h"
#include "LogManager.h"
#include "ImmediateProcessing.h"
#include "BatchProcessing.h"
#include "Reporting.h"
#include "ConfigManager.h"
#include "Daemon.h"
#include "ObjectFactory.h"
#include "CapturePluginProxy.h"
#include "AudioCapturePlugin.h"
#include "Filter.h"
#include <list>


static volatile bool serviceStop = false;

void StopHandler()
{
	serviceStop = true;
}

#ifdef WIN32
long ExceptionFilter(struct _EXCEPTION_POINTERS *ptr)
{
	return EXCEPTION_CONTINUE_SEARCH;
}
#endif

void LoadPlugins(std::list<ACE_DLL>& pluginDlls)
{
#ifdef WIN32
	CStdString pluginDirectory = "./plugins/";
#else
	CStdString pluginDirectory = "/usr/lib/orkaudio/plugins/";
#endif
	CStdString pluginPath;
	ACE_DLL dll;

	ACE_DIR* dir = ACE_OS::opendir((PCSTR)pluginDirectory);
	if (!dir)
	{
		LOG4CXX_ERROR(LOG.rootLog, CStdString("Plugin directory could not be found:" + pluginDirectory));
	}
	else
	{
		dirent* dirEntry = NULL;
		while(dirEntry = ACE_OS::readdir(dir))
		{	
			if (ACE_OS::strstr(dirEntry->d_name, ".dll"))
			{
				pluginPath = pluginDirectory + dirEntry->d_name;
				dll.open((PCSTR)pluginPath);
				ACE_TCHAR* error = dll.error();
				if(error)
				{
					LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to load plugin: ") + pluginPath);
				}
				else
				{
					LOG4CXX_INFO(LOG.rootLog, CStdString("Loaded plugin: ") + pluginPath);

					InitializeFunction initfunction;
					initfunction = (InitializeFunction)dll.symbol("OrkInitialize");

					if (initfunction)
					{
						initfunction();
						pluginDlls.push_back(dll);
					}
					else
					{
						LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to initialize plugin: ") + pluginPath);
					}
				}
			}
		}
		ACE_OS::closedir(dir);
	}
}

void MainThread()
{
	OrkLogManagerSingleton::instance()->Initialize();
	LOG4CXX_INFO(LOG.rootLog, CStdString("\n\nOrkAudio service starting\n"));

	// Initialize object factory and register existing objects
	ObjectFactorySingleton::instance()->Initialize();

	ObjectRef objRef;
	objRef.reset(new PingMsg);
	ObjectFactorySingleton::instance()->RegisterObject(objRef);
	objRef.reset(new TapeMsg);
	ObjectFactorySingleton::instance()->RegisterObject(objRef);
	objRef.reset(new SimpleResponseMsg);
	ObjectFactorySingleton::instance()->RegisterObject(objRef);
	objRef.reset(new DeleteTapeMsg);
	ObjectFactorySingleton::instance()->RegisterObject(objRef);
	objRef.reset(new CaptureMsg);
	ObjectFactorySingleton::instance()->RegisterObject(objRef);
	objRef.reset(new TestMsg);
	ObjectFactorySingleton::instance()->RegisterObject(objRef);

	ConfigManagerSingleton::instance()->Initialize();

	// Register in-built filters
	FilterRef filter(new AlawToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new UlawToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);

	std::list<ACE_DLL> pluginDlls;
	LoadPlugins(pluginDlls);

	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(ImmediateProcessing::ThreadHandler)))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create immediate processing thread"));
	}
	if(CONFIG.m_storageAudioFormat != AudioTape::FfNative)
	{
		// storage format is not native, which means we need batch workers to compress to wanted format 
		if (!ACE_Thread_Manager::instance()->spawn_n(CONFIG.m_numBatchThreads, ACE_THR_FUNC(BatchProcessing::ThreadHandler)))
		{
			LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create batch processing thread"));
		}
	}
	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(Reporting::ThreadHandler)))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create reporting thread"));
	}
	// Create command line server on port 10000
	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(CommandLineServer::run), (void *)10000))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create command line server"));
	}

	// Create Http server on port 20000
	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(HttpServer::run), (void *)20000))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create Http server"));
	}

	if(CapturePluginProxySingleton::instance()->Initialize())
	{
		CapturePluginProxySingleton::instance()->Run();
	}

	//ACE_Thread_Manager::instance ()->wait ();
	while(!DaemonSingleton::instance()->IsStopping())
	{
		ACE_OS::sleep(1);
	}

	CapturePluginProxySingleton::instance()->Shutdown();

	// Wait that all ACE threads have returned
	//ACE_Thread_Manager::instance ()->wait ();
	ACE_OS::sleep(2);
	
	//***** This is to avoid an exception when NT service exiting
	//***** Need to find out the real problem and fix
#ifdef WIN32
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ExceptionFilter);
#endif
	//*****

	LOG4CXX_INFO(LOG.rootLog, CStdString("Service stopped"));
	OrkLogManagerSingleton::instance()->Shutdown();
}


int main(int argc, char* argv[])
{
	// figure out service name
	CStdString program(argv[0]);
	CStdString serviceNameWithExtension = BaseName(program);
	CStdString serviceName = StripFileExtension(serviceNameWithExtension);
	if (serviceName.IsEmpty())
	{
		printf("Error: Could not determine service name.\n");
		return -1;
	}

	DaemonSingleton::instance()->Initialize(serviceName, MainThread, StopHandler);
	CStdString argument = argv[1];

	if (argc>1)
	{
		if (argument.CompareNoCase("debug") == 0)
		{
			MainThread();
		}
		else if (argument.CompareNoCase("install") == 0)
		{
			DaemonSingleton::instance()->Install();
		}
		else if  (argument.CompareNoCase("uninstall") == 0)
		{
			DaemonSingleton::instance()->Uninstall();
		}
		else
		{
#ifdef WIN32
	printf("Argument incorrect. Possibilies are:\n\tinstall:\t\tinstall NT service\n\tuninstall:\t\tuninstall NT service\n\n");
#else
	printf("Argument incorrect. Possibilies are:\n\tdebug:\trun attached to tty\n\n");
#endif
		}
	}
	else
	{
		// No arguments, launch the daemon
		printf("Starting orkaudio daemon ... (type 'orkaudio debug' if you prefer running attached to tty)\n");
		DaemonSingleton::instance()->Start();		
	}
	return 0;
}

