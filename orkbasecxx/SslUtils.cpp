#include "Utils.h"
#include "OrkClient.h"
#include "SslUtils.h"
#include "ConfigManager.h"
#include "LogManager.h"

#if defined(SUPPORT_TLS_CLIENT) || defined(SUPPORT_TLS_SERVER)
#if OPENSSL_VERSION_NUMBER < 0x10100000L
// in OpenSSL 1.1.0, SSL_SESSION is opaque, and we access the master key in it using a helper function
// For earlier versions, implement that functions.
inline size_t SSL_SESSION_get_master_key(const SSL_SESSION *session, unsigned char *out, size_t outlen)
{
	int len = outlen < 48 ? outlen : 48;
	memcpy(out, session->master_key, len);
	return len;
}
// in OpenSSL 1.1.0, SSL is opaque, and we access the client random key in it using a helper function
// For earlier versions, implement that functions.
inline size_t SSL_get_client_random(const SSL *ssl, unsigned char *out, size_t outlen)
{
	int len = outlen < SSL3_RANDOM_SIZE ? outlen : SSL3_RANDOM_SIZE;
	memcpy(out, ssl->s3->client_random, len);
	return len;
}

#endif
static LoggerPtr s_SSL_Log = Logger::getLogger("orkclient.ssl");

void OrkHttpClient::SSL_Session::check_deadline()
{
	// Check whether the deadline has passed. We compare the deadline against
	// the current time since a new asynchronous operation may have moved the
	// deadline before this actor had a chance to run.
	if (deadline_.expires_at() <= boost::asio::deadline_timer::traits_type::now())
	{
		// The deadline has passed. The socket is closed so that any outstanding
		// asynchronous operations are cancelled. This allows the blocked
		// connect(), read_line() or write_line() functions to return.
		boost::system::error_code ignored_ec;
		close();

		// There is no longer an active deadline. The expiry is set to positive
		// infinity so that the actor takes no action until a new deadline is set.
		deadline_.expires_at(boost::posix_time::pos_infin);
	}

	// Put the actor back to sleep.
	deadline_.async_wait(boost::bind(&OrkHttpClient::SSL_Session::check_deadline, this));
}


void hexEncode(char *out, const unsigned char * in, unsigned int length)
{
	static const char hextable[] = "0123456789abcdef";

	for(int i = 0; i < length; i++) {
		*out++ = hextable[in[i] >> 4];
		*out++ = hextable[in[i] & 0xf];
	}
}

static bool SSL_KeyLogging_init=false;
static FILE *SSL_key_log_fp = NULL;


void SSL_LoggingInit()
{
	CStdString logMsg;

	if(CONFIG.m_tlsClientKeylogFile.length()!=0)
	{
		SSL_key_log_fp=fopen(CONFIG.m_tlsClientKeylogFile,"a");
		if (SSL_key_log_fp == NULL) {
			logMsg.Format("Unable to open SSL key logging file: %s", CONFIG.m_tlsClientKeylogFile);
			LOG4CXX_ERROR(s_SSL_Log, logMsg);
		}
		else {
			logMsg.Format("Logging negotiated SSL keys into file: %s", CONFIG.m_tlsClientKeylogFile);
			LOG4CXX_WARN(s_SSL_Log, logMsg);
		}
	}
	SSL_KeyLogging_init=true;
}

//
// LogSSLKeys borrows most of its logic from Mozilla nss library equivalent
// functions. Except we don't handle RSA pre-master keys. Input is handle to
// OpenSSL session
void LogSSLKeys(SSL *ssl_socket)
{
	char buf[256];

	if(!SSL_KeyLogging_init) SSL_LoggingInit();
	if(SSL_key_log_fp == NULL) return;

	SSL_SESSION *ss=SSL_get_session(ssl_socket);
	unsigned char master_key[48];
	size_t sz= SSL_SESSION_get_master_key(ss,master_key, 48);
	memcpy(buf, "CLIENT_RANDOM ",14);
	unsigned char client_random[SSL3_RANDOM_SIZE];
	SSL_get_client_random(ssl_socket, client_random, SSL3_RANDOM_SIZE);
	int j = 14;
	hexEncode(buf+j, client_random, SSL3_RANDOM_SIZE);
	j += SSL3_RANDOM_SIZE*2;
	buf[j++] = ' ';
	hexEncode(buf+j, master_key, 48);
	j+= 48 * 2;
	buf[j++] = '\n';

	if (fwrite(buf, j, 1, SSL_key_log_fp) <=0) {
		fclose(SSL_key_log_fp);
		SSL_key_log_fp = NULL;
		return;
	}
	fflush(SSL_key_log_fp);

}

