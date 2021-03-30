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

#define STREAM_CLASS "stream"
#define END_CLASS "end"
#define GET_CLASS "get"
using namespace std;

void StreamMsg::Define(Serializer *s)
{
	CStdString streamClass(STREAM_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, streamClass, true);
	s->StringValue(PARTY_PARAM, m_party, false);
	s->StringValue(ORKUID_PARAM, m_orkuid, false);
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
	LiveStreamServerProxy::Singleton()->StartStream(m_party, m_orkuid, m_nativecallid);
	logMsg.Format("Starting stream for nativecallid: %s", m_nativecallid);
	msg->m_success = true;
	msg->m_comment = logMsg;

	return ref;
}

//============================

void EndMsg::Define(Serializer *s)
{
	CStdString endClass(END_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, endClass, true);
	s->StringValue(PARTY_PARAM, m_party, false);
	s->StringValue(ORKUID_PARAM, m_orkuid, false);
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
	LiveStreamServerProxy::Singleton()->EndStream(m_party, m_orkuid, m_nativecallid);
	logMsg.Format("Ending stream for nativecallid: %s", m_nativecallid);
	msg->m_success = true;
	msg->m_comment = logMsg;

	return ref;
}

//=========================

void GetMsg::Define(Serializer *s)
{
	CStdString getClass(GET_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, getClass, true);
	s->StringValue(PARTY_PARAM, m_party, false);
	s->StringValue(ORKUID_PARAM, m_orkuid, false);
	s->StringValue(NATIVE_CALLID_PARAM, m_nativecallid, false);
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
	CStdString logMsg;
	for (auto callId : LiveStreamServerProxy::Singleton()->GetStream())
	{
		msg->m_comment = msg->m_comment + callId + CStdString(",\n");
	}
	msg->m_success = true;
	return ref;
}