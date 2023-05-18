/*
 * Oreka -- A media capture and retrieval platform
 *
 * LiveStreamFilter Plugin
 * Author Shushant Sharan
 *
 */

#ifndef __LIVESTREAMSESSION_H__
#define __LIVESTREAMSESSION_H__

#include "VoIpSession.h"
#include <set>
#include "AudioCapturePlugin.h"
#include "Utils.h"
#include "AudioCapture.h"

class LiveStreamSessions : public OrkSingleton<LiveStreamSessions> {
    public:
        LiveStreamSessions();
        bool StartStreamNativeCallId(CStdString &nativecallid);
        bool StopStreamNativeCallId(CStdString &nativecallid);
        std::set<std::string> GetLiveCallList();
        std::set<std::string> GetStreamCallList();
        void AddToStreamCallList(CStdString &nativecallid);
        void RemoveFromStreamCallList(CStdString &nativecallid);

    private:
        VoIpSessions *voIpSessions;
        bool SessionFoundForNativeCallId(CStdString &nativecallid, VoIpSessionRef &session);
        bool NativeCallIdInStreamCallList(CStdString &nativecallid);
};

#define LiveStreamSessionsSingleton LiveStreamSessions
#endif
