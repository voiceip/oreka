#ifndef __ACPCONFIG_H__
#define __ACPCONFIG_H__

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Object.h"
#include "serializers/Serializer.h"
#include "StdString.h"
#include "Utils.h"

typedef enum{
	MatchLocalParty,
	MatchRemoteParty,
	MatchCtiRemoteParty,
	MatchUcid,
	MatchUcidTimestamp,
	MatchXrefci,
	MatchTag,
	MatchNone
} CtiMatchingCriteriaEnum;

class AcpConfig : public Object
{
public:
	AcpConfig();
	void Define(Serializer* s);
	void Validate();
	void Reset();

	CStdString m_onDemandViaDtmfDigitsString;
	CStdString m_onDemandPauseViaDtmfDigitsString;
	bool m_rtpReportDtmf;
	bool m_dtmfReportFullStringAsTag;
	int m_rtpEventPayloadTypeDefaultValue;

	bool m_rtpS1S2MappingDeterministic;
	bool m_rtpS1S2MappingDeterministicS1IsLocal;
	int m_rtpS1MinNumPacketsBeforeStart;
	int m_rtpS2MinNumPacketsBeforeStart;

	bool IsMediaGateway(struct in_addr);
	IpRanges m_mediaGateways;
	bool m_ctiDrivenEnable;
	int m_ctiDrivenMatchingTimeoutSec;
	std::list<CStdString> m_ctiDrivenMatchingCriteria;
	bool m_ctiDrivenStopIgnore;
	std::list<CStdString >m_ctiDrivenMatchingTapeMetadataTagPair;
	CStdString m_ctiDrivenMatchingTapeTag;
	CStdString m_ctiDrivenMatchingMetadataTag;
	CStdString m_sipUcidFieldName;
	bool m_ctiDrivenEarlyStartReconsider;
	int m_ctiDrivenEarlyStartReconsiderTimeout;

	bool m_sipReportFullAddress;
	std::list <CtiMatchingCriteriaEnum> m_ctiMatchingCriteriaList;
	double m_sessionStartsOnS2ActivityDb;
	bool m_sipRemotePartyFrom200OKEnable;
};

#endif
