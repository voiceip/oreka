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

#include <list>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include "ace/Singleton.h"
#include "Config.h"
#include "AudioCapturePlugin.h"

class ConfigManager;

class DLL_IMPORT_EXPORT_ORKBASE ConfigManager
{
public:
	static ConfigManager* Instance();
	void Initialize();
	void AddConfigureFunction(ConfigureFunction);

	Config m_config;
private:
	static ConfigManager* m_singleton;
	std::list<ConfigureFunction> m_configureFunctions;
	DOMNode* m_configTopNode;
	XercesDOMParser *m_parser;
};

#define CONFIG ConfigManager::Instance()->m_config

#endif

