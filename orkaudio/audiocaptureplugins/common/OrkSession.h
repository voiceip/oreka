#ifndef __ORKSESSION_H__
#define __ORKSESSION_H__

#include "StdString.h"
#include "shared_ptr.h"
#include "AcpConfig.h"
#include "Rtp.h"

class OrkSession {
	public:
		OrkSession (AcpConfig * config) :
			m_capturePort(""),
			m_trackingId(""),
			m_beginDate(0),
			m_config(config),

			m_keepRtp(true),
			m_startWhenReceiveS2(false),
			m_nonLookBackSessionStarted(false),

			// DTMF related
			m_dtmfDigitString(""),
			m_currentRtpEventTs(0),
			m_telephoneEventPayloadType(config->m_rtpEventPayloadTypeDefaultValue)
		{}

		CStdString m_capturePort;
		CStdString m_trackingId;
		time_t m_beginDate;			// When the session has seen a few RTP packets

		int DetectChannel(RtpPacketInfoRef& rtpPacket, bool* pIsFirstPacket=NULL ) ;

		CStdString m_dtmfDigitString;
		unsigned int m_currentRtpEventTs;
		int m_telephoneEventPayloadType;

		virtual void TriggerOnDemandViaDtmf()=0;
		virtual void Start();
		virtual void ReportMetadata();

		AcpConfig * m_config;

		bool m_keepRtp;
		CStdString m_logMsg;
		bool m_startWhenReceiveS2;
		bool m_nonLookBackSessionStarted;

		RtpPacketInfoRef m_lastRtpPacketSide1;
		RtpPacketInfoRef m_lastRtpPacketSide2;
		bool ShouldSwapChannels();
		struct in_addr m_localIp;
		bool m_mappedS1S2;
};
typedef oreka::shared_ptr<OrkSession> OrkSessionRef;

#endif
