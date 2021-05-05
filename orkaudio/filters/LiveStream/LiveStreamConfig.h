#ifndef __LIVESTREAMCONFIG_H__
#define __LIVESTREAMCONFIG_H__

#include "Object.h"
#include "serializers/Serializer.h"
#include "StdString.h"
#include "Utils.h"

#define RTMP_SERVER_ENDPOINT "RTMPServerEndPoint"
#define RTMP_SERVER_PORT "RTMPServerPort"
#define LIVE_STREAMING_SERVER_PORT_PARAM "LiveStreamingServerPort"
#define LIVE_STREAMING_QUEUE_FLUSH_THRESHOLD_SECONDS "LiveStreamingQueueFlushThresholdSeconds"
#define LIVE_STREAMING_SERVER_PORT_DEFAULT 59160
#define DEFAULT_LIVE_STREAMING_QUEUE_FLUSH_THRESHOLD_SECONDS 30
#define SHOULD_STREAM_ALL_CALLS "ShouldStreamAllCalls"

class LiveStreamConfig : public Object {
    public:
        LiveStreamConfig();
        void Define(Serializer* s);
        void Validate();
        void Reset();
        ObjectRef NewInstance();
        CStdString GetClassName();
        inline ObjectRef Process() {return ObjectRef();};

        static void Configure(DOMNode* node);

        CStdString m_rtmpServerEndpoint;
        CStdString m_rtmpServerPort;
        int m_liveStreamingServerPort;
        int m_liveStreamingQueueFlushThresholdSeconds;
        bool m_shouldStreamAllCalls;
 
};
// typedef oreka::shared_ptr<LiveStreamConfig> LiveStreamConfigObjectRef;
// extern LiveStreamConfigObjectRef g_LiveStreamConfigObjectRef;

extern LiveStreamConfig g_LiveStreamConfigObjectRef;

#define LIVESTREAMCONFIG g_LiveStreamConfigObjectRef 

#endif