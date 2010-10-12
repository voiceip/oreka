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

#ifndef __READLOGGINGPROPERITESMSG_H__
#define __READLOGGINGPROPERITESMSG_H__

#include "messages/SyncMessage.h"
#include "messages/AsyncMessage.h"


class DLL_IMPORT_EXPORT_ORKBASE ReadLoggingPropertiesResponseMsg : public AsyncMessage
{
public:
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	bool m_success;
};

class DLL_IMPORT_EXPORT_ORKBASE ReadLoggingPropertiesMsg : public SyncMessage
{
public:
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();
};

#endif

