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

#include "dll.h"
#include <queue>
#include "Filter.h"
#include "AudioCapture.h"
#include <log4cxx/logger.h>
extern "C"
{
#include "g711.h"
}

#define NUM_SAMPLES_CIRCULAR_BUFFER 16000
#define NUM_SAMPLES_TRIGGER 12000			// when we have this number of available samples make a shipment
#define NUM_SAMPLES_SHIPMENT_HOLDOFF 11000	// when shipping, ship everything but this number of samples 


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
	inline void __CDECL__ CaptureEventIn(CaptureEventRef& event) {;}
	inline void __CDECL__ CaptureEventOut(CaptureEventRef& event) {;}

private:
	//AudioChunkRef m_outputAudioChunk;
	std::queue<AudioChunkRef> m_outputQueue;

	void StoreRtpPacket(AudioChunkRef& chunk, unsigned int correctedTimestamp);
	void ManageOutOfRangeTimestamp(AudioChunkRef& chunk);
	void CreateShipment(size_t silenceSize = 0, bool force = false);
	void Reset(unsigned int timestamp);
	unsigned int FreeSpace();
	unsigned int UsedSpace();
	short* GetHoldOffPtr();
	short* CircularPointerAddOffset(short *ptr, size_t offset);
	short* CicularPointerSubtractOffset(short *ptr, size_t offset);
	bool CheckChunkDetails(AudioChunkDetails&);

	short* m_writePtr;		// pointer after newest RTP data we've received
	short* m_readPtr;		// where to read from next
	unsigned int m_readTimestamp;	// timestamp that the next shipment will have
	unsigned int m_writeTimestamp;	// timestamp that the next RTP buffer should have
	short* m_bufferEnd;
	short m_buffer[NUM_SAMPLES_CIRCULAR_BUFFER];
	unsigned int m_shippedSamples;
	log4cxx::LoggerPtr m_log;
	double m_timestampCorrectiveDelta;
	bool m_invalidChannelReported;
	size_t m_numProcessedSamples;
	bool m_error;
};

RtpMixer::RtpMixer()
{
	m_writePtr = m_buffer;
	m_readPtr = m_buffer;
	m_bufferEnd = m_buffer + NUM_SAMPLES_CIRCULAR_BUFFER;
	m_writeTimestamp = 0;
	m_readTimestamp = 0;
	m_log = log4cxx::Logger::getLogger("rtpmixer");
	m_shippedSamples = 0;
	m_timestampCorrectiveDelta = 0.0;
	m_invalidChannelReported = false;
	m_numProcessedSamples = 0;
	m_error = false;
}

FilterRef RtpMixer::Instanciate()
{
	FilterRef Filter(new RtpMixer());
	return Filter;
}

void RtpMixer::AudioChunkIn(AudioChunkRef& chunk)
{
	CStdString logMsg;

	if(m_error)
	{
		return;
	}
	if(chunk.get() == NULL)
	{
		LOG4CXX_DEBUG(m_log, "Null input chunk");
		return;
	}
	else if(chunk->GetNumSamples() == 0)
	{
		LOG4CXX_DEBUG(m_log, "Empty input chunk");
		return;
	}
	if(chunk->GetNumBytes() > 100000)
	{
		m_error = true;
		LOG4CXX_ERROR(m_log, "RtpMixer: input chunk too big");
		return;
	}

	AudioChunkDetails* details = chunk->GetDetails();		
	if(details->m_encoding != PcmAudio)
	{
		throw (CStdString("RtpMixer input audio must be PCM !"));
	}

	unsigned int correctedTimestamp = 0;

	if(details->m_channel == 1)
	{
		correctedTimestamp = details->m_timestamp;
		m_numProcessedSamples += chunk->GetNumSamples();
		if(m_numProcessedSamples > 115200000)	// arbitrary high number (= 4 hours worth of 8KHz samples)
		{
			m_error = true;
			LOG4CXX_ERROR(m_log, "RtpMixer: Reached input stream size limit");
			return;
		}
	}
	else if(details->m_channel == 2)
	{
		// Corrective delta always only applied to side 2.
		double tmp = (double)details->m_timestamp - m_timestampCorrectiveDelta;
		if(tmp < 0.0)
		{
			// Unsuccessful correction, do not correct.
			correctedTimestamp = details->m_timestamp;
		}
		else
		{
			correctedTimestamp = (unsigned int)tmp;
		}
	}
	else
	{
		if(m_invalidChannelReported == false)
		{
			m_invalidChannelReported = true;
			logMsg.Format("Invalid Channel:%d", details->m_channel);
			LOG4CXX_ERROR(m_log, logMsg);
		}
	}
	unsigned int rtpEndTimestamp = correctedTimestamp + chunk->GetNumSamples();

	if(m_log->isDebugEnabled())
	{
		logMsg.Format("New chunk, s%d ts:%u corr-ts:%u", details->m_channel, details->m_timestamp, correctedTimestamp);
		LOG4CXX_DEBUG(m_log, logMsg);
	}

	if(m_writeTimestamp == 0)
	{
		if(details->m_channel == 1)
		{
			// First RTP packet of the session
			LOG4CXX_DEBUG(m_log, "first chunk");
			m_writeTimestamp = correctedTimestamp;
			m_readTimestamp = m_writeTimestamp;
			StoreRtpPacket(chunk, correctedTimestamp);
		}
		else
		{
			return;
		}
	}
	else if (correctedTimestamp >= m_readTimestamp)
	{
		if( (int)(rtpEndTimestamp - m_writeTimestamp) <= (int)FreeSpace() && (int)(m_writeTimestamp - correctedTimestamp) <= (int)UsedSpace())
		{
			// RTP packet fits into current buffer
			StoreRtpPacket(chunk, correctedTimestamp);

			if(UsedSpace() > NUM_SAMPLES_TRIGGER)
			{
				// We have enough stuff, make a shipment
				CreateShipment();
			}
		}
		else
		{
			// RTP packet does not fit into current buffer
			// work out how much silence we need to add to the current buffer when shipping
			//size_t silenceSize = correctedTimestamp - m_writeTimestamp;

			//if(silenceSize < (8000*10) && (correctedTimestamp > m_writeTimestamp))	// maximum silence is 10 seconds @8KHz
			//{
			//	CreateShipment(silenceSize);

				// reset buffer
			//	Reset(correctedTimestamp);

				// Store new packet
			//	StoreRtpPacket(chunk, correctedTimestamp);
			//}
			//else
			//{
				// This chunk is newer than the curent timestamp window
				ManageOutOfRangeTimestamp(chunk);
			//}
		}
	}
	else
	{
		// This chunk is older than the current timestamp window
		ManageOutOfRangeTimestamp(chunk);
	}
	if(m_log->isDebugEnabled())
	{
		logMsg.Format("free:%u used:%u wr:%x rd:%x wrts:%u rdts:%d", FreeSpace(), UsedSpace(), m_writePtr, m_readPtr, m_writeTimestamp, m_readTimestamp);
		LOG4CXX_DEBUG(m_log, logMsg);
	}
}

