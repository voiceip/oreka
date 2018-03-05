/*
 * Oreka -- A media capture and retrieval platform
 *
 */
#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "G729Codec.h"
#include <log4cxx/logger.h>
#include "AudioCapture.h"
#include <iostream>
#include <string>

static log4cxx::LoggerPtr s_log = log4cxx::Logger::getLogger("codec.g729");
 
template<class T>
std::string toString(const T &value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

G729CodecDecoder::G729CodecDecoder()
{
    decoder = initBcg729DecoderChannel();
}

G729CodecDecoder::~G729CodecDecoder()
{
    closeBcg729DecoderChannel(decoder);
}

FilterRef G729CodecDecoder::Instanciate()
{
    FilterRef Filter(new G729CodecDecoder());
    return Filter;
}

void G729CodecDecoder::AudioChunkIn(AudioChunkRef& inputAudioChunk)
{
    int16_t pcmdata[8000];
    int input_size = 0;
    int output_size;
    CStdString logMsg;

    memset(pcmdata, 0, sizeof(pcmdata));
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

    unsigned char* inputBuffer = (unsigned char*)inputAudioChunk->m_pBuffer;
    input_size = outputDetails.m_numBytes;

    LOG4CXX_DEBUG(s_log, "G729 AudioChunkIn Size : " + toString(input_size));

    int16_t *ddp = pcmdata;
    uint8_t *edp = inputBuffer;

    if(input_size == 0){
       /* Native PLC interpolation */
       LOG4CXX_INFO(s_log, "G729  zero length frame");
       bcg729Decoder(decoder, NULL, 0, 1, 0, 0, ddp);
       ddp += 80;
       output_size = 160;
    } else {
        int framesize;
        uint32_t new_len = 0;
        for (int x = 0; x < input_size; x += framesize) {
            uint8_t isSID = (input_size - x < 8) ? 1 : 0;
            framesize = (isSID == 1) ? 2 : 10;
            bcg729Decoder(decoder, edp, 10, 0, isSID, 0, ddp);
            ddp += 80;
            edp += framesize;
            new_len += 160;
        }
        output_size = new_len;
    }

    m_outputAudioChunk.reset(new AudioChunk());
    outputDetails.m_rtpPayloadType = -1;
    outputDetails.m_encoding = PcmAudio;
    outputDetails.m_numBytes = output_size;

    LOG4CXX_DEBUG(s_log, "G729 AudioChunkOut Size : " + toString(outputDetails.m_numBytes));
    short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(outputDetails);
    memcpy(outputBuffer, pcmdata, outputDetails.m_numBytes);
}

void G729CodecDecoder::AudioChunkOut(AudioChunkRef& chunk)
{
    chunk = m_outputAudioChunk;
}

AudioEncodingEnum G729CodecDecoder::GetInputAudioEncoding()
{
    return G729Audio;
}

AudioEncodingEnum G729CodecDecoder::GetOutputAudioEncoding()
{
    return PcmAudio;
}

CStdString G729CodecDecoder::GetName()
{
    return "G729CodecDecoder";
}

bool G729CodecDecoder::SupportsInputRtpPayloadType(int rtpPayloadType)
{
    return rtpPayloadType == 18;
}

void G729CodecDecoder::CaptureEventIn(CaptureEventRef& event)
{
    ;
}

void G729CodecDecoder::CaptureEventOut(CaptureEventRef& event)
{
    ;
}

extern "C"
{
DLL_EXPORT void __CDECL__ OrkInitialize()
{
    LOG4CXX_INFO(s_log, "G729 codec filter starting.");
    FilterRef filter(new G729CodecDecoder());
    FilterRegistry::instance()->RegisterFilter(filter);
    LOG4CXX_INFO(s_log, "G729 codec filter initialized.");
}
}
