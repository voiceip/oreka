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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "dll.h"
#include <queue>
#include <vector>
#include "ConfigManager.h"
#include <log4cxx/logger.h>
#include "Filter.h"
#include "AudioCapture.h"

extern "C"
{
#include "g711.h"
}

#define NUM_SAMPLES_CIRCULAR_BUFFER 16000
#define NUM_SAMPLES_TRIGGER 12000			// when we have this number of available samples make a shipment
#define NUM_SAMPLES_SHIPMENT_HOLDOFF 11000	// when shipping, ship everything but this number of samples 

class RtpMixerChannel
{
public:
	RtpMixerChannel();
	~RtpMixerChannel();

	int m_channel;
	double m_timestampCorrectiveDelta;
	short m_buffer[NUM_SAMPLES_CIRCULAR_BUFFER];

	// Statistics related to channel
	AudioChunkRef m_lastChunk;
	int m_seqNumMisses;
	int m_seqMaxGap;
	int m_seqNumOutOfOrder;
	int m_seqNumDiscontinuities;
};
typedef boost::shared_ptr<RtpMixerChannel> RtpMixerChannelRef;

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
	inline void __CDECL__ CaptureEventIn(CaptureEventRef& event) {;}
	inline void __CDECL__ CaptureEventOut(CaptureEventRef& event) {;}

private:
	//AudioChunkRef m_outputAudioChunk;
	std::queue<AudioChunkRef> m_outputQueue;

	void StoreRtpPacket(AudioChunkRef& chunk, unsigned int correctedTimestamp);
	void ManageOutOfRangeTimestamp(AudioChunkRef& chunk);
	void HandleMixedOutput(AudioChunkRef &chunk, AudioChunkDetails& details, short *readPtr);
	void CreateShipment(size_t silenceSize = 0, bool force = false);
	void Reset(unsigned int timestamp);
	unsigned int FreeSpace();
	unsigned int UsedSpace();
	short* GetHoldOffPtr();
	short* CircularPointerAddOffset(short *ptr, size_t offset);
	short* CicularPointerSubtractOffset(short *ptr, size_t offset);
	bool CheckChunkDetails(AudioChunkDetails&);
	void DoStats(	AudioChunkDetails* details, AudioChunkDetails* lastDetails, 
					int& seqNumMisses, int& seqMaxGap, int& seqNumOutOfOrder, 
					int& seqNumDiscontinuities);
	void CreateChannels(int channelNumber);

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
	bool m_oneS1PacketState;

	// Statistics related variables
	AudioChunkRef m_lastChunkS1;
	AudioChunkRef m_lastChunkS2;
	int m_seqNumMissesS1;
	int m_seqNumMissesS2;
	int m_seqMaxGapS1;
	int m_seqMaxGapS2;
	int m_seqNumOutOfOrderS1;
	int m_seqNumOutOfOrderS2;
	int m_seqNumDiscontinuitiesS1;
	int m_seqNumDiscontinuitiesS2;

	//==========================================================
	// Multi-channel separated output

	int m_numChannels; // Number of channels
	std::vector<RtpMixerChannelRef> m_rtpMixerChannels; // Channel information
};

//==========================================================
RtpMixerChannel::RtpMixerChannel()
{
	m_timestampCorrectiveDelta = 0.0;
	m_seqNumMisses = 0;
	m_seqMaxGap = 0;
	m_seqNumOutOfOrder = 0;
	m_seqNumDiscontinuities = 0;
	m_channel = 0;
	for(int i = 0; i < NUM_SAMPLES_CIRCULAR_BUFFER; i++)
	{
		m_buffer[i] = 0;
	}
}

RtpMixerChannel::~RtpMixerChannel()
{
	//printf("\n\n\n\n\n\n\nRtpMixerChannel%d being destroyed\n\n\n\n\n", m_channel);
}

//==========================================================

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
	m_seqNumMissesS1 = 0;
	m_seqNumMissesS2 = 0;
	m_seqMaxGapS1 = 0;
	m_seqMaxGapS2 = 0;
	m_seqNumOutOfOrderS1 = 0;
	m_seqNumOutOfOrderS2 = 0;
	m_seqNumDiscontinuitiesS1 = 0;
	m_seqNumDiscontinuitiesS2 = 0;
	m_numChannels = 0;
	m_oneS1PacketState = false;
	m_rtpMixerChannels.clear();
}

