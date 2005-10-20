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
}

void VoIpConfig::Define(Serializer* s)
{
	s->StringValue(DEVICE_PARAM, m_device);
	s->CsvValue("LanMasks", m_asciiLanMasks);
	s->CsvValue("MediaGateways", m_asciiMediaGateways);
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
	s->ObjectValue(SOUND_DEVICE_CONFIG_PARAM, m_config, true);
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

