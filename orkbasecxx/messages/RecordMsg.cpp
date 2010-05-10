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

#include "RecordMsg.h"
#include "messages/AsyncMessage.h"
#include "CapturePluginProxy.h"

#define RECORD_CLASS "record"
#define PAUSE_CLASS "pause"
#define STOP_CLASS "stop"

void PauseMsg::Define(Serializer* s)
{
	CStdString pauseClass(PAUSE_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, pauseClass, true);
	s->StringValue(PARTY_PARAM, m_party, false);
	s->StringValue(ORKUID_PARAM, m_orkuid, false);
	s->StringValue(NATIVE_CALLID_PARAM, m_nativecallid, false);
}

CStdString PauseMsg::GetClassName()
{
	return  CStdString(PAUSE_CLASS);
}

ObjectRef PauseMsg::NewInstance()
{
	return ObjectRef(new PauseMsg);
}

ObjectRef PauseMsg::Process()
{
	SimpleResponseMsg* msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	CStdString logMsg;

	CapturePluginProxy::Singleton()->PauseCapture(m_party, m_orkuid, m_nativecallid);
	logMsg.Format("Pausing capture for party:%s orkuid:%s nativecallid:%s", m_party, m_orkuid, m_nativecallid);
	msg->m_success = true;
	msg->m_comment = logMsg;

	return ref;
}

//===================================================

void StopMsg::Define(Serializer* s)
{
	CStdString stopClass(STOP_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, stopClass, true);
	s->StringValue(PARTY_PARAM, m_party, false);
	s->StringValue(ORKUID_PARAM, m_orkuid, false);
	s->StringValue(NATIVE_CALLID_PARAM, m_nativecallid, false);
}

CStdString StopMsg::GetClassName()
{
	return  CStdString(STOP_CLASS);
}

ObjectRef StopMsg::NewInstance()
{
	return ObjectRef(new StopMsg);
}

ObjectRef StopMsg::Process()
{
	SimpleResponseMsg* msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	CStdString logMsg;
	CStdString qos;

	CapturePluginProxy::Singleton()->StopCapture(m_party, m_orkuid, m_nativecallid, qos);
	logMsg.Format("Stopping capture for party:%s orkuid:%s nativecallid:%s %s", m_party, m_orkuid, m_nativecallid, qos);
	msg->m_success = true;
	msg->m_comment = logMsg;

	return ref;
}

//===================================================

void RecordMsg::Define(Serializer* s)
{
	CStdString recordClass(RECORD_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, recordClass, true);
	s->StringValue(PARTY_PARAM, m_party, false);
	s->StringValue(ORKUID_PARAM, m_orkuid, false);
	s->StringValue(NATIVE_CALLID_PARAM, m_nativecallid, false);
	s->StringValue(SIDE_PARAM, m_side, false);
}

CStdString RecordMsg::GetClassName()
{
	return  CStdString(RECORD_CLASS);
}

ObjectRef RecordMsg::NewInstance()
{
	return ObjectRef(new RecordMsg);
}

ObjectRef RecordMsg::Process()
{
	SimpleResponseMsg* msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	CStdString logMsg;

	EnsureValidSide();

	CapturePluginProxy::Singleton()->StartCapture(m_party, m_orkuid, m_nativecallid, m_side);
	logMsg.Format("Starting capture for party:%s orkuid:%s nativecallid:%s side:%s", m_party, m_orkuid, m_nativecallid, m_side);
	msg->m_success = true;
	msg->m_comment = logMsg;

	return ref;
}

void RecordMsg::EnsureValidSide()
{
	if(CaptureEvent::AudioKeepDirectionToEnum(m_side) == CaptureEvent::AudioKeepDirectionInvalid)
	{
		m_side = "both";
		return;
	}

	return;
}