void RtpMixer::CreateChannels(int channelNumber)
{
	if(CONFIG.m_stereoRecording == false)
	{
		return;
	}

	if(CONFIG.m_tapeNumChannels <= 1)
	{
		return;
	}

	if(channelNumber <= m_numChannels)
	{
		// Channel already created
		return;
	}

	RtpMixerChannelRef rtpMixerChannel;

	int i;

	i = m_numChannels;

	// Always create channels until this number
	for(; i < channelNumber; i++)
	{
		rtpMixerChannel.reset(new RtpMixerChannel());
		rtpMixerChannel->m_channel = i+1;
		m_rtpMixerChannels.push_back(rtpMixerChannel);
		m_numChannels += 1;
	}

	return;
}

FilterRef RtpMixer::Instanciate()
{
	FilterRef Filter(new RtpMixer());
	return Filter;
}

void RtpMixer::DoStats(	AudioChunkDetails* details, AudioChunkDetails* lastDetails, 
						int& seqNumMisses, int& seqMaxGap, int& seqNumOutOfOrder, 
						int& seqNumDiscontinuities)
{
	double seqNumDelta = (double)details->m_sequenceNumber - (double)lastDetails->m_sequenceNumber;
	double timestampDelta = (double)details->m_timestamp - (double)lastDetails->m_timestamp;
	if(	abs(seqNumDelta) > 1000.0  &&
		abs(timestampDelta) > 160000.0)	
	{
		seqNumDiscontinuities++;
		CStdString logMsg;
		logMsg.Format("[%s] RTP discontinuity s%d: before: seq:%u ts:%u after: seq:%u ts:%u", 
			m_trackingId, details->m_channel, lastDetails->m_sequenceNumber, lastDetails->m_timestamp, 
			details->m_sequenceNumber, details->m_timestamp);
		LOG4CXX_DEBUG(m_log, logMsg);
	}
	else
	{
		if(seqNumDelta > (double)seqMaxGap)
		{
			seqMaxGap = (unsigned int)seqNumDelta;
		}
		if(seqNumDelta < 0.0)
		{
			seqNumOutOfOrder++;
		}
		if(seqNumDelta != 1.0 && details->m_sequenceNumber != 1)
		{
			seqNumMisses++;
		}
	}
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
		logMsg.Format("[%s] Null input chunk",m_trackingId);
		LOG4CXX_DEBUG(m_log, logMsg);
		return;
	}

	AudioChunkDetails* details = chunk->GetDetails();

	if(details->m_marker == MEDIA_CHUNK_EOS_MARKER)
	{
		if(m_numChannels)
		{
			for(int i = 0; i < m_numChannels; i++)
			{
				CStdString statsChan;

				statsChan.Format("EOS s%d: misses:%d maxgap:%d oo:%d disc:%d", i+1,
					m_rtpMixerChannels[i]->m_seqNumMisses, m_rtpMixerChannels[i]->m_seqMaxGap,
					m_rtpMixerChannels[i]->m_seqNumOutOfOrder, m_rtpMixerChannels[i]->m_seqNumDiscontinuities);

				if(logMsg.size())
				{
					logMsg = logMsg + "  " + statsChan;
				}
				else
				{
					logMsg = statsChan;
				}
			}
		}
		else
		{
			logMsg.Format("[%s] EOS s1: misses:%d maxgap:%d oo:%d disc:%d  s2: misses:%d maxgap:%d oo:%d disc:%d", 
				m_trackingId, m_seqNumMissesS1, m_seqMaxGapS1, m_seqNumOutOfOrderS1, m_seqNumDiscontinuitiesS1,
				m_seqNumMissesS2, m_seqMaxGapS2, m_seqNumOutOfOrderS2, m_seqNumDiscontinuitiesS2);
			
		}

		LOG4CXX_INFO(m_log, logMsg);

		//logMsg.Format("COMPARISON: EOS s1: misses:%d maxgap:%d oo:%d disc:%d  s2: misses:%d maxgap:%d oo:%d disc:%d",
		//	m_seqNumMissesS1, m_seqMaxGapS1, m_seqNumOutOfOrderS1, m_seqNumDiscontinuitiesS1,
		//	m_seqNumMissesS2, m_seqMaxGapS2, m_seqNumOutOfOrderS2, m_seqNumDiscontinuitiesS2);

		//LOG4CXX_INFO(m_log, logMsg);

		CreateShipment(0, true);	// flush the buffer
		return;
	}
	else if(chunk->GetNumSamples() == 0)
	{
		logMsg.Format("[%s] Empty input chunk",m_trackingId);
		LOG4CXX_DEBUG(m_log, logMsg);
		return;
	}
	if(chunk->GetNumBytes() > 100000)
	{
		m_error = true;
		logMsg.Format("[%s] RtpMixer: input chunk too big",m_trackingId);
		LOG4CXX_ERROR(m_log,logMsg);
		return;
	}
	if(details->m_encoding != PcmAudio)
	{
		throw (CStdString("RtpMixer input audio must be PCM !"));
	}	

	unsigned int correctedTimestamp = 0;

	if(CONFIG.m_stereoRecording == true)
	{
		CreateChannels(details->m_channel);
	}

	if(details->m_channel == 1)
	{
		if(m_numChannels)
		{
			int chanIdx = 0;
			AudioChunkRef lastChunk = m_rtpMixerChannels[chanIdx]->m_lastChunk;

			if(lastChunk.get())
			{
				DoStats(details, lastChunk->GetDetails(), m_rtpMixerChannels[chanIdx]->m_seqNumMisses,
					m_rtpMixerChannels[chanIdx]->m_seqMaxGap, m_rtpMixerChannels[chanIdx]->m_seqNumOutOfOrder,
					m_rtpMixerChannels[chanIdx]->m_seqNumDiscontinuities);
			}
			m_rtpMixerChannels[chanIdx]->m_lastChunk = chunk;
		}

		if(m_lastChunkS1.get())
		{
			DoStats(details, m_lastChunkS1->GetDetails(), m_seqNumMissesS1, m_seqMaxGapS1, 
			m_seqNumOutOfOrderS1, m_seqNumDiscontinuitiesS1);
		}
		m_lastChunkS1 = chunk;
		correctedTimestamp = details->m_timestamp;
		m_numProcessedSamples += chunk->GetNumSamples();
		if(m_numProcessedSamples > 115200000)	// arbitrary high number (= 4 hours worth of 8KHz samples)
		{
			m_error = true;
			logMsg.Format("[%s] RtpMixer: Reached input stream size limit",m_trackingId);
			LOG4CXX_ERROR(m_log, logMsg);
			return;
		}
	}
	else if(details->m_channel == 2)
	{
		if(m_numChannels)
		{
			int chanIdx = 1;
			AudioChunkRef lastChunk = m_rtpMixerChannels[chanIdx]->m_lastChunk;

			if(lastChunk.get())
			{
				DoStats(details, lastChunk->GetDetails(), m_rtpMixerChannels[chanIdx]->m_seqNumMisses,
					m_rtpMixerChannels[chanIdx]->m_seqMaxGap, m_rtpMixerChannels[chanIdx]->m_seqNumOutOfOrder,
					m_rtpMixerChannels[chanIdx]->m_seqNumDiscontinuities);
			}
			m_rtpMixerChannels[chanIdx]->m_lastChunk = chunk;
		}

		if(m_lastChunkS2.get())
		{
			DoStats(details, m_lastChunkS2->GetDetails(), m_seqNumMissesS2, m_seqMaxGapS2, 
				m_seqNumOutOfOrderS2, m_seqNumDiscontinuitiesS2);
		}
		if(m_oneS1PacketState)
		{
			m_timestampCorrectiveDelta = (double)details->m_timestamp - (double)m_writeTimestamp;
			m_oneS1PacketState = false;
		}
		else
		{
			if(!m_timestampCorrectiveDelta && (m_writeTimestamp != 0))
			{
				m_timestampCorrectiveDelta = (double)details->m_timestamp - (double)m_writeTimestamp;
			}
		}

		m_lastChunkS2 = chunk;
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
		// Support for channel 3, 4, 5, ...
		if(m_numChannels)
		{
			int chanIdx = (details->m_channel)-1;
			AudioChunkRef lastChunk = m_rtpMixerChannels[chanIdx]->m_lastChunk;

			if(lastChunk.get())
			{
				DoStats(details, lastChunk->GetDetails(), m_rtpMixerChannels[chanIdx]->m_seqNumMisses,
					m_rtpMixerChannels[chanIdx]->m_seqMaxGap, m_rtpMixerChannels[chanIdx]->m_seqNumOutOfOrder,
					m_rtpMixerChannels[chanIdx]->m_seqNumDiscontinuities);
			}
			m_rtpMixerChannels[chanIdx]->m_lastChunk = chunk;

			// Corrective delta always only applied to side 2 and other sides.
			double tmp = (double)details->m_timestamp - m_rtpMixerChannels[chanIdx]->m_timestampCorrectiveDelta;
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
				logMsg.Format("[%s] Invalid Channel:%d",m_trackingId,details->m_channel);
				LOG4CXX_ERROR(m_log, logMsg);
			}
		}
	}
	unsigned int rtpEndTimestamp = correctedTimestamp + chunk->GetNumSamples();

	if(m_log->isDebugEnabled())
	{
		logMsg.Format("[%s] New chunk, s%d seq:%u ts:%u corr-ts:%u",m_trackingId, details->m_channel, details->m_sequenceNumber, details->m_timestamp, correctedTimestamp);
		LOG4CXX_DEBUG(m_log, logMsg);
	}

	if(m_writeTimestamp == 0)
	{
		if(details->m_channel == 1)
		{
			// First RTP packet of the session
			logMsg.Format("[%s] First chunk",m_trackingId);
			LOG4CXX_DEBUG(m_log, logMsg);
			m_writeTimestamp = correctedTimestamp;
			m_readTimestamp = m_writeTimestamp;
			m_oneS1PacketState = true;
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
		logMsg.Format("[%s] free:%u used:%u wr:%x rd:%x wrts:%u rdts:%d",m_trackingId, FreeSpace(), UsedSpace(), m_writePtr-m_buffer, m_readPtr-m_buffer, m_writeTimestamp, m_readTimestamp);
		LOG4CXX_DEBUG(m_log, logMsg);
	}
}

