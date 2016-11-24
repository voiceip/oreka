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
#include <map>
#include "StdString.h"
#include "Object.h"
#include "shared_ptr.h"
#include "PacketHeaderDefs.h"
#include "AudioCapture.h"
#include "Utils.h"
 
#define DEVICE_PARAM "Device"

/** This class defines various configuration parameters for the generator. */
class VoIpConfig : public Object
{
public:
	VoIpConfig();	
	void Define(Serializer* s);
	void Validate();
	void Reset();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	bool IsPartOfLan(struct in_addr);
	bool IsMediaGateway(struct in_addr);
	bool IsRtpTrackingIpAddress(struct in_addr addr);
	bool IsDeviceWanted(CStdString device);
	bool IsPacketWanted(IpHeaderStruct* ipHeader);
	void PopulateIntMapFromCsv(Serializer *s, const char *param, std::map<unsigned int, unsigned int>& intList);

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
	IpRanges m_rtpBlockedIpRanges;
	bool m_rtpTrackByUdpPortOnly;
	bool m_rtpAllowMultipleMappings;
	int m_rtpSeqGapThreshold;
	std::map<unsigned int, unsigned int> m_rtpPayloadTypeBlockList;

	bool m_iax2Support;
	bool m_iax2TreatCallerIdNameAsXUniqueId;
	bool m_sipOverTcpSupport;
	bool m_sipLogFailedCalls;
	bool m_sipUse200OkMediaAddress;
	bool m_sipDetectSessionProgress;
	bool m_sipReportFullAddress;
	bool m_sipDynamicMediaAddress;
	IpRanges m_sipIgnoredMediaAddresses;
	bool m_sipNotifySupport;
	bool m_sipIgnoreBye;
	bool m_sipReportNamesAsTags;
	bool m_sipRequestUriAsLocalParty;
	bool m_sipTreat200OkAsInvite;
	bool m_sipAllowMultipleMediaAddresses;	// deprecated
	bool m_sip302MovedTemporarilySupport;
	bool m_sipInviteCanPutOffHold;
	bool m_sipCallPickUpSupport;
	CStdString m_sipOnDemandFieldName;
	CStdString m_sipOnDemandFieldValue;
	CStdString m_sipOnDemandFieldValueOff;
	CStdString m_sipDialedNumberFieldName;
	CStdString m_sipRemotePartyFieldName;
	CStdString m_sipGroupPickUpPattern;
	bool m_sipTrackMediaAddressOnSender;
	bool m_sipAllowMetadataUpdateOnRtpChange;
	bool m_sipAllowMediaAddressTransfer;

	bool m_rtcpDetect;
	bool m_inInMode;

	bool m_useMacIfNoLocalParty;
	bool m_localPartyForceLocalIp;
	bool m_localPartyForceLocalMac;
	bool m_localPartyUseName;
	bool m_partiesUseName;

	bool m_skinnyIgnoreStopMediaTransmission;
	bool m_skinnyIgnoreOpenReceiveChannelAck;
	bool m_skinnyDynamicMediaAddress;	// Controls whether a single session can have a changing media address
	bool m_skinnyAllowCallInfoUpdate;
	bool m_skinnyAllowLateCallInfo;
	bool m_skinnyNameAsLocalParty;
	bool m_skinnyCallInfoStopsPrevious;
	int m_skinnyCallInfoStopsPreviousToleranceSec;
	bool m_cucm7_1Mode;		// deprecated, use 
	std::list<CStdString> m_skinnyReportTags;
	bool m_skinnyAllowMediaAddressTransfer;		// Controls whether a new session (new CallInfo) can take a media address away from an existing session that has already received RTP.
	bool m_skinnyRtpSearchesForCallInfo;
	bool m_SkinnyTrackConferencesTransfers;
	bool m_skinnyBehindNat;
	bool m_skinnyIgnoreHold;

	std::list<CStdString> m_dnisNumbers;
	std::list<CStdString> m_sipExtractFields;

	bool m_sangomaEnable;	// not a config parm, derived from the two following
	int m_sangomaTcpPortDelta;	// not a config parm, derived from the two following
	int m_sangomaRxTcpPortStart;
	int m_sangomaTxTcpPortStart;
	int m_skinnyTcpPort;
	std::list<CStdString> m_sipDomains;
	IpRanges m_sipDirectionReferenceIpAddresses;
	std::list<CStdString> m_sipDirectionReferenceUserAgents;
	IpRanges m_lanIpRanges;
	IpRanges m_mediaAddressBlockedIpRanges;
	IpRanges m_mediaAddressAllowedIpRanges;

	bool m_dahdiIntercept;
	bool m_holdReportStats;
	bool m_Iax2RewriteTimestamps;
	bool m_trackRawRtpSessionInNonLookBackMode;

	bool m_urlExtractorEnable;
	int m_urlExtractorPort;
	CStdString m_urlExtractorPattern;
	bool m_urlExtractorEndpointIsSender;
	CStdString m_remotePartyUseExtractedKey;
	int m_rtpMinAmountOfPacketsBeforeStart;
	bool m_rtpBreakupOnStreamPause;
	bool m_rtpS1S2MappingDeterministic;
	bool m_localPartyAddLocalIp;
	bool m_rtpLogAllSsrc;
	bool m_orekaEncapsulationMode;
	int m_orekaEncapsulationPort;
	CStdString m_orekaEncapsulationHost;
	CStdString m_onDemandTcpMarkerKey;
	CStdString m_onDemandTcpMarkerValue;
	bool m_sipInfoDtmfRfc2976Detect;
	CStdString m_onDemandViaDtmfDigitsString;
	CStdString m_onDemandPauseViaDtmfDigitsString;
	bool m_mediaAddressUseSecondRtpAddress;
	int m_transferTimeOutInSec;
	bool m_rtpDtmfOnlyLocal;
	int m_udpMinPort;
	bool m_localPartyNameMapEnable;
	bool m_ipFragmentsReassemble;
	bool m_rtpS1S2MappingDeterministicS1IsLocal;
	CStdString m_necNotifyDispLine;
	std::list<CStdString> m_necVoipGatewayNames;	// new style devices csv
	bool m_sipMetadataUseLastInvite;
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

typedef oreka::shared_ptr<VoIpConfigTopObject> VoIpConfigTopObjectRef;

extern VoIpConfigTopObjectRef g_VoIpConfigTopObjectRef;

#ifdef TESTING
	extern VoIpConfig* g_pTestingVoIpConfig;
	#define DLLCONFIG (g_pTestingVoIpConfig?*g_pTestingVoIpConfig:g_VoIpConfigTopObjectRef.get()->m_config)
#else
	#define DLLCONFIG g_VoIpConfigTopObjectRef.get()->m_config
#endif

#endif

