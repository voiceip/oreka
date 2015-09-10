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

#ifndef __ORKCLIENT_H__
#define __ORKCLIENT_H__

#include <log4cxx/logger.h>
#include "messages/SyncMessage.h"
#include "messages/AsyncMessage.h"

/** Abstract base class for all clients. */
class DLL_IMPORT_EXPORT_ORKBASE OrkClient
{
public:
	OrkClient();
	virtual bool Execute(SyncMessage& request, AsyncMessage& response, const CStdString& hostname, const int tcpPort, const CStdString& serviceName, int timeout = 5) = 0;
protected:
	void LogError(CStdString& errorString);

	log4cxx::LoggerPtr m_log;
	static time_t s_lastErrorReportedTime;
};

/** Abstract base class for all clients based on http. */
class DLL_IMPORT_EXPORT_ORKBASE OrkHttpClient : public OrkClient
{
public:
	virtual bool Execute(SyncMessage& request, AsyncMessage& response, const CStdString& hostname, const int tcpPort, const CStdString& serviceName, int timeout = 5) = 0;
	bool ExecuteUrl(const CStdString& request, CStdString& response, const CStdString& hostname, const int tcpPort, int timeout = 5);
protected:
};

/** Client that uses a HTTP URL request and receives the response back in the SingleLine format. */
class DLL_IMPORT_EXPORT_ORKBASE OrkHttpSingleLineClient : public OrkHttpClient
{
public:
	bool Execute(SyncMessage& request, AsyncMessage& response, const CStdString& hostname, const int tcpPort, const CStdString& serviceName, int timeout = 5);
};

#endif
