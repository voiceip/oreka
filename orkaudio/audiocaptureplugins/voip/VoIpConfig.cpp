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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Utils.h"
#include "serializers/Serializer.h"
#include "VoIpConfig.h"
#include "ace/OS_NS_arpa_inet.h"

VoIpConfig::VoIpConfig()
{
	// Standard LAN internal IP range masks 
	m_asciiLanMasks.push_back("192.168.255.255");
	m_asciiLanMasks.push_back("10.255.255.255");
	m_asciiLanMasks.push_back("172.31.255.255");
	
	m_sipDropIndirectInvite = false;
	m_pcapRepeat = false;
	m_pcapSocketBufferSize = 0;
	m_pcapFastReplay = true;
	m_pcapFastReplaySleepUsPerSec = 0;
	m_rtpSessionTimeoutSec = 10;
	m_rtpSessionWithSignallingTimeoutSec = 10;
	m_rtpSessionWithSignallingInitialTimeoutSec = 5*60;
	m_rtpSessionOnHoldTimeOutSec = 1800;
	m_rtpReportDtmf = false;
	m_rtpTrackByUdpPortOnly = false;
	m_rtpAllowMultipleMappings = true;
	m_rtpSeqGapThreshold = 500;
	m_pcapTest= false;
	m_rtpDiscontinuityDetect = false;
	m_rtpDiscontinuityMinSeqDelta = 1000;
	m_iax2Support = false; // Disabled by default
	m_iax2TreatCallerIdNameAsXUniqueId = true;
	m_rtpDetectOnOddPorts = true;
	m_sipOverTcpSupport = true;
	m_sipLogFailedCalls = false;
	m_sipUse200OkMediaAddress = true;
	m_sipDetectSessionProgress = true; // Enabled by default
	m_sipReportFullAddress = false;
	m_sipDynamicMediaAddress = true;
	m_sipIgnoreBye = false;
	m_sipNotifySupport = false;
	m_sipReportNamesAsTags = false;
	m_sipRequestUriAsLocalParty = true;
	m_sipTreat200OkAsInvite = true;
	m_sipAllowMultipleMediaAddresses = false;	// deprecated
	m_sip302MovedTemporarilySupport = true;
	m_sipInviteCanPutOffHold = false;
	m_sipOnDemandFieldName = "X-record";
	m_sipDialedNumberFieldName = "";
	m_sipRemotePartyFieldName = "";
	m_sipGroupPickUpPattern = "";
	m_sipOnDemandFieldValue = "1";
	m_sipTrackMediaAddressOnSender = true;
	m_sipAllowMetadataUpdateOnRtpChange = false;

	m_rtcpDetect = false;
	m_inInMode = false;

	m_useMacIfNoLocalParty = false; // Uses IP address by default
	m_localPartyForceLocalIp = false;
	m_localPartyForceLocalMac = false;
	m_localPartyUseName = false;
	m_partiesUseName = false;

	m_skinnyIgnoreStopMediaTransmission = false;
	m_skinnyIgnoreOpenReceiveChannelAck = false;
	m_skinnyDynamicMediaAddress = true;
	m_skinnyAllowCallInfoUpdate = true;
	m_skinnyAllowLateCallInfo = false;
	m_skinnyNameAsLocalParty = false;
	m_skinnyCallInfoStopsPrevious = false;
	m_skinnyCallInfoStopsPreviousToleranceSec = 3;
	m_cucm7_1Mode = false;
	m_skinnyAllowMediaAddressTransfer = false;
	m_skinnyRtpSearchesForCallInfo = false;
	m_SkinnyTrackConferencesTransfers = false;

	m_sangomaEnable = false;
	m_sangomaRxTcpPortStart = 0;
	m_sangomaTxTcpPortStart = 0;
	m_skinnyTcpPort = 2000;

	// Initialize LAN IP ranges to standard values.
	m_lanIpRanges.m_asciiIpRanges.push_back(CStdString("10.0.0.0/8"));
	m_lanIpRanges.m_asciiIpRanges.push_back(CStdString("192.168.0.0/16"));
	m_lanIpRanges.m_asciiIpRanges.push_back(CStdString("172.16.0.0/20"));
	m_lanIpRanges.Compute();

	m_dahdiIntercept = false;
	m_holdReportStats = false;
	m_Iax2RewriteTimestamps = false;
}

