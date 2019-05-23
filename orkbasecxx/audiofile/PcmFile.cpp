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

#include "Utils.h"
#include "PcmFile.h"
#include "ConfigManager.h"

PcmFile::PcmFile()
{
	m_mode = READ;
	m_numChunksWritten = 0;
	m_sampleRate = 0;
}

PcmFile::~PcmFile()
{
	Close();
}


void PcmFile::Close()
{
	if(m_stream.is_open())
	{
		m_stream.close();
	}
}

void PcmFile::WriteChunk(AudioChunkRef chunkRef)
{
	if(chunkRef.get() == NULL)
	{
		return;
	}
	if(chunkRef->GetDetails()->m_numBytes == 0)
	{
		return;
	}

	int numWritten = 0;
	if(m_stream.is_open())
	{
		numWritten = chunkRef->GetNumSamples()*sizeof(short);
		m_stream.write((char*)chunkRef->m_pBuffer, numWritten);
	}
	else
	{
		throw(CStdString("Write attempt on unopened file:")+ m_filename);
	}

	if(m_stream.fail())
	{
		throw(CStdString("Could not write to file:")+ m_filename);
	}
}

int PcmFile::ReadChunkMono(AudioChunkRef& chunkRef)
{
	int numRead = 0;
	if(m_stream.is_open())
	{
		chunkRef.reset(new AudioChunk());
		short temp[PCM_FILE_DEFAULT_CHUNK_NUM_SAMPLES];
		numRead = PCM_FILE_DEFAULT_CHUNK_NUM_SAMPLES*sizeof(short);
		m_stream.read((char*)temp, numRead);
		if(m_stream.eof()){
			return 0;
		}
		AudioChunkDetails details;
		details.m_encoding = PcmAudio;
		details.m_numBytes = sizeof(short)*numRead;
		chunkRef->SetBuffer(temp, details);
	}
	else
	{
		throw(CStdString("Read attempt on unopened file:")+ m_filename);
	}
	return numRead;
}


void PcmFile::Open(CStdString& filename, fileOpenModeEnum mode, bool stereo, int sampleRate)
{
	if(m_sampleRate == 0)
	{
		m_sampleRate = sampleRate;
	}

	if(!m_filename.Equals(filename))
	{
		m_filename = filename + ".pcm";
	}

	m_mode = mode;
	if (mode == READ)
	{
		m_stream.open(m_filename, std::fstream::in | std::fstream::binary);
	}
	else
	{
		FileRecursiveMkdir(m_filename, CONFIG.m_audioFilePermissions, CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup, CONFIG.m_audioOutputPath);
		m_stream.open(m_filename, std::fstream::out | std::fstream::binary);
	}
	if(!m_stream)
	{
		throw(CStdString("Could not open file: ") + m_filename);
	}
}

CStdString PcmFile::GetExtension()
{
	return ".pcm";
}
