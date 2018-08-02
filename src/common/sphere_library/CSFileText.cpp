#include "../../sphere/threads.h"
#include "../CLog.h"
#include "CSFileText.h"

#ifdef _WIN32
    #include <io.h> // for _get_osfhandle (used by STDFUNC_FILENO)
#endif

// CSFileText:: Constructors, Destructor, Asign operator.

CSFileText::CSFileText()
{
    m_pStream = nullptr;
#ifdef _WIN32
    _fNoBuffer = false;
#endif
}

CSFileText::~CSFileText()
{
    Close();
}

// CSFileText:: File management.

bool CSFileText::IsFileOpen() const
{
    return ( m_pStream != nullptr );
}

bool CSFileText::OpenBase()
{
    ADDTOCALLSTACK("CSFileText::OpenBase");

    // Open a file.
    m_pStream = fopen( GetFilePath(), GetModeStr() );
    if ( m_pStream == nullptr )
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
    m_pStream = nullptr;
    m_llFile = -1;
}

// CSFileText:: Content management.
int CSFileText::Seek( int iOffset, int iOrigin )
{
    // RETURN:
    //  true = success
    ADDTOCALLSTACK("CSFileText::Seek");

    if ( !IsFileOpen() )
        return 0;
    if ( iOffset < 0 )
        return 0;
    if ( fseek(m_pStream, iOffset, iOrigin) != 0 )
        return 0;

    long iPos = ftell(m_pStream);
    if ( iPos < 0 )
    {
        return 0;
    }
    else if (iPos > INT_MAX)   // be consistent between windows and linux: support on both platforms at maximum an int (long has 4 or 8 bytes width, depending on the os)
    {
        NotifyIOError("CSFileText::Seek (length)");
        return INT_MAX;
    }
    return (int)iPos;
}

void CSFileText::Flush() const
{
    ADDTOCALLSTACK("CSFileText::Flush");

    if ( !IsFileOpen() )
        return;

    ASSERT(m_pStream);
    fflush(m_pStream);
}

bool CSFileText::IsEOF() const
{
    ADDTOCALLSTACK("CSFileText::IsEOF");

    if ( !IsFileOpen() )
        return true;

    return feof(m_pStream) ? true : false;
}

int _cdecl CSFileText::Printf( lpctstr pFormat, ... )
{
    ADDTOCALLSTACK("CSFileText::Printf");

    ASSERT(pFormat);
    va_list vargs;
    va_start( vargs, pFormat );
    int iRet = VPrintf( pFormat, vargs );
    va_end( vargs );
    return iRet;
}

int CSFileText::Read( void * pBuffer, int sizemax ) const
{
    // This can return: EOF(-1) constant.
    // returns the number of full items actually read
    ADDTOCALLSTACK("CSFileText::Read");
    ASSERT(pBuffer);

    if ( IsEOF() )
        return 0;	// LINUX will ASSERT if we read past end.

    size_t ret = fread( pBuffer, 1, sizemax, m_pStream );
    if (ret > INT_MAX)
    {
        NotifyIOError("CSFileText::Read (length)");
        return 0;
    }
    return (int)ret;
}

tchar * CSFileText::ReadString( tchar * pBuffer, int sizemax ) const
{
    // Read a line of text. NULL/nullptr = EOF
    ADDTOCALLSTACK("CSFileText::ReadString");
    ASSERT(pBuffer);

    if ( IsEOF() )
        return nullptr;	// LINUX will ASSERT if we read past end.

    return fgets( pBuffer, sizemax, m_pStream );
}

int CSFileText::VPrintf( lpctstr pFormat, va_list args )
{
    ADDTOCALLSTACK("CSFileText::VPrintf");
    ASSERT(pFormat);

    if ( !IsFileOpen() )
        return -1;

    int lenret = vfprintf( m_pStream, pFormat, args );
    return lenret;
}

#ifndef _WIN32
bool CSFileText::Write( const void * pData, int iLen ) const
#else
bool CSFileText::Write( const void * pData, int iLen )
#endif
{
    // RETURN: 1 = success else fail.
    ADDTOCALLSTACK("CSFileText::Write");
    ASSERT(pData);

    if ( !IsFileOpen() )
        return false;

#ifdef _WIN32 // Windows flushing, the only safe mode to cancel it ;)
    if ( !_fNoBuffer )
    {
        setvbuf(m_pStream, NULL, _IONBF, 0);
        _fNoBuffer = true;
    }
#endif
    size_t uiStatus = fwrite( pData, iLen, 1, m_pStream );
#ifndef _WIN32	// However, in unix, it works
    fflush( m_pStream );
#endif
    return ( uiStatus == 1 );
}

bool CSFileText::WriteString( lpctstr pStr )
{
    // RETURN: < 0 = failed.
    ADDTOCALLSTACK("CSFileText::WriteString");
    ASSERT(pStr);

    return Write( pStr, (int)strlen( pStr ) );
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
