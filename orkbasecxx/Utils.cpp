#include "Utils.h"
#include "time.h"
#include <fstream>
#include "AudioCapture.h"
#include "Config.h"
#include "ConfigManager.h"
#ifndef WIN32
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/capability.h>
#else
#include <cctype>  //needed in WIN32 for std::toupper
#endif

//========================================================
//thread_local OrkAprSubPool aprThreadPool;
//#define APR_POOL_MAX_FREE_SIZE (4*1024*1024*1024) 
std::mutex OrkAprSingleton::aprLock;
OrkAprSingleton::OrkAprSingleton()
{
    apr_status_t rt;
    if (apr_initialize() != APR_SUCCESS)
    {
        throw (CStdString("Failed to initialized apr"));
    }
    if(apr_pool_create(&m_aprMp, NULL) != APR_SUCCESS)
    {
        throw (CStdString("Failed to create apr pool"));
    }
    apr_allocator_t* pa = apr_pool_allocator_get(m_aprMp);
    // if(pa)
    // {
    //     apr_allocator_max_free_set(pa, APR_POOL_MAX_FREE_SIZE);
    // }
}
OrkAprSingleton* OrkAprSingleton::m_instance(NULL);
apr_pool_t* OrkAprSingleton::m_aprMp(NULL);
void OrkAprSingleton::Initialize()
{
	if(m_instance == NULL)
    {
        std::lock_guard<std::mutex> lg(aprLock);
        m_instance = new OrkAprSingleton();
    }
}
OrkAprSingleton::~OrkAprSingleton()
{
    apr_pool_destroy(m_aprMp);
}
// std::shared_ptr<OrkAprSingleton> OrkAprSingleton::instance = 0;
// std::shared_ptr<OrkAprSingleton> OrkAprSingleton::GetInstance()
OrkAprSingleton* OrkAprSingleton::GetInstance()
{
    return m_instance;
}

apr_pool_t* OrkAprSingleton::GetAprMp()
{
    return m_aprMp;
}

//========================================================
#ifdef SUPPORT_TLS_SERVER

CStdString SSLErrorQ()
{
	CStdString ErrorQ;
	int rc;
	int n = 0;

	while (rc = ERR_get_error())
	{
		char buf[256];
		if (++n) ErrorQ += CStdString("\n");
		ErrorQ +=CStdString("        ");
		ERR_error_string_n(rc, buf, sizeof(buf)-1);
		ErrorQ.AppendFormat("%s", buf);
	}
	return ErrorQ;
}
OrkOpenSslSingleton::OrkOpenSslSingleton()
{
	s_log = log4cxx::Logger::getLogger("interface.ssl");

	SslInitialize();
	try
	{
		CreateCTXServer();
		ConfigureServerCtx();
	}
	catch (CStdString& e)
	{
		m_serverCtx = NULL;
	}
}

void OrkOpenSslSingleton::SslInitialize()
{
    SSL_load_error_strings();
    SSL_library_init();
    OpenSSL_add_all_algorithms();
}

void OrkOpenSslSingleton::CreateCTXServer()
{
	const SSL_METHOD *method;
	method = SSLv23_server_method();
	m_serverCtx = SSL_CTX_new(method);
	if (!m_serverCtx) {
		LOG4CXX_ERROR(s_log,"Unable to create SSL server context");
		throw (CStdString("Unable to create SSL server context\n"));
	}
}


void OrkOpenSslSingleton::ConfigureServerCtx()
{
	CStdString logMsg;
	SSL_CTX_set_ecdh_auto(m_serverCtx, 1);
	/* Set the key and cert */
	if(SSL_CTX_use_certificate_chain_file(m_serverCtx, CONFIG.m_tlsServerCertPath) <= 0) {
		logMsg.Format("Error with certificate %s: %s",	CONFIG.m_tlsServerCertPath, SSLErrorQ());
		LOG4CXX_ERROR(s_log, logMsg);
		throw (CStdString("Unable to find cert.pem\n"));
	}

	if(SSL_CTX_use_PrivateKey_file(m_serverCtx, CONFIG.m_tlsServerKeyPath, SSL_FILETYPE_PEM) <= 0 ) {
		logMsg.Format("Error with key %s: %s",	CONFIG.m_tlsServerKeyPath, SSLErrorQ());
		LOG4CXX_ERROR(s_log, logMsg);
		throw (CStdString("Unable to find key.pem\n"));
	}

	/* Set to require peer (server) certificate verification */
	SSL_CTX_set_verify(m_serverCtx,SSL_VERIFY_NONE,NULL);
	SSL_CTX_set_verify_depth(m_serverCtx,1);
	unsigned int currentPid = getpid();
	CStdString ssl_session_ctx_id;
	ssl_session_ctx_id.Format("orkaudio-%d", currentPid);
	SSL_CTX_set_session_id_context(m_serverCtx, (unsigned char *)&ssl_session_ctx_id, ssl_session_ctx_id.length());
}