void VoIpConfig::Define(Serializer* s)
{
	s->StringValue(DEVICE_PARAM, m_device);
	s->CsvValue("Devices", m_devices);
	s->CsvValue("LanMasks", m_asciiLanMasks);
	s->CsvValue("MediaGateways", m_asciiMediaGateways);
	s->CsvValue("RtpTrackUsingIpAddresses", m_asciiRtpTrackUsingIpAddresses);

	s->CsvValue("BlockedIpRanges", m_asciiBlockedIpRanges);
	s->CsvValue("AllowedIpRanges", m_asciiAllowedIpRanges);

	s->StringValue("PcapFile", m_pcapFile);
	s->StringValue("PcapDirectory", m_pcapDirectory);
	s->BoolValue("PcapRepeat", m_pcapRepeat);
	s->BoolValue("PcapFastReplay", m_pcapFastReplay);
	s->IntValue("PcapFastReplaySleepUsPerSec", m_pcapFastReplaySleepUsPerSec);
	s->BoolValue("SipDropIndirectInvite", m_sipDropIndirectInvite);
	s->IntValue("PcapSocketBufferSize", m_pcapSocketBufferSize);
	s->IntValue("RtpSessionTimeoutSec", m_rtpSessionTimeoutSec);
	s->IntValue("RtpSessionWithSignallingTimeoutSec", m_rtpSessionWithSignallingTimeoutSec);
	s->IntValue("RtpSessionWithSignallingInitialTimeoutSec", m_rtpSessionWithSignallingInitialTimeoutSec);
	s->IntValue("RtpSessionOnHoldTimeOutSec", m_rtpSessionOnHoldTimeOutSec);
	s->BoolValue("RtpReportDtmf", m_rtpReportDtmf);
	s->IpRangesValue("RtpBlockedIpRanges", m_rtpBlockedIpRanges);
	s->BoolValue("RtpTrackByUdpPortOnly", m_rtpTrackByUdpPortOnly);
	s->BoolValue("RtpAllowMultipleMappings", m_rtpAllowMultipleMappings);
	s->IntValue("RtpSeqGapThreshold", m_rtpSeqGapThreshold);

	s->BoolValue("PcapTest", m_pcapTest);
	s->StringValue("PcapFilter", m_pcapFilter);
	s->BoolValue("RtpDiscontinuityDetect", m_rtpDiscontinuityDetect);
	s->IntValue("RtpDiscontinuityMinSeqDelta", m_rtpDiscontinuityMinSeqDelta);
	s->CsvValue("DnisNumbers", m_dnisNumbers);
	s->BoolValue("Iax2Support", m_iax2Support);
	s->BoolValue("Iax2TreatCallerIdNameAsXUniqueId", m_iax2TreatCallerIdNameAsXUniqueId);
	s->BoolValue("RtpDetectOnOddPorts", m_rtpDetectOnOddPorts);
	PopulateIntMapFromCsv(s, "RtpPayloadTypeBlockList", m_rtpPayloadTypeBlockList);

	s->CsvValue("SipExtractFields", m_sipExtractFields);
	s->BoolValue("SipNotifySupport", m_sipNotifySupport);
	s->BoolValue("SipOverTcpSupport", m_sipOverTcpSupport);
	s->BoolValue("SipLogFailedCalls", m_sipLogFailedCalls);
	s->BoolValue("SipUse200OkMediaAddress", m_sipUse200OkMediaAddress);
	s->BoolValue("SipDetectSessionProgress", m_sipDetectSessionProgress);
	s->BoolValue("SipReportFullAddress", m_sipReportFullAddress);
	s->BoolValue("SipDynamicMediaAddress", m_sipDynamicMediaAddress);
	s->IpRangesValue("SipIgnoredMediaAddresses", m_sipIgnoredMediaAddresses);
	s->BoolValue("SipIgnoreBye", m_sipIgnoreBye);
	s->BoolValue("SipReportNamesAsTags", m_sipReportNamesAsTags);
	s->BoolValue("SipRequestUriAsLocalParty", m_sipRequestUriAsLocalParty);
	s->BoolValue("SipTreat200OkAsInvite", m_sipTreat200OkAsInvite);
	s->BoolValue("SipAllowMultipleMediaAddresses", m_sipAllowMultipleMediaAddresses);
	s->BoolValue("Sip302MovedTemporarilySupport", m_sip302MovedTemporarilySupport);
	s->BoolValue("SipInviteCanPutOffHold", m_sipInviteCanPutOffHold);
	s->StringValue("SipOnDemandFieldName", m_sipOnDemandFieldName);
	s->StringValue("SipOnDemandFieldValue", m_sipOnDemandFieldValue);
	s->StringValue("SipDialedNumberFieldName", m_sipDialedNumberFieldName);
	s->StringValue("SipRemotePartyFieldName", m_sipRemotePartyFieldName);
	s->StringValue("SipGroupPickUpPattern", m_sipGroupPickUpPattern);
	s->BoolValue("SipTrackMediaAddressOnSender", m_sipTrackMediaAddressOnSender);
	s->BoolValue("SipAllowMetadataUpdateOnRtpChange", m_sipAllowMetadataUpdateOnRtpChange);

	s->BoolValue("RtcpDetect", m_rtcpDetect);
	s->BoolValue("InInMode", m_inInMode);

	s->BoolValue("UseMacIfNoLocalParty", m_useMacIfNoLocalParty);
	s->BoolValue("LocalPartyForceLocalIp", m_localPartyForceLocalIp);
	s->BoolValue("LocalPartyForceLocalMac", m_localPartyForceLocalMac);
	s->BoolValue("LocalPartyUseName", m_localPartyUseName);
	s->BoolValue("PartiesUseName", m_partiesUseName);

	s->BoolValue("SkinnyIgnoreStopMediaTransmission", m_skinnyIgnoreStopMediaTransmission);
	s->BoolValue("SkinnyIgnoreOpenReceiveChannelAck", m_skinnyIgnoreOpenReceiveChannelAck);
	s->BoolValue("SkinnyDynamicMediaAddress", m_skinnyDynamicMediaAddress);
	s->BoolValue("SkinnyAllowCallInfoUpdate", m_skinnyAllowCallInfoUpdate);
	s->IntValue("SkinnyTcpPort", m_skinnyTcpPort);
	s->BoolValue("SkinnyAllowLateCallInfo", m_skinnyAllowLateCallInfo);
	s->BoolValue("SkinnyNameAsLocalParty", m_skinnyNameAsLocalParty);
	s->BoolValue("SkinnyCallInfoStopsPrevious", m_skinnyCallInfoStopsPrevious);
	s->IntValue("SkinnyCallInfoStopsPreviousToleranceSec", m_skinnyCallInfoStopsPreviousToleranceSec);
	s->CsvValue("SkinnyReportTags", m_skinnyReportTags);
	s->BoolValue("Cucm7-1Mode", m_cucm7_1Mode);
	s->BoolValue("SkinnyCucm7Mode", m_cucm7_1Mode);	// synonym to the preceding line (Cucm7-1Mode)
	s->BoolValue("SkinnyAllowMediaAddressTransfer", m_skinnyAllowMediaAddressTransfer);
	s->BoolValue("SkinnyRtpSearchesForCallInfo", m_skinnyRtpSearchesForCallInfo);
	s->BoolValue("SkinnyTrackConferencesTransfers", m_SkinnyTrackConferencesTransfers);

	s->IntValue("SangomaRxTcpPortStart", m_sangomaRxTcpPortStart);
	s->IntValue("SangomaTxTcpPortStart", m_sangomaTxTcpPortStart);

	s->CsvValue("SipDomains", m_sipDomains);
	s->IpRangesValue("SipDirectionReferenceIpAddresses", m_sipDirectionReferenceIpAddresses);
	s->CsvValue("SipDirectionReferenceUserAgents", m_sipDirectionReferenceUserAgents);
	
	s->IpRangesValue("LanIpRanges", m_lanIpRanges);
	s->IpRangesValue("MediaAddressBlockedIpRanges", m_mediaAddressBlockedIpRanges);

	s->BoolValue("DahdiIntercept", m_dahdiIntercept);
	s->BoolValue("HoldReportStats",m_holdReportStats);
	s->BoolValue("Iax2RewriteTimestamps",m_Iax2RewriteTimestamps);
}

