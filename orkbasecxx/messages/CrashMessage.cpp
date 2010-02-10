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

#include "CrashMessage.h"
#include "messages/AsyncMessage.h"

#define CRASH_CLASS "crash"

void CrashMsg::Define(Serializer* s)
{
	CStdString crashClass(CRASH_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, crashClass, true);
}

CStdString CrashMsg::GetClassName()
{
	return CStdString(CRASH_CLASS);
}

ObjectRef CrashMsg::NewInstance()
{
	return ObjectRef(new CrashMsg);
}

ObjectRef CrashMsg::Process()
{
	char *ptr;
	SimpleResponseMsg* msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	CStdString logMsg;

	ptr = NULL;
	*ptr = 0;

	logMsg.Format("Hopefully we'll crash in a bit");
	msg->m_success = true;
	msg->m_comment = logMsg;

	return ref;
}

