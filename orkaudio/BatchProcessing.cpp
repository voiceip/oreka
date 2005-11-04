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

#include "ConfigManager.h"
#include "BatchProcessing.h"
#include "LogManager.h"
#include "ace/OS_NS_unistd.h"
#include "audiofile/LibSndFileFile.h"

BatchProcessing BatchProcessing::m_batchProcessingSingleton;

BatchProcessing::BatchProcessing()
{
	m_threadCount = 0;
}


BatchProcessing* BatchProcessing::GetInstance()
{
	return &m_batchProcessingSingleton;
}

void BatchProcessing::AddAudioTape(AudioTapeRef audioTapeRef)
{
	if (!m_audioTapeQueue.push(audioTapeRef))
	{
		// Log error
		LOG4CXX_ERROR(LOG.batchProcessingLog, CStdString("BatchProcessing: queue full"));
	}
}

void BatchProcessing::ThreadHandler(void *args)
{
	BatchProcessing* pBatchProcessing = BatchProcessing::GetInstance();
	int threadId = 0;
	{
		MutexSentinel sentinel(pBatchProcessing->m_mutex);
		threadId = pBatchProcessing->m_threadCount++;
	}
	CStdString threadIdString = IntToString(threadId);
	LOG4CXX_DEBUG(LOG.batchProcessingLog, CStdString("Created thread #") + threadIdString);

	for(;;)
	{
		try
		{
			AudioTapeRef audioTapeRef = pBatchProcessing->m_audioTapeQueue.pop();
			CStdString threadIdString = IntToString(threadId);
			LOG4CXX_INFO(LOG.batchProcessingLog, CStdString("Th") + threadIdString + " processing: " + audioTapeRef->GetIdentifier());

			AudioFileRef fileRef = audioTapeRef->GetAudioFileRef();
			fileRef->MoveOrig();
			fileRef->Open(AudioFile::READ);

			AudioChunkRef chunkRef;
			AudioFileRef outFileRef;

			switch(CONFIG.m_storageAudioFormat)
			{
			case AudioTape::FfUlaw:
				outFileRef.reset(new LibSndFileFile(SF_FORMAT_ULAW | SF_FORMAT_WAV));
				break;
			case AudioTape::FfAlaw:
				outFileRef.reset(new LibSndFileFile(SF_FORMAT_ALAW | SF_FORMAT_WAV));
				break;
			case AudioTape::FfGsm:
			default:
				outFileRef.reset(new LibSndFileFile(SF_FORMAT_GSM610 | SF_FORMAT_WAV));
			}
			CStdString file = CONFIG.m_audioOutputPath + "/" + audioTapeRef->GetPath() + audioTapeRef->GetIdentifier();
			outFileRef->Open(file, AudioFile::WRITE);

			while(fileRef->ReadChunkMono(chunkRef))
			{
				outFileRef->WriteChunk(chunkRef);
			}

			if(CONFIG.m_deleteNativeFile)
			{
				fileRef->Close();
				fileRef->Delete();
				CStdString threadIdString = IntToString(threadId);
				LOG4CXX_INFO(LOG.batchProcessingLog, CStdString("Th") + threadIdString + " deleting native: " + audioTapeRef->GetIdentifier());
			}
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.batchProcessingLog, CStdString("BatchProcessing: ") + e);
		}
		catch(...)
		{
		}

	}
}


