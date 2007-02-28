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

#ifndef __PINGMSG_H__
#define __PINGMSG_H__

#include "messages/SyncMessage.h"
#include "messages/AsyncMessage.h"


class DLL_IMPORT_EXPORT_ORKBASE PingResponseMsg : public AsyncMessage
{
public:
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	bool m_success;
};

class DLL_IMPORT_EXPORT_ORKBASE PingMsg : public SyncMessage
{
public:
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();
};

class DLL_IMPORT_EXPORT_ORKBASE TcpPingMsg : public SyncMessage
{
public:
	TcpPingMsg();

	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_hostname;
	int m_port;
};

#endif

