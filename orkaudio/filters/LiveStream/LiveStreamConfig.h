#ifndef __LIVESTREAMCONFIG_H__
#define __LIVESTREAMCONFIG_H__

#include "Object.h"
#include "serializers/Serializer.h"
#include "StdString.h"
#include "Utils.h"

#define RTMP_SERVER_ENDPOINT "RTMPServerEndPoint"
#define LIVE_STREAMING_SERVER_PORT_PARAM "LiveStreamingServerPort"
#define LIVE_STREAMING_QUEUE_FLUSH_THRESHOLD "LiveStreamingQueueFlushThreshold"
#define LIVE_STREAMING_SERVER_PORT_DEFAULT 59160
#define DEFAULT_LIVE_STREAMING_QUEUE_FLUSH_THRESHOLD 500
#define LIVE_STREAM_ALL_CALLS "LiveStreamAllCalls"

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
        int m_serverPort;
        int m_queueFlushThresholdMillis;
        bool m_shouldStreamAllCalls;
};

extern LiveStreamConfig g_LiveStreamConfigObjectRef;

#define LIVESTREAMCONFIG g_LiveStreamConfigObjectRef 

#endif
