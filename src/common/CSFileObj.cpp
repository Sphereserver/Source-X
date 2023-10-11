#include "../sphere/threads.h"
#include "CScript.h"
#include "CLog.h"
#include "CException.h"
#include "CExpression.h"
#include "CSFileObj.h"


enum FO_TYPE
{
#define ADD(a,b) FO_##a,
#include "../tables/CSFileObj_props.tbl"
#undef ADD
    FO_QTY
};

lpctstr const CSFileObj::sm_szLoadKeys[FO_QTY+1] =
{
#define ADD(a,b) b,
#include "../tables/CSFileObj_props.tbl"
#undef ADD
    nullptr
};

enum FOV_TYPE
{
#define ADD(a,b) FOV_##a,
#include "../tables/CSFileObj_functions.tbl"
#undef ADD
    FOV_QTY
};

lpctstr const CSFileObj::sm_szVerbKeys[FOV_QTY+1] =
{
#define ADD(a,b) b,
#include "../tables/CSFileObj_functions.tbl"
#undef ADD
    nullptr
};

CSFileObj::CSFileObj()
{
    _pFile = new CSFileText();
    _ptcReadBuffer = new tchar [SCRIPT_MAX_LINE_LEN];
    _psWriteBuffer = new CSString();
    SetDefaultMode();
}

CSFileObj::~CSFileObj()
{
    if (_pFile->IsFileOpen())
        _pFile->Close();

    delete _psWriteBuffer;
    delete[] _ptcReadBuffer;
    delete _pFile;
}

void CSFileObj::SetDefaultMode()
{
    ADDTOCALLSTACK("CSFileObj::SetDefaultMode");
    _fAppend = true; _fCreate = false;
    _fRead = true; _fWrite = true;
}

tchar * CSFileObj::GetReadBuffer(bool fDelete)
{
    ADDTOCALLSTACK("CSFileObj::GetReadBuffer");
    if ( fDelete )
    {
        memset(this->_ptcReadBuffer, 0, SCRIPT_MAX_LINE_LEN);
    }
    else
    {
        *_ptcReadBuffer = 0;
    }

    return _ptcReadBuffer;
}

CSString * CSFileObj::GetWriteBuffer()
{
    ADDTOCALLSTACK("CSFileObj::GetWriteBuffer");
    if ( !_psWriteBuffer )
        _psWriteBuffer = new CSString();

    _psWriteBuffer->Clear( (_psWriteBuffer->GetLength() > (SCRIPT_MAX_LINE_LEN/4)) );

    return _psWriteBuffer;
}

bool CSFileObj::IsInUse()
{
    ADDTOCALLSTACK("CSFileObj::IsInUse");
    return _pFile->IsFileOpen();
}

void CSFileObj::FlushAndClose()
{
    ADDTOCALLSTACK("CSFileObj::FlushAndClose");
    if ( _pFile->IsFileOpen() )
    {
        _pFile->Flush();
        _pFile->Close();
    }
}

bool CSFileObj::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
    UnreferencedParameter(ptcKey);
    UnreferencedParameter(pRef);
    return false;
}

bool CSFileObj::_OnTick()		{ return true; }
int CSFileObj::FixWeirdness()	{ return 0; }

bool CSFileObj::r_LoadVal( CScript & s )
{
    ADDTOCALLSTACK("CSFileObj::r_LoadVal");
    EXC_TRY("LoadVal");
    lpctstr ptcKey = s.GetKey();

    if ( !strnicmp("MODE.",ptcKey,5) )
    {
        ptcKey += 5;
        if ( ! _pFile->IsFileOpen() )
        {
            if ( !strnicmp("APPEND",ptcKey,6) )
            {
                _fAppend = (s.GetArgVal() != 0);
                _fCreate = false;
            }
            else if ( !strnicmp("CREATE",ptcKey,6) )
            {
                _fCreate = (s.GetArgVal() != 0);
                _fAppend = false;
            }
            else if ( !strnicmp("READFLAG",ptcKey,8) )
                _fRead = (s.GetArgVal() != 0);
            else if ( !strnicmp("WRITEFLAG",ptcKey,9) )
                _fWrite = (s.GetArgVal() != 0);
            else if ( !strnicmp("SETDEFAULT",ptcKey,7) )
                SetDefaultMode();
            else
                return false;

            return true;
        }
        else
        {
            g_Log.Event(LOGL_ERROR, "FILE (%s): Cannot set mode after file opening\n", _pFile->GetFilePath());
        }
        return false;
    }

    int index = FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );

    switch ( index )
    {
        case FO_WRITE:
        case FO_WRITECHR:
        case FO_WRITELINE:
        {
            const bool fLine = (index == FO_WRITELINE);
            const bool fChr = (index == FO_WRITECHR);

            if ( !_pFile->IsFileOpen() )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Cannot write content. Open the file first.\n");
                return false;
            }

            if ( !s.HasArgs() )
                return false;

            CSString * pcsWriteBuf = this->GetWriteBuffer();

            if ( fLine )
            {
                pcsWriteBuf->Copy( s.GetArgStr() );
#ifdef _WIN32
                pcsWriteBuf->Add( "\r\n" );
#else
                pcsWriteBuf->Add( "\n" );
#endif
            }
            else if ( fChr )
            {
                pcsWriteBuf->Format( "%c", s.GetArgCVal() );
            }
            else
            {
                pcsWriteBuf->Copy( s.GetArgStr() );
            }

            bool fSuccess = false;

            if ( fChr )
            {
                fSuccess = _pFile->Write(pcsWriteBuf->GetBuffer(), 1);
            }
            else
            {
                fSuccess = _pFile->WriteString( pcsWriteBuf->GetBuffer() );
            }

            if ( !fSuccess )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Failed writing to \"%s\".\n", _pFile->GetFilePath());
                return false;
            }
        } break;

        default:
            return false;
    }

    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

