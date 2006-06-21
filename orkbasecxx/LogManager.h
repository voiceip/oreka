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

using namespace log4cxx;


class DLL_IMPORT_EXPORT OrkLogManager
{
public:
	void Initialize();
	void Shutdown();

	LoggerPtr rootLog;
	LoggerPtr topLog;
	LoggerPtr immediateProcessingLog;
	LoggerPtr batchProcessingLog;
	LoggerPtr portLog;
	LoggerPtr fileLog;
	LoggerPtr reportingLog;
	LoggerPtr configLog;
	LoggerPtr tapelistLog;
	LoggerPtr tapeLog;
};

typedef ACE_Singleton<OrkLogManager, ACE_Thread_Mutex> OrkLogManagerSingleton;

#define LOG (*OrkLogManagerSingleton::instance())

#endif

