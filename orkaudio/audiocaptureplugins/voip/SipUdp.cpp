#include "SipUdp.h"

SipUdpStream::SipUdpStream()
{
	m_isFragmentsWillFollow = false;
	m_isFollowedFragment = false;
	m_offset = 0;
	m_mergedIpLen = 0;
	m_udpPayload = SafeBufferRef(new SafeBuffer());
}

SipUdpStream::~SipUdpStream()
{

}

void SipUdpStream::GetUdpPacketInfo(IpHeaderStruct* ipHeader)
{
	unsigned char first3bits = 0;
	unsigned char last13bits = 0;
	unsigned short mask = 0x1FFF;
	first3bits = (ntohs(ipHeader->ip_off)) >> 13;
	last13bits = (ntohs(ipHeader->ip_off)) & mask;
	m_fragmentFlag = (int)first3bits;
	m_offset = (int)last13bits;
	switch(m_fragmentFlag)
	{
		case 0:		// could be a followed fragment
			if(m_offset > 0)		//The fragment flag is neither fragment or do-not-fragment, check offset value
			{
				m_isFollowedFragment = true;
				m_packetIpId = ntohs(ipHeader->ip_id);
				m_mergedIpLen = (ntohs(ipHeader->ip_len));
			}
		break;
		case 1:		//fragment flag set, fragments will follow
			m_isFragmentsWillFollow = true;
			m_packetIpId = ntohs(ipHeader->ip_id);
			m_mergedIpLen = ntohs(ipHeader->ip_len) - ipHeader->ip_hl*4 - sizeof(UdpHeaderStruct);
		break;
		case 2:		//Do not fragment flag, do nothing here
		break;
	}
}

void SipUdpStream::AddUdpPayload(u_char *pBuffer, int payloadLen)
{
	m_udpPayload->Add(pBuffer, payloadLen);
}

SafeBufferRef SipUdpStream::GetCompleteUdpPayload()
{
	SafeBufferRef buf(new SafeBuffer());

	buf->Store(m_udpPayload->GetBuffer(), m_udpPayload->Size());

	return buf;
}
