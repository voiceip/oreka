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
#include "messages/RecordMsg.h"
#include "messages/InitMsg.h"
//#include "messages/CrashMessage.cpp"
#include "Config.h"
#include "LogManager.h"
#include "ImmediateProcessing.h"
#include "BatchProcessing.h"
#include "Reporting.h"
#include "CommandProcessing.h"
#include "TapeFileNaming.h"
#include "DirectionSelector.h"
#include "ConfigManager.h"
#include "Daemon.h"
#include "ObjectFactory.h"
#include "CapturePluginProxy.h"
#include "AudioCapturePlugin.h"
#include "Filter.h"
#include "GsmFilters.h"
#include "IlbcFilters.h"
#include "G722Codec.h"
#include "filters/audiogain/AudioGain.h"
#include "TapeProcessor.h"
#include <list>
#include "EventStreaming.h"
#include "OrkTrack.h"
#include "SocketStreamer.h"
#include "SpeexCodec.h"
#include "G721Codec.h"

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
	CStdString pluginsDirectory = CONFIG.m_pluginsDirectory;
#ifdef WIN32
	if(pluginsDirectory.size() == 0)
	{
		// default windows plugins directory
		pluginsDirectory = "./plugins/";
	}
	CStdString pluginExtension = ".dll";
#else
	if(pluginsDirectory.size() == 0)
	{
		// default unix plugins directory
		pluginsDirectory = "/usr/lib/orkaudio/plugins/";
	}
	CStdString pluginExtension = ".so";
#endif
	CStdString pluginPath;
	ACE_DLL dll;

	ACE_DIR* dir = ACE_OS::opendir((PCSTR)pluginsDirectory);
	if (!dir)
	{
		LOG4CXX_ERROR(LOG.rootLog, CStdString("Plugins directory could not be found:" + pluginsDirectory + " check your config.xml"));
	}
	else
	{
		dirent* dirEntry = NULL;
		while((dirEntry = ACE_OS::readdir(dir)))
		{	
			CStdString dirEntryFilename = dirEntry->d_name;
			int extensionPos = dirEntryFilename.Find(pluginExtension);

			if ( extensionPos != -1 && (dirEntryFilename.size() - extensionPos) == pluginExtension.size() )
			{
				pluginPath = pluginsDirectory + "/" + dirEntry->d_name;
				dll.open((PCSTR)pluginPath);
				ACE_TCHAR* error = dll.error();
				if(error)
				{
					LOG4CXX_ERROR(LOG.rootLog, CStdString("Failed to load plugin: ") + pluginPath);
#ifndef WIN32
					CStdString logMsg;
					logMsg.Format("DLL Error: %s", dlerror());
					LOG4CXX_ERROR(LOG.rootLog, logMsg);
#endif
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

void Transcode(CStdString &file)
{
	OrkLogManager::Instance()->Initialize();

	ObjectFactory::GetSingleton()->Initialize();

	ConfigManager::Instance()->Initialize();

	std::list<ACE_DLL> pluginDlls;
	LoadPlugins(pluginDlls);

	// Register in-built filters
	FilterRef filter(new AlawToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new UlawToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new GsmToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new IlbcToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new AudioGainFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new G722ToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new SpeexDecoder() );
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new G721CodecDecoder());
	FilterRegistry::instance()->RegisterFilter(filter);
	
	// Register in-built tape processors and build the processing chain
	BatchProcessing::Initialize();
	Reporting::Initialize();
	TapeFileNaming::Initialize();

	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(BatchProcessing::ThreadHandler)))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create batch processing thread"));
	}


	
	// Transmit the tape to the BatchProcessing
	CStdString ProcessorName("BatchProcessing");
	TapeProcessorRef bp = TapeProcessorRegistry::instance()->GetNewTapeProcessor(ProcessorName);
	CStdString portName("SinglePort");
	AudioTapeRef tape(new AudioTape(portName, file));
	bp->AddAudioTape(tape);
	
	// Make sure it stops after processing
	tape.reset();
	bp->AddAudioTape(tape);

	// Wait for completion
	while(!Daemon::Singleton()->IsStopping())
	{
		ACE_OS::sleep(1);
	}
}

