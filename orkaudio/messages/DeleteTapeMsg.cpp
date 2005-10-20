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

#include "DeleteTapeMsg.h"
#include "messages/AsyncMessage.h"

#define DELETE_TAPE_CLASS "deletetape"

void DeleteTapeMsg::Define(Serializer* s)
{
	CStdString deleteTapeClass(DELETE_TAPE_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, deleteTapeClass, true);
	s->StringValue(FILENAME_PARAM, m_filename, true);
}


CStdString DeleteTapeMsg::GetClassName()
{
	return  CStdString(DELETE_TAPE_CLASS);
}

ObjectRef DeleteTapeMsg::NewInstance()
{
	return ObjectRef(new DeleteTapeMsg);
}

ObjectRef DeleteTapeMsg::Process()
{
	SimpleResponseMsg* msg = new SimpleResponseMsg;
	ObjectRef ref(msg);

	// Check that the audio file to delete is actually an audio file
	if(m_filename.Find('/') != -1 && (m_filename.Find(".pcm") != -1 || m_filename.Find(".wav") != -1 ))
	{
		if (ACE_OS::unlink((PCSTR)m_filename) == -1)
		{
			msg->m_success = false;
			msg->m_comment = "could not delete file";
		}

	}
	else
	{
		msg->m_success = false;
		msg->m_comment = "filename not valid";
	}

	return ref;
}

