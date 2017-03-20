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
#pragma warning( disable: 4786 )

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include <vector>
#include <bitset>

#include "ConfigManager.h"
#include "BatchProcessing.h"
#include "ace/OS_NS_unistd.h"
#include "audiofile/LibSndFileFile.h"
#include "Daemon.h"
#include "Filter.h"
#include "Reporting.h"

#ifndef WIN32
#include <pwd.h>
#include <grp.h>
#endif

TapeProcessorRef BatchProcessing::m_singleton;

void BatchProcessing::Initialize()
{
	if(m_singleton.get() == NULL)
	{
		m_singleton.reset(new BatchProcessing());
		TapeProcessorRegistry::instance()->RegisterTapeProcessor(m_singleton);
	}
}


BatchProcessing::BatchProcessing()
{
	m_threadCount = 0;

	struct tm date = {0};
	time_t now = time(NULL);
	ACE_OS::localtime_r(&now, &date);
	m_currentDay = date.tm_mday;
}

CStdString __CDECL__ BatchProcessing::GetName()
{
	return "BatchProcessing";
}

TapeProcessorRef  BatchProcessing::Instanciate()
{
	return m_singleton;
}

void BatchProcessing::AddAudioTape(AudioTapeRef& audioTapeRef)
{
	if (!m_audioTapeQueue.push(audioTapeRef))
	{
		// Log error
		LOG4CXX_ERROR(LOG.batchProcessingLog, CStdString("queue full"));
	}
}

void BatchProcessing::SetQueueSize(int size)
{
	m_audioTapeQueue.setSize(size);
}

bool BatchProcessing::SkipChunk(AudioTapeRef& audioTapeRef, AudioChunkRef& chunkRef, int& channelToSkip)
{
	AudioChunkDetails details = *chunkRef->GetDetails();
	bool skip = false;

	channelToSkip = 0;

	switch(audioTapeRef->m_audioKeepDirectionEnum)
	{
	case CaptureEvent::AudioKeepDirectionBoth:
	{
		skip = false;
		break;
	}
	case CaptureEvent::AudioKeepDirectionLocal:
	{
		switch(audioTapeRef->m_localSide)
		{
		case CaptureEvent::LocalSideUnkn:
		{
			skip = false;
			break;
		}
		case CaptureEvent::LocalSideSide1:
		{
			if(details.m_channel != 1)
			{
				skip = true;
			}
			else
			{
				skip = false;
			}
			break;
		}
		case CaptureEvent::LocalSideSide2:
		{
			if(details.m_channel != 2)
			{
				skip = true;
			}
			else
			{
				skip = false;
			}
			break;
		}
		case CaptureEvent::LocalSideBoth:
		{
			skip = false;
			break;
		}
		default:
		{
			skip = false;
			break;
		}
		}
		break;
	}
	case CaptureEvent::AudioKeepDirectionRemote:
	{
		switch(audioTapeRef->m_localSide)
		{
		case CaptureEvent::LocalSideUnkn:
		{
			skip = false;
			break;
		}
		case CaptureEvent::LocalSideSide1:
		{
			if(details.m_channel == 1)
			{
				skip = true;
			}
			else
			{
				skip = false;
			}
			break;
		}
		case CaptureEvent::LocalSideSide2:
		{
			if(details.m_channel == 2)
			{
				skip = true;
			}
			else
			{
				skip = false;
			}
			break;
		}
		case CaptureEvent::LocalSideBoth:
		{
			skip = true;
			break;
		}
		default:
		{
			skip = false;
			break;
		}
		}

		break;
	}
	case CaptureEvent::AudioKeepDirectionNone:
	{
		skip = true;
		break;
	}
	case CaptureEvent::AudioKeepDirectionInvalid:
	default:
	{
		skip = false;
		break;
	}
	}

	if(skip == true)
	{
		channelToSkip = details.m_channel;
	}

	return skip;
}

