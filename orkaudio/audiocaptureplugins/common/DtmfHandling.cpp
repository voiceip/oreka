#include "DtmfHandling.h"

#include "Utils.h"
#include "LogManager.h"
#include "AudioCapturePluginCommon.h"
#include "OrkSession.h"
#include "AudioCapturePlugin.h"
#include "ConfigManager.h"
#include "AcpConfig.h"

extern CaptureEventCallBackFunction g_captureEventCallBack;


static LoggerPtr getLog() {
	static LoggerPtr s_log = Logger::getLogger("dtmf");
	return s_log;
}

void RtpEventInfo::ToString(CStdString& string)
{
	string.Format("digit:%d duration:%d end:%d timestamp:%u volume:%d reserved:%d", 
		m_event, m_duration, m_end, m_startTimestamp, m_volume, m_reserved);
}

int DtmfDigitToEnum(CStdString& msg)
{
	int msgEnum = DtmfDigitUnknown;
	if(msg.CompareNoCase(DTMF_DIGIT_ZERO) == 0)
	{
		msgEnum = DtmfDigitZero;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_ONE) == 0)
	{
		msgEnum = DtmfDigitOne;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_TWO) == 0)
	{
		msgEnum = DtmfDigitTwo;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_THREE) == 0)
	{
		msgEnum = DtmfDigitThree;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_FOUR) == 0)
	{
		msgEnum = DtmfDigitFour;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_FIVE) == 0)
	{
		msgEnum = DtmfDigitFive;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_SIX) == 0)
	{
		msgEnum = DtmfDigitSix;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_SEVEN) == 0)
	{
		msgEnum = DtmfDigitSeven;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_EIGHT) == 0)
	{
		msgEnum = DtmfDigitEight;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_NINE) == 0)
	{
		msgEnum = DtmfDigitNine;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_START) == 0)
	{
		msgEnum = DtmfDigitStart;
	}
	else if (msg.CompareNoCase(DTMF_DIGIT_SHARP) == 0)
	{
		msgEnum = DtmfDigitSharp;
	}
	return msgEnum;
}

CStdString DtmfDigitToString(int msgEnum)
{
	CStdString msgString;
	switch (msgEnum)
	{
	case DtmfDigitZero:
		msgString = DTMF_DIGIT_ZERO;
		break;
	case DtmfDigitOne:
		msgString = DTMF_DIGIT_ONE;
		break;
	case DtmfDigitTwo:
		msgString = DTMF_DIGIT_TWO;
		break;
	case DtmfDigitThree:
		msgString = DTMF_DIGIT_THREE;
		break;
	case DtmfDigitFour:
		msgString = DTMF_DIGIT_FOUR;
		break;
	case DtmfDigitFive:
		msgString = DTMF_DIGIT_FIVE;
		break;
	case DtmfDigitSix:
		msgString = DTMF_DIGIT_SIX;
		break;
	case DtmfDigitSeven:
		msgString = DTMF_DIGIT_SEVEN;
		break;
	case DtmfDigitEight:
		msgString = DTMF_DIGIT_EIGHT;
		break;
	case DtmfDigitNine:
		msgString = DTMF_DIGIT_NINE;
		break;
	case DtmfDigitStart:
		msgString = DTMF_DIGIT_START;
		break;
	case DtmfDigitSharp:
		msgString = DTMF_DIGIT_SHARP;
		break;
	default:
		msgString = DTMF_DIGIT_UNKN;
	}
	return msgString;
}