SSL_CTX* OrkOpenSslSingleton::GetServerCtx()
{
	return m_serverCtx;
}
#endif
//========================================================
// String related stuff

bool StringIsDigit(CStdString& string)
{
	int size = string.size();
	for(int i=0; i<size; i++)
	{
		if(isdigit(string.GetAt(i)) == false)
		{
			return false;
		}
	}
	return true;
}

bool StringIsPhoneNumber(CStdString& string)
{
	int size = string.size();
	for(int i=0; i<size; i++)
	{
		char c = string.GetAt(i);
		if(isdigit(c) == false && c != '-' && c != '*' && c != '#' && c != '(' && c != ')' )
		{
			return false;
		}
	}
	return true;
}

bool ChopToken(CStdString &token, CStdString separator, CStdString &s) {
	size_t pos = s.find(separator);
	if (pos != std::string::npos) {
		token = s.substr(0,pos);
		s = s.substr(pos+separator.length());
		return true;
	}
	return false;
}

bool MatchesStringList(CStdString& string, std::list<CStdString>& stringList)
{
	if(string.size() == 0)
	{
		return false;
	}
	for(std::list<CStdString>::iterator it = stringList.begin(); it != stringList.end(); it++)
	{
		CStdString element = *it;

		if(element.CompareNoCase(string) == 0)
		{
			return true;
		}
	}
	return false;
}

CStdString FormatDataSize(unsigned long int size)
{
	CStdString sizeStr;
	double newsize;

	if(size <= 1024)
	{
		sizeStr.Format("%lu Byte(s)", size);
	}
	else if(size > 1024 && size <= 1048576)
	{
		newsize = (double)size / 1024;
		sizeStr.Format("%.2f KByte(s)", newsize);
	}
	else if(size > 1048576 && size <= 1073741824)
	{
		newsize = (double)size / 1048576;
		sizeStr.Format("%.2f MByte(s)", newsize);
	}
	else if(size > 1073741824)
	{
		newsize = (double)size / 1073741824;
		sizeStr.Format("%.2f GByte(s)", newsize);
	}

	return sizeStr;
}

CStdString HexToString(const CStdString& hexInput)
{
   	static const char* const lut = "0123456789ABCDEF";
    size_t len = hexInput.length();
    if (len & 1) 	//odd length
	{
		return "Invalid Hex";
	}

    CStdString output;
    output.reserve(len / 2);
    for (size_t i = 0; i < len; i += 2)
    {
        char a = hexInput.at(i);
        const char* p = std::lower_bound(lut, lut + 16, a);
        if (*p != a) 	//not a hex digit
		{
			continue;
		}

        char b = hexInput.at(i + 1);
        const char* q = std::lower_bound(lut, lut + 16, b);
        if (*q != b) 	//not a hex digit
		{
			continue;
		}

		int asciiVal = ((p - lut) << 4) | (q - lut);
		if(asciiVal  > 47 && asciiVal < 58)		//only take number here
		{
        	output.push_back(((p - lut) << 4) | (q - lut));
		}
		else if(output.length() > 0)
		{
			break;
		}
    }
    return output;

}

CStdString IntUnixTsToString(int ts)
{
	CStdString dateFormat;
	struct tm structTm;
	char calendarDate[80];
	time_t date;
	date = (time_t)ts;
	structTm = *localtime(&date);
	strftime(calendarDate, sizeof(calendarDate), "%a %Y-%m-%d %H:%M:%S %Z", &structTm);
	dateFormat.Format("%s", calendarDate);
	return dateFormat;
}

void StringTokenizeToList(CStdString input, std::list<CStdString>& output)
{
    int pos;
    CStdString token;
    input.TrimLeft();
    input.TrimRight();
    while((pos = input.find(" ")) != std::string::npos)
    {
        token = input.substr(0, pos);
        output.push_back(token);
        input = input.substr(pos+1);
    }
    output.push_back(input);
}

