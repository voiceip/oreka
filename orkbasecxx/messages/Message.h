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

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

//#ifdef WIN32
#pragma warning( disable: 4786 ) // disables truncated symbols in browse-info warning
#pragma warning( disable: 4018 ) // signed/unsigned mismatch
//#endif

#include "OrkBase.h"
#include "ace/OS_NS_time.h"
#include "ace/OS_NS_unistd.h"

#include "serializers/Serializer.h"
#include "Object.h"

#define HOSTNAME_BUF_LEN 40

#define TIMESTAMP_PARAM "timestamp"
#define CAPTURE_PORT_PARAM "captureport"
#define FILENAME_PARAM "filename"

#define SUCCESS_PARAM "sucess"
#define SUCCESS_DEFAULT true

/** A Message is an Object that is meant to be sent to a remote server.
*/
class DLL_IMPORT_EXPORT Message : public Object
{
public:
	Message();

	bool InvokeXmlRpc(CStdString& hostname, int tcpport);
protected:
	time_t m_creationTime;
	bool m_sent;
	static char m_hostname[HOSTNAME_BUF_LEN];
};

typedef boost::shared_ptr<Message> MessageRef;

#endif

