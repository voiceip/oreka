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

#ifndef __SOUNDDEVICECONFIG_H__
#define __SOUNDDEVICECONFIG_H__

#include "StdString.h"
#include "Object.h"
#include "shared_ptr.h"
 
#define AUDIO_CHUNK_SIZE_PARAM "AudioChunkSize"
#define AUDIO_CHUNK_SIZE_DEFAULT 8000
#define SAMPLE_RATE_PARAM "SampleRate"
#define SAMPLE_RATE_DEFAULT 8000

/** This class defines various configuration parameters for the generator. */
class SoundDeviceConfig : public Object
{
public:
	SoundDeviceConfig();	
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	int m_audioChunkSize;
	int m_sampleRate;
};

//========================================

#define SOUND_DEVICE_CONFIG_PARAM "SoundDevicePlugin"

/** This class represents the top of the configuration hierarchy for the generator. */
class SoundDeviceConfigTopObject : public Object
{
public:
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};
	
	SoundDeviceConfig m_config;
};

typedef boost::shared_ptr<SoundDeviceConfigTopObject> SoundDeviceConfigTopObjectRef;


#endif