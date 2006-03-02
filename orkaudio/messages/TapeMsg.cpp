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

#include "Utils.h"
#include "TapeMsg.h"
#include "ConfigManager.h"

TapeMsg::TapeMsg()
{
	// Here is where default values are set
	m_timestamp = 0;
	m_direction = CaptureEvent::DirectionToString(CaptureEvent::DirUnkn);
	m_duration = 0;
	m_serviceName = CONFIG.m_serviceName;
}

void TapeMsg::Define(Serializer* s)
{
	DefineMessage(s);

	CStdString tapeMessageName(TAPE_MESSAGE_NAME);
	s->StringValue(OBJECT_TYPE_TAG, tapeMessageName, true);
	s->StringValue(REC_ID_PARAM, m_recId, true);
	s->StringValue(STAGE_PARAM, m_stage, true);
	s->StringValue(CAPTURE_PORT_PARAM, m_capturePort, true);
	s->IntValue(TIMESTAMP_PARAM, (int&)m_timestamp, true);
	s->StringValue(FILENAME_PARAM, m_fileName, true);
	s->StringValue(LOCALPARTY_PARAM, m_localParty);
	s->StringValue(LOCALENTRYPOINT_PARAM, m_localEntryPoint);
	s->StringValue(REMOTEPARTY_PARAM, m_remoteParty);
	s->StringValue(DIRECTION_PARAM, m_direction);
	s->IntValue(DURATION_PARAM, m_duration);
	s->StringValue(SERVICE_PARAM, m_serviceName);
}

void TapeMsg::Validate()
{
}

CStdString TapeMsg::GetClassName()
{
	return CStdString(TAPE_MESSAGE_NAME);
}

ObjectRef TapeMsg::NewInstance()
{
	return ObjectRef(new TapeMsg);
}


//==========================================================
TapeResponse::TapeResponse()
{
	m_deleteTape = false;
}


void TapeResponse::Define(Serializer* s)
{
	SimpleResponseMsg::Define(s);
	s->BoolValue("deletetape", m_deleteTape);
}

CStdString TapeResponse::GetClassName()
{
	return CStdString("taperesponse");
}

ObjectRef TapeResponse::NewInstance()
{
	return ObjectRef(new TapeResponse);
}

