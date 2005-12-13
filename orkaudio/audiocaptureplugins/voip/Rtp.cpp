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
#include "Rtp.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "PacketHeaderDefs.h"
#include "assert.h"

extern "C"
{
#include "g711.h"
}
extern AudioChunkCallBackFunction g_audioChunkCallBack;


void RtpPacketInfo::ToString(CStdString& string)
{
	char sourceIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_sourceIp, sourceIp, sizeof(sourceIp));
	char destIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&m_destIp, destIp, sizeof(destIp));
	string.Format("%s,%d %s,%d seq:%u ts:%u len:%d type:%x", sourceIp, m_sourcePort, destIp, m_destPort, m_seqNum, m_timestamp, m_payloadSize, m_payloadType);
}


RtpRingBuffer::RtpRingBuffer()
{
	m_writePtr = m_buffer;
	m_readPtr = m_buffer;
	m_bufferEnd = m_buffer + NUM_SAMPLES_CIRCULAR_BUFFER;
	m_writeTimestamp = 0;
	m_readTimestamp = 0;
	m_log = Logger::getLogger("rtpringbuffer");
	m_shippedSamples = 0;
}


void RtpRingBuffer::AddRtpPacket(RtpPacketInfoRef& rtpInfo)
{
	unsigned int rtpEndTimestamp = rtpInfo->m_timestamp + rtpInfo->m_payloadSize;

	if(m_writeTimestamp == 0)
	{
		// First RTP packet of the session
		LOG4CXX_DEBUG(m_log, m_capturePort + " first packet");
		m_writeTimestamp = rtpInfo->m_timestamp;
		m_readTimestamp = m_writeTimestamp;
		StoreRtpPacket(rtpInfo);
	}
	else if (rtpInfo->m_timestamp >= m_readTimestamp)	// drop packets that are older than last shipment
	{
		if( (int)(rtpEndTimestamp - m_writeTimestamp) <= (int)FreeSpace())
		{
			// RTP packet fits into current buffer
			StoreRtpPacket(rtpInfo);

			if(UsedSpace() > NUM_SAMPLES_TRIGGER)
			{
				// We have enough stuff, make a shipment
				CreateShipment(0);
			}
		}
		else
		{
			// RTP packet does not fit into current buffer
			// work out how much silence we need to add to the current buffer when shipping
			size_t silenceSize = rtpInfo->m_timestamp - m_writeTimestamp;
			CreateShipment(silenceSize);

			// reset buffer
			m_writePtr = m_buffer;
			m_readPtr = m_buffer;
			m_writeTimestamp = rtpInfo->m_timestamp;
			m_readTimestamp = m_writeTimestamp;

			// Store new packet
			StoreRtpPacket(rtpInfo);
		}
	}
	else
	{
		LOG4CXX_DEBUG(m_log, m_capturePort + " packet too old, dropped");
	}
	CStdString debug;
	debug.Format("free:%u used:%u wr:%x rd:%x wrts:%u rdts:%d", FreeSpace(), UsedSpace(), m_writePtr, m_readPtr, m_writeTimestamp, m_readTimestamp);
	LOG4CXX_DEBUG(m_log, debug);
}

// Writes to the internal buffer without any size verification
void RtpRingBuffer::StoreRtpPacket(RtpPacketInfoRef& rtpInfo)
{
	CStdString debug;

	// 1. Silence from write pointer until end of RTP packet
	unsigned int endRtpTimestamp = rtpInfo->m_timestamp + rtpInfo->m_payloadSize;
	if (endRtpTimestamp > m_writeTimestamp)
	{
		for(int i=0; i<(endRtpTimestamp - m_writeTimestamp); i++)
		{
			*m_writePtr = 0;
			m_writePtr++;
			if(m_writePtr >= m_bufferEnd)
			{
				m_writePtr = m_buffer;
			}
		}
		int silenceSize = endRtpTimestamp - m_writeTimestamp;
		m_writeTimestamp = endRtpTimestamp;
		debug.Format("Zeroed %d samples, wr:%x wrts:%u", silenceSize, m_writePtr, m_writeTimestamp);
		LOG4CXX_DEBUG(m_log, debug);
	}

	// 2. Mix in the latest samples from this RTP packet
	unsigned int timestampDelta = m_writeTimestamp - rtpInfo->m_timestamp;
	ASSERT(timestampDelta>=0);
	short* tempWritePtr = CicularPointerSubtractOffset(m_writePtr, timestampDelta);
	unsigned char* payload = rtpInfo->m_payload;

	for(int i=0; i<rtpInfo->m_payloadSize ; i++)
	{
		if(rtpInfo->m_payloadType == RTP_PT_PCMA)
		{
			*tempWritePtr += (short)alaw2linear(payload[i]);
		}
		else if(rtpInfo->m_payloadType == RTP_PT_PCMU)
		{
			*tempWritePtr += (short)ulaw2linear(payload[i]);
		}
		tempWritePtr++;
		if(tempWritePtr >= m_bufferEnd)
		{
			tempWritePtr = m_buffer;
		}
	}
	debug.Format("Copied %d samples, tmpwr:%x", rtpInfo->m_payloadSize, tempWritePtr);
	LOG4CXX_DEBUG(m_log, debug);
}

