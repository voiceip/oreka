#ifndef __PROPRIETARYPARSERS_H__
#define __PROPRIETARYPARSERS_H__ s

#include "ParsingUtils.h"
#include "Iax2Session.h"
#include "LogManager.h"

static int iax2_codec_to_rtp_payloadtype(int codec);
static int get_uncompressed_subclass(unsigned char c_sub);
static int parse_iax2_ies(struct iax2_ies *ies, unsigned char *data, int datalen);
void iax2_dump_frame(struct Iax2FullHeader *fh, char *source, char *dest);
/* See if this is an IAX2 NEW.  If so, process */
bool TryIax2New(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload);
bool TryIax2Accept(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload);
bool TryIax2Authreq(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload);
/* HANGUP via IAX frame */
bool TryIax2Hangup(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload);
/* HANGUP via CONTROL frame */
bool TryIax2ControlHangup(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload);
bool TryIax2Reject(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload);
bool TryIax2FullVoiceFrame(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload);
bool TryIax2MetaTrunkFrame(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader,UdpHeaderStruct* udpHeader, u_char* udpPayload);
bool TryIax2MiniVoiceFrame(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, UdpHeaderStruct* udpHeader, u_char* udpPayload);


#endif
