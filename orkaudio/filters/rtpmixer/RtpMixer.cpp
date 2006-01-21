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
#pragma warning( disable: 4786 )

#ifdef WIN32
#define __CDECL__ __cdecl
#else
#define __CDECL__
#endif
#ifdef WIN32
	#define DLL_EXPORT  __declspec( dllexport )
#else
	#define DLL_EXPORT
#endif

#include <queue>
#include "Filter.h"
#include "AudioCapture.h"
extern "C"
{
#include "g711.h"
}

#define NUM_SAMPLES_CIRCULAR_BUFFER 8000
#define NUM_SAMPLES_TRIGGER 4000			// when we have this number of available samples make a shipment
#define NUM_SAMPLES_SHIPMENT_HOLDOFF 2000	// when shipping, ship everything but this number of samples 


class RtpMixer : public Filter
{
public:
	RtpMixer();

	FilterRef __CDECL__ Instanciate();
	void __CDECL__ AudioChunkIn(AudioChunkRef& chunk);
	void __CDECL__ AudioChunkOut(AudioChunkRef& chunk);
	AudioEncodingEnum __CDECL__ GetInputAudioEncoding();
	AudioEncodingEnum __CDECL__ GetOutputAudioEncoding();
	CStdString __CDECL__ GetName();
	int __CDECL__ GetInputRtpPayloadType();

private:
	//AudioChunkRef m_outputAudioChunk;
	std::queue<AudioChunkRef> m_outputQueue;

	void StoreRtpPacket(AudioChunkRef& chunk);
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
	unsigned int m_shippedSamples;

};

RtpMixer::RtpMixer()
{
	m_writePtr = m_buffer;
	m_readPtr = m_buffer;
	m_bufferEnd = m_buffer + NUM_SAMPLES_CIRCULAR_BUFFER;
	m_writeTimestamp = 0;
	m_readTimestamp = 0;
	//m_log = Logger::getLogger("rtpringbuffer");
	m_shippedSamples = 0;
}

FilterRef RtpMixer::Instanciate()
{
	FilterRef Filter(new RtpMixer());
	return Filter;
}

void RtpMixer::AudioChunkIn(AudioChunkRef& chunk)
{	
	AudioChunkDetails* details = chunk->GetDetails();		
	if(details->m_encoding != PcmAudio)
	{
		throw (CStdString("RtpMixer input audio must be PCM !"));
	}

	unsigned int rtpEndTimestamp = details->m_timestamp + chunk->GetNumSamples();  // GetNumSamples()  #############################

	if(m_writeTimestamp == 0)
	{
		// First RTP packet of the session
		//LOG4CXX_DEBUG(m_log, m_capturePort + " first packet");
		m_writeTimestamp = details->m_timestamp;
		m_readTimestamp = m_writeTimestamp;
		StoreRtpPacket(chunk);
	}
	else if (details->m_timestamp >= m_readTimestamp)	// drop packets that are older than last shipment
	{
		if( (int)(rtpEndTimestamp - m_writeTimestamp) <= (int)FreeSpace())
		{
			// RTP packet fits into current buffer
			StoreRtpPacket(chunk);

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
			size_t silenceSize = details->m_timestamp - m_writeTimestamp;
			CreateShipment(silenceSize);

			// reset buffer
			m_writePtr = m_buffer;
			m_readPtr = m_buffer;
			m_writeTimestamp = details->m_timestamp;
			m_readTimestamp = m_writeTimestamp;

			// Store new packet
			StoreRtpPacket(chunk);
		}
	}
	else
	{
		//LOG4CXX_DEBUG(m_log, m_capturePort + " packet too old, dropped");
	}
	CStdString debug;
	debug.Format("free:%u used:%u wr:%x rd:%x wrts:%u rdts:%d", FreeSpace(), UsedSpace(), m_writePtr, m_readPtr, m_writeTimestamp, m_readTimestamp);
	//LOG4CXX_DEBUG(m_log, debug);
}

void RtpMixer::AudioChunkOut(AudioChunkRef& chunk)
{
	if(m_outputQueue.size() > 0)
	{
		chunk = m_outputQueue.front();
		m_outputQueue.pop();
	}
	else
	{
		chunk.reset();
	}
}

AudioEncodingEnum RtpMixer::GetInputAudioEncoding()
{
	return PcmAudio;
}

AudioEncodingEnum RtpMixer::GetOutputAudioEncoding()
{
	return PcmAudio;
}

CStdString RtpMixer::GetName()
{
	return "RtpMixer";
}


