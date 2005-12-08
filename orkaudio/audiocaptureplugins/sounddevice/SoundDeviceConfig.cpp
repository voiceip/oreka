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
#include "Serializers/Serializer.h"
#include "SoundDeviceConfig.h"

SoundDeviceConfig::SoundDeviceConfig()
{
	m_audioChunkSize = AUDIO_CHUNK_SIZE_DEFAULT;
	m_sampleRate = SAMPLE_RATE_DEFAULT;
}

void SoundDeviceConfig::Define(Serializer* s)
{
	s->IntValue(AUDIO_CHUNK_SIZE_PARAM, m_audioChunkSize);
	s->IntValue(SAMPLE_RATE_PARAM, m_sampleRate);
}

void SoundDeviceConfig::Validate()
{
	if(m_audioChunkSize > 80000 || m_audioChunkSize<100)
	{
		CStdString audioChunkSizeString = IntToString(m_audioChunkSize);
		throw(CStdString("SoundDeviceConfig: ") + AUDIO_CHUNK_SIZE_PARAM + "=" + audioChunkSizeString + " this is out of range");
	}
}

CStdString SoundDeviceConfig::GetClassName()
{
	return CStdString("SoundDeviceConfig");
}

ObjectRef SoundDeviceConfig::NewInstance()
{
	return ObjectRef(new SoundDeviceConfig);
}

//====================================


void SoundDeviceConfigTopObject::Define(Serializer* s)
{
	s->ObjectValue(SOUND_DEVICE_CONFIG_PARAM, m_config, true);
}

void SoundDeviceConfigTopObject::Validate()
{
	;
}

CStdString SoundDeviceConfigTopObject::GetClassName()
{
	return CStdString("SoundDeviceConfigTopObject");
}

ObjectRef SoundDeviceConfigTopObject::NewInstance()
{
	return ObjectRef(new SoundDeviceConfigTopObject);
}