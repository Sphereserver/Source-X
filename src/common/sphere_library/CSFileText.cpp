#include "../../sphere/threads.h"
#include "../CLog.h"
#include "CSFileText.h"

#ifdef _WIN32
    #include <io.h> // for _get_osfhandle (used by STDFUNC_FILENO)
#endif

// CSFileText:: Constructors, Destructor, Asign operator.

CSFileText::CSFileText()
{
    _pStream = nullptr;
#ifdef _WIN32
    _fNoBuffer = false;
#endif
}

CSFileText::~CSFileText()
{
    Close();
}

// CSFileText:: File management.

bool CSFileText::_IsFileOpen() const
{
    return (_pStream != nullptr);
}
bool CSFileText::IsFileOpen() const
{
    MT_SHARED_LOCK_RETURN(_pStream != nullptr);
}

bool CSFileText::_Open(lpctstr ptcFilename, uint uiModeFlags)
{
    ADDTOCALLSTACK("CSFileText::_Open");

    // Open a text file.

	if ( !ptcFilename )
        ptcFilename = _strFileName.GetBuffer();
    else
        _strFileName = ptcFilename;

    if ( _strFileName.IsEmpty() )
        return false;

    _uiMode = uiModeFlags;
    lpctstr ptcModeStr = _GetModeStr();

    _pStream = fopen( ptcFilename, ptcModeStr );
    if ( _pStream == nullptr )
        return false;

    // Get the file descriptor for it.
    _fileDescriptor = (file_descriptor_t)STDFUNC_FILENO(_pStream);

    return true;
}
bool CSFileText::Open(lpctstr ptcFilename, uint uiModeFlags)
{
    ADDTOCALLSTACK("CSFileText::Open");
    MT_UNIQUE_LOCK_RETURN(CSFileText::_Open(ptcFilename, uiModeFlags));
}

void CSFileText::_Close()
{
    ADDTOCALLSTACK("CSFileText::_Close");

    // CCacheableScriptFile opens the file, reads and closes it. It should never be opened, so pStream should be always nullptr.
    if ((_pStream != nullptr) /*&& (_pStream != _kInvalidFD)*/)
    {
        if (_IsWriteMode())
        {
            fflush(_pStream);
        }

        fclose(_pStream);
        _pStream = nullptr;
        _fileDescriptor = _kInvalidFD;
    }
}
void CSFileText::Close()
{
    ADDTOCALLSTACK("CSFileText::Close");
    MT_UNIQUE_LOCK_SET;
    CSFileText::_Close();
}

// CSFileText:: Content management.
int CSFileText::_Seek( int iOffset, int iOrigin )
{
    // RETURN:
    //  true = success
    ADDTOCALLSTACK("CSFileText::_Seek");

    if ( !CSFileText::_IsFileOpen() )
        return 0;
    if ( iOffset < 0 )
        return 0;

    if ( fseek(_pStream, iOffset, iOrigin) != 0 )
        return 0;

    long iPos = ftell(_pStream);
    if ( iPos < 0 )
    {
        return 0;
    }
    else if (iPos > INT_MAX)   // be consistent between windows and linux: support on both platforms at maximum an int (long has 4 or 8 bytes width, depending on the os)
    {
        _NotifyIOError("CSFileText::Seek (length)");
        return INT_MAX;
    }
    return (int)iPos;
}
int CSFileText::Seek( int iOffset, int iOrigin )
{
    // RETURN:
    //  true = success
    ADDTOCALLSTACK("CSFileText::Seek");
    MT_UNIQUE_LOCK_RETURN(CSFileText::_Seek(iOffset, iOrigin));
}

void CSFileText::_Flush() const
{
    ADDTOCALLSTACK("CSFileText::_Flush");

    if ( !_IsFileOpen() )
        return;

    ASSERT(_pStream);
    fflush(_pStream);
}
void CSFileText::Flush() const
{
    ADDTOCALLSTACK("CSFileText::Flush");
    MT_UNIQUE_LOCK_SET;
    CSFileText::_Flush();
}

bool CSFileText::_IsEOF() const
{
    //ADDTOCALLSTACK("CSFileText::_IsEOF");

    if ( !_IsFileOpen() )
        return true;

    return (feof(_pStream) ? true : false);
}
bool CSFileText::IsEOF() const
{
    //ADDTOCALLSTACK("CSFileText::IsEOF");
    MT_SHARED_LOCK_RETURN(CSFileText::_IsEOF());
}


