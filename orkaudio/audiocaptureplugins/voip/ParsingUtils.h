/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */
#ifndef __PARSINGUTILS_H__
#define __PARSINGUTILS_H__ s

#include <list>
#include "Utils.h"
#include <map>

void memToHex(unsigned char* input, size_t len, CStdString&output);
inline char* memnchr(void *s, int c, size_t len);
static char* memFindStr(const char* toFind, char* start, char* stop);
char* memFindAfter(const char* toFind, char* start, char* stop);
char* memFindEOL(char* start, char* limit);
void GrabToken(char* in, char* limit, CStdString& out);
void GrabTokenAcceptSpace(char* in, char* limit, CStdString& out);
void GrabTokenSkipLeadingWhitespaces(char* in, char* limit, CStdString& out);
void GrabAlphaNumToken(char * in, char* limit, CStdString& out);
void GrabAlphaNumTokenSkipLeadingWhitespaces(char* in, char* limit, CStdString& out);
char* SkipWhitespaces(char* in, char* limit);
void GrabSipUriDomain(char* in, char* limit, CStdString& out);
char* GrabDisplayableToken(char* in, char* limit, CStdString& out);
void GrabSipName(char* in, char* limit, CStdString& out);
void GrabSipUserAddress(char* in, char* limit, CStdString& out);
void GrabSipUriUser(char* in, char* limit, CStdString& out);
void GrabString(char* start, char* stop, CStdString& out);
char* GrabLine(char* start, char* limit, CStdString& out);
void GrabLineSkipLeadingWhitespace(char* start, char* limit, CStdString& out);
void GetDynamicPayloadMapping(char* start, char* stop, unsigned char* map);

#endif
