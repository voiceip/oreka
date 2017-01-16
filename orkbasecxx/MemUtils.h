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
#include "OrkBase.h"

void DLL_IMPORT_EXPORT_ORKBASE	MemToHex(unsigned char* input, size_t len, CStdString&output);
DLL_IMPORT_EXPORT_ORKBASE char* MemFindAfter(char* toFind, char* start, char* stop);
DLL_IMPORT_EXPORT_ORKBASE char* MemFindEOL(char* start, char* limit);
void DLL_IMPORT_EXPORT_ORKBASE	MemGrabToken(char* in, CStdString& out);
void DLL_IMPORT_EXPORT_ORKBASE	MemGrabAlphaNumToken(char * in, char* limit, CStdString& out);
void DLL_IMPORT_EXPORT_ORKBASE	MemGrabString(char* start, char* stop, CStdString& out);
DLL_IMPORT_EXPORT_ORKBASE char* MemGrabLine(char* start, char* limit, CStdString& out);
void DLL_IMPORT_EXPORT_ORKBASE  MemMacToHumanReadable(unsigned char* macAddress, CStdString&output);
DLL_IMPORT_EXPORT_ORKBASE char* MemGrabAlphaNumSpace(char* in, char* limit, CStdString& out);
DLL_IMPORT_EXPORT_ORKBASE char* MemGrabDigits(char* in, char* limit, CStdString& out);
DLL_IMPORT_EXPORT_ORKBASE char* memFindAfterBinary(const char * const memToFind, size_t size, const char* const  memStart, const char * const memEnd);

#endif

