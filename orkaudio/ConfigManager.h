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

#ifndef __CONFIGMANAGER_H__
#define __CONFIGMANAGER_H__

#include <xercesc/parsers/XercesDOMParser.hpp>
#include "ace/Singleton.h"
#include "Config.h"
#include "AudioCapturePlugin.h"

class ConfigManager
{
public:
	void Initialize();
	void AddConfigureFunction(ConfigureFunction);

	Config m_config;
private:
	ConfigureFunction m_configureFunction;
	DOMNode* m_configTopNode;
	XercesDOMParser *m_parser;
};

typedef ACE_Singleton<ConfigManager, ACE_Thread_Mutex> ConfigManagerSingleton;

#define CONFIG ConfigManagerSingleton::instance()->m_config

#endif