void MainThread()
{
	// Avoid program exit on broken pipe
	ACE_OS::signal (SIGPIPE, (ACE_SignalHandler) SIG_IGN);
	OrkLogManager::Instance()->Initialize();
	LOG4CXX_INFO(LOG.rootLog, CStdString("\n\nOrkAudio service starting\n"));

	ConfigManager::Instance()->Initialize();

	// Initialize object factory and register existing objects
	ObjectFactory::GetSingleton()->Initialize();

	ObjectRef objRef;
	objRef.reset(new PingMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new TapeMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new TapeResponse);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new SimpleResponseMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new DeleteTapeMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new CaptureMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new TcpPingMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new ReportingSkipTapeMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new RecordMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new PauseMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new StopMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	objRef.reset(new InitMsg);
	ObjectFactory::GetSingleton()->RegisterObject(objRef);
	//objRef.reset(new CrashMsg);
	//ObjectFactory::GetSingleton()->RegisterObject(objRef);
	//objRef.reset(new TestMsg);
	//ObjectFactory::GetSingleton()->RegisterObject(objRef);

	bool capturePluginOk = false;
	if(CapturePluginProxy::Singleton()->Initialize())
	{
		capturePluginOk = true;
	}

	std::list<ACE_DLL> pluginDlls;
	LoadPlugins(pluginDlls);

	// Register in-built filters
	FilterRef filter(new AlawToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new UlawToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new GsmToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new IlbcToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new AudioGainFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new G722ToPcmFilter());
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new SpeexDecoder() );
	FilterRegistry::instance()->RegisterFilter(filter);
	filter.reset(new G721CodecDecoder());
	FilterRegistry::instance()->RegisterFilter(filter);
	
	// Register in-built tape processors and build the processing chain
	OrkTrack::Initialize(CONFIG.m_trackerHostname, CONFIG.m_trackerServicename, CONFIG.m_trackerTcpPort);
	BatchProcessing::Initialize();
	CommandProcessing::Initialize();
	Reporting::Initialize();
	TapeFileNaming::Initialize();
	DirectionSelector::Initialize();
	TapeProcessorRegistry::instance()->CreateProcessingChain();

	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(ImmediateProcessing::ThreadHandler)))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create immediate processing thread"));
	}
	if(CONFIG.m_storageAudioFormat != FfNative)
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
	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(TapeFileNaming::ThreadHandler)))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create tape file naming thread"));
	}

	if (!ACE_Thread_Manager::instance()->spawn_n(CONFIG.m_numCommandThreads,ACE_THR_FUNC(CommandProcessing::ThreadHandler)))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create the Command Processing thread"));
	}

	if (!ACE_Thread_Manager::instance()->spawn_n(CONFIG.m_numDirectionSelectorThreads,ACE_THR_FUNC(DirectionSelector::ThreadHandler)))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create the DirectionSelector thread"));
	}

	// Create command line server
	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(CommandLineServer::run), (void *)CONFIG.m_commandLineServerPort))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create command line server"));
	}

	// Create Http server
	if (!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(HttpServer::run), (void *)CONFIG.m_httpServerPort))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create Http server"));
	}

	// Create event streaming server
	if(!ACE_Thread_Manager::instance()->spawn(ACE_THR_FUNC(EventStreamingServer::run), (void *)CONFIG.m_eventStreamingServerPort))
	{
		LOG4CXX_INFO(LOG.rootLog, CStdString("Failed to create event streaming server"));
	}

	if(capturePluginOk)
	{
		CapturePluginProxy::Singleton()->Run();
	}

	SocketStreamer::Initialize();

	//ACE_Thread_Manager::instance ()->wait ();
	while(!Daemon::Singleton()->IsStopping())
	{
		ACE_OS::sleep(1);
	}

	CapturePluginProxy::Singleton()->Shutdown();

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
	OrkLogManager::Instance()->Shutdown();
}


int main(int argc, char* argv[])
{
	// the "service name" reported on the tape messages uses CONFIG.m_serviceName
	// which also defaults to orkaudio-[hostname] but can be different depending on the 
	// value set in config.xml
	char hostname[40];
	ACE_OS::hostname(hostname, 40);
	CStdString serviceName = CStdString("orkaudio-") + hostname;
	Daemon::Initialize(serviceName, MainThread, StopHandler);

	CStdString argument = argv[1];
	if (argc>1)
	{
		if (argument.CompareNoCase("debug") == 0)
		{
			MainThread();
		}
		else if (argument.CompareNoCase("transcode") == 0)
		{
			if(argc == 3)
			{
				Daemon::Singleton()->SetShortLived();
				CStdString file = argv[2];
				Transcode(file);
			}
			else
			{
				printf("Please specify file to transcode\n\n");
			}
		}
		else if (argument.CompareNoCase("install") == 0)
		{
			Daemon::Singleton()->Install();
		}
		else if  (argument.CompareNoCase("uninstall") == 0)
		{
			Daemon::Singleton()->Uninstall();
		}
		else
		{
#ifdef WIN32
	printf("Argument incorrect. Possibilies are:\ninstall: install NT service\nuninstall: uninstall NT service\ntranscode <file>: convert .mcf file to storage format specified in config.xml\n\n");
#else
	printf("Argument incorrect. Possibilies are:\ndebug: run attached to tty\ntranscode <file>: convert .mcf file to storage format specified in config.xml\n\n");
#endif
		}
	}
	else
	{
		// No arguments, launch the daemon
		printf("Starting orkaudio daemon ... (type 'orkaudio debug' if you prefer running attached to tty)\n");
		Daemon::Singleton()->Start();		
	}
	return 0;
}

