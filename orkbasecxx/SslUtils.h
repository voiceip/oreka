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

#ifndef __SSL_UTILS_H__
#define __SSL_UTILS_H__

#include <openssl/ssl.h>


void LogSSLKeys(SSL *s);

#ifdef SUPPORT_TLS_CLIENT
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/deadline_timer.hpp>
typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SSLSocket;

//----------------------------------------------------------------------
// This class manages SSL connections. It uses the boost ASIO SSL library,
// but provides a interface that presents blocking with timeout semantics.
// Largely based on the ASIO examples for SSL and blocking TCP
// http://www.boost.org/doc/libs/1_63_0/doc/html/boost_asio/example/cpp03/ssl/client.cpp
// http://www.boost.org/doc/libs/1_63_0/doc/html/boost_asio/example/cpp03/timeouts/blocking_tcp_client.cpp
class OrkHttpClient::SSL_Session
{
public:
	SSL_Session()
		: deadline_(iosvc), sslcontext(boost::asio::ssl::context::sslv23), session_is_connected(false)
	{
		// infinity--> means don't have running timer
		deadline_.expires_at(boost::posix_time::pos_infin);

		// Start the persistent actor that checks for deadline expiry.
		check_deadline();
	};

	bool SSL_Connect(const std::string& hostname, const int tcpPort, int timeout);
	bool established() { return session_is_connected; }

	bool read_until(boost::asio::streambuf &s, const std::string & delim, int timeout);
	bool read(boost::asio::streambuf &s, boost::system::error_code &ec,  int timeout);
	size_t write(boost::asio::streambuf &buf) { return boost::asio::write(*ssl_socket, buf); };
	void close();

private:

	void check_deadline();
	bool TCP_connect(const std::string& host, const std::string& service, int timeout);
	bool SSL_handshake(const std::string& host, const std::string& service, int timeout);


	boost::asio::io_service  iosvc;
	boost::asio::deadline_timer deadline_;
	boost::asio::ssl::context sslcontext;
	oreka::shared_ptr<SSLSocket> ssl_socket;
	//
	// session_is_connected is used to track whether we think the lower session is connected or
	// not. Sample ASIO code uses "is_open" on the cocket, but that hasn't proved reliable
	// for SSL streams (via ssl_socket->lower_layer().is_open()
	bool session_is_connected;
};
#endif


#endif