CStdString ReplaceRegexBy(CStdString input, CStdString pattern, CStdString replacedBy)
{
	CStdString output;
	std::regex patternRegex(pattern);
	output = std::regex_replace(input,  patternRegex, replacedBy, std::regex_constants::format_first_only);
	return output;
}

void OrkSleepSec(unsigned int sec)
{
//    std::this_thread::sleep_for(std::chrono::seconds(sec));
    apr_interval_time_t tsleep = sec*1000*1000;
    apr_sleep(tsleep);
}
void OrkSleepMs(unsigned int msec)
{
    // std::this_thread::sleep_for(std::chrono::microseconds(msec));
    apr_interval_time_t tsleep = msec*1000;
    apr_sleep(tsleep);
}
void OrkSleepMicrSec(unsigned int microsec)
{
    // std::this_thread::sleep_for(std::chrono::microseconds(msec));
    apr_interval_time_t tsleep = microsec;
    apr_sleep(tsleep);
}
void OrkSleepNs(unsigned int nsec)
{
    // std::this_thread::sleep_for(std::chrono::nanoseconds(nsec));
}
 
int ork_vsnprintf(char *buf, apr_size_t len, const char *format, ...)
{
va_list ap;
	int ret;
	va_start(ap, format);
	ret = apr_vsnprintf(buf, len, format, ap);
	va_end(ap);
	return ret;
}

CStdString AprGetErrorMsg(apr_status_t ret)
{
	CStdString errorMsg;
	char errStr[256];
	apr_strerror(ret, errStr, 256);
	errorMsg.Format("%s", errStr);
	return errorMsg;
}

CStdString GetRevertedNormalizedPhoneNumber(CStdString input)
{
	CStdString output;
	for(int i = input.length() -1; i >= 0; i--){
		if(input.at(i) != '-' && input.at(i) != '+' && input.at(i) != '(' && input.at(i) != ')' && input.at(i) != ' '){
			output += input.at(i);
		}
	}
	return output;
}

bool CompareNormalizedPhoneNumbers(CStdString input1, CStdString input2)
{
	bool ret = false;
	CStdString normalizedInput1, normalizedInput2;
	normalizedInput1 = GetRevertedNormalizedPhoneNumber(input1);
	normalizedInput2 = GetRevertedNormalizedPhoneNumber(input2);
	int minLen = std::min<size_t>(normalizedInput1.length(), normalizedInput2.length());

	if(minLen < 6){
		return normalizedInput1.Equals(normalizedInput2);
	}
	else{
		return memcmp(&normalizedInput1[0], &normalizedInput2[0], minLen) == 0;
	}

	return ret;
}
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
	apr_status_t ret;
	std::fstream file;
	file.open(path, std::fstream::in);
	if(file.is_open()){
		file.close();
		return true;
	}
	return false;
}

void FileRecursiveMkdir(CStdString& path, int permissions, CStdString owner, CStdString group, CStdString rootDirectory)
{
	OrkAprSubPool locPool;

	int position = 0, newPermissions = permissions;
	bool done = false;

	/*
	 * Create the directories first. We have separated this because
	 * we do not want the introduction of rootDirectory to break
	 * any old functionality.
	 */
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
			apr_status_t ret;
			ret = apr_dir_make(level, APR_OS_DEFAULT, AprLp);
		}
	}

	done = false;
	position = 0;
	if(rootDirectory.size())
	{
	        if(path.Find(rootDirectory) >= 0)
		{
			position = 1 + rootDirectory.size();
	        }
	}
#ifndef WIN32
	if(newPermissions & S_IRUSR)
	{
		newPermissions |= S_IXUSR;
	}

	if(newPermissions & S_IRGRP)
	{
		newPermissions |= S_IXGRP;
	}

	if(newPermissions & S_IROTH)
	{
		newPermissions |= S_IXOTH;
	}

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

			if(owner.size() && group.size())
			{
				FileSetOwnership(level, owner, group);
			}

			if(newPermissions)
			{
				FileSetPermissions(level, newPermissions);
			}
		}
	}
#endif
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

static char file_ok_chars[] = "-.0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
static char hex_digits[17] = "0123456789ABCDEF";

