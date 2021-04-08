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

#include "StreamMsg.h"
#include "messages/AsyncMessage.h"
#include "LiveStreamServerProxy.h"
#include <set>
#include "ConfigManager.h"

#define STREAM_CLASS "stream"
#define END_CLASS "end"
#define GET_CLASS "get"
using namespace std;

void StreamMsg::Define(Serializer *s)
{
	CStdString streamClass(STREAM_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, streamClass, true);
	s->StringValue(NATIVE_CALLID_PARAM, m_nativecallid, false);
}

CStdString StreamMsg::GetClassName()
{
	return CStdString(STREAM_CLASS);
}

ObjectRef StreamMsg::NewInstance()
{
	return ObjectRef(new StreamMsg);
}

ObjectRef StreamMsg::Process()
{
	SimpleResponseMsg *msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	CStdString logMsg;
	CStdString response;
	CStdString url;
	CStdString status;
	msg->m_success = true;
	try
	{
		if (LiveStreamServerProxy::Singleton()->StartStream(m_nativecallid))
		{
			logMsg.Format("Starting stream for nativecallid: %s", m_nativecallid);
			LOG4CXX_INFO(LOG.rootLog, logMsg);
			url = "rtmp://" + CONFIG.m_rtmpServerEndpoint + ":" + CONFIG.m_rtmpServerPort + "/live/" + m_nativecallid;
			status = "true";
		}
	}
	catch (const std::exception &ex)
	{
		CStdString logMsg;
		logMsg.Format("Failed to start stream for nativecallid:%s", m_nativecallid);
		LOG4CXX_ERROR(LOG.rootLog, logMsg);
		msg->m_success = false;
	}

	response = "{\"url\": \"" + url + CStdString("\", \"status\":\"" + status + "\"}");
	msg->m_comment = response;

	return ref;
}

//============================

void EndMsg::Define(Serializer *s)
{
	CStdString endClass(END_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, endClass, true);
	s->StringValue(NATIVE_CALLID_PARAM, m_nativecallid, false);
}

CStdString EndMsg::GetClassName()
{
	return CStdString(END_CLASS);
}

ObjectRef EndMsg::NewInstance()
{
	return ObjectRef(new EndMsg);
}

ObjectRef EndMsg::Process()
{
	SimpleResponseMsg *msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	CStdString logMsg;
	CStdString response;
	CStdString url;
	CStdString status;
	msg->m_success = true;

	try
	{
		if (LiveStreamServerProxy::Singleton()->EndStream(m_nativecallid))
		{
			logMsg.Format("Ending stream for nativecallid: %s", m_nativecallid);
			LOG4CXX_INFO(LOG.rootLog, logMsg);
			url = "rtmp://" + CONFIG.m_rtmpServerEndpoint + ":" + CONFIG.m_rtmpServerPort + "/live/" + m_nativecallid;
			status = "false";
		}
	}
	catch (const std::exception &ex)
	{
		CStdString logMsg;
		logMsg.Format("Failed to end stream for nativecallid:%s", m_nativecallid);
		LOG4CXX_ERROR(LOG.rootLog, logMsg);
		msg->m_success = false;
	}

	response = "{\"url\": \"" + url + CStdString("\", \"status\":\"" + status + "\"}");
	msg->m_comment = response;

	return ref;
}

//=========================

void GetMsg::Define(Serializer *s)
{
	CStdString getClass(GET_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, getClass, true);
}

CStdString GetMsg::GetClassName()
{
	return CStdString(GET_CLASS);
}

ObjectRef GetMsg::NewInstance()
{
	return ObjectRef(new GetMsg);
}

ObjectRef GetMsg::Process()
{
	SimpleResponseMsg *msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	CStdString liveCalls;
	msg->m_success = true;
	try
	{
		for (auto callId : LiveStreamServerProxy::Singleton()->GetStream())
		{
			if (callId.length() > 0)
				liveCalls = liveCalls + CStdString("\"") + callId + CStdString("\",");
		}
		liveCalls = "{\"liveCalls\": [" + liveCalls.substr(0, liveCalls.length() - 1) + "]}";
	}
	catch (const std::exception &ex)
	{
		CStdString logMsg;
		logMsg.Format("Failed to get livestream");
		LOG4CXX_ERROR(LOG.rootLog, logMsg);
		msg->m_success = false;
	}

	msg->m_comment = liveCalls;

	return ref;
}