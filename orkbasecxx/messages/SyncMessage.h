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

#pragma warning( disable: 4786 )

#ifndef __SYNCMESSAGE_H__
#define __SYNCMESSAGE_H__


#include "Message.h"

/** A SyncMessage is a synchronous message that needs an immediate answer from the remote server.
    The response should be an AsyncMessage
*/
class DLL_IMPORT_EXPORT_ORKBASE SyncMessage : public Message
{
public:
};

#endif

