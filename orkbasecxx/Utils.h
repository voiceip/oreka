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

#include <list>
#include <sys/stat.h>

#include "ace/Guard_T.h"	// For some reason, this include must always come before the StdString include
							// otherwise it gives the following compile error:
							//	error C2039: 'TryEnterCriticalSection' : is not a member of '`global namespace''
							// If seeing this error somewhere, the remedy is to #include "Utils.h" first
#include "ace/Thread_Mutex.h"
#include "ace/OS_NS_stdlib.h"
#include "ace/OS_NS_time.h"
#include "ace/OS_NS_arpa_inet.h"
#include "StdString.h"

#include "OrkBase.h"
#include "dll.h"

//============================================
// String related stuff

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

inline CStdString IpToString(const struct in_addr& ip) {
	char s[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&ip, s, sizeof(s));
	return CStdString(s);
}

bool DLL_IMPORT_EXPORT_ORKBASE StringIsDigit(CStdString& string);
bool DLL_IMPORT_EXPORT_ORKBASE StringIsPhoneNumber(CStdString& string);
bool DLL_IMPORT_EXPORT_ORKBASE MatchesStringList(CStdString& string, std::list<CStdString>& stringList);
CStdString DLL_IMPORT_EXPORT_ORKBASE GetHostFromAddressPair(CStdString& hostname);
int DLL_IMPORT_EXPORT_ORKBASE GetPortFromAddressPair(CStdString& hostname);
CStdString DLL_IMPORT_EXPORT_ORKBASE FormatDataSize(unsigned long int size);
CStdString DLL_IMPORT_EXPORT_ORKBASE HexToString(const CStdString& hexInput);		//Only return digits
CStdString DLL_IMPORT_EXPORT_ORKBASE IntUnixTsToString(int ts);
void DLL_IMPORT_EXPORT_ORKBASE StringTokenizeToList(CStdString input, std::list<CStdString>& output);

//========================================================
// file related stuff

CStdString DLL_IMPORT_EXPORT_ORKBASE FileBaseName(CStdString& path);
CStdString DLL_IMPORT_EXPORT_ORKBASE FilePath(CStdString& path);
CStdString DLL_IMPORT_EXPORT_ORKBASE FileStripExtension(CStdString& filename);
bool DLL_IMPORT_EXPORT_ORKBASE FileCanOpen(CStdString& path);
void DLL_IMPORT_EXPORT_ORKBASE FileRecursiveMkdir(CStdString& path, int permissions, CStdString owner, CStdString group, CStdString rootDirectory);
int DLL_IMPORT_EXPORT_ORKBASE FileSetPermissions(CStdString filename, int permissions);
int DLL_IMPORT_EXPORT_ORKBASE FileSetOwnership(CStdString filename, CStdString owner, CStdString group);
void DLL_IMPORT_EXPORT_ORKBASE FileEscapeName(CStdString& in, CStdString& out);
bool DLL_IMPORT_EXPORT_ORKBASE FileIsExist(CStdString fileName);
int DLL_IMPORT_EXPORT_ORKBASE FileSizeInKb(CStdString fileName);	//return file's size in Kb

//===========================================================
int DLL_IMPORT_EXPORT_ORKBASE GetOrekaRtpPayloadTypeForSdpRtpMap(CStdString sdp);

//=====================================================
// threading related stuff

typedef ACE_Guard<ACE_Thread_Mutex> MutexSentinel;


//=====================================================
// Network related stuff
typedef struct 
{
	void ToString(CStdString& string);

	in_addr ip;
	unsigned short port;
} TcpAddress;

class DLL_IMPORT_EXPORT_ORKBASE TcpAddressList
{
public:
	void AddAddress(struct in_addr ip, unsigned short port);
	bool HasAddress(struct in_addr ip, unsigned short port);
	bool HasAddressOrAdd(struct in_addr ip, unsigned short port);
private:
	std::list<TcpAddress> m_addresses;
};

class DLL_IMPORT_EXPORT_ORKBASE IpRanges
{
public:
	bool Matches(struct in_addr ip);
	void Compute();
	bool Empty();

	std::list<CStdString> m_asciiIpRanges;
private:
	std::list<unsigned int> m_ipRangePrefixes;
	std::list<unsigned int> m_ipRangeBitWidths;
};

void DLL_IMPORT_EXPORT_ORKBASE GetHostFqdn(CStdString& fqdn, int size);

//=====================================================
// Miscellanous stuff

/** A counter that generates a "counting" 3 character strings, i.e. aaa, aab, ..., zzz 
	that represents a number between 0 and 26^3-1 (wrapping counter)
	and starts at a random location in this range.
	useful for generating debugging IDs
*/
class AlphaCounter
{
public:
	inline AlphaCounter(int start = 0)
	{
		if(start)
		{
			m_counter = start;
		}
		else
		{
			// Generate pseudo-random number from high resolution time least significant two bytes
			ACE_hrtime_t hrtime = ACE_OS::gethrtime();
			unsigned short srandom = (short)hrtime;
			double drandom = (double)srandom/65536.0; 	// 0 <= random < 1 

			m_counter = (unsigned int)(drandom*(26*26*26*26));
		}
	}

	inline CStdString GetNext()
	{
		m_counter++;
		if(m_counter >= (26*26*26*26) )
		{
			m_counter = 0;
		}
		unsigned int char1val = m_counter/(26*26*26);
		unsigned int remains = m_counter%(26*26*26);
		unsigned int char2val = remains/(26*26);
		unsigned int remains2 = remains%(26*26);
		unsigned int char3val = remains2/26;
		unsigned int char4val = remains2%26;

		char1val += 65;
		char2val += 65;
		char3val += 65;
		char4val += 65;

		CStdString string;
		string.Format("%c%c%c%c", char1val, char2val, char3val, char4val);
		return string;
	}

	inline void Reset()
	{
		m_counter = 0;
	}
private:
	unsigned int m_counter;
};

#endif

