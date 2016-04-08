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
#include <map>

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
#define NUM_COMMAND_THREADS_PARAM "NumCommandThreads"
#define NUM_BATCH_THREADS_DEFAULT 1
#define NUM_DIRECTIONSELECTOR_THREADS_PARAM "NumDirSelectorThreads"
#define NUM_DIRECTIONSELECTOR_THREADS_DEFAULT 1
#define NUM_COMMAND_THREADS_DEFAULT 1
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
#define AUDIO_OUTPUT_SECONDARY_PATH_PARAM "AudioOutputPathSecondary"
#define AUDIO_OUTPUT_PATH_DEFAULT "."
#define AUDIO_OUTPUT_SECONDARY_PATH_DEFAULT ""
#define AUDIO_FILE_PERMISSIONS_PARAM "AudioFilePermissions"
#define AUDIO_FILE_PERMISSIONS_DEFAULT "0644"
#define AUDIO_FILE_OWNER_PARAM "AudioFileOwner"
#define AUDIO_FILE_OWNER_DEFAULT "root"
#define AUDIO_FILE_GROUP_PARAM "AudioFileGroup"
#define AUDIO_FILE_GROUP_DEFAULT "root"
#define REPORTING_QUEUE_SIZE_PARAM "ReportingQueueSize"
#define REPORTING_QUEUE_SIZE_DEFAULT 50000
#define IMMEDIATE_PROCESSING_QUEUE_SIZE_PARAM "ImmediateProcessingQueueSize"
#define IMMEDIATE_PROCESSING_QUEUE_SIZE_DEFAULT 10000
#define BATCH_PROCESSING_QUEUE_SIZE_PARAM "BatchProcessingQueueSize"
#define BATCH_PROCESSING_QUEUE_SIZE_DEFAULT 20000
#define DIRECTIONSELECTOR_QUEUE_SIZE_PARAM "DirectionSelectorQueueSize"
#define DIRECTIONSELECTOR_QUEUE_SIZE_DEFAULT 20000
#define BATCH_PROCESSING_ENHANCE_PRIORITY_PARAM "BatchProcessingEnhancePriority"
#define BATCH_PROCESSING_ENHANCE_PRIORITY_DEFAULT false
#define DELETE_FAILED_CAPTURE_FILE_PARAM "DeleteFailedCaptureFile"
#define DELETE_FAILED_CAPTURE_FILE_DEFAULT false
#define CAPTURE_PORT_FILTERS_PARAM "CapturePortFilters" 
#define TAPE_PROCESSORS_PARAM "TapeProcessors"
#define CAPTURE_FILE_BATCH_SIZE_KBYTE_PARAM "CaptureFileBatchSizeKByte"
#define CAPTURE_FILE_BATCH_SIZE_KBYTE_DEFAULT 64
#define DEBUG_PARAM "Debug"
#define DEBUG_DEFAULT false
#define TAPE_FILE_NAMING_PARAM "TapeFileNaming"
#define TAPE_PATH_NAMING_PARAM "TapePathNaming"
#define REMOTE_PROCESSING_INPUT_PATH_PARAM "RemoteProcessingInputPath"
#define REMOTE_PROCESSING_OUTPUT_PATH_PARAM "RemoteProcessingOutputPath"
#define REMOTE_PROCESSING_HOSTNAME_PARAM "RemoteProcessingHostname"
#define REMOTE_PROCESSING_HOSTNAME_DEFAULT "localhost"
#define REMOTE_PROCESSING_TCP_PORT_PARAM "RemoteProcessingTcpPort"
#define REMOTE_PROCESSING_TCP_PORT_DEFAULT HTTP_SERVER_PORT_DEFAULT
#define REMOTE_PROCESSING_SERVICE_NAME_PARAM "RemoteProcessingServiceName"
#define REMOTE_PROCESSING_SERVICE_NAME_DEFAULT "orkaudio"
#define COMMAND_LINE_SERVER_PORT_PARAM "CommandLineServerPort"
#define COMMAND_LINE_SERVER_PORT_DEFAULT 59130
#define HTTP_SERVER_PORT_PARAM "HttpServerPort"
#define HTTP_SERVER_PORT_DEFAULT 59140
#define STREAMING_SERVER_PORT_PARAM "EventStreamingServerPort"
#define STREAMING_SERVER_PORT_DEFAULT 59150
#define LOOKBACK_RECORDING_PARAM "LookBackRecording"
#define LOOKBACK_RECORDING_DEFAULT true
#define ALLOW_AUTOMATIC_RECORDING_PARAM "AllowAutomaticRecording"
#define ALLOW_AUTOMATIC_RECORDING_DEFAULT true
#define CAPTURE_FILE_SIZE_LIMIT_KB_PARAM "CaptureFileSizeLimitKb"
#define CAPTURE_FILE_SIZE_LIMIT_KB_DEFAULT 300000
#define PARTY_FILTER_PARAM "PartyFilter"
#define PARTY_FILTER_DEFAULT ""
#define STEREO_RECORDING_PARAM "StereoRecording"
#define STEREO_RECORDING_DEFAULT false
#define TAPE_NUM_CHANNELS_PARAM "TapeNumChannels"
#define TAPE_NUM_CHANNELS_DEFAULT 2
#define TAPE_DURATION_MINIMUM_SEC_PARAM "TapeDurationMinimumSec"
#define TAPE_DURATION_MINIMUM_SEC_DEFAULT 2
#define TRANSCODING_SLEEP_EVERY_NUM_FRAMES_PARAM "TranscodingSleepEveryNumFrames"
#define TRANSCODING_SLEEP_EVERY_NUM_FRAMES_DEFAULT 0
#define TRANSCODING_SLEEP_US_PARAM "TranscodingSleepUs"
#define TRANSCODING_SLEEP_US_DEFAULT 0
#define AUDIO_GAIN_DB_PARAM "AudioGainDb"
#define AUDIO_GAIN_DB_DEFAULT 0
#define AUDIO_GAIN_CHANNEL_1_DB_PARAM "AudioGainChannel1Db"
#define AUDIO_GAIN_CHANNEL_1_DB_DEFAULT 0
#define AUDIO_GAIN_CHANNEL_2_DB_PARAM "AudioGainChannel2Db"
#define AUDIO_GAIN_CHANNEL_2_DB_DEFAULT 0
#define AUDIO_KEEP_DIRECTION_DEFAULT_PARAM "AudioKeepDirectionDefault"
#define AUDIO_KEEP_DIRECTION_DEFAULT_DEFAULT "both"
#define AUDIO_KEEP_DIRECTION_INCOMING_DEFAULT_PARAM "AudioKeepDirectionIncomingDefault"
#define AUDIO_KEEP_DIRECTION_INCOMING_DEFAULT_DEFAULT "both"
#define AUDIO_KEEP_DIRECTION_OUTGOING_DEFAULT_PARAM "AudioKeepDirectionOutgoingDefault"
#define AUDIO_KEEP_DIRECTION_OUTGOING_DEFAULT_DEFAULT "both"
#define SOCKET_STREAMER_TARGETS_PARAM "SocketStreamerTargets"
#define RECORDING_START_SHELL_COMMAND_PARAM "RecordingStartShellCommand"
#define RECORDING_STOP_SHELL_COMMAND_PARAM "RecordingStopShellCommand"
#define COMMAND_PROCESSING_COMMAND_PARAM "CommandProcessingCommand"
#define	TAGS_LIST_USE_INITIAL_VALUE_PARAM "TagsListUseInitialValue"
#define DIRECTION_FORCE_OUTGOING_FOR_REMOTE_PARTY_PREFIX "DirectionForceOutgoingForRemotePartyPrefix"
#define DIRECTION_FORCE_OUTGOING_FOR_REMOTE_PARTY_MIN_LENGTH "DirectionForceOutgoingForRemotePartyMinLength"
#define DIRECTION_FORCE_OUTGOING_FOR_REMOTE_PARTY_MIN_LENGTH_DEFAULT 11
#define PAUSE_RECORDING_ON_REJECTED_START "PauseRecordingOnRejectedStart"
#define PAUSE_RECORDING_ON_REJECTED_START_DEFAULT false
#define DIRECTION_LOOKBACK_PARAM "DirectionLookBack"
#define DIRECTION_LOOKBACK_DEFAULT true
#define SPEEX_PAYLOAD_TYPES "SpeexPayloadTypes"
#define DTMF_REPORTING_DETAILED false;
#define CLIENT_RETRY_PERIOD_SEC "ClientRetryPeriodSec"
#define CLIENT_RETRY_PERIOD_SEC_DEFAULT 2
#define DYNAMIC_TAGS "DynamicTags"
#define HOSTNAME_REPORT_FQDN "HostnameReportFqdn"
#define HOSTNAME_REPORT_FQDN_DEFAULT false

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
	int m_numDirectionSelectorThreads;
	int m_numCommandThreads;
	bool m_deleteNativeFile;
	int m_audioChunkDefaultSize;
	bool m_audioSegmentation;
	int m_audioSegmentDuration;
	FileFormatEnum m_storageAudioFormat;
	bool m_vad;
	double m_vadHighThresholdDb;
	double m_vadLowThresholdDb;
	double m_vadHoldOnSec;
	std::list<CStdString> m_trackerHostname;
	CStdString m_trackerServicename;
	int m_trackerTcpPort;
	CStdString m_serviceName;
	int m_reportingRetryDelay;
	int m_clientTimeout;
	CStdString m_audioOutputPath;
	CStdString m_audioOutputPathSecondary;
	int m_audioFilePermissions;
	CStdString m_audioFileOwner;
	CStdString m_audioFileGroup;
	int m_reportingQueueSize;
	int m_immediateProcessingQueueSize;
	int m_batchProcessingQueueSize;
	int m_directionSelectorQueueSize;
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
	int m_eventStreamingServerPort;
	bool m_lookBackRecording;
	bool m_allowAutomaticRecording;
	int m_captureFileSizeLimitKb;
	std::list<CStdString> m_partyFilter;
	bool m_stereoRecording;
	int m_tapeNumChannels;
	int m_tapeDurationMinimumSec;
	int m_transcodingSleepEveryNumFrames;
	int m_transcodingSleepUs;
	double m_audioGainDb;
	double m_audioGainChannel1Db;
	double m_audioGainChannel2Db;
	CStdString m_audioKeepDirectionDefault;
	CStdString m_audioKeepDirectionIncomingDefault;
	CStdString m_audioKeepDirectionOutgoingDefault;
	std::list<CStdString> m_socketStreamerTargets;
	std::list<CStdString> m_tagsListUseInitialValue;
	CStdString m_recordingStartShellCommand;
	CStdString m_recordingStopShellCommand;
	CStdString m_commandProcessingCommand;
	CStdString m_directionForceOutgoingForRemotePartyPrefix;
	int m_directionForceOutgoingForRemotePartyMinLength;
	bool m_pauseRecordingOnRejectedStart;
	CStdString m_partyFilterChars;
	CStdString m_partyFilterCharsReplaceWith;
	std::map<char, char> m_partyFilterMap;
	bool m_directionLookBack;
	std::list<CStdString> m_speexPayloadTypes;
	int m_remotePartyMaxDigits;
	bool m_dtmfReportingDetailed;
	int m_clientRetryPeriodSec;
	std::list<CStdString> m_dynamicTags;
	bool m_hostnameReportFqdn;

private:
	log4cxx::LoggerPtr m_log;
	CStdString m_audioFilePermissionsStr;
};


#endif

