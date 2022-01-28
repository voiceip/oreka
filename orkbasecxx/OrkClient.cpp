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
#include "ConfigManager.h"
#include "OrkClient.h"
#include "LogManager.h"
#include "SslUtils.h"

 
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

#ifdef SUPPORT_TLS_CLIENT
bool OrkHttpClient::SSL_OpenSession(const std::string& hostname, const int tcpPort, int timeout)
{
	CStdString logMsg;
	bool rc = true;

	if (!ssl_session) ssl_session.reset(new SSL_Session);

	if (!ssl_session->established())
	{
		//SSL_Connect will establish a TCP connection, and perform
		//SSL handshake with certificate verification.
		rc = ssl_session->SSL_Connect(hostname, tcpPort, timeout);
	}

	return rc;
}

void OrkHttpClient::SSL_CloseSession()
{
	ssl_session->close();
}

bool OrkHttpClient::SSL_SessionEstablished()
{
	return (ssl_session && ssl_session->established());
}

bool OrkHttpClient::ExecuteSSLRequest(const std::string& request, std::string& responseString, const std::string& hostname, const int tcpPort, int timeout)
{
	CStdString logMsg;
	CStdString requestDetails;

	requestDetails.Format("http://%s:%d/%s", hostname, tcpPort, request);

	if (!SSL_OpenSession(hostname, tcpPort, timeout))
		return false;

	try {
		boost::system::error_code ec;

		boost::asio::streambuf request_buf;
		std::ostream request_stream(&request_buf);

		request_stream << "GET " << request << " HTTP/1.1\r\n";
		request_stream << "Host: " << hostname << "\r\n";
		request_stream << "Accept: */*\r\n";
		request_stream << "Connection: keep-alive\r\n\r\n";

		// Send the request.
		if (m_log->isTraceEnabled())
		{
			boost::asio::streambuf::const_buffers_type bufs = request_buf.data();
			std::string str(boost::asio::buffers_begin(bufs),
			                boost::asio::buffers_begin(bufs) + request_buf.size());
			logMsg.Format("send of %d bytes to %s:%d [%s]",
					request_buf.size(), hostname, tcpPort, str);
			LOG4CXX_TRACE(m_log,logMsg);
		}
		ssl_session->write(request_buf);

		// Read the response
		boost::asio::streambuf response;
		if (!ssl_session->read_until(response, "\r\n", timeout))
			return false; //error logged in read_until

		if (m_log->isTraceEnabled())
		{
			boost::asio::streambuf::const_buffers_type bufs = response.data();
			std::string str(boost::asio::buffers_begin(bufs),
			                boost::asio::buffers_begin(bufs) + response.size());
			logMsg.Format("recv of %d bytes from %s:%d [%s]",
					response.size(), hostname, tcpPort, str);
			LOG4CXX_TRACE(m_log,logMsg);
		}

		// Verify the response
		std::istream response_stream(&response);

		std::string http_version;
		response_stream >> http_version;

		unsigned int status_code;
		response_stream >> status_code;

		if  (!response_stream || (http_version.substr(0, 5) != "HTTP/")) {
			logMsg.Format("Unexpected version [%s] request:%s", http_version, requestDetails);
			LogError(logMsg);
			SSL_CloseSession();
			return false;
		}

		if (status_code != 200) {
			logMsg.Format("Bad status:%d ** request:%s", status_code, requestDetails);
			LogError(logMsg);
			SSL_CloseSession();
			return false;
		}

		boost::system::error_code error;
		std::string line;
		bool readContent=false;

		//
		// read response from remote. We read lines from the response_stream
		// until we have skipped past the response header (blank line). We then
		// take the first line as a response. This works with OrkTrack, but
		// technically we should parse "Content-Length" from the response header
		// and read the stream for that precise number of bytes.
		while (!error)
		{
			bool response_received = false;
			std::istream response_stream(&response);
			while (std::getline(response_stream, line)) {
				if (readContent) {
					responseString += line;
					response_received = true;
					break;
				}
				else if (line == "\r") { // Empty line marks the end of header
					readContent = true;
				}
			}
			if (response_received) break;
			//response stream is empty -- refill it from underlying session
			if (!ssl_session->read(response, error, timeout))
				return false; //error logged in lower routine
			if (m_log->isTraceEnabled())
			{
				boost::asio::streambuf::const_buffers_type bufs = response.data();
				std::string str(boost::asio::buffers_begin(bufs),
				                boost::asio::buffers_begin(bufs) + response.size());
				logMsg.Format("recv of %d bytes from %s:%d [%s]",
						response.size(), hostname, tcpPort, str);
				LOG4CXX_TRACE(m_log,logMsg);
			}
 			response_stream.seekg(0);
		}
		logMsg.Format("%s:%d response:%s",hostname, tcpPort, responseString);
		LOG4CXX_DEBUG(m_log, logMsg);

		if (error) {
			logMsg.Format("Read Error %d: %s", error.value(), error.message());
			LogError(logMsg);
			SSL_CloseSession();
			return false;
		}
		return true;
	}
	catch (std::exception& e) {
		// log the exception -- most should be caught by lower levels
		// but write could potentially have thrown something.
		logMsg.Format("%s:%d exception %s", hostname, tcpPort, e.what());
		LogError(logMsg);
		SSL_CloseSession();
		return false;
	}
}

