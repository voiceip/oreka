#include "Utils.h"
#include "ace/OS_NS_stdio.h"

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
