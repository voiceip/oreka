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
	m_stream = NULL;
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
	if(m_stream)
	{
		ACE_OS::fclose(m_stream);
		m_stream = NULL;
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

	unsigned int numWritten = 0;
	if (m_stream)
	{
		numWritten = ACE_OS::fwrite(chunkRef->m_pBuffer, sizeof(short), chunkRef->GetNumSamples(), m_stream);
	}
	else
	{
		throw(CStdString("Write attempt on unopened file:")+ m_filename);
	}

	if (numWritten != (unsigned int)chunkRef->GetNumSamples())
	{
		throw(CStdString("Could not write to file:")+ m_filename);
	}
}

int PcmFile::ReadChunkMono(AudioChunkRef& chunkRef)
{
	unsigned int numRead = 0;
	if (m_stream)
	{
		chunkRef.reset(new AudioChunk());
		short temp[PCM_FILE_DEFAULT_CHUNK_NUM_SAMPLES];
		numRead = ACE_OS::fread(temp, sizeof(short), PCM_FILE_DEFAULT_CHUNK_NUM_SAMPLES, m_stream);
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
	m_stream = NULL;
	m_mode = mode;
	if (mode == READ)
	{
		m_stream = ACE_OS::fopen((PCSTR)m_filename, "rb");
	}
	else
	{
		FileRecursiveMkdir(m_filename, CONFIG.m_audioFilePermissions, CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup, CONFIG.m_audioOutputPath);
		m_stream = ACE_OS::fopen((PCSTR)m_filename, "wb");
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
