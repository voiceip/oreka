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

#ifndef __CAPTUREMSG_H__
#define __CAPTUREMSG_H__

#include "messages/SyncMessage.h"
#include "messages/AsyncMessage.h"
#include "AudioCapture.h"


class DLL_IMPORT_EXPORT_ORKBASE CaptureResponseMsg : public AsyncMessage
{
public:
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	bool m_success;
	CStdString m_comment;
};

class DLL_IMPORT_EXPORT_ORKBASE CaptureMsg : public SyncMessage
{
public:
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CaptureEvent::EventTypeEnum m_eventType;
	CStdString m_capturePort;
};

#endif

