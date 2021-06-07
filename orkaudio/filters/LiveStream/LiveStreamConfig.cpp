
#include "LiveStreamConfig.h"
#include <log4cxx/logger.h>

static log4cxx::LoggerPtr s_log = log4cxx::Logger::getLogger("plugin.livestream");

LiveStreamConfig g_LiveStreamConfigObjectRef;

LiveStreamConfig::LiveStreamConfig() {
    m_serverPort = LIVE_STREAMING_SERVER_PORT_DEFAULT;
    m_queueFlushThresholdMillis = DEFAULT_LIVE_STREAMING_QUEUE_FLUSH_THRESHOLD;
    m_shouldStreamAllCalls = false;
}

void LiveStreamConfig::Reset() {
    m_rtmpServerEndpoint = "";
    m_shouldStreamAllCalls = false;
    m_serverPort = LIVE_STREAMING_SERVER_PORT_DEFAULT;
    m_queueFlushThresholdMillis = DEFAULT_LIVE_STREAMING_QUEUE_FLUSH_THRESHOLD;
}

void LiveStreamConfig::Define(Serializer* s) {
    s->StringValue(RTMP_SERVER_ENDPOINT, m_rtmpServerEndpoint);
    s->IntValue(LIVE_STREAMING_SERVER_PORT_PARAM, m_serverPort);
    s->IntValue(LIVE_STREAMING_QUEUE_FLUSH_THRESHOLD, m_queueFlushThresholdMillis);
    s->BoolValue(LIVE_STREAM_ALL_CALLS, m_shouldStreamAllCalls);
    LOG4CXX_INFO(s_log, "LiveStreamConfig Endpoint " + m_rtmpServerEndpoint);
}

void LiveStreamConfig::Validate() {
}

CStdString LiveStreamConfig::GetClassName() {
	return CStdString("LiveStreamConfig");
}

ObjectRef LiveStreamConfig::NewInstance() {
	return ObjectRef(new LiveStreamConfig);
}

void LiveStreamConfig::Configure(DOMNode* node) {
	if (node){
		try {
			g_LiveStreamConfigObjectRef.DeSerializeDom(node);
			LOG4CXX_INFO(s_log, "LiveStreamConfig Configured");
		} catch (CStdString& e) {
			LOG4CXX_ERROR(s_log, e + " - check your config.xml");
		}
	} else {
		LOG4CXX_ERROR(s_log, "Got empty DOM tree");
	}
}