int _cdecl CSFileText::_Printf(lpctstr pFormat, ...)
{
    ADDTOCALLSTACK("CSFileText::_Printf");
    ASSERT(pFormat);

    va_list vargs;
    va_start(vargs, pFormat);
    int iRet = _VPrintf(pFormat, vargs);
    va_end(vargs);

    return iRet;
}
int _cdecl CSFileText::Printf( lpctstr pFormat, ... )
{
    ADDTOCALLSTACK("CSFileText::Printf");
    ASSERT(pFormat);

    MT_UNIQUE_LOCK_SET;

    va_list vargs;
    va_start( vargs, pFormat );
    int iRet = _VPrintf( pFormat, vargs );
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

    MT_UNIQUE_LOCK_SET;
    size_t ret = fread( pBuffer, 1, sizemax, _pStream );
    if (ret > INT_MAX)
    {
        _NotifyIOError("CSFileText::Read (length)");
        return 0;
    }
    return (int)ret;
}

tchar * CSFileText::_ReadString( tchar * pBuffer, int sizemax )
{
    // Read a line of text. nullptr/nullptr = EOF
    ADDTOCALLSTACK("CSFileText::_ReadString");
    ASSERT(pBuffer);

    if ( _IsEOF() )
        return nullptr;	// LINUX will ASSERT if we read past end.

    return fgets( pBuffer, sizemax, _pStream );
}

tchar * CSFileText::ReadString( tchar * pBuffer, int sizemax )
{
    ADDTOCALLSTACK("CSFileText::ReadString");
    MT_UNIQUE_LOCK_RETURN(CSFileText::_ReadString(pBuffer, sizemax));
}

int CSFileText::_VPrintf( lpctstr pFormat, va_list args )
{
    ADDTOCALLSTACK("CSFileText::_VPrintf");
    ASSERT(pFormat);

    if ( !_IsFileOpen() )
        return -1;

    return vfprintf( _pStream, pFormat, args );
}

int CSFileText::VPrintf(lpctstr pFormat, va_list args)
{
    ADDTOCALLSTACK("CSFileText::VPrintf");
    ASSERT(pFormat);

    MT_UNIQUE_LOCK_RETURN(CSFileText::_VPrintf(pFormat, args));
}

bool CSFileText::_Write( const void * pData, int iLen )
{
    // RETURN: 1 = success else fail.
    ADDTOCALLSTACK("CSFileText::_Write");
    ASSERT(pData);

    if ( !_IsFileOpen() )
        return false;

#ifdef _WIN32 // Windows flushing, the only safe mode to cancel it ;)
    if ( !_fNoBuffer )
    {
        setvbuf(_pStream, nullptr, _IONBF, 0);
        _fNoBuffer = true;
    }
#endif
    size_t uiStatus = fwrite( pData, iLen, 1, _pStream );
#ifndef _WIN32	// However, in unix, it works
    fflush( _pStream );
#endif
    return ( uiStatus == 1 );
}

bool CSFileText::Write(const void* pData, int iLen)
{
    // RETURN: 1 = success else fail.
    ADDTOCALLSTACK("CSFileText::Write");
    MT_UNIQUE_LOCK_RETURN(CSFileText::_Write(pData, iLen));
}

bool CSFileText::_WriteString( lpctstr pStr )
{
    // RETURN: < 0 = failed.
    ADDTOCALLSTACK("CSFileText::_WriteString");
    ASSERT(pStr);

    return _Write( pStr, (int)strlen( pStr ) );
}

bool CSFileText::WriteString(lpctstr pStr)
{
    ADDTOCALLSTACK("CSFileText::WriteString");
    MT_UNIQUE_LOCK_RETURN(CSFileText::_WriteString(pStr));
}

// CSFileText:: Mode operations.

lpctstr CSFileText::_GetModeStr() const
{
    ADDTOCALLSTACK("CSFileText::_GetModeStr");
    // end of line translation is crap. ftell and fseek don't work correctly when you use it.
    // fopen() args
    if ( IsBinaryMode())
        return ( _IsWriteMode() ? "wb" : "rb" );
    if ( _GetMode() & OF_READWRITE )
        return "a+b";
    if ( _GetMode() & OF_CREATE )
        return "w";
    if ( _IsWriteMode() )
        return "w";
    else
        return "rb";	// don't parse out the \n\r
}
