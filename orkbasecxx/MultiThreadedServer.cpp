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
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
//#include "winsock2.h"
#endif
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationRegistry.hpp>

#include "ObjectFactory.h"
#include "serializers/SingleLineSerializer.h"
#include "serializers/DomSerializer.h"
#include "serializers/UrlSerializer.h"
#include "MultiThreadedServer.h"
#include "EventStreaming.h"
#include "apr_portable.h"


void LogSSLKeys(SSL *s);

#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

log4cxx::LoggerPtr CommandLineServer::s_log;
log4cxx::LoggerPtr HttpServer::s_log;
#ifdef SUPPORT_TLS_SERVER
log4cxx::LoggerPtr HttpsServer::s_log;
#endif
void HandleSslHttpMessage(log4cxx::LoggerPtr s_log, apr_socket_t* sock);
CommandLineServer::CommandLineServer(int port)
{
	s_log = log4cxx::Logger::getLogger("interface.commandlineserver");
	m_port = port;
	OrkAprSingleton* orkAprsingle = OrkAprSingleton::GetInstance();
	m_mp = orkAprsingle->GetAprMp();
	
}

bool CommandLineServer::Initialize()
{
	apr_status_t ret;


    ret = apr_sockaddr_info_get(&m_sockAddr, NULL, APR_INET, m_port, 0, m_mp);
    if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to get sockaddr for commandlineserver");
		return false;
	}
    ret = apr_socket_create(&m_socket, m_sockAddr->family, SOCK_STREAM,APR_PROTO_TCP, m_mp);
    if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to create a socket commandlineserver");
		return false;
	}
    apr_socket_opt_set(m_socket, APR_SO_REUSEADDR, 1);
	apr_socket_opt_set(m_socket, APR_SO_NONBLOCK, 0);
	apr_socket_timeout_set(m_socket, -1);

    ret = apr_socket_bind(m_socket, m_sockAddr);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to bind commandlineserver socket");
		return false;
	}
    ret = apr_socket_listen(m_socket, SOMAXCONN);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to have a listening socket for commandlineserver");
		return false;
	}
	CStdString tcpPortString = IntToString(m_port);
    LOG4CXX_INFO(s_log, CStdString("Started CommandlineServer on port:")+tcpPortString);
	return true;
}

void CommandLineServer::Run()
{
	while(true)
	{
		apr_status_t ret;
		apr_socket_t* incomingSocket;
        ret = apr_socket_accept(&incomingSocket, m_socket,m_mp);
        if (ret != APR_SUCCESS) {
            continue;
        }
 		apr_socket_opt_set(incomingSocket, APR_SO_NONBLOCK, 0);
        apr_interval_time_t timeout =  apr_time_from_sec(5);
        apr_socket_timeout_set(incomingSocket, timeout);
		try{
        	std::thread handler(RoutineSvc, incomingSocket);
        	handler.detach();
		} catch(const std::exception &ex){
			CStdString logMsg;
			logMsg.Format("Failed to start CommandLineServer thread reason:%s",  ex.what());
			LOG4CXX_ERROR(s_log, logMsg);
			continue;
		}
	}

}

