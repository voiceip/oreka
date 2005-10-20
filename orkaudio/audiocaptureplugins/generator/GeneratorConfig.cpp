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

#include "serializers/Serializer.h"
#include "GeneratorConfig.h"

GeneratorConfig::GeneratorConfig()
{
	m_numConcurrentPorts = NUM_CONCURRENT_PORTS_DEFAULT;
	m_audioDuration = AUDIO_DURATION_DEFAULT;
	m_audioFilename = AUDIO_FILE_NAME_DEFAULT;
}

void GeneratorConfig::Define(Serializer* s)
{
	s->IntValue(NUM_CONCURRENT_PORTS_PARAM, m_numConcurrentPorts);
	s->IntValue(AUDIO_DURATION_PARAM, m_audioDuration);
	s->StringValue(AUDIO_FILE_NAME_PARAM, m_audioFilename);
}

void GeneratorConfig::Validate()
{
	;
}

CStdString GeneratorConfig::GetClassName()
{
	return CStdString("GeneratorConfig");
}

ObjectRef GeneratorConfig::NewInstance()
{
	return ObjectRef(new GeneratorConfig);
}

//====================================


void GeneratorConfigTopObject::Define(Serializer* s)
{
	s->ObjectValue(GENERATOR_CONFIG_PARAM, m_config, true);
}

void GeneratorConfigTopObject::Validate()
{
	;
}

CStdString GeneratorConfigTopObject::GetClassName()
{
	return CStdString("GeneratorConfigTopObject");
}

ObjectRef GeneratorConfigTopObject::NewInstance()
{
	return ObjectRef(new GeneratorConfigTopObject);
}