bool CSFileObj::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(pSrc);
    UnreferencedParameter(fNoCallParent);
    UnreferencedParameter(fNoCallChildren);
    ADDTOCALLSTACK("CSFileObj::r_WriteVal");
    EXC_TRY("WriteVal");
    ASSERT(ptcKey != nullptr);

    if ( !strnicmp("MODE.",ptcKey,5) )
    {
        ptcKey += 5;
        if ( !strnicmp("APPEND",ptcKey,6) )
            sVal.FormatVal( _fAppend );
        else if ( !strnicmp("CREATE",ptcKey,6) )
            sVal.FormatVal( _fCreate );
        else if ( !strnicmp("READFLAG",ptcKey,8) )
            sVal.FormatVal( _fRead );
        else if ( !strnicmp("WRITEFLAG",ptcKey,9) )
            sVal.FormatVal( _fWrite );
        else
            return false;

        return true;
    }

    int index = FindTableHeadSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );

    switch ( index )
    {
        case FO_FILEEXIST:
        {
            ptcKey += 9;
            ptcKey = Str_GetUnQuoted(const_cast<tchar *>(ptcKey));
            if ( !ptcKey || !strlen(ptcKey) )
                return false;

            CSFile * pFileTest = new CSFile();
            sVal.FormatVal(pFileTest->Open(ptcKey));

            delete pFileTest;
        } break;

        case FO_FILELINES:
        {
            ptcKey += 9;
            ptcKey = Str_GetUnQuoted(const_cast<tchar *>(ptcKey));
            if ( !ptcKey || !strlen(ptcKey) )
                return false;

            CSFileText * sFileLine = new CSFileText();
            if ( !sFileLine->Open(ptcKey, OF_READ|OF_TEXT) )
            {
                delete sFileLine;
                return false;
            }

            tchar * ppArg = this->GetReadBuffer();
            int iLines = 0;

            while ( ! sFileLine->IsEOF() )
            {
                sFileLine->ReadString( ppArg, SCRIPT_MAX_LINE_LEN );
                ++iLines;
            }
            sFileLine->Close();

            sVal.FormatVal( iLines );

            delete sFileLine;
        } break;

        case FO_FILEPATH:
            if (_pFile->IsFileOpen())
                sVal.Copy(_pFile->GetFilePath());
            else
                sVal.Clear();
            break;

        case FO_INUSE:
            sVal.FormatVal( _pFile->IsFileOpen() );
            break;

        case FO_ISEOF:
            sVal.FormatVal( _pFile->IsEOF() );
            break;

        case FO_LENGTH:
            sVal.FormatVal( _pFile->IsFileOpen() ? _pFile->GetLength() : -1 );
            break;

        case FO_OPEN:
        {
            ptcKey += strlen(sm_szLoadKeys[index]);
            ptcKey = Str_GetUnQuoted(const_cast<tchar *>(ptcKey));
            if ( !ptcKey || !strlen(ptcKey) )
                return false;

            if ( _pFile->IsFileOpen() )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Cannot open file (%s). First close \"%s\".\n", ptcKey, _pFile->GetFilePath());
                return false;
            }

            sVal.FormatVal( FileOpen(ptcKey) );
        } break;

        case FO_POSITION:
            sVal.FormatVal( _pFile->GetPosition() );
            break;

        case FO_READBYTE:
        case FO_READCHAR:
        {
            bool fChr = ( index == FO_READCHAR );
            int iRead = 1;

            if ( !fChr )
            {
                ptcKey += strlen(sm_szLoadKeys[index]);
                GETNONWHITESPACE( ptcKey );

                iRead = Exp_GetVal(ptcKey);
                if ( iRead <= 0 || iRead >= SCRIPT_MAX_LINE_LEN)
                    return false;
            }

            if ( ( (_pFile->GetPosition() + iRead) > _pFile->GetLength() ) || _pFile->IsEOF() )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Failed reading %d bytes from \"%s\". Too near to EOF.\n", iRead, _pFile->GetFilePath());
                return false;
            }

            tchar * psReadBuf = this->GetReadBuffer(true);

            if ( iRead != _pFile->Read(psReadBuf, iRead) )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Failed reading %d bytes from \"%s\".\n", iRead, _pFile->GetFilePath());
                return false;
            }

            if ( fChr )
                sVal.FormatVal( *psReadBuf );
            else
                sVal.Copy(psReadBuf);
        } break;

        case FO_READLINE:
        {
            ptcKey += 8;
            GETNONWHITESPACE( ptcKey );

            tchar * psReadBuf = this->GetReadBuffer();
            ASSERT(psReadBuf != nullptr);

            int iLines = Exp_GetVal(ptcKey);
            if ( iLines < 0 )
                return false;

            int iSeek = _pFile->GetPosition();
            _pFile->SeekToBegin();

            if ( iLines == 0 )
            {
                while ( ! _pFile->IsEOF() )
                {
                    _pFile->ReadString( psReadBuf, SCRIPT_MAX_LINE_LEN );
                }
            }
            else
            {
                for ( int x = 1; x <= iLines; ++x )
                {
                    if ( _pFile->IsEOF() )
                        break;

                    psReadBuf = this->GetReadBuffer();
                    _pFile->ReadString( psReadBuf, SCRIPT_MAX_LINE_LEN );
                }
            }

            _pFile->Seek(iSeek);

            size_t uiLinelen = strlen(psReadBuf);
            while ( uiLinelen > 0 )
            {
                --uiLinelen;
                char iChar = psReadBuf[uiLinelen];
                // iChar needs to be converted to unsigned for the use with isgraph because, if it's a valid UTF-8 but invalid ASCII char, it means that its value is > 127
                //  which is a quantity that will be represented as a negative number in a signed char. Passing a number < -1 to isgraph makes a debug assertion to fail.
                if ( isgraph((uchar)iChar) || (iChar == 0x20) || (iChar == '\t') )
                {
                    ++uiLinelen;
                    psReadBuf[uiLinelen] = '\0';
                    break;
                }
            }

            sVal.Format( "%s", psReadBuf );
        } break;

        case FO_SEEK:
        {
            ptcKey += 4;
            GETNONWHITESPACE(ptcKey);

            if (ptcKey[0] == '\0')
                return false;

            if ( !strnicmp("BEGIN", ptcKey, 5) )
            {
                sVal.FormatVal( _pFile->Seek(0, SEEK_SET) );
            }
            else if ( !strnicmp("END", ptcKey, 3) )
            {
                sVal.FormatVal( _pFile->Seek(0, SEEK_END) );
            }
            else
            {
                sVal.FormatVal( _pFile->Seek(Exp_GetVal(ptcKey), SEEK_SET) );
            }
        } break;

        default:
            return false;
    }

    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pSrc);
    EXC_DEBUG_END;
    return false;
}


