/*
 * Oreka -- A media capture and retrieval platform
 *
 * LiveStreamFilter Plugin
 * Author Shushant Sharan
 *
 */
#include "LiveStreamSession.h"

extern CaptureEventCallBackFunction g_captureEventCallBack;

static std::mutex s_mutex;
static log4cxx::LoggerPtr s_log = log4cxx::Logger::getLogger("plugin.livestream");
static std::set<std::string> streamCallList;

LiveStreamSessions::LiveStreamSessions() {
    voIpSessions = VoIpSessionsSingleton::instance();
}

bool LiveStreamSessions::StartStreamNativeCallId(CStdString & nativecallid) {
    MutexSentinel mutexSentinel(s_mutex);
    CStdString logMsg;
    VoIpSessionRef session;

    if (SessionFoundForNativeCallId(nativecallid, session)) {
        CaptureEventRef event(new CaptureEvent());
        event->m_type = CaptureEvent::EtKeyValue;
        event->m_key = "LiveStream";
        event->m_value = "start";
        g_captureEventCallBack(event, session->m_capturePort);

        logMsg.Format("[%s] StartStreamNativeCallId: Started capture, nativecallid:%s", session->m_trackingId, nativecallid);

        return NativeCallIdInStreamCallList(nativecallid);
    } else {
        logMsg.Format("StartStreamNativeCallId: No session has native callid:%s", nativecallid);
    }

    LOG4CXX_INFO(s_log, logMsg);

    return false;
}

bool LiveStreamSessions::StopStreamNativeCallId(CStdString & nativecallid) {
    MutexSentinel mutexSentinel(s_mutex);
    CStdString logMsg;
    VoIpSessionRef session;

    if (NativeCallIdInStreamCallList(nativecallid) && SessionFoundForNativeCallId(nativecallid, session)) {
        CaptureEventRef event(new CaptureEvent());
        event->m_type = CaptureEvent::EtKeyValue;
        event->m_key = "LiveStream";
        event->m_value = "stop";
        g_captureEventCallBack(event, session->m_capturePort);

        logMsg.Format("[%s] StopStreamNativeCallId: Stoped capture, nativecallid:%s", session->m_trackingId, nativecallid);
        return true;
    } else {
        logMsg.Format("StopStreamNativeCallId: No session has native callid:%s", nativecallid);
    }

    LOG4CXX_INFO(s_log, logMsg);

    return false;
}

std::set<std::string> LiveStreamSessions::GetLiveCallList() {
    MutexSentinel mutexSentinel(s_mutex);
    std::set<std::string> liveCallList;
    try {
        auto sessions = voIpSessions->getByIpAndPort();
        for (auto & p: sessions) {
            liveCallList.insert(p.second->m_callId);
        }  
    } catch (const std::exception & ex) {
        CStdString logMsg;
        logMsg.Format("LiveStreamSession::Failed to get live call list:%s", ex.what());
        LOG4CXX_ERROR(s_log, logMsg);
    }

    return liveCallList;
}

std::set<std::string> LiveStreamSessions::GetStreamCallList() {
    return streamCallList;
}

void LiveStreamSessions::AddToStreamCallList(CStdString & nativecallid) {
    streamCallList.insert(nativecallid);
}

void LiveStreamSessions::RemoveFromStreamCallList(CStdString & nativecallid) {
    streamCallList.erase(nativecallid);
}

bool LiveStreamSessions::NativeCallIdInStreamCallList(CStdString & nativecallid) {
    return streamCallList.find(nativecallid) != streamCallList.end();
}

bool LiveStreamSessions::SessionFoundForNativeCallId(CStdString & nativecallid, VoIpSessionRef & session) {
    bool found = false;
    try {
        auto sessions = voIpSessions->getByIpAndPort();
        for (auto & p: sessions) {
            session = p.second;
            if (session->NativeCallIdMatches(nativecallid)) {
                found = true;
                break;
            }
        }  
    } catch (const std::exception & ex) {
        CStdString logMsg;
        logMsg.Format("LiveStreamSession::Failure while finding active session for nativeCallId :%s", ex.what());
        LOG4CXX_ERROR(s_log, logMsg);
    }
    return found;
}