void VoIpConfig::Validate()
{
	// iterate over ascii LAN masks and populate the binary LAN Masks list
	m_lanMasks.clear();
	std::list<CStdString>::iterator it;
	for(it = m_asciiLanMasks.begin(); it != m_asciiLanMasks.end(); it++)
	{
		struct in_addr a;
		if(ACE_OS::inet_aton((PCSTR)*it, &a))
		{
			m_lanMasks.push_back((unsigned int)a.s_addr);
		}
		else
		{
			throw (CStdString("VoIpConfig: invalid IP address in LanMasks:" + *it));
		}
	}

	// iterate over ascii Media gateway IP addresses and populate the binary Media gateway IP addresses list
	m_mediaGateways.clear();
	for(it = m_asciiMediaGateways.begin(); it != m_asciiMediaGateways.end(); it++)
	{
		struct in_addr a;
		if(ACE_OS::inet_aton((PCSTR)*it, &a))
		{
			m_mediaGateways.push_back((unsigned int)a.s_addr);
		}
		else
		{
			throw (CStdString("VoIpConfig: invalid IP address in MediaGateways:" + *it));
		}
	}

	// iterate over ascii RTP tracking IP addresses and populate the binary IP addresses list
	m_rtpTrackUsingIpAddresses.clear();
	for(it = m_asciiRtpTrackUsingIpAddresses.begin(); it != m_asciiRtpTrackUsingIpAddresses.end(); it++)
	{
		struct in_addr a;
		if(ACE_OS::inet_aton((PCSTR)*it, &a))
		{
			m_rtpTrackUsingIpAddresses.push_back((unsigned int)a.s_addr);
		}
		else
		{
			throw (CStdString("VoIpConfig: invalid IP address in RtpTrackUsingIpAddresses:" + *it));
		}
	}

	// Iterate over ascii allowed IP ranges and populate the bit width and prefix lists
	m_allowedIpRangePrefixes.clear();
	m_allowedIpRangeBitWidths.clear();
	for(it = m_asciiAllowedIpRanges.begin(); it != m_asciiAllowedIpRanges.end(); it++)
	{
		CStdString cidrPrefixLengthString;
		unsigned int cidrPrefixLength = 32;		// by default, x.x.x.x/32
		CStdString cidrIpAddressString;
		struct in_addr cidrIpAddress;
		
		CStdString entry = *it;
		int slashPosition = entry.Find('/');
		if(slashPosition > 0)
		{
			cidrIpAddressString = entry.Left(slashPosition);
			cidrPrefixLengthString = entry.Mid(slashPosition+1);

			bool notAnInt = false;
			try
			{
				cidrPrefixLength = StringToInt(cidrPrefixLengthString);
			}
			catch (...) {notAnInt = true;}
			if(cidrPrefixLength < 1 || cidrPrefixLength > 32 || notAnInt)
			{
				throw (CStdString("VoIpConfig: invalid CIDR prefix length in AllowedIpRanges:" + entry));
			}
		}
		else
		{
			cidrIpAddressString = entry;
		}

		if(ACE_OS::inet_aton((PCSTR)cidrIpAddressString, &cidrIpAddress))
		{
			unsigned int rangeBitWidth = 32-cidrPrefixLength;
			unsigned int prefix = ntohl((unsigned int)cidrIpAddress.s_addr) >> (rangeBitWidth);
			m_allowedIpRangePrefixes.push_back(prefix);
			m_allowedIpRangeBitWidths.push_back(rangeBitWidth);
		}
		else
		{
			throw (CStdString("VoIpConfig: invalid IP range in AllowedIpRanges:" + entry));
		}
	}


	// Iterate over ascii blocked IP ranges and populate the bit width and prefix lists
	m_blockedIpRangePrefixes.clear();
	m_blockedIpRangeBitWidths.clear();
	for(it = m_asciiBlockedIpRanges.begin(); it != m_asciiBlockedIpRanges.end(); it++)
	{
		CStdString cidrPrefixLengthString;
		unsigned int cidrPrefixLength = 32;		// by default, x.x.x.x/32
		CStdString cidrIpAddressString;
		struct in_addr cidrIpAddress;
		
		CStdString entry = *it;
		int slashPosition = entry.Find('/');
		if(slashPosition > 0)
		{
			cidrIpAddressString = entry.Left(slashPosition);
			cidrPrefixLengthString = entry.Mid(slashPosition+1);

			bool notAnInt = false;
			try
			{
				cidrPrefixLength = StringToInt(cidrPrefixLengthString);
			}
			catch (...) {notAnInt = true;}
			if(cidrPrefixLength < 1 || cidrPrefixLength > 32 || notAnInt)
			{
				throw (CStdString("VoIpConfig: invalid CIDR prefix length in blockedIpRanges:" + entry));
			}
		}
		else
		{
			cidrIpAddressString = entry;
		}

		if(ACE_OS::inet_aton((PCSTR)cidrIpAddressString, &cidrIpAddress))
		{
			unsigned int rangeBitWidth = 32-cidrPrefixLength;
			unsigned int prefix = ntohl((unsigned int)cidrIpAddress.s_addr) >> (rangeBitWidth);
			m_blockedIpRangePrefixes.push_back(prefix);
			m_blockedIpRangeBitWidths.push_back(rangeBitWidth);
		}
		else
		{
			throw (CStdString("VoIpConfig: invalid IP range in BlockedIpRanges:" + entry));
		}
	}
	if(m_pcapSocketBufferSize < 0)
	{
		CStdString exception;
		exception.Format("VoIpConfig: PcapSocketBufferSize must be a positive number (currently:%d)", m_pcapSocketBufferSize);
		throw (exception);

	}
	if(m_rtpSeqGapThreshold < 2)
	{
		CStdString exception;
		exception.Format("VoIpConfig: RtpSeqGapThreshold must be > 1 (currently:%d)", m_rtpSeqGapThreshold);
		throw(exception);
	}
	if(m_rtpSessionTimeoutSec < 1)
	{
		CStdString exception;
		exception.Format("VoIpConfig: RtpSessionTimeoutSec must be > 0 (currently:%d)", m_rtpSessionTimeoutSec);
		throw (exception);
	}
	if(m_rtpSessionWithSignallingTimeoutSec < 1)
	{
		CStdString exception;
		exception.Format("VoIpConfig: RtpSessionWithSignallingTimeoutSec must be > 0 (currently:%d)", m_rtpSessionWithSignallingTimeoutSec);
		throw (exception);
	}
	if(m_rtpSessionOnHoldTimeOutSec < 1)
	{
		CStdString exception;
		exception.Format("VoIpConfig: RtpSessionOnHoldTimeOutSec must be > 0 (currently:%d)", m_rtpSessionOnHoldTimeOutSec);
		throw (exception);
	}
	if(m_rtpSessionWithSignallingInitialTimeoutSec < 1)
	{
		CStdString exception;
		exception.Format("VoIpConfig: RtpSessionWithSignallingInitialTimeoutSec must be > 0 (currently:%d)", m_rtpSessionWithSignallingInitialTimeoutSec);
		throw (exception);
	}
	if(m_skinnyTcpPort < 1)
	{
		CStdString exception;
		exception.Format("VoIpConfig: SkinnyTcpPort must be > 0 (currently:%d) please fix in config.xml", m_skinnyTcpPort);
		throw (exception);
	}
	if(m_sangomaRxTcpPortStart == 0)
	{
	}
	else if(m_sangomaRxTcpPortStart > 65000 || m_sangomaRxTcpPortStart < 2000 || ((m_sangomaRxTcpPortStart%1000) != 0) )
	{
		CStdString exception;
		exception.Format("VoIpConfig: SangomaRxTcpPort must be between 2000 and 65000 and be a multiple of 1000 (currently:%d)", m_sangomaRxTcpPortStart);
		throw (exception);
	}
	if(m_sangomaTxTcpPortStart == 0)
	{
	}
	else if(m_sangomaTxTcpPortStart > 65000 || m_sangomaTxTcpPortStart < 2000 || ((m_sangomaTxTcpPortStart%1000) != 0) )
	{
		CStdString exception;
		exception.Format("VoIpConfig: SangomaTxTcpPort must be between 2000 and 65000 and be a multiple of 1000 (currently:%d)", m_sangomaTxTcpPortStart);
		throw (exception);
	}

	if(m_sangomaRxTcpPortStart > m_sangomaTxTcpPortStart)
	{
		CStdString exception;
		exception.Format("VoIpConfig: SangomaTxTcpPort should always be bigger than SangomaRxTcpPort");
		throw (exception);
	}
	else if(m_sangomaRxTcpPortStart > 0 && m_sangomaTxTcpPortStart>0)
	{
		m_sangomaTcpPortDelta = m_sangomaTxTcpPortStart - m_sangomaRxTcpPortStart;
		m_sangomaEnable = true;
		m_rtpDetectOnOddPorts = true;
	}

	if(m_inInMode == true)
	{
		CStdString inInVar = "ININCrn";

		m_rtcpDetect = true;
		m_sipExtractFields.push_back(inInVar);
	}
	if(m_sipDirectionReferenceUserAgents.size() == 0)
	{
		m_sipDirectionReferenceUserAgents.push_back("Asterisk");
	}

	if(m_dahdiIntercept == true)
	{
		m_sipAllowMultipleMediaAddresses = true;
		m_rtpDetectOnOddPorts = true;
		m_sipRequestUriAsLocalParty = false;
		m_sipIgnoreBye = false;
	}
	if(m_skinnyCallInfoStopsPreviousToleranceSec < 0)
	{
		CStdString exception;
		exception.Format("VoIpConfig: SkinnyCallInfoStopsPreviousToleranceSec must be > 0 (currently:%d", m_skinnyCallInfoStopsPreviousToleranceSec);
		throw(exception);
	}
	if(m_sipAllowMultipleMediaAddresses)
	{
		// m_sipAllowMultipleMediaAddresses is deprecated, turns on m_rtpAllowMultipleMappings instead
		m_rtpAllowMultipleMappings = true;
	}
	if(m_sipOnDemandFieldName.size())
	{
		m_sipExtractFields.push_back(m_sipOnDemandFieldName);
	}
}

