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
#include "LogManager.h"
#include "Filter.h"
#include <math.h>

class DLL_IMPORT_EXPORT_ORKBASE AudioGainFilter : public Filter
{
public:
	AudioGainFilter();
	~AudioGainFilter();

	FilterRef __CDECL__ Instanciate();
	void __CDECL__ AudioChunkIn(AudioChunkRef& chunk);
	void __CDECL__ AudioChunkOut(AudioChunkRef& chunk);
	AudioEncodingEnum __CDECL__ GetInputAudioEncoding();
	AudioEncodingEnum __CDECL__ GetOutputAudioEncoding();
	CStdString __CDECL__ GetName();
	inline void __CDECL__ CaptureEventIn(CaptureEventRef& event);
	inline void __CDECL__ CaptureEventOut(CaptureEventRef& event);

private:
	AudioChunkRef m_outputAudioChunk;
	LoggerPtr m_log;
	int m_numEncodingErrors;
};

