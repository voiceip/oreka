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

#define _WINSOCKAPI_            // prevents the inclusion of winsock.h

#include "ConfigManager.h"
#include "Utils.h"

#ifdef WIN32
# ifndef snprintf
#  define snprintf _snprintf
# endif
#endif

bool PartyFilterActive(void)
{
	if(!CONFIG.m_partyFilter.size())
	{
		return false;
	}

	return true;
}

bool PartyFilterMatches(CStdString& party)
{
	char buf[1024];
	char *lval = NULL;
	char *rval = NULL;
	long leftval = 0;
	long rightval = 0;
	bool res = false;
	long partyVal = 0;

	try {
		partyVal = StringToInt(party);
	} catch (CStdString& e) {
		partyVal = 0;
	}

	for(std::list<CStdString>::iterator it = CONFIG.m_partyFilter.begin(); it != CONFIG.m_partyFilter.end(); it++)
	{
		CStdString pattern = *it;

		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "%s", pattern.c_str());
		leftval = 0;
		rightval = 0;

		lval = buf;
		if((rval = strchr(buf, '-')))
		{
			*rval++ = '\0';
		}
		else
		{
			rval = NULL;
		}

		
		if(lval && strlen(lval))
		{
			leftval = strtol(lval, NULL, 10);
		}
		else
		{
			continue;
		}

		if(rval && strlen(rval))
		{
			rightval = strtol(rval, NULL, 10);
			if(partyVal >= leftval && partyVal <= rightval)
			{
				res = true;
				break;
			}
		}
		else
		{
			if(strcmp(lval, party.c_str()) == 0)
			{
				res = true;
				break;
			}
		}
	}

	return res;
}
