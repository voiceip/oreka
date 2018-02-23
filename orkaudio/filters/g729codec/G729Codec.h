/*
 * Oreka -- A media capture and retrieval platform
 *
 * G729 Plugin
 * Author Kinshuk Bairagi
 *
 */

#ifndef __G729CODEC_H__
#define __G729CODEC_H__ 1

#include "LogManager.h"
#include "Filter.h"

extern "C"
{
  #include "bcg729/encoder.h"
  #include "bcg729/decoder.h"
}

class DLL_IMPORT_EXPORT_ORKBASE G729CodecDecoder : public Filter
{
  public:
    G729CodecDecoder();
    ~G729CodecDecoder();

    FilterRef __CDECL__ Instanciate();
    void __CDECL__ AudioChunkIn(AudioChunkRef& chunk);
    void __CDECL__ AudioChunkOut(AudioChunkRef& chunk);
    AudioEncodingEnum __CDECL__ GetInputAudioEncoding();
    AudioEncodingEnum __CDECL__ GetOutputAudioEncoding();
    CStdString __CDECL__ GetName();
    bool __CDECL__ SupportsInputRtpPayloadType(int rtpm_payloadType );
    void __CDECL__ CaptureEventIn(CaptureEventRef& event);
    void __CDECL__ CaptureEventOut(CaptureEventRef& event);

  private:
    AudioChunkRef m_outputAudioChunk;
    bcg729DecoderChannelContextStruct *decoder;
    bool m_initialized;

};

#endif
