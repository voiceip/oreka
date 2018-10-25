/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */
#ifndef __SKINNYPARSERS_H__
#define __SKINNYPARSERS_H__ s

#include <list>
#include "Utils.h"
#include <map>
#include "PacketHeaderDefs.h"
#include "Rtp.h"
#include "VoIpSession.h"

void HandleSkinnyMessage(SkinnyHeaderStruct* skinnyHeader, IpHeaderStruct* ipHeader, u_char* packetEnd, TcpHeaderStruct* tcpHeader);
void ScanAllSkinnyMessages(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader, u_char* ipPacketEnd);

#endif