void RtpMixer::ManageOutOfRangeTimestamp(AudioChunkRef& chunk)
{
	CStdString logMsg;

	AudioChunkDetails* details = chunk->GetDetails();

	logMsg.Format("[%s] ManageOutOfRangeTimestamp - channel:%d",m_trackingId, details->m_channel);
	LOG4CXX_DEBUG(m_log, logMsg);
	if(details->m_channel == 1)
	{
		// 1. Ship what we have
		CreateShipment(0, true);

		// 2. Reset circular buffer and add this new chunk
		Reset(details->m_timestamp);
		StoreRtpPacket(chunk ,details->m_timestamp);

		// 3. Reset corrective delta to force reevaluation.
		m_timestampCorrectiveDelta = 0.0;
	}
	else if(details->m_channel == 2)
	{
		// Calculate timestamp corrective delta so that next channel-2 chunk 
		// will be in the circular buffer timestamp window.
		m_timestampCorrectiveDelta = (double)details->m_timestamp - (double)m_writeTimestamp;
	}
	else
	{
		if(m_numChannels)
		{
			int chanIdx = details->m_channel - 1;

			// Calculate timestamp corrective delta so that next channel-x chunk
			// will be in the circular buffer timestamp window.
			m_rtpMixerChannels[chanIdx]->m_timestampCorrectiveDelta = (double)details->m_timestamp - (double)m_writeTimestamp;
		}
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

// Writes to the internal buffer without any size verification
void RtpMixer::StoreRtpPacket(AudioChunkRef& audioChunk, unsigned int correctedTimestamp)
{
	CStdString debug;
	AudioChunkDetails* details = audioChunk->GetDetails();
	RtpMixerChannelRef myChannel;

	if(m_numChannels)
	{
		// Define this once and for all
		myChannel = m_rtpMixerChannels[details->m_channel - 1];
	}

	// 1. Silence from write pointer until end of RTP packet
	// Doing this also will help us determine the offset at the channel's circular
	// buffer at which we should write - for stereo recording
	unsigned int endRtpTimestamp = correctedTimestamp + audioChunk->GetNumSamples();
	if (endRtpTimestamp > m_writeTimestamp)
	{
		for(unsigned int i=0; i<(endRtpTimestamp - m_writeTimestamp); i++)
		{
			*m_writePtr = 0;

			if(m_numChannels)
			{
				for(int x = 0; x < m_numChannels; x++)
				{
					// We follow in step and zero the bytes in the buffers of all the
					// RtpMixer channels, in preparation for the write - this means that
					// if there is no data for this timestamp in any one buffer,
					// then we have silence there
					short *myZeroPtr = m_rtpMixerChannels[x]->m_buffer + (m_writePtr - m_buffer);
					*myZeroPtr = 0;
				}
			}

			m_writePtr++;
			if(m_writePtr >= m_bufferEnd)
			{
				m_writePtr = m_buffer;
			}
		}
		int silenceSize = endRtpTimestamp - m_writeTimestamp;
		m_writeTimestamp = endRtpTimestamp;
		debug.Format("[%s] Zeroed %d samples, wr:%x wrts:%u",m_trackingId, silenceSize, m_writePtr-m_buffer, m_writeTimestamp);
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

		// Follow in step and save the payload in the respective channel buffer
		if(m_numChannels)
		{
			short* myChannelTempWritePtr = myChannel->m_buffer + (tempWritePtr - m_buffer);
			*myChannelTempWritePtr = (short)payload[i];
		}

		tempWritePtr++;
		if(tempWritePtr >= m_bufferEnd)
		{
			tempWritePtr = m_buffer;
		}
	}
	debug.Format("[%s] Copied %d samples, tmpwr:%x",m_trackingId, audioChunk->GetNumSamples(), tempWritePtr-m_buffer);
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

void RtpMixer::HandleMixedOutput(AudioChunkRef &chunk, AudioChunkDetails& details, short *readPtr)
{
	if(CONFIG.m_stereoRecording == false)
	{
		return;
	}
	if(CONFIG.m_tapeNumChannels <= 1)
	{
		return;
	}

	// If we're required to output a different number of channels than we currently
	// know about in the RtpMixer, we will need to adjust the chunk such that it has
	// the exact number of channel buffers we're required to output i.e so that
	// the number of channel buffers in the chunk == CONFIG.m_tapeNumChannels
	if(CONFIG.m_tapeNumChannels != m_numChannels)
	{
		chunk.reset(new AudioChunk(CONFIG.m_tapeNumChannels));
		chunk->SetBuffer((void*)readPtr, details);
	}

	// 2 is hardcoded here because as per instructions we are currently to support
	// only output of 2 channels.  So tapeChannels[0] will contain a mixed
	// output of all even channels in the system (2, 4, 6...) and
	// tapeChannels[1] will contain a mixed output of all odd channels
	// in the system (1, 3, 5...).  Now, the rest of the chunks for the rest of
	// the output channels i.e output channel 3, 4, 5... will be zeroed
	short *tapeChannels[2];

	tapeChannels[0] = (short*)malloc(details.m_numBytes); // Even channels mixed into here
	tapeChannels[1] = (short*)malloc(details.m_numBytes); // Odd channels mixed into here

	if(!tapeChannels[0] || !tapeChannels[1])
	{
		CStdString logMsg;

		logMsg.Format("[%s] Out of memory in RtpMixer::AddMixedChannels while allocating %d bytes",m_trackingId, details.m_numBytes);
		LOG4CXX_ERROR(m_log, logMsg);

		if(tapeChannels[0])
		{
			free(tapeChannels[0]);
		}
		if(tapeChannels[1])
		{
			free(tapeChannels[1]);
		}

		return;
	}

	memset(tapeChannels[0], 0, details.m_numBytes);
	memset(tapeChannels[1], 0, details.m_numBytes);

	int chanNo = 0;
	int chanIdx = 0;

	for(int i = 0; i < m_numChannels; i++)
	{
		chanNo = i+1;
		chanIdx = i;

		for(unsigned int j = 0; j < (details.m_numBytes/2); j++)
		{
			int sample;
			int saveIdx = 0;

			if(!(chanNo % 2))
			{
				saveIdx = 0;
			}
			else
			{
				saveIdx = 1;
			}

			short *chanReadPtr = (short*)m_rtpMixerChannels[chanIdx]->m_buffer + ((readPtr - m_buffer) + j);

			sample = tapeChannels[saveIdx][j] + *chanReadPtr;
			if (sample > 32767)
			{
				sample = 32767;
			}
			if(sample < -32768)
			{
				sample = -32768;
			}

			tapeChannels[saveIdx][j] = (short)sample;
		}
	}

	// Set the two supported output channels
	chunk->SetBuffer((void*)tapeChannels[0], details, 1);
	chunk->SetBuffer((void*)tapeChannels[1], details, 2);

	memset(tapeChannels[0], 0, details.m_numBytes);

	// If we're required to output more than two channels then we need to
	// zero the rest of the output channels i.e 3, 4, 5...
	if(CONFIG.m_tapeNumChannels > 2)
	{
		for(int i = 2; i < CONFIG.m_tapeNumChannels; i++)
		{
			chunk->SetBuffer((void*)tapeChannels[0], details, (i+1));
		}
	}

	free(tapeChannels[0]);
	free(tapeChannels[1]);
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
	AudioChunkRef chunk;
	AudioChunkDetails details;

	if(m_numChannels)
	{
		chunk.reset(new AudioChunk(m_numChannels));
		details.m_channel = 100;
	}
	else
	{
		chunk.reset(new AudioChunk());
	}

	details.m_encoding = PcmAudio;
	details.m_numBytes = byteSize;
	if(CheckChunkDetails(details))
	{
		chunk->SetBuffer((void*)m_readPtr, details);
		if(CONFIG.m_stereoRecording == true)
		{
			HandleMixedOutput(chunk, details, m_readPtr);
		}
		m_outputQueue.push(chunk);
	}
	m_shippedSamples += shortSize;
	m_readPtr = CircularPointerAddOffset(m_readPtr ,shortSize);
	m_readTimestamp += shortSize;

	CStdString debug;
	debug.Format("[%s] Ship %d samples, rd:%x rdts:%u",m_trackingId, shortSize, m_readPtr-m_buffer, m_readTimestamp);
	LOG4CXX_DEBUG(m_log, debug);


	// 2. ship from beginning of buffer until stop ptr
	if(bufferWrapped) 
	{
		AudioChunkDetails details;

		shortSize = wrappedStopPtr - m_buffer;
		byteSize = shortSize*2;

		if(m_numChannels)
		{
			chunk.reset(new AudioChunk(m_numChannels));
			details.m_channel = 100;
		}
		else
		{
			chunk.reset(new AudioChunk());
		}

		details.m_encoding = PcmAudio;
		details.m_numBytes = byteSize;

		if(CheckChunkDetails(details))
		{
			chunk->SetBuffer((void*)m_buffer, details);
			if(CONFIG.m_stereoRecording == true)
			{
				HandleMixedOutput(chunk, details, m_buffer);
			}
			m_outputQueue.push(chunk);
		}

		m_shippedSamples += shortSize;
		m_readPtr = CircularPointerAddOffset(m_readPtr ,shortSize);
		m_readTimestamp += shortSize;
		debug.Format("[%s] Ship wrapped %d samples, rd:%x rdts:%u",m_trackingId, shortSize, m_readPtr-m_buffer, m_readTimestamp);
		LOG4CXX_DEBUG(m_log, debug);
	}

	// 3. ship silence
	if (silenceSize)
	{
		byteSize = silenceSize*2;
		AudioChunkRef chunk;
		AudioChunkDetails details;

		if(CONFIG.m_stereoRecording == true)
		{
			if(CONFIG.m_tapeNumChannels > 1)
			{
				chunk.reset(new AudioChunk(CONFIG.m_tapeNumChannels));
				details.m_channel = 100;
			}
			else
			{
				chunk.reset(new AudioChunk());
			}
		}
		else
		{
			chunk.reset(new AudioChunk());
		}

		details.m_encoding = PcmAudio;
		details.m_numBytes = byteSize;
		if(CheckChunkDetails(details))
		{
			chunk->CreateBuffer(details);
			m_outputQueue.push(chunk);
		}
		m_shippedSamples += silenceSize;
		m_readPtr = CircularPointerAddOffset(m_readPtr ,silenceSize);
		m_readTimestamp += silenceSize;
		debug.Format("[%s] Ship %d silence samples, rd:%x rdts:%u",m_trackingId, silenceSize, m_readPtr-m_buffer, m_readTimestamp);
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
		CStdString logMsg;
		logMsg.Format("[%s] RtpMixer: output chunk too big",m_trackingId);
		LOG4CXX_ERROR(m_log,logMsg);
		return false;
	}
	if(details.m_numBytes == 0)
	{
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

