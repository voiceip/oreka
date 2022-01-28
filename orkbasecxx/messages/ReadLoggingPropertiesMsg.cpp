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
#include <log4cxx/logmanager.h>


#define READ_LOGGING_PROPERITES_CLASS "reloadloggingconfig"
#define READ_LOGGING_PROPERITES_RESPONSE_CLASS "reloadloggingconfig"

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

	SimpleResponseMsg* msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	msg->m_success = true;
	return ref;
}


//============ listLoggingProperties
// iterates through the log4cxx hierarch and shows what loggers are
// configured (and at what level)

#define LIST_LOGGING_PROPERITES_CLASS "listloggingconfig"
#define LIST_LOGGING_PROPERITES_RESPONSE_CLASS "listloggingconfig"

void ListLoggingPropertiesResponseMsg::Define(Serializer* s)
{
	SimpleResponseMsg::Define(s);
	s->IntValue("count", m_count);
	s->StringValue("loggers", m_loggerInfo);
}

CStdString ListLoggingPropertiesResponseMsg::GetClassName()
{
	return CStdString(LIST_LOGGING_PROPERITES_RESPONSE_CLASS);
}

ObjectRef ListLoggingPropertiesResponseMsg::NewInstance()
{
	return ObjectRef(new ListLoggingPropertiesResponseMsg);
}

//===============================

void ListLoggingPropertiesMsg::Define(Serializer* s)
{
	CStdString rlpClass(LIST_LOGGING_PROPERITES_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, rlpClass, true);
}


CStdString ListLoggingPropertiesMsg::GetClassName()
{
	return  CStdString(LIST_LOGGING_PROPERITES_CLASS);
}

ObjectRef ListLoggingPropertiesMsg::NewInstance()
{
	return ObjectRef(new ListLoggingPropertiesMsg);
}

void LoggerInfo(LoggerPtr log, CStdString& rsp)
{
	CStdString data;

	rsp.Format("%s", log->getName());

	// logging level that this logger is running at.
	// the effective level may be inherited from a parent
	LevelPtr lvl = log->getEffectiveLevel();
	if (lvl)
	{
		lvl->toString(data);
	}
	else
	{
		data="[NONE]";
	}
	rsp += ":" + data;

	//
	// get the configured level. If this doesn't exist, then the
	// logger is running with an inherited level
	lvl = log->getLevel();
	if (!lvl) rsp+= "(Inherited)";

	rsp += "\n";
}
int  ListLoggers(CStdString& rsp)
{

	LoggerList loggers = LogManager::getCurrentLoggers();
	rsp = "\n";
	for (int i = 0; i < loggers.size(); i++)
	{
		CStdString info;
		LoggerInfo(loggers[i], info);
		rsp += info;
	}
	return loggers.size();
}

ObjectRef ListLoggingPropertiesMsg::Process()
{
	CStdString logMsg;

	ListLoggingPropertiesResponseMsg* msg = new ListLoggingPropertiesResponseMsg;
	ObjectRef ref(msg);
	msg->m_count = ListLoggers(logMsg);
	msg->m_success = true;
	msg->m_loggerInfo = logMsg;
	return ref;
}


