
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
        _fileContent = nullptr;
    }

    if ((uiModeFlags & OF_WRITE) || (uiModeFlags & OF_READWRITE))
    {
        ;
    }
    else
    {
        TemporaryString tsBuf;
        tchar* ptcBuf = tsBuf.buffer();
        size_t uiStrLen;
        bool fUTF = false, fFirstLine = true;
        const int iFileLength = _GetLength();
        _fileContent = new std::vector<std::string>;
        _fileContent->reserve(iFileLength / 20);

        while ( !feof(_pStream) ) 
        {
            tsBuf.setAt(0, '\0');
            fgets(ptcBuf, SCRIPT_MAX_LINE_LEN, _pStream);
            uiStrLen = strlen(ptcBuf);
            ASSERT(SCRIPT_MAX_LINE_LEN < INT_MAX);
            if (uiStrLen > SCRIPT_MAX_LINE_LEN)
                uiStrLen = SCRIPT_MAX_LINE_LEN;

            // first line may contain utf marker (byte order mark)
            if (fFirstLine && uiStrLen >= 3 &&
                (uchar)(ptcBuf[0]) == 0xEF &&
                (uchar)(ptcBuf[1]) == 0xBB &&
                (uchar)(ptcBuf[2]) == 0xBF)
            {
                fUTF = true;
            }

            const lpctstr str_start = (fUTF ? &ptcBuf[3] : ptcBuf);
            size_t len_to_copy = uiStrLen - (fUTF ? 3 : 0);
            while (len_to_copy > 0)
            {
                const int ch = str_start[len_to_copy - 1];
                if (ch == '\n' || ch == '\r')
                    len_to_copy -= 1;
                else
                    break;
            }
            if (len_to_copy == 0)
                _fileContent->emplace_back();
            else
                _fileContent->emplace_back(str_start, len_to_copy);
            fFirstLine = false;
            fUTF = false;
        }

        fclose(_pStream);
        _pStream = nullptr;
        _fileDescriptor = _kInvalidFD;
        _uiMode = 0;
        _iCurrentLine = 0;
        _fileContent->shrink_to_fit();
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
    //ADDTOCALLSTACK("CCacheableScriptFile::_IsEOF");
    if ( _useDefaultFile() ) 
        return CSFileText::_IsEOF();

    return (_fileContent->empty() || ((uint)_iCurrentLine == _fileContent->size()) );
}
bool CCacheableScriptFile::IsEOF() const 
{
    //ADDTOCALLSTACK("CCacheableScriptFile::IsEOF");
    THREAD_SHARED_LOCK_RETURN(_IsEOF());
}

tchar * CCacheableScriptFile::_ReadString(tchar *pBuffer, int sizemax) 
{
    // This function is called for each script line which is being parsed (so VERY frequently), and ADDTOCALLSTACK is expensive if called
    // this much often, so here it's to be preferred ADDTOCALLSTACK_INTENSIVE, even if we'll lose stack trace precision.
    //ADDTOCALLSTACK_INTENSIVE("CCacheableScriptFile::_ReadString");
    ASSERT(sizemax > 0);
    if ( _useDefaultFile() ) 
        return CSFileText::_ReadString(pBuffer, sizemax);

    //*pBuffer = '\0';
    ASSERT(_fileContent);

    if (_fileContent->empty() || ((uint)_iCurrentLine >= _fileContent->size()))
        return nullptr;
    
    std::string const& cur_line = (*_fileContent)[_iCurrentLine];
    ++_iCurrentLine;
    if (cur_line.empty())
        return pBuffer;

    size_t bytes_to_copy = cur_line.size();
    if (bytes_to_copy >= size_t(sizemax))
        bytes_to_copy = size_t(sizemax) - 1;
    if (cur_line.size() != 0)
    {
        const lpctstr cur_line_cstr = cur_line.c_str();
        size_t copied = Str_CopyLimit(pBuffer, cur_line_cstr, bytes_to_copy);
        pBuffer[copied] = '\0';
    }

    return pBuffer;
}

tchar * CCacheableScriptFile::ReadString(tchar *pBuffer, int sizemax) 
{
    //ADDTOCALLSTACK_INTENSIVE("CCacheableScriptFile::ReadString");
    THREAD_UNIQUE_LOCK_RETURN(CCacheableScriptFile::_ReadString(pBuffer, sizemax));
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
        return CSFile::_GetPosition();  // this requires a unique lock

    return _iCurrentLine;
}
int CCacheableScriptFile::GetPosition() const
{
    ADDTOCALLSTACK("CCacheableScriptFile::GetPosition");
    THREAD_UNIQUE_LOCK_RETURN(CCacheableScriptFile::_GetPosition());
}
