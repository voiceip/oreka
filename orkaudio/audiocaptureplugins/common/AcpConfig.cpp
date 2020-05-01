#include "AcpConfig.h"
#include "ConfigManager.h"

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
	m_rtpS1MinNumPacketsBeforeStart = 0;
	m_rtpS2MinNumPacketsBeforeStart = 0;

	m_ctiDrivenEnable = false;
	m_ctiDrivenMatchingTimeoutSec = 30;
	m_ctiDrivenStopIgnore = true;

	m_sipReportFullAddress = false;
	m_sessionStartsOnS2ActivityDb = 0;
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

	s->IpRangesValue("MediaGateways", m_mediaGateways);
	s->BoolValue("CtiDrivenEnable", m_ctiDrivenEnable);
	s->IntValue("CtiDrivenMatchingTimeoutSec",m_ctiDrivenMatchingTimeoutSec);
	s->CsvValue("CtiDrivenMatchingCriteria", m_ctiDrivenMatchingCriteria);
	s->BoolValue("CtiDrivenStopIgnore", m_ctiDrivenStopIgnore);
	s->BoolValue("SipReportFullAddress", m_sipReportFullAddress);
	s->IntValue("RtpS1MinNumPacketsBeforeStart",m_rtpS1MinNumPacketsBeforeStart);
	s->IntValue("RtpS2MinNumPacketsBeforeStart",m_rtpS2MinNumPacketsBeforeStart);
	s->DoubleValue("SessionStartsOnS2ActivityDb", m_sessionStartsOnS2ActivityDb);
}

void AcpConfig::Validate() {
	m_mediaGateways.Compute();
	if(m_sessionStartsOnS2ActivityDb != 0)
	{
		m_rtpS1S2MappingDeterministic = true;
		m_rtpS1S2MappingDeterministicS1IsLocal = true;
		CONFIG.m_discardUnidirectionalCalls = true;
	}
}
#include <iostream>
bool AcpConfig::IsMediaGateway(struct in_addr addr)
{
	bool rc = false;
	if (!m_mediaGateways.Empty())
	{
		rc = m_mediaGateways.Matches(addr);
	}
	return rc;
}