void BatchProcessing::ThreadHandler(void *args)
{
	CStdString debug;
	CStdString logMsg;

	CStdString processorName("BatchProcessing");
	TapeProcessorRef batchProcessing = TapeProcessorRegistry::instance()->GetNewTapeProcessor(processorName);
	if(batchProcessing.get() == NULL)
	{
		LOG4CXX_ERROR(LOG.batchProcessingLog, "Could not instanciate BatchProcessing");
		return;
	}
	BatchProcessing* pBatchProcessing = (BatchProcessing*)(batchProcessing->Instanciate().get());

	pBatchProcessing->SetQueueSize(CONFIG.m_batchProcessingQueueSize);

	int threadId = 0;
	{
		MutexSentinel sentinel(pBatchProcessing->m_mutex);
		threadId = pBatchProcessing->m_threadCount++;
	}
	CStdString threadIdString = IntToString(threadId);
	debug.Format("thread Th%s starting - queue size:%d", threadIdString, CONFIG.m_batchProcessingQueueSize);
	LOG4CXX_INFO(LOG.batchProcessingLog, debug);

	bool stop = false;

	for(;stop == false;)
	{
		AudioFileRef fileRef;
		AudioFileRef outFileRef, outFileSecondaryRef;
		AudioTapeRef audioTapeRef;
		CStdString trackingId = "[no-trk]";

		try
		{
			audioTapeRef = pBatchProcessing->m_audioTapeQueue.pop();
			if(audioTapeRef.get() == NULL)
			{
				if(Daemon::Singleton()->IsStopping())
				{
					stop = true;
				}
				if(Daemon::Singleton()->GetShortLived())
				{
					Daemon::Singleton()->Stop();
				}
			}
			else
			{
				fileRef = audioTapeRef->GetAudioFileRef();
				trackingId = audioTapeRef->m_trackingId;

				// Let's work on the tape we have pulled
				//CStdString threadIdString = IntToString(threadId);
				LOG4CXX_INFO(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " processing " + audioTapeRef->GetIdentifier() + " localside:" + CaptureEvent::LocalSideToString(audioTapeRef->m_localSide) + " audiokeepdirection:" + CaptureEvent::AudioKeepDirectionToString(audioTapeRef->m_audioKeepDirectionEnum));
				if(audioTapeRef->m_audioKeepDirectionEnum == CaptureEvent::AudioKeepDirectionInvalid)
				{
					LOG4CXX_WARN(LOG.batchProcessingLog, "[" + trackingId + 
						"] Th" + threadIdString + 
						" invalid audiokeepdirection:" + 
						IntToString(audioTapeRef->m_audioKeepDirectionEnum));
				}



				//fileRef->MoveOrig();	// #### could do this only when original and output file have the same extension. Irrelevant for now as everything is captured as mcf file
				fileRef->Open(AudioFile::READ);

				AudioChunkRef chunkRef;
				AudioChunkRef tmpChunkRef, tmpChunkSecondaryRef;
				unsigned int frameSleepCounter;

				frameSleepCounter = 0;

				switch(CONFIG.m_storageAudioFormat)
				{
				case FfUlaw:
					outFileRef.reset(new LibSndFileFile(SF_FORMAT_ULAW | SF_FORMAT_WAV));
					break;
				case FfAlaw:
					outFileRef.reset(new LibSndFileFile(SF_FORMAT_ALAW | SF_FORMAT_WAV));
					break;
				case FfGsm:
					outFileRef.reset(new LibSndFileFile(SF_FORMAT_GSM610 | SF_FORMAT_WAV));
					break;
				case FfPcmWav:
				default:
					outFileRef.reset(new LibSndFileFile(SF_FORMAT_PCM_16 | SF_FORMAT_WAV));
				}

				if(CONFIG.m_stereoRecording == true)
				{
					outFileRef->SetNumOutputChannels(2);
				}

				FilterRef rtpMixer, rtpMixerSecondary;
				FilterRef decoder1;
				FilterRef decoder2;
				FilterRef decoder;
				FilterRef audiogain;

				std::bitset<RTP_PAYLOAD_TYPE_MAX> seenRtpPayloadTypes;
				std::vector<FilterRef> decoders1;
				std::vector<FilterRef> decoders2;
				for(int pt=0; pt<RTP_PAYLOAD_TYPE_MAX; pt++)
				{
					decoder1 = FilterRegistry::instance()->GetNewFilter(pt);
					decoders1.push_back(decoder1);
					decoder2 = FilterRegistry::instance()->GetNewFilter(pt);
					decoders2.push_back(decoder2);
				}

				bool firstChunk = true;
				bool voIpSession = false;

				size_t numSamplesS1 = 0;
				size_t numSamplesS2 = 0;
				size_t numSamplesOut = 0;

				CStdString filterName("AudioGain");

				audiogain = FilterRegistry::instance()->GetNewFilter(filterName);
				if(audiogain.get() == NULL)
				{
					debug = "Could not instanciate AudioGain rtpMixer";
					throw(debug);
				}

				bool forceChannel1 = false;

				while(fileRef->ReadChunkMono(chunkRef))
				{
					// ############ HACK
					//ACE_Time_Value yield;
					//yield.set(0,1);
					//ACE_OS::sleep(yield);
					// ############ HACK

					AudioChunkDetails details = *chunkRef->GetDetails();
					int channelToSkip = 0;
					if(CONFIG.m_directionLookBack == true)					//if DirectionLookBack is not enable, DirectionSelector Tape should have taken care everything
					{
						if(BatchProcessing::SkipChunk(audioTapeRef, chunkRef, channelToSkip) == true)
						{
							LOG4CXX_DEBUG(LOG.batchProcessingLog, "[" + trackingId +
	                                                "] Th" + threadIdString +
	                                                " skipping chunk of channel:" +
							IntToString(details.m_channel));

							if(forceChannel1 == false)
							{
								if(channelToSkip == 1)
								{
									forceChannel1 = true;
								}
							}

							continue;
						}
					}

					if(forceChannel1 == true)
					{
						details.m_channel = 1;
						chunkRef->SetDetails(&details);
					}

					decoder.reset();

					if(details.m_rtpPayloadType < -1 || details.m_rtpPayloadType >= RTP_PAYLOAD_TYPE_MAX)
					{
						logMsg.Format("RTP payload type out of bound:%d", details.m_rtpPayloadType);
						LOG4CXX_DEBUG(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " " + logMsg);
						continue;
					}

					// Instanciate any decoder we might need during a VoIP session
					if(details.m_rtpPayloadType != -1)
					{
						voIpSession = true;

						if(details.m_channel == 2)
						{
							decoder2 = decoders2.at(details.m_rtpPayloadType);
							decoder = decoder2;
						}
						else
						{
							decoder1 = decoders1.at(details.m_rtpPayloadType);
							decoder = decoder1;
						}

						bool ptAlreadySeen = seenRtpPayloadTypes.test(details.m_rtpPayloadType);
						seenRtpPayloadTypes.set(details.m_rtpPayloadType);

						if(decoder.get() == NULL)
						{
							if(ptAlreadySeen == false)
							{
								// First time we see a particular unsupported payload type in this session, log it
								CStdString rtpPayloadType = IntToString(details.m_rtpPayloadType);
								LOG4CXX_ERROR(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " unsupported RTP payload type:" + rtpPayloadType);
							}
							// We cannot decode this chunk due to unknown codec, go to next chunk
							continue;
						}
						else if(ptAlreadySeen == false)
						{
							// First time we see a particular supported payload type in this session, log it
							CStdString rtpPayloadType = IntToString(details.m_rtpPayloadType);
							LOG4CXX_INFO(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " RTP payload type:" + rtpPayloadType);
						}
					}
					if(!voIpSession || (firstChunk && decoder.get()))
					{
						firstChunk = false;

						// At this point, we know we have a working codec, create an RTP mixer and open the output file
						if(voIpSession)
						{
							CStdString filterName("RtpMixer");
							rtpMixer = FilterRegistry::instance()->GetNewFilter(filterName);
							if(rtpMixer.get() == NULL)
							{
								debug = "Could not instanciate RTP mixer";
								throw(debug);
							}
							if(CONFIG.m_stereoRecording == true)
							{
								rtpMixer->SetNumOutputChannels(2);
							}
							rtpMixer->SetSessionInfo(trackingId);

							//create another rtpmixer to store stereo audio
							if(CONFIG.m_audioOutputPathSecondary.length() > 3)
							{
								outFileSecondaryRef.reset(new LibSndFileFile(SF_FORMAT_PCM_16 | SF_FORMAT_WAV));
								outFileSecondaryRef->SetNumOutputChannels(2);
								rtpMixerSecondary = FilterRegistry::instance()->GetNewFilter(filterName);
								if(rtpMixerSecondary.get() == NULL)
								{
									debug = "Could not instanciate RTP mixer";
									throw(debug);
								}
								rtpMixerSecondary->SetNumOutputChannels(2);
								rtpMixerSecondary->SetSessionInfo(trackingId);

							}

						}

						CStdString path = CONFIG.m_audioOutputPath + "/" + audioTapeRef->GetPath();
						FileRecursiveMkdir(path, CONFIG.m_audioFilePermissions, CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup, CONFIG.m_audioOutputPath);

						CStdString file = path + "/" + audioTapeRef->GetIdentifier();
						outFileRef->Open(file, AudioFile::WRITE, false, fileRef->GetSampleRate());

						if(CONFIG.m_audioOutputPathSecondary.length() > 3)
						{
							path = CONFIG.m_audioOutputPathSecondary + "/" + audioTapeRef->GetPath();
							FileRecursiveMkdir(path, CONFIG.m_audioFilePermissions, CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup, CONFIG.m_audioOutputPathSecondary);
							CStdString storageFile = path + "/" + audioTapeRef->GetIdentifier();
							outFileSecondaryRef->Open(storageFile, AudioFile::WRITE, false, fileRef->GetSampleRate());
						}

					}
					if(voIpSession)
					{
						if(details.m_channel == 2)
						{
							decoder2->AudioChunkIn(chunkRef);
							decoder2->AudioChunkOut(tmpChunkRef);
							if(tmpChunkRef.get())
							{
								numSamplesS2 += tmpChunkRef->GetNumSamples();
							}

							if(rtpMixerSecondary.get() != NULL)
							{
								decoder2->AudioChunkOut(tmpChunkSecondaryRef);
							}
						}
						else
						{
							decoder1->AudioChunkIn(chunkRef);
							decoder1->AudioChunkOut(tmpChunkRef);
							if(tmpChunkRef.get())
							{
								numSamplesS1 += tmpChunkRef->GetNumSamples();
							}

							if(rtpMixerSecondary.get() != NULL)
							{
								decoder1->AudioChunkOut(tmpChunkSecondaryRef);
							}
						}

						audiogain->AudioChunkIn(tmpChunkRef);
						audiogain->AudioChunkOut(tmpChunkRef);
						rtpMixer->AudioChunkIn(tmpChunkRef);
						rtpMixer->AudioChunkOut(tmpChunkRef);
						if(rtpMixerSecondary.get() != NULL)
						{
							rtpMixerSecondary->AudioChunkIn(tmpChunkSecondaryRef);
							rtpMixerSecondary->AudioChunkOut(tmpChunkSecondaryRef);
						}

					} else {
						audiogain->AudioChunkIn(tmpChunkRef);
						audiogain->AudioChunkOut(tmpChunkRef);
					}

					outFileRef->WriteChunk(tmpChunkRef);
					if(rtpMixerSecondary.get() != NULL)
					{
						outFileSecondaryRef->WriteChunk(tmpChunkSecondaryRef);
					}

					if(tmpChunkRef.get())
					{
						numSamplesOut += tmpChunkRef->GetNumSamples();
					}

					if(CONFIG.m_batchProcessingEnhancePriority == false)
					{
						// Give up CPU between every audio buffer to make sure the actual recording always has priority
						//ACE_Time_Value yield;
						//yield.set(0,1);	// 1 us
						//ACE_OS::sleep(yield);

						// Use this instead, even if it still seems this holds the whole process under Linux instead of this thread only.
						struct timespec ts;
						ts.tv_sec = 0;
						ts.tv_nsec = 1;
						ACE_OS::nanosleep (&ts, NULL);
					}
					
					if(CONFIG.m_transcodingSleepEveryNumFrames > 0 && CONFIG.m_transcodingSleepUs > 0)
					{
						if(frameSleepCounter >= (unsigned int)CONFIG.m_transcodingSleepEveryNumFrames)
						{
							frameSleepCounter = 0;
							struct timespec ts;
							ts.tv_sec = 0;
							ts.tv_nsec = CONFIG.m_transcodingSleepUs*1000;
							ACE_OS::nanosleep (&ts, NULL);
						}
						else
						{
							frameSleepCounter += 1;
						}
					}
				}

				if(voIpSession && !firstChunk)
				{
					// Flush the RTP mixer
					AudioChunkRef stopChunk(new AudioChunk());
					stopChunk->GetDetails()->m_marker = MEDIA_CHUNK_EOS_MARKER;
					rtpMixer->AudioChunkIn(stopChunk);
					rtpMixer->AudioChunkOut(tmpChunkRef);
					if(rtpMixerSecondary.get() != NULL)
					{
						rtpMixerSecondary->AudioChunkOut(tmpChunkSecondaryRef);
					}

					while(tmpChunkRef.get())
					{
						outFileRef->WriteChunk(tmpChunkRef);
						numSamplesOut += tmpChunkRef->GetNumSamples();
						rtpMixer->AudioChunkOut(tmpChunkRef);
					}
					while(tmpChunkSecondaryRef.get())
					{
						outFileSecondaryRef->WriteChunk(tmpChunkSecondaryRef);
						rtpMixerSecondary->AudioChunkOut(tmpChunkSecondaryRef);
					}
				}

				fileRef->Close();
				outFileRef->Close();
				if(rtpMixerSecondary.get() != NULL)
				{
					outFileSecondaryRef->Close();
				}
				logMsg.Format("[%s] Th%s stop: num samples: s1:%u s2:%u out:%u queueSize:%d", trackingId, threadIdString, numSamplesS1, numSamplesS2, numSamplesOut, pBatchProcessing->m_audioTapeQueue.numElements());
				LOG4CXX_INFO(LOG.batchProcessingLog, logMsg);

				CStdString audioFilePath = CONFIG.m_audioOutputPath + "/" + audioTapeRef->GetPath();
				CStdString audioFileName;

				audioFileName = audioFilePath + "/" + audioTapeRef->GetIdentifier() + outFileRef->GetExtension();
				if(CONFIG.m_audioFilePermissions) {
					if(FileSetPermissions(audioFileName, CONFIG.m_audioFilePermissions))
					{
						CStdString logMsg;

						logMsg.Format("Error setting permissions of %s to %o: %s", audioFileName.c_str(), CONFIG.m_audioFilePermissions, strerror(errno));
						LOG4CXX_ERROR(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " " + logMsg);
					}
				}

				if(CONFIG.m_audioFileGroup.size() && CONFIG.m_audioFileOwner.size()) {
					if(FileSetOwnership(audioFileName, CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup))
					{
						logMsg.Format("Error setting ownership and group of %s to %s:%s: %s", audioFileName.c_str(), CONFIG.m_audioFileOwner, CONFIG.m_audioFileGroup, strerror(errno));
						LOG4CXX_ERROR(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " " + logMsg);
					}
				}
				
				if(CONFIG.m_deleteNativeFile && numSamplesOut)
				{
					fileRef->Delete();
					LOG4CXX_INFO(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " deleting native: " + audioTapeRef->GetIdentifier());
				}
				else if(CONFIG.m_deleteFailedCaptureFile)
				{
					fileRef->Delete();
					if(outFileRef.get()) 
					{
						outFileRef->Close();
						outFileRef->Delete();
					}
					LOG4CXX_INFO(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " deleting native that could not be transcoded: " + audioTapeRef->GetIdentifier());
				}

				// Finished processing the tape, pass on to next processor
				if(numSamplesOut)
				{
					pBatchProcessing->RunNextProcessor(audioTapeRef);
				}
			}
		}
		catch (CStdString& e)
		{
			LOG4CXX_ERROR(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " " + e);
			if(fileRef.get()) {fileRef->Close();}
			if(outFileRef.get()) {outFileRef->Close();}
			if(CONFIG.m_deleteFailedCaptureFile && fileRef.get() != NULL)
			{
				LOG4CXX_INFO(LOG.batchProcessingLog, "[" + trackingId + "] Th" + threadIdString + " deleting native and transcoded");
				if(fileRef.get()) {fileRef->Delete();}
				if(outFileRef.get()) {outFileRef->Delete();}
			}
		}
		//catch(...)
		//{
		//	LOG4CXX_ERROR(LOG.batchProcessingLog, CStdString("unknown exception"));
		//}
	}
	LOG4CXX_INFO(LOG.batchProcessingLog, CStdString("Exiting thread Th" + threadIdString));
}



