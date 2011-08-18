#include "PacketHeaderDefs.h"

int SkinnyMessageToEnum(CStdString& msg)
{
	int msgEnum = SkUnkn;
	if(msg.CompareNoCase(SKINNY_MSG_START_MEDIA_TRANSMISSION) == 0)
	{
		msgEnum = SkStartMediaTransmission;
	}
	else if (msg.CompareNoCase(SKINNY_MSG_STOP_MEDIA_TRANSMISSION) == 0)
	{
		msgEnum = SkStopMediaTransmission;
	}
	else if (msg.CompareNoCase(SKINNY_MSG_CALL_INFO_MESSAGE) == 0)
	{
		msgEnum = SkCallInfoMessage;
	}
	else if (msg.CompareNoCase(SKINNY_MSG_OPEN_RECEIVE_CHANNEL_ACK) == 0)
	{
		msgEnum = SkOpenReceiveChannelAck;
	}
	else if (msg.CompareNoCase(SKINNY_MSG_CLOSE_RECEIVE_CHANNEL) == 0)
	{
		msgEnum = SkCloseReceiveChannel;
	}
	else if (msg.CompareNoCase(SKINNY_MSG_LINE_STAT_MESSAGE) == 0)
	{
		msgEnum = SkLineStatMessage;
	}
	else if (msg.CompareNoCase(SKINNY_MSG_CCM5_CALL_INFO_MESSAGE) == 0)
	{
		msgEnum = SkCcm5CallInfoMessage;
	}
	else if (msg.CompareNoCase(SKINNY_MSG_SOFT_KEY_EVENT_MESSAGE) == 0)
	{
		msgEnum = SkSoftKeyEventMessage;
	}
	else if (msg.CompareNoCase(SKINNY_MSG_SOFT_KEY_SET_DESCRIPTION) == 0)
	{
		msgEnum = SkSoftKeySetDescription;
	}
	return msgEnum;
}

CStdString SkinnyMessageToString(int msgEnum)
{
	CStdString msgString;
	switch (msgEnum)
	{
	case SkStartMediaTransmission:
		msgString = SKINNY_MSG_START_MEDIA_TRANSMISSION;
		break;
	case SkStopMediaTransmission:
		msgString = SKINNY_MSG_STOP_MEDIA_TRANSMISSION;
		break;
	case SkCallInfoMessage:
		msgString = SKINNY_MSG_CALL_INFO_MESSAGE;
		break;
	case SkOpenReceiveChannelAck:
		msgString = SKINNY_MSG_OPEN_RECEIVE_CHANNEL_ACK;
		break;
	case SkCloseReceiveChannel:
		msgString = SKINNY_MSG_CLOSE_RECEIVE_CHANNEL;
		break;
	case SkLineStatMessage:
		msgString = SKINNY_MSG_LINE_STAT_MESSAGE;
		break;
	case SkCcm5CallInfoMessage:
		msgString = SKINNY_MSG_CCM5_CALL_INFO_MESSAGE;
		break;
	case SkSoftKeyEventMessage:
		msgString = SKINNY_MSG_SOFT_KEY_EVENT_MESSAGE;
		break;
	case SkSoftKeySetDescription:
		msgString = SKINNY_MSG_SOFT_KEY_SET_DESCRIPTION;
		break;
	default:
		msgString = SKINNY_MSG_UNKN;
	}
	return msgString;
}


bool SkinnyValidateStartMediaTransmission(SkStartMediaTransmissionStruct* smt, u_char* packetEnd)
{
	bool valid = true;
	if(((u_char*)smt + sizeof(SkStartMediaTransmissionStruct)) > packetEnd)
	{
		valid = false;
	}
	else if (smt->remoteTcpPort > 65535)
	{
		valid = false;
	}
	return valid;
}

bool SkinnyValidateCcm7_1StartMediaTransmission(SkCcm7_1StartMediaTransmissionStruct *smt, u_char* packetEnd)
{
	bool valid = true;
	if(((u_char*)smt + sizeof(SkCcm7_1StartMediaTransmissionStruct)) > packetEnd)
	{
		valid = false;
	}
	else if (smt->remoteTcpPort > 65535)
	{
		valid = false;
	}
	return valid;
}

bool SkinnyValidateStopMediaTransmission(SkStopMediaTransmissionStruct* smt, u_char* packetEnd)
{
	bool valid = true;
	if(((u_char*)smt + sizeof(SkStopMediaTransmissionStruct)) > packetEnd)
	{
		valid = false;
	}
	return valid;
}

