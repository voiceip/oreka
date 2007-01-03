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

#include "ace/OS_NS_unistd.h"
#include "Utils.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "ConfigManager.h"
#include "GeneratorConfig.h"

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;
extern OrkLogManager* g_logManager;

GeneratorConfigTopObjectRef g_generatorConfigTopObjectRef;
#define GCONFIG g_generatorConfigTopObjectRef.get()->m_config

void Configure(DOMNode* node)
{
	if (node)
	{
		GeneratorConfigTopObjectRef generatorConfigTopObjectRef(new GeneratorConfigTopObject);
		try
		{
			generatorConfigTopObjectRef.get()->DeSerializeDom(node);
			g_generatorConfigTopObjectRef = generatorConfigTopObjectRef;
		}
		catch (CStdString& e)
		{
			LOG4CXX_WARN(g_logManager->rootLog, "Generator.dll: " + e + " - using defaults");
		}
	}
	else
	{
		LOG4CXX_WARN(g_logManager->rootLog, "Generator.dll: got empty DOM tree");
	}
}

void Initialize()
{
	// create a default config object in case it was not properly initialized by Configure
	if (!g_generatorConfigTopObjectRef.get())
	{
		g_generatorConfigTopObjectRef.reset(new GeneratorConfigTopObject);
	}
}

void Run()
{

#define NUM_SAMPLES_PER_CHUNK 8000

	// Load test file data into memory
	int fileSize = 0;
	int audioBufferSize = 0;
	int numChunks;
	short* audioBuffer = NULL;
	FILE* file = fopen((PCSTR)GCONFIG.m_audioFilename,"rb");
	if (file)
	{
		fseek (file, 0, SEEK_END);
        fileSize = ftell(file);
		fseek (file, 0, SEEK_SET);

		// round up file size to the next NUM_SAMPLES_PER_CHUNK multiple
		numChunks = (fileSize/NUM_SAMPLES_PER_CHUNK) + 1;
		fileSize = numChunks * NUM_SAMPLES_PER_CHUNK;

		audioBuffer = (short *)malloc(sizeof(short)*fileSize);
		audioBufferSize = fileSize/sizeof(short);
		for(int i=0; i<fileSize; i++)
		{
			audioBuffer[i] = 0;
		}
		int numRead = fread(audioBuffer, sizeof(short), fileSize, file);
		fclose(file);
	}
	else
	{
		// can't find test file - have a single zeroed buffer in memory
		LOG4CXX_WARN(g_logManager->rootLog, "Generator.dll: Could not load audio test file:" + GCONFIG.m_audioFilename + " using empty buffer instead");

		numChunks = 1;
		audioBuffer = (short *)malloc(sizeof(short)*NUM_SAMPLES_PER_CHUNK);
		audioBufferSize = NUM_SAMPLES_PER_CHUNK;
		for(int i=0; i<NUM_SAMPLES_PER_CHUNK; i++)
		{
			audioBuffer[i] = 0;
		}
	}

	int elapsed = 0;

	for(;;)
	{

		for(int portId = 0; portId<GCONFIG.m_numConcurrentPorts; portId++)
		{
			CStdString portName;
			portName.Format("port%d", portId);

			if ((elapsed%GCONFIG.m_audioDuration)  == 0)
			{
				// signal call stop and start on all ports
				CaptureEventRef stopEvent(new CaptureEvent);
				stopEvent->m_type = CaptureEvent::EtStop;
				stopEvent->m_timestamp = time(NULL);
				g_captureEventCallBack(stopEvent, portName);

				CaptureEventRef startEvent(new CaptureEvent);
				startEvent->m_type = CaptureEvent::EtStart;
				startEvent->m_timestamp = time(NULL);
				g_captureEventCallBack(startEvent, portName);
			}
			// send audio buffer
			AudioChunkRef chunkRef(new AudioChunk);
			int sampleOffset = (elapsed % numChunks)*NUM_SAMPLES_PER_CHUNK;
			AudioChunkDetails details;
			details.m_encoding = PcmAudio;
			details.m_numBytes = sizeof(short) * NUM_SAMPLES_PER_CHUNK;
			chunkRef->SetBuffer(audioBuffer+sampleOffset, details);
			g_audioChunkCallBack(chunkRef, portName);
		}

		ACE_OS::sleep(1);
		elapsed++;
	}
}


void __CDECL__ StartCapture(CStdString& capturePort)
{
	;
}

void __CDECL__ StopCapture(CStdString& capturePort)
{
	;
}
