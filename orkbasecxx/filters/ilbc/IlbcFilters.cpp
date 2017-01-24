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
#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "IlbcFilters.h"

IlbcToPcmFilter::IlbcToPcmFilter()
{
	/* Initialize iLBC decoder states */
	memset(&dec30, 0, sizeof(dec30));
        memset(&dec20, 0, sizeof(dec20));

	initDecode(&dec30, 30, 0);
        initDecode(&dec20, 20, 0);

	this->s_ilbclog = Logger::getLogger("iLBC");
#if 0
	LOG4CXX_INFO(this->s_ilbclog, "Initialized new iLBC decoder");
#endif
}

IlbcToPcmFilter::~IlbcToPcmFilter()
{
        memset(&dec30, 0, sizeof(dec30));
        memset(&dec20, 0, sizeof(dec20));

#if 0
	LOG4CXX_INFO(this->s_ilbclog, "Decommissioned iLBC decoder");
#endif
}

FilterRef IlbcToPcmFilter::Instanciate()
{
	FilterRef Filter(new IlbcToPcmFilter());
	return Filter;
}

void IlbcToPcmFilter::AudioChunkIn(AudioChunkRef& inputAudioChunk)
{
	int r_samples = 0, fs = 0, i = 0, pos = 0, o_samples = 0, j = 0;
	float ilbcf[240];

	m_outputAudioChunk.reset();

	if(inputAudioChunk.get() == NULL) {
		return;
	}

	if(inputAudioChunk->GetNumSamples() == 0) {
		return;
	}

	AudioChunkDetails outputDetails = *inputAudioChunk->GetDetails();

	if(SupportsInputRtpPayloadType(outputDetails.m_rtpPayloadType) == false)
	{
		return;
	}

	r_samples = inputAudioChunk->GetNumSamples();
	if((r_samples % 50) && (r_samples % 38)) {
		/* Strange iLBC frame that is not a multiple of 50 bytes
		 * (30ms frame) and neither is it a multiple of 38 bytes
		 * (20ms frame). We should probably log something? */
                LOG4CXX_ERROR(this->s_ilbclog, "Error, received iLBC frame is not a multiple of 50 or 38!");
		return;
	}

	if(!(r_samples % 50)) {
		i = r_samples / 50;
		o_samples = i * 240;
		fs = 50;
#if 0
		LOG4CXX_INFO(this->s_ilbclog, "Frame size is 50 bytes");
#endif
	} else {
		i = r_samples / 38;
		o_samples = i * 160;
		fs = 38;
#if 0
		LOG4CXX_INFO(this->s_ilbclog, "Frame size is 38 bytes");
#endif
	}

	m_outputAudioChunk.reset(new AudioChunk());
	outputDetails.m_rtpPayloadType = -1;
	outputDetails.m_encoding = PcmAudio;

	outputDetails.m_numBytes = (o_samples * 2);
	short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(outputDetails);
	unsigned char* inputBuffer = (unsigned char*)inputAudioChunk->m_pBuffer;

	for(i = 0; i < r_samples; i += fs) {
		if((pos+((fs == 50) ? 240 : 160)) <= o_samples) {
			if(fs == 50) {
				iLBC_decode(ilbcf, inputBuffer+i, &dec30, 1);

				for(j = 0; j < 240; j++) {
					outputBuffer[pos + j] = (short)ilbcf[j];
				}

				pos += 240;
			} else {
				iLBC_decode(ilbcf, inputBuffer+i, &dec20, 1);

				for(j = 0; j < 160; j++) {
                                        outputBuffer[pos + j] = (short)ilbcf[j];
                                }

                                pos += 160;
			}
		} else {
			/* This should ordinarily never happen.
			 * Log something? */
			CStdString logMsg;

			logMsg.Format("Strange, I ran out of space: pos=%d, o_samples=%d, r_samples=%d, i=%d, "
					"(pos+((fs == 50) ? 240 : 160))=%d",
					pos, o_samples, r_samples, i, (pos+((fs == 50) ? 240 : 160)));
			LOG4CXX_ERROR(this->s_ilbclog, logMsg);
			return;
		}
	}
}

void IlbcToPcmFilter::AudioChunkOut(AudioChunkRef& chunk)
{
	chunk = m_outputAudioChunk;
}

AudioEncodingEnum IlbcToPcmFilter::GetInputAudioEncoding()
{
	return IlbcAudio;
}

AudioEncodingEnum IlbcToPcmFilter::GetOutputAudioEncoding()
{
	return PcmAudio;
}

CStdString IlbcToPcmFilter::GetName()
{
	return "IlbcToPcm";
}

bool IlbcToPcmFilter::SupportsInputRtpPayloadType(int rtpPayloadType)
{
	return (rtpPayloadType == 0x61 || rtpPayloadType == 0x62 || rtpPayloadType == 63 );
}

void IlbcToPcmFilter::CaptureEventIn(CaptureEventRef& event)
{
	;
}

void IlbcToPcmFilter::CaptureEventOut(CaptureEventRef& event)
{
	;
}

