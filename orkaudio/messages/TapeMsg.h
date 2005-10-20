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

#ifndef __TAPEMSG_H__
#define __TAPEMSG_H__

#include "messages/SyncMessage.h"
#include "AudioTape.h"

#define TAPE_MESSAGE_NAME "tape"
#define FILENAME_PARAM "filename"
#define STAGE_PARAM "stage"
#define LOCALPARTY_PARAM "localparty"
#define REMOTEPARTY_PARAM "remoteparty"
#define DIRECTION_PARAM "direction"
#define LOCALENTRYPOINT_PARAM "localentrypoint"
#define DURATION_PARAM "duration"
#define SERVICE_PARAM "service"

class TapeMsg : public SyncMessage
{
public:
	TapeMsg();

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
	int m_duration;
	CStdString m_serviceName;
};

typedef boost::shared_ptr<TapeMsg> TapeMsgRef;

#endif

