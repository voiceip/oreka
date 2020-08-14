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
#ifndef __SILKDECODER_H__
#define __SILKDECODER_H__ 1

#include "LogManager.h"
#include "Filter.h"
#define MAX_BYTES_PER_FRAME     1024
#define MAX_INPUT_FRAMES        5
#define MAX_FRAME_LENGTH        480
#define FRAME_LENGTH_MS         20
#define MAX_API_FS_KHZ          48
#define MAX_LBRR_DELAY          2
extern "C"
{
#include "SKP_Silk_SDK_API.h"
#include "SKP_Silk_SigProc_FIX.h"
}

#ifdef WIN32
#undef DLL_IMPORT_EXPORT_ORKBASE 
#define DLL_IMPORT_EXPORT_ORKBASE
#endif

class DLL_IMPORT_EXPORT_ORKBASE SilkCodecDecoder : public Filter
{
public:
	SilkCodecDecoder();
	~SilkCodecDecoder();

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
	void Decode8KhzChunk(SKP_uint8* inputBuffer, SKP_int16* outputBuffer, int numBytes);
	void Decode16KhzChunk(SKP_uint8* inputBuffer, SKP_int16* outputBuffer, int numBytes);
	SKP_SILK_SDK_DecControlStruct m_decControl;
	SKP_int32 m_decSizeBytes;
	void      *m_psDec;
	int m_sampleRate8KhzMultiplier;
	AudioChunkRef m_outputChunk;
	unsigned int m_lastRtpSeq;
	unsigned int m_lastRtpTs;
	int m_channels;
	bool m_initialized;
};

#endif
