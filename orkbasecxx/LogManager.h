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

#ifndef __LOGMANAGER_H__
#define __LOGMANAGER_H__

#include <log4cxx/logger.h>
#include "ace/Singleton.h"
#include "dll.h"
#include "OrkBase.h"

using namespace log4cxx;


class DLL_IMPORT_EXPORT_ORKBASE OrkLogManager
{
public:
	static OrkLogManager* Instance();

	void Initialize();
	void Shutdown();

	LoggerPtr rootLog;
	LoggerPtr topLog;
	LoggerPtr immediateProcessingLog;
	LoggerPtr batchProcessingLog;
	LoggerPtr tapeFileNamingLog;
	LoggerPtr portLog;
	LoggerPtr fileLog;
	LoggerPtr configLog;
	LoggerPtr tapelistLog;
	LoggerPtr tapeLog;
	LoggerPtr clientLog;
	LoggerPtr directionSelectorLog;
	LoggerPtr reporting;
	LoggerPtr ipfragmentation;
	LoggerPtr messaging;

private:
	static OrkLogManager m_orkLogManager;
};

#define LOG (*OrkLogManager::Instance())

#define FLOG_DEBUG(logger,fmt, ...) logMsg.Format(fmt,__VA_ARGS__); LOG4CXX_DEBUG(logger, logMsg);
#define FLOG_INFO(logger,fmt, ...) logMsg.Format(fmt,__VA_ARGS__); LOG4CXX_INFO(logger, logMsg);
#define FLOG_WARN(logger,fmt, ...) logMsg.Format(fmt,__VA_ARGS__); LOG4CXX_WARN(logger, logMsg);
#define FLOG_ERROR(logger,fmt, ...) logMsg.Format(fmt,__VA_ARGS__); LOG4CXX_ERROR(logger, logMsg);

#endif

