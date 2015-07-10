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

#ifndef SizedBuffer_h
#define SizedBuffer_h 1

#include "shared_ptr.h"
#include <stdexcept>
#include "OrkBase.h"

class DLL_IMPORT_EXPORT_ORKBASE SizedBuffer {
	public:
		SizedBuffer(size_t len);
		SizedBuffer(unsigned char* buf,size_t len);
		SizedBuffer() : m_buffer(NULL), m_size(0) {} ;
		~SizedBuffer();

		unsigned char* get(size_t offset=0) { 
			if (!m_buffer || offset>=m_size) {
				throw std::runtime_error("SizedBuffer out of bound");
			}
			return m_buffer+offset; 
		}
		size_t size() {
		   	return m_size; 
		};

		void reset(unsigned char *buf, size_t len);
		void append(unsigned char *buf, size_t len);
		void write(size_t offset, unsigned char* buf, size_t len);
		void resize(size_t newSize); 

	private:
		unsigned char* m_buffer;
		size_t m_size;

		// private and unimplemented to prevent their use
		SizedBuffer(const SizedBuffer&);
		SizedBuffer& operator=(const SizedBuffer&);
};

typedef oreka::shared_ptr<SizedBuffer> SizedBufferRef;

#endif