void CommandLineServer::RoutineSvc(apr_socket_t* sock)
{
	for (bool active = true;active == true;)
	{
		char buf[2048];
		int i = 0;
		apr_status_t ret;
		apr_size_t sentLen = 3;

		// Display prompt
		char prompt[] = "\r\n>";
		ret = apr_socket_send(sock, prompt, &sentLen);
		if(ret != APR_SUCCESS)
		{
			LOG4CXX_ERROR(s_log, "failed to send line terminated");
			break;
		}
		// Get one command line
		bool foundCRLF = false;
		while(active && !foundCRLF && i<2040)
		{
			apr_size_t size = 2040-i;
			ret = apr_socket_recv(sock, buf+i, &size);
			if (size == 0 || size == -1 || ret != APR_SUCCESS)
			{
				active = false;
			}
			else
			{
				for(int j=0; j<size && !foundCRLF;j++)
				{
					if (buf[i+j] < 32 || buf[i+j] > 126)
					{
							// detected a char that cannot be part of an URL
							LOG4CXX_WARN(s_log, "detected command URL with invalid character(s), ignoring");
							apr_socket_close(sock);
							return;
					}
					else if(buf[i+j] == '\r' || buf[i+j] == '\n')
					{
						foundCRLF = true;
						buf[i+j] = '\0';
						CStdString command(buf);
						try
						{
							CStdString className = SingleLineSerializer::FindClass(command);
							ObjectRef objRef = ObjectFactory::GetSingleton()->NewInstance(className);
							if (objRef.get())
							{
								objRef->DeSerializeSingleLine(command);
								ObjectRef response = objRef->Process();
								CStdString responseString = response->SerializeSingleLine();
								apr_size_t leng = responseString.GetLength();
								ret = apr_socket_send(sock, responseString, &leng);
							}
							else
							{
								CStdString error = "Unrecognized command";
								apr_size_t leng = error.GetLength();
								ret = apr_socket_send(sock, error, &leng);							;
							}
						}
						catch (CStdString& e)
						{	
							apr_size_t leng = e.GetLength();
							ret = apr_socket_send(sock, e, &leng);						;
						}
					}
				}
				i += size;
			}
		}
	}

    apr_socket_close(sock);
}


//==============================================================
#define HTTP_MAX_SESSIONS 200
std::atomic<unsigned int> s_numHttpSessions(0);
std::mutex s_HttpMutex;

HttpServer::HttpServer(int port)
{
	s_log = log4cxx::Logger::getLogger("interface.httpserver");
	m_port = port;
	OrkAprSingleton* orkAprsingle = OrkAprSingleton::GetInstance();
	m_mp = orkAprsingle->GetAprMp();
}

bool HttpServer::Initialize()
{
	apr_status_t ret;

	if(m_port == 0)
	{
		LOG4CXX_INFO(s_log, "HTTP server disabled");
		return false;
	}
	ret = apr_sockaddr_info_get(&m_sockAddr, NULL, APR_INET, m_port, 0, m_mp);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to get sockaddr for http server");
		return false;
	}
	ret = apr_socket_create(&m_socket, m_sockAddr->family, SOCK_STREAM,APR_PROTO_TCP, m_mp);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to create a socket http server");
		return false;
	}
	apr_socket_opt_set(m_socket, APR_SO_REUSEADDR, 1);
	apr_socket_timeout_set(m_socket, -1);

	ret = apr_socket_bind(m_socket, m_sockAddr);
	if(ret != APR_SUCCESS)
	{
		CStdString logMsg;
		logMsg.Format("Failed to bind HTTP server socket %d: rc = %d: %s", m_port, ret, AprGetErrorMsg(ret));
		LOG4CXX_ERROR(s_log, logMsg);
		return false;
	}
	ret = apr_socket_listen(m_socket, SOMAXCONN);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to have a listening socket for http server");
		return false;
	}
	CStdString tcpPortString = IntToString(m_port);
	LOG4CXX_INFO(s_log, CStdString("Started HttpServer on port:")+tcpPortString);

	return true;
}

void HttpServer::Run()
{
	try{
		std::thread httpHandler(&HttpServer::RunHttpServer, this);
		httpHandler.detach();
	} catch(const std::exception &ex){
		CStdString logMsg;
		logMsg.Format("Failed to start RunHttpServer thread reason:%s",  ex.what());
		LOG4CXX_ERROR(s_log, logMsg);
	}
}

void HttpServer::RunHttpServer()
{
	SetThreadName("orka:httpsrv");
	while(true)
	{
		apr_status_t ret;
		apr_socket_t* incomingSocket;

		apr_pool_t* request_pool;
		apr_pool_create(&request_pool, m_mp);
		ret = apr_socket_accept(&incomingSocket, m_socket,request_pool);
		if (ret != APR_SUCCESS) {
			continue;
		}
		apr_interval_time_t timeout =  apr_time_from_sec(2);
		apr_socket_timeout_set(incomingSocket, timeout);
		try{
			if(s_numHttpSessions > HTTP_MAX_SESSIONS){
				LOG4CXX_WARN(s_log, "Closing incoming HTTP request: session limit exceeded.");
				apr_socket_close(incomingSocket);
				apr_pool_destroy(request_pool);
				continue;
			}
			std::thread handler(HandleHttpMessage, incomingSocket, request_pool);
			handler.detach();
		} catch(const std::exception &ex){
			CStdString logMsg;
			logMsg.Format("Failed to start HttpServer thread reason:%s",  ex.what());
			LOG4CXX_ERROR(s_log, logMsg);
			apr_socket_close(incomingSocket);
			apr_pool_destroy(request_pool);
			continue;
		}
	}
}

