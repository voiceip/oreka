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
#include "ace/Singleton.h"
#include "ace/Min_Max.h"
#include "AudioCapturePlugin.h"
#include "AudioCapturePluginCommon.h"
#include "portaudio.h"
#include "Utils.h"
#include "SoundDeviceConfig.h"

extern AudioChunkCallBackFunction g_audioChunkCallBack;
extern CaptureEventCallBackFunction g_captureEventCallBack;
extern OrkLogManager* g_logManager;

SoundDeviceConfigTopObjectRef g_soundDeviceConfigTopObjectRef;
#define DLLCONFIG g_soundDeviceConfigTopObjectRef.get()->m_config

static LoggerPtr s_soundDeviceLog;
static LoggerPtr s_soundDeviceBufferLog;


typedef struct 
{
	PaDeviceID deviceID;
	int channelCount;
	PortAudioStream* stream;
} DeviceUserData;

int portAudioCallBack(void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, PaTimestamp outTime, void *userData)
{
	DeviceUserData *device = (DeviceUserData *)userData;
	short * inputSamples = (short *)inputBuffer;
	CStdString portName;

	if(s_soundDeviceBufferLog->isDebugEnabled())
	{
		CStdString debug;
		debug.Format("Dev:%u NumSamples:%u Time:%f", device->deviceID, framesPerBuffer, outTime);
		LOG4CXX_DEBUG(s_soundDeviceBufferLog, debug);
	}

	if (device->channelCount == 2)
	{
		// stereo -> split into two different chunks
		short* rightBuffer = new short[DLLCONFIG.m_audioChunkSize];
		short* leftBuffer = new short[DLLCONFIG.m_audioChunkSize];

		for (int sampleId=0; sampleId<DLLCONFIG.m_audioChunkSize ; sampleId++)
		{
			rightBuffer[sampleId] = inputSamples[2*sampleId];
			leftBuffer[sampleId] = inputSamples[2*sampleId+1];
		}
		AudioChunkRef chunkRef(new AudioChunk);
		AudioChunkDetails details;
		details.m_encoding = PcmAudio;
		details.m_sampleRate = DLLCONFIG.m_sampleRate;
		details.m_channel = 1;
		details.m_numBytes = sizeof(short)*framesPerBuffer;
		chunkRef->SetBuffer(rightBuffer, details);
		portName.Format("port%d-%d", device->deviceID, 1);
		g_audioChunkCallBack(chunkRef, portName);

		details.m_channel = 2;
		chunkRef.reset(new AudioChunk);
		chunkRef->SetBuffer(leftBuffer, details);
		portName.Format("port%d-%d", device->deviceID, 2);
		g_audioChunkCallBack(chunkRef, portName);

		delete rightBuffer;
		delete leftBuffer;
	}
	else
	{
		// mono
		AudioChunkRef chunkRef(new AudioChunk);
		AudioChunkDetails details;
		details.m_encoding = PcmAudio;
		details.m_sampleRate = DLLCONFIG.m_sampleRate;
		details.m_channel = 1;
		details.m_numBytes = sizeof(short)*framesPerBuffer;
		chunkRef->SetBuffer(inputSamples, details);
		portName.Format("port%d", device->deviceID);
		g_audioChunkCallBack(chunkRef, portName);
	}

	return 0;
}


class SoundDevice
{
public:
	SoundDevice();
	void Initialize();
	void Run();
	void StartCapture(CStdString& port);
	void StopCapture(CStdString& port);
private:
	DeviceUserData** m_devices;
	int m_deviceCount;
	PaStream* m_stream;
};

typedef ACE_Singleton<SoundDevice, ACE_Thread_Mutex> SoundDeviceSingleton;

SoundDevice::SoundDevice()
{
	m_deviceCount = 0;
	m_devices = NULL;
}

void Configure(DOMNode* node)
{
	if (node)
	{
		SoundDeviceConfigTopObjectRef soundDeviceConfigTopObjectRef(new SoundDeviceConfigTopObject);
		try
		{
			soundDeviceConfigTopObjectRef.get()->DeSerializeDom(node);
			g_soundDeviceConfigTopObjectRef = soundDeviceConfigTopObjectRef;
		}
		catch (CStdString& e)
		{
			LOG4CXX_WARN(g_logManager->rootLog, "SoundDevice.dll: " + e + " - using defaults");
		}
	}
	else
	{
		LOG4CXX_WARN(g_logManager->rootLog, "SoundDevice.dll: got empty DOM tree");
	}
}


