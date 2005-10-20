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

using namespace log4cxx;


class LogManager
{
public:
	void Initialize();

	LoggerPtr rootLog;
	LoggerPtr topLog;
	LoggerPtr immediateProcessingLog;
	LoggerPtr batchProcessingLog;
	LoggerPtr portLog;
	LoggerPtr fileLog;
	LoggerPtr reportingLog;
	LoggerPtr configLog;
	LoggerPtr tapelistLog;
};

typedef ACE_Singleton<LogManager, ACE_Thread_Mutex> LogManagerSingleton;

#define LOG (*LogManagerSingleton::instance())

#endif

