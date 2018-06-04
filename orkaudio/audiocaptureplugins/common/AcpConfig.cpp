#include "AcpConfig.h"

AcpConfig::AcpConfig() {
}

void AcpConfig::Reset() {

//  DtmfHandling Related
	m_rtpReportDtmf = false;
	m_dtmfReportFullStringAsTag = false;
	m_onDemandViaDtmfDigitsString = "";
}

void AcpConfig::Define(Serializer* s) {

//  DtmfHandling Related
	s->BoolValue("RtpReportDtmf", m_rtpReportDtmf);
	s->BoolValue("DtmfReportFullStringAsTag", m_dtmfReportFullStringAsTag);
	s->StringValue("OnDemandViaDtmfDigitsString", m_onDemandViaDtmfDigitsString);
}

void AcpConfig::Validate() {
}

