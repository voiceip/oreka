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
#include "messages/AsyncMessage.h"

#define TAPE_MESSAGE_NAME "tape"
#define REC_ID_PARAM "recid"
#define FILENAME_PARAM "filename"
#define STAGE_PARAM "stage"
#define LOCALPARTY_PARAM "localparty"
#define REMOTEPARTY_PARAM "remoteparty"
#define DIRECTION_PARAM "direction"
#define LOCALENTRYPOINT_PARAM "localentrypoint"
#define DURATION_PARAM "duration"
#define SERVICE_PARAM "service"
#define LOCAL_IP_PARAM "localip"
#define REMOTE_IP_PARAM "remoteip"
#define LOCAL_MAC_PARAM "localmac"
#define REMOTE_MAC_PARAM "remotemac"
#define NATIVE_CALLID_PARAM "nativecallid"
#define TAGS_PARAM "tags"
#define ON_DEMAND_PARAM "ondemand"
//#define LOCALSIDE_PARAM "localside"
#define AUDIOKEEPDIRECTION_PARAM "side"

class DLL_IMPORT_EXPORT_ORKBASE TapeMsg :  public SyncMessage, public IReportable
{
public:
	TapeMsg();

	void Define(Serializer* s);
	void Validate();

	//IReportable interface
	MessageRef CreateResponse();
	void HandleResponse(MessageRef responseRef);
	virtual bool IsRealtime();
	MessageRef Clone();
	bool IsValid();

	CStdString GetClassName();
	ObjectRef NewInstance();
	ObjectRef Process();

	CStdString m_recId;
	CStdString m_stage;
	time_t m_timestamp;
	CStdString m_fileName;
	CStdString m_capturePort;
	CStdString m_localParty;
	CStdString m_localEntryPoint;
	CStdString m_remoteParty;
	CStdString m_direction;
	//CStdString m_localSide;
	CStdString m_audioKeepDirection;
	int m_duration;
	CStdString m_serviceName;
	CStdString m_localIp;
	CStdString m_remoteIp;
	CStdString m_localMac;
	CStdString m_remoteMac;
	CStdString m_nativeCallId;

	std::map<CStdString, CStdString> m_tags;
	bool m_onDemand;
	bool m_live;
};

typedef oreka::shared_ptr<TapeMsg> TapeMsgRef;

/** A TapeResponse is a response to TapeMsg 
*/
class DLL_IMPORT_EXPORT_ORKBASE TapeResponse : public SimpleResponseMsg
{
public:
	TapeResponse();
	void Define(Serializer* s);
	inline void Validate() {};

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	bool m_deleteTape;
};

typedef oreka::shared_ptr<TapeResponse> TapeResponseRef;

//class DLL_IMPORT_EXPORT_ORKBASE TapeResponseFwd : public TapeResponse
//{
//public:
//	TapeResponseFwd();
//	void Define(Serializer* s);
//
//	ObjectRef NewInstance();
//
//	bool m_boolean2;
//};


//class DLL_IMPORT_EXPORT_ORKBASE TapeTagMsg : public SyncMessage
//{
//public:
	//TapeTagMsg();

	//void Define(Serializer* s);
	//void Validate();

	//CStdString GetClassName();
	//ObjectRef NewInstance();
	//inline ObjectRef Process() {return ObjectRef();};

	//CStdString m_orkUid;
	//time_t m_timestamp;
	//CStdString m_key;
	//CStdString m_value;
//};
//typedef oreka::shared_ptr<TapeTagMsg> TapeTagMsgRef;

#endif

