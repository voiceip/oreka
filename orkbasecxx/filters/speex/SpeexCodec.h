#ifndef __SPEEXCODEC_H__
#define __SPEEXCODEC_H__

#include "LogManager.h"
#include "Filter.h"
#include "speex/speex.h"

class DLL_IMPORT_EXPORT_ORKBASE SpeexDecoder : public Filter
{
public:
	SpeexDecoder();
	~SpeexDecoder();

	FilterRef __CDECL__ Instanciate();
	void __CDECL__ AudioChunkIn(AudioChunkRef& chunk);
	void __CDECL__ AudioChunkOut(AudioChunkRef& chunk);
	AudioEncodingEnum __CDECL__ GetInputAudioEncoding();
	AudioEncodingEnum __CDECL__ GetOutputAudioEncoding();
	CStdString __CDECL__ GetName();
	bool __CDECL__ SupportsInputRtpPayloadType(int rtpPayloadType );
	void __CDECL__ CaptureEventIn(CaptureEventRef& event);
	void __CDECL__ CaptureEventOut(CaptureEventRef& event);

private:
	AudioChunkRef m_outputAudioChunk;
	SpeexBits m_bits;
	void * m_state;
	bool m_initialized;
	int m_frameSize;
	int m_enh;
};

#endif
