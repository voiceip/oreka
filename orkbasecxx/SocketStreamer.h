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
#ifndef __SOCKETSTREAMER_H__
#define __SOCKETSTREAMER_H__ 1


#include "StdString.h"
#include "OrkBase.h"
#include "Utils.h"
#include <list>
#include "LogManager.h"
#include "shared_ptr.h"

class SocketStreamerFactory;

class CdrInfo {
	public:
	CdrInfo () :
		m_duration(0)
	{}

	CStdString m_localParty;
	CStdString m_remoteParty;
	int m_duration;

	virtual bool IsValid() { return false; }
	virtual void Parse(const char *buf, size_t len) {}

	virtual CStdString ToString() {
		CStdString s;
		s.Format("localparty:%s remoteparty:%s duration:%d ", m_localParty, m_remoteParty, m_duration);
		return s;
	}
};
typedef oreka::shared_ptr<CdrInfo> CdrInfoRef;


class DLL_IMPORT_EXPORT_ORKBASE SocketStreamer {
public:
	static void Initialize(std::list<CStdString>& targetList, SocketStreamerFactory *factory=NULL);

protected:
	SocketStreamer(LoggerPtr log, CStdString threadName);
	bool Connect();
	void Close();
	size_t Recv();
	virtual bool ProcessData();
	virtual bool Handshake();
	virtual bool Parse(CStdString target);

	CStdString m_protocol;
	CStdString m_logMsg;
	CStdString m_threadName;
	LoggerPtr m_log;

	in_addr m_ip;
	unsigned short m_port;
	unsigned char m_buf[1024];
	size_t m_bytesRead;
	static void ThreadHandler(void *args);

	friend class SocketStreamerFactory;

	apr_pool_t* m_peer;
	apr_socket_t* m_socket;
private:
	bool Spawn();
};

class DLL_IMPORT_EXPORT_ORKBASE SocketStreamerFactory
{
protected:
	SocketStreamerFactory() {}
    virtual SocketStreamer* Create();
	virtual bool Accepts(CStdString protocolName) { return (protocolName == ""); }

	friend class SocketStreamer;
};

#define DEFINE_CDR(CdrType,LoggerName) \
static std::list<CdrType>& getList() { static std::list<CdrType> list; return list; }  \
static std::mutex& getMutex() { static std::mutex m; return m; } \
static LoggerPtr getLog() { static LoggerPtr s_log = Logger::getLogger(LoggerName); return s_log; } \
static void SaveCdr(CdrType info) \
{ \
	CStdString logMsg; \
	if ( info->IsValid() ) { \
		MutexSentinel ms(getMutex()); \
		getList().push_back(info); \
		unsigned int sz = getList().size(); \
		FLOG_INFO(getLog(),"Saving : %s . new list size = %u",info->ToString(),sz); \
	} \
	else { \
		FLOG_INFO(getLog(),"Discarding : %s",info->ToString()); \
	} \
}

#endif
