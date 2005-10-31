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
	default:
		msgString = SKINNY_MSG_UNKN;
	}
	return msgString;
}