void FileEscapeName(CStdString& in, CStdString& out)
{
	// Translates all the characters that are not in file_ok_chars string into %xx sequences
	// %xx specifies the character ascii code in hexadecimal
	out = "";
	for (unsigned int i = 0 ; i<in.size() ; i++)
	{
		if (strchr(file_ok_chars, in.GetAt(i)))
		{
			out += in.GetAt(i);
		}
		else
		{
			out += '%';
			out += hex_digits[((unsigned char) in.GetAt(i)) >> 4];
			out += hex_digits[in.GetAt(i) & 0x0F];
		}
	}
}

#ifdef WIN32
#define stat _stat
#endif

bool FileIsExist(CStdString fileName)
{
	struct stat finfo;
	if(stat((PCSTR)fileName.c_str(), &finfo) != 0){
		return false;
	}
	else{
		return true;
	}
}

int FileSizeInKb(CStdString fileName)
{
	struct stat finfo;
	if(stat((PCSTR)fileName.c_str(), &finfo) != 0){
		return 0;
	}
	else{
		return finfo.st_size/1024;
	}

}

//=====================================================
const char *inet_ntopV4(int inet, void *srcAddr, char *dst, size_t size)
{
	//AF_INET for now
		unsigned char* src = (unsigned char*)srcAddr;
	static const char fmt[] = "%u.%u.%u.%u";
	char tmp[sizeof "255.255.255.255"];

	if (ork_vsnprintf(tmp, sizeof tmp, fmt, src[0], src[1], src[2], src[3]) >= (int) size) {
		return NULL;
	}

	return strcpy(dst, tmp);
}
#define INADDRSZ         4
int inet_pton4(const char *src, struct in_addr* dstAddr)
{
    unsigned char* dst = (unsigned char*)dstAddr;
	static const char digits[] = "0123456789";
	int saw_digit, octets, ch;
	unsigned char tmp[INADDRSZ], *tp;

	saw_digit = 0;
	octets = 0;
	tp = tmp;
	*tp = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		if ((pch = strchr(digits, ch)) != NULL) {
			unsigned int val = *tp * 10 + (unsigned int) (pch - digits);

			if (val > 255)
				return (0);
			*tp = (unsigned char) val;
			if (!saw_digit) {
				if (++octets > 4)
					return (0);
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
				return (0);
			*++tp = 0;
			saw_digit = 0;
		} else
			return (0);
	}
	if (octets < 4)
		return (0);
	/* bcopy(tmp, dst, INADDRSZ); */
	memcpy(dst, tmp, INADDRSZ);
	return (1);
}
//1:read 0:write
int OrkAprSocketWait(apr_socket_t *sock, int direction)
{
#if defined (WIN32) || defined(WIN64)
	fd_set fdset, *rptr, *wptr;
	apr_os_sock_t socketdes;
	apr_os_sock_get(&socketdes, sock);
	apr_interval_time_t timeout;
	apr_socket_timeout_get(sock, &timeout);
	int rc;
	struct timeval tv, *tvptr;

	FD_ZERO(&fdset);
	FD_SET(socketdes, &fdset);

	if (direction == 1) {
		rptr = &fdset;
		wptr = NULL;
	}
	else { /* APR_WAIT_WRITE */
		rptr = NULL;
		wptr = &fdset;
	}

	if (timeout < 0) {
		tvptr = NULL;
	}
	else {
		/* casts for winsock/timeval definition */
		tv.tv_sec = (long)apr_time_sec(timeout);
		tv.tv_usec = (int)apr_time_usec(timeout);
		tvptr = &tv;
	}
	rc = select(/* ignored */ FD_SETSIZE + 1, rptr, wptr, NULL, tvptr);
	if (rc == SOCKET_ERROR) {
		return apr_get_netos_error();
	}
	else if (!rc) {
		return APR_FROM_OS_ERROR(WSAETIMEDOUT);
	}

	return APR_SUCCESS;
#else
	return apr_wait_for_io_or_timeout(NULL, sock, direction);
#endif
}

