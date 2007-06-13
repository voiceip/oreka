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
	m_rtpSessionWithSignallingTimeoutSec = 5*60;
	m_rtpSessionOnHoldTimeOutSec = 1800;
	m_pcapTest= false;
	m_rtpDiscontinuityDetect = false;
	m_rtpDiscontinuityMinSeqDelta = 1000;
	m_iax2Support = false; // Disabled by default
	m_rtpDetectOnOddPorts = false;
	m_sipOverTcpSupport = false;  // Disabled by default
	m_useMacIfNoLocalParty = false; // Uses IP address by default

	m_skinnyIgnoreStopMediaTransmission = false;
	m_skinnyIgnoreOpenReceiveChannelAck = false;
	m_skinnyDynamicMediaAddress = false;
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
	s->IntValue("RtpSessionOnHoldTimeOutSec", m_rtpSessionOnHoldTimeOutSec);
	s->BoolValue("PcapTest", m_pcapTest);
	s->BoolValue("RtpDiscontinuityDetect", m_rtpDiscontinuityDetect);
	s->IntValue("RtpDiscontinuityMinSeqDelta", m_rtpDiscontinuityMinSeqDelta);
	s->CsvValue("DnisNumbers", m_dnisNumbers);
	s->BoolValue("Iax2Support", m_iax2Support);
	s->BoolValue("RtpDetectOnOddPorts", m_rtpDetectOnOddPorts);
	s->CsvValue("SipExtractFields", m_sipExtractFields);
	s->BoolValue("SipOverTcpSupport", m_sipOverTcpSupport);
	s->BoolValue("UseMacIfNoLocalParty", m_useMacIfNoLocalParty);

	s->BoolValue("SkinnyIgnoreStopMediaTransmission", m_skinnyIgnoreStopMediaTransmission);
	s->BoolValue("SkinnyIgnoreOpenReceiveChannelAck", m_skinnyIgnoreOpenReceiveChannelAck);
	s->BoolValue("SkinnyDynamicMediaAddress", m_skinnyDynamicMediaAddress);
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
			throw (CStdString("VoIpConfig: invalid IP address in LanMasks:" + *it)  + " please fix config.xml");
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
			throw (CStdString("VoIpConfig: invalid IP address in MediaGateways:" + *it)  + " please fix config.xml");
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
			throw (CStdString("VoIpConfig: invalid IP address in RtpTrackUsingIpAddresses:" + *it)  + " please fix config.xml");
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
				throw (CStdString("VoIpConfig: invalid CIDR prefix length in AllowedIpRanges:" + entry) + " please fix config.xml");
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
			throw (CStdString("VoIpConfig: invalid IP range in AllowedIpRanges:" + entry) + " please fix config.xml");
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
				throw (CStdString("VoIpConfig: invalid CIDR prefix length in blockedIpRanges:" + entry) + " please fix config.xml");
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
			throw (CStdString("VoIpConfig: invalid IP range in BlockedIpRanges:" + entry) + " please fix config.xml");
		}
	}
	if(m_pcapSocketBufferSize < 0)
	{
		CStdString exception;
		exception.Format("VoIpConfig: PcapSocketBufferSize must be a positive number (currently:%d) please fix config.xml", m_pcapSocketBufferSize);
		throw (exception);

	}
	if(m_rtpSessionTimeoutSec < 1)
	{
		CStdString exception;
		exception.Format("VoIpConfig: RtpSessionTimeoutSec must be > 0 (currently:%d) please fix config.xml", m_rtpSessionTimeoutSec);
		throw (exception);
	}
	if(m_rtpSessionWithSignallingTimeoutSec < 1)
	{
		CStdString exception;
		exception.Format("VoIpConfig: RtpSessionWithSignallingTimeoutSec must be > 0 (currently:%d) please fix config.xml", m_rtpSessionWithSignallingTimeoutSec);
		throw (exception);
	}
	if(m_rtpSessionOnHoldTimeOutSec < 1)
	{
		CStdString exception;
                exception.Format("VoIpConfig: RtpSessionOnHoldTimeOutSec must be > 0 (currently:%d) please fix config.xml", m_rtpSessionWithSignallingTimeoutSec);
                throw (exception);
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

