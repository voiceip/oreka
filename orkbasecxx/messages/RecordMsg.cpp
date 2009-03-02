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

void PauseMsg::Define(Serializer* s)
{
	CStdString pauseClass(PAUSE_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, pauseClass, true);
	s->StringValue(PARTY_PARAM, m_party, false);
	s->StringValue(ORKUID_PARAM, m_orkuid, false);
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

	logMsg.Format("Pausing capture for party:%s orkuid:%s", m_party, m_orkuid);
	CapturePluginProxy::Singleton()->PauseCapture(m_party, m_orkuid);
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

	CapturePluginProxy::Singleton()->StartCapture(m_party, m_orkuid);
	logMsg.Format("Starting capture for party:%s orkuid:%s", m_party, m_orkuid);
	msg->m_success = true;
	msg->m_comment = logMsg;

	return ref;
}