bool CSFileObj::r_Verb( CScript & s, CTextConsole * pSrc )
{
    UnreferencedParameter(pSrc);
    ADDTOCALLSTACK("CSFileObj::r_Verb");
    EXC_TRY("Verb");
    ASSERT(pSrc);

    lpctstr ptcKey = s.GetKey();

    int index = FindTableSorted( ptcKey, sm_szVerbKeys, ARRAY_COUNT( sm_szVerbKeys )-1 );

    if ( index < 0 )
        return( this->r_LoadVal( s ) );

    switch ( index )
    {
        case FOV_CLOSE:
            if ( _pFile->IsFileOpen() )
                _pFile->Close();
            break;

        case FOV_DELETEFILE:
        {
            if ( !s.GetArgStr() )
                return false;

            if ( _pFile->IsFileOpen() && !strcmp(s.GetArgStr(), _pFile->GetFileTitle()) )
                return false;

            STDFUNC_UNLINK(s.GetArgRaw());
        } break;

        case FOV_FLUSH:
            if ( _pFile->IsFileOpen() )
                _pFile->Flush();
            break;

        default:
            return false;
    }

    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPTSRC;
    EXC_DEBUG_END;
    return false;
}

bool CSFileObj::FileOpen( lpctstr sPath )
{
    ADDTOCALLSTACK("CSFileObj::FileOpen");
    if ( _pFile->IsFileOpen() )
        return false;

    uint uiMode = OF_SHARE_DENY_NONE | OF_TEXT;

    if ( _fCreate )	// if we create, we can't append or read
    {
        uiMode |= OF_CREATE;
    }
    else
    {
        if (( _fRead && _fWrite ) || _fAppend )
        {
            uiMode |= OF_READWRITE;
        }
        else
        {
            if ( _fRead )
                uiMode |= OF_READ;
            else if ( _fWrite )
                uiMode |= OF_WRITE;
        }
    }

    return ( _pFile->Open(sPath, uiMode) );
}
