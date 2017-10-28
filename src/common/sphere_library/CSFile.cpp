
#ifndef _WIN32
	#include <errno.h>	// errno
	#include <sys/types.h>
	#include <sys/stat.h>
#endif
#include "../../common/CLog.h"
#include "../../sphere/threads.h"
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
	if ( pszName == NULL )
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

#ifdef _WIN32
void CFile::NotifyIOError( lpctstr szMessage ) const
{
	LPVOID lpMsgBuf;
	FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, reinterpret_cast<lptstr>(&lpMsgBuf), 0, NULL );
	DEBUG_ERR(( "File I/O \"%s\" failed on file \"%s\" (%lX): %s\n", szMessage, static_cast<lpctstr>(GetFilePath()), GetLastError(), static_cast<lptstr>(lpMsgBuf) ));

	LocalFree( lpMsgBuf );
}
#endif

bool CFile::Open( lpctstr pszName, uint uMode, CSError * e )
{
	UNREFERENCED_PARAMETER(e);
	ASSERT( m_llFile == -1 );
	SetFilePath( pszName );

#ifdef _WIN32
	DWORD dwShareMode, dwCreationDisposition, dwDesiredAccess;

	dwDesiredAccess = GENERIC_READ;
	if ( uMode & OF_WRITE )
		dwDesiredAccess |= GENERIC_WRITE;
	if ( uMode & OF_READWRITE )
		dwDesiredAccess |= (GENERIC_READ | GENERIC_WRITE);

	if ( uMode & OF_SHARE_COMPAT )
		dwShareMode = (FILE_SHARE_READ | FILE_SHARE_WRITE);
	else if ( uMode & OF_SHARE_EXCLUSIVE )
		dwShareMode = 0;
	else if ( uMode & OF_SHARE_DENY_WRITE )
		dwShareMode = FILE_SHARE_READ;
	else if ( uMode & OF_SHARE_DENY_READ )
		dwShareMode = FILE_SHARE_WRITE;
	else if ( uMode & OF_SHARE_DENY_NONE )
		dwShareMode = (FILE_SHARE_READ | FILE_SHARE_WRITE);
	else
		dwShareMode = 0;

	if ( uMode & OF_CREATE )
		dwCreationDisposition = (OPEN_ALWAYS|CREATE_NEW);
	else
		dwCreationDisposition = OPEN_EXISTING;

	m_llFile = (llong)CreateFile( GetFilePath(), dwDesiredAccess, dwShareMode, NULL, dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, NULL );
#else
	m_llFile = open( GetFilePath(), uMode );
#endif // _WIN32
	return (m_llFile != -1);
}

// CFile:: Content management.

size_t CFile::GetLength()
{
	// Get the size of the file.
	size_t pPos = GetPosition();	// save current pos.
	size_t pSize = SeekToEnd();
	Seek( pPos, SEEK_SET );	// restore previous pos.
	return pSize;
}

size_t CFile::GetPosition() const
{
	// TODO: use C++ ifstream/ofstream classes to write a code suitable on all platforms
#ifdef _WIN32
	return SetFilePointer( (HANDLE)m_llFile, 0, NULL, FILE_CURRENT );
#else
	return lseek( m_llFile, 0, SEEK_CUR );
#endif
}

size_t CFile::Read( void * pData, size_t stLength ) const
{
	// TODO: use C++ ifstream/ofstream classes to write a code suitable on all platforms
#ifdef _WIN32
	DWORD dwRead;
	if ( !ReadFile( (HANDLE)m_llFile, pData, (DWORD)stLength, &dwRead, NULL ) )
	{
		NotifyIOError("read");
		return 0;
	}
	return dwRead;
#else
	ssize_t ret = read(m_llFile, pData, stLength);
	if (ret == -1)
		return 0;
	return (size_t)ret;
#endif
}

size_t CFile::Seek( size_t stOffset, int iOrigin )
{
	// TODO: use C++ ifstream/ofstream classes to write a code suitable on all platforms
//	if ( m_llFile <= 0 )
//		return -1;
#ifdef _WIN32
	return SetFilePointer( (HANDLE)m_llFile, (LONG)stOffset, NULL, (DWORD)iOrigin );
#else
	return (size_t)lseek( m_llFile, stOffset, iOrigin );
#endif
}

void CFile::SeekToBegin()
{
	Seek( 0, SEEK_SET );
}

size_t CFile::SeekToEnd()
{
	return Seek( 0, SEEK_END );
}

bool CFile::Write( const void * pData, size_t stLength ) const
{
	// TODO: use C++ ifstream/ofstream classes to write a code suitable on all platforms
#ifdef _WIN32
	DWORD dwWritten;
	BOOL ret = ::WriteFile( (HANDLE)m_llFile, pData, (DWORD)stLength, &dwWritten, NULL );
	if ( ret == FALSE )
	{
		NotifyIOError("write");
		return false;
	}
	return true;
#else
	ssize_t ret = write(m_llFile, (const char *)pData, stLength);
	if (ret == (ssize_t)-1 || (size_t)ret != stLength)
		return false;
	return true;
#endif
}

// CSFile:: Constructors, Destructor, Asign operator.