#ifndef CENTOS_6
//Note: OrkAprSocketWait use socket timeout, so we need to set it to desire value before invoke this
int OrkSslRead(apr_socket_t* sock, SSL* ssl, char* buf, int len)
{
	apr_interval_time_t timestart = apr_time_now();
	int read;
	OrkAprSocketWait(sock, 1);    //1 is read, 0 is write
	//regardless there is data in the pipe or timeup, we still need to do ssl_read. It will help to detect if connection down
	read = SSL_read(ssl, buf, len); 
	return read;
}
//note:socket timeout should be much shorter than timeoutMs here to minimize delay
void OrkSslRead_n(apr_socket_t* sock, SSL* ssl, char* buf, int len, int64_t timeoutMs, int &lenRead)
{
	apr_interval_time_t timestart = apr_time_now();
	lenRead = 0;
	int read;
	while((apr_time_now() - timestart) < timeoutMs*1000) 
	{
		OrkAprSocketWait(sock, 1);    //1 is read, 0 is write
		//regardless there is data in the pipe or timeup, we still need to do ssl_read. It will help in case of connection down
		read = SSL_read(ssl, buf + lenRead, len - lenRead); 
		if(read <= 0){
		int er = SSL_get_error(ssl, read);
			if(er == SSL_ERROR_SYSCALL){
				lenRead = -1; //to signal the connection is down
				break;
			}
			continue;
		}
		else{
			lenRead += read;
		}
		if(lenRead >= len) break;
	}
}

int OrkSslWrite(apr_socket_t* sock, SSL* ssl, const char* buf, int len)
{
	apr_interval_time_t timestart = apr_time_now();
	int write;
	OrkAprSocketWait(sock, 0);    //1 is read, 0 is write
	//regardless there is data in the pipe or timeup, we still need to do ssl_write. It will help in case of connection down
	write = SSL_write(ssl, buf, len); 
	return write;
}

void OrkSslWrite_n(apr_socket_t* sock, SSL* ssl, const char* buf, int len, int64_t timeoutMs, int &lenWritten)
{
	apr_interval_time_t timestart = apr_time_now();
	lenWritten = 0;
	int write;
	while((apr_time_now() - timestart) < timeoutMs*1000) 
	{
		OrkAprSocketWait(sock, 0);    //1 is read, 0 is write
		//regardless there is data in the pipe or timeup, we still need to do ssl_write. It will help in case of connection down
		write = SSL_write(ssl, buf + lenWritten, len - lenWritten); 
		if(write <= 0){
			int er = SSL_get_error(ssl, write);
			if(er == SSL_ERROR_SYSCALL){
				lenWritten = -1; //to signal the connection is down
				break;
			}
			continue;
		}
		else{
			lenWritten += write;
		}
		if(lenWritten >= len) break;    
	}

}
#endif

//we should set apr_socket timeout very small explicitly before, otherwise timeout would be higher
int OrkRecv_n(apr_socket_t* socket, char* buf, int len, int64_t timeoutMs, int &lenRead)
{
	int ret = 1;
	apr_interval_time_t timestart = apr_time_now();
	lenRead = 0;
	while((apr_time_now() - timestart) < timeoutMs*1000) 
	{
		apr_size_t read = len - lenRead;
		apr_status_t rt = apr_socket_recv(socket, buf+lenRead, &read);
		if(rt == APR_SUCCESS){
			lenRead += read;
		}
		else if(rt == 70007)    //apr socket timeout
		{
			continue;
		}
		else if(rt == APR_EOF)
		{
			ret = -1;
			break;
		}
		else
		{

			break;
		}
		
		if(lenRead >= len) break;
		
	}
	return ret;
}

int OrkSend_n(apr_socket_t* socket, const char* buf, int len, int64_t timeoutMs, int &lenSent)
{
	int ret = 1;
	apr_interval_time_t timestart = apr_time_now();
	lenSent = 0;
	while((apr_time_now() - timestart) < timeoutMs*1000) 
	{
		apr_size_t sent = len - lenSent;
		apr_status_t rt = apr_socket_send(socket, buf+lenSent, &sent);
		if(rt == APR_SUCCESS){
			lenSent += sent;
		}
		else if(rt == 70007)    //apr socket timeout
		{
			continue;
		}
		else if(rt == APR_EOF)
		{
			ret = -1;
			break;
		}
		else
		{
			ret = -1;
			break;
		}
		
		if(lenSent >= len) break;
		
	}
	return ret;
}
#if defined (WIN32) || defined(WIN64)
#define alloca _alloca
#endif 
#ifndef CENTOS_6
int SSL_writev (SSL *ssl, const struct iovec *vector, int count)
{
	char *buffer;
	char *bp;
	size_t bytes, to_copy;
	int i;

	/* Find the total number of bytes to be written.  */
	bytes = 0;
	for (i = 0; i < count; ++i)
	bytes += vector[i].iov_len;

	/* Allocate a temporary buffer to hold the data.  */
	buffer = (char *) alloca (bytes);

	/* Copy the data into BUFFER.  */
	to_copy = bytes;
	bp = buffer;
	for (i = 0; i < count; ++i)
	{
	#     define min(a, b)		((a) > (b) ? (b) : (a))
		size_t copy = min (vector[i].iov_len, to_copy);

		memcpy ((void *) bp, (void *) vector[i].iov_base, copy);
		bp += copy;

		to_copy -= copy;
		if (to_copy == 0)
		break;
	}

	return SSL_write (ssl, buffer, bytes);
}

