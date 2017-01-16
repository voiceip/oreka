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

#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_strings.h"
#include "ace/OS_NS_ctype.h"

#include "MemUtils.h"

// Convert a piece of memory to hex string
void MemToHex(unsigned char* input, size_t len, CStdString&output)
{
	char byteAsHex[10];
	for(unsigned int i=0; i<len; i++)
	{
		sprintf(byteAsHex, "%.2x", input[i]);
		output += byteAsHex;
	}
}

// find the address that follows the given search string between start and stop pointers - case insensitive
char* MemFindAfter(char* toFind, char* start, char* stop)
{
	for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memchr(ptr+1, toFind[0],(stop - start)))
	{
		if(ACE_OS::strncasecmp(toFind, ptr, strlen(toFind)) == 0)
		{
			return (ptr+strlen(toFind));
		}
	}
	return NULL;
}

char* MemFindEOL(char* start, char* limit)
{
	char* c = start;
	while(*c != '\r' && *c != '\n' && c<limit)
	{
		c++;
	}
	if(*c == '\r' || *c == '\n')
	{
		return c;
	}
	return start;
}

// Grabs a string in memory until encountering null char, a space a CR or LF chars
void MemGrabToken(char* in, CStdString& out)
{
	for(char* c = in; *c != '\0' && *c != 0x20 && *c != 0x0D && *c != 0x0A; c = c+1)
	{
		out += *c;
	}
}

void MemGrabAlphaNumToken(char * in, char* limit, CStdString& out)
{
	// Look for first alphanum character
	char* start = in;
	while (!ACE_OS::ace_isalnum(*start) && start<limit)
	{
		start++;
	}

	if(start != (limit -1))
	{
		for(char* c = start; ACE_OS::ace_isalnum(*c); c = c+1)
		{
			out += *c;
		}
	}
}

void MemGrabString(char* start, char* stop, CStdString& out)
{
	char* c = start;
	while(c <= stop)
	{
		out += *c++;
	}
}

// Grabs a line of characters in memory from start pointer
// returns the end of line
char* MemGrabLine(char* start, char* limit, CStdString& out)
{
	char* c = start;
	while(c < limit && *c != 0x0D && *c != 0x0A)
	{
		out += *c++;
	}
	return c;
}

void MemMacToHumanReadable(unsigned char* macAddress, CStdString&output)
{
	char byteAsHex[10];

	for(int i=0; i<6; i++)
	{
		ACE_OS::snprintf(byteAsHex, sizeof(byteAsHex), "%.2x", macAddress[i]);
		if(output.size())
		{
			output += ":";
		}
		output += byteAsHex;
	}
}

char* MemGrabAlphaNumSpace(char* in, char* limit, CStdString& out)
{
	// Look for first printable character
	char* start = in;
	char* c = NULL;
	while (!ACE_OS::ace_isalnum(*start) && start<limit)
	{
		start++;
	}

	if(start < (limit -1))
	{
		for(c = start; (ACE_OS::ace_isalnum(*c) || ACE_OS::ace_isspace(*c)) && c<limit; c = c+1)
		{
			out += *c;
		}

		return c;
	}

	return NULL;
}

char* MemGrabDigits(char* in, char* limit, CStdString& out)
{
	// Look for first printable character
	char* start = in;
	char* c = NULL;
	while (!ACE_OS::ace_isdigit(*start) && start<limit)
	{
		start++;
	}

	if(start < (limit -1))
	{
		for(c = start; ACE_OS::ace_isdigit(*c) && c<limit; c = c+1)
		{
			out += *c;
		}

		return c;
	}

	return NULL;
}

char* memFindAfterBinary(const char * const memToFind, size_t size, const char* const  memStart, const char * const memEnd) {
	const char* pos = memStart;

	while (pos <= (memEnd - size)) {
		bool equal = true;
		for (int i=0; i<size; i++) {
			if (pos[i] != memToFind[i]) {
				equal = false;
				break;
			}
		}
		if (equal) {
			return (char*) (pos+size);
		}
		pos++;
	}

	return NULL;
}

