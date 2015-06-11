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

#define _WINSOCKAPI_ 

#include "SizedBuffer.h"
#include "StdString.h"
#include "ace/OS_NS_string.h"

SizedBuffer::SizedBuffer(unsigned char* buf, size_t len) : m_buffer(NULL), m_size(0) {
	reset(buf,len);
}

SizedBuffer::SizedBuffer(size_t len) : m_buffer(NULL), m_size(0) {
	resize(len);
}

SizedBuffer::~SizedBuffer() {
	if (m_buffer) {
		free(m_buffer);
	}
}

void SizedBuffer::reset(unsigned char *buf, size_t len) {
	if (m_buffer) {
		free(m_buffer);
		m_size = 0;
		m_buffer = NULL;
	}
	write(0,buf,len);
}

void SizedBuffer::append(unsigned char *buf, size_t len) {
	write(m_size,buf,len);
}

void SizedBuffer::write(size_t offset, unsigned char* buf, size_t len) {
	if (offset+len > m_size) {
		resize(offset+len);
	}
	memcpy(m_buffer+offset,buf,len);
}

void SizedBuffer::resize(size_t newSize) {
	unsigned char* newBuf = (unsigned char*)::realloc(m_buffer, newSize);
	if(!newBuf) {
		throw std::runtime_error("SizedBuffer failed to allocate space");
	}
	m_buffer = newBuf;
	m_size = newSize;
}