bool OrkHttpClient::ExecuteSslUrl(const std::string& request, std::string& responseString, const std::string& hostname, const int tcpPort, int timeout)
{
	bool result;
	bool retry=false;

	// If we're using an already established SSL session, remote may have
	// disconnected without our knowledge, so be prepared to retry.
	if (SSL_SessionEstablished()) retry = true;
	result = ExecuteSSLRequest(request, responseString, hostname, tcpPort, timeout);
	if (!result && retry) result = ExecuteSSLRequest(request, responseString, hostname, tcpPort, timeout); //2nd bite at the apple

	return result;
}
#endif

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
		logMsg.Format("error looking up %s: %s", hostname, AprGetErrorMsg(rt));
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
	if(header.size() > 12 && header.GetAt(9) == '4' && header.GetAt(10) == '0' && header.GetAt(11) == '0')
	{
		logMsg.Format("HTTP header:%s ** request:%s\nIgnore this message", header, requestDetails);
		LOG4CXX_ERROR(m_log, logMsg);
		return true;
	}
	if(header.size() < 12 || response.size() <= 0)
	{
		logMsg.Format("HTTP header:%s ** request:%s ** response:%s ** header size:%d  response size:%d", header, requestDetails, response, header.size(), response.size());
		LogError(logMsg);
		return false;
	}
	if(	header.GetAt(9) != '2' ||
		header.GetAt(10) != '0' ||
		header.GetAt(11) != '0'	)
	{
		logMsg.Format("HTTP header:%s ** request:%s", header, requestDetails);
		LogError(logMsg);
		return false;
	}
	return true;
}

bool OrkHttpSingleLineClient::Execute(SyncMessage& request, AsyncMessage& response, const CStdString& hostname,const int tcpPort, const CStdString& serviceName, const int timeout, const bool useHttps)
{
	CStdString requestString = "/" + serviceName + "/command?";
	requestString += request.SerializeUrl();
	CStdString responseString;

	if (useHttps == false)
	{
		if (ExecuteUrl(requestString, responseString, hostname, tcpPort, timeout))
		{
			response.DeSerializeSingleLine(responseString);
			return true;
		}
	}
#ifdef SUPPORT_TLS_CLIENT
	else
	{
		if (ExecuteSslUrl(requestString, responseString, hostname, tcpPort, timeout))
		{
			response.DeSerializeSingleLine(responseString);
			return true;
		}

	}
#endif
	return false; 
}