bool VoIpConfig::IsPartOfLan(struct in_addr addr)
{
	for(std::list<unsigned int>::iterator it = m_lanMasks.begin(); it != m_lanMasks.end(); it++)
	{
		if(((unsigned int)addr.s_addr & *it) == (unsigned int)addr.s_addr)
		{
			return true;
		}
	}
	return false;
}

bool VoIpConfig::IsMediaGateway(struct in_addr addr)
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

bool VoIpConfig::IsRtpTrackingIpAddress(struct in_addr addr)
{
	for(std::list<unsigned int>::iterator it = m_rtpTrackUsingIpAddresses.begin(); it != m_rtpTrackUsingIpAddresses.end(); it++)
	{
		if((unsigned int)addr.s_addr == *it)
		{
			return true;
		}
	}
	return false;
}

bool VoIpConfig::IsPacketWanted(IpHeaderStruct* ipHeader)
{
	bool wanted = true;	// keep packet by default

	// If source or destination IP address does not match any existing allowing mask, drop packet
	if(m_allowedIpRangePrefixes.size() > 0)
	{
		wanted = false;	// Presence of allowing ranges -> drop packet by default

		bool sourceWanted = false;
		std::list<unsigned int>::iterator bitWidthIt = m_allowedIpRangeBitWidths.begin();
		std::list<unsigned int>::iterator prefixIt = m_allowedIpRangePrefixes.begin();
		while(prefixIt != m_allowedIpRangePrefixes.end())
		{
			unsigned int bitWidth = *bitWidthIt;
			unsigned int prefix = *prefixIt;
			unsigned int packetSourcePrefix = ntohl((unsigned int)ipHeader->ip_src.s_addr) >> bitWidth;
			if(packetSourcePrefix == prefix)
			{
				sourceWanted = true;
				break;
			}
			prefixIt++; 
			bitWidthIt++;
		}
		if(sourceWanted)
		{
			std::list<unsigned int>::iterator bitWidthIt = m_allowedIpRangeBitWidths.begin();
			std::list<unsigned int>::iterator prefixIt = m_allowedIpRangePrefixes.begin();
			while(prefixIt != m_allowedIpRangePrefixes.end())
			{
				unsigned int bitWidth = *bitWidthIt;
				unsigned int prefix = *prefixIt;
				unsigned int packetDestPrefix = ntohl((unsigned int)ipHeader->ip_dest.s_addr) >> bitWidth;
				if(packetDestPrefix == prefix)
				{
					wanted = true;
					break;
				}
				prefixIt++; 
				bitWidthIt++;
			}
		}
	}
	// If source or destination IP address does match any existing blocking range, drop packet
	std::list<unsigned int>::iterator bitWidthIt = m_blockedIpRangeBitWidths.begin();
	std::list<unsigned int>::iterator prefixIt = m_blockedIpRangePrefixes.begin();

	while(prefixIt != m_blockedIpRangePrefixes.end() && wanted == true)
	{
		unsigned int bitWidth = *bitWidthIt;
		unsigned int prefix = *prefixIt;
		unsigned int packetSourcePrefix = ntohl((unsigned int)ipHeader->ip_src.s_addr) >> bitWidth;
		unsigned int packetDestPrefix = ntohl((unsigned int)ipHeader->ip_dest.s_addr) >> bitWidth;

		if(packetSourcePrefix == prefix)
		{
			wanted = false;
		}
		if(packetDestPrefix == prefix)
		{
			wanted = false;
		}
		prefixIt++; 
		bitWidthIt++;
	}
	return wanted;
}

