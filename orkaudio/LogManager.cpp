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

void LogManager::Initialize()
{
	BasicConfigurator::configure();

	// If this one fails, the above default configuration stays valid
	PropertyConfigurator::configure("logging.properties");

	rootLog  = Logger::getLogger("root");
	topLog = Logger::getLogger("top");
	immediateProcessingLog = Logger::getLogger("immediateProcessing");
	batchProcessingLog = Logger::getLogger("batchProcessing");
	portLog =  Logger::getLogger("port");
	fileLog = Logger::getLogger("file");
	reportingLog = Logger::getLogger("reporting");
	configLog = Logger::getLogger("config");
	tapelistLog = Logger::getLogger("tapelist");
}

