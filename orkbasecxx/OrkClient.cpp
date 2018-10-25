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
	OrkAprSubPool locPool;

	CStdString logMsg;
	response = "";
	CStdString requestDetails;
	requestDetails.Format("timeout:%d http://%s:%d/%s", timeout, hostname, tcpPort, request);
	time_t beginRequestTimestamp = time(NULL);

	apr_status_t rt;
	apr_sockaddr_t* sa;
	apr_socket_t* socket;

	char szTcpPort[10];
	sprintf(szTcpPort, "%d", tcpPort);
	iovec iov[8];
	iov[0].iov_base = (void*)"GET ";
	iov[0].iov_len = 4; // Length of "GET ".
	iov[1].iov_base = (PSTR)(PCSTR)request;
	iov[1].iov_len = request.size();
	iov[2].iov_base = (void*)" HTTP/1.0\r\n";
	iov[2].iov_len = 11;
	iov[3].iov_base = (void*)"Host: ";
	iov[3].iov_len = 6;
	iov[4].iov_base = (PSTR)(PCSTR)hostname;
	iov[4].iov_len = hostname.size();
	iov[5].iov_base = (void*)":";
	iov[5].iov_len = 1;
	iov[6].iov_base = szTcpPort;
	iov[6].iov_len = strlen(szTcpPort);
	iov[7].iov_base = (void*)"\r\n\r\n";
	iov[7].iov_len = 4;

	rt = apr_sockaddr_info_get(&sa, (PCSTR)hostname, APR_INET, tcpPort, 0, AprLp);
	if(rt != APR_SUCCESS){
		logMsg.Format("apr_sockaddr_info_get errno:%s", AprGetErrorMsg(rt));
			LogError(logMsg);
			return false;
	}
	rt = apr_socket_create(&socket, sa->family, SOCK_STREAM,APR_PROTO_TCP, AprLp);
	if(rt != APR_SUCCESS){
		logMsg.Format("apr_socket_create errno:%s", AprGetErrorMsg(rt));
			LogError(logMsg);
			return false;
	}
	if(timeout < 1){timeout = 1;}
	apr_socket_opt_set(socket, APR_SO_NONBLOCK, 0);
	apr_interval_time_t to = timeout*1000*1000;
	apr_socket_timeout_set(socket, to);
	rt = apr_socket_connect(socket, sa);
	if(rt != APR_SUCCESS){
		logMsg.Format("apr_socket_connect connect failed errno:%s", AprGetErrorMsg(rt));
		LogError(logMsg);
		apr_socket_close(socket);
		return false;
	}

	apr_size_t len;
	rt = apr_socket_sendv(socket,iov, 8, &len);
	if(rt != APR_SUCCESS)
	{
		logMsg.Format("apr_socket_sendv  errno=%s %s", AprGetErrorMsg(rt), requestDetails);
		LogError(logMsg);
		apr_socket_close(socket);
		return false;
	}

#define BUFSIZE 4096
	char buf [BUFSIZE];
	CStdString header;
	bool gotHeader = false;
	apr_size_t numReceived = BUFSIZE;
	rt = apr_socket_recv(socket, buf, &numReceived);
	
	while((rt == APR_SUCCESS) && (numReceived > 0) && ((time(NULL) - beginRequestTimestamp) <= timeout))
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
		rt = apr_socket_recv(socket, buf, &numReceived);
	}
	apr_socket_close(socket);

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

