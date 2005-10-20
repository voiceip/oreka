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


#ifndef __PACKETHEADERDEFS_H__
#define __PACKETHEADERDEFS_H__


// Structure of Ethernet header
typedef struct
{
	unsigned char sourceMac[6];
	unsigned char destinationMac[6];
	unsigned short length;

} EthernetHeaderStruct;

// Structure of an internet header, naked of options, only valid for little endian
typedef struct
{
	unsigned char	ip_hl:4;		// Header length
	unsigned char	ip_v:4;			// IP protocol version
	unsigned char	ip_tos;			// Type of service 
	unsigned short	ip_len;			// Total length 
	unsigned short	ip_id;			// Identification
	unsigned short	ip_off;			// Fragment offset field
	unsigned char	ip_ttl;			// Time to live
	unsigned char	ip_p;			// Protocol
	unsigned short	ip_sum;			// Header checksum
	struct in_addr	ip_src;			// Source address
	struct in_addr	ip_dest;		// Destination address
} IpHeaderStruct;

// Structure of UDP header
typedef struct
{
	unsigned short source;			// Source port
	unsigned short dest;			// Destination port
	unsigned short len;				// UDP length
	unsigned short check;			// UDP Checksum
} UdpHeaderStruct;

#define RTP_PT_PCMU 0
#define RTP_PT_PCMA 8

// Structure of RTP header, only valid for little endian
typedef struct {
	unsigned short cc:4;		// CSRC count
	unsigned short x:1;			// header extension flag
	unsigned short p:1;			// padding flag
	unsigned short version:2;	// protocol version
	unsigned short pt:7;		// payload type
	unsigned short m:1;			// marker bit
	unsigned short seq;			// sequence number
	unsigned int ts;			// timestamp
	unsigned int ssrc;			// synchronization source
	//unsigned int csrc[1];		// optional CSRC list
} RtpHeaderStruct;

#endif