//
// callback to verify SSL certificate.
// Used mostly to print out information aboiut the certificate we received from the remote party
// As a special case, if the remote certificate has expired, we will override the verification
// and accept the certificate: this allows OrkAudio to continue to work even when the certificate has expired.
int verify_certificate(log4cxx::LoggerPtr &log, int preverified, boost::asio::ssl::verify_context& ctx)
{
	CStdString logMsg;

	char subject_name[256];
	X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
	X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
	X509_STORE_CTX *cts = ctx.native_handle();
	int cert_error = X509_STORE_CTX_get_error(cts);
	logMsg.Format("Verify Certificate %s. (Depth = %d, Error = %d, preverified = %d)", subject_name, X509_STORE_CTX_get_error_depth(cts), cert_error, preverified);
	LOG4CXX_DEBUG(log, logMsg);


	if (cert_error == X509_V_ERR_CERT_HAS_EXPIRED && CONFIG.m_tlsClientCertCheckIgnoreExpiry)
	{
		X509_STORE_CTX_set_error(cts, 0);
		logMsg.Format("**Ignoring Certificate Expiry** : %s", subject_name);
		LOG4CXX_WARN(log, logMsg);
		return 1;
	}

	if (cert_error) {
		logMsg.Format("Certificate Error: %s: %s", subject_name, X509_verify_cert_error_string(cert_error));
		LOG4CXX_ERROR(log, logMsg);
	}

	return preverified;
}

bool OrkHttpClient::SSL_Session::SSL_Connect(const std::string& hostname, const int tcpPort, int timeout)
{
	CStdString logMsg;
	char port[10];
	sprintf(port, "%i", tcpPort);



	if (!CONFIG.m_tlsClientCertCheckDisable) {
		sslcontext.set_verify_mode(boost::asio::ssl::context::verify_peer | boost::asio::ssl::verify_fail_if_no_peer_cert);
		sslcontext.set_verify_callback(boost::bind(&verify_certificate, s_SSL_Log, _1, _2));

	}
	else {
		FLOG_WARN(LOG.reporting, "%s:%d SSL peer validation is turned off", hostname, tcpPort);
		sslcontext.set_verify_mode(boost::asio::ssl::context::verify_none);
	}


	if (CONFIG.m_tlsClientCertFile.length() != 0) {
		if (FileCanOpen(CONFIG.m_tlsClientCertFile)) {
			sslcontext.load_verify_file(CONFIG.m_tlsClientCertFile);
		}
		else {
			logMsg.Format("Can't open certificate file [%s]", CONFIG.m_tlsClientCertFile);
			LOG4CXX_ERROR(s_SSL_Log, logMsg);
			return false;
		}
	}
	else {
		//if we're not matching a specific cert, use the default system certificates
		sslcontext.set_default_verify_paths();
	}

	ssl_socket.reset(new SSLSocket(iosvc, sslcontext));
	if (!TCP_connect(hostname, port, timeout))
		return false;

	if (!SSL_handshake(hostname, port, timeout))
		return false;

	return true;
}

// callback handler for handshake: pass on any error to the initiator of the handshake
void handshake_handler(const boost::system::error_code& error, boost::system::error_code* out_ec)
{
	*out_ec = error;
}

// callback handler for connect: pass on any error to the initiator of the tcp connection
void handler(const boost::system::error_code& error, boost::asio::ip::tcp::resolver::iterator iterator, boost::system::error_code* out_ec)
{
	*out_ec = error;
}

// callback handler for read: pass on any error to the initiator of the read
void read_handler(const boost::system::error_code& error, std::size_t size, boost::system::error_code* out_ec, std::size_t *out_size)
{
	*out_ec = error;
	*out_size = size;
}


