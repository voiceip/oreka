#include "Utils.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/OS_NS_sys_stat.h"
#include "time.h"

#ifndef WIN32
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <unistd.h>
#endif

//========================================================
// String related stuff

bool StringIsDigit(CStdString& string)
{
	int size = string.size();
	for(int i=0; i<size; i++)
	{
		if(isdigit(string.GetAt(i)) == false)
		{
			return false;
		}
	}
	return true;
}

bool StringIsPhoneNumber(CStdString& string)
{
	int size = string.size();
	for(int i=0; i<size; i++)
	{
		char c = string.GetAt(i);
		if(isdigit(c) == false && c != '-' && c != '*' && c != '#' && c != '(' && c != ')' )
		{
			return false;
		}
	}
	return true;
}

bool MatchesStringList(CStdString& string, std::list<CStdString>& stringList)
{
	if(string.size() == 0)
	{
		return false;
	}
	for(std::list<CStdString>::iterator it = stringList.begin(); it != stringList.end(); it++)
	{
		CStdString element = *it;

		if(element.CompareNoCase(string) == 0)
		{
			return true;
		}
	}
	return false;
}

CStdString GetHostFromAddressPair(CStdString& hostname)
{
        int colidx = 0;

        if((colidx = hostname.Find(CStdString(":"))) >= 0)
        {
                return hostname.Left(colidx);
        }

        return hostname;
}

int GetPortFromAddressPair(CStdString& hostname)
{
        int colidx = 0;

        if((colidx = hostname.Find(CStdString(":"))) >= 0)
        {
		CStdString portString;

		portString = hostname.Right(hostname.size() - colidx - 1);
                return StringToInt(portString);
        }

        return 0;
}

CStdString FormatDataSize(unsigned long int size)
{
	CStdString sizeStr;
	double newsize;

	if(size <= 1024)
	{
		sizeStr.Format("%lu Byte(s)", size);
	}
	else if(size > 1024 && size <= 1048576)
	{
		newsize = (double)size / 1024;
		sizeStr.Format("%.2f KByte(s)", newsize);
	}
	else if(size > 1048576 && size <= 1073741824)
	{
		newsize = (double)size / 1048576;
		sizeStr.Format("%.2f MByte(s)", newsize);
	}
	else if(size > 1073741824)
	{
		newsize = (double)size / 1073741824;
		sizeStr.Format("%.2f GByte(s)", newsize);
	}

	return sizeStr;
}

CStdString HexToString(const CStdString& hexInput)
{
   	static const char* const lut = "0123456789ABCDEF";
    size_t len = hexInput.length();
    if (len & 1) 	//odd length
	{
		return "Invalid Hex";
	}

    CStdString output;
    output.reserve(len / 2);
    for (size_t i = 0; i < len; i += 2)
    {
        char a = hexInput.at(i);
        const char* p = std::lower_bound(lut, lut + 16, a);
        if (*p != a) 	//not a hex digit
		{
			continue;
		}

        char b = hexInput.at(i + 1);
        const char* q = std::lower_bound(lut, lut + 16, b);
        if (*q != b) 	//not a hex digit
		{
			continue;
		}

		int asciiVal = ((p - lut) << 4) | (q - lut);
		if(asciiVal  > 47 && asciiVal < 58)		//only take number here
		{
        	output.push_back(((p - lut) << 4) | (q - lut));
		}
		else if(output.length() > 0)
		{
			break;
		}
    }
    return output;

}

CStdString IntUnixTsToString(int ts)
{
	CStdString dateFormat;
	struct tm structTm;
	char calendarDate[80];
	time_t date;
	date = (time_t)ts;
	structTm = *localtime(&date);
	strftime(calendarDate, sizeof(calendarDate), "%a %Y-%m-%d %H:%M:%S %Z", &structTm);
	dateFormat.Format("%s", calendarDate);
	return dateFormat;
}

void StringTokenizeToList(CStdString input, std::list<CStdString>& output)
{
    int pos;
    CStdString token;
    input.TrimLeft();
    input.TrimRight();
    while((pos = input.find(" ")) != std::string::npos)
    {
        token = input.substr(0, pos);
        output.push_back(token);
        input = input.substr(pos+1);
    }
    output.push_back(input);
}

//========================================================
// file related stuff

CStdString FileBaseName(CStdString& path)
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

CStdString FilePath(CStdString& path)
{
	CStdString result;
	int lastSeparatorPosition = path.ReverseFind('/');
	if(lastSeparatorPosition == -1)
	{
		lastSeparatorPosition = path.ReverseFind('\\');
	}
	if(lastSeparatorPosition != -1 && path.GetLength()>3)
	{
		result = path.Left(lastSeparatorPosition + 1);
	}
	return result;
}

