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

#include "Utils.h"
#include "OrkClient.h"
#include "ace/INET_Addr.h"
#include "ace/SOCK_Connector.h"
#include "ace/SOCK_Stream.h"
 
#ifdef WIN32
	#define IOV_TYPE char*
#else
	#define IOV_TYPE void*	
#endif

bool OrkHttpClient::ExecuteUrl(CStdString& request, CStdString& response, CStdString& hostname, int tcpPort, int timeout)
{
	ACE_SOCK_Connector  connector;
	ACE_SOCK_Stream peer;
	ACE_INET_Addr peer_addr;
	if(timeout < 1){timeout = 1;}
	ACE_Time_Value aceTimeout (timeout);
	time_t beginRequestTimestamp = time(NULL);

	char szTcpPort[10];
	sprintf(szTcpPort, "%d", tcpPort);
	iovec iov[8];
	iov[0].iov_base = (IOV_TYPE)"GET ";
	iov[0].iov_len = 4; // Length of "GET ".
	iov[1].iov_base = (PSTR)(PCSTR)request;
	iov[1].iov_len = request.size();
	iov[2].iov_base = (IOV_TYPE)" HTTP/1.0\r\n";
	iov[2].iov_len = 11;
	iov[3].iov_base = (IOV_TYPE)"Host: ";
	iov[3].iov_len = 6;
	iov[4].iov_base = (PSTR)(PCSTR)hostname;
	iov[4].iov_len = hostname.size();
	iov[5].iov_base = (IOV_TYPE)":";
	iov[5].iov_len = 1;
	iov[6].iov_base = szTcpPort;
	iov[6].iov_len = strlen(szTcpPort);
	iov[7].iov_base = (IOV_TYPE)"\r\n\r\n";
	iov[7].iov_len = 4;

	if (peer_addr.set (tcpPort, (PCSTR)hostname) == -1)
	{
		return false;
	}
	else if (connector.connect (peer, peer_addr, &aceTimeout) == -1)
	{
		if (errno == ETIME)
		{
		}
		return false;
	}
	else if (peer.sendv_n (iov, 8, &aceTimeout) == -1)
	{
		return false;
	}

	ssize_t numReceived = 0;
#define BUFSIZE 4096
	char buf [BUFSIZE];

	CStdString header;
	bool gotHeader = false;
	while ( ((numReceived = peer.recv (buf, BUFSIZE, &aceTimeout)) > 0)  && ((time(NULL) - beginRequestTimestamp) <= timeout) )
	{
		for(int i=0; i<numReceived; i++)
		{
			if(!gotHeader)
			{
				// extract header (delimited by CR-LF-CR-LF)
				header += buf[i];
				size_t headerSize = header.size();
				if (headerSize > 4 &&
					header.GetAt(headerSize-1) == '\n' && 
					header.GetAt(headerSize-2) == '\r' &&
					header.GetAt(headerSize-3) == '\n' &&
					header.GetAt(headerSize-4) == '\r'		)
				{
					gotHeader = true;
				}
			}
			else
			{
				// extract content
				response += buf[i];
			}
		}
	}
	peer.close();

	if(numReceived < 0)
	{
		return false;
	}
	if(header.size() <= 0 || response.size() <= 0)
	{
		return false;
	}
	if(	header.GetAt(10) != '2' &&
		header.GetAt(11) != '0' &&
		header.GetAt(12) != '0' &&
		header.GetAt(13) != ' ' &&
		header.GetAt(14) != 'O' &&
		header.GetAt(15) != 'K'		)
	{
		return false;
	}
	return true;
}

bool OrkHttpSingleLineClient::Execute(SyncMessage& request, AsyncMessage& response, CStdString& hostname, int tcpPort, CStdString& serviceName, int timeout)
{
	CStdString requestString = "/" + serviceName + "/command?";
	requestString += request.SerializeUrl();
	CStdString responseString;
	if (ExecuteUrl(requestString, responseString, hostname, tcpPort, timeout))
	{
		response.DeSerializeSingleLine(responseString);
		return true;
	}
	return false; 
}

