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

#include "Utils.h"
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
	apr_pool_create(&m_loc_pool, OrkAprSingleton::GetInstance()->GetAprMp());

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
	apr_status_t rt;
	apr_sockaddr_t* sa;
	apr_socket_t* socket;
	if ((rt = apr_sockaddr_info_get(&sa, (PCSTR)m_hostname, APR_INET, m_port, 0, m_loc_pool)) != APR_SUCCESS)
	{
		logMsg.Format("apr_sockaddr_info_get failed: %s", AprGetErrorMsg(rt));
		success =  false;
	}
	else if ((rt = apr_socket_create(&socket, sa->family, SOCK_STREAM, APR_PROTO_TCP, m_loc_pool)) != APR_SUCCESS)
	{
		logMsg.Format("apr_socket_create failed: %s", AprGetErrorMsg(rt));
		success =  false;
	}
	else
	{
		apr_socket_opt_set(socket, APR_SO_NONBLOCK, 0);
		apr_interval_time_t to = 5*1000*1000;
		apr_socket_timeout_set(socket, to);
		if((rt = apr_socket_connect(socket, sa)) != APR_SUCCESS)
		{
			logMsg.Format("apr_socket_connect failed: %s", AprGetErrorMsg(rt));
			success =  false;
		}
	}


	SimpleResponseMsg* msg = new SimpleResponseMsg;
	ObjectRef ref(msg);
	msg->m_success = success;
	msg->m_comment = logMsg;
	return ref;
}