CStdString FileStripExtension(CStdString& filename)
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

bool FileCanOpen(CStdString& path)
{
	FILE* file = ACE_OS::fopen((PCSTR)path, "r");
	if(file)
	{
		ACE_OS::fclose(file);
		return true;
	}
	return false;
}

void FileRecursiveMkdir(CStdString& path, int permissions, CStdString owner, CStdString group, CStdString rootDirectory)
{
	int position = 0, newPermissions = permissions;
	bool done = false;

	/*
	 * Create the directories first. We have separated this because
	 * we do not want the introduction of rootDirectory to break
	 * any old functionality.
	 */
	while (!done)
	{
		position = path.Find('/', position+1);
		if (position == -1)
		{
			done = true;
		}
		else
		{
			CStdString level = path.Left(position);
			ACE_OS::mkdir((PCSTR)level);
		}
	}

	done = false;
	position = 0;
	if(rootDirectory.size())
	{
	        if(path.Find(rootDirectory) >= 0)
		{
			position = 1 + rootDirectory.size();
	        }
	}

	if(newPermissions & S_IRUSR)
	{
		newPermissions |= S_IXUSR;
	}

	if(newPermissions & S_IRGRP)
	{
		newPermissions |= S_IXGRP;
	}

	if(newPermissions & S_IROTH)
	{
		newPermissions |= S_IXOTH;
	}

	while (!done)
	{
		position = path.Find('/', position+1);
		if (position == -1)
		{
			done = true;
		}
		else
		{
			CStdString level = path.Left(position);

			if(owner.size() && group.size())
			{
				FileSetOwnership(level, owner, group);
			}

			if(newPermissions)
			{
				FileSetPermissions(level, newPermissions);
			}
		}
	}
}

int FileSetPermissions(CStdString filename, int permissions)
{
	int res = 0;

#ifndef WIN32
	res = chmod(filename.c_str(), permissions);
#endif

	return res;
}

int FileSetOwnership(CStdString filename, CStdString owner, CStdString group)
{
	int res = 0;

#ifndef WIN32
	struct group fileGroup, *fgP = NULL;
	struct passwd fileUser, *fuP = NULL;
	char infoGroupBuf[4096], infoUserBuf[4096];

	memset(infoGroupBuf, 0, sizeof(infoGroupBuf));
	memset(infoUserBuf, 0, sizeof(infoUserBuf));
	memset(&fileGroup, 0, sizeof(fileGroup));
	memset(&fileUser, 0, sizeof(fileUser));

	if(!getgrnam_r(group.c_str(), &fileGroup, infoGroupBuf, sizeof(infoGroupBuf), &fgP))
	{
		if(!getpwnam_r(owner.c_str(), &fileUser, infoUserBuf, sizeof(infoUserBuf), &fuP))
		{
			if(chown(filename.c_str(), fileUser.pw_uid, fileGroup.gr_gid))
			{
				res = -1;
			}
		}
		else
		{
			res = -1;
		}
	}
	else
	{
		res = -1;
	}
#endif

	return res;
}

static char file_ok_chars[] = "-.0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
static char hex_digits[17] = "0123456789ABCDEF";

void FileEscapeName(CStdString& in, CStdString& out)
{
	// Translates all the characters that are not in file_ok_chars string into %xx sequences
	// %xx specifies the character ascii code in hexadecimal
	out = "";
	for (unsigned int i = 0 ; i<in.size() ; i++)
	{
		if (strchr(file_ok_chars, in.GetAt(i)))
		{
			out += in.GetAt(i);
		}
		else
		{
			out += '%';
			out += hex_digits[((unsigned char) in.GetAt(i)) >> 4];
			out += hex_digits[in.GetAt(i) & 0x0F];
		}
	}
}

bool FileIsExist(CStdString fileName)
{
	ACE_stat info;
 	int file_att = -1;

    file_att = ACE_OS::stat(fileName, &info);
    if(file_att == 0)
    {
		return true;
    }
    else
    {
    	return false;
    }
}

int FileSizeInKb(CStdString fileName)
{
	ACE_stat info;
	int file_att = -1;

	file_att = ACE_OS::stat(fileName, &info);

    if(file_att == 0)
    {
		return ((info.st_size)/1024) ;
    }
    else
    {
    	return 0;
    }
}

//=====================================================
// TcpAddress

void TcpAddress::ToString(CStdString& string)
{
	char szIp[16];
	ACE_OS::inet_ntop(AF_INET, (void*)&ip, szIp, sizeof(szIp));

	string.Format("%s,%u", szIp, port);
}

