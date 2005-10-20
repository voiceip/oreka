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

/*
#include "XmlRpcSerializer.h"

#define XMLRPC_METHOD_NAME "exec"


void XmlRpcSerializer::AddInt(const char* key, int value)
{
	XmlRpcValue pair;
	pair[0] = key;
	pair[1] = value;
	m_array[m_numParams++] = pair;
}

void XmlRpcSerializer::AddString(const char* key, CStdString& value)
{
	XmlRpcValue pair;
	pair[0] = key;
	pair[1] = (PCSTR)value;
	m_array[m_numParams++] = pair;
}

bool XmlRpcSerializer::Invoke(CStdString& hostname, int tcpPort)
{
	m_object->Define(this);
	m_hostname = hostname;
	m_tcpPort = tcpPort;
	m_params[0] = m_array;
	XmlRpcClient client(m_hostname, m_tcpPort);
	client.execute(XMLRPC_METHOD_NAME, m_params, m_ret);
	client.close();
	return m_ret.valid();
}
*/
