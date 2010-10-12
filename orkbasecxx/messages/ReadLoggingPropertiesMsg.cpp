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

#include "ReadLoggingPropertiesMsg.h"
#include "LogManager.h"

#define READ_LOGGING_PROPERITES_CLASS "readloggingproperties"
#define READ_LOGGING_PROPERITES_RESPONSE_CLASS "readloggingpropertiesresponse"

void ReadLoggingPropertiesResponseMsg::Define(Serializer* s)
{
	s->BoolValue(SUCCESS_PARAM, m_success);
}

CStdString ReadLoggingPropertiesResponseMsg::GetClassName()
{
	return CStdString(READ_LOGGING_PROPERITES_RESPONSE_CLASS);
}

ObjectRef ReadLoggingPropertiesResponseMsg::NewInstance()
{
	return ObjectRef(new ReadLoggingPropertiesResponseMsg);
}

//===============================

void ReadLoggingPropertiesMsg::Define(Serializer* s)
{
	CStdString rlpClass(READ_LOGGING_PROPERITES_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, rlpClass, true);
}


CStdString ReadLoggingPropertiesMsg::GetClassName()
{
	return  CStdString(READ_LOGGING_PROPERITES_CLASS);
}

ObjectRef ReadLoggingPropertiesMsg::NewInstance()
{
	return ObjectRef(new ReadLoggingPropertiesMsg);
}

ObjectRef ReadLoggingPropertiesMsg::Process()
{
	OrkLogManager::Instance()->Initialize();	

	ReadLoggingPropertiesResponseMsg* msg = new ReadLoggingPropertiesResponseMsg;
	ObjectRef ref(msg);
	msg->m_success = true;
	return ref;
}