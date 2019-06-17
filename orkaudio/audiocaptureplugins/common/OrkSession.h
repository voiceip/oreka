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
		{
			for (int i = 0; i < 32; i++) m_orekaRtpPayloadTypeMap[i] = i+96;
		}

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
		unsigned char m_orekaRtpPayloadTypeMap[32];

		void	UpdateRtpPayloadMap(unsigned char *map)
		{
			for (int i = 0; i < 32; i++)
			{
				// The map we're passed is an array of 32 payload types.
				// each entry is 0 if it hasn't been seen in the SDP, and
				// will otherwise be the internal Oreka RTP payload type
				// as used in BatchProcessing
				if (map[i]) m_orekaRtpPayloadTypeMap[i] = map[i];
			}
		}
};
typedef oreka::shared_ptr<OrkSession> OrkSessionRef;

#endif
