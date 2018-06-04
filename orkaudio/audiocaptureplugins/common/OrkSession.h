#ifndef __ORKSESSION_H__
#define __ORKSESSION_H__

#include "StdString.h"
#include "shared_ptr.h"

class AcpConfig;

class OrkSession {
	public:
		CStdString m_capturePort;
		CStdString m_trackingId;
		CStdString m_dtmfDigitString;
		time_t m_beginDate;			// When the session has seen a few RTP packets
		unsigned int m_currentRtpEventTs;

		virtual AcpConfig const * const GetConfig()=0;
		virtual void TriggerOnDemandViaDtmf()=0;
};
typedef oreka::shared_ptr<OrkSession> OrkSessionRef;

#endif