int RtpMixer::GetInputRtpPayloadType(void)	// does not link if not defined here ?
{
	return -1;
}

// Writes to the internal buffer without any size verification
void RtpMixer::StoreRtpPacket(AudioChunkRef& audioChunk)
{
	CStdString debug;
	AudioChunkDetails* details = audioChunk->GetDetails();

	// 1. Silence from write pointer until end of RTP packet
	unsigned int endRtpTimestamp = details->m_timestamp + audioChunk->GetNumSamples();	  // GetNumSamples()  #############################
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
		//LOG4CXX_DEBUG(m_log, debug);
	}

	// 2. Mix in the latest samples from this RTP packet
	unsigned int timestampDelta = m_writeTimestamp - details->m_timestamp;
	ASSERT(timestampDelta>=0);
	short* tempWritePtr = CicularPointerSubtractOffset(m_writePtr, timestampDelta);
	short* payload = (short *)audioChunk->m_pBuffer;	// payload should be short* #################################################

	for(int i=0; i<audioChunk->GetNumSamples() ; i++)	//#################################################
	{
		*tempWritePtr += payload[i];
		tempWritePtr++;
		if(tempWritePtr >= m_bufferEnd)
		{
			tempWritePtr = m_buffer;
		}
	}
	debug.Format("Copied %d samples, tmpwr:%x", audioChunk->GetNumSamples(), tempWritePtr);	//#################################################
	//LOG4CXX_DEBUG(m_log, debug);
}

short* RtpMixer::CircularPointerAddOffset(short *ptr, size_t offset)
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

short* RtpMixer::CicularPointerSubtractOffset(short *ptr, size_t offset)
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

void RtpMixer::CreateShipment(size_t silenceSize)
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
	AudioChunkDetails details;
	details.m_encoding = PcmAudio;
	chunk->SetBuffer((void*)m_readPtr, byteSize, details);
	m_outputQueue.push(chunk);
	m_shippedSamples += shortSize;
	m_readPtr = CircularPointerAddOffset(m_readPtr ,shortSize);
	m_readTimestamp += shortSize;

	CStdString debug;
	debug.Format("Ship %d samples, rd:%x rdts:%u", shortSize, m_readPtr, m_readTimestamp);
	//LOG4CXX_DEBUG(m_log, debug);


	// 2. ship from beginning of buffer until stop ptr
	if(bufferWrapped) 
	{
		shortSize = wrappedStopPtr - m_buffer;
		byteSize = shortSize*2;
		chunk.reset(new AudioChunk());
		AudioChunkDetails details;
		details.m_encoding = PcmAudio;
		chunk->SetBuffer((void*)m_buffer, byteSize, details);
		m_outputQueue.push(chunk);
		m_shippedSamples += shortSize;
		m_readPtr = CircularPointerAddOffset(m_readPtr ,shortSize);
		m_readTimestamp += shortSize;
		debug.Format("Ship wrapped %d samples, rd:%x rdts:%u", shortSize, m_readPtr, m_readTimestamp);
		//LOG4CXX_DEBUG(m_log, debug);
	}

	// 3. ship silence
	if (silenceSize)
	{
		byteSize = silenceSize*2;
		char* silenceBuffer = (char*)calloc(byteSize, 1);
		if (silenceBuffer)
		{
			AudioChunkRef chunk(new AudioChunk());
			AudioChunkDetails details;
			details.m_encoding = PcmAudio;
			chunk->SetBuffer((void*)silenceBuffer, byteSize, details);
			m_outputQueue.push(chunk);
			m_shippedSamples += silenceSize;
			m_readPtr = CircularPointerAddOffset(m_readPtr ,silenceSize);
			m_readTimestamp += silenceSize;
		}
		debug.Format("Ship %d silence samples, rd:%x rdts:%u", silenceSize, m_readPtr, m_readTimestamp);
		//LOG4CXX_DEBUG(m_log, debug);
	}
}


unsigned int RtpMixer::UsedSpace()
{
	if(m_writePtr >= m_readPtr)
	{
		return m_writePtr - m_readPtr;
	}
	return NUM_SAMPLES_CIRCULAR_BUFFER + m_writePtr - m_readPtr;
}


unsigned int RtpMixer::FreeSpace()
{
	return NUM_SAMPLES_CIRCULAR_BUFFER - UsedSpace();
}


//=====================================================================


extern "C"
{
	DLL_EXPORT void __CDECL__ Initialize()
	{
		FilterRef filter(new RtpMixer());
		FilterRegistry::instance()->RegisterFilter(filter);
	}
}