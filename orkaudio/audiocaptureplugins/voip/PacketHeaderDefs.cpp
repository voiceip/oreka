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
	default:
		msgString = SKINNY_MSG_UNKN;
	}
	return msgString;
}


bool SkinnyValidateStartMediaTransmission(SkStartMediaTransmissionStruct* smt)
{
	bool valid = true;
	if (smt->remoteTcpPort > 65535)
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

bool SkinnyValidateCallInfo(SkCallInfoStruct* sci)
{
	bool valid = true;
	if (sci->callType > SKINNY_CALL_TYPE_FORWARD)
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


bool SkinnyValidateCcm5CallInfo(SkCcm5CallInfoStruct *sci)
{
	bool valid = true;
	if (sci->callType > SKINNY_CALL_TYPE_FORWARD)
	{
		valid = false;
	}
	if(valid)
	{
		valid = checkPartyString(sci->parties, SKINNY_CCM5_PARTIES_BLOCK_SIZE);
	}
	if(valid)
	{
		// Find the first null char separating the calling and called parties (at this point, we know there's one)
		char* nullChar = (char*)&sci->parties;
		while(*nullChar != '\0')
		{
			nullChar++;
		}
		valid = checkPartyString(nullChar+1, SKINNY_CCM5_PARTIES_BLOCK_SIZE);
	}
	return valid;
}


bool SkinnyValidateOpenReceiveChannelAck(SkOpenReceiveChannelAckStruct* orca)
{
	bool valid = true;
	if (orca->endpointTcpPort > 65535)
	{
		valid = false;
	}
	return valid;
}

bool SkinnyValidateLineStat(SkLineStatStruct* lineStat)
{
	bool valid = true;
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

bool SkinnyValidateSoftKeyEvent(SkSoftKeyEventMessageStruct* softKeyEvent)
{
	bool valid = true;

	if(softKeyEvent->soft_key_event > SKINNY_SOFTKEY_MAX_EVENT ||
		softKeyEvent->soft_key_event < SKINNY_SOFTKEY_MIN_EVENT)
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
