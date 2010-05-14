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

#include "OrkBase.h"
#include <log4cxx/logger.h>
#include "ConfigManager.h"
#include "LogManager.h"
#include "ace/Thread_Manager.h"
#include "Utils.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Stream.h"
#include "ace/OS_NS_unistd.h"


class DLL_IMPORT_EXPORT_ORKBASE SocketStreamer {
public:
	static void Initialize();
	static void ThreadHandler(void *args);
	static int Connect(TcpAddress* tcpAddress, ACE_INET_Addr& srvr, ACE_SOCK_Connector& connector, ACE_SOCK_Stream& peer);
};

#endif