bool checkPartyString(char* string, int size)
{
	bool valid = false;
	bool invalidCharFound = false;
	bool endOfStringFound = false;
	for(int i=0; i<size && invalidCharFound == false && endOfStringFound == false; i++)
	{
		if(string[i] == 0)
		{
			endOfStringFound = true;
		}
		else if(string[i] > 122 || string[i] < 32)
		{
			invalidCharFound = true;
		}
	}
	if(invalidCharFound == false && endOfStringFound == true)
	{
		valid = true;
	}
	return valid;
}

bool SkinnyValidateCallInfo(SkCallInfoStruct* sci, u_char* packetEnd)
{
	bool valid = true;
	if(((u_char*)sci + sizeof(SkCallInfoStruct)) > packetEnd)
	{
		valid = false;
	}
	else if (sci->callType > SKINNY_CALL_TYPE_FORWARD)
	{
		valid = false;
	}
	if(valid)
	{
		valid = checkPartyString(sci->calledParty, SKINNY_CALLED_PARTY_SIZE);
	}
	if(valid)
	{
		valid = checkPartyString(sci->callingParty, SKINNY_CALLING_PARTY_SIZE);
	}
	if(valid)
	{
		valid = checkPartyString(sci->calledPartyName, SKINNY_CALLED_PARTY_NAME_SIZE);
	}
	if(valid)
	{
		valid = checkPartyString(sci->callingPartyName, SKINNY_CALLING_PARTY_NAME_SIZE);
	}
	return valid;
}


bool SkinnyValidateCcm5CallInfo(SkCcm5CallInfoStruct *sci, u_char* packetEnd)
{
	long partiesLen = 0;
	bool valid = true;
	if(((u_char*)sci + sizeof(SkCcm5CallInfoStruct)) > packetEnd)
	{
		valid = false;
	}
	else if (sci->callType > SKINNY_CALL_TYPE_FORWARD)
	{
		valid = false;
	}
	partiesLen = (long)packetEnd - (long)sci - sizeof(SkCcm5CallInfoStruct);
	if(valid)
	{
		valid = checkPartyString(sci->parties, partiesLen);
	}
	if(valid)
	{
		// Find the first null char separating the calling and called parties (at this point, we know there's one)
		char* nullChar = (char*)&sci->parties;
		int skip = 0;
		while(*nullChar != '\0')
		{
			nullChar++;
			skip++;
		}
		skip += 1;
		valid = checkPartyString(nullChar+1, (partiesLen - skip));
	}
	return valid;
}


bool SkinnyValidateOpenReceiveChannelAck(SkOpenReceiveChannelAckStruct* orca, u_char* packetEnd)
{
	bool valid = true;
	if(((u_char*)orca + sizeof(SkOpenReceiveChannelAckStruct)) > packetEnd)
	{
		valid = false;
	}
	else if (orca->endpointTcpPort > 65535)
	{
		valid = false;
	}
	return valid;
}

bool SkinnyValidateCcm7_1SkOpenReceiveChannelAckStruct(SkCcm7_1SkOpenReceiveChannelAckStruct *orca, u_char* packetEnd)
{
	bool valid = true;
	if(((u_char*)orca + sizeof(SkCcm7_1SkOpenReceiveChannelAckStruct)) > packetEnd)
	{
		valid = false;
	}
	else if (orca->endpointTcpPort > 65535)
	{
		valid = false;
	}
	return valid;
}

bool SkinnyValidateLineStat(SkLineStatStruct* lineStat, u_char* packetEnd)
{
	bool valid = true;
	if(((u_char*)lineStat + sizeof(SkLineStatStruct)) > packetEnd)
	{
		valid = false;
	}
	if(valid)
	{
		valid = checkPartyString(lineStat->displayName, SKINNY_DISPLAY_NAME_SIZE);
	}
	if(valid)
	{
		valid = checkPartyString(lineStat->lineDirNumber, SKINNY_LINE_DIR_NUMBER_SIZE);
	}
	return valid;
}

bool SkinnyValidateSoftKeyEvent(SkSoftKeyEventMessageStruct* softKeyEvent, u_char* packetEnd)
{
	bool valid = true;
	if(((u_char*)softKeyEvent + sizeof(SkSoftKeyEventMessageStruct)) > packetEnd)
	{
		valid = false;
	}
	else if(softKeyEvent->softKeyEvent > SKINNY_SOFTKEY_MAX_EVENT ||
		softKeyEvent->softKeyEvent < SKINNY_SOFTKEY_MIN_EVENT)
	{
		valid = false;
	}

	return valid;
}

