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

#ifndef __LIVESTREAMMSG_H__
#define __LIVESTREAMMSG_H__

#include "messages/SyncMessage.h"
#include "AudioCapture.h"
#include "CapturePluginProxy.h"
using namespace std;

class DLL_IMPORT_EXPORT_ORKBASE StreamMsg : public SyncMessage
{
public:
	void Define(Serializer *s);
	inline void Validate(){};

	void EnsureValidSide();
	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_party;
	CStdString m_orkuid;
	CStdString m_nativecallid;
};

class DLL_IMPORT_EXPORT_ORKBASE EndMsg : public SyncMessage
{
public:
	void Define(Serializer *s);
	inline void Validate(){};

	void EnsureValidSide();
	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_party;
	CStdString m_orkuid;
	CStdString m_nativecallid;
};

class DLL_IMPORT_EXPORT_ORKBASE GetMsg : public SyncMessage
{
public:
	void Define(Serializer *s);
	inline void Validate(){};

	void EnsureValidSide();
	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_party;
	CStdString m_orkuid;
	CStdString m_nativecallid;
};

#endif