#ifdef SUPPORT_TLS_SERVER
HttpsServer::HttpsServer()
{
	s_log = log4cxx::Logger::getLogger("interface.tlsserver");
}

bool HttpsServer::Initialize(int port, FN_HandleSslHttpMessage msgThread)
{
	apr_status_t ret;
	CStdString logMsg;

	m_sslPort = port;
	OrkAprSingleton* orkAprsingle = OrkAprSingleton::GetInstance();
	m_mp = orkAprsingle->GetAprMp();
	HandleSslHttpMessageThread = msgThread? msgThread: HandleSslHttpMessage;

	if(m_sslPort == 0)
	{
		LOG4CXX_INFO(s_log, "HTTPS server disabled");
		return false;
	}

	ret = apr_sockaddr_info_get(&m_sslSockAddr, NULL, APR_INET, m_sslPort, 0, m_mp);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to get sockaddr for https server");
		return false;
	}
	ret = apr_socket_create(&m_sslSocket, m_sslSockAddr->family, SOCK_STREAM,APR_PROTO_TCP, m_mp);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to create a socket https server");
		return false;
	}
	apr_socket_opt_set(m_sslSocket, APR_SO_REUSEADDR, 1);
	apr_socket_timeout_set(m_sslSocket, -1);

	ret = apr_socket_bind(m_sslSocket, m_sslSockAddr);
	if(ret != APR_SUCCESS)
	{
		CStdString logMsg;
		logMsg.Format("Failed to bind HTTPS server socket %d: rc = %d: %s", m_sslPort, ret, AprGetErrorMsg(ret));
		LOG4CXX_ERROR(s_log, logMsg);
		return false;
	}
    ret = apr_socket_listen(m_sslSocket, SOMAXCONN);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to have a listening socket for http server");
		return false;
	}
	CStdString sslTcpPortString = IntToString(m_sslPort);
	LOG4CXX_INFO(s_log, CStdString("Started HttpsServer on port:")+sslTcpPortString);

	//We will need a config param enable to activate ssl connection
	if(!OrkOpenSslSingleton::GetInstance()->GetServerCtx()){
		LOG4CXX_ERROR(s_log, "No context for HTTPS server");
		return false;
	}

	logMsg.Format("HttpsServer::Initialize: SSL port == %d", m_sslPort);
	LOG4CXX_INFO(s_log, logMsg);

	return true;
}

void HttpsServer::Run()
{
	if(OrkOpenSslSingleton::GetInstance()->GetServerCtx()){
		try{
		std::thread httpsHandler(&HttpsServer::RunHttpsServer, this);
		httpsHandler.detach();
		} catch(const std::exception &ex){
			CStdString logMsg;
			logMsg.Format("Failed to start RunHttpsServer thread reason:%s",  ex.what());
			LOG4CXX_ERROR(s_log, logMsg);
		}
	}
}


void HttpsServer::RunHttpsServer()
{
	SetThreadName("orka:httpssrv");
	while(true)
	{
		apr_status_t ret;
		apr_pool_t* request_pool;
		apr_pool_create(&request_pool, m_mp);
		apr_socket_t* incomingSocket;
		ret = apr_socket_accept(&incomingSocket, m_sslSocket,request_pool);
		if (ret != APR_SUCCESS) {
			continue;
		}
		apr_interval_time_t timeout =  apr_time_from_sec(2);
		apr_socket_timeout_set(incomingSocket, timeout);	//maybe not neccessary here, since sslread is blocking&timeout=0, we have our own timeout mechanism
		try{
			if(s_numHttpSessions > HTTP_MAX_SESSIONS){
				LOG4CXX_WARN(s_log, "Closing incoming HTTPS request: session limit exceeded.");
				apr_socket_close(incomingSocket);
				apr_pool_destroy(request_pool);
				continue;
			}
			std::thread handler(HandleSslHttpMessageThread, s_log, incomingSocket);
			handler.detach();		
		} catch(const std::exception &ex){
			CStdString logMsg;
			logMsg.Format("Failed to start HttpsServer thread reason:%s",  ex.what());
			LOG4CXX_ERROR(s_log, logMsg);
			apr_socket_close(incomingSocket);
			apr_pool_destroy(request_pool);
			continue;
		}
	}
}
#endif

