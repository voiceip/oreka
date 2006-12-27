#include "Utils.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_arpa_inet.h"

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

//=====================================================
// Network related stuff

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

