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

#ifndef __ADDTAGMSG_H__
#define __ADDTAGMSG_H__

#include "messages/SyncMessage.h"
#include "messages/AsyncMessage.h"

class DLL_IMPORT_EXPORT_ORKBASE AddTagMsg : public SyncMessage
{
public:
	AddTagMsg();
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_party;
	CStdString m_orkuid;
	CStdString m_tagType;
	CStdString m_tagText;
	CStdString m_dtagOffsetMs;
	bool m_success;

};
typedef boost::shared_ptr<AddTagMsg> AddTagMsgRef;

#endif
