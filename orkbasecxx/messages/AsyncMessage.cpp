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

#include "AsyncMessage.h"

//void AsyncMessage::send(XmlRpc::XmlRpcClient& c)
//{
//	;
//}

#define SIMPLERESPONSE_CLASS "simpleresponse"
#define SUCCESS_PARAM "success"
#define SUCCESS_DEFAULT true
#define COMMENT_PARAM "comment"

SimpleResponseMsg::SimpleResponseMsg()
{
	m_success = false;
}


void SimpleResponseMsg::Define(Serializer* s)
{
	s->BoolValue(SUCCESS_PARAM, m_success);
	s->StringValue(COMMENT_PARAM, m_comment);
}

CStdString SimpleResponseMsg::GetClassName()
{
	return CStdString(SIMPLERESPONSE_CLASS);
}

ObjectRef SimpleResponseMsg::NewInstance()
{
	return ObjectRef(new SimpleResponseMsg);
}