void VoIpConfig::PopulateIntMapFromCsv(Serializer *s, const char *param, std::map<unsigned int, unsigned int>& intList)
{
	std::list<CStdString> csvList;
	std::list<CStdString>::iterator it;

	s->CsvValue(param, csvList);
	if(csvList.size() > 0)
	{
		for(it = csvList.begin(); it != csvList.end(); it++)
		{
			CStdString element = *it;
			unsigned int elem = 0;

			elem = StringToInt(element);
			intList.insert(std::make_pair(elem, 1));
		}
	}
}

bool VoIpConfig::IsDeviceWanted(CStdString device)
{
	if(device.Equals(m_device))
	{
		// Old style single device configuration setting.
		return true;
	}
	for(std::list<CStdString>::iterator it = m_devices.begin(); it != m_devices.end(); it++)
	{
		if(it->Equals(device))
		{
			return true;
		}
	}
	return false;
}


CStdString VoIpConfig::GetClassName()
{
	return CStdString("VoIpConfig");
}

ObjectRef VoIpConfig::NewInstance()
{
	return ObjectRef(new VoIpConfig);
}

//====================================


void VoIpConfigTopObject::Define(Serializer* s)
{
	s->ObjectValue(VOIP_CONFIG_PARAM, m_config, true);
}

void VoIpConfigTopObject::Validate()
{
	;
}

CStdString VoIpConfigTopObject::GetClassName()
{
	return CStdString("VoIpConfigTopObject");
}

ObjectRef VoIpConfigTopObject::NewInstance()
{
	return ObjectRef(new VoIpConfigTopObject);
}

