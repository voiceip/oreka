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

#ifndef _SIPUDP_H__
#define _SIPUDP_H__ 1

#include <log4cxx/logger.h>
#include "ace/Singleton.h"
#include "PacketHeaderDefs.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include "boost/multi_index/indexed_by.hpp"
#include "boost/multi_index/sequenced_index.hpp"
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/member.hpp>
#include "boost/shared_ptr.hpp"
#include "SipTcp.h"

class SipUdpStream
{
public:
	SipUdpStream();
	~SipUdpStream();
	bool m_isFragmentsWillFollow;
	bool m_isFollowedFragment;
	int m_offset;
	int m_fragmentFlag;
	int m_mergedIpLen;
	unsigned short m_packetIpId;	//this is packet Identification from Ip header

	void GetUdpPacketInfo(IpHeaderStruct* ipHeader);
	void AddUdpPayload(u_char *pBuffer, int payloadLen);
	SafeBufferRef GetCompleteUdpPayload();

private:
	SafeBufferRef m_udpPayload;

};
typedef boost::shared_ptr<SipUdpStream> SipUdpStreamRef;


typedef boost::multi_index_container
		<
			SipUdpStreamRef,
			boost::multi_index::indexed_by
			<
				boost::multi_index::sequenced<>,
				boost::multi_index::ordered_unique<boost::multi_index::member<SipUdpStream, unsigned short, &SipUdpStream::m_packetIpId> >
			>
		> SipFragmentedUdpMap;

typedef SipFragmentedUdpMap::nth_index<0>::type SipFragmentedUdpMapSeqIndex;
typedef SipFragmentedUdpMap::nth_index<1>::type SipFragmentedUdpMapSearchIndex;


#endif
