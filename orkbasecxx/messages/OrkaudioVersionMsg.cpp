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

#include "OrkaudioVersionMsg.h"
#include "LogManager.h"
#include <log4cxx/logmanager.h>


#define ORKAUDIO_VERSION_CLASS "orkaudioversion"
#define ORKAUDIO_VERSION_RESPONSE_CLASS "orkaudioversionresponse"


static const char* __version = "TBD";
const char*  OrkAudioVersion() { return __version; };

void RegisterOrkaudioVersion(const char *ver) { __version = ver; };

//===============================

void OrkaudioVersionMsg::Define(Serializer* s)
{
	CStdString rlpClass(ORKAUDIO_VERSION_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, rlpClass, true);
}


CStdString OrkaudioVersionMsg::GetClassName()
{
	return  CStdString(ORKAUDIO_VERSION_CLASS);
}

ObjectRef OrkaudioVersionMsg::NewInstance()
{
	return ObjectRef(new OrkaudioVersionMsg);
}


void OrkaudioVersionResponseMsg::Define(Serializer* s)
{
	SimpleResponseMsg::Define(s);
	s->StringValue("version", m_version);
}

CStdString OrkaudioVersionResponseMsg::GetClassName()
{
	return CStdString(ORKAUDIO_VERSION_RESPONSE_CLASS);
}

ObjectRef OrkaudioVersionResponseMsg::NewInstance()
{
	return ObjectRef(new OrkaudioVersionResponseMsg);
}


ObjectRef OrkaudioVersionMsg::Process()
{
	CStdString logMsg;
	OrkaudioVersionResponseMsg* msg = new OrkaudioVersionResponseMsg;
	ObjectRef ref(msg);
	msg->m_success = true;
	msg->m_version = OrkAudioVersion();
	return ref;
}


