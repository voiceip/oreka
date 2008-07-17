/*
 * Oreka -- A media capture and retrieval platform
 * 
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 * Please refer to http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef __VOIPCONFIG_H__
#define __VOIPCONFIG_H__

#include <list>
#include "StdString.h"
#include "Object.h"
#include "boost/shared_ptr.hpp"
#include "PacketHeaderDefs.h"
 
#define DEVICE_PARAM "Device"

/** This class defines various configuration parameters for the generator. */
class VoIpConfig : public Object
{
public:
	VoIpConfig();	
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	bool IsPartOfLan(struct in_addr);
	bool IsMediaGateway(struct in_addr);
	bool IsRtpTrackingIpAddress(struct in_addr addr);
	bool IsDeviceWanted(CStdString device);
	bool IsPacketWanted(IpHeaderStruct* ipHeader);

	CStdString m_device;				// old style but can still be used for specifying single device
	std::list<CStdString> m_devices;	// new style devices csv
	std::list<unsigned int> m_mediaGateways;
	std::list<CStdString> m_asciiMediaGateways;
	std::list<unsigned int> m_lanMasks;
	std::list<CStdString> m_asciiLanMasks;
	std::list<unsigned int> m_rtpTrackUsingIpAddresses;
	std::list<CStdString> m_asciiRtpTrackUsingIpAddresses;

	std::list<CStdString> m_asciiAllowedIpRanges;	// CIDR notation
	std::list<unsigned int> m_allowedIpRangePrefixes;
	std::list<unsigned int> m_allowedIpRangeBitWidths;
	std::list<CStdString> m_asciiBlockedIpRanges;	// CIDR notation
	std::list<unsigned int> m_blockedIpRangePrefixes;
	std::list<unsigned int> m_blockedIpRangeBitWidths;

	CStdString m_pcapFile;
	CStdString m_pcapDirectory;
	bool m_pcapRepeat;
	bool m_sipDropIndirectInvite;
	int m_pcapSocketBufferSize;
	bool m_pcapFastReplay;
	int m_pcapFastReplaySleepUsPerSec;
	int m_rtpSessionTimeoutSec;
	int m_rtpSessionWithSignallingTimeoutSec;
	int m_rtpSessionOnHoldTimeOutSec;
	int m_rtpSessionWithSignallingInitialTimeoutSec;
	bool m_pcapTest;
	CStdString m_pcapFilter;
	bool m_rtpDiscontinuityDetect;
	int  m_rtpDiscontinuityMinSeqDelta;
	bool m_rtpDetectOnOddPorts;
	bool m_rtpReportDtmf;
	bool m_iax2Support;
	bool m_sipOverTcpSupport;
	bool m_sipLogFailedCalls;
	bool m_sipUse200OkMediaAddress;
	bool m_sipReportFullAddress;
	bool m_sipDynamicMediaAddress;
	IpRanges m_sipIgnoredMediaAddresses;

	bool m_useMacIfNoLocalParty;
	bool m_localPartyForceLocalIp;
	bool m_localPartyForceLocalMac;

	bool m_skinnyIgnoreStopMediaTransmission;
	bool m_skinnyIgnoreOpenReceiveChannelAck;
	bool m_skinnyDynamicMediaAddress;
	bool m_skinnyAllowCallInfoUpdate;
	bool m_skinnyAllowLateCallInfo;
	bool m_skinnyNameAsLocalParty;
	bool m_skinnyCallInfoStopsPrevious;
	std::list<CStdString> m_skinnyReportTags;

	std::list<CStdString> m_dnisNumbers;
	std::list<CStdString> m_sipExtractFields;

	bool m_sangomaEnable;	// not a config parm, derived from the two following
	int m_sangomaTcpPortDelta;	// not a config parm, derived from the two following
	int m_sangomaRxTcpPortStart;
	int m_sangomaTxTcpPortStart;
	int m_skinnyTcpPort;
	std::list<CStdString> m_sipDomains;
	std::list<CStdString> m_sipDirectionRefenceIpAddresses;
	IpRanges m_lanIpRanges;
};

//========================================

#define VOIP_CONFIG_PARAM "VoIpPlugin"

/** This class represents the top of the configuration hierarchy for the generator. */
class VoIpConfigTopObject : public Object
{
public:
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};
	
	VoIpConfig m_config;
};

typedef boost::shared_ptr<VoIpConfigTopObject> VoIpConfigTopObjectRef;


#endif