short* RtpRingBuffer::CircularPointerAddOffset(short *ptr, size_t offset)
{
	if((ptr + offset) >= m_bufferEnd)
	{
		return m_buffer + offset - (m_bufferEnd-ptr);
	}
	else
	{
		return ptr + offset;
	}
}

short* RtpRingBuffer::CicularPointerSubtractOffset(short *ptr, size_t offset)
{
	if((ptr-offset) < m_buffer)
	{
		return m_bufferEnd - offset + (ptr-m_buffer);
	}
	else
	{
		return ptr - offset;
	}
}

void RtpRingBuffer::CreateShipment(size_t silenceSize)
{
	// 1. ship from readPtr until stop pointer or until end of buffer if wrapped
	bool bufferWrapped = false;
	short* stopPtr = NULL;
	short* wrappedStopPtr = NULL;
	if (silenceSize)
	{
		// There is additional silence to ship, do not take holdoff into account
		stopPtr = m_writePtr;
	}
	else
	{
		stopPtr = CicularPointerSubtractOffset(m_writePtr, NUM_SAMPLES_SHIPMENT_HOLDOFF);
	}

	if (stopPtr < m_readPtr)
	{
		wrappedStopPtr = stopPtr;
		stopPtr = m_bufferEnd;
		bufferWrapped = true;
	}
	size_t shortSize = stopPtr-m_readPtr;
	size_t byteSize = shortSize*2;
	AudioChunkRef chunk(new AudioChunk());
	chunk->SetBuffer((void*)m_readPtr, byteSize, AudioChunk::PcmAudio);
	g_audioChunkCallBack(chunk, m_capturePort);
	m_shippedSamples += shortSize;
	m_readPtr = CircularPointerAddOffset(m_readPtr ,shortSize);
	m_readTimestamp += shortSize;

	CStdString debug;
	debug.Format("Ship %d samples, rd:%x rdts:%u", shortSize, m_readPtr, m_readTimestamp);
	LOG4CXX_DEBUG(m_log, debug);


	// 2. ship from beginning of buffer until stop ptr
	if(bufferWrapped) 
	{
		shortSize = wrappedStopPtr - m_buffer;
		byteSize = shortSize*2;
		chunk.reset(new AudioChunk());
		chunk->SetBuffer((void*)m_buffer, byteSize, AudioChunk::PcmAudio);
		g_audioChunkCallBack(chunk, m_capturePort);
		m_shippedSamples += shortSize;
		m_readPtr = CircularPointerAddOffset(m_readPtr ,shortSize);
		m_readTimestamp += shortSize;
		debug.Format("Ship wrapped %d samples, rd:%x rdts:%u", shortSize, m_readPtr, m_readTimestamp);
		LOG4CXX_DEBUG(m_log, debug);
	}

	// 3. ship silence
	if (silenceSize)
	{
		byteSize = silenceSize*2;
		char* silenceBuffer = (char*)calloc(byteSize, 1);
		if (silenceBuffer)
		{
			AudioChunkRef chunk(new AudioChunk());
			chunk->SetBuffer((void*)silenceBuffer, byteSize, AudioChunk::PcmAudio);
			g_audioChunkCallBack(chunk, m_capturePort);
			m_shippedSamples += silenceSize;
			m_readPtr = CircularPointerAddOffset(m_readPtr ,silenceSize);
			m_readTimestamp += silenceSize;
		}
		debug.Format("Ship %d silence samples, rd:%x rdts:%u", silenceSize, m_readPtr, m_readTimestamp);
		LOG4CXX_DEBUG(m_log, debug);
	}
}


unsigned int RtpRingBuffer::UsedSpace()
{
	if(m_writePtr >= m_readPtr)
	{
		return m_writePtr - m_readPtr;
	}
	return NUM_SAMPLES_CIRCULAR_BUFFER + m_writePtr - m_readPtr;
}


unsigned int RtpRingBuffer::FreeSpace()
{
	return NUM_SAMPLES_CIRCULAR_BUFFER - UsedSpace();
}

void RtpRingBuffer::SetCapturePort(CStdString& port)
{
	m_capturePort = port;
}

