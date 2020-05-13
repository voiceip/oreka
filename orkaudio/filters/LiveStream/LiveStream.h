/*
 * Oreka -- A media capture and retrieval platform
 *
 * G729 Plugin
 * Author Kinshuk Bairagi
 *
 */

#ifndef __LIVESTREAM_H__
#define __LIVESTREAM_H__ 1

#include "LogManager.h"
#include "Filter.h"
#include "EventStreaming.h"
#include <log4cxx/logger.h>
#include "Utils.h"

class DLL_IMPORT_EXPORT_ORKBASE LiveStreamFilter : public Filter
{
  public:
    LiveStreamFilter();
    ~LiveStreamFilter();
   
    FilterRef __CDECL__ Instanciate();
    void __CDECL__ AudioChunkIn(AudioChunkRef& chunk);
    void __CDECL__ AudioChunkOut(AudioChunkRef& chunk);
    AudioEncodingEnum __CDECL__ GetInputAudioEncoding();
    AudioEncodingEnum __CDECL__ GetOutputAudioEncoding();
    CStdString __CDECL__ GetName();
    bool __CDECL__ SupportsInputRtpPayloadType(int rtpm_payloadType );
    void __CDECL__ CaptureEventIn(CaptureEventRef& event);
    void __CDECL__ CaptureEventOut(CaptureEventRef& event);

  private:
    AudioChunkRef m_outputAudioChunk;
    bool m_initialized;

};

class DLL_IMPORT_EXPORT_ORKBASE LiveStreamServer
{
  public: 
    LiveStreamServer(int port);
    ~LiveStreamServer(){ apr_pool_destroy(m_mp);};
    bool Initialize();
    void Run();

  private:
    apr_pool_t* m_mp;
    int m_port;
    apr_socket_t* m_socket;
    apr_sockaddr_t* m_sockAddr;
    static void StreamingSvc(apr_socket_t* sock, apr_pool_t* pool);

};


#endif
