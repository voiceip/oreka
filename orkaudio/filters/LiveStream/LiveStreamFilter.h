/*
 * Oreka -- A media capture and retrieval platform
 *
 * LiveStreamFilter Plugin
 * Author Kinshuk Bairagi
 *
 */

#ifndef __LIVESTREAM_H__
#define __LIVESTREAM_H__ 1

#include "LogManager.h"
#include "Filter.h"
#include <log4cxx/logger.h>
#include "Utils.h"
#include "srs_librtmp.h"
#include <deque>
#include "LiveStreamSession.h"
#include "AudioCapture.h"
#include <iostream>
#include <string>
#include <cstring>
#include "ConfigManager.h"
#include "LiveStreamServer.h"
#include "RingBuffer.h"

class DLL_IMPORT_EXPORT_ORKBASE LiveStreamFilter : public Filter {
    public:
        LiveStreamFilter();
        ~LiveStreamFilter();

        FilterRef __CDECL__ Instanciate();
        void __CDECL__ AudioChunkIn(AudioChunkRef &chunk);
        void __CDECL__ AudioChunkOut(AudioChunkRef &chunk);
        AudioEncodingEnum __CDECL__ GetInputAudioEncoding();
        AudioEncodingEnum __CDECL__ GetOutputAudioEncoding();
        CStdString __CDECL__ GetName();
        bool __CDECL__ SupportsInputRtpPayloadType(int rtpm_payloadType);
        void __CDECL__ CaptureEventIn(CaptureEventRef &event);
        void __CDECL__ CaptureEventOut(CaptureEventRef &event);
        void __CDECL__ SetSessionInfo(CStdString &trackingId);

    private:
        AudioChunkRef m_outputAudioChunk;
        bool m_initialized;
        CStdString m_callId;
        CStdString m_orkRefId;
        bool status = false;
        bool isFirstPacket = true;
        unsigned char headChannel;
        srs_rtmp_t rtmp = NULL;
        u_int32_t timestamp = 0;
        RingBuffer<AudioChunkRef> bufferQueue;
        bool shouldStreamAllCalls;
        char * silentChannelBuffer = NULL;
        void PushToRTMP(AudioChunkDetails& channelDetails, char* firstChannelBuffer, char* secondChannelBuffer);
};

#endif
