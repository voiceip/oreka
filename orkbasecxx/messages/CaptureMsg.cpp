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

#include "Utils.h"
#include "CaptureMsg.h"
//#include "LogManager.h"
//#include "CapturePluginProxy.h"

#define CAPTURE_CLASS "capture"
#define CAPTURE_RESPONSE_CLASS "captureresponse"
#define CAPTURE_STATE_PARAM "state"
#define COMMENT_PARAM "comment"

void CaptureResponseMsg::Define(Serializer* s)
{
	s->BoolValue(SUCCESS_PARAM, m_success);
	s->StringValue(COMMENT_PARAM, m_comment);
}

CStdString CaptureResponseMsg::GetClassName()
{
	return CStdString(CAPTURE_RESPONSE_CLASS);
}

ObjectRef CaptureResponseMsg::NewInstance()
{
	return ObjectRef(new CaptureResponseMsg);
}

//===============================

void CaptureMsg::Define(Serializer* s)
{
	CStdString captureClass(CAPTURE_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, captureClass, true);
	s->StringValue(CAPTURE_PORT_PARAM, m_capturePort, true);
	s->EnumValue(CAPTURE_STATE_PARAM, (int&)m_eventType, CaptureEvent::EventTypeToEnum, CaptureEvent::EventTypeToString, true);
}


CStdString CaptureMsg::GetClassName()
{
	return  CStdString(CAPTURE_CLASS);
}

ObjectRef CaptureMsg::NewInstance()
{
	return ObjectRef(new CaptureMsg);
}

ObjectRef CaptureMsg::Process()
{
	CaptureResponseMsg* msg = new CaptureResponseMsg;
	ObjectRef ref(msg);

	if(m_eventType == CaptureEvent::EtStart)
	{
		//CapturePluginProxySingleton::instance()->StartCapture(m_capturePort);
		msg->m_success = true;
	}
	else if(m_eventType == CaptureEvent::EtStop)
	{
		//CapturePluginProxySingleton::instance()->StopCapture(m_capturePort);
		msg->m_success = true;
	}
	else
	{
		msg->m_success = false;
		msg->m_comment =  CAPTURE_STATE_PARAM;
		msg->m_comment += " needs to be start or stop";
	}
	return ref;
}