void RtpMixer::ManageOutOfRangeTimestamp(AudioChunkRef& chunk)
{
	CStdString logMsg;

	AudioChunkDetails* details = chunk->GetDetails();
	if(details->m_channel == 1)
	{
		// 1. Ship what we have
		CreateShipment(0, true);

		// 2. Reset circular buffer and add this new chunk
		Reset(details->m_timestamp);
		StoreRtpPacket(chunk ,details->m_timestamp);
	}
	else if(details->m_channel == 2)
	{
		// Calculate timestamp corrective delta so that next channel-2 chunk 
		// will be in the circular buffer timestamp window.
		m_timestampCorrectiveDelta = (double)details->m_timestamp - (double)m_writeTimestamp;
	}
}

void RtpMixer::Reset(unsigned int timestamp)
{
	m_writePtr = m_buffer;
	m_readPtr = m_buffer;
	m_writeTimestamp = timestamp;
	m_readTimestamp = m_writeTimestamp;
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
void RtpMixer::StoreRtpPacket(AudioChunkRef& audioChunk, unsigned int correctedTimestamp)
{
	CStdString debug;
	AudioChunkDetails* details = audioChunk->GetDetails();

	// 1. Silence from write pointer until end of RTP packet
	unsigned int endRtpTimestamp = correctedTimestamp + audioChunk->GetNumSamples();
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
	unsigned int timestampDelta = m_writeTimestamp - correctedTimestamp;
	ASSERT(timestampDelta>=0);
	short* tempWritePtr = CicularPointerSubtractOffset(m_writePtr, timestampDelta);
	short* payload = (short *)audioChunk->m_pBuffer;

	for(int i=0; i<audioChunk->GetNumSamples() ; i++)
	{
		int sample = *tempWritePtr + payload[i];
        	if (sample > 32767)
		{
           		sample = 32767;
		}
        	if (sample < -32768)
		{
           		sample = -32768;
		}
		*tempWritePtr = (short)sample;
		tempWritePtr++;
		if(tempWritePtr >= m_bufferEnd)
		{
			tempWritePtr = m_buffer;
		}
	}
	debug.Format("Copied %d samples, tmpwr:%x", audioChunk->GetNumSamples(), tempWritePtr);
	LOG4CXX_DEBUG(m_log, debug);
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

void RtpMixer::CreateShipment(size_t silenceSize, bool force)
{
	// 1. ship from readPtr until stop pointer or until end of buffer if wrapped
	bool bufferWrapped = false;
	short* stopPtr = NULL;
	short* wrappedStopPtr = NULL;
	if (silenceSize || force)
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
	if(CheckChunkDetails(details))
	{
		chunk->SetBuffer((void*)m_readPtr, byteSize, details);
		m_outputQueue.push(chunk);
	}
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
		AudioChunkDetails details;
		details.m_encoding = PcmAudio;
		if(CheckChunkDetails(details))
		{
			chunk->SetBuffer((void*)m_buffer, byteSize, details);
			m_outputQueue.push(chunk);
		}
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
		AudioChunkRef chunk(new AudioChunk());
		AudioChunkDetails details;
		details.m_encoding = PcmAudio;
		if(CheckChunkDetails(details))
		{
			chunk->CreateBuffer(byteSize, details);
			m_outputQueue.push(chunk);
		}
		m_shippedSamples += silenceSize;
		m_readPtr = CircularPointerAddOffset(m_readPtr ,silenceSize);
		m_readTimestamp += silenceSize;
		debug.Format("Ship %d silence samples, rd:%x rdts:%u", silenceSize, m_readPtr, m_readTimestamp);
		LOG4CXX_DEBUG(m_log, debug);
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

bool RtpMixer::CheckChunkDetails(AudioChunkDetails& details)
{
	if(details.m_numBytes > 100000)
	{
		m_error = true;
		LOG4CXX_ERROR(m_log, "RtpMixer: output chunk too big");
		return false;
	}
	return true;
}

//=====================================================================


extern "C"
{
	DLL_EXPORT void __CDECL__ OrkInitialize()
	{
		FilterRef filter(new RtpMixer());
		FilterRegistry::instance()->RegisterFilter(filter);
	}
}