int OrkSsl_Connect(SSL* ssl, int timeoutMs, CStdString &errstr)
{
	int r = 0;
	apr_interval_time_t timestart = apr_time_now();
	ERR_clear_error();
	while((r = SSL_connect(ssl)) <=0)
	{
		int er = SSL_get_error(ssl, r);
		if(er == SSL_ERROR_WANT_READ || er== SSL_ERROR_WANT_WRITE)
		{
			OrkSleepMs(100);
		}
		else
		{
			char errorstr[256];           
			int error;
			while((error = ERR_get_error()) != 0){
				memset(errorstr, 0, 256);
				ERR_error_string_n(error, errorstr, 256);
				CStdString tmpstr;
				tmpstr.Format("errno:%d errstr:%s\n", r, errorstr);
				errstr = errstr + tmpstr;
			} 
			break;
		}
		
		if((apr_time_now() - timestart) > timeoutMs*1000)
		{
			break;
		}
	}
	return r;
}

int OrkSsl_Accept(SSL* ssl, int timeoutMs, CStdString &errstr)
{
	int r = 0;
	apr_interval_time_t timestart = apr_time_now();
	ERR_clear_error();
	while((r = SSL_accept(ssl)) <=0)
	{
		int er = SSL_get_error(ssl, r);
		if(er == SSL_ERROR_WANT_READ || er== SSL_ERROR_WANT_WRITE)
		{
			OrkSleepMs(100);
		}
		else
		{
			char errorstr[256];           
			int error;
			while((error = ERR_get_error()) != 0){
				memset(errorstr, 0, 256);
				ERR_error_string_n(error, errorstr, 256);
				CStdString tmpstr;
				tmpstr.Format("errno:%d errstr:%s\n", r, errorstr);
				errstr = errstr + tmpstr;
			}            
			break;
		}
		
		if((apr_time_now() - timestart) > timeoutMs*1000)
		{
			break;
		}
	}
	return r;
}
#endif

// TcpAddress

