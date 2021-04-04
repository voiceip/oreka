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
#pragma warning(disable : 4786) // disables truncated symbols in browse-info warning
#define _WINSOCKAPI_            // prevents the inclusion of winsock.h

#include "Utils.h"
#include "AudioCapture.h"
#include "LiveStreamSession.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include <set>

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;

#define CONFERENCE_TRANSFER_TRACKING_TAG_KEY "orig-orkuid"

LiveStreamSessions::LiveStreamSessions()
{
    voIpSessions = VoIpSessionsSingleton::instance();
}

CStdString LiveStreamSessions::StartStreamNativeCallId(CStdString &nativecallid)
{
    std::map<unsigned long long, VoIpSessionRef>::iterator pair;
    bool found = false;
    CStdString logMsg;
    VoIpSessionRef session;
    CStdString orkUid = CStdString("");

    for (pair = voIpSessions->getByIpAndPort().begin(); pair != voIpSessions->getByIpAndPort().end() && found == false; pair++)
    {
        session = pair->second;

        if (session->NativeCallIdMatches(nativecallid))
        {
            session->m_keepRtp = true;
            found = true;
            orkUid = session->GetOrkUid();
        }
    }

    if (found)
    {
        CaptureEventRef event(new CaptureEvent());
        event->m_type = CaptureEvent::EtKeyValue;
        event->m_key = "LiveStream";
        event->m_value = "start";
        g_captureEventCallBack(event, session->m_capturePort);

        logMsg.Format("[%s] StartStreamNativeCallId: Started capture, nativecallid:%s", session->m_trackingId, nativecallid);
    }
    else
    {
        logMsg.Format("StartStreamNativeCallId: No session has native callid:%s", nativecallid);
    }

    LOG4CXX_INFO(voIpSessions->getLogger(), logMsg);

    return orkUid;
}

CStdString LiveStreamSessions::EndStreamNativeCallId(CStdString &nativecallid)
{
    std::map<unsigned long long, VoIpSessionRef>::iterator pair;
    bool found = false;
    CStdString logMsg;
    VoIpSessionRef session;
    CStdString orkUid = CStdString("");

    for (pair = voIpSessions->getByIpAndPort().begin(); pair != voIpSessions->getByIpAndPort().end() && found == false; pair++)
    {
        session = pair->second;

        if (session->NativeCallIdMatches(nativecallid))
        {
            session->m_keepRtp = true;
            found = true;
            orkUid = session->GetOrkUid();
        }
    }

    if (found)
    {
        CaptureEventRef event(new CaptureEvent());
        event->m_type = CaptureEvent::EtKeyValue;
        event->m_key = "LiveStream";
        event->m_value = "end";
        g_captureEventCallBack(event, session->m_capturePort);

        logMsg.Format("[%s] EndStreamNativeCallId: Ended capture, nativecallid:%s", session->m_trackingId, nativecallid);
    }
    else
    {
        logMsg.Format("EndStreamNativeCallId: No session has native callid:%s", nativecallid);
    }

    LOG4CXX_INFO(voIpSessions->getLogger(), logMsg);

    return orkUid;
}

std::set<std::string> LiveStreamSessions::GetStreamNativeCallId()
{
    std::map<unsigned long long, VoIpSessionRef>::iterator pair;
    bool found = false;
    CStdString logMsg;
    VoIpSessionRef session;
    std::set<std::string> callList;

    for (pair = voIpSessions->getByIpAndPort().begin(); pair != voIpSessions->getByIpAndPort().end() && found == false; pair++)
    {
        session = pair->second;
        callList.insert(session->m_callId);
    }
    return callList;
}
