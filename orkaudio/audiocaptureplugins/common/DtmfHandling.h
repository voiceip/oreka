#ifndef __DTMFHANDLING_H__
#define __DTMFHANDLING_H__

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "StdString.h"
#include "OrkSession.h"

#define DTMF_DIGIT_ZERO "0"
#define DTMF_DIGIT_ONE "1"
#define DTMF_DIGIT_TWO "2"
#define DTMF_DIGIT_THREE "3"
#define DTMF_DIGIT_FOUR "4"
#define DTMF_DIGIT_FIVE "5"
#define DTMF_DIGIT_SIX "6"
#define DTMF_DIGIT_SEVEN "7"
#define DTMF_DIGIT_EIGHT "8"
#define DTMF_DIGIT_NINE "9"
#define DTMF_DIGIT_START "*"
#define DTMF_DIGIT_SHARP "#"
#define DTMF_DIGIT_UNKN "unknown"

class RtpEventInfo
{
	public:
		void ToString(CStdString& string);

		unsigned short m_event;
		unsigned short m_end;
		unsigned short m_reserved;
		unsigned short m_volume;
		unsigned short m_duration;
		unsigned int m_startTimestamp;
};
typedef oreka::shared_ptr<RtpEventInfo> RtpEventInfoRef;

typedef enum{
	DtmfDigitZero = 0,
	DtmfDigitOne = 1,
	DtmfDigitTwo = 2,
	DtmfDigitThree = 3,
	DtmfDigitFour = 4,
	DtmfDigitFive = 5,
	DtmfDigitSix = 6,
	DtmfDigitSeven = 7,
	DtmfDigitEight = 8,
	DtmfDigitNine = 9,
	DtmfDigitStart = 10,
	DtmfDigitSharp = 11,
	DtmfDigitUnknown = 12
}DtmfDigitEnum;

typedef struct
{
	unsigned char event;
	unsigned char er_volume;	// Also contains end and error booleans on bit 8 and 7 respectively.
	unsigned short duration;
} RtpEventPayloadFormat;

int DtmfDigitToEnum(CStdString& msg);
CStdString DtmfDigitToString(int msgEnum);

void ReportDtmfDigit(OrkSession* ss, int channel, CStdString digitValue,  unsigned int digitDuration, unsigned int digitVolume, unsigned int rtpEventTs, unsigned int rtpEventSeqNo);
void HandleRtpEvent(OrkSession* ss,  int channel, RtpEventPayloadFormat *payloadFormat , size_t payloadFormatSize, int rtpTimestamp, int seqNum);

#endif