void HttpServer::HandleHttpMessage(apr_socket_t* sock, apr_pool_t* pool)
{
	HttpCounter threadCounter;
	OrkAprSubPool subPool(pool); // fix scope of pool to this thread
	char buf[2048];
	buf[2047] = '\0';	// security
	SetThreadName("orka:httpreq");

    while(true)
    { 
		apr_size_t size = 2047;
        	apr_status_t ret = apr_socket_recv(sock, buf, &size);
		if (size > 5)
		{
			try
			{
				bool singleLineResp = false;
				int startUrlOffset = 5;		// get rid of "GET /" from Http request, so skip 5 chars
				char* stopUrl = strstr(buf+startUrlOffset, " HTTP");	// detect location of post-URL trailing stuff
				if(!stopUrl)
				{
					LOG4CXX_WARN(s_log, "Malformed http request");
					throw (CStdString("Malformed http request"));							;
				}
				char* singleLine = NULL;
				singleLine = strstr(buf, "text/single-line");
				if(singleLine != NULL){
					singleLineResp = true;
				}

				*stopUrl = '\0';									// Remove post-URL trailing stuff
				int urlLen = stopUrl - buf;
				for(int i = startUrlOffset; i< urlLen; i++)
				{
					if(buf[i] < 33 || buf[i] > 126)
					{
						// detected a char that cannot be part of an URL
						LOG4CXX_WARN(s_log, "detected command URL with invalid character(s), ignoring");
						throw (CStdString("Malformed command URL with invalid character(s)"));		
					}
				}

				CStdString url(buf+startUrlOffset);
				int queryOffset = url.Find("?");
				if (queryOffset	 > 0)
				{
					// Strip beginning of URL in case the command is received as an URL "query" of the form:
					// http://hostname/service/command?type=ping
					url = url.Right(url.size() - queryOffset - 1);
				}

				CStdString className = UrlSerializer::FindClass(url);
				ObjectRef objRef = ObjectFactory::GetSingleton()->NewInstance(className);
				if (objRef.get())
				{
					LOG4CXX_INFO(s_log, "command: " + url);
					objRef->DeSerializeUrl(url);
					ObjectRef response = objRef->Process();

					if(response.get() == NULL)
					{
						LOG4CXX_WARN(s_log, "Command does not return a response:" + className);
						throw (CStdString("Command does not return a response:") + className);
					}
					else
					{
						CStdString httpOkBody;
						CStdString httpOk;

						if(singleLineResp == false){
							DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(XStr("Core").unicodeForm());
							XERCES_CPP_NAMESPACE::DOMDocument* myDoc;
							myDoc = impl->createDocument(
										0,			 // root element namespace URI.
										XStr("response").unicodeForm(),	   // root element name
										0);			 // document type object (DTD).
							response->SerializeDom(myDoc);
							httpOkBody = DomSerializer::DomNodeToString(myDoc);
							myDoc->release(); 
							httpOk = "HTTP/1.0 200 OK\r\nContent-type: text/xml\r\n\r\n";						
						}
						else{
							SingleLineSerializer responseSerializer(response.get());
							httpOkBody = responseSerializer.Serialize();
							httpOk = "HTTP/1.0 200 OK\r\nContent-type: text/single-line\r\n\r\n";
						}
						CStdString singleLineResponseString = response->SerializeSingleLine();
						LOG4CXX_INFO(s_log, "response: " + singleLineResponseString);
						apr_size_t leng = httpOk.GetLength();
						ret = apr_socket_send(sock, httpOk, &leng);
						leng = httpOkBody.GetLength();
						ret = apr_socket_send(sock, httpOkBody, &leng);
					}
				}
				else
				{
					LOG4CXX_WARN(s_log, "Command not found:" + className);
					throw (CStdString("Command not found:") + className);							;
				}

			}
			catch (CStdString &e)
			{
				CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nError\r\n");
				error = error + e + "\r\n";
				apr_size_t leng = error.GetLength();
				ret = apr_socket_send(sock, error, &leng);
			}
			catch(const XMLException& e)
			{
				CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nXML Error\r\n");
				apr_size_t leng = error.GetLength();
				ret = apr_socket_send(sock, error, &leng);
			}
		}
		else
		{
			CStdString notFound("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nNot found\r\n");
			apr_size_t leng = notFound.GetLength();
			ret = apr_socket_send(sock, notFound, &leng);
		}
        break;
    }

    apr_socket_close(sock);
}

