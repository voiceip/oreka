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
#include "AudioTape.h"

#define LOG_MESSAGES_PARAM "LogMessages"
#define LOG_MESSAGES_DEFAULT false
#define CAPTURE_PLUGIN_PARAM "CapturePlugin"
#define CAPTURE_PLUGIN_DEFAULT ""
#define CAPTURE_PLUGIN_PATH_PARAM "CapturePluginPath"
#define CAPTURE_PLUGIN_PATH_DEFAULT "AudioCapturePlugins"
#define STORAGE_AUDIO_FORMAT_PARAM "StorageAudioFormat"
#define STORAGE_AUDIO_FORMAT_DEFAULT (AudioTape::FfGsm)
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

class Config : public Object
{
public:
	Config();	
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	bool m_logMessages;
	bool m_logRms;
	bool m_enableReporting;
	CStdString m_capturePlugin;
	CStdString m_capturePluginPath;
	int m_numBatchThreads;
	bool m_deleteNativeFile;
	int m_audioChunkDefaultSize;
	bool m_audioSegmentation;
	int m_audioSegmentDuration;
	AudioTape::FileFormatEnum m_storageAudioFormat;
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
};


#endif

