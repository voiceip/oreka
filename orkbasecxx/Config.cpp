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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "ace/OS_NS_unistd.h"
#include "Utils.h"
#include "serializers/Serializer.h"
#include "Config.h"

Config::Config()
{
	m_log = log4cxx::Logger::getLogger("config");

	m_logMessages = LOG_MESSAGES_DEFAULT;
	m_logRms = LOG_RMS_DEFAULT;
	m_enableReporting = ENABLE_REPORTING_DEFAULT;
	m_capturePluginPath = CAPTURE_PLUGIN_PATH_DEFAULT;
	m_storageAudioFormat = STORAGE_AUDIO_FORMAT_DEFAULT;
	m_numBatchThreads = NUM_BATCH_THREADS_DEFAULT;
	m_deleteNativeFile = DELETE_NATIVE_FILE_DEFAULT;
	m_audioChunkDefaultSize = AUDIO_CHUNK_DEFAULT_SIZE_DEFAULT;
	m_audioSegmentation = AUDIO_SEGMENTATION_DEFAULT;
	m_audioSegmentDuration = AUDIO_SEGMENT_DURATION_DEFAULT;
	m_vad = VAD_DEFAULT;
	m_vadHighThresholdDb = VAD_HIGH_THRESHOLD_DB_DEFAULT;
	m_vadLowThresholdDb = VAD_LOW_THRESHOLD_DB_DEFAULT;
	m_vadHoldOnSec = VAD_HOLD_ON_SEC_DEFAULT;
	m_trackerHostname = TRACKER_HOSTNAME_DEFAULT;
	m_trackerTcpPort = TRACKER_TCP_PORT_DEFAULT;
	m_trackerServicename = TRACKER_SERVICENAME_DEFAULT;
	m_audioOutputPath = AUDIO_OUTPUT_PATH_DEFAULT;
	m_immediateProcessingQueueSize = IMMEDIATE_PROCESSING_QUEUE_SIZE_DEFAULT;
	m_batchProcessingQueueSize = BATCH_PROCESSING_QUEUE_SIZE_DEFAULT;
	m_batchProcessingEnhancePriority = BATCH_PROCESSING_ENHANCE_PRIORITY_DEFAULT;
	m_deleteFailedCaptureFile = DELETE_FAILED_CAPTURE_FILE_DEFAULT;

	char hostname[40];
	ACE_OS::hostname(hostname, 40);
	//ACE_OS::hostname(hostname, HOSTNAME_BUF_LEN);
	m_serviceName = CStdString("orkaudio-") + hostname;

	m_reportingRetryDelay = 5;
	m_clientTimeout = 5;
}

void Config::Define(Serializer* s)
{
	s->BoolValue(LOG_MESSAGES_PARAM, m_logMessages);
	s->BoolValue(LOG_RMS_PARAM, m_logRms);
	s->BoolValue(ENABLE_REPORTING_PARAM, m_enableReporting);
	s->StringValue(CAPTURE_PLUGIN_PARAM, m_capturePlugin);
	s->StringValue(CAPTURE_PLUGIN_PATH_PARAM, m_capturePluginPath);
	s->EnumValue(STORAGE_AUDIO_FORMAT_PARAM, (int&)m_storageAudioFormat, FileFormatToEnum, FileFormatToString);
	s->IntValue(NUM_BATCH_THREADS_PARAM, m_numBatchThreads);
	s->BoolValue(DELETE_NATIVE_FILE_PARAM, m_deleteNativeFile);
	s->IntValue(AUDIO_CHUNK_DEFAULT_SIZE_PARAM, m_audioChunkDefaultSize);
	s->BoolValue(AUDIO_SEGMENTATION_PARAM, m_audioSegmentation);
	s->IntValue(AUDIO_SEGMENT_DURATION_PARAM, m_audioSegmentDuration);
	s->BoolValue(VAD_PARAM, m_vad);
	s->DoubleValue(VAD_HIGH_THRESHOLD_DB_PARAM, m_vadHighThresholdDb);
	s->DoubleValue(VAD_LOW_THRESHOLD_DB_PARAM, m_vadLowThresholdDb);
	s->DoubleValue(VAD_HOLD_ON_SEC_PARAM, m_vadHoldOnSec);
	s->StringValue(TRACKER_HOSTNAME_PARAM, m_trackerHostname);
	s->IntValue(TRACKER_TCP_PORT_PARAM, m_trackerTcpPort);
	s->StringValue(TRACKER_SERVICENAME_PARAM, m_trackerServicename);
	s->StringValue(SERVICE_NAME_PARAM, m_serviceName);
	s->IntValue(REPORTING_RETRY_DELAY_PARAM, m_reportingRetryDelay);
	s->IntValue(CLIENT_TIMEOUT_PARAM, m_clientTimeout);
	s->StringValue(AUDIO_OUTPUT_PATH_PARAM, m_audioOutputPath);
	s->IntValue(IMMEDIATE_PROCESSING_QUEUE_SIZE_PARAM, m_immediateProcessingQueueSize);
	s->IntValue(BATCH_PROCESSING_QUEUE_SIZE_PARAM, m_batchProcessingQueueSize);
	s->BoolValue(BATCH_PROCESSING_ENHANCE_PRIORITY_PARAM, m_batchProcessingEnhancePriority);
	s->BoolValue(DELETE_FAILED_CAPTURE_FILE_PARAM, m_deleteFailedCaptureFile);
	s->CsvValue(CAPTURE_PORT_FILTERS_PARAM, m_capturePortFilters);
}

void Config::Validate()
{
	if (m_storageAudioFormat <= FfUnknown || m_storageAudioFormat >= FfInvalid)
	{
		throw CStdString(CStdString("Config::Validate: value out of range:") + STORAGE_AUDIO_FORMAT_PARAM);
	}
	if (m_numBatchThreads > 2)
	{
		LOG4CXX_WARN(m_log, "It is not recommended to have more batch threads than CPUs");
	}
	if (m_vadHighThresholdDb < -45.0 || m_vadHighThresholdDb>0.0)
	{
		throw CStdString(CStdString("Config::Validate: value out of range:") + VAD_HIGH_THRESHOLD_DB_PARAM);
	}
	if (m_vadLowThresholdDb < -45.0 || m_vadLowThresholdDb>0.0)
	{
		throw CStdString(CStdString("Config::Validate: value out of range:") + VAD_LOW_THRESHOLD_DB_PARAM);
	}
	if (m_vadHighThresholdDb < m_vadLowThresholdDb)
	{
		throw CStdString(CStdString("Config::Validate: ") + VAD_LOW_THRESHOLD_DB_PARAM + " should be lower than " + VAD_HIGH_THRESHOLD_DB_PARAM);
	}
	if (m_vad && m_audioSegmentation)
	{
		throw CStdString(CStdString("Config::Validate: please choose between audio segmentation and VAD ! Both cannot be true at the same time"));
	}
}

CStdString Config::GetClassName()
{
	return CStdString("Config");
}

ObjectRef Config::NewInstance()
{
	return ObjectRef(new Config);
}