#ifdef SUPPORT_TLS_SERVER
void HandleSslHttpMessage(log4cxx::LoggerPtr s_log, apr_socket_t* sock)
{
	HttpCounter threadCounter;
	SetThreadName("orka:httpsreq");

	apr_interval_time_t timeout =  apr_time_from_sec(2);
	apr_status_t ret;
	SSL *ssl;
	ssl = SSL_new(OrkOpenSslSingleton::GetInstance()->GetServerCtx());
	CStdString logMsg;
	//need to to explicitly set the socket here again to obtain the stage NONBLOCK=0
	apr_socket_timeout_set(sock, -1);

	apr_os_sock_t nativeSock;
	ret = apr_os_sock_get(&nativeSock, sock);
	if(ret != APR_SUCCESS){
		LOG4CXX_ERROR(s_log, "Unable to obtain native socket");
	}

	SSL_set_fd(ssl, nativeSock);
	CStdString errstr;
	int timeoutMs = 5000;
	if(OrkSsl_Accept(ssl, timeoutMs, errstr) <= 0)
	{
		logMsg.Format("SSL_accept failed. %s", errstr);
		LOG4CXX_WARN(s_log, logMsg);
		SSL_shutdown(ssl);
		SSL_free(ssl);
		apr_socket_close(sock);
		return;
	}
	LogSSLKeys(ssl);
	//In order to have SslRead/ssl_read blocking with timeout, we need to explicitly set timeout here
	//if having socket blocking,  ssl_read blocking forever, and as the result SslRead blocking forever too
	//set timeout for socket here will NOT have ssl_read blocking with timeout, it made ssl_read return right away, but our SslRead has its own timeout mechanism

	apr_socket_timeout_set(sock, apr_time_from_sec(2));
	char buf[2048];
	buf[2047] = '\0';	// security
	int lenSent;
	while(true)
	{
		int size;
		size = OrkSslRead(sock, ssl, buf, 2048);
		if (size > 5)
		{
			try
			{
				int startUrlOffset = 5;		// get rid of "GET /" from Http request, so skip 5 chars
				char* stopUrl = strstr(buf+startUrlOffset, " HTTP");	// detect location of post-URL trailing stuff
				if(!stopUrl)
				{
					LOG4CXX_WARN(s_log, "Malformed http request");
					throw (CStdString("Malformed http request"));							;
				}
				*stopUrl = '\0';									// Remove post-URL trailing stuff
				int urlLen = stopUrl - buf;
				for(int i = startUrlOffset; i< urlLen; i++)
				{
					if(buf[i] < 33 || buf[i] > 126)
					{
						// detected a char that cannot be part of an URL
						LOG4CXX_WARN(s_log, "detected command URL with invalid character(s), ignoring");
						throw (CStdString("Malformed command URL with invalid character(s)"));		
					}
				}

				CStdString url(buf+startUrlOffset);
				int queryOffset = url.Find("?");
				if (queryOffset	 > 0)
				{
					// Strip beginning of URL in case the command is received as an URL "query" of the form:
					// http://hostname/service/command?type=ping
					url = url.Right(url.size() - queryOffset - 1);
				}

				CStdString className = UrlSerializer::FindClass(url);
				ObjectRef objRef = ObjectFactory::GetSingleton()->NewInstance(className);
				if (objRef.get())
				{
					LOG4CXX_INFO(s_log, "command: " + url);
					objRef->DeSerializeUrl(url);
					ObjectRef response = objRef->Process();

					if(response.get() == NULL)
					{
						LOG4CXX_WARN(s_log, "Command does not return a response:" + className);
						throw (CStdString("Command does not return a response:") + className);
					}
					else
					{
						DOMImplementation* impl =  DOMImplementationRegistry::getDOMImplementation(XStr("Core").unicodeForm());
						XERCES_CPP_NAMESPACE::DOMDocument* myDoc;
						myDoc = impl->createDocument(
									0,			 // root element namespace URI.
									XStr("response").unicodeForm(),	   // root element name
									0);			 // document type object (DTD).
						response->SerializeDom(myDoc);
						CStdString pingResponse = DomSerializer::DomNodeToString(myDoc);

						CStdString httpOk("HTTP/1.0 200 OK\r\nContent-type: text/xml\r\n\r\n");
											CStdString singleLineResponseString = response->SerializeSingleLine();
											LOG4CXX_INFO(s_log, "response: " + singleLineResponseString);
						int leng = httpOk.GetLength();
						OrkSslWrite(sock, ssl, httpOk, leng);
						leng = pingResponse.GetLength();
						OrkSslWrite(sock, ssl, pingResponse, leng);

						myDoc->release(); 
					}
				}
				else
				{
					LOG4CXX_WARN(s_log, "Command not found:" + className);
					throw (CStdString("Command not found:") + className);							;
				}

			}
			catch (CStdString &e)
			{
				CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nError\r\n");
				error = error + e + "\r\n";
				int leng = error.GetLength();
				OrkSslWrite(sock, ssl, error, leng);
			}
			catch(const XMLException& e)
			{
				CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nXML Error\r\n");
				int leng = error.GetLength();
				OrkSslWrite(sock, ssl, error, leng);
			}
		}
		else
		{
			CStdString notFound("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nNot found\r\n");
			int leng = notFound.GetLength();
			OrkSslWrite(sock, ssl, notFound, leng);
		}
		break;
	}
	SSL_shutdown(ssl);
	SSL_free(ssl);
	apr_socket_close(sock);
}
#endif
//==============================================

