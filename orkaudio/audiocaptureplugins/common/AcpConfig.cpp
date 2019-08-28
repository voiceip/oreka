#include "AcpConfig.h"

std::list<CStdString> m_asciiMediaGateways;

AcpConfig::AcpConfig() {
}

void AcpConfig::Reset() {

//  DtmfHandling Related
	m_rtpReportDtmf = false;
	m_dtmfReportFullStringAsTag = false;
	m_onDemandViaDtmfDigitsString = "";
	m_rtpEventPayloadTypeDefaultValue = 0;
	m_onDemandPauseViaDtmfDigitsString = "";

	m_rtpS1S2MappingDeterministic = false;
	m_rtpS1S2MappingDeterministicS1IsLocal = true;

	m_asciiMediaGateways.clear();
	m_ctiDrivenEnable = false;
	m_ctiDrivenMatchingTimeoutSec = 30;

	m_sipReportFullAddress = false;
}

void AcpConfig::Define(Serializer* s) {

//  DtmfHandling Related
	s->BoolValue("RtpReportDtmf", m_rtpReportDtmf);
	s->BoolValue("DtmfReportFullStringAsTag", m_dtmfReportFullStringAsTag);
	s->StringValue("OnDemandViaDtmfDigitsString", m_onDemandViaDtmfDigitsString);
	s->IntValue("RtpEventPayloadTypeDefaultValue",m_rtpEventPayloadTypeDefaultValue);
	s->StringValue("OnDemandPauseViaDtmfDigitsString", m_onDemandPauseViaDtmfDigitsString);

	s->BoolValue("RtpS1S2MappingDeterministic", m_rtpS1S2MappingDeterministic);
	s->BoolValue("RtpS1S2MappingDeterministicS1IsLocal", m_rtpS1S2MappingDeterministicS1IsLocal);

	s->CsvValue("MediaGateways", m_asciiMediaGateways);
	s->BoolValue("CtiDrivenEnable", m_ctiDrivenEnable);
	s->IntValue("CtiDrivenMatchingTimeoutSec",m_ctiDrivenMatchingTimeoutSec);
	s->CsvValue("CtiDrivenMatchingCriteria", m_ctiDrivenMatchingCriteria);
	s->BoolValue("SipReportFullAddress", m_sipReportFullAddress);
}

void AcpConfig::Validate() {
	// iterate over ascii Media gateway IP addresses and populate the binary Media gateway IP addresses list
	std::list<CStdString>::iterator it;
	m_mediaGateways.clear();
	for(it = m_asciiMediaGateways.begin(); it != m_asciiMediaGateways.end(); it++)
	{
		struct in_addr a;
		if(inet_pton4((PCSTR)*it, &a))
		{
			m_mediaGateways.push_back((unsigned int)a.s_addr);
		}
		else
		{
			throw (CStdString("VoIpConfig: invalid IP address in MediaGateways:" + *it));
		}
	}
}

bool AcpConfig::IsMediaGateway(struct in_addr addr)
{
	for(std::list<unsigned int>::iterator it = m_mediaGateways.begin(); it != m_mediaGateways.end(); it++)
	{
		if((unsigned int)addr.s_addr == *it)
		{
			return true;
		}
	}
	return false;
}

