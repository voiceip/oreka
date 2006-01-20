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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "Utils.h"
#include "LibSndFileFile.h"

LibSndFileFile::LibSndFileFile(int fileFormat)
{
    m_fileInfo.format = fileFormat;
	m_fileInfo.frames = 0;
    m_fileInfo.samplerate = 0;
    m_fileInfo.channels = 0;
    m_fileInfo.sections = 0;
    m_fileInfo.seekable = 0; 
	m_pFile = NULL;
	m_numChunksWritten = 0;
	m_mode = READ;
	m_sampleRate = 0;
}

LibSndFileFile::~LibSndFileFile()
{
	Close();
}

void LibSndFileFile::Open(CStdString& filename, fileOpenModeEnum mode, bool stereo, int sampleRate)
{
	if(!m_filename.Equals(filename))
	{
		m_filename = filename + ".wav";
	}
	m_mode = mode;
	stereo ? m_fileInfo.channels = 2 : m_fileInfo.channels = 1;
	if(m_sampleRate == 0)
	{
		m_sampleRate = sampleRate;
		m_fileInfo.samplerate = sampleRate;
	}

	if( (mode==WRITE) && !sf_format_check(&m_fileInfo))
	{
		throw(CStdString("libsndfile: Selected output format not supported"));
	}

	RecursiveMkdir(m_filename);

	int sndFileMode;
	mode == READ ? sndFileMode = SFM_READ : sndFileMode = SFM_WRITE;
	m_pFile = sf_open((PCSTR)m_filename, sndFileMode, &m_fileInfo);

	if(!m_pFile)
	{
		throw(CStdString("sf_open failed, audio file could not be created:"+ m_filename));
	}
}

void LibSndFileFile::WriteChunk(AudioChunkRef chunkRef)
{
	if(chunkRef.get() == NULL)
	{
		return;
	}
	if(chunkRef->GetDetails()->m_numBytes == 0)
	{
		return;
	}

	if (m_pFile)
	{
		if( chunkRef->GetEncoding() == AlawAudio ||  chunkRef->GetEncoding() == UlawAudio)
		{
			if(sf_write_raw(m_pFile, chunkRef->m_pBuffer, chunkRef->GetNumSamples()) != chunkRef->GetNumSamples())
			{
				CStdString numChunksWrittenString = IntToString(m_numChunksWritten);
				throw(CStdString("sf_write_raw failed, audio file " + m_filename + " could not be written after " + numChunksWrittenString + " chunks written"));
			}
		}
		else if (chunkRef->GetEncoding() == PcmAudio)
		{
			if(sf_write_short(m_pFile, (short*)chunkRef->m_pBuffer, chunkRef->GetNumSamples()) != chunkRef->GetNumSamples())
			{
				CStdString numChunksWrittenString = IntToString(m_numChunksWritten);
				throw(CStdString("sf_write_short failed, audio file " + m_filename + " could not be written after " + numChunksWrittenString + " chunks written"));
			}
		}
		m_numChunksWritten++;
	}
	else
	{
		throw(CStdString("Write attempt on unopened file:")+ m_filename);
	}
}

int LibSndFileFile::ReadChunkMono(AudioChunkRef& chunk)
{
	unsigned int numRead = 0;
	if (m_pFile)
	{
		chunk.reset(new AudioChunk());
		short temp[8000];
		numRead = sf_read_short(m_pFile, temp, 8000);
		AudioChunkDetails details;
		details.m_encoding = PcmAudio;
		chunk->SetBuffer(temp, sizeof(short)*numRead, details);
	}
	else
	{
		throw(CStdString("Read attempt on unopened file:")+ m_filename);
	}
	return numRead;
}


void LibSndFileFile::Close()
{
	if (m_pFile)
	{
		sf_close(m_pFile);
		m_pFile = NULL;
	}
}

CStdString LibSndFileFile::GetExtension()
{
	return ".wav";
}
