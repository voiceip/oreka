#include "SafeBuffer.h"
#include "string.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

SafeBuffer::SafeBuffer()
{
	m_pBuffer = NULL;
	m_size = 0;
}

SafeBuffer::~SafeBuffer()
{
	if(m_size) {
		free(m_pBuffer);
		m_size = 0;
	}

	m_pBuffer = NULL;
}

int SafeBuffer::Size()
{
	return m_size;
}

void SafeBuffer::Store(u_char *buf, int len)
{
	if(m_size) {
		free(m_pBuffer);
		m_size = 0;
		m_pBuffer = NULL;
	}

	m_pBuffer = (u_char *)calloc(len+1, 1);
	m_size = len;

	if(!m_pBuffer) {
		char tmp[80];
		snprintf(tmp, sizeof(tmp), "%d", len);

		CStdString numBytes = CStdString(tmp);
                throw("SafeBuffer::Store could not malloc a buffer of size:" + numBytes);
	}

	memcpy(m_pBuffer, buf, len);
}

void SafeBuffer::Add(u_char *buf, int len)
{
        u_char *newBuf = NULL;

        if(!m_size) {
                Store(buf, len);
                return;
        }

        newBuf = (u_char*)realloc(m_pBuffer, m_size+len+1);
        if(!newBuf) {
                char tmp[80];
                snprintf(tmp, sizeof(tmp), "%d", len+m_size);

                CStdString numBytes = CStdString(tmp);
                throw("SafeBuffer::Add failed to realloc buffer to " + numBytes);
        }

        m_pBuffer = newBuf;
        memcpy(m_pBuffer+m_size, buf, len);
        m_size += len;
        *(m_pBuffer+m_size) = 0;
}

u_char *SafeBuffer::GetBuffer()
{
	return m_pBuffer;
}

