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

#include "LogManager.h"
#include <log4cxx/propertyconfigurator.h>
#include <log4cxx/basicconfigurator.h>
#include <log4cxx/logmanager.h>
#include "Utils.h"
#include <fstream>

OrkLogManager OrkLogManager::m_orkLogManager;

OrkLogManager* OrkLogManager::Instance()
{
	return &m_orkLogManager;
}

void OrkLogManager::Initialize()
{
	OrkAprSubPool locPool;

	BasicConfigurator::resetConfiguration();
	BasicConfigurator::configure();
	apr_status_t ret;
	char* logCfgFilename = NULL;
	char* cfgEnvPath = "";
	int cfgAlloc = 0;

	ret = apr_env_get(&cfgEnvPath, "ORKAUDIO_CONFIG_PATH", AprLp);
	if(ret == APR_SUCCESS) {
		apr_dir_t* dir;
		ret = apr_dir_open(&dir, cfgEnvPath, AprLp);
		if(ret == APR_SUCCESS)
		{
			int len = 0;
			apr_dir_close(dir);
			len = strlen(cfgEnvPath)+1+strlen("logging.properties")+1;
			logCfgFilename = (char*)malloc(len);

					if(logCfgFilename) {
							cfgAlloc = 1;
							apr_snprintf(logCfgFilename, len, "%s/%s", cfgEnvPath, "logging.properties", AprLp);
					}
		}
	}

	if(!logCfgFilename) {
		std::fstream file;
		file.open("logging.properties", std::fstream::in);
		if(file.is_open()){
			logCfgFilename = (char*)"logging.properties";
			file.close();
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

