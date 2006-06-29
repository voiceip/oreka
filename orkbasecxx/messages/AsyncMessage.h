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

#ifndef __ASYNCMESSAGE_H__
#define __ASYNCMESSAGE_H__

//#include "XmlRpc.h"
#include "Message.h"

/** An AsyncMessage is an asynchronous message ("fire and forget").
    It can also be the response to a synchronous message.
*/
class DLL_IMPORT_EXPORT_ORKBASE AsyncMessage : public Message
{
//public:
//	void send(XmlRpc::XmlRpcClient& c);
};

/** A SimpleResponseMsg is used as a response when commands can just succeed or fail.
    Additionally, there is textual comment field e.g. for error messages.
*/
class DLL_IMPORT_EXPORT_ORKBASE SimpleResponseMsg : public AsyncMessage
{
public:
	SimpleResponseMsg();
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	bool m_success;
	CStdString m_comment;
};

#endif

