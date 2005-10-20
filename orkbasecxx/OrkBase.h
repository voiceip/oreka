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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#ifdef WIN32
	#ifdef BUILD_ORKBASE
	#define DLL_IMPORT_EXPORT  __declspec( dllexport )
	#else
	#define DLL_IMPORT_EXPORT  __declspec( dllimport )
	#endif
#else
	#define DLL_IMPORT_EXPORT
#endif

#include "StdString.h"