bool SkinnyValidateSoftKeySetDescription(SkSoftKeySetDescriptionStruct* softKeySetDescription, u_char* packetEnd)
{
	bool valid = true;
	if(((u_char*)softKeySetDescription + sizeof(SkSoftKeySetDescriptionStruct)) > packetEnd)
	{
		valid = false;
	}
	else if(softKeySetDescription->softKeySetDescription > SKINNY_SOFTKEYSET_MAX_EVENT ||
			softKeySetDescription->softKeySetDescription < SKINNY_SOFTKEYSET_MIN_EVENT)
	{
		valid = false;
	}

	return valid;
}
//===================================================================
int SoftKeyEvent::SoftKeyEventToEnum(CStdString& event)
{
	int skEnum = SkSoftKeyUnkn;

	if(event.CompareNoCase(SKINNY_SOFTKEY_REDIAL) == 0)
	{
		skEnum = SkSoftKeyRedial;
	}
	else if(event.CompareNoCase(SKINNY_SOFTKEY_NEWCALL) == 0)
        {
                skEnum = SkSoftKeyNewCall;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_HOLD) == 0)
        {
                skEnum = SkSoftKeyHold;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_TRNSFER) == 0)
        {
                skEnum = SkSoftKeyTrnsfer;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_CFWDALL) == 0)
        {
                skEnum = SkSoftKeyCFwdAll;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_CFWDBUSY) == 0)
        {
                skEnum = SkSoftKeyCFwdBusy;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_CFWDNOANSWER) == 0)
        {
                skEnum = SkSoftKeyCFwdNoAnswer;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_BACKSPACE) == 0)
        {
                skEnum = SkSoftKeyBackSpace;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_ENDCALL) == 0)
        {
                skEnum = SkSoftKeyEndCall;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_RESUME) == 0)
        {
                skEnum = SkSoftKeyResume;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_ANSWER) == 0)
        {
                skEnum = SkSoftKeyAnswer;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_INFO) == 0)
        {
                skEnum = SkSoftKeyInfo;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_CONFRN) == 0)
        {
                skEnum = SkSoftKeyConfrn;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_PARK) == 0)
        {
                skEnum = SkSoftKeyPark;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_JOIN) == 0)
        {
                skEnum = SkSoftKeyJoin;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_MEETMECONFRN) == 0)
        {
                skEnum = SkSoftKeyConfrn;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_PARK) == 0)
        {
                skEnum = SkSoftKeyPark;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_JOIN) == 0)
        {
                skEnum = SkSoftKeyJoin;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_MEETMECONFRN) == 0)
        {
                skEnum = SkSoftKeyMeetMeConfrn;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_CALLPICKUP) == 0)
        {
                skEnum = SkSoftKeyCallPickUp;
        }
        else if(event.CompareNoCase(SKINNY_SOFTKEY_GRPCALLPICKUP) == 0)
        {
                skEnum = SkSoftKeyGrpCallPickUp;
        }

	return skEnum;
}

