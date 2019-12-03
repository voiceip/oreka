#ifndef __ACPCONFIG_H__
#define __ACPCONFIG_H__

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Object.h"
#include "serializers/Serializer.h"
#include "StdString.h"

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

	bool IsMediaGateway(struct in_addr);
	std::list<unsigned int> m_mediaGateways;
	bool m_ctiDrivenEnable;
	int m_ctiDrivenMatchingTimeoutSec;
	std::list<CStdString> m_ctiDrivenMatchingCriteria;
};

#endif
