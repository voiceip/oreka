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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning

#include "serializers/XmlRpcSerializer.h"
#include "Message.h"


char Message::m_hostname[HOSTNAME_BUF_LEN] = "";

Message::Message()
{
	m_creationTime = time(NULL);
	ACE_OS::hostname(m_hostname, HOSTNAME_BUF_LEN);
}


bool Message::InvokeXmlRpc(CStdString& hostname, int tcpPort)
{
/*
	XmlRpcSerializer serializer(this);
	return serializer.Invoke(hostname, tcpPort);
	//serializerRef.reset(new XmlRpcSerializer(this));
	//serializerRef->Invoke(hostname, tcpPort);
*/
	return true;
}

