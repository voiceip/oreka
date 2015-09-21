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

#include "LogManager.h"
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


MessageRef AddTagMsg::CreateResponse() {
	return oreka::shared_ptr<SimpleResponseMsg>(new SimpleResponseMsg());
}

void AddTagMsg::HandleResponse(MessageRef responseRef) {
	// do not do anything with the response
}

bool AddTagMsg::IsRealtime() {
	return false;
}

MessageRef AddTagMsg::Clone() {
	oreka::shared_ptr<AddTagMsg> clone(new AddTagMsg());

	clone->m_party        = m_party; 
	clone->m_orkuid       = m_orkuid;
	clone->m_tagType      = m_tagType;
	clone->m_tagText      = m_tagText;
	clone->m_dtagOffsetMs = m_dtagOffsetMs;
	clone->m_success      = m_success;

	return clone;
}
