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
#include "ace/OS_NS_unistd.h"
#include "ace/OS_NS_string.h"
#include "ace/OS_NS_strings.h"
#include "ace/OS_NS_dirent.h"
#include "ace/Min_Max.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/OS_NS_ctype.h"
#include "PacketHeaderDefs.h"
#include "Rtp.h"
#include "VoIpSession.h"

void HandleSkinnyMessage(SkinnyHeaderStruct* skinnyHeader, IpHeaderStruct* ipHeader, u_char* packetEnd, TcpHeaderStruct* tcpHeader);
void ScanAllSkinnyMessages(EthernetHeaderStruct* ethernetHeader, IpHeaderStruct* ipHeader, TcpHeaderStruct* tcpHeader, u_char* ipPacketEnd);

#endif
