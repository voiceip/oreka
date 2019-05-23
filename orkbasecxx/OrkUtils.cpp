
#include "OrkUtils.h" 

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
void OrkSleepNs(unsigned int nsec)
{
    // std::this_thread::sleep_for(std::chrono::nanoseconds(nsec));
}
 
#define APR_POOL_MAX_FREE_SIZE (4*1024*1024*1024) 
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
    if(pa)
    {
        apr_allocator_max_free_set(pa, APR_POOL_MAX_FREE_SIZE);
    }
}
OrkAprSingleton::~OrkAprSingleton()
{
    apr_pool_destroy(m_aprMp);
}
// std::shared_ptr<OrkAprSingleton> OrkAprSingleton::instance = 0;
// std::shared_ptr<OrkAprSingleton> OrkAprSingleton::GetInstance()
OrkAprSingleton* OrkAprSingleton::instance = NULL;
OrkAprSingleton* OrkAprSingleton::GetInstance()
{
    if(instance == NULL)
    {
        //instance = std::shared_ptr<OrkAprSingleton>(new OrkAprSingleton());
        instance = new OrkAprSingleton();
    }
    return instance;
}

apr_pool_t* OrkAprSingleton::GetAprMp()
{
    return m_aprMp;
}

OrkTcpSocket::OrkTcpSocket(CStdString addr, int port, int timeout)
{
    OrkAprSingleton* orkAprsingle = OrkAprSingleton::GetInstance();
	m_mp = orkAprsingle->GetAprMp();
}

OrkTcpSocket::OrkTcpSocket(int port, int timeoutMs)
{
    m_isValid = true;
    apr_status_t ret;
    OrkAprSingleton* orkAprsingle = OrkAprSingleton::GetInstance();
	m_mp = orkAprsingle->GetAprMp();
    ret = apr_sockaddr_info_get(&m_sockAddr, NULL, APR_INET, 10001, 0, m_mp);
    if(ret != APR_SUCCESS) m_isValid = false;
    ret = apr_socket_create(&m_socket, m_sockAddr->family, SOCK_STREAM,APR_PROTO_TCP, m_mp);
    if(ret != APR_SUCCESS) m_isValid = false;
    
    apr_socket_opt_set(m_socket, APR_SO_REUSEADDR, 1);
    if(timeoutMs > 0)
    {
        apr_interval_time_t timeout = timeoutMs*1000;
        apr_socket_opt_set(m_socket, APR_SO_NONBLOCK, 0);
        apr_socket_timeout_set(m_socket, timeout);
    }
    else
    {
        apr_socket_opt_set(m_socket, APR_SO_NONBLOCK, 1);
    }
    ret = apr_socket_bind(m_socket, m_sockAddr);
    if(ret != APR_SUCCESS) m_isValid = false;
    ret = apr_socket_listen(m_socket, SOMAXCONN);
    if(ret != APR_SUCCESS) m_isValid = false;
}

bool OrkTcpSocket::GetStatus()
{
    return m_isValid;
}

void OrkTcpSocket::Run()
{

}

OrkTime::OrkTime()
{
    auto now = std::chrono::system_clock::now();
    auto duration_in_seconds = std::chrono::duration<double>(now.time_since_epoch());
    sec = duration_in_seconds.count();
    auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    msec = now_ms.time_since_epoch().count();

}
