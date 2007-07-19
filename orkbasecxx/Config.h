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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "StdString.h"
#include "Object.h"
#include "AudioCapture.h"
#include <log4cxx/logger.h>


#define LOG_MESSAGES_PARAM "LogMessages"
#define LOG_MESSAGES_DEFAULT false
#define CAPTURE_PLUGIN_PARAM "CapturePlugin"
#define CAPTURE_PLUGIN_DEFAULT ""
#define CAPTURE_PLUGIN_PATH_PARAM "CapturePluginPath"
#define CAPTURE_PLUGIN_PATH_DEFAULT "AudioCapturePlugins"
#define PLUGINS_DIRECTORY_PARAM "PluginsDirectory"
#define STORAGE_AUDIO_FORMAT_PARAM "StorageAudioFormat"
#define STORAGE_AUDIO_FORMAT_DEFAULT FfGsm
#define NUM_BATCH_THREADS_PARAM "NumBatchThreads"
#define NUM_BATCH_THREADS_DEFAULT 1
#define DELETE_NATIVE_FILE_PARAM "DeleteNativeFile"
#define DELETE_NATIVE_FILE_DEFAULT true
#define ENABLE_REPORTING_PARAM "EnableReporting"
#define ENABLE_REPORTING_DEFAULT true
#define AUDIO_CHUNK_DEFAULT_SIZE_PARAM "AudioChunkDefaultSize"
#define AUDIO_CHUNK_DEFAULT_SIZE_DEFAULT 8000
#define AUDIO_SEGMENTATION_PARAM "AudioSegmentation"
#define AUDIO_SEGMENTATION_DEFAULT false
#define AUDIO_SEGMENT_DURATION_PARAM "AudioSegmentDuration"
#define AUDIO_SEGMENT_DURATION_DEFAULT 60
#define LOG_RMS_PARAM "LogRms"
#define LOG_RMS_DEFAULT false
#define VAD_PARAM "VAD"
#define VAD_DEFAULT false
#define VAD_HIGH_THRESHOLD_DB_PARAM "VadHighThresholdDb"
#define VAD_HIGH_THRESHOLD_DB_DEFAULT -12.2
#define VAD_LOW_THRESHOLD_DB_PARAM "VadLowThresholdDb"
#define VAD_LOW_THRESHOLD_DB_DEFAULT -12.5
#define VAD_HOLD_ON_SEC_PARAM "VadHoldOnSec"
#define VAD_HOLD_ON_SEC_DEFAULT 4
#define TRACKER_HOSTNAME_PARAM "TrackerHostname"
#define TRACKER_HOSTNAME_DEFAULT "localhost"
#define TRACKER_TCP_PORT_PARAM "TrackerTcpPort"
#define TRACKER_TCP_PORT_DEFAULT 8080
#define TRACKER_SERVICENAME_PARAM "TrackerServicename"
#define TRACKER_SERVICENAME_DEFAULT "orktrack"
#define SERVICE_NAME_PARAM "ServiceName"
#define REPORTING_RETRY_DELAY_PARAM "ReportingRetryDelay"
#define CLIENT_TIMEOUT_PARAM "ClientTimeout"
#define AUDIO_OUTPUT_PATH_PARAM "AudioOutputPath"
#define AUDIO_OUTPUT_PATH_DEFAULT "."
#define IMMEDIATE_PROCESSING_QUEUE_SIZE_PARAM "ImmediateProcessingQueueSize"
#define IMMEDIATE_PROCESSING_QUEUE_SIZE_DEFAULT 10000
#define BATCH_PROCESSING_QUEUE_SIZE_PARAM "BatchProcessingQueueSize"
#define BATCH_PROCESSING_QUEUE_SIZE_DEFAULT 20000
#define BATCH_PROCESSING_ENHANCE_PRIORITY_PARAM "BatchProcessingEnhancePriority"
#define BATCH_PROCESSING_ENHANCE_PRIORITY_DEFAULT false
#define DELETE_FAILED_CAPTURE_FILE_PARAM "DeleteFailedCaptureFile"
#define DELETE_FAILED_CAPTURE_FILE_DEFAULT false
#define CAPTURE_PORT_FILTERS_PARAM "CapturePortFilters" 
#define TAPE_PROCESSORS_PARAM "TapeProcessors"
#define CAPTURE_FILE_BATCH_SIZE_KBYTE_PARAM "CaptureFileBatchSizeKByte"
#define CAPTURE_FILE_BATCH_SIZE_KBYTE_DEFAULT 4
#define DEBUG_PARAM "Debug"
#define DEBUG_DEFAULT false
#define TAPE_FILE_NAMING_PARAM "TapeFileNaming"
#define TAPE_PATH_NAMING_PARAM "TapePathNaming"
#define REMOTE_PROCESSING_INPUT_PATH_PARAM "RemoteProcessingInputPath"
#define REMOTE_PROCESSING_OUTPUT_PATH_PARAM "RemoteProcessingOutputPath"
#define REMOTE_PROCESSING_HOSTNAME_PARAM "RemoteProcessingHostname"
#define REMOTE_PROCESSING_HOSTNAME_DEFAULT "localhost"
#define REMOTE_PROCESSING_TCP_PORT_PARAM "RemoteProcessingTcpPort"
#define REMOTE_PROCESSING_TCP_PORT_DEFAULT 20000
#define REMOTE_PROCESSING_SERVICE_NAME_PARAM "RemoteProcessingServiceName"
#define REMOTE_PROCESSING_SERVICE_NAME_DEFAULT "orkaudio"
#define COMMAND_LINE_SERVER_PORT_PARAM "CommandLineServerPort"
#define COMMAND_LINE_SERVER_PORT_DEFAULT 59130
#define HTTP_SERVER_PORT_PARAM "HttpServerPort"
#define HTTP_SERVER_PORT_DEFAULT 59140

class DLL_IMPORT_EXPORT_ORKBASE Config : public Object
{
public:
	Config();	
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};
	time_t m_serviceStartedTime;
	bool m_logMessages;
	bool m_logRms;
	bool m_enableReporting;
	CStdString m_capturePlugin;
	CStdString m_capturePluginPath;
	CStdString m_pluginsDirectory;
	int m_numBatchThreads;
	bool m_deleteNativeFile;
	int m_audioChunkDefaultSize;
	bool m_audioSegmentation;
	int m_audioSegmentDuration;
	FileFormatEnum m_storageAudioFormat;
	bool m_vad;
	double m_vadHighThresholdDb;
	double m_vadLowThresholdDb;
	double m_vadHoldOnSec;
	CStdString m_trackerHostname;
	CStdString m_trackerServicename;
	int m_trackerTcpPort;
	CStdString m_serviceName;
	int m_reportingRetryDelay;
	int m_clientTimeout;
	CStdString m_audioOutputPath;
	int m_immediateProcessingQueueSize;
	int m_batchProcessingQueueSize;
	bool m_batchProcessingEnhancePriority;
	bool m_deleteFailedCaptureFile;
	std::list<CStdString> m_capturePortFilters;
	std::list<CStdString> m_tapeProcessors;
	int m_captureFileBatchSizeKByte;
	bool m_debug;
	std::list<CStdString> m_tapeFileNaming;
	std::list<CStdString> m_tapePathNaming;
	std::list<CStdString> m_remoteProcessingInputPath;
	CStdString m_remoteProcessingOutputPath;
	CStdString m_remoteProcessingHostname;
	int m_remoteProcessingTcpPort;
	CStdString m_remoteProcessingServiceName;
	int m_commandLineServerPort;
	int m_httpServerPort;

private:
	log4cxx::LoggerPtr m_log;
};


#endif

