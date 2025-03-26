/**
* @file CSFileList.cpp
*/

#include <time.h>
#if defined(_WIN32)
	#include <io.h> 		// findfirst
#else	// LINUX
	#include <sys/stat.h>
	#include <dirent.h>
#endif
#include "CSFileList.h"
#include "../../common/CLog.h"
#include "../../sphere/threads.h"


CSFileList::CSFileList() = default;
CSFileList::~CSFileList() = default;

// Similar to the MFC CFileFind
bool CSFileList::ReadFileInfo( lpctstr pszFilePath, time_t & dwDateChange, dword & dwSize ) // static
{
	ADDTOCALLSTACK("CSFileList::ReadFileInfo");
#ifdef _WIN32
	// WIN32
	struct _finddata_t fileinfo;
	fileinfo.attrib = _A_NORMAL;
	intptr_t lFind = _findfirst( pszFilePath, &fileinfo );

	if ( lFind == -1 )
#else
	// LINUX
	struct stat fileStat;
	if ( stat( pszFilePath, &fileStat) == -1 )
#endif
	{
		g_Log.EventError( "Can't open input directory for '%s' (error: '%s').\n", pszFilePath, strerror(errno) );
		return false;
	}

#ifdef _WIN32
	// WIN32
	dwDateChange = fileinfo.time_write;
	dwSize = fileinfo.size;
	_findclose( lFind );
#else
	// LINUX
	dwDateChange = fileStat.st_mtime;
	dwSize = fileStat.st_size;
#endif

	return true;
}

int CSFileList::ReadDir( lpctstr pszFileDir, bool bShowError )
{
	ADDTOCALLSTACK("CSFileList::ReadDir");
	// NOTE: It seems NOT to like the trailing \ alone
	tchar szFileDir[SPHERE_MAX_PATH];
	size_t len = Str_CopyLen(szFileDir, pszFileDir);
#ifdef _WIN32
	if ( len > 0 )
	{
		--len;
		if ( szFileDir[len] == '\\' || szFileDir[len] == '/' )
			strcat( szFileDir, "*.*" );
	}
#endif

#ifdef _WIN32
	struct _finddata_t fileinfo;
	intptr_t lFind = _findfirst(szFileDir, &fileinfo);

	if ( lFind == -1 )
#else
	char szFilename[SPHERE_MAX_PATH];
	// Need to strip out the *.scp part
	for ( size_t i = len; i > 0; --i )
	{
		if ( szFileDir[i] == '/' )
		{
			szFileDir[i+1] = 0x00;
			break;
		}
	}
	
	DIR* dirp = opendir(szFileDir);
	struct dirent * fileinfo = nullptr;

	if ( !dirp )
#endif
	{
		if (bShowError == true)
		{
			g_Log.EventError("Unable to open directory '%s' (error: '%s').\n", szFileDir, strerror(errno));
		}
		return -1;
	}

	do
	{
#if defined(_WIN32)
		if ( fileinfo.name[0] == '.' )
			continue;
		AddTail(fileinfo.name);
#else
		fileinfo = readdir(dirp);
		if ( !fileinfo )
			break;
		if ( fileinfo->d_name[0] == '.' )
			continue;

		const int ret = snprintf(szFilename, SPHERE_MAX_PATH, "%s%s", szFileDir, fileinfo->d_name);
		szFilename[SPHERE_MAX_PATH - 1] = '\0';
		if ((ret < 0) || (ret > SPHERE_MAX_PATH - 1))
		{
			g_Log.EventError("Unable to concatenate the path (too long). Current file '%s'.\n", fileinfo->d_name);
			break;
		}
		len = strlen(szFilename);
        if ( (len > SPHERE_SCRIPT_EXT_LEN) && !strcmpi(&szFilename[len - SPHERE_SCRIPT_EXT_LEN], SPHERE_SCRIPT_EXT) )
			AddTail(fileinfo->d_name);
#endif
	}
#if defined(_WIN32)
	while ( !_findnext(lFind, &fileinfo));

	_findclose(lFind);
#else
	while ( fileinfo != nullptr );

	closedir(dirp);
#endif
	return (int)(GetContentCount());
}