log4cxx::LoggerPtr EventStreamingServer::s_log;

EventStreamingServer::EventStreamingServer(int port)
{
	s_log = log4cxx::Logger::getLogger("interface.eventstreamingserver");
	m_port = port;
	OrkAprSingleton* orkAprsingle = OrkAprSingleton::GetInstance();
	apr_pool_create(&m_mp,orkAprsingle->GetAprMp());
	
}

bool EventStreamingServer::Initialize()
{
	apr_status_t ret;

    ret = apr_sockaddr_info_get(&m_sockAddr, NULL, APR_INET, m_port, 0, m_mp);
    if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to get sockaddr for eventstreamingserver");
		return false;
	}
    ret = apr_socket_create(&m_socket, m_sockAddr->family, SOCK_STREAM,APR_PROTO_TCP, m_mp);
    if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to create a socket eventstreamingserver");
		return false;
	}
    apr_socket_opt_set(m_socket, APR_SO_REUSEADDR, 1);
	apr_socket_opt_set(m_socket, APR_SO_NONBLOCK, 0);
	apr_socket_timeout_set(m_socket, -1);

    ret = apr_socket_bind(m_socket, m_sockAddr);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to bind eventstreaming server socket");
		return false;
	}
    ret = apr_socket_listen(m_socket, SOMAXCONN);
	if(ret != APR_SUCCESS)
	{
		LOG4CXX_ERROR(s_log, "Failed to have a listening socket for eventstreamingserver");
		return false;
	}
	CStdString tcpPortString = IntToString(m_port);
    LOG4CXX_INFO(s_log, CStdString("Started EventstreamingServer on port:")+tcpPortString);
	return true;
}

void EventStreamingServer::Run()
{
	SetThreadName("orka:eventsrv");
	while(true)
	{
		apr_status_t ret;
		apr_socket_t* incomingSocket;
		apr_pool_t* request_pool;
		apr_pool_create(&request_pool, m_mp);
		ret = apr_socket_accept(&incomingSocket, m_socket, request_pool);
		if (ret != APR_SUCCESS) {
			apr_pool_destroy(request_pool);
			continue;
		}
		apr_socket_opt_set(incomingSocket, APR_SO_NONBLOCK, 0);
		apr_socket_timeout_set(incomingSocket, -1);
		try{
			std::thread handler(StreamingSvc, incomingSocket, request_pool);
			handler.detach();
		} catch(const std::exception &ex){
			CStdString logMsg;
			logMsg.Format("Failed to start StreamingSvc thread reason:%s",  ex.what());
			LOG4CXX_ERROR(s_log, logMsg);
			apr_pool_destroy(request_pool);
			continue;
		}
	}

}

