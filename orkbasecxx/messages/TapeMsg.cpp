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
#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Utils.h"
#include "TapeMsg.h"
#include "ConfigManager.h"

TapeMsg::TapeMsg()
{
	// Here is where default values are set
	m_timestamp = 0;
	m_direction = CaptureEvent::DirectionToString(CaptureEvent::DirUnkn);
	//m_localSide = CaptureEvent::LocalSideToString(CaptureEvent::LocalSideUnkn);
	m_audioKeepDirection = CaptureEvent::AudioKeepDirectionToString(CaptureEvent::AudioKeepDirectionBoth);
	m_duration = 0;
	m_serviceName = CONFIG.m_serviceName;
}

void TapeMsg::Define(Serializer* s)
{
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

	s->StringValue(LOCAL_IP_PARAM, m_localIp);
	s->StringValue(REMOTE_IP_PARAM, m_remoteIp);
	//s->StringValue(LOCAL_MAC_PARAM, m_localMac);
	//s->StringValue(REMOTE_MAC_PARAM, m_remoteMac);
	s->StringValue(NATIVE_CALLID_PARAM, m_nativeCallId);

	s->CsvMapValue(TAGS_PARAM, m_tags);
	s->BoolValue(ON_DEMAND_PARAM, m_onDemand);
	//s->StringValue(LOCALSIDE_PARAM, m_localSide);
	s->StringValue(AUDIOKEEPDIRECTION_PARAM, m_audioKeepDirection);


	DefineMessage(s);
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

ObjectRef TapeMsg::Process()
{
	return ObjectRef();
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

//====================================================================
//TapeResponseFwd::TapeResponseFwd()
//{
//	m_boolean2 = false;
//}
//
//
//void TapeResponseFwd::Define(Serializer* s)
//{	
//	TapeResponse::Define(s);
//	s->BoolValue("boolean2", m_boolean2);
//}
//
//ObjectRef TapeResponseFwd::NewInstance()
//{
//	return ObjectRef(new TapeResponseFwd);
//}

//====================================================================
TapeTagMsg::TapeTagMsg()
{
	// Here is where default values are set
}

void TapeTagMsg::Define(Serializer* s)
{
	CStdString tapeMessageName("tapetagmsg");
	s->StringValue(OBJECT_TYPE_TAG, tapeMessageName, true);
	DefineMessage(s);
}

void TapeTagMsg::Validate()
{
}

CStdString TapeTagMsg::GetClassName()
{
	return CStdString("tapetagmsg");
}

ObjectRef TapeTagMsg::NewInstance()
{
	return ObjectRef(new TapeTagMsg);
}
