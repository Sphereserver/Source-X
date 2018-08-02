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
    NULL
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
    NULL
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

    _psWriteBuffer->Empty( ( _psWriteBuffer->GetLength() > (SCRIPT_MAX_LINE_LEN/4) ) );

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

bool CSFileObj::r_GetRef( lpctstr & pszKey, CScriptObj * & pRef )
{
    UNREFERENCED_PARAMETER(pszKey);
    UNREFERENCED_PARAMETER(pRef);
    return false;
}

bool CSFileObj::OnTick()		{ return true; }
int CSFileObj::FixWeirdness()	{ return 0; }

bool CSFileObj::r_LoadVal( CScript & s )
{
    ADDTOCALLSTACK("CSFileObj::r_LoadVal");
    EXC_TRY("LoadVal");
    lpctstr pszKey = s.GetKey();

    if ( !strnicmp("MODE.",pszKey,5) )
    {
        pszKey += 5;
        if ( ! _pFile->IsFileOpen() )
        {
            if ( !strnicmp("APPEND",pszKey,6) )
            {
                _fAppend = (s.GetArgVal() != 0);
                _fCreate = false;
            }
            else if ( !strnicmp("CREATE",pszKey,6) )
            {
                _fCreate = (s.GetArgVal() != 0);
                _fAppend = false;
            }
            else if ( !strnicmp("READFLAG",pszKey,8) )
                _fRead = (s.GetArgVal() != 0);
            else if ( !strnicmp("WRITEFLAG",pszKey,9) )
                _fWrite = (s.GetArgVal() != 0);
            else if ( !strnicmp("SETDEFAULT",pszKey,7) )
                SetDefaultMode();
            else
                return false;

            return true;
        }
        else
        {
            g_Log.Event(LOGL_ERROR, "FILE (%s): Cannot set mode after file opening\n", static_cast<lpctstr>(_pFile->GetFilePath()));
        }
        return false;
    }

    int index = FindTableSorted( pszKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );

    switch ( index )
    {
        case FO_WRITE:
        case FO_WRITECHR:
        case FO_WRITELINE:
        {
            bool fLine = (index == FO_WRITELINE);
            bool fChr = (index == FO_WRITECHR);

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
                pcsWriteBuf->Format( "%c", static_cast<tchar>(s.GetArgVal()) );
            }
            else
            {
                pcsWriteBuf->Copy( s.GetArgStr() );
            }

            bool fSuccess = false;

            if ( fChr )
            {
                fSuccess = _pFile->Write(pcsWriteBuf->GetPtr(), 1);
            }
            else
            {
                fSuccess = _pFile->WriteString( pcsWriteBuf->GetPtr() );
            }

            if ( !fSuccess )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Failed writing to \"%s\".\n", static_cast<lpctstr>(_pFile->GetFilePath()));
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

bool CSFileObj::r_WriteVal( lpctstr pszKey, CSString &sVal, CTextConsole * pSrc )
{
    UNREFERENCED_PARAMETER(pSrc);
    ADDTOCALLSTACK("CSFileObj::r_WriteVal");
    EXC_TRY("WriteVal");
    ASSERT(pszKey != nullptr);

    if ( !strnicmp("MODE.",pszKey,5) )
    {
        pszKey += 5;
        if ( !strnicmp("APPEND",pszKey,6) )
            sVal.FormatVal( _fAppend );
        else if ( !strnicmp("CREATE",pszKey,6) )
            sVal.FormatVal( _fCreate );
        else if ( !strnicmp("READFLAG",pszKey,8) )
            sVal.FormatVal( _fRead );
        else if ( !strnicmp("WRITEFLAG",pszKey,9) )
            sVal.FormatVal( _fWrite );
        else
            return false;

        return true;
    }

    int index = FindTableHeadSorted( pszKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );

    switch ( index )
    {
        case FO_FILEEXIST:
        {
            pszKey += 9;
            GETNONWHITESPACE( pszKey );

            tchar * ppCmd = Str_TrimWhitespace(const_cast<tchar *>(pszKey));
            if ( !( ppCmd && strlen(ppCmd) ))
                return false;

            CSFile * pFileTest = new CSFile();
            sVal.FormatVal(pFileTest->Open(ppCmd));

            delete pFileTest;
        } break;

        case FO_FILELINES:
        {
            pszKey += 9;
            GETNONWHITESPACE( pszKey );

            tchar * ppCmd = Str_TrimWhitespace(const_cast<tchar *>(pszKey));
            if ( !( ppCmd && strlen(ppCmd) ))
                return false;

            CSFileText * sFileLine = new CSFileText();
            if ( !sFileLine->Open(ppCmd, OF_READ|OF_TEXT) )
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
            sVal.Format("%s", _pFile->IsFileOpen() ? static_cast<lpctstr>(_pFile->GetFilePath()) : "" );
            break;

        case FO_INUSE:
            sVal.FormatVal( _pFile->IsFileOpen() );
            break;

        case FO_ISEOF:
            sVal.FormatVal( _pFile->IsEOF() );
            break;

        case FO_LENGTH:
            sVal.FormatSTVal( _pFile->IsFileOpen() ? _pFile->GetLength() : -1 );
            break;

        case FO_OPEN:
        {
            pszKey += strlen(sm_szLoadKeys[index]);
            GETNONWHITESPACE( pszKey );

            tchar * ppCmd = Str_TrimWhitespace(const_cast<tchar *>(pszKey));
            if ( !( ppCmd && strlen(ppCmd) ))
                return false;

            if ( _pFile->IsFileOpen() )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Cannot open file (%s). First close \"%s\".\n", ppCmd, static_cast<lpctstr>(_pFile->GetFilePath()));
                return false;
            }

            sVal.FormatVal( FileOpen(ppCmd) );
        } break;

        case FO_POSITION:
            sVal.FormatSTVal( _pFile->GetPosition() );
            break;

        case FO_READBYTE:
        case FO_READCHAR:
        {
            bool bChr = ( index == FO_READCHAR );
            int iRead = 1;

            if ( !bChr )
            {
                pszKey += strlen(sm_szLoadKeys[index]);
                GETNONWHITESPACE( pszKey );

                iRead = Exp_GetVal(pszKey);
                if ( iRead <= 0 || iRead >= SCRIPT_MAX_LINE_LEN)
                    return false;
            }

            if ( ( ( _pFile->GetPosition() + iRead ) > _pFile->GetLength() ) || ( _pFile->IsEOF() ) )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Failed reading %" PRIuSIZE_T " byte from \"%s\". Too near to EOF.\n", iRead, static_cast<lpctstr>(_pFile->GetFilePath()));
                return false;
            }

            tchar * psReadBuf = this->GetReadBuffer(true);

            if ( iRead != _pFile->Read(psReadBuf, iRead) )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Failed reading %" PRIuSIZE_T " byte from \"%s\".\n", iRead, static_cast<lpctstr>(_pFile->GetFilePath()));
                return false;
            }

            if ( bChr )
                sVal.FormatVal( *psReadBuf );
            else
                sVal.Format( "%s", psReadBuf );
        } break;

        case FO_READLINE:
        {
            pszKey += 8;
            GETNONWHITESPACE( pszKey );

            tchar * psReadBuf = this->GetReadBuffer();
            ASSERT(psReadBuf != nullptr);

            int64 iLines = Exp_GetVal(pszKey);
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
                for ( int64 x = 1; x <= iLines; ++x )
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
                if ( isgraph(psReadBuf[uiLinelen]) || (psReadBuf[uiLinelen] == 0x20) || (psReadBuf[uiLinelen] == '\t') )
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
            pszKey += 4;
            GETNONWHITESPACE(pszKey);

            if (pszKey[0] == '\0')
                return false;

            if ( !strnicmp("BEGIN", pszKey, 5) )
            {
                sVal.FormatVal( _pFile->Seek(0, SEEK_SET) );
            }
            else if ( !strnicmp("END", pszKey, 3) )
            {
                sVal.FormatVal( _pFile->Seek(0, SEEK_END) );
            }
            else
            {
                sVal.FormatVal( _pFile->Seek(Exp_GetVal(pszKey), SEEK_SET) );
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
    UNREFERENCED_PARAMETER(pSrc);
    ADDTOCALLSTACK("CSFileObj::r_Verb");
    EXC_TRY("Verb");
    ASSERT(pSrc);

    lpctstr pszKey = s.GetKey();

    int index = FindTableSorted( pszKey, sm_szVerbKeys, CountOf( sm_szVerbKeys )-1 );

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

            if ( _pFile->IsFileOpen() && !strcmp(s.GetArgStr(),_pFile->GetFileTitle()) )
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