void SoundDevice::Initialize()
{
	LOG4CXX_INFO(g_logManager->rootLog, "Initializing Sound Device plugin");

	s_soundDeviceLog = Logger::getLogger("sounddevice");
	s_soundDeviceBufferLog = Logger::getLogger("sounddevice.buffer");

	// create a default config object in case it was not properly initialized by Configure
	if(!g_soundDeviceConfigTopObjectRef.get())
	{
		g_soundDeviceConfigTopObjectRef.reset(new SoundDeviceConfigTopObject);
	}


	PaError result = Pa_Initialize();
	if (result)
	{
		LOG4CXX_ERROR(g_logManager->rootLog, "Could not initialize Sound Device plugin");
	}

	m_deviceCount = Pa_CountDevices();
	m_devices = new DeviceUserData*[m_deviceCount];

	for( PaDeviceID deviceID=0; deviceID<m_deviceCount; deviceID++ )
	{
		const PaDeviceInfo* deviceInfo = Pa_GetDeviceInfo( deviceID );

		m_devices[deviceID] = new DeviceUserData;
		m_devices[deviceID]->deviceID = deviceID;
		m_devices[deviceID]->channelCount = ace_max(deviceInfo->maxInputChannels,2);
		m_devices[deviceID]->stream = NULL;
		CStdString deviceName(deviceInfo->name);

		if (deviceInfo->maxInputChannels > 0 && deviceName.Find("Primary") == -1)	// Primary audio device is a duplicate of one of the devices
		{
			CStdString capturePorts;
			if (deviceInfo->maxInputChannels > 1)
			{
				capturePorts.Format("port%d-1, port%d-2", deviceID, deviceID);	// stereo
			}
			else
			{
				capturePorts.Format("port%d-1", deviceID);	// mono
			}

			CStdString maxInputChannelsString = IntToString(deviceInfo->maxInputChannels);
			LOG4CXX_INFO(g_logManager->rootLog, "Ports:" + capturePorts + " Name:" + deviceName + " Channels:" + maxInputChannelsString);

			result =  Pa_OpenStream(	&m_devices[deviceID]->stream,
										deviceID,
										m_devices[deviceID]->channelCount,
										paInt16,
										NULL,
										paNoDevice ,
										0,
										paInt16,
										NULL,
										(double)DLLCONFIG.m_sampleRate,
										DLLCONFIG.m_audioChunkSize,
										0,
										0,
										portAudioCallBack,
										(void*)m_devices[deviceID] );

			if (result)
			{
				CStdString deviceIdString = IntToString(deviceID);
				LOG4CXX_ERROR(g_logManager->rootLog, "Device:" + deviceIdString + CStdString(" Pa_OpenStream error:") + Pa_GetErrorText(result));
			}
		}
	}
}

void SoundDevice::Run()
{
	for( PaDeviceID deviceID=0; deviceID<m_deviceCount; deviceID++ )
	{
		if (m_devices[deviceID]->channelCount > 0 && m_devices[deviceID]->stream)
		{
			PaError result = Pa_StartStream(m_devices[deviceID]->stream);
			if (result)
			{
				CStdString deviceIdString = IntToString(deviceID);
				LOG4CXX_ERROR(g_logManager->rootLog, "Device:" + deviceIdString + CStdString(" Pa_StartStream error:") + Pa_GetErrorText(result));
			}
		}
	}
}

void SoundDevice::StartCapture(CStdString& port)
{
	CaptureEventRef startEvent(new CaptureEvent);
	startEvent->m_type = CaptureEvent::EtStart;
	startEvent->m_timestamp = time(NULL);
	g_captureEventCallBack(startEvent, port);
}

void SoundDevice::StopCapture(CStdString& port)
{
	CaptureEventRef stopEvent(new CaptureEvent);
	stopEvent->m_type = CaptureEvent::EtStop;
	stopEvent->m_timestamp = time(NULL);
	g_captureEventCallBack(stopEvent, port);
}

void __CDECL__ Initialize()
{
	SoundDeviceSingleton::instance()->Initialize();
}

void __CDECL__ Run()
{
	SoundDeviceSingleton::instance()->Run();
	for(;;)
	{
		// if this idle thread is missing, it will crash winmm
		ACE_OS::sleep(5);
	}
}

void __CDECL__ Shutdown()
{
	;
}

void __CDECL__ PauseCapture(CStdString& capturePort, CStdString& orkuid, CStdString& nativecallid)
{
	;
}

void __CDECL__ SetOnHold(CStdString& port, CStdString& orkuid)
{
	;
}

void __CDECL__ SetOffHold(CStdString& port, CStdString& orkuid)
{
	;
}

void __CDECL__ StartCapture(CStdString& capturePort, CStdString& orkuid, CStdString& nativecallid)
{
	SoundDeviceSingleton::instance()->StartCapture(capturePort);
}

void __CDECL__ StopCapture(CStdString& capturePort, CStdString& orkuid, CStdString& nativecallid)
{
	SoundDeviceSingleton::instance()->StopCapture(capturePort);
}