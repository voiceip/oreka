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

#ifndef __MEMUTILS_H__
#define __MEMUTILS_H__

#include "StdString.h"

void	MemToHex(unsigned char* input, size_t len, CStdString&output);
char*	MemFindAfter(char* toFind, char* start, char* stop);
char*	MemFindEOL(char* start, char* limit);
void	MemGrabToken(char* in, CStdString& out);
void	MemGrabAlphaNumToken(char * in, char* limit, CStdString& out);
void	MemGrabString(char* start, char* stop, CStdString& out);
char*	MemGrabLine(char* start, char* limit, CStdString& out);


#endif

