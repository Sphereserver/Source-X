/**
* @file sphereversion.h
* @brief Versioning stuff.
*/

#ifndef _INC_SPHEREVERSION_H
#define _INC_SPHEREVERSION_H

#include "target_info.h"

#ifdef _GITVERSION
	#include "version/GitRevision.h"
	#define SPHERE_VER_BUILD			__GITREVISION__
#else
	#define SPHERE_VER_BUILD			0
#endif


#define SPHERE_VER_FILEVERSION		1,1,0,SPHERE_VER_BUILD		// version to be set on generated .exe file (Windows only)
#define SPHERE_VER_ID_STR           "110"                       // to be used for script and server VERSION property, which is numerical
#define SPHERE_VER_NAME				"X1"						// share version with all files

#define SPHERE_BUILD_NAME_PREFIX       "Version "
#if defined(_DEBUG)
	#define SPHERE_BUILD_NAME				SPHERE_VER_NAME "-Debug"
	#define SPHERE_VER_FILEFLAGS			0x1L	//VS_FF_DEBUG
#elif defined(_NIGHTLYBUILD)
	#define SPHERE_BUILD_NAME				SPHERE_VER_NAME "-Nightly"
	#define SPHERE_VER_FILEFLAGS			0x2L	//VS_FF_PRERELEASE
#elif defined(_PRIVATEBUILD)
	#define SPHERE_BUILD_NAME				SPHERE_VER_NAME "-Private"
	#define SPHERE_VER_FILEFLAGS			0x8L	//VS_FF_PRIVATEBUILD
#else
	#define SPHERE_BUILD_NAME				SPHERE_VER_NAME "-Release"
	#define SPHERE_VER_FILEFLAGS			0x0L
#endif

#ifdef _WIN32
	#define SPHERE_VER_FILEOS			0x4L	// VOS__WINDOWS32
#else
	#define SPHERE_VER_FILEOS			0x0L	// VOS_UNKNOWN
#endif


#endif // _INC_SPHEREVERSION_H
