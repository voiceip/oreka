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
#ifndef __G721DECODER_H__
#define __G721DECODER_H__ 1

#include "LogManager.h"
#include "Filter.h"

#ifdef __cplusplus
extern "C"
{
	#include "g72x.h"
}
#endif
class DLL_IMPORT_EXPORT_ORKBASE G721CodecDecoder : public Filter
{
public:
	G721CodecDecoder();
	~G721CodecDecoder();

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
	struct g72x_state m_decoderState;
};

#endif