void EventStreamingServer::StreamingSvc(apr_socket_t* sock, apr_pool_t* pool)
{
	char buf[2048];
	CStdString logMsg;
	CStdString sessionId;
	int messagesSent = 0;
	SetThreadName("orka:event");
	OrkAprSubPool subPool(pool); //change lifetime of pool to this request.

	while(true)
	{
		apr_size_t size = 2048;
		apr_status_t ret = apr_socket_recv(sock, buf, &size);
		if(size <= 5)
		{
			CStdString notFound("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nNot found\r\n");
			apr_size_t leng = notFound.GetLength();
			ret = apr_socket_send(sock, notFound, &leng);
			break;
		}

		try
		{
			int startUrlOffset = 5;
			char* stopUrl = strstr(buf+startUrlOffset, " HTTP");

			if(!stopUrl)
			{
				throw (CStdString("Malformed http request"));
			}

			CStdString header;
			struct tm date = {0};
			time_t now = time(NULL);
			CStdString rfc822Date;

			apr_time_t tn = apr_time_now();
			apr_time_exp_t texp;
			apr_time_exp_gmt(&texp, tn);
			rfc822Date.Format("Tue, %.2d Nov %.4d %.2d:%.2d:%.2d GMT", texp.tm_mday, (texp.tm_year+1900), texp.tm_hour, texp.tm_min, texp.tm_sec);
			header.Format("HTTP/1.1 200 OK\r\nLast-Modified:%s\r\nContent-Type:text/plain\r\n\r\n", rfc822Date);

			apr_size_t leng = header.GetLength();
			ret = apr_socket_send(sock, header, &leng);

			time_t startTime = time(NULL);

			sessionId = EventStreamingSingleton::instance()->GetNewSessionId() + " -";
			logMsg.Format("%s Event streaming start", sessionId);
			LOG4CXX_INFO(s_log, logMsg);

			EventStreamingSessionRef session(new EventStreamingSession());
			EventStreamingSingleton::instance()->AddSession(session);
			if(EventStreamingSingleton::instance()->GetNumSessions() > 100)
			{
				EventStreamingSingleton::instance()->RemoveSession(session);
				logMsg.Format("Event streaming number of session exceeds 100. Stopping %s", sessionId);
				LOG4CXX_ERROR(s_log, logMsg);
				break;
			}

			ret=APR_SUCCESS;
			while(ret == APR_SUCCESS)
			{
				session->WaitForMessages();

				while(session->GetNumMessages() && ret == APR_SUCCESS)
				{
					MessageRef message;

					session->GetTapeMessage(message);
					if(message.get())
					{
						CStdString msgAsSingleLineString;
						msgAsSingleLineString = message->SerializeUrl() + "\r\n";

						apr_size_t leng = msgAsSingleLineString.GetLength();
						ret = apr_socket_send(sock, msgAsSingleLineString, &leng);
						if (ret == APR_SUCCESS)
						{
							messagesSent += 1;
						}
						else
						{
							char errstr[256];
							apr_strerror(ret, errstr, sizeof(errstr));

							logMsg.Format("%s Event streaming session error %d: %s", sessionId, ret, errstr);
							LOG4CXX_WARN(s_log, logMsg);
						}
					}
				}
			}

			EventStreamingSingleton::instance()->RemoveSession(session);
			logMsg.Format("%s Stream client stop - sent %d messages in %d sec", sessionId, messagesSent, (time(NULL) - startTime));
			LOG4CXX_INFO(s_log, logMsg);
		}
		catch (CStdString& e)
		{
			CStdString error("HTTP/1.0 404 not found\r\nContent-type: text/html\r\n\r\nError\r\n");
			error = error + e + "\r\n";
			LOG4CXX_ERROR(s_log, e);
			apr_size_t leng = error.GetLength();
			ret = apr_socket_send(sock, error, &leng);
		}
        break;
    }

    apr_socket_close(sock);
}


