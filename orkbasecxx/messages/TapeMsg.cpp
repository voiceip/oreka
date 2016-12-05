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
#include "LogManager.h"
#include "CapturePluginProxy.h"

TapeMsg::TapeMsg() : m_live(false)
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
	s->BoolValue("live", m_live);


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

MessageRef TapeMsg::CreateResponse() {
	return TapeResponseRef(new TapeResponse());
}

bool TapeMsg::IsValid() {
	if(m_recId.length() == 0) {
		LOG4CXX_WARN(LOG.messaging,"Ignoring tape message with empty recId");
		return false;
	}
}

void TapeMsg::HandleResponse(MessageRef responseRef) {
	CStdString logMsg;

	TapeResponse* tr = dynamic_cast<TapeResponse*>(responseRef.get());
	if (!tr) {
		LOG4CXX_WARN(LOG.messaging,"Ignoring wrong response type");
		return;
	}

	if(tr->m_deleteTape && this->m_stage.Equals("ready") )
	{
		CStdString tapeFilename = this->m_fileName;

		CStdString absoluteFilename = CONFIG.m_audioOutputPath + "/" + tapeFilename;
		if (ACE_OS::unlink((PCSTR)absoluteFilename) == 0)
		{
			FLOG_INFO(LOG.messaging,"deleted tape: %s", tapeFilename);
		}
		else
		{
			FLOG_DEBUG(LOG.messaging,"could not delete tape: %s ", tapeFilename);
		}

	}
	else if(tr->m_deleteTape && this->m_stage.Equals("start") && CONFIG.m_pauseRecordingOnRejectedStart == true)
	{
		CStdString orkUid = this->m_recId;
		CStdString empty;
		CapturePluginProxy::Singleton()->PauseCapture(empty, orkUid, empty);
	}
	else 
	{
		// Tape is wanted
		if(CONFIG.m_lookBackRecording == false && CONFIG.m_allowAutomaticRecording && this->m_stage.Equals("start"))
		{
			CStdString orkuid = "", nativecallid = "", side = "";
			CapturePluginProxy::Singleton()->StartCapture(this->m_localParty, orkuid, nativecallid, side);
			CapturePluginProxy::Singleton()->StartCapture(this->m_remoteParty, orkuid, nativecallid, side);
		}
	}

}

bool TapeMsg::IsRealtime() {
	return (this->m_stage.Equals("start") || this->m_stage.Equals("stop"));
}

MessageRef TapeMsg::Clone() {
	oreka::shared_ptr<TapeMsg> clone(new TapeMsg());

	clone->m_recId              = this->m_recId;
	clone->m_fileName           = this->m_fileName;
	clone->m_stage              = this->m_stage;
	clone->m_capturePort        = this->m_capturePort;
	clone->m_localParty         = this->m_localParty;
	clone->m_localEntryPoint    = this->m_localEntryPoint;
	clone->m_remoteParty        = this->m_remoteParty;
	clone->m_direction          = this->m_direction;
	//clone->m_localSide        = this->m_localSide;
	clone->m_audioKeepDirection = this->m_audioKeepDirection;
	clone->m_duration           = this->m_duration;
	clone->m_timestamp          = this->m_timestamp;
	clone->m_localIp            = this->m_localIp;
	clone->m_remoteIp           = this->m_remoteIp;
	clone->m_nativeCallId       = this->m_nativeCallId;
	clone->m_onDemand           = this->m_onDemand;

	// Copy the tags!
	std::copy(this->m_tags.begin(), this->m_tags.end(), std::inserter(clone->m_tags, clone->m_tags.begin()));

	return clone;
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
//TapeTagMsg::TapeTagMsg()
//{
	//// Here is where default values are set
//}

//void TapeTagMsg::Define(Serializer* s)
//{
	//CStdString tapeMessageName("tapetagmsg");
	//s->StringValue(OBJECT_TYPE_TAG, tapeMessageName, true);
	//DefineMessage(s);
//}

//void TapeTagMsg::Validate()
//{
//}

//CStdString TapeTagMsg::GetClassName()
//{
	//return CStdString("tapetagmsg");
//}

//ObjectRef TapeTagMsg::NewInstance()
//{
	//return ObjectRef(new TapeTagMsg);
//}
