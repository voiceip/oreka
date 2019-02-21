#include "OrkSession.h"

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
