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

#include "AddTagMsg.h"
#include "messages/TapeMsg.h"
#define ADDTAG_CLASS "addtag"
#define TAG_TYPE_PARAM "tagtype"
#define TAG_TEXT_PARAM "tagtext"
#define DTAG_OFFSET_MS_PARAM "offsetmsec"
AddTagMsg::AddTagMsg()
{
	m_success = false;
}

void AddTagMsg::Define(Serializer* s)
{
	CStdString userQueryClass(ADDTAG_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, userQueryClass, true);
	s->StringValue(LOCALPARTY_PARAM, m_party, false);
	s->StringValue(ORKUID_PARAM, m_orkuid, false);
	s->StringValue(TAG_TYPE_PARAM, m_tagType, false);
	s->StringValue(TAG_TEXT_PARAM, m_tagText, false);
	s->StringValue(DTAG_OFFSET_MS_PARAM, m_dtagOffsetMs, false);
}

void AddTagMsg::Validate()
{
}

CStdString AddTagMsg::GetClassName()
{
	return  CStdString(ADDTAG_CLASS);
}

ObjectRef AddTagMsg::NewInstance()
{
	return ObjectRef(new AddTagMsg);
}

ObjectRef AddTagMsg::Process()
{
	m_success = true;
	return ObjectRef();
}

