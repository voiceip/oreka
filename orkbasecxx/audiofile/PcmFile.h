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

#ifndef __PCMFILE_H__
#define __PCMFILE_H__

#include "audiofile/AudioFile.h"

#define PCM_FILE_DEFAULT_CHUNK_NUM_SAMPLES 8000

/** File class for raw 16 bit signed PCM output */
class DLL_IMPORT_EXPORT_ORKBASE PcmFile : public AudioFile
{
public:
	PcmFile();
	~PcmFile();

	void Open(CStdString& filename, fileOpenModeEnum mode, bool stereo = false, int sampleRate = 8000);
	void Close();

	void WriteChunk(AudioChunkRef chunkRef);
	int ReadChunkMono(AudioChunkRef& chunkRef);

	CStdString GetExtension();
protected:

	FILE* m_stream;
};

#endif

