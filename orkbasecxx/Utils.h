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

#ifndef __UTILS_H__
#define __UTILS_H__

#include "ace/Guard_T.h"	// For some reason, this include must always come before the StdString include
							// otherwise it gives the following compile error:
							//	error C2039: 'TryEnterCriticalSection' : is not a member of '`global namespace''
							// If seeing this error somewhere, the remedy is to #include "Utils.h" first
#include "ace/Thread_Mutex.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_time.h"
#include "StdString.h"

#include "OrkBase.h"

inline  CStdString IntToString(int integer)
{
	CStdString ret;
	ret.Format("%d", integer);
	return ret;
}

inline int StringToInt(CStdString& value)
{
	char* errorLocation = NULL;
	PCSTR szValue = (PCSTR)value;
	int intValue = strtol(szValue, &errorLocation, 10);
	if(*errorLocation != '\0')
		throw CStdString(CStdString("StringToInt: invalid integer:") + value);
	return intValue;
}

inline CStdString DoubleToString(double value)
{
	CStdString ret;
	ret.Format("%f", value);
	return ret;
}

inline double StringToDouble(CStdString& value)
{
	char* errorLocation = NULL;
	PCSTR szValue = (PCSTR)value;
	double doubleValue = strtod(szValue, &errorLocation);
	if(errorLocation == szValue)
		throw CStdString(CStdString("StringToDouble: invalid double:") + value);
	return doubleValue;
}

inline CStdString BaseName(CStdString& path)
{
	CStdString result;
	int lastSeparatorPosition = path.ReverseFind('/');
	if(lastSeparatorPosition == -1)
	{
		lastSeparatorPosition = path.ReverseFind('\\');
	}
	if(lastSeparatorPosition != -1 && path.GetLength()>3)
	{
		result = path.Right(path.GetLength() - lastSeparatorPosition - 1);
	}
	else
	{
		result = path;
	}
	return result;
}

inline CStdString StripFileExtension(CStdString& filename)
{
	CStdString result;
	int extensionPosition = filename.ReverseFind('.');
	if (extensionPosition != -1)
	{
		result = filename.Left(extensionPosition);
	}
	else
	{
		result = filename;
	}
	return result;
}

typedef ACE_Guard<ACE_Thread_Mutex> MutexSentinel;

/** A counter that generates a "counting" 3 character strings, i.e. aaa, aab, ..., zzz 
	that represents a number between 0 and 26^3-1 (wrapping counter)
	and starts at a random location in this range.
	useful for generating debugging IDs
*/
class AlphaCounter
{
public:
	inline AlphaCounter::AlphaCounter()
	{
		// Generate pseudo-random number from high resolution time least significant two bytes
		ACE_hrtime_t hrtime = ACE_OS::gethrtime();
		unsigned short srandom = (short)hrtime;
		double drandom = (double)srandom/65536.0; 	// 0 <= random < 1 

		m_counter = (unsigned int)(drandom*(26*26*26));
	}

	inline CStdString AlphaCounter::GetNext()
	{
		m_counter++;
		if(m_counter >= (26*26*26) )
		{
			m_counter = 0;
		}
		unsigned int char1val = m_counter/(26*26);
		unsigned int remains = m_counter%(26*26);
		unsigned int char2val = remains/26;
		unsigned int char3val = remains%26;

		char1val += 65;
		char2val += 65;
		char3val += 65;

		CStdString string;
		string.Format("%c%c%c", char1val, char2val, char3val);
		return string;
	}
private:
	unsigned int m_counter;
};

#endif