CSFile::CSFile()
{
m_uMode = 0;
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

int CSFile::GetLastError()
{
#ifdef _WIN32
	return ::GetLastError();
#else
	return errno;
#endif
}

bool CSFile::IsFileOpen() const
{
	return ( m_llFile != -1 );
}

bool CSFile::Open( lpctstr pszFilename, uint uModeFlags, void FAR * pExtra )
{
	ADDTOCALLSTACK("CSFile::Open");
	// RETURN: true = success.
	// OF_BINARY | OF_WRITE
	if ( pszFilename == NULL )
	{
		if ( IsFileOpen() )
			return true;
	}
	else
		Close();	// Make sure it's closed first.

	if ( pszFilename == NULL )
		pszFilename = GetFilePath();
	else
		m_strFileName = pszFilename;

	if ( m_strFileName.IsEmpty() )
		return false;

	m_uMode = uModeFlags;
	if ( !OpenBase( pExtra ) )
		return false;

	return true;
}

bool CSFile::OpenBase( void * pExtra )
{
	ADDTOCALLSTACK("CSFile::OpenBase");
	UNREFERENCED_PARAMETER(pExtra);
	return static_cast<CFile *>(this)->Open(GetFilePath(), GetMode());
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
		len--;
		if ( pszPath[len] == '\\' || pszPath[len] == '/' )
		{
			len++;
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
		len--;
		if ( pszName[len] == '\\' || pszName[len] == '/' )
			break;
		if ( pszName[len] == '.' )
		{
			return ( pszName + len );
		}
	}
	return NULL;	// has no ext.
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
	return static_cast<CSString>(szFilePath);
}

// CSFile:: Mode operations.

uint CSFile::GetFullMode() const
{
	return m_uMode;
}

uint CSFile::GetMode() const
{
	return( m_uMode & 0x0FFFFFFF );
}

bool CSFile::IsBinaryMode() const
{
	return true;
}

bool CSFile::IsWriteMode() const
{
	return ( m_uMode & OF_WRITE );
}

// CSFileText:: Constructors, Destructor, Asign operator.

CSFileText::CSFileText()
{
	m_pStream = NULL;
#ifdef _WIN32
	bNoBuffer = false;
#endif
}

CSFileText::~CSFileText()
{
	Close();
}

// CSFileText:: File management.

bool CSFileText::IsFileOpen() const
{
	return ( m_pStream != NULL );
}

bool CSFileText::OpenBase( void FAR * pszExtra )
{
	ADDTOCALLSTACK("CSFileText::OpenBase");
	UNREFERENCED_PARAMETER(pszExtra);

	// Open a file.
	m_pStream = fopen( GetFilePath(), GetModeStr() );
	if ( m_pStream == NULL )
		return false;

	// Get the file descriptor for it.
	m_llFile = STDFUNC_FILENO(m_pStream);

	return true;
}

void CSFileText::CloseBase()
{
	ADDTOCALLSTACK("CSFileText::CloseBase");
	if ( IsWriteMode() )
		fflush(m_pStream);

	fclose(m_pStream);
	m_pStream = NULL;
}

// CSFileText:: Content management.

void CSFileText::Flush() const
{
	if ( !IsFileOpen() )
		return;
	ASSERT(m_pStream);
	fflush(m_pStream);
}

bool CSFileText::IsEOF() const
{
	if ( !IsFileOpen() )
		return true;
	return feof(m_pStream) ? true : false;
}

size_t _cdecl CSFileText::Printf( lpctstr pFormat, ... )
{
	ASSERT(pFormat);
	va_list vargs;
	va_start( vargs, pFormat );
	size_t iRet = VPrintf( pFormat, vargs );
	va_end( vargs );
	return iRet;
}

size_t CSFileText::Read( void * pBuffer, size_t sizemax ) const
{
	// This can return: EOF(-1) constant.
	// returns the number of full items actually read
	ASSERT(pBuffer);
	if ( IsEOF() )
		return 0;	// LINUX will ASSERT if we read past end.
	return fread( pBuffer, 1, sizemax, m_pStream );
}

tchar * CSFileText::ReadString( tchar * pBuffer, size_t sizemax ) const
{
// Read a line of text. NULL = EOF
	ASSERT(pBuffer);
	if ( IsEOF() )
		return NULL;	// LINUX will ASSERT if we read past end.
	return fgets( pBuffer, (int)sizemax, m_pStream );
}

size_t CSFileText::VPrintf( lpctstr pFormat, va_list args )
{
	ASSERT(pFormat);
	if ( !IsFileOpen() )
		return (size_t)(-1);

	size_t lenret = vfprintf( m_pStream, pFormat, args );
	return lenret;
}

#ifndef _WIN32
bool CSFileText::Write( const void * pData, size_t stLen ) const
#else
bool CSFileText::Write( const void * pData, size_t stLen )
#endif
{
// RETURN: 1 = success else fail.
	ASSERT(pData);
	if ( !IsFileOpen() )
		return false;
#ifdef _WIN32 //	Windows flushing, the only safe mode to cancel it ;)
	if ( !bNoBuffer )
	{
		setvbuf(m_pStream, NULL, _IONBF, 0);
		bNoBuffer = true;
	}
#endif
	size_t iStatus = fwrite( pData, stLen, 1, m_pStream );
#ifndef _WIN32	// However, in unix, it works
	fflush( m_pStream );
#endif
	return ( iStatus == 1 );
}

bool CSFileText::WriteString( lpctstr pStr )
{
// RETURN: < 0 = failed.
	ASSERT(pStr);
	return Write( pStr, (dword)strlen( pStr ) );
}

// CSFileText:: Mode operations.

lpctstr CSFileText::GetModeStr() const
{
	ADDTOCALLSTACK("CSFileText::GetModeStr");
// end of line translation is crap. ftell and fseek don't work correctly when you use it.
// fopen() args
	if ( IsBinaryMode())
		return ( IsWriteMode() ? "wb" : "rb" );
	if ( GetMode() & OF_READWRITE )
		return "a+b";
	if ( GetMode() & OF_CREATE )
		return "w";
	if ( IsWriteMode() )
		return "w";
	else
		return "rb";	// don't parse out the \n\r
}

bool CSFileText::IsBinaryMode() const
{
	return false;
}

