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
#include "Filter.h"
#include "LogManager.h"
extern "C"
{
#include "iLBC_encode.h"
#include "iLBC_decode.h"
}

class DLL_IMPORT_EXPORT_ORKBASE IlbcToPcmFilter : public Filter
{
public:
	IlbcToPcmFilter();
	~IlbcToPcmFilter();

	FilterRef __CDECL__ Instanciate();
	void __CDECL__ AudioChunkIn(AudioChunkRef& chunk);
	void __CDECL__ AudioChunkOut(AudioChunkRef& chunk);
	AudioEncodingEnum __CDECL__ GetInputAudioEncoding();
	AudioEncodingEnum __CDECL__ GetOutputAudioEncoding();
	CStdString __CDECL__ GetName();
	int __CDECL__ GetInputRtpPayloadType();
	void __CDECL__ CaptureEventIn(CaptureEventRef& event);
	void __CDECL__ CaptureEventOut(CaptureEventRef& event);

private:
	AudioChunkRef m_outputAudioChunk;
	/* 30ms frames */
	iLBC_Dec_Inst_t dec30;

	/* 20ms frames */
	iLBC_Dec_Inst_t dec20;

	LoggerPtr s_ilbclog;
};