void ReportDtmfDigit(OrkSession* ss, int channel, CStdString digitValue,  unsigned int digitDuration, unsigned int digitVolume, unsigned int rtpEventTs, unsigned int rtpEventSeqNo)
{
	CaptureEventRef event(new CaptureEvent());

	if(ss->m_dtmfDigitString.length() > 100)
	{
		ss->m_dtmfDigitString.clear();
	}
	ss->m_dtmfDigitString += digitValue;
	LOG4CXX_INFO(getLog(), "[" + ss->m_trackingId + "]" +  " DTMF event [ " + digitValue + " ] new string:" + ss->m_dtmfDigitString);

	if (!ss->m_config->m_onDemandViaDtmfDigitsString.empty())
	{

		bool stringIsOnDemandCommand = (!ss->m_config->m_onDemandViaDtmfDigitsString.empty())
			&& (ss->m_dtmfDigitString.find(ss->m_config->m_onDemandViaDtmfDigitsString) != std::string::npos);
		bool stringIsPauseCommand = (!ss->m_config->m_onDemandPauseViaDtmfDigitsString.empty())
			&& (ss->m_dtmfDigitString.find(ss->m_config->m_onDemandPauseViaDtmfDigitsString) != std::string::npos);

		if (stringIsOnDemandCommand || stringIsPauseCommand)
		{
			ss->m_dtmfDigitString.clear();
		}

		bool identicalPauseAndCommandStrings = (ss->m_config->m_onDemandPauseViaDtmfDigitsString == ss->m_config->m_onDemandViaDtmfDigitsString);

		int curentState = 00;
		if (ss->m_onDemand)
		{
			curentState = 10;
		}
		if (ss->m_keepRtp)
		{
			curentState += 01;
		}

		if (identicalPauseAndCommandStrings && stringIsOnDemandCommand)
		{
			// both strings are equal, which is the actual event?
			if (ss->m_keepRtp == true)
			{
				stringIsOnDemandCommand = false; //recording is running, so event is an OnDemandPause
			}
			else
			{
				stringIsPauseCommand = false; // recording is NOT running, command is OnDemand
			}
		}

		switch (curentState)
		{
		case 00: // This is initial state when LookBack is set to false
			if (stringIsOnDemandCommand)
			{
				ss->TriggerOnDemandViaDtmf();
				ss->m_onDemand = true;
				ss->m_keepRtp = true;
				LOG4CXX_INFO(getLog(), "[" + ss->m_trackingId + "] Trigger OnDemand via DTMF (00->11)");
			}
			break;
		case 01:  // This is initial state when LookBack is set to true
			if (stringIsOnDemandCommand)
			{
				ss->TriggerOnDemandViaDtmf();
				ss->m_onDemand = true;
				ss->m_keepRtp = true;
				LOG4CXX_INFO(getLog(), "[" + ss->m_trackingId + "] Trigger OnDemand via DTMF (10->11)");
			}
			else if (stringIsPauseCommand)
			{
				// PauseCommand  will set / keep paused mode
				ss->m_onDemand = false;
				ss->m_keepRtp = false;
				LOG4CXX_INFO(getLog(), "[" + ss->m_trackingId + "] Pause OnDemand via DTMF (10->00)");
			}
			break;
		case 10:
			if (stringIsOnDemandCommand)
			{
				ss->TriggerOnDemandViaDtmf();
				ss->m_onDemand = true;
				ss->m_keepRtp = true;
				LOG4CXX_INFO(getLog(), "[" + ss->m_trackingId + "] Resume OnDemand via DTMF (10->11)");
			}
			break;
		case 11:
			if (stringIsPauseCommand)
			{
				ss->m_onDemand = true;
				ss->m_keepRtp = false;
				LOG4CXX_INFO(getLog(), "[" + ss->m_trackingId + "] Pause OnDemand via DTMF (11->10)");
			}
			break;
		}
	}

	int rtpEvent = DtmfDigitToEnum(digitValue);
	CStdString dtmfEventString, dtmfEventKey;

	OrkTimeValue timeNow;
	OrkTimeValue beginTime(ss->m_beginDate, 0);
	OrkTimeValue timeDiff;
	int msDiff = 0;

	timeDiff = timeNow - beginTime;
	msDiff = timeDiff.sec() * 1000;

	if(CONFIG.m_dtmfReportingDetailed == true)
	{
		dtmfEventString.Format("event:%d timestamp:%u duration:%d volume:%d seqno:%d offsetms:%d channel:%d", rtpEvent, rtpEventTs, digitDuration, digitVolume, rtpEventSeqNo, msDiff, channel);
		dtmfEventKey.Format("RtpDtmfEvent_%u", rtpEventTs);
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key = dtmfEventKey;
		event->m_value = dtmfEventString;
		g_captureEventCallBack(event, ss->m_capturePort);
		LOG4CXX_INFO(getLog(), "[" + ss->m_trackingId + "] RTP DTMF event [ " + dtmfEventString + " ]");
	}

	if (ss->m_config->m_dtmfReportFullStringAsTag) {
		event.reset(new CaptureEvent());
		event->m_type = CaptureEvent::EtKeyValue;
		event->m_key = "dtmfstring";
		event->m_value = ss->m_dtmfDigitString;
		event->m_offsetMs = msDiff;
		g_captureEventCallBack(event, ss->m_capturePort);
	}

	CStdString dtmfDigitEventValue = digitValue;
	if(CONFIG.m_dtmfReportingDetailed == true) {
		dtmfDigitEventValue.Format("%s_%d_%d_%d",digitValue,digitVolume,digitDuration,channel);
	}

	event.reset(new CaptureEvent());
	event->m_type = CaptureEvent::EtKeyValue;
	event->m_key = "dtmfdigit";
	event->m_value = dtmfDigitEventValue;
	event->m_offsetMs = msDiff;
	g_captureEventCallBack(event, ss->m_capturePort);
}

void HandleRtpEvent(OrkSession* ss,  int channel, RtpEventPayloadFormat *payloadFormat , size_t payloadFormatSize, int rtpTimestamp, int seqNum)
{
	CStdString logMsg;

	if(payloadFormatSize < sizeof(RtpEventPayloadFormat))
	{
		LOG4CXX_WARN(getLog(), "[" + ss->m_trackingId + "] Payload size for event packet too small");
		return;
	}

	RtpEventInfoRef rtpEventInfo(new RtpEventInfo());

	rtpEventInfo->m_event = (unsigned short)payloadFormat->event;
	rtpEventInfo->m_end = (payloadFormat->er_volume & 0x80) ? 1 : 0;
	rtpEventInfo->m_reserved = (payloadFormat->er_volume & 0x40) ? 1 : 0;
	rtpEventInfo->m_volume = (unsigned short)(payloadFormat->er_volume & 0x3F);
	rtpEventInfo->m_duration = ntohs(payloadFormat->duration);
	rtpEventInfo->m_startTimestamp = rtpTimestamp;

	if(getLog()->isDebugEnabled())
	{
		CStdString eventString;
		rtpEventInfo->ToString(eventString);
		logMsg.Format("[%s] RTP DTMF Event Packet: %s", ss->m_trackingId, eventString);
		LOG4CXX_DEBUG(getLog(), logMsg);
	}

	if(ss->m_currentRtpEventTs != rtpEventInfo->m_startTimestamp && rtpEventInfo->m_end == 1)
	{
		ss->m_currentRtpEventTs = rtpEventInfo->m_startTimestamp;
		ReportDtmfDigit(ss, channel, DtmfDigitToString(rtpEventInfo->m_event), rtpEventInfo->m_duration, rtpEventInfo->m_volume, rtpEventInfo->m_startTimestamp, seqNum);
	}

	return;
}
