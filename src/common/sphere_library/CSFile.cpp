
#ifndef _WIN32
	#include <errno.h>	// errno
	#include <sys/types.h>
	#include <sys/stat.h>
#endif
#include "../../sphere/threads.h"
#include "../CLog.h"
#include "CSFile.h"


// CFile:: Constructors, Destructor, Asign operator.

CFile::CFile()
{
	m_llFile = -1;
}

CFile::~CFile()
{
	Close();
}

// CFile:: File Management.

void CFile::Close()
{
	if ( m_llFile != -1)
	{
#ifdef _WIN32
		CloseHandle( (HANDLE)m_llFile );
#else
		close( m_llFile );
#endif
		m_llFile = -1;
	}
}

const CSString & CFile::GetFilePath() const
{
	return m_strFileName;
}

bool CFile::SetFilePath( lpctstr pszName )
{
	ADDTOCALLSTACK("CFile::SetFilePath");
	if ( pszName == nullptr )
		return false;
	if ( ! m_strFileName.CompareNoCase( pszName ))
		return true;

	bool fIsOpen = ( m_llFile != -1);
	if ( fIsOpen )
		Close();
	m_strFileName = pszName;
	if ( fIsOpen )
		return Open( NULL, OF_READ|OF_BINARY ); // GetMode()	// open it back up. (in same mode as before)
	return true;
}

lpctstr CFile::GetFileTitle() const
{
	ADDTOCALLSTACK("CFile::GetFileTitle");
	return CSFile::GetFilesTitle( GetFilePath() );
}

int CFile::GetLastError()
{
#ifdef _WIN32
    return ::GetLastError();
#else
    return errno;
#endif
}

void CFile::NotifyIOError( lpctstr szMessage ) const
{
    int iErrorCode = this->GetLastError();
#ifdef _WIN32
    lpctstr pMsg;
    LPVOID lpMsgBuf;
    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, iErrorCode, 0, reinterpret_cast<LPTSTR>(&lpMsgBuf), 0, NULL );
    if (lpMsgBuf == NULL)
        pMsg = "No System Error";
    else
        pMsg = static_cast<lptstr>(lpMsgBuf);
    DEBUG_ERR(( "File I/O \"%s\" failed on file \"%s\" (%lX): %s\n", szMessage, static_cast<lpctstr>(GetFilePath()), iErrorCode, pMsg ));
    if (lpMsgBuf != NULL)
        LocalFree( lpMsgBuf );
#else
    DEBUG_ERR(( "File I/O \"%s\" failed on file \"%s\" (%lX): %s\n", szMessage, static_cast<lpctstr>(GetFilePath()), iErrorCode, strerror(iErrorCode) ));
#endif
}

bool CFile::Open( lpctstr pszName, uint uiMode )
{
	ASSERT( m_llFile == -1 );
	SetFilePath( pszName );

#ifdef _WIN32
	DWORD dwShareMode, dwCreationDisposition, dwDesiredAccess;

	dwDesiredAccess = GENERIC_READ;
	if ( uiMode & OF_WRITE )
		dwDesiredAccess |= GENERIC_WRITE;
	if ( uiMode & OF_READWRITE )
		dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);

	if ( uiMode & OF_SHARE_COMPAT )
		dwShareMode = (FILE_SHARE_READ | FILE_SHARE_WRITE);
	else if ( uiMode & OF_SHARE_EXCLUSIVE )
		dwShareMode = 0;
	else if ( uiMode & OF_SHARE_DENY_WRITE )
		dwShareMode = FILE_SHARE_READ;
	else if ( uiMode & OF_SHARE_DENY_READ )
		dwShareMode = FILE_SHARE_WRITE;
	else if ( uiMode & OF_SHARE_DENY_NONE )
		dwShareMode = (FILE_SHARE_READ | FILE_SHARE_WRITE);
	else
		dwShareMode = 0;

	if ( uiMode & OF_CREATE )
		dwCreationDisposition = (OPEN_ALWAYS|CREATE_NEW);
	else
		dwCreationDisposition = OPEN_EXISTING;

	m_llFile = (llong)CreateFile( GetFilePath(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL );
#else
	m_llFile = open( GetFilePath(), uiMode );
#endif // _WIN32
	return (m_llFile != -1);
}

// CFile:: Content management.

int CFile::GetLength()
{
	// Get the size of the file.
    int iPos = GetPosition();   // save current pos.
    int iSize = SeekToEnd();
	Seek( iPos, SEEK_SET );     // restore previous pos.
	return iSize;
}

int CFile::GetPosition() const
{
#ifdef _WIN32
    DWORD ret = SetFilePointer( (HANDLE)m_llFile, 0, NULL, FILE_CURRENT );
    if (ret == INVALID_SET_FILE_POINTER)
    {
        NotifyIOError("CFile::GetPosition");
        return 0;
    }
#else
	off_t ret = lseek( m_llFile, 0, SEEK_CUR );
    if (ret == (off_t)-1)
    {
        NotifyIOError("CFile::GetPosition");
        return 0;
    }
#endif
    if (ret > INT_MAX)
    {
        NotifyIOError("CFile::GetPosition (length)");
        return 0;
    }
    return ret;
}

int CFile::Read( void * pData, int iLength ) const
{
#ifdef _WIN32
	DWORD ret;
	if ( !ReadFile( (HANDLE)m_llFile, pData, (DWORD)iLength, &ret, NULL ) )
	{
		NotifyIOError("CFile::Read");
		return 0;
	}
#else
	ssize_t ret = read(m_llFile, pData, iLength);
	if (ret == -1)
    {
        NotifyIOError("CFile::Read");
		return 0;
    }
#endif
    if (ret > INT_MAX)
    {
        NotifyIOError("CFile::Read (length)");
        return 0;
    }
    return (int)ret;
}

