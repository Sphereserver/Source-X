
#include "../sphere/threads.h"
#include "CacheableScriptFile.h"

#ifdef _WIN32
#include <io.h> // for _get_osfhandle (used by STDFUNC_FILENO)
#endif


CacheableScriptFile::CacheableScriptFile()
{
    _fileContent = nullptr;
    _fClosed = true;
    _fRealFile = false;
    _iCurrentLine = 0;
}

CacheableScriptFile::~CacheableScriptFile() 
{
    Close();
    if ( _fRealFile && _fileContent )   // be sure that i'm the original file and not a copy/link
    {
        delete _fileContent;
        _fileContent = nullptr;
    }
}

bool CacheableScriptFile::_Open(lpctstr ptcFilename, uint uiModeFlags) 
{
    ADDTOCALLSTACK("CacheableScriptFile::_Open");

    if ( _useDefaultFile() ) 
        return CSFileText::_Open(ptcFilename, uiModeFlags);

    if ( !ptcFilename )
    {
        if ( _IsFileOpen() )
            return true;
    }
    else
    {
        _Close();	// make sure it's closed first
    }

    _strFileName = ptcFilename;
    _uiMode = uiModeFlags;
    lpctstr ptcModeStr = _GetModeStr();
    _pStream = fopen(ptcFilename, ptcModeStr);
    if ( _pStream == nullptr ) 
        return false;

    _fileDescriptor = (file_descriptor_t)STDFUNC_FILENO(_pStream);
    _fClosed = false;
    _fRealFile = true;  // this is the original CacheableScriptFile that will be referenced from others

    // If the file is opened in read mode, it's opened, cached and closed. No need to close it manually unless
    //  you want to delete the cached file content.
    // If the file is opened in write mode, it's simply opened, and no more. You'll need to close it manually
    //  when you're done with the writing operations.
    if (_fileContent)
    {
        delete _fileContent;
    }
    if ((uiModeFlags & OF_WRITE) || (uiModeFlags & OF_READWRITE))
    {
        _fileContent = nullptr;
    }
    else
    {
        TemporaryString buf;
        size_t uiStrLen;
        bool fUTF = false, fFirstLine = true;
        int iFileLength = _GetLength();
        _fileContent = new std::vector<std::string>();
        _fileContent->reserve(iFileLength);

        while ( !feof(_pStream) ) 
        {
            buf.setAt(0, '\0');
            fgets(buf, SCRIPT_MAX_LINE_LEN, _pStream);
            uiStrLen = strlen(buf);
            if (uiStrLen > INT_MAX)
                uiStrLen = INT_MAX;

            // first line may contain utf marker
            if ( fFirstLine && uiStrLen >= 3 &&
                (uchar)(buf[0]) == 0xEF &&
                (uchar)(buf[1]) == 0xBB &&
                (uchar)(buf[2]) == 0xBF )
                fUTF = true;

            _fileContent->emplace_back( (fUTF ? &buf[3]:buf), uiStrLen - (fUTF ? 3:0) );
            fFirstLine = false;
            fUTF = false;
        }

        fclose(_pStream);
        _pStream = nullptr;
        _fileDescriptor = _kInvalidFD;
        _uiMode = 0;
        _iCurrentLine = 0;
    }

    return true;
}
bool CacheableScriptFile::Open(lpctstr ptcFilename, uint uiModeFlags) 
{
    ADDTOCALLSTACK("CacheableScriptFile::Open");
    THREAD_UNIQUE_LOCK_RETURN(CacheableScriptFile::_Open(ptcFilename, uiModeFlags));
}

void CacheableScriptFile::_Close()
{
    ADDTOCALLSTACK("CacheableScriptFile::_Close");
    if ( _useDefaultFile() )
    {
        CSFileText::_Close();
    }
    else 
    {
        _iCurrentLine = 0;
        _fClosed = true;
    }
}
void CacheableScriptFile::Close()
{
    ADDTOCALLSTACK("CacheableScriptFile::Close");
    THREAD_UNIQUE_LOCK_SET;
    _Close();
}