void TcpAddress::ToString(CStdString& string)
{
	char szIp[16];
	inet_ntopV4(AF_INET, (void*)&ip, szIp, sizeof(szIp));

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

void GetHostFqdn(CStdString& fqdn, int size)
{
#ifndef WIN32	
	char hostname[255];
	struct hostent *hp;
	gethostname(hostname, size);
	hp = gethostbyname(hostname);
	fqdn.Format("%s", hp->h_name);
#endif
}

//=========================
// IpRanges

void IpRanges::Compute()
{
	m_ipRangePrefixes.clear();
	m_ipRangeBitWidths.clear();
	std::list<CStdString>::iterator it;

	for(it = m_asciiIpRanges.begin(); it != m_asciiIpRanges.end(); it++)
	{
		CStdString cidrPrefixLengthString;
		unsigned int cidrPrefixLength = 32;		// by default, x.x.x.x/32
		CStdString cidrIpAddressString;
		struct in_addr cidrIpAddress;
		
		CStdString entry = *it;
		int slashPosition = entry.Find('/');
		if(slashPosition > 0)
		{
			cidrIpAddressString = entry.Left(slashPosition);
			cidrPrefixLengthString = entry.Mid(slashPosition+1);

			bool notAnInt = false;
			try
			{
				cidrPrefixLength = StringToInt(cidrPrefixLengthString);
			}
			catch (...) {notAnInt = true;}
			if(cidrPrefixLength < 1 || cidrPrefixLength > 32 || notAnInt)
			{
				throw (CStdString("IpRanges: invalid CIDR prefix length" + entry));
			}
		}
		else
		{
			cidrIpAddressString = entry;
		}

		if(inet_pton4((PCSTR)cidrIpAddressString, &cidrIpAddress))
		{
			unsigned int rangeBitWidth = 32-cidrPrefixLength;
			unsigned int prefix = ntohl((unsigned int)cidrIpAddress.s_addr) >> (rangeBitWidth);
			m_ipRangePrefixes.push_back(prefix);
			m_ipRangeBitWidths.push_back(rangeBitWidth);
		}
		else
		{
			throw (CStdString("invalid IP range:" + entry));
		}
	}
}

bool IpRanges::Matches(struct in_addr ip)
{
	bool matches = false;
	std::list<unsigned int>::iterator bitWidthIt = m_ipRangeBitWidths.begin();
	std::list<unsigned int>::iterator prefixIt = m_ipRangePrefixes.begin();

	while(prefixIt != m_ipRangePrefixes.end())
	{
		unsigned int bitWidth = *bitWidthIt;
		unsigned int prefix = *prefixIt;
		unsigned int packetSourcePrefix = ntohl((unsigned int)ip.s_addr) >> bitWidth;
		if(packetSourcePrefix == prefix)
		{
			matches = true;
			break;
		}
		prefixIt++; 
		bitWidthIt++;
	}
	return matches;
}

bool IpRanges::Empty()
{
	if(m_asciiIpRanges.empty())
		return true;
	else
		return false;
}

// ciFind: case insensitive find helper function
// we're using this for codecs, which are ASCII, so we don't
// need to worry about unicode and/or locales
size_t ciFind(const std::string &Haystack, const std::string &Needle)
{
	auto it = std::search(
			Haystack.begin(), Haystack.end(),
			Needle.begin(),   Needle.end(),
			[](char ch1, char ch2) { return std::toupper(ch1) == std::toupper(ch2); }
	);
	if ( it != Haystack.end() ) return it - Haystack.begin();
	else return std::string::npos; // not found
}
//This maps dynamic payload types >= 96  that are detected in the SDP to our own
//arbitrary internal payload values. For example, the opus codec will always be
//mapped to oreka payload type 60, regardless of the dynamic payload
//type it is given in a particular SIP session.
int GetOrekaRtpPayloadTypeForSdpRtpMap(CStdString sdp)
{
	CStdString rtpCodec;
	int ret = 0;
	if(ciFind(sdp, "opus") != std::string::npos)
	{
		rtpCodec = "opus";
		ret = pt_OPUS;
	}
	else if(ciFind(sdp, "AMR/8000") != std::string::npos)
	{
		rtpCodec = "amr-nb";
		ret = pt_AMRNB;
	}
	else if(ciFind(sdp, "AMR-WB") != std::string::npos)
	{
		rtpCodec = "amr-wb";
		ret = pt_AMRWB;
	}
	else if(ciFind(sdp, "iLBC") != std::string::npos)
	{
		rtpCodec = "ilbc";
		ret = pt_ILBC;
	} 
	else if(ciFind(sdp, "SILK/8000") != std::string::npos)
	{
		rtpCodec = "silk";
		ret = pt_SILK;
	}
	else if(ciFind(sdp, "SILK/16000") != std::string::npos)
	{
		rtpCodec = "silk";
		ret = pt_SILK;
	}
	else if(ciFind(sdp, "speex") != std::string::npos)
	{
		rtpCodec = "speex";
		ret = pt_SPEEX;
	}
	else if(ciFind(sdp, "telephone-event") != std::string::npos)
	{
		rtpCodec = "telephone-event";
		ret = pt_TEL_EVENT;
	}
	else if(ciFind(sdp, "AAL2-G726-32") != std::string::npos)
	{
		//No support for AAL2-G726-32 (big endian ordering),
		// However we need to match against against this string or 
		// else the next check below (G726-32) will match.
		ret = 0;  //0 --> no match;
	}
	else if(ciFind(sdp, "G726-32") != std::string::npos)
	{
		rtpCodec = "G726-32";
		ret = 2;
	}
	return ret;
}


int OrkGetHostname(char *name, int len)
{
	OrkAprSubPool locPool;

	return apr_gethostname(name,len,AprLp);
}

//
// apr routines to set socket option do not support SO_RCVBUFFORCE, so we
// need our own helper funtion.
void set_socket_buffer_size(log4cxx::LoggerPtr log, const char *msg, apr_socket_t *sock, int size)
{
	CStdString logMsg;
	apr_os_sock_t socket;
	apr_os_sock_get(&socket, sock); //always returns success

	if (socket == -1)
	{
		logMsg.Format("Error trying to get OS socket[%s]", msg);
		LOG4CXX_ERROR(log, logMsg);
		return;
	}
#ifndef WIN32
	int rc = setsockopt(socket, SOL_SOCKET, SO_RCVBUFFORCE, (const char *)&size, sizeof(size));
#else
	int rc = setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (const char *)&size, sizeof(size));
#endif
	if (rc < 0)
	{
		logMsg.Format("[%s]: Error setting socket buffer size to %d: %s",
				msg, size, strerror(errno));
		LOG4CXX_ERROR(log, logMsg);
	}
	int socketsize;
	socklen_t len = sizeof(socketsize);
	getsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char *)&socketsize, &len);
	logMsg.Format("[%s] Request socket buffer size of %d; actual size = %d", msg, size, socketsize);
	LOG4CXX_INFO(log, logMsg);
}