void TcpAddressList::AddAddress(struct in_addr ip, unsigned short port)
{
	TcpAddress addr;
	addr.ip = ip;
	addr.port = port;
	m_addresses.push_back(addr);
}

bool TcpAddressList::HasAddress(struct in_addr ip, unsigned short port)
{
	for(std::list<TcpAddress>::iterator it = m_addresses.begin(); it != m_addresses.end(); it++)
	{
		if ((unsigned int)((*it).ip.s_addr) == (unsigned int)ip.s_addr  && (*it).port == port)
		{
			return true;
		}
	}
	return false;
}

bool TcpAddressList::HasAddressOrAdd(struct in_addr ip, unsigned short port)
{
	if(HasAddress(ip, port) == false)
	{
		AddAddress(ip, port);
		return false;
	}
	return true;
}

void GetHostFqdn(CStdString& fqdn, int size)
{
#ifndef WIN32	
	char hostname[255];
	struct hostent *hp;
	gethostname(hostname, size);
	hp = gethostbyname(hostname);
	fqdn.Format("%s", hp->h_name);
#endif
}

//=========================
// IpRanges

void IpRanges::Compute()
{
	m_ipRangePrefixes.clear();
	m_ipRangeBitWidths.clear();
	std::list<CStdString>::iterator it;

	for(it = m_asciiIpRanges.begin(); it != m_asciiIpRanges.end(); it++)
	{
		CStdString cidrPrefixLengthString;
		unsigned int cidrPrefixLength = 32;		// by default, x.x.x.x/32
		CStdString cidrIpAddressString;
		struct in_addr cidrIpAddress;
		
		CStdString entry = *it;
		int slashPosition = entry.Find('/');
		if(slashPosition > 0)
		{
			cidrIpAddressString = entry.Left(slashPosition);
			cidrPrefixLengthString = entry.Mid(slashPosition+1);

			bool notAnInt = false;
			try
			{
				cidrPrefixLength = StringToInt(cidrPrefixLengthString);
			}
			catch (...) {notAnInt = true;}
			if(cidrPrefixLength < 1 || cidrPrefixLength > 32 || notAnInt)
			{
				throw (CStdString("IpRanges: invalid CIDR prefix length" + entry));
			}
		}
		else
		{
			cidrIpAddressString = entry;
		}

		if(ACE_OS::inet_aton((PCSTR)cidrIpAddressString, &cidrIpAddress))
		{
			unsigned int rangeBitWidth = 32-cidrPrefixLength;
			unsigned int prefix = ntohl((unsigned int)cidrIpAddress.s_addr) >> (rangeBitWidth);
			m_ipRangePrefixes.push_back(prefix);
			m_ipRangeBitWidths.push_back(rangeBitWidth);
		}
		else
		{
			throw (CStdString("invalid IP range:" + entry));
		}
	}
}

bool IpRanges::Matches(struct in_addr ip)
{
	bool matches = false;
	std::list<unsigned int>::iterator bitWidthIt = m_ipRangeBitWidths.begin();
	std::list<unsigned int>::iterator prefixIt = m_ipRangePrefixes.begin();

	while(prefixIt != m_ipRangePrefixes.end())
	{
		unsigned int bitWidth = *bitWidthIt;
		unsigned int prefix = *prefixIt;
		unsigned int packetSourcePrefix = ntohl((unsigned int)ip.s_addr) >> bitWidth;
		if(packetSourcePrefix == prefix)
		{
			matches = true;
			break;
		}
		prefixIt++; 
		bitWidthIt++;
	}
	return matches;
}

bool IpRanges::Empty()
{
	if(m_asciiIpRanges.empty())
		return true;
	else
		return false;
}

//This maps dynamic payload types >= 96  that are detected in the SDP to our own arbitrary static payload type values
//for example, the opus codec will always be mapped to oreka payload type 60, regardless of the dynamic payload type it is given in a particular SIP session.
int GetOrekaRtpPayloadTypeForSdpRtpMap(CStdString sdp)
{
	CStdString rtpCodec;
	int ret = 0;
	if(sdp.Find("opus") != std::string::npos)
	{
		rtpCodec = "opus";
		ret = 60;
	}
	else if(sdp.Find("AMR/8000") != std::string::npos)
	{
		rtpCodec = "amr-nb";
		ret = 61;
	}
	else if(sdp.Find("AMR-WB") != std::string::npos)
	{
		rtpCodec = "amr-wb";
		ret = 62;
	}
	else if(sdp.Find("iLBC") != std::string::npos) 
	{
		rtpCodec = "ilbc";
		ret = 63;
	} 
	return ret;
}

