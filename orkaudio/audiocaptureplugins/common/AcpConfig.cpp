#include "AcpConfig.h"
#include "ConfigManager.h"
#include "log4cxx/logger.h"

LoggerPtr getConfigLogger() {
	static LoggerPtr s_log = Logger::getLogger("config");
	return s_log;
}

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
	m_sipUcidFieldName = "User-To-User";
	m_ctiDrivenEarlyStartReconsider = false;
	m_ctiDrivenEarlyStartReconsiderTimeout = 2;

	m_sipReportFullAddress = false;
	m_sessionStartsOnS2ActivityDb = 0;
	m_sipRemotePartyFrom200OKEnable = false;
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
	s->CsvValue("CtiDrivenMatchingTapeMetadataTagPair", m_ctiDrivenMatchingTapeMetadataTagPair);
	s->BoolValue("CtiDrivenEarlyStartReconsider", m_ctiDrivenEarlyStartReconsider);
	s->IntValue("CtiDrivenEarlyStartReconsiderTimeout",m_ctiDrivenEarlyStartReconsiderTimeout);
	s->BoolValue("SipReportFullAddress", m_sipReportFullAddress);
	s->IntValue("RtpS1MinNumPacketsBeforeStart",m_rtpS1MinNumPacketsBeforeStart);
	s->IntValue("RtpS2MinNumPacketsBeforeStart",m_rtpS2MinNumPacketsBeforeStart);
	s->DoubleValue("SessionStartsOnS2ActivityDb", m_sessionStartsOnS2ActivityDb);
	s->StringValue("SipUcidFieldName", m_sipUcidFieldName);
	s->BoolValue("SipRemotePartyFrom200OKEnable", m_sipRemotePartyFrom200OKEnable);
}

void AcpConfig::Validate() {
	m_mediaGateways.Compute();
	if(m_sessionStartsOnS2ActivityDb != 0)
	{
		m_rtpS1S2MappingDeterministic = true;
		m_rtpS1S2MappingDeterministicS1IsLocal = true;
		CONFIG.m_discardUnidirectionalCalls = true;
	}
	if(m_ctiDrivenEnable)
	{
		if(m_ctiDrivenMatchingCriteria.size() > 0)
		{
			for(auto it = m_ctiDrivenMatchingCriteria.begin(); it != m_ctiDrivenMatchingCriteria.end(); it++){
				CtiMatchingCriteriaEnum criteria = MatchNone;
				if((*it).CompareNoCase("localparty") == 0){
					criteria = MatchLocalParty;
				}
				else if((*it).CompareNoCase("remoteparty") == 0){
					criteria = MatchRemoteParty;
				}
				else if((*it).CompareNoCase("ctiremoteparty") == 0){
					criteria = MatchCtiRemoteParty;
				}
				else if((*it).CompareNoCase("ucid") == 0){
					criteria = MatchUcid;
				}
				else if((*it).CompareNoCase("ucid-timestamp") == 0){
					criteria = MatchUcidTimestamp;
				}
				else if((*it).CompareNoCase("x-refci") == 0){
					criteria = MatchXrefci;
				}
				else if((*it).CompareNoCase("tag") == 0){					
					if(m_ctiDrivenMatchingTapeMetadataTagPair.size() == 2){
						criteria = MatchTag;
						m_ctiDrivenMatchingTapeTag = m_ctiDrivenMatchingTapeMetadataTagPair.front();
						m_ctiDrivenMatchingMetadataTag = m_ctiDrivenMatchingTapeMetadataTagPair.back();
					}	
				}

				if(criteria != MatchNone){
					m_ctiMatchingCriteriaList.push_back(criteria);
				}			
			}
			if(m_ctiMatchingCriteriaList.size() == 0){
				m_ctiMatchingCriteriaList.push_back(MatchUcid);
				m_ctiMatchingCriteriaList.push_back(MatchUcidTimestamp);
			}
		}
		else
		{
			m_ctiMatchingCriteriaList.push_back(MatchUcid);
			m_ctiMatchingCriteriaList.push_back(MatchUcidTimestamp);
		}
	}

	m_onDemandViaDtmfDigitsString.Trim();
	m_onDemandPauseViaDtmfDigitsString.Trim();
	if (m_onDemandViaDtmfDigitsString.empty() && !m_onDemandPauseViaDtmfDigitsString.empty())
	{
		CStdString logMsg("OnDemandViaDtmfDigitsString not set while OnDemandPauseViaDtmfDigitsString is set to '");
		logMsg += m_onDemandPauseViaDtmfDigitsString;
		logMsg += "'";
		LOG4CXX_ERROR(getConfigLogger(), logMsg);
	}

}

bool AcpConfig::IsMediaGateway(struct in_addr addr)
{
	bool rc = false;
	if (!m_mediaGateways.Empty())
	{
		rc = m_mediaGateways.Matches(addr);
	}
	return rc;
}

