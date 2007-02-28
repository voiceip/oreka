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

#include "ace/INET_Addr.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Stream.h"
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

//===============================
#define TCP_PING_CLASS "tcpping"

TcpPingMsg::TcpPingMsg()
{
	m_port = 0;
}


void TcpPingMsg::Define(Serializer* s)
{
	CStdString tcpPingClass(TCP_PING_CLASS);
	s->StringValue(OBJECT_TYPE_TAG, tcpPingClass, true);
	s->StringValue("hostname", m_hostname, true);
	s->IntValue("port", (int&)m_port, true);
}


CStdString TcpPingMsg::GetClassName()
{
	return  CStdString(TCP_PING_CLASS);
}

ObjectRef TcpPingMsg::NewInstance()
{
	return ObjectRef(new TcpPingMsg);
}

ObjectRef TcpPingMsg::Process()
{
	bool success = true;
	CStdString logMsg;
	ACE_SOCK_Connector  connector;
	ACE_SOCK_Stream peer;
	ACE_INET_Addr peer_addr;
	ACE_Time_Value aceTimeout (5);

	if (peer_addr.set (m_port, (PCSTR)m_hostname) == -1)
	{
		logMsg.Format("peer_addr.set()  errno=%d", errno);
		success = false;
	}
	else if (connector.connect (peer, peer_addr, &aceTimeout) == -1)
	{
		if (errno == ETIME)
		{
		}
		logMsg.Format("connector.connect()  errno=%d", errno);
		success =  false;
	}

	SimpleResponseMsg* msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	msg->m_success = success;
	msg->m_comment = logMsg;
	return ref;
}

