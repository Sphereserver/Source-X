/**
* @file sphereversion.h
* @brief Versioning stuff.
*/

#ifndef _INC_SPHEREVERSION_H
#define _INC_SPHEREVERSION_H

#include "target_info.h"

#ifdef _GITVERSION
	#include "version/GitRevision.h"
	#define SPHERE_BUILD_VER			__GITREVISION__
	#define SPHERE_BUILD_BRANCH_NAME	__GITBRANCH__
#else
	#define SPHERE_BUILD_VER			0
	#define SPHERE_BUILD_BRANCH_NAME	"no-git"
#endif


#define SPHEREV_STRINGIFY_AUX(x)	#x
#define SPHEREV_STRINGIFY(x)		SPHEREV_STRINGIFY_AUX(x)

#define SPHERE_BUILD_VER_STR		SPHEREV_STRINGIFY(__GITREVISION__)


#define SPHERE_VER_FILEVERSION		1,1,0,SPHERE_BUILD_VER		// version to be set on generated .exe file (Windows only)
#define SPHERE_VER_ID_STR           "110"                       // to be used for script and server VERSION property, which is numerical
#define SPHERE_VER_NAME				"X1"						// share version with all files

#if defined(_PRIVATEBUILD)
	#define SPHERE_BUILD_TYPE_STR			"-Private"
	#define SPHERE_VER_FILEFLAGS			0x8L	//VS_FF_PRIVATEBUILD
#elif defined(_DEBUG)
	#define SPHERE_BUILD_TYPE_STR			"-Debug"
	#define SPHERE_VER_FILEFLAGS			0x1L	//VS_FF_DEBUG
#elif defined(_NIGHTLYBUILD)
	#define SPHERE_BUILD_TYPE_STR			"-Nightly"
	#define SPHERE_VER_FILEFLAGS			0x2L	//VS_FF_PRERELEASE
#else
	#define SPHERE_BUILD_TYPE_STR			"-Release"
	#define SPHERE_VER_FILEFLAGS			0x0L
#endif

#ifdef _WIN32
	#define SPHERE_VER_FILEOS			0x4L	// VOS__WINDOWS32
#else
	#define SPHERE_VER_FILEOS			0x0L	// VOS_UNKNOWN
#endif


#define SPHERE_BUILD_NAME_VER_PREFIX    "Version "
#define SPHERE_BUILD_INFO_STR			SPHERE_VER_NAME SPHERE_BUILD_TYPE_STR

// For security purposes, do NOT show the branch/revision to players or send it to a client or assistant, since those can be catched.
#define SPHERE_BUILD_INFO_GIT_STR		SPHERE_VER_NAME SPHERE_BUILD_TYPE_STR " (" SPHERE_BUILD_BRANCH_NAME ", rev " SPHERE_BUILD_VER_STR ")"


#endif // _INC_SPHEREVERSION_H
