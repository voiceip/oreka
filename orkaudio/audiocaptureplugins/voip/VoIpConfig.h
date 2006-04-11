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
	bool IsDeviceWanted(CStdString device);
	bool IsPacketWanted(IpHeaderStruct* ipHeader);

	CStdString m_device;				// old style but can still be used for specifying single device
	std::list<CStdString> m_devices;	// new style devices csv
	std::list<unsigned int> m_mediaGateways;
	std::list<CStdString> m_asciiMediaGateways;
	std::list<unsigned int> m_lanMasks;
	std::list<CStdString> m_asciiLanMasks;

	std::list<CStdString> m_asciiAllowedIpRanges;	// CIDR notation
	std::list<unsigned int> m_allowedIpRangePrefixes;
	std::list<unsigned int> m_allowedIpRangeBitWidths;
	std::list<CStdString> m_asciiBlockedIpRanges;	// CIDR notation
	std::list<unsigned int> m_blockedIpRangePrefixes;
	std::list<unsigned int> m_blockedIpRangeBitWidths;

	CStdString m_pcapFile;
	CStdString m_pcapDirectory;
	bool m_sipDropIndirectInvite;
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

