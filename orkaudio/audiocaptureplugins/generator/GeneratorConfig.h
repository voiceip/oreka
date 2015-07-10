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

#ifndef __GENERATORCONFIG_H__
#define __GENERATORCONFIG_H__

#include "StdString.h"
#include "Object.h"
#include "shared_ptr.h"

#define NUM_CONCURRENT_PORTS_PARAM "NumConcurrentPorts"
#define NUM_CONCURRENT_PORTS_DEFAULT 10 
#define AUDIO_DURATION_PARAM "AudioDuration"
#define AUDIO_DURATION_DEFAULT 10
#define AUDIO_FILE_NAME_PARAM "AudioFilename"
#define AUDIO_FILE_NAME_DEFAULT "test.wav"

/** This class defines various configuration parameters for the generator. */
class GeneratorConfig : public Object
{
public:
	GeneratorConfig();	
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};

	int m_numConcurrentPorts;
	int m_audioDuration;
	CStdString m_audioFilename;
};

//========================================

#define GENERATOR_CONFIG_PARAM "GeneratorPlugin"

/** This class represents the top of the configuration hierarchy for the generator. */
class GeneratorConfigTopObject : public Object
{
public:
	void Define(Serializer* s);
	void Validate();

	CStdString GetClassName();
	ObjectRef NewInstance();
	inline ObjectRef Process() {return ObjectRef();};
	
	GeneratorConfig m_config;
};

typedef oreka::shared_ptr<GeneratorConfigTopObject> GeneratorConfigTopObjectRef;


#endif
