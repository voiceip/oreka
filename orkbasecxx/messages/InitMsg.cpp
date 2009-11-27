/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */
#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning

#define _WINSOCKAPI_            // prevents the inclusion of winsock.h

#include "InitMsg.h"
#include "ConfigManager.h"

InitMsg::InitMsg()
{
	m_tcpPort = 59140;
	m_fileServePort = 8080;
	m_streamingPort = 59120;
	m_sshPort = 22;
}

void InitMsg::Define(Serializer* s)
{
	CStdString initMessageName(INIT_MESSAGE_NAME);
	s->StringValue(OBJECT_TYPE_TAG, initMessageName, true);
	s->StringValue(NAME_PARAM, m_name, true);
	s->StringValue(HOSTNAME_PARAM, m_hostname, true);
	s->StringValue(TYPE_PARAM, m_type, true);

	s->IntValue(TCP_PORT_PARAM, m_tcpPort);
	s->StringValue(PROTOCOL_PARAM, m_protocol);
	s->IntValue(FILE_SERVE_PORT_PARAM, m_fileServePort);
	s->StringValue(CONTEXT_PATH_PARAM, m_contextPath);
	s->StringValue(SERVE_PATH_PARAM, m_servePath);
	s->StringValue(ABSOLUTE_PATH_PARAM, m_absolutePath);
	s->IntValue(STREAMING_PORT_PARAM, m_streamingPort);
	s->StringValue(USERNAME_PARAM, m_username);
	s->StringValue(PASSWORD_PARAM, m_password);
	s->IntValue(SSH_PORT_PARAM, m_sshPort);

	DefineMessage(s);
}

void InitMsg::Validate()
{
}

CStdString InitMsg::GetClassName()
{
	return CStdString(INIT_MESSAGE_NAME);
}

ObjectRef InitMsg::NewInstance()
{
	return ObjectRef(new InitMsg);
}

ObjectRef InitMsg::Process()
{
	return ObjectRef();
}

