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

#ifndef _SAFEBUFFER_H__
#define _SAFEBUFFER_H__ 1

#include "shared_ptr.h"
#include "StdString.h"

class SafeBuffer
{
	public:
		SafeBuffer();
		~SafeBuffer();

		void Store(u_char *buf, int len);
		void Add(u_char *buf, int len);
		u_char *GetBuffer();
		int Size();

	private:
		u_char *m_pBuffer;
		int m_size;
};
typedef oreka::shared_ptr<SafeBuffer> SafeBufferRef;

#endif

