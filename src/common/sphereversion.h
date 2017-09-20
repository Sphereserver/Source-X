/**
* @file sphereversion.h
* @brief Versioning stuff.
*/

#pragma once
#ifndef _INC_SPHEREVERSION_H
#define _INC_SPHEREVERSION_H

#ifdef _GITVERSION
	#include "version/GitRevision.h"
	#define SPHERE_VER_BUILD			__GITREVISION__
#else
	#define SPHERE_VER_BUILD			0
#endif

#define SPHERE_VER_FILEVERSION		0,56,4,SPHERE_VER_BUILD		// version to be set on generated .exe file
//#define	SPHERE_VER_NUM				0x00005604L				// version for some internal usage, like compiled scripts	//unused
#define SPHERE_VER_STR				"0.56d"						// share version with all files

#if defined(_DEBUG)
	#define SPHERE_VERSION					SPHERE_VER_STR "-Debug"
	#define SPHERE_VER_FILEFLAGS			0x1L	//VS_FF_DEBUG
#elif defined(_NIGHTLYBUILD)
	#define SPHERE_VERSION					SPHERE_VER_STR "-Nightly"
	#define SPHERE_VER_FILEFLAGS			0x2L	//VS_FF_PRERELEASE
#elif defined(_PRIVATEBUILD)
	#define SPHERE_VERSION					SPHERE_VER_STR "-Private"
	#define SPHERE_VER_FILEFLAGS			0x8L	//VS_FF_PRIVATEBUILD
#else
	#define SPHERE_VERSION					SPHERE_VER_STR "-Release"
	#define SPHERE_VER_FILEFLAGS			0x0L
#endif

#if defined(_WIN32)
	#define SPHERE_VER_FILEOS				0x4L	// VOS__WINDOWS32
	#if defined(_64BITS)
		#define SPHERE_VER_FILEOS_STR		"[WIN64]"
	#else
		#define SPHERE_VER_FILEOS_STR		"[WIN32]"
	#endif
#elif defined(_BSD)
	#define SPHERE_VER_FILEOS				0x0L	// VOS_UNKNOWN
	#if defined(_64BITS)
		#define SPHERE_VER_FILEOS_STR		"[FreeBSD-64]"
	#else
		#define SPHERE_VER_FILEOS_STR		"[FreeBSD-32]"
	#endif
#else
	#define SPHERE_VER_FILEOS				0x0L	// VOS_UNKNOWN
	#if defined(_64BITS)
		#define SPHERE_VER_FILEOS_STR		"[Linux-64]"
	#else
		#define SPHERE_VER_FILEOS_STR		"[Linux-32]"
	#endif
#endif


#endif // _INC_SPHEREVERSION_H
