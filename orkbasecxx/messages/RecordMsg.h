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

#ifndef __RECORDMSG_H__
#define __RECORDMSG_H__

#include "messages/SyncMessage.h"
#include "AudioCapture.h"

class DLL_IMPORT_EXPORT_ORKBASE RecordMsg : public SyncMessage
{
public:
	void Define(Serializer* s);
	inline void Validate() {};

	void EnsureValidSide();
	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_party;
	CStdString m_orkuid;
	CStdString m_nativecallid;
	CStdString m_side;
};

class DLL_IMPORT_EXPORT_ORKBASE PauseMsg : public SyncMessage
{
public:
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_party;
	CStdString m_orkuid;
	CStdString m_nativecallid;
};

class DLL_IMPORT_EXPORT_ORKBASE StopMsg : public SyncMessage
{
public:
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_party;
	CStdString m_orkuid;
	CStdString m_nativecallid;
};

#endif

