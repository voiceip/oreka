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

#include "PingMsg.h"

#define PING_CLASS "ping"
#define PING_RESPONSE_CLASS "pingresponse"

void PingResponseMsg::Define(Serializer* s)
{
	s->BoolValue(SUCCESS_PARAM, m_success);
}

CStdString PingResponseMsg::GetClassName()
{
	return CStdString(PING_RESPONSE_CLASS);
}

ObjectRef PingResponseMsg::NewInstance()
{
	return ObjectRef(new PingResponseMsg);
}

//===============================

void PingMsg::Define(Serializer* s)
{
	CStdString pingClass(PING_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, pingClass, true);
}


CStdString PingMsg::GetClassName()
{
	return  CStdString(PING_CLASS);
}

ObjectRef PingMsg::NewInstance()
{
	return ObjectRef(new PingMsg);
}

ObjectRef PingMsg::Process()
{
	PingResponseMsg* msg = new PingResponseMsg;
	ObjectRef ref(msg);
	msg->m_success = true;
	return ref;
}

