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
#include "LogManager.h"
 
#ifdef WIN32
	#define IOV_TYPE char*
#else
	#define IOV_TYPE void*	
#endif

time_t OrkClient::s_lastErrorReportedTime = 0;

OrkClient::OrkClient()
{
	m_log = OrkLogManager::Instance()->clientLog;
}

void OrkClient::LogError(CStdString& logMsg)
{
	if((time(NULL) - s_lastErrorReportedTime) > 60)
	{
		s_lastErrorReportedTime = time(NULL);
		LOG4CXX_ERROR(m_log, logMsg);
	}
}

bool OrkHttpClient::ExecuteUrl(const CStdString& request, CStdString& response, const CStdString& hostname, const int tcpPort, int timeout)
{
	CStdString logMsg;
	response = "";
	ACE_SOCK_Connector  connector;
	ACE_SOCK_Stream peer;
	ACE_INET_Addr peer_addr;
	if(timeout < 1){timeout = 1;}
	ACE_Time_Value aceTimeout (timeout);
	CStdString requestDetails;
	requestDetails.Format("timeout:%d http://%s:%d/%s", timeout, hostname, tcpPort, request);
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
		logMsg.Format("peer_addr.set()  errno=%d %s", errno, requestDetails);
		LogError(logMsg);
		return false;
	}
	else if (connector.connect (peer, peer_addr, &aceTimeout) == -1)
	{
		if (errno == ETIME)
		{
		}
		logMsg.Format("connector.connect()  errno=%d %s", errno, requestDetails);
		LogError(logMsg);
		return false;
	}
	else if (peer.sendv_n (iov, 8, &aceTimeout) == -1)
	{
		logMsg.Format("peer.sendv_n  errno=%d %s", errno, requestDetails);
		LogError(logMsg);
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
	
	logMsg.Format("%s:%d response:%s",hostname, tcpPort, response);
	LOG4CXX_DEBUG(m_log, logMsg);
	if(numReceived < 0)
	{
		logMsg.Format("numReceived:%d %s", numReceived, requestDetails);
		LogError(logMsg);
		return false;
	}
	if(header.size() > 15 && header.GetAt(9) == '4' && header.GetAt(10) == '0' && header.GetAt(11) == '0')
	{
		logMsg.Format("HTTP header:%s ** request:%s\nIgnore this message", header, requestDetails);
		LOG4CXX_ERROR(m_log, logMsg);
		return true;
	}
	if(header.size() < 15 || response.size() <= 0)
	{
		logMsg.Format("HTTP header:%s ** request:%s ** response:%s ** header size:%d  response size:%d", header, requestDetails, response, header.size(), response.size());
		LogError(logMsg);
		return false;
	}
	if(	header.GetAt(9) != '2' ||
		header.GetAt(10) != '0' ||
		header.GetAt(11) != '0' ||
		header.GetAt(12) != ' ' ||
		header.GetAt(13) != 'O' ||
		header.GetAt(14) != 'K'		)
	{
		logMsg.Format("HTTP header:%s ** request:%s", header, requestDetails);
		LogError(logMsg);
		return false;
	}
	return true;
}

bool OrkHttpSingleLineClient::Execute(SyncMessage& request, AsyncMessage& response, const CStdString& hostname,const int tcpPort, const CStdString& serviceName, const int timeout)
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

