#ifndef __DLL_H__
#define __DLL_H__

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


