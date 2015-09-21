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

#include "ace/OS_NS_dirent.h"
#include "LogManager.h"
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/logmanager.h>

OrkLogManager OrkLogManager::m_orkLogManager;

OrkLogManager* OrkLogManager::Instance()
{
	return &m_orkLogManager;
}

void OrkLogManager::Initialize()
{
	BasicConfigurator::resetConfiguration();
	BasicConfigurator::configure();

	char* logCfgFilename = NULL;
        char* cfgEnvPath = "";
        int cfgAlloc = 0;

        cfgEnvPath = ACE_OS::getenv("ORKAUDIO_CONFIG_PATH");
        if(cfgEnvPath) {
                ACE_DIR* dir = ACE_OS::opendir(cfgEnvPath);
                if(dir) {
                        int len = 0;

                        ACE_OS::closedir(dir);
                        len = strlen(cfgEnvPath)+1+strlen("logging.properties")+1;
                        logCfgFilename = (char*)malloc(len);

                        if(logCfgFilename) {
                                cfgAlloc = 1;
								ACE_OS::snprintf(logCfgFilename, len, "%s/%s", cfgEnvPath, "logging.properties");
                        }
                }
        }

	if(!logCfgFilename) {
		FILE* file = ACE_OS::fopen("logging.properties", "r");
		if(file)
		{
			// logging.properties exists in the current directory
			logCfgFilename = (char*)"logging.properties";
			fclose(file);
		}
		else
		{
			// logging.properties could not be found in the current
			// directory, try to find it in system configuration directory
			logCfgFilename = (char*)"/etc/orkaudio/logging.properties";
		}
	}

	// If this one fails, the above default configuration stays valid
	PropertyConfigurator::configure(logCfgFilename);

	// XXX should we free this here?
	if(cfgAlloc) {
		free(logCfgFilename);
	}

	rootLog  = Logger::getLogger("root");
	topLog = Logger::getLogger("top");
	immediateProcessingLog = Logger::getLogger("immediateProcessing");
	batchProcessingLog = Logger::getLogger("batchProcessing");
	tapeFileNamingLog = Logger::getLogger("tapeFileNamingLog");
	portLog =  Logger::getLogger("port");
	fileLog = Logger::getLogger("file");
	configLog = Logger::getLogger("config");
	tapelistLog = Logger::getLogger("tapelist");
	tapeLog = Logger::getLogger("tape");
	clientLog = Logger::getLogger("orkclient");
	directionSelectorLog = Logger::getLogger("directionSelector");
	reporting = Logger::getLogger("reporting");
	ipfragmentation = Logger::getLogger("ipfragmentation");
	messaging = Logger::getLogger("messaging");
}

void OrkLogManager::Shutdown()
{
	log4cxx::LogManager::shutdown();
}

