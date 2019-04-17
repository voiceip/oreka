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

	if (!s1_isLocal && !s2_isLocal) {

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
// We dont return exact channel number on first s1/2  because they are treated differently.
// We are not sure if this is the most correct approach but it was implemented this way previously
// and breaks a pcap if implemented other way. See T831 for more details
//
// =========================================================================

int OrkSession::DetectChannel(RtpPacketInfoRef& rtpPacket) {
	CStdString logMsg;

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
		return 10;
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
		return 20;
	}
	else if(rtpPacket->m_ssrc == m_lastRtpPacketSide2->m_ssrc && m_lastRtpPacketSide2->m_destIp.s_addr == rtpPacket->m_destIp.s_addr) {
		return 2;
	}

	return 0;
}
