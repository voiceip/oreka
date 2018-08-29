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
#include "ace/INET_Addr.h"
#include "ace/SOCK_Stream.h"
#include <list>

class SocketStreamerFactory;

class DLL_IMPORT_EXPORT_ORKBASE SocketStreamer {
public:
	static void Initialize(std::list<CStdString>& targetList, SocketStreamerFactory *factory=NULL);

protected:
	SocketStreamer() {}
	bool Connect();
	void Close();
	size_t Recv();
	virtual void ProcessData();
	virtual bool Handshake();
	virtual bool Parse(CStdString target);

	CStdString m_protocol;
	CStdString m_logMsg;

	in_addr m_ip;
	unsigned short m_port;
	ACE_SOCK_Stream m_peer;
	unsigned char m_buf[1024];
	size_t m_bytesRead;
	static void ThreadHandler(void *args);

	friend class SocketStreamerFactory;

private:
	bool Spawn();
};

class SocketStreamerFactory
{
protected:
	SocketStreamerFactory() {}
    virtual SocketStreamer* Create() { return new SocketStreamer(); } 
	virtual bool Accepts(CStdString protocolName) { return (protocolName == ""); }

	friend class SocketStreamer;
};

#endif
