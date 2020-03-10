#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
//#include "winsock2.h"
#endif
#include "OrkSession.h"
#include "LogManager.h"
#include "ConfigManager.h"


static LoggerPtr getLog() {
	static LoggerPtr s_log = Logger::getLogger("orksession");
	return s_log;
}


bool OrkSession::ShouldSwapChannels()
{
	bool s1_shouldBeLocal = m_config->m_rtpS1S2MappingDeterministicS1IsLocal == true;
	bool s2_shouldBeLocal = !s1_shouldBeLocal;

	//
	// If Media Gateways have been defined, check the ip source against the
	// media gateways to determine whether the flow is "local" or " remote"
	// Otherwise, check against localIp. Using media gateways is the preferred approach
	// as localIp follows the SIP endpoints, which are not necessarily the same as
	// the media endpoints
	if ( !m_config->m_mediaGateways.Empty())
	{
		bool s1_isMediaGateway = m_config->IsMediaGateway(m_lastRtpPacketSide1->m_sourceIp);
		bool s2_isMediaGateway = m_config->IsMediaGateway(m_lastRtpPacketSide2->m_sourceIp);

		if (s1_isMediaGateway && s1_shouldBeLocal) {
			m_logMsg = "s1 is a MediaGateway";
			return true;
		}

		if (s2_isMediaGateway && s2_shouldBeLocal) {
			m_logMsg = "s2 is a MediaGateway";
			return true;
		}
	}
	else
	{
		bool s1_isLocal = (unsigned int)m_lastRtpPacketSide1->m_sourceIp.s_addr == (unsigned int)m_localIp.s_addr;
		bool s2_isLocal = (unsigned int)m_lastRtpPacketSide2->m_sourceIp.s_addr == (unsigned int)m_localIp.s_addr;

		if (s2_isLocal && s1_shouldBeLocal) {
			m_logMsg = "s2 matches localip";
			return true;
		}

		if (s1_isLocal && s2_shouldBeLocal) {
			m_logMsg = "s1 matches localip";
			return true;
		}
	}


	return false;
}

void OrkSession::Start() {
}

void OrkSession::ReportMetadata() {
}

// ==========================================================================
//
// DetectChannel (RtpPacketInfoRef& rtpPacket)
//
// Detects and returns the correct channel number for the rtp packet
// it starts the session if session is set to start with the first s2.
//
// =========================================================================

int OrkSession::DetectChannel(RtpPacketInfoRef& rtpPacket, bool* pIsFirstPacket)
{
	CStdString logMsg;

	if (pIsFirstPacket)
	{
		*pIsFirstPacket = false;
	}

	if(m_lastRtpPacketSide1.get() == NULL)
	{
		// First RTP packet for side 1
		m_lastRtpPacketSide1 = rtpPacket;

		if(getLog()->isInfoEnabled())
		{
			rtpPacket->ToString(logMsg);
			logMsg =  "[" + m_trackingId + "] 1st packet s1: " + logMsg;
			LOG4CXX_INFO(getLog(), logMsg);
		}
		if (pIsFirstPacket)
		{
			*pIsFirstPacket = true;
		}
		return 1;
	}
	else if( rtpPacket->m_ssrc == m_lastRtpPacketSide1->m_ssrc && m_lastRtpPacketSide1->m_destIp.s_addr == rtpPacket->m_destIp.s_addr ) {
		return 1;
	}

	if(m_lastRtpPacketSide2.get() == NULL)
	{
		// First RTP packet for side 2
		m_lastRtpPacketSide2 = rtpPacket;

		if(getLog()->isInfoEnabled())
		{
			rtpPacket->ToString(logMsg);
			logMsg =  "[" + m_trackingId + "] 1st packet s2: " + logMsg;
			LOG4CXX_INFO(getLog(), logMsg);
		}
		if (CONFIG.m_discardUnidirectionalCalls && m_startWhenReceiveS2)
		{
			Start();
			ReportMetadata();
			if (CONFIG.m_lookBackRecording == false)
			{
				m_nonLookBackSessionStarted = true;
			}
		}
		if (pIsFirstPacket)
		{
			*pIsFirstPacket = true;
		}
		return 2;
	}
	else if(rtpPacket->m_ssrc == m_lastRtpPacketSide2->m_ssrc && m_lastRtpPacketSide2->m_destIp.s_addr == rtpPacket->m_destIp.s_addr) {
		return 2;
	}

	return 0;
}
