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

