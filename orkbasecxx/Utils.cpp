#include "Utils.h"
#include "ace/OS_NS_stdio.h"
#include "ace/OS_NS_arpa_inet.h"
#include "ace/OS_NS_sys_stat.h"

#ifndef WIN32
#include <pwd.h>
#include <grp.h>
#endif

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

void FileRecursiveMkdir(CStdString& path)
{
	int position = 0;
	bool done = false;
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