bool CacheableScriptFile::_IsFileOpen() const 
{
    ADDTOCALLSTACK("CacheableScriptFile::_IsFileOpen");

    if ( _useDefaultFile() ) 
        return CSFileText::_IsFileOpen();

    return (!_fClosed);
}
bool CacheableScriptFile::IsFileOpen() const 
{
    ADDTOCALLSTACK("CacheableScriptFile::IsFileOpen");

    THREAD_SHARED_LOCK_SET;
    if ( _useDefaultFile() ) 
        TS_RETURN(CSFileText::_IsFileOpen());

    TS_RETURN(!_fClosed);
}

bool CacheableScriptFile::_IsEOF() const 
{
    ADDTOCALLSTACK_INTENSIVE("CacheableScriptFile::_IsEOF");
    if ( _useDefaultFile() ) 
        return CSFileText::_IsEOF();

    return (_fileContent->empty() || ((uint)_iCurrentLine == _fileContent->size()) );
}
bool CacheableScriptFile::IsEOF() const 
{
    ADDTOCALLSTACK_INTENSIVE("CacheableScriptFile::IsEOF");
    THREAD_SHARED_LOCK_RETURN(_IsEOF());
}

tchar * CacheableScriptFile::_ReadString(tchar *pBuffer, int sizemax) 
{
    ADDTOCALLSTACK("CacheableScriptFile::_ReadString");

    if ( _useDefaultFile() ) 
        return CSFileText::_ReadString(pBuffer, sizemax);

    *pBuffer = '\0';

    if ( !_fileContent->empty() && ((uint)_iCurrentLine < _fileContent->size()) )
    {
        strcpy(pBuffer, (*_fileContent)[_iCurrentLine].c_str() );
        ++_iCurrentLine;
    }
    else
    {
        return nullptr;
    }

    return pBuffer;
}

tchar * CacheableScriptFile::ReadString(tchar *pBuffer, int sizemax) 
{
    ADDTOCALLSTACK("CacheableScriptFile::ReadString");
    THREAD_UNIQUE_LOCK_RETURN(_ReadString(pBuffer, sizemax));
}

void CacheableScriptFile::_dupeFrom(CacheableScriptFile *other) 
{
    if ( _useDefaultFile() ) 
        return;

    _fClosed = other->_fClosed;
    _fRealFile = false;
    _fileContent = other->_fileContent;
}
void CacheableScriptFile::dupeFrom(CacheableScriptFile *other) 
{
    THREAD_UNIQUE_LOCK_SET;
    _dupeFrom(other);
}

bool CacheableScriptFile::_HasCache() const
{
    if ((_fileContent == nullptr) || _fileContent->empty())
        return false;
    return true;
}
bool CacheableScriptFile::HasCache() const
{
    THREAD_SHARED_LOCK_RETURN(_HasCache());
}

bool CacheableScriptFile::_useDefaultFile() const 
{
    if ( _IsWriteMode() || ( _GetFullMode() & OF_DEFAULTMODE )) 
        return true;
    return false;
}
/*
bool CacheableScriptFile::useDefaultFile() const 
{
    THREAD_SHARED_LOCK_RETURN(_useDefaultFile());
}*/

int CacheableScriptFile::_Seek(int iOffset, int iOrigin)
{
    ADDTOCALLSTACK("CacheableScriptFile::_Seek");
    if (_useDefaultFile())
        return CSFileText::_Seek(iOffset, iOrigin);

    int iLinenum = iOffset;
    if (iOrigin != SEEK_SET)
        iLinenum = 0;	//	do not support not SEEK_SET rotation

    if ( _fileContent && ((uint)iLinenum <= _fileContent->size()) )
    {
        _iCurrentLine = iLinenum;
        return iLinenum;
    }

    return 0;
}
int CacheableScriptFile::Seek(int iOffset, int iOrigin)
{
    ADDTOCALLSTACK("CacheableScriptFile::Seek");
    THREAD_UNIQUE_LOCK_RETURN(CacheableScriptFile::_Seek(iOffset, iOrigin));
}

int CacheableScriptFile::_GetPosition() const
{
    ADDTOCALLSTACK("CacheableScriptFile::_GetPosition");
    if (_useDefaultFile())
        return CSFileText::_GetPosition();  // this requires a unique lock

    return _iCurrentLine;
}
int CacheableScriptFile::GetPosition() const
{
    ADDTOCALLSTACK("CacheableScriptFile::GetPosition");
    THREAD_UNIQUE_LOCK_RETURN(_GetPosition());
}
