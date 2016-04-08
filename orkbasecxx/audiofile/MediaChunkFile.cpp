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
#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning

#include "ConfigManager.h"
#include "MediaChunkFile.h"
#include "Utils.h"

#define MAX_CHUNK_SIZE 100000


MediaChunkFile::MediaChunkFile()
{
	m_stream = NULL;
	m_mode = READ;
	m_numChunksWritten = 0;
	m_sampleRate = 0;

	m_chunkQueueDataSize = 0;
}

MediaChunkFile::~MediaChunkFile()
{
	Close();
}


void MediaChunkFile::Close()
{
	if(m_stream)
	{
		FlushToDisk();
		ACE_OS::fclose(m_stream);
		m_stream = NULL;
	}
}

bool MediaChunkFile::FlushToDisk()
{
	bool writeError = false;
	while(m_chunkQueue.size() > 0)
	{
		AudioChunkRef tmpChunk = m_chunkQueue.front();
		m_chunkQueue.pop();
		if(tmpChunk.get() == NULL)
		{
			continue;
		}
		if(tmpChunk->m_pBuffer == NULL)
		{
			continue;
		}

		int numWritten = ACE_OS::fwrite(tmpChunk->GetDetails(), sizeof(AudioChunkDetails), 1, m_stream);
		if(numWritten != 1)
		{
			writeError = true;
			break;
		}
		numWritten = ACE_OS::fwrite(tmpChunk->m_pBuffer, sizeof(char), tmpChunk->GetNumBytes(), m_stream);
		if(numWritten != tmpChunk->GetNumBytes())
		{
			writeError = true;
			break;
		}
	}
	m_chunkQueueDataSize = 0;
	return writeError;
}


void MediaChunkFile::WriteChunk(AudioChunkRef chunkRef)
{
	if(chunkRef.get() == NULL)
	{
		return;
	}
	if(chunkRef->GetDetails()->m_numBytes == 0)
	{
		return;
	}

	bool writeError = false;

	AudioChunk* pChunk = chunkRef.get();
	m_chunkQueueDataSize += pChunk->GetNumBytes();
	m_chunkQueue.push(chunkRef);

	if(m_chunkQueueDataSize > (unsigned int)(CONFIG.m_captureFileBatchSizeKByte*1024))
	{
		if (m_stream)
		{
			writeError = FlushToDisk();
		}
		else
		{
			throw(CStdString("Write attempt on unopened file:")+ m_filename);
		}
	}
	if (writeError)
	{
		throw(CStdString("Could not write to file:")+ m_filename);
	}
}

int MediaChunkFile::ReadChunkMono(AudioChunkRef& chunkRef)
{
	unsigned int numRead = 0;

	if (m_stream)
	{
		chunkRef.reset(new AudioChunk());
		short temp[MAX_CHUNK_SIZE];
		numRead = ACE_OS::fread(temp, sizeof(AudioChunkDetails), 1, m_stream);
		if(numRead == 1)
		{
			AudioChunkDetails details;
			memcpy(&details, temp, sizeof(AudioChunkDetails));

			if(details.m_marker != MEDIA_CHUNK_MARKER)
			{
				throw(CStdString("Invalid marker in file:")+ m_filename);
			}
			if(details.m_numBytes >= MAX_CHUNK_SIZE)
			{
				throw(CStdString("Chunk too big in file:")+ m_filename);
			}
			else
			{
				numRead = ACE_OS::fread(temp, sizeof(char), details.m_numBytes, m_stream);
				if(numRead != details.m_numBytes)
				{
					throw(CStdString("Incomplete chunk in file:")+ m_filename);
				}
				chunkRef->SetBuffer(temp, details);
			}
		}
	}
	else
	{
		throw(CStdString("Read attempt on unopened file:")+ m_filename);
	}
	
	return numRead;
}


void MediaChunkFile::Open(CStdString& filename, fileOpenModeEnum mode, bool stereo, int sampleRate)
{
	if(m_sampleRate == 0)
	{
		m_sampleRate = sampleRate;
	}

	if(!m_filename.Equals(filename))
	{
		m_filename = filename + GetExtension();
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

		if(CONFIG.m_audioFilePermissions)
		{
			FileSetPermissions(m_filename, CONFIG.m_audioFilePermissions);
		}

		if(CONFIG.m_audioFileGroup.size() && CONFIG.m_audioFileOwner.size())
		{
			FileSetOwnership(m_filename, CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup);
		}
	}
	if(!m_stream)
	{
		throw(CStdString("Could not open file: ") + m_filename);
	}
}

CStdString MediaChunkFile::GetExtension()
{
	return ".mcf";
}

void MediaChunkFile::SetNumOutputChannels(int numChan)
{

}

