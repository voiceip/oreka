#ifndef __DLL_H__
#define __DLL_H__

#ifdef WIN32
	#ifdef BUILD_ORKBASE
	#define DLL_IMPORT_EXPORT  __declspec( dllexport )
	#else
	#define DLL_IMPORT_EXPORT  __declspec( dllimport )
	#endif
#else
	#define DLL_IMPORT_EXPORT
#endif

#ifdef WIN32
#define __CDECL__ __cdecl
#else
#define __CDECL__
#endif
#ifdef WIN32
	#define DLL_EXPORT  __declspec( dllexport )
#else
	#define DLL_EXPORT
#endif

#endif