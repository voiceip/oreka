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

void RecordMsg::Define(Serializer* s)
{
	CStdString recordClass(RECORD_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, recordClass, true);
	s->StringValue(PARTY_PARAM, m_party, true);
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

	logMsg.Format("Starting capture for %s", m_party);
	CapturePluginProxy::Singleton()->StartCapture(m_party);
	msg->m_success = true;
	msg->m_comment = logMsg;

	return ref;
}

