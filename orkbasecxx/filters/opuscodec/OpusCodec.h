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
#ifndef __OPUSDECODER_H__
#define __OPUSDECODER_H__ 1

#include "LogManager.h"
#include "Filter.h"
#include "opus.h"

class DLL_IMPORT_EXPORT_ORKBASE OpusCodecDecoder : public Filter
{
public:
	OpusCodecDecoder();
	~OpusCodecDecoder();

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
	AudioChunkRef m_outputChunk;
	unsigned int m_lastRtpSeq;
	unsigned int m_lastRtpTs;
    int m_channels;
    int m_max_frame_size;
    OpusDecoder* m_decoder;
    int m_sampleRate8KhzMultiplier;
    bool m_initialized;
	bool m_errorLogged;
	bool m_warnLogged;

};

#endif
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
#ifndef __OPUSDECODER_H__
#define __OPUSDECODER_H__ 1

#include "LogManager.h"
#include "Filter.h"
#include "opus.h"

class DLL_IMPORT_EXPORT_ORKBASE OpusCodecDecoder : public Filter
{
public:
	OpusCodecDecoder();
	~OpusCodecDecoder();

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
	AudioChunkRef m_outputChunk;
	unsigned int m_lastRtpSeq;
	unsigned int m_lastRtpTs;
    int m_channels;
    int m_max_frame_size;
    OpusDecoder* m_decoder;
    int m_sampleRate8KhzMultiplier;
    bool m_initialized;

};

#endif