bool OrkHttpClient::ExecuteHttpsUrl(const CStdString& request, CStdString& response, const CStdString& hostname, const int tcpPort, int timeout)
{
	OrkAprSubPool locPool;
	CStdString logMsg;
	response = "";
	CStdString requestDetails;
	requestDetails.Format("timeout:%d http://%s:%d/%s", timeout, hostname, tcpPort, request);
	time_t beginRequestTimestamp = time(NULL);

	apr_status_t rt;
	apr_sockaddr_t* sa;
    apr_socket_t* socket;

	char szTcpPort[10];
	sprintf(szTcpPort, "%d", tcpPort);
	iovec iov[8];
	iov[0].iov_base = (void*)"GET ";
	iov[0].iov_len = 4; // Length of "GET ".
	iov[1].iov_base = (PSTR)(PCSTR)request;
	iov[1].iov_len = request.size();
	iov[2].iov_base = (void*)" HTTP/1.0\r\n";
	iov[2].iov_len = 11;
	iov[3].iov_base = (void*)"Host: ";
	iov[3].iov_len = 6;
	iov[4].iov_base = (PSTR)(PCSTR)hostname;
	iov[4].iov_len = hostname.size();
	iov[5].iov_base = (void*)":";
	iov[5].iov_len = 1;
	iov[6].iov_base = szTcpPort;
	iov[6].iov_len = strlen(szTcpPort);
	iov[7].iov_base = (void*)"\r\n\r\n";
	iov[7].iov_len = 4;

	rt = apr_sockaddr_info_get(&sa, (PCSTR)hostname, APR_INET, tcpPort, 0, AprLp);
	if(rt != APR_SUCCESS){
            logMsg.Format("apr_sockaddr_info_get errno:%d", rt);
			LogError(logMsg);
			return false;
    }
    rt = apr_socket_create(&socket, sa->family, SOCK_STREAM,APR_PROTO_TCP, AprLp);
	if(rt != APR_SUCCESS){
            logMsg.Format("apr_socket_create errno:%d", rt);
			LogError(logMsg);
			return false;
    }

	apr_interval_time_t to = timeout*1000*1000;
	apr_socket_opt_set(socket, APR_SO_NONBLOCK, 0);   
    apr_socket_timeout_set(socket, to);

    rt = apr_socket_connect(socket, sa);
    if(rt != APR_SUCCESS){
            logMsg.Format("apr_socket_connect connect failed errno:%d", rt);
			LogError(logMsg);
			apr_socket_close(socket);
			return false;
    }
	if(timeout < 1){timeout = 1;}
	//need to to explicitly set the socket here again to obtain the stage NONBLOCK=0&timeout
	apr_socket_opt_set(socket, APR_SO_NONBLOCK, 0);   
    apr_socket_timeout_set(socket, to);

	SSL_CTX* ctx = OrkOpenSslSingleton::GetInstance()->GetClientCtx();
	if(!ctx){
		logMsg.Format("Failed to create CTX for orkclient");
		LogError(logMsg);
		apr_socket_close(socket);
		return false;
	}
	SSL *ssl;
    ssl = SSL_new(ctx);
	apr_os_sock_t nativeSock;
    apr_os_sock_get(&nativeSock, socket);
	SSL_set_fd(ssl, nativeSock);

	CStdString errstr;
	int timeoutMs = 5000;
	if(OrkSsl_Connect(ssl, timeoutMs, errstr) <= 0)
	{
		logMsg.Format("SSL_connect failed. %s", errstr);
		LogError(logMsg);
		SSL_shutdown(ssl);
		SSL_free(ssl);
		apr_socket_close(socket);
		return false;
	}

	 //Below to set the socket back to NONBLOCK=1 & timeout=0. This necessary to use our timeout mechanism
    apr_socket_opt_set(socket, APR_SO_NONBLOCK, 1);
    apr_socket_timeout_set(socket, to);

	int sslwr = SSL_writev(ssl, iov, 8);
	if(sslwr<= 0){
        int re = SSL_get_error(ssl, sslwr);
        logMsg.Format("SSL_writev failed:%d\n",re);
		LogError(logMsg);
		SSL_shutdown(ssl);
		SSL_free(ssl);
		apr_socket_close(socket);
        return false;
    }

#define BUFSIZE 4096
	char buf [BUFSIZE];
	CStdString header;
	bool gotHeader = false;
	//should we timeout only 100ms here, since there is a while timeout loop?
	int numReceived = OrkSslRead(socket, ssl, buf, BUFSIZE);
	
	while((numReceived > 0) && ((time(NULL) - beginRequestTimestamp) <= timeout))
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
		numReceived = OrkSslRead(socket,ssl, buf, BUFSIZE);
	}
	
	SSL_shutdown(ssl);
	SSL_free(ssl);
	apr_socket_close(socket);

	logMsg.Format("%s:%d response:%s",hostname, tcpPort, response);
	LOG4CXX_DEBUG(m_log, logMsg);
	if(numReceived < 0)
	{
		logMsg.Format("numReceived:%d %s", numReceived, requestDetails);
		LogError(logMsg);
		SSL_shutdown(ssl);
		SSL_free(ssl);
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
	//We will need a TSL config param enable to use ExecuteHttpsUrl
	if (ExecuteUrl(requestString, responseString, hostname, tcpPort, timeout))
	//if(ExecuteHttpsUrl(requestString, responseString, hostname, tcpPort, timeout))
	{
		response.DeSerializeSingleLine(responseString);
		return true;
	}
	return false; 
}

