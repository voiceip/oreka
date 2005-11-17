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

#endif