CStdString SoftKeyEvent::SoftKeyEventToString(int event)
{
	CStdString msgString;

	switch(event) {
	case SkSoftKeyRedial:
		msgString = SKINNY_SOFTKEY_REDIAL;
		break;
	case SkSoftKeyNewCall:
		msgString = SKINNY_SOFTKEY_NEWCALL;
                break;
        case SkSoftKeyHold:
                msgString = SKINNY_SOFTKEY_HOLD;
                break;
        case SkSoftKeyTrnsfer:
                msgString = SKINNY_SOFTKEY_TRNSFER;
                break;
        case SkSoftKeyCFwdAll:
                msgString = SKINNY_SOFTKEY_CFWDALL;
                break;
        case SkSoftKeyCFwdBusy:
                msgString = SKINNY_SOFTKEY_CFWDBUSY;
                break;
        case SkSoftKeyCFwdNoAnswer:
                msgString = SKINNY_SOFTKEY_CFWDNOANSWER;
                break;
        case SkSoftKeyBackSpace:
                msgString = SKINNY_SOFTKEY_BACKSPACE;
                break;
        case SkSoftKeyEndCall:
                msgString = SKINNY_SOFTKEY_ENDCALL;
                break;
        case SkSoftKeyResume:
                msgString = SKINNY_SOFTKEY_RESUME;
                break;
        case SkSoftKeyAnswer:
                msgString = SKINNY_SOFTKEY_ANSWER;
                break;
        case SkSoftKeyInfo:
                msgString = SKINNY_SOFTKEY_INFO;
                break;
        case SkSoftKeyConfrn:
                msgString = SKINNY_SOFTKEY_CONFRN;
                break;
        case SkSoftKeyPark:
                msgString = SKINNY_SOFTKEY_PARK;
                break;
        case SkSoftKeyJoin:
                msgString = SKINNY_SOFTKEY_JOIN;
                break;
        case SkSoftKeyMeetMeConfrn:
                msgString = SKINNY_SOFTKEY_MEETMECONFRN;
                break;
        case SkSoftKeyCallPickUp:
                msgString = SKINNY_SOFTKEY_CALLPICKUP;
                break;
        case SkSoftKeyGrpCallPickUp:
                msgString = SKINNY_SOFTKEY_GRPCALLPICKUP;
                break;
        default:
                msgString = SKINNY_SOFTKEY_UNKN;
                break;

	}

	return msgString;
}
//==================================================================================
int SoftKeySetDescription::SoftKeySetDescriptionToEnum(CStdString& event)
{
	int skEnum = SkSoftKeySetUnkn;
	if(event.CompareNoCase(SKINNY_SOFTKEYSET_ONHOOK) == 0)
	{
		skEnum = SkSoftKeySetOnHook;
	}
	else if(event.CompareNoCase(SKINNY_SOFTKEYSET_CONNECTED) == 0)
    {
        skEnum = SkSoftKeySetConnected;
    }
	else if(event.CompareNoCase(SKINNY_SOFTKEYSET_ONHOLD) == 0)
    {
        skEnum = SkSoftKeySetOnHold;
    }
	else if(event.CompareNoCase(SKINNY_SOFTKEYSET_RINGIN) == 0)
    {
        skEnum = SkSoftKeySetRIngIn;
    }
	else if(event.CompareNoCase(SKINNY_SOFTKEYSET_OFFHOOK) == 0)
    {
        skEnum = SkSoftKeySetOffHook;
    }
	else if(event.CompareNoCase(SKINNY_SOFTKEYSET_TRANSFER) == 0)
    {
        skEnum = SkSoftKeySetTransfer;
    }
	else if(event.CompareNoCase(SKINNY_SOFTKEYSET_DIGITS) == 0)
    {
        skEnum = SkSoftKeySetDigits;
    }
	else if(event.CompareNoCase(SKINNY_SOFTKEYSET_CONFERENCE) == 0)
    {
        skEnum = SkSoftKeySetConference;
    }
	else if(event.CompareNoCase(SKINNY_SOFTKEYSET_RINGOUT) == 0)
    {
        skEnum = SkSoftKeySetRingOut;
    }
	else if(event.CompareNoCase(SKINNY_SOFTKEYSET_ONHOOKFEATURE) == 0)
    {
        skEnum = SkSoftKeySetOffHookFeature;
    }
	return skEnum;
}

CStdString SoftKeySetDescription::SoftKeySetDescriptionToString(int event)
{
	CStdString descString;
	switch(event)
	{
	case SkSoftKeySetOnHook:
		descString = SKINNY_SOFTKEYSET_ONHOOK;
		break;
	case SkSoftKeySetConnected:
		descString = SKINNY_SOFTKEYSET_CONNECTED;
		break;
	case SkSoftKeySetOnHold:
		descString = SKINNY_SOFTKEYSET_ONHOLD;
		break;
	case SkSoftKeySetRIngIn:
		descString = SKINNY_SOFTKEYSET_RINGIN;
		break;
	case SkSoftKeySetOffHook:
		descString = SKINNY_SOFTKEYSET_OFFHOOK;
		break;
	case SkSoftKeySetTransfer:
		descString = SKINNY_SOFTKEYSET_TRANSFER;
		break;
	case SkSoftKeySetDigits:
		descString = SKINNY_SOFTKEYSET_DIGITS;
		break;
	case SkSoftKeySetConference:
		descString = SKINNY_SOFTKEYSET_CONFERENCE;
		break;
	case SkSoftKeySetRingOut:
		descString = SKINNY_SOFTKEYSET_RINGOUT;
		break;
	case SkSoftKeySetOffHookFeature:
		descString = SKINNY_SOFTKEYSET_ONHOOKFEATURE;
		break;
	default:
		descString = SKINNY_SOFTKEYSET_UNKN;
		break;
	} //switch
	return descString;
}

