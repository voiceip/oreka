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
#include "ace/OS_NS_dirent.h"
#include "Utils.h"
#include "serializers/Serializer.h"
#include "Config.h"

Config::Config()
{
	m_log = log4cxx::Logger::getLogger("config");
	m_serviceStartedTime = time(NULL);
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
	m_trackerHostname.push_back(TRACKER_HOSTNAME_DEFAULT);
	m_trackerTcpPort = TRACKER_TCP_PORT_DEFAULT;
	m_trackerServicename = TRACKER_SERVICENAME_DEFAULT;
	m_audioOutputPath = AUDIO_OUTPUT_PATH_DEFAULT;

	m_audioFilePermissions = 0;

	m_immediateProcessingQueueSize = IMMEDIATE_PROCESSING_QUEUE_SIZE_DEFAULT;
	m_batchProcessingQueueSize = BATCH_PROCESSING_QUEUE_SIZE_DEFAULT;
	m_batchProcessingEnhancePriority = BATCH_PROCESSING_ENHANCE_PRIORITY_DEFAULT;
	m_deleteFailedCaptureFile = DELETE_FAILED_CAPTURE_FILE_DEFAULT;
	m_captureFileBatchSizeKByte = CAPTURE_FILE_BATCH_SIZE_KBYTE_DEFAULT;

	char hostname[40];
	ACE_OS::hostname(hostname, 40);
	m_serviceName = CStdString("orkaudio-") + hostname;

	m_reportingRetryDelay = 5;
	m_clientTimeout = 5;
	m_debug = DEBUG_DEFAULT;

	m_remoteProcessingHostname = REMOTE_PROCESSING_HOSTNAME_DEFAULT;
	m_remoteProcessingTcpPort = REMOTE_PROCESSING_TCP_PORT_DEFAULT;
	m_remoteProcessingServiceName = REMOTE_PROCESSING_SERVICE_NAME_DEFAULT;

	m_commandLineServerPort = COMMAND_LINE_SERVER_PORT_DEFAULT;
	m_httpServerPort = HTTP_SERVER_PORT_DEFAULT;
	m_lookBackRecording = LOOKBACK_RECORDING_DEFAULT;
	m_allowAutomaticRecording = ALLOW_AUTOMATIC_RECORDING_DEFAULT;
	m_captureFileSizeLimitKb = CAPTURE_FILE_SIZE_LIMIT_KB_DEFAULT;
	m_partyFilter.clear();
}

void Config::Define(Serializer* s)
{
	s->BoolValue(LOG_MESSAGES_PARAM, m_logMessages);
	s->BoolValue(LOG_RMS_PARAM, m_logRms);
	s->BoolValue(ENABLE_REPORTING_PARAM, m_enableReporting);
	s->StringValue(CAPTURE_PLUGIN_PARAM, m_capturePlugin);
	s->StringValue(CAPTURE_PLUGIN_PATH_PARAM, m_capturePluginPath);
	s->StringValue(PLUGINS_DIRECTORY_PARAM, m_pluginsDirectory);
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
	//s->StringValue(TRACKER_HOSTNAME_PARAM, m_trackerHostname);
	s->CsvValue(TRACKER_HOSTNAME_PARAM, m_trackerHostname);
	s->IntValue(TRACKER_TCP_PORT_PARAM, m_trackerTcpPort);
	s->StringValue(TRACKER_SERVICENAME_PARAM, m_trackerServicename);
	s->StringValue(SERVICE_NAME_PARAM, m_serviceName);
	s->IntValue(REPORTING_RETRY_DELAY_PARAM, m_reportingRetryDelay);
	s->IntValue(CLIENT_TIMEOUT_PARAM, m_clientTimeout);

	s->StringValue(AUDIO_OUTPUT_PATH_PARAM, m_audioOutputPath);

	if(!m_audioOutputPath.size()) {
		char *loggingPath = NULL;

		loggingPath = ACE_OS::getenv("ORKAUDIO_LOGGING_PATH");
		if(loggingPath) {
			ACE_DIR* dir = ACE_OS::opendir(loggingPath);
			if(dir) {
				ACE_OS::closedir(dir);
				m_audioOutputPath.Format("%s", loggingPath);
		        }
		}
	}

	s->StringValue(AUDIO_FILE_PERMISSIONS_PARAM, m_audioFilePermissionsStr);
	if(m_audioFilePermissionsStr.size())
	{
		m_audioFilePermissions = strtoul(m_audioFilePermissionsStr.c_str(), NULL, 8);
	}

	s->StringValue(AUDIO_FILE_OWNER_PARAM, m_audioFileOwner);
	s->StringValue(AUDIO_FILE_GROUP_PARAM, m_audioFileGroup);
	s->IntValue(IMMEDIATE_PROCESSING_QUEUE_SIZE_PARAM, m_immediateProcessingQueueSize);
	s->IntValue(BATCH_PROCESSING_QUEUE_SIZE_PARAM, m_batchProcessingQueueSize);
	s->BoolValue(BATCH_PROCESSING_ENHANCE_PRIORITY_PARAM, m_batchProcessingEnhancePriority);
	s->BoolValue(DELETE_FAILED_CAPTURE_FILE_PARAM, m_deleteFailedCaptureFile);
	s->CsvValue(CAPTURE_PORT_FILTERS_PARAM, m_capturePortFilters);
	s->CsvValue(TAPE_PROCESSORS_PARAM, m_tapeProcessors);
	s->IntValue(CAPTURE_FILE_BATCH_SIZE_KBYTE_PARAM, m_captureFileBatchSizeKByte);
	s->BoolValue(DEBUG_PARAM, m_debug);
	s->CsvValue(TAPE_FILE_NAMING_PARAM, m_tapeFileNaming);
	s->CsvValue(TAPE_PATH_NAMING_PARAM, m_tapePathNaming);
	s->CsvValue(REMOTE_PROCESSING_INPUT_PATH_PARAM, m_remoteProcessingInputPath);
	s->StringValue(REMOTE_PROCESSING_OUTPUT_PATH_PARAM, m_remoteProcessingOutputPath);
	s->StringValue(REMOTE_PROCESSING_HOSTNAME_PARAM, m_remoteProcessingHostname);
	s->IntValue(REMOTE_PROCESSING_TCP_PORT_PARAM, m_remoteProcessingTcpPort);
	s->StringValue(REMOTE_PROCESSING_SERVICE_NAME_PARAM, m_remoteProcessingServiceName);
	s->IntValue(COMMAND_LINE_SERVER_PORT_PARAM, m_commandLineServerPort);
	s->IntValue(HTTP_SERVER_PORT_PARAM, m_httpServerPort);
	s->BoolValue(LOOKBACK_RECORDING_PARAM, m_lookBackRecording);
	s->BoolValue(ALLOW_AUTOMATIC_RECORDING_PARAM, m_allowAutomaticRecording);	// only valid in non-lookback mode
	s->IntValue(CAPTURE_FILE_SIZE_LIMIT_KB_PARAM, m_captureFileSizeLimitKb);
	s->CsvValue(PARTY_FILTER_PARAM, m_partyFilter);
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

	if(m_audioFilePermissions == 0)
	{
		int perm10;

		perm10 = strtoul(m_audioFilePermissionsStr.c_str(), NULL, 10);
		if(perm10 < 0 || perm10 > 511)
		{
			CStdString exc;

			exc.Format("Config::Validate: please set valid permissions the AudioFilePermissions paramiter in config.xml - %s is not a valid file permission", m_audioFilePermissionsStr);

			throw(exc);
		}
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

