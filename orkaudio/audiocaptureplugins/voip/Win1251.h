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

#ifndef __WIN1251_H__
#define __WIN1251_H__

#include "PacketHeaderDefs.h"

#define PARTY_NAME_SIZE 40		//SKINNY_CALLING_PARTY_NAME_SIZE and SKINNY_CALLED_PARTY_NAME_SIZE have same size

void InitializeWin1251Table(unsigned short utf[256]);
void ConvertWin1251ToUtf8(char partyName[PARTY_NAME_SIZE], unsigned short utf[256]);


#endif
