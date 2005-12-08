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

#ifndef __RTP_H__
#define __RTP_H__

#include "AudioCapture.h"
#include "boost/shared_ptr.hpp"
#include "StdString.h"
#include "LogManager.h"

#define NUM_SAMPLES_CIRCULAR_BUFFER 8000
#define NUM_SAMPLES_TRIGGER 4000			// when we have this number of available samples make a shipment
#define NUM_SAMPLES_SHIPMENT_HOLDOFF 2000	// when shipping, ship everything but this number of samples 


// useful info we extract from an RTP packet
class RtpPacketInfo
{
public:
	//CStdString m_sourceMac;
	//CStdString m_destMac;
	struct in_addr m_sourceIp;
	struct in_addr m_destIp;
	unsigned short m_sourcePort;
	unsigned short m_destPort;
	unsigned int m_payloadSize;
	unsigned short m_payloadType;
	unsigned char* m_payload;
	unsigned short m_seqNum;
	unsigned int m_timestamp;
	time_t m_arrivalTimestamp;
};
typedef boost::shared_ptr<RtpPacketInfo> RtpPacketInfoRef;


// Ring buffer based on RTP timestamps
// Gathers both sides of the conversation and mixes them into single channel
// Robust to silence suppression
// Supports RTP buffers of arbitrary sizes in both directions.
class RtpRingBuffer
{
public:
	RtpRingBuffer();
	void AddRtpPacket(RtpPacketInfoRef&);
	void SetCapturePort(CStdString& port);
private:
	void StoreRtpPacket(RtpPacketInfoRef&);
	void CreateShipment(size_t silenceSize);
	unsigned int FreeSpace();
	unsigned int UsedSpace();
	short* GetHoldOffPtr();
	short* CircularPointerAddOffset(short *ptr, size_t offset);
	short* CicularPointerSubtractOffset(short *ptr, size_t offset);

	short* m_writePtr;		// pointer after newest RTP data we've received
	short* m_readPtr;		// where to read from next
	unsigned int m_readTimestamp;	// timestamp that the next shipment will have
	unsigned int m_writeTimestamp;	// timestamp that the next RTP buffer should have
	short* m_bufferEnd;
	short m_buffer[NUM_SAMPLES_CIRCULAR_BUFFER];
	CStdString m_capturePort;
	LoggerPtr m_log;
	unsigned int m_shippedSamples;
};

#endif

