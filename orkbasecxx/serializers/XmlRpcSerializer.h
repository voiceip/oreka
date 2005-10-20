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

#ifndef __XMLRPCSERIALIZER_H__
#define __XMLRPCSERIALIZER_H__
/*
#include "XmlRpc.h"
#include "messages/Message.h"
#include "serializers/Serializer.h"

using namespace XmlRpc;
*/
/** Serializer that generates and parses XMLRPC parameters from and to objects.
*/
/*
class DLL_IMPORT_EXPORT XmlRpcSerializer : public KeyValueSerializer
{
public:
	//XmlRpcSerializer(Message* message, CStdString& hostname, int tcpPort);
	XmlRpcSerializer(Object* object) : KeyValueSerializer(object){};

	void AddInt(const char* key, int value);
	void AddString(const char* key, CStdString& value);

	bool Invoke(CStdString& hostname, int tcpPort);
private:
	CStdString m_hostname;
	int m_tcpPort;
	XmlRpcValue m_params;
	XmlRpcValue m_array;
	XmlRpcValue m_ret;
};
*/
#endif

