
#include "../sphere/threads.h"
#include "CCacheableScriptFile.h"

#ifdef _WIN32
#include <io.h> // for _get_osfhandle (used by STDFUNC_FILENO)
#endif


CCacheableScriptFile::CCacheableScriptFile()
{
    _fileContent = nullptr;
    _fClosed = true;
    _fRealFile = false;
    _iCurrentLine = 0;
}

CCacheableScriptFile::~CCacheableScriptFile() 
{
    //_Close(); // No need to Close(), since it's already done by CSFileText destructor.
    if ( _fRealFile && _fileContent )   // be sure that i'm the original file and not a copy/link
    {
        delete _fileContent;
        _fileContent = nullptr;
    }
}

bool CCacheableScriptFile::_Open(lpctstr ptcFilename, uint uiModeFlags) 
{
    ADDTOCALLSTACK("CCacheableScriptFile::_Open");

    _uiMode = uiModeFlags;
    if ( _useDefaultFile() ) 
        return CSFileText::_Open(ptcFilename, uiModeFlags);

    if ( !ptcFilename )
    {
        return _IsFileOpen();
    }
    else
    {
        _Close();	// make sure it's closed first
    }

    _strFileName = ptcFilename;
    lpctstr ptcModeStr = _GetModeStr();
    _pStream = fopen(ptcFilename, ptcModeStr);
    if ( _pStream == nullptr ) 
        return false;

    _fileDescriptor = (file_descriptor_t)STDFUNC_FILENO(_pStream);
    _fClosed = false;
    _fRealFile = true;  // this is the original CCacheableScriptFile that will be referenced from others

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
            ASSERT(SCRIPT_MAX_LINE_LEN < INT_MAX);
            if (uiStrLen > SCRIPT_MAX_LINE_LEN)
                uiStrLen = SCRIPT_MAX_LINE_LEN;

            // first line may contain utf marker (byte order mark)
            if (fFirstLine && uiStrLen >= 3 &&
                (uchar)(buf[0]) == 0xEF &&
                (uchar)(buf[1]) == 0xBB &&
                (uchar)(buf[2]) == 0xBF)
            {
                fUTF = true;
            }

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
bool CCacheableScriptFile::Open(lpctstr ptcFilename, uint uiModeFlags) 
{
    ADDTOCALLSTACK("CCacheableScriptFile::Open");
    THREAD_UNIQUE_LOCK_RETURN(CCacheableScriptFile::_Open(ptcFilename, uiModeFlags));
}

void CCacheableScriptFile::_Close()
{
    ADDTOCALLSTACK("CCacheableScriptFile::_Close");
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
void CCacheableScriptFile::Close()
{
    ADDTOCALLSTACK("CCacheableScriptFile::Close");
    THREAD_UNIQUE_LOCK_SET;
    _Close();
}

bool CCacheableScriptFile::_IsFileOpen() const 
{
    ADDTOCALLSTACK("CCacheableScriptFile::_IsFileOpen");

    if ( _useDefaultFile() ) 
        return CSFileText::_IsFileOpen();

    return (!_fClosed);
}
bool CCacheableScriptFile::IsFileOpen() const 
{
    ADDTOCALLSTACK("CCacheableScriptFile::IsFileOpen");

    THREAD_SHARED_LOCK_SET;
    if ( _useDefaultFile() ) 
        TS_RETURN(CSFileText::_IsFileOpen());

    TS_RETURN(!_fClosed);
}

bool CCacheableScriptFile::_IsEOF() const 
{
    ADDTOCALLSTACK("CCacheableScriptFile::_IsEOF");
    if ( _useDefaultFile() ) 
        return CSFileText::_IsEOF();

    return (_fileContent->empty() || ((uint)_iCurrentLine == _fileContent->size()) );
}
bool CCacheableScriptFile::IsEOF() const 
{
    ADDTOCALLSTACK("CCacheableScriptFile::IsEOF");
    THREAD_SHARED_LOCK_RETURN(_IsEOF());
}

tchar * CCacheableScriptFile::_ReadString(tchar *pBuffer, int sizemax) 
{
    // This function is called for each script line which is being parsed (so VERY frequently), and ADDTOCALLSTACK is expensive if called
    // this much often, so here it's to be preferred ADDTOCALLSTACK_INTENSIVE, even if we'll lose stack trace precision.
    ADDTOCALLSTACK_INTENSIVE("CCacheableScriptFile::_ReadString");

    if ( _useDefaultFile() ) 
        return CSFileText::_ReadString(pBuffer, sizemax);

    *pBuffer = '\0';

    if ( !_fileContent->empty() && ((uint)_iCurrentLine < _fileContent->size()) )
    {
        //strcpy(pBuffer, (*_fileContent)[_iCurrentLine].c_str() );
        strcpy(pBuffer, _fileContent->data()[_iCurrentLine].c_str()); // may be faster with MS compiler
        ++_iCurrentLine;
    }
    else
    {
        return nullptr;
    }

    return pBuffer;
}

tchar * CCacheableScriptFile::ReadString(tchar *pBuffer, int sizemax) 
{
    ADDTOCALLSTACK_INTENSIVE("CCacheableScriptFile::ReadString");
    THREAD_UNIQUE_LOCK_RETURN(_ReadString(pBuffer, sizemax));
}

void CCacheableScriptFile::_dupeFrom(CCacheableScriptFile *other) 
{
    if ( _useDefaultFile() ) 
        return;

    _strFileName = other->_strFileName;
    _fClosed = other->_fClosed;
    _fRealFile = false;
    _fileContent = other->_fileContent;
}
void CCacheableScriptFile::dupeFrom(CCacheableScriptFile *other) 
{
    THREAD_UNIQUE_LOCK_SET;
    _dupeFrom(other);
}

bool CCacheableScriptFile::_HasCache() const
{
    if ((_fileContent == nullptr) || _fileContent->empty())
        return false;
    return true;
}
bool CCacheableScriptFile::HasCache() const
{
    THREAD_SHARED_LOCK_RETURN(_HasCache());
}

bool CCacheableScriptFile::_useDefaultFile() const 
{
    if ( _IsWriteMode() || ( _GetFullMode() & OF_DEFAULTMODE )) 
        return true;
    return false;
}
/*
bool CCacheableScriptFile::useDefaultFile() const 
{
    THREAD_SHARED_LOCK_RETURN(_useDefaultFile());
}*/

int CCacheableScriptFile::_Seek(int iOffset, int iOrigin)
{
    ADDTOCALLSTACK("CCacheableScriptFile::_Seek");
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
int CCacheableScriptFile::Seek(int iOffset, int iOrigin)
{
    ADDTOCALLSTACK("CCacheableScriptFile::Seek");
    THREAD_UNIQUE_LOCK_RETURN(CCacheableScriptFile::_Seek(iOffset, iOrigin));
}

int CCacheableScriptFile::_GetPosition() const
{
    ADDTOCALLSTACK("CCacheableScriptFile::_GetPosition");
    if (_useDefaultFile())
        return CSFileText::_GetPosition();  // this requires a unique lock

    return _iCurrentLine;
}
int CCacheableScriptFile::GetPosition() const
{
    ADDTOCALLSTACK("CCacheableScriptFile::GetPosition");
    THREAD_UNIQUE_LOCK_RETURN(_GetPosition());
}
