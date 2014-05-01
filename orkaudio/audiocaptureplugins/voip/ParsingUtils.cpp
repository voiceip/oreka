/*
 * Oreka -- A media capture and retrieval platform
 *
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 */
#include "ParsingUtils.h"

// Convert a piece of memory to hex string
void memToHex(unsigned char* input, size_t len, CStdString&output)
{
	char byteAsHex[10];
	for(unsigned int i=0; i<len; i++)
	{
		sprintf(byteAsHex, "%.2x", input[i]);
		output += byteAsHex;
	}
}

// Same as standard memchr but case insensitive
inline char* memnchr(void *s, int c, size_t len)
{
	char lowerCase = tolower(c);
	char upperCase = toupper(c);
	char* stop = (char*)s + len;
	for(char* ptr = (char*)s ; ptr < stop; ptr++)
	{
		if(*ptr == lowerCase || *ptr == upperCase)
		{
			return ptr;
		}
	}
	return NULL;
}

static char* memFindStr(const char* toFind, char* start, char* stop)
{
	for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memchr(ptr+1, toFind[0],(stop - ptr - 1)))
	{
		if(ACE_OS::strncasecmp(toFind, ptr, ((int)strlen(toFind) > (stop-ptr) ? (stop-ptr) : strlen(toFind))) == 0)
		{
			return (ptr);
		}
	}
	return NULL;
}

// find the address that follows the given search string between start and stop pointers - case insensitive
char* memFindAfter(const char* toFind, char* start, char* stop)
{
	for(char * ptr = start; (ptr<stop) && (ptr != NULL); ptr = (char *)memnchr(ptr+1, toFind[0],(stop - ptr - 1)))
	{
		if(ACE_OS::strncasecmp(toFind, ptr, strlen(toFind)) == 0)
		{
			return (ptr+strlen(toFind));
		}
	}
	return NULL;
}

char* memFindEOL(char* start, char* limit)
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
void GrabToken(char* in, char* limit, CStdString& out)
{
	for(char* c = in; *c != '\0' && *c != 0x20 && *c != 0x0D && *c != 0x0A && c<limit; c = c+1)
	{
		out += *c;
	}
}

// Same as GrabToken but includes spaces in the token grabbed as opposed to stopping
// when a space is encountered
void GrabTokenAcceptSpace(char* in, char* limit, CStdString& out)
{
	for(char* c = in; *c != '\0' && *c != 0x0D && *c != 0x0A && c<limit; c = c+1)
	{
		out += *c;
	}
}

// Same as GrabToken but skipping leading whitespaces
void GrabTokenSkipLeadingWhitespaces(char* in, char* limit, CStdString& out)
{
	char* c = in;
	while(*c == 0x20 && (*c != '\0' && *c != 0x0D && *c != 0x0A) && c<limit)
	{
		c = c+1;
	}
	GrabToken(c, limit, out);
}

void GrabAlphaNumToken(char * in, char* limit, CStdString& out)
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

// Same as GrabAlphaNumToken but skipping leading whitespaces
void GrabAlphaNumTokenSkipLeadingWhitespaces(char* in, char* limit, CStdString& out)
{
	char* c = in;
	while(*c == 0x20 && (*c != '\0' && *c != 0x0D && *c != 0x0A) && c < limit)
	{
		c = c+1;
	}
	GrabAlphaNumToken(c, limit, out);
}

char* SkipWhitespaces(char* in, char* limit)
{
	char* c = in;
	while(*c == 0x20 && (*c != '\0' && *c != 0x0D && *c != 0x0A) && c < limit)
	{
		c = c+1;
	}
	return c;
}

void GrabSipUriDomain(char* in, char* limit, CStdString& out)
{
	char* userStart = SkipWhitespaces(in, limit);
	if(userStart >= limit)
	{
		return;
	}

	char* domainStart = strchr(userStart, '@');
	if(!domainStart)
	{
		return;
	}

	domainStart += 1;
	if(*domainStart == '\0' || domainStart >= limit)
	{
		return;
	}

	for(char *c = domainStart; (ACE_OS::ace_isalnum(*c) || *c == '.' || *c == '-' || *c == '_') && (c < limit); c = c+1)
	{
		out += *c;
	}
}

char* GrabDisplayableToken(char* in, char* limit, CStdString& out)
{
	// Look for first printable character
	char* start = in;
	char* c = NULL;
	while (!ACE_OS::ace_isgraph(*start) && start<limit)
	{
		start++;
	}

	if(start != (limit -1))
	{
		for(c = start; ACE_OS::ace_isgraph(*c) && c<limit; c = c+1)
		{
			out += *c;
		}
		return c;
	}
	return NULL;
}


void GrabSipName(char* in, char* limit, CStdString& out)
{
	char* nameStart = SkipWhitespaces(in, limit);
	char* nameEnd = memFindStr("<sip:", nameStart, limit);

	if(nameStart >= limit)
	{
		return;
	}

	if(nameEnd == NULL)
	{
		return;
	}

	if(nameEnd <= nameStart)
	{
		return;
	}

	// Get all characters before the <sip:
	for(char *c = nameStart; c < nameEnd; c = c+1)
	{
		if(c == nameStart && *c == '"')
		{
			continue;
		}
		if(((c+2 == nameEnd) || (c+1 == nameEnd)) && *c == '"')
		{
			break;
		}
		if(c+1 == nameEnd && *c == ' ')
		{
			break;
		}
		out += *c;
	}
}

void GrabSipUserAddress(char* in, char* limit, CStdString& out)
{
	char* userStart = SkipWhitespaces(in, limit);
	bool passedUserPart = false;

	if(userStart >= limit)
	{
		return;
	}

	/* Taken from RFC 1035, section 2.3.1 recommendation for
	 * domain names, we will add checks for '.' and '@' to allow
	 * the host part */
	for(char* c = userStart; (ACE_OS::ace_isalnum(*c) || *c == '#' || *c == '*' || *c == '.' || *c == '+' || *c == '-' || *c == '_' || *c == ':' || *c == '@' ) && c < limit ; c = c+1)
	{
		if(*c == '@' && !passedUserPart)
		{
			passedUserPart = true;
		}

		if(*c == ':' && passedUserPart)
		{
			break;
		}

		out += *c;
	}
}

void GrabSipUriUser(char* in, char* limit, CStdString& out)
{
	char* userStart = SkipWhitespaces(in, limit);
	if(userStart>=limit)
	{
		return;
	}
	// What stops a SIP URI user is a ':' (separating user from pwd) or an '@' (separating user from hostname)
	// but no need to test for these as we only allow alphanums and a few other chars
	for(char* c = userStart; (ACE_OS::ace_isalnum(*c) || *c == '#' || *c == '*' || *c == '.' || *c == '+' || *c == '-' || *c == '_' || *c == '%') && c < limit ; c = c+1)
	{
		out += *c;
	}
}


void GrabString(char* start, char* stop, CStdString& out)
{
	char* c = start;
	while(c <= stop)
	{
		out += *c++;
	}
}

// Grabs a line of characters in memory from start pointer
// returns the end of line
char* GrabLine(char* start, char* limit, CStdString& out)
{
	char* c = start;
	while(c < limit && *c != 0x0D && *c != 0x0A)
	{
		out += *c++;
	}
	return c;
}

// Grabs a line of characters in memory skipping any leading
// whitespaces.  This is intended for use in the case of SIP
// headers, ref RFC 3261, section 7.3.1
void GrabLineSkipLeadingWhitespace(char* start, char* limit, CStdString& out)
{
	char* c = SkipWhitespaces(start, limit);

	GrabLine(c, limit, out);
}
