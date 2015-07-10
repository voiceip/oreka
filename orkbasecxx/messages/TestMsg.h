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

#ifndef __TESTMSG_H__
#define __TESTMSG_H__

#include <list>
#include "messages/SyncMessage.h"

#define FILENAME_PARAM "filename"
#define STAGE_PARAM "stage"
#define LOCALPARTY_PARAM "localparty"
#define REMOTEPARTY_PARAM "remoteparty"
#define DIRECTION_PARAM "direction"
#define LOCALENTRYPOINT_PARAM "localentrypoint"

class DLL_IMPORT_EXPORT_ORKBASE TestMsg : public SyncMessage
{
public:
	TestMsg();

	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	CStdString m_stage;
	time_t m_timestamp;
	CStdString m_fileName;
	CStdString m_capturePort;
	CStdString m_localParty;
	CStdString m_localEntryPoint;
	CStdString m_remoteParty;
	CStdString m_direction;
	CStdString m_loginString;
	std::list<CStdString> m_csv;
	time_t m_time;
};

typedef oreka::shared_ptr<TestMsg> TestMsgRef;

#endif

