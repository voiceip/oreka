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

#ifndef __AUDIOFILE_H__
#define __AUDIOFILE_H__

#include "shared_ptr.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_sys_stat.h"

#include "StdString.h"
#include "AudioCapture.h"


/** Base class for all file accessor classes. */
class DLL_IMPORT_EXPORT_ORKBASE AudioFile
{
public:
	AudioFile();
	typedef enum {READ = 0, WRITE = 1} fileOpenModeEnum;

	/** Open audio file for reading or writing.
		Filename should include path information but not extension (which is automatically appended)
		If the underlying format does not support stereo, data is transparently read from two files in read mode 
		or two files are transparently written to in write mode */
	virtual void Open(CStdString& filename, fileOpenModeEnum mode, bool stereo = false, int sampleRate = 8000) = 0;
	/** Same as above but uses the intenal filename */
	void Open(fileOpenModeEnum mode, bool stereo = false, int sampleRate = 8000);
	/** Explicitely close the underlying file(s). This is also done automatically by the destructor */
	virtual void Close() = 0;

	/** Writes a chunk of audio to disk.
		If stereo capture, this represents the local party */
	virtual void WriteChunk(AudioChunkRef chunkRef) = 0;
	/** Writes a chunk of audio from the remote pary to disk (if stereo capture) */
	//virtual bool WriteRemoteChunk(AudioChunkRef chunkRef) = 0;
	/** Reads a chunk of audio stereo-wise 
		If underlying storage is mono, remoteChunk will be NULL 
		ReadChunkStereo guarantees that local and remote chunks returned are in sync */
	//virtual bool ReadChunkStereo(AudioChunkRef& chunk, AudioChunkRef& remoteChunk) = 0;
	/** Reads a chunk of audio mono-wise
		If underlying file is stereo, ReadChunkMono merges the two streams in a synchronized manner and returns the result */
	virtual int ReadChunkMono(AudioChunkRef& chunk) = 0;

	/** Move the file to a new name including ".orig" suffix */
	void MoveOrig();
	void SetFilename(CStdString&);
	void Delete();
	virtual CStdString GetExtension() = 0;
	virtual int GetSampleRate();
	virtual void SetNumOutputChannels(int numChan) = 0;

protected:
	CStdString m_filename;
	fileOpenModeEnum m_mode;
	int m_numChunksWritten;
	int m_sampleRate;
	int m_numOutputChannels;
};

typedef oreka::shared_ptr<AudioFile> AudioFileRef;

#endif

