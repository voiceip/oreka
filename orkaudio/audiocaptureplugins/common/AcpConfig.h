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
};

#endif