bool OrkHttpClient::SSL_Session::TCP_connect(const std::string& host, const std::string& service, int time)
{
	boost::posix_time::time_duration timeout = boost::posix_time::seconds(time);

	// Resolve the host name and service to a list of endpoints.
	boost::asio::ip::tcp::resolver resolver(iosvc);
	boost::asio::ip::tcp::resolver::query query(host, service);
	boost::asio::ip::tcp::resolver::iterator iter;
	try {
		iter = resolver.resolve(query);
	}
	catch (std::exception& e) {
		CStdString logMsg;
		// log the exception
		// in case of host cannot resolved
		// boost throws an exception
		logMsg.Format("%s:%s DNS %s", host, service, e.what());
		LOG4CXX_ERROR(s_SSL_Log, logMsg);
		return false;
	}


	// Set a deadline for the asynchronous operation. As a host name may
	// resolve to multiple endpoints, this function uses the composed operation
	// async_connect. The deadline applies to the entire operation, rather than
	// individual connection attempts.
	deadline_.expires_from_now(timeout);

	// Set the variable that receives the result of the asynchronous
	// operation. The error code is set to would_block to signal that the
	// operation is incomplete. Asio guarantees that its asynchronous
	// operations will never fail with would_block, so any other value in
	// ec indicates completion.
	boost::system::error_code ec = boost::asio::error::would_block;

	// Start the asynchronous operation itself.
	boost::asio::async_connect(ssl_socket->lowest_layer(), iter, boost::bind(&handler, _1, _2, &ec));

	// set session_is_connected flag to true. If connect fails, this will get
	// reset below. More importantly, if deadline timer fires, it will call
	// close, which will reset the flag. So "session_is_connected" is set here to
	// deal with a race between session completion and timer firing.
	session_is_connected = true;
	// Block until the asynchronous operation has completed.
	do iosvc.run_one(); while (ec == boost::asio::error::would_block);

	// Determine whether a connection was successfully established. The
	// deadline actor may have had a chance to run and close our socket, even
	// though the connect operation notionally succeeded. Therefore we must
	// check whether the socket is still open before deciding if we succeeded
	// or failed.
	if (ec || !established()) {
		CStdString logMsg;
		if (ec == boost::asio::error::operation_aborted)
			logMsg.Format("connect error  %s:%s Timeout.", host, service, ec.value(), ec.message());
		else
			logMsg.Format("connect error  %s:%s errno=%d %s", host, service, ec.value(), ec.message());
		LOG4CXX_ERROR(s_SSL_Log, logMsg);
		close();
		return false;
	}
	return true;

}
bool OrkHttpClient::SSL_Session::SSL_handshake(const std::string& host, const std::string& service, int time)
{
	boost::posix_time::time_duration timeout = boost::posix_time::seconds(time);
	boost::system::error_code ec = boost::asio::error::would_block;

	ssl_socket->async_handshake(boost::asio::ssl::stream_base::client,
		boost::bind(&handshake_handler, _1, &ec)
		);

	// Set a deadline for the asynchronous operation
	deadline_.expires_from_now(timeout);

	// Block until the asynchronous operation has completed.
	do iosvc.run_one(); while (ec == boost::asio::error::would_block);

	// Determine whether a handshake  was successfully completed. The
	// deadline actor may have had a chance to run and close the underlying TCP
	// socket. If the underlying session has disappeared (it had to be up before starting
	// the handshake), then handshake was not successful
	if (ec || !established()) {
		CStdString logMsg;
		if (ec == boost::asio::error::operation_aborted)
			logMsg.Format("%s:%s SSL handshake timeout.", host, service);
		else
			logMsg.Format("%s:%s SSL handshake error: %s", host, service, ec.message());
		LOG4CXX_ERROR(s_SSL_Log, logMsg);
		close();
		return false;
	}

	LogSSLKeys(ssl_socket->native_handle());
	return true;
}

bool OrkHttpClient::SSL_Session::read_until(boost::asio::streambuf &s, const std::string & delim, int time)
{
	boost::posix_time::time_duration timeout = boost::posix_time::seconds(time);
	boost::system::error_code ec = boost::asio::error::would_block;
	size_t bytes_read = 0;

	async_read_until(*ssl_socket, s, delim, boost::bind(&read_handler, _1, _2, &ec, &bytes_read));

	// Set a deadline for the asynchronous operation
	deadline_.expires_from_now(timeout);

	// Block until the asynchronous operation has completed.
	do iosvc.run_one(); while (ec == boost::asio::error::would_block);

	// check for an error, or for the underling tcp session disappearing
	// before determining success
	if (ec || !established()) {
		CStdString logMsg;
		logMsg.Format("SSL session closed by remote");
		LOG4CXX_DEBUG(s_SSL_Log, logMsg);
		close();
		return false;
	}
	return true;
}

bool OrkHttpClient::SSL_Session::read(boost::asio::streambuf &s, boost::system::error_code &ec,  int time)
{
	boost::posix_time::time_duration timeout = boost::posix_time::seconds(time);
	size_t bytes_read = 0;

	ec = boost::asio::error::would_block;
	async_read(*ssl_socket, s, boost::asio::transfer_at_least(1), boost::bind(&read_handler, _1, _2, &ec, &bytes_read));

	// Set a deadline for the asynchronous operation
	deadline_.expires_from_now(timeout);

	// Block until the asynchronous operation has completed.
	do iosvc.run_one(); while (ec == boost::asio::error::would_block);

	// check for an error, or for the underling tcp session disappearing
	// before determining success
	if (ec || !established()) {
		CStdString logMsg;
		logMsg.Format("SSL session closed by remote");
		LOG4CXX_DEBUG(s_SSL_Log, logMsg);
		close();
		return false;
	}
	return true;
}


void OrkHttpClient::SSL_Session::close()
{
	ssl_socket->lowest_layer().close();
	session_is_connected = false;
}

#endif //#if defined(SUPPORT_TLS_CLIENT) || defined(SUPPORT_TLS_SERVER)

