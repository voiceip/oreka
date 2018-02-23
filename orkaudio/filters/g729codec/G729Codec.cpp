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

#define L_FRAME 80
#define L_FRAME_COMPRESSED 10

template<class T>
std::string toString(const T &value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

G729CodecDecoder::G729CodecDecoder()
{
    // Initialize decoder
    m_initialized = false;
    // void *handle = dlopen ("libbcg729.so", RTLD_LAZY);
    // if (!handle) {
    //         LOG4CXX_ERROR(LOG.rootLog, CStdString("Couldn't load Bcg729 plugin"));
    //     }
    // else {
    //   *(void **) (&initer) = dlsym(handle,"initBcg729DecoderChannel");
    //   *(void **) (&decoder) = dlsym(handle,"bcg729Decoder");
    //   if ((error = dlerror()) != NULL)  {
    //       std::string str(error);
    //       LOG4CXX_ERROR(LOG.rootLog, CStdString("Couldn't load Bcg729 plugin's functions => "+str));
    //   } else {
    decoder = initBcg729DecoderChannel();
    m_initialized = true;
    // }
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
    short outputSample = 0;
    short pcmdata[8000];
    int input_size = 0;
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
        logMsg.Format("Wrong input RTP payload type:%i", outputDetails.m_rtpPayloadType);
        LOG4CXX_DEBUG(s_log, logMsg);
        return;
    }

    unsigned char* inputBuffer = (unsigned char*)inputAudioChunk->m_pBuffer;
    input_size = outputDetails.m_numBytes;

    LOG4CXX_INFO(s_log, "G729 AudioChunkIn Size : " + toString(input_size));

    char *ret = (char*)malloc(10);

    memcpy(ret, inputBuffer, 10);
    char* firstFragment = ret;
    ret = (char*)malloc(10);
    memcpy(ret, inputBuffer+10, 10);
    char* secondFragment = ret;

    LOG4CXX_INFO(s_log, CStdString("G729 AudioChunkIn buffer separated into two 10 bit buffers"));

    int16_t outputChannelABuffer[80]; /* output buffer: the reconstructed signal */
    int16_t outputChannelBBuffer[80]; /* output buffer: the reconstructed signal */
    uint8_t bitStream[10]; /* binary input for the decoder */

    LOG4CXX_ERROR(LOG.rootLog, CStdString("G729 AudioChunkIn Buffers Created"));

    LOG4CXX_ERROR(LOG.rootLog, CStdString("G729 AudioChunkIn Decoder Initialized"));

     for(int i=0; i < 10 ; i++){
         bitStream[i] = (uint8_t)atoi(&firstFragment[i]);
     }

     LOG4CXX_ERROR(LOG.rootLog, CStdString("G729 AudioChunkIn First BitStream Created"));

     bcg729Decoder(decoder,  bitStream, 1, outputChannelABuffer);

     LOG4CXX_ERROR(LOG.rootLog, CStdString("G729 AudioChunkIn First Stream Decoding DONE"));

     for(int i=0; i < 10 ; i++){
         bitStream[i] = (uint8_t)atoi(&secondFragment[i]);
     }

     LOG4CXX_ERROR(LOG.rootLog, CStdString("G729 AudioChunkIn Second BitStream Created"));

     bcg729Decoder(decoder,  bitStream, 0, outputChannelBBuffer);

     LOG4CXX_ERROR(LOG.rootLog, CStdString("G729 AudioChunkIn Second Stream Decoding DONE"));

    // if(input_size == 0){
    //    /* Native PLC interpolation */
    //    LOG4CXX_INFO(s_log, "G729  zero length frame");
    // } else {
    //
    //   int framesize;
    //   int x;
    //   uint32_t new_len = 0;
    //
    //   for(x = 0; x < input_size; x += framesize) {
    //     uint8_t isSID = (input_size - x < 8) ? 1 : 0;
    //         framesize = (isSID==1) ? 2 : 10;
    //         bcg729Decoder(decoder, inputBuffer, input_size, 0, isSID, 0, *pcmdata);
    //       ddp += 80;
    //       edp += framesize;
    //       new_len += 160;
    //     }
    //
    //     std::cout << "new_len" << new_len << "\n";
    //
    //
    // }
    //

//    int j = 0;
//    for(int i=0; i<input_size; i++)
//    {
//        unsigned char inputSample = inputBuffer[i];
//        unsigned char lower4bits = inputSample & 0xF;
//        unsigned char upper4bits = (inputSample>>4) & 0xF;
//        outputSample = g721_decoder(lower4bits, AUDIO_ENCODING_LINEAR, &m_decoderState);
//        memcpy(pcmdata + j, &outputSample, sizeof(short));
//        outputSample = g721_decoder(upper4bits, AUDIO_ENCODING_LINEAR, &m_decoderState);
//        memcpy(pcmdata + j + 1, &outputSample, sizeof(short));
//        j += 2;
//    }
//
    m_outputAudioChunk.reset(new AudioChunk());
    outputDetails.m_rtpPayloadType = -1;
    outputDetails.m_encoding = PcmAudio;
    outputDetails.m_numBytes = 80 * 2;
    // outputDetails.m_numBytes = input_size * (L_FRAME / L_FRAME_COMPRESSED) * sizeof(short);

    LOG4CXX_INFO(s_log, "G729 AudioChunkOut Size : " + toString(outputDetails.m_numBytes));
    short* outputBuffer = (short*)m_outputAudioChunk->CreateBuffer(outputDetails);

   //  for(; byteIndex <= inByteCount - L_FRAME_COMPRESSED; byteIndex += L_FRAME_COMPRESSED) {
   //    bcg729Decoder(decoder,&inputBuffer[byteIndex], outputBuffer, 0);
   //    outputBuffer += L_FRAME;
   // }


    //
    // std::cout << "outputBuffer \n" ;
    // for(int i = 0 ; i <80 ; i++){
    //   std::cout << outputBuffer[i] << " ";
    // }
    // std::cout << "\n";
    //
    int i;
    for(i = 0; i < 80; i++){
        pcmdata[i] = outputChannelABuffer[i];
        pcmdata[i+80] = outputChannelBBuffer[i];
    }

    // for (int j = i+1 ; j < outputDetails.m_numBytes ;  j++){
    //   pcmdata[j] = 0;
    // }
    //
    // std::cout << "outputBufferFinal \n" ;
    // for(int i = 0 ; i <80 ; i++){
    //   std::cout << outputBufferFinal[i] << " ";
    // }
    // std::cout << "\n";


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
