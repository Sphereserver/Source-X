#include "../sphere/threads.h"
#include "CLog.h"
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
    if (_pStream == nullptr)
    {
#ifdef _DEBUG
        g_Log.EventDebug("Can't open script file '%s'. Errno: %d, strerr: %s.\n", ptcFilename, errno, strerror(errno));
#endif
        return false;
    }

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
        static constexpr int iMaxFileLength = 5 * 1'000'000;
        const int iFileLength = _GetLength();
        ASSERT(iFileLength >= 0);
        bool fUTF = false, fFirstLine = true;
        if (iFileLength > iMaxFileLength)
        {
            g_Log.EventError("Single script file bigger than %d MB? Size is %d MB. Skipping.\n", iMaxFileLength / 1'000'000, iFileLength / 1'000'000);
        }
        else
        {
            // Fastest method: read the script file all at once.
            /*
            auto fileContentCopy = std::make_unique<char[]>((size_t)iFileLength + 1u);
            fread(fileContentCopy.get(), sizeof(char), (size_t)iFileLength, _pStream);
            */
            auto fileContentCopy = std::make_unique_for_overwrite<char[]>((size_t)iFileLength + 1u);
            fread(fileContentCopy.get(), sizeof(char), (size_t)iFileLength, _pStream);
            fileContentCopy[(size_t)iFileLength] = '\0';
            //if (iFileLength > 0) {
            //    fileContentCopy[(size_t)iFileLength] = '\0';
            //}

            // Allocate string vectors for each script line.
            _fileContent = new std::vector<std::string>;
            _fileContent->reserve(iFileLength / 25);

            const char *fileCursor = fileContentCopy.get();
            size_t uiFileCursorRemainingLegth = iFileLength;
            ssize_t iStrLen;
            for (;; fileCursor += (size_t)iStrLen, uiFileCursorRemainingLegth -= (size_t)iStrLen)
            {
                if (uiFileCursorRemainingLegth == 0)
                    break;

                iStrLen = sGetLine_StaticBuf(fileCursor, minimum(uiFileCursorRemainingLegth, SCRIPT_MAX_LINE_LEN));
                if (iStrLen < 0)
                break;

                if (iStrLen < 1 /*|| (fileCursor[iStrLen] != '\n') It can also be a '\0' value, but it might not be necessary to check for either of the two...*/)
                {
                    ++ iStrLen; // Skip \n
                    continue;
                }

                // first line may contain utf marker (byte order mark)
                if (fFirstLine && iStrLen >= 3 &&
                    (uchar)(fileCursor[0]) == 0xEF &&
                    (uchar)(fileCursor[1]) == 0xBB &&
                    (uchar)(fileCursor[2]) == 0xBF)
                {
                    fUTF = true;
                }

                const lpctstr str_start = (fUTF ? &fileCursor[3] : fileCursor);
                size_t len_to_copy = (size_t)iStrLen - (fUTF ? 3 : 0);
                while (len_to_copy > 0)
                {
                    const int ch = str_start[len_to_copy - 1];
                    if (ch == '\n' || ch == '\r')
                    {
                        // Do not copy that, but skip it.
                        len_to_copy -= 1;
                        iStrLen += 1;
                    }
                    else {
                        break;
                    }
                }

                if (len_to_copy == 0)
                    _fileContent->emplace_back();
                else
                    _fileContent->emplace_back(str_start, len_to_copy);
                fFirstLine = false;
                fUTF = false;
            }   // closes while

            ASSERT(uiFileCursorRemainingLegth == 0);    // Ensure i have consumed the whole file.
        }   // closes else

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
    MT_UNIQUE_LOCK_RETURN(CCacheableScriptFile::_Open(ptcFilename, uiModeFlags));
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
    MT_UNIQUE_LOCK_SET;
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

    MT_SHARED_LOCK_SET;
    if ( _useDefaultFile() )
        MT_RETURN(CSFileText::_IsFileOpen());

    MT_RETURN(!_fClosed);
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
    MT_SHARED_LOCK_RETURN(_IsEOF());
}

tchar * CCacheableScriptFile::_ReadString(tchar *pBuffer, int sizemax)
{
    // This function is called for each script line which is being parsed (so VERY frequently), and ADDTOCALLSTACK is expensive if called
    // this much often, so here it's to be preferred ADDTOCALLSTACK_DEBUG, even if we'll lose stack trace precision.
    //ADDTOCALLSTACK_DEBUG("CCacheableScriptFile::_ReadString");
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
    //ADDTOCALLSTACK_DEBUG("CCacheableScriptFile::ReadString");
    MT_UNIQUE_LOCK_RETURN(CCacheableScriptFile::_ReadString(pBuffer, sizemax));
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
    MT_UNIQUE_LOCK_SET;
    _dupeFrom(other);
}

bool CCacheableScriptFile::_HasCache() const
{
    if ((_fileContent == nullptr) /* || _fileContent->empty()  Exclude this: might just be an empty file!*/)
        return false;
    return true;
}
bool CCacheableScriptFile::HasCache() const
{
    MT_SHARED_LOCK_RETURN(_HasCache());
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
    MT_SHARED_LOCK_RETURN(_useDefaultFile());
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
    MT_UNIQUE_LOCK_RETURN(CCacheableScriptFile::_Seek(iOffset, iOrigin));
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
    MT_UNIQUE_LOCK_RETURN(CCacheableScriptFile::_GetPosition());
}
