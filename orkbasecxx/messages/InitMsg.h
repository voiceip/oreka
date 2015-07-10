/*
 * RfbRecorder -- A Simple RFB recording program
 *
 * Copyright (C) 2007, orecx LLC
 *
 * http://www.orecx.com/
 *
 */
#ifndef __INITMSG_H__
#define __INITMSG_H__ 1

#include "messages/SyncMessage.h"
#include "messages/AsyncMessage.h"

#define INIT_MESSAGE_NAME "init"
#define NAME_PARAM "name"
#define HOSTNAME_PARAM "hostname"
#define TYPE_PARAM "servicetype"
#define TCP_PORT_PARAM "tcpport"
#define PROTOCOL_PARAM "protocol"
#define FILE_SERVE_PORT_PARAM "fileserveport"
#define CONTEXT_PATH_PARAM "contextpath"
#define SERVE_PATH_PARAM "servepath"
#define ABSOLUTE_PATH_PARAM "absolutepath"
#define STREAMING_PORT_PARAM "streamingport"
#define USERNAME_PARAM "username"
#define PASSWORD_PARAM "password"
#define SSH_PORT_PARAM "sshport"

class DLL_IMPORT_EXPORT_ORKBASE InitMsg : public SyncMessage
{
public:
	InitMsg();

	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_name;
	CStdString m_hostname;
	CStdString m_type;
	int m_tcpPort;
	CStdString m_protocol;
	int m_fileServePort;
	CStdString m_contextPath;
	CStdString m_servePath;
	CStdString m_absolutePath;
	int m_streamingPort;
	CStdString m_username;
	CStdString m_password;
	int m_sshPort;
};
typedef oreka::shared_ptr<InitMsg> InitMsgRef;

#endif