CStdString RtpPayloadTypeEnumToString(char pt)
{
	CStdString ptStr = "unknown";
	switch(pt)
	{
		case pt_PCMU: ptStr = "PCMU";
			break;
		case 2:ptStr = "G721/G726-32";
			break;
		case pt_GSM: ptStr = "GSM";
			break;
		case pt_G723: ptStr = "G723";
			break;
		case pt_PCMA: ptStr = "PCMA";
			break;
		case pt_G722: ptStr = "G722";
			break;
		case pt_G729: ptStr = "G729";
			break;
		case pt_OPUS: ptStr = "opus";
			break;
		case pt_AMRNB: ptStr = "AMR-NB";
			break;
		case pt_AMRWB: ptStr = "AMR-WB";
			break;
		case pt_ILBC: ptStr = "iLBC";
			break;
		case pt_SILK: ptStr = "SILK";
			break;
		case pt_SPEEX: ptStr = "speex";
			break;
		case pt_TEL_EVENT: ptStr = "telephone-event";
			break;
		// ==== unsupported codecs
		// the following codecs are not supported, but included for
		// logging purposes
		case 1:
		case 19:
			ptStr = "reserved";
			break;
		case 5:
		case 6:
		case 16:
		case 17:
			ptStr = "DVI4";
			break;
		case 7:
			ptStr = "LPC";
			break;
		case 10:
		case 11:
			ptStr = "L16";
			break;
		case 12:
			ptStr = "QCELP";
			break;
		case 13:
			ptStr = "CN";
			break;
		case 14:
			ptStr = "MPA";
			break;
		case 15:
			ptStr = "G728";
			break;
		case 25:
			ptStr = "CELB";
			break;
		case 26:
			ptStr = "JPEG";
			break;
		case 28:
			ptStr = "nv";
			break;
		case 32:
			ptStr = "MPV";
			break;
		case 33:
			ptStr = "MP2T";
			break;
		case 34:
			ptStr = "H263";
			break;
		default: ptStr = "unknown";
	}
	return ptStr;

}

#ifndef WIN32
void check_pcap_capabilities(log4cxx::LoggerPtr log)
{
	bool rc = true;

	cap_t caps;
	caps=cap_get_pid(getpid());

	cap_flag_value_t cap_val;
	int ret;

	// Do we actually need to check effective AND permitted?
	ret=cap_get_flag(caps, CAP_NET_RAW, CAP_EFFECTIVE, &cap_val);
	if (ret || !cap_val) rc=false;
	ret=cap_get_flag(caps, CAP_NET_RAW, CAP_PERMITTED, &cap_val);
	if (ret || !cap_val) rc=false;
	ret=cap_get_flag(caps, CAP_NET_ADMIN, CAP_EFFECTIVE, &cap_val);
	if (ret || !cap_val) rc=false;
	ret=cap_get_flag(caps, CAP_NET_ADMIN, CAP_PERMITTED, &cap_val);
	if (ret || !cap_val) rc=false;

	if (rc == false)
	{
		LOG4CXX_ERROR(log,"Do not have the necessary privileges to capture data. "
				"Probable fix: use \"setcap\" to add necessary capabilities: "
				"\"setcap cap_net_raw,cap_net_admin+ep /usr/sbin/orkaudio\""
		);
	}
}
#endif
