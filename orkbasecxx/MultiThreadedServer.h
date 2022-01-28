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

#ifndef __MULTITHREADEDSERVER_H__
#define __MULTITHREADEDSERVER_H__

#include <log4cxx/logger.h>

#include "OrkBase.h"
#include "dll.h"
#include "Utils.h"
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/err.h>

/** This server accepts permanent telnet like connections.
	commands are accepted in "single line" format.
	one thread per connection
*/
class DLL_IMPORT_EXPORT_ORKBASE CommandLineServer
{
public:
	CommandLineServer(int port);
	bool Initialize();
	void Run();


private:
	apr_pool_t* m_mp;
	int m_port;
	apr_socket_t* m_socket;
    apr_sockaddr_t* m_sockAddr;
	static log4cxx::LoggerPtr s_log;
	static void RoutineSvc(apr_socket_t* sock);
};

/** This server is a lightweight http server that extracts commands from URLs and outputs results in xml format
	one thread per connection
	Example url:
	http://localhost:23000/message=print&text=hello
*/
class DLL_IMPORT_EXPORT_ORKBASE HttpServer
{
public:
	HttpServer(int port);
	bool Initialize();
	void Run();
	void RunHttpServer();

private:
	apr_pool_t* m_mp;
	int m_port;
	apr_socket_t* m_socket;
	apr_socket_t* m_sslSocket;
    apr_sockaddr_t* m_sockAddr;
	static log4cxx::LoggerPtr s_log;
	static void HandleHttpMessage(apr_socket_t* sock, apr_pool_t* pool);

};

#ifdef SUPPORT_TLS_SERVER
typedef void FN_HandleSslHttpMessage(log4cxx::LoggerPtr s_log, apr_socket_t* sock);

class DLL_IMPORT_EXPORT_ORKBASE HttpsServer
{
public:
	HttpsServer();
	bool Initialize(int port, FN_HandleSslHttpMessage msgThread=NULL);
	void Run();
	void RunHttpsServer();

private:
	apr_pool_t* m_mp;
	apr_socket_t* m_sslSocket;
	int m_sslPort;
	apr_sockaddr_t* m_sslSockAddr;
	static log4cxx::LoggerPtr s_log;
	FN_HandleSslHttpMessage* HandleSslHttpMessageThread;
};

class OrekaSslCtx
{
public:
	OrekaSslCtx(SSL_CTX* ctx) { ssl = SSL_new(ctx); };
	~OrekaSslCtx() { SSL_shutdown(ssl); SSL_free(ssl); };

	SSL* SslCtx() { return ssl; };
private:
	SSL *ssl;
};
#endif //#ifndef CENTOS_6

//==========================================================

/** This server is a lightweight http server that prints out the single line
    format of all events from a given port, one thread per connection e.g
	http://localhost:23000/message=streamevents
*/
class DLL_IMPORT_EXPORT_ORKBASE EventStreamingServer
{
public:
	EventStreamingServer(int port);
	~EventStreamingServer() { apr_pool_destroy(m_mp);};
	bool Initialize();
	void Run();


private:
	apr_pool_t* m_mp;
	int m_port;
	apr_socket_t* m_socket;
	apr_sockaddr_t* m_sockAddr;
	static log4cxx::LoggerPtr s_log;
	static void StreamingSvc(apr_socket_t* sock, apr_pool_t* pool);
};

extern DLL_IMPORT_EXPORT_ORKBASE std::atomic<unsigned int> s_numHttpSessions;
extern DLL_IMPORT_EXPORT_ORKBASE std::mutex s_HttpMutex;

class HttpCounter
{
public:
	HttpCounter() { std::lock_guard<std::mutex> lk(s_HttpMutex); s_numHttpSessions++; }
	~HttpCounter() {std::lock_guard<std::mutex> lk(s_HttpMutex); s_numHttpSessions--; }
};


#endif