int CFile::Seek( int iOffset, int iOrigin )
{
	if ( m_llFile <= 0 )
    {
        NotifyIOError("CFile::Seek");
        return -1;
    }
#ifdef _WIN32
	DWORD ret = SetFilePointer( (HANDLE)m_llFile, (LONG)iOffset, NULL, (DWORD)iOrigin );
#else
	off_t ret = lseek( m_llFile, iOffset, iOrigin );
#endif
    if (ret > INT_MAX)
    {
        NotifyIOError("CFile::Seek (length)");
        return 0;
    }
    return (int)ret;
}

void CFile::SeekToBegin()
{
	Seek( 0, SEEK_SET );
}

int CFile::SeekToEnd()
{
	return Seek( 0, SEEK_END );
}

bool CFile::Write( const void * pData, int iLength ) const
{
#ifdef _WIN32
	DWORD dwWritten;
	BOOL ret = ::WriteFile( (HANDLE)m_llFile, pData, (DWORD)iLength, &dwWritten, NULL );
	if ( ret == FALSE )
	{
		NotifyIOError("CFile::Write");
		return false;
	}
	return true;
#else
	ssize_t ret = write(m_llFile, (const char *)pData, iLength);
	if (ret == (ssize_t)-1 || ret != iLength)
    {
        NotifyIOError("CFile::Write");
		return false;
    }
	return true;
#endif
}

// CSFile:: Constructors, Destructor, Asign operator.

CSFile::CSFile()
{
    m_uiMode = 0;
}

CSFile::~CSFile()
{
    Close();
}

// CSFile:: File Management.

void CSFile::Close()
{
	ADDTOCALLSTACK("CSFile::Close");
	if ( ! IsFileOpen() )
		return;

	CloseBase();
	m_llFile = -1;
}

void CSFile::CloseBase()
{
	ADDTOCALLSTACK("CSFile::CloseBase");
	CFile::Close();
}

bool CSFile::IsFileOpen() const
{
	return ( m_llFile != -1 );
}

bool CSFile::Open( lpctstr pszFilename, uint uiModeFlags )
{
	ADDTOCALLSTACK("CSFile::Open");
	// RETURN: true = success.
	// OF_BINARY | OF_WRITE
	if ( !pszFilename )
	{
		if ( IsFileOpen() )
			return true;
	}
	else
    {
		Close();	// Make sure it's closed first.
    }

	if ( !pszFilename )
		pszFilename = GetFilePath();
	else
		m_strFileName = pszFilename;

	if ( m_strFileName.IsEmpty() )
		return false;

	m_uiMode = uiModeFlags;

	return OpenBase();
}

bool CSFile::OpenBase()
{
	ADDTOCALLSTACK("CSFile::OpenBase");
	return CFile::Open(GetFilePath(), GetMode());
}

// CSFile:: File name operations.

lpctstr CSFile::GetFilesTitle( lpctstr pszPath )
{
	ADDTOCALLSTACK("CSFile::GetFilesTitle");
	// Just use COMMDLG.H GetFileTitle(lpctstr, lptstr, word) instead ?
	// strrchr
	size_t len = strlen(pszPath);
	while ( len > 0 )
	{
		--len;
		if ( pszPath[len] == '\\' || pszPath[len] == '/' )
		{
			++len;
			break;
		}
	}
	return (pszPath + len);
}

lpctstr CSFile::GetFileExt() const
{
	ADDTOCALLSTACK("CSFile::GetFileExt");
	// get the EXTension including the .
	return GetFilesExt( GetFilePath() );
}

lpctstr CSFile::GetFilesExt( lpctstr pszName )	// static
{
	ADDTOCALLSTACK("CSFile::GetFilesExt");
    // get the EXTension including the .
	size_t lenall = strlen( pszName );
	size_t len = lenall;
	while ( len > 0 )
	{
		--len;
		if ( pszName[len] == '\\' || pszName[len] == '/' )
			break;
		if ( pszName[len] == '.' )
		{
			return ( pszName + len );
		}
	}
	return nullptr;	// has no ext.
}

CSString CSFile::GetMergedFileName( lpctstr pszBase, lpctstr pszName ) // static
{
	ADDTOCALLSTACK("CSFile::GetMergedFileName");
    // Merge path and file name.

	tchar szFilePath[ _MAX_PATH ];
	if ( pszBase && pszBase[0] )
	{
		strcpy( szFilePath, pszBase );
		int len = (int)(strlen( szFilePath ));
		if (len && szFilePath[len - 1] != '\\' && szFilePath[len - 1] != '/')
		{
#ifdef _WIN32
			strcat(szFilePath, "\\");
#else
			strcat(szFilePath, "/");
#endif
		}
	}
	else
	{
		szFilePath[0] = '\0';
	}
	if ( pszName )
	{
		strcat( szFilePath, pszName );
	}
	return CSString(szFilePath);
}

// CSFile:: Mode operations.

uint CSFile::GetFullMode() const
{
	return m_uiMode;
}

uint CSFile::GetMode() const
{
	return ( m_uiMode & 0x0FFFFFFF );
}

bool CSFile::IsBinaryMode() const
{
	return true;
}

bool CSFile::IsWriteMode() const
{
	return ( m_uiMode & OF_WRITE );
}
