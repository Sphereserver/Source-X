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
    sWrite = new CSFileText();
    tBuffer = new tchar [SCRIPT_MAX_LINE_LEN];
    cgWriteBuffer = new CSString();
    SetDefaultMode();
}

CSFileObj::~CSFileObj()
{
    if (sWrite->IsFileOpen())
        sWrite->Close();

    delete cgWriteBuffer;
    delete[] tBuffer;
    delete sWrite;
}

void CSFileObj::SetDefaultMode(void)
{
    ADDTOCALLSTACK("CSFileObj::SetDefaultMode");
    bAppend = true; bCreate = false;
    bRead = true; bWrite = true;
}

tchar * CSFileObj::GetReadBuffer(bool bDelete = false)
{
    ADDTOCALLSTACK("CSFileObj::GetReadBuffer");
    if ( bDelete )
        memset(this->tBuffer, 0, SCRIPT_MAX_LINE_LEN);
    else
        *tBuffer = 0;

    return tBuffer;
}

CSString * CSFileObj::GetWriteBuffer(void)
{
    ADDTOCALLSTACK("CSFileObj::GetWriteBuffer");
    if ( !cgWriteBuffer )
        cgWriteBuffer = new CSString();

    cgWriteBuffer->Empty( ( cgWriteBuffer->GetLength() > (SCRIPT_MAX_LINE_LEN/4) ) );

    return cgWriteBuffer;
}

bool CSFileObj::IsInUse()
{
    ADDTOCALLSTACK("CSFileObj::IsInUse");
    return sWrite->IsFileOpen();
}

void CSFileObj::FlushAndClose()
{
    ADDTOCALLSTACK("CSFileObj::FlushAndClose");
    if ( sWrite->IsFileOpen() )
    {
        sWrite->Flush();
        sWrite->Close();
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
        if ( ! sWrite->IsFileOpen() )
        {
            if ( !strnicmp("APPEND",pszKey,6) )
            {
                bAppend = (s.GetArgVal() != 0);
                bCreate = false;
            }
            else if ( !strnicmp("CREATE",pszKey,6) )
            {
                bCreate = (s.GetArgVal() != 0);
                bAppend = false;
            }
            else if ( !strnicmp("READFLAG",pszKey,8) )
                bRead = (s.GetArgVal() != 0);
            else if ( !strnicmp("WRITEFLAG",pszKey,9) )
                bWrite = (s.GetArgVal() != 0);
            else if ( !strnicmp("SETDEFAULT",pszKey,7) )
                SetDefaultMode();
            else
                return false;

            return true;
        }
        else
        {
            g_Log.Event(LOGL_ERROR, "FILE (%s): Cannot set mode after file opening\n", static_cast<lpctstr>(sWrite->GetFilePath()));
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
            bool bLine = (index == FO_WRITELINE);
            bool bChr = (index == FO_WRITECHR);

            if ( !sWrite->IsFileOpen() )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Cannot write content. Open the file first.\n");
                return false;
            }

            if ( !s.HasArgs() )
                return false;

            CSString * pcsWriteBuf = this->GetWriteBuffer();

            if ( bLine )
            {
                pcsWriteBuf->Copy( s.GetArgStr() );
#ifdef _WIN32
                pcsWriteBuf->Add( "\r\n" );
#else
                pcsWriteBuf->Add( "\n" );
#endif
            }
            else if ( bChr )
            {
                pcsWriteBuf->Format( "%c", static_cast<tchar>(s.GetArgVal()) );
            }
            else
                pcsWriteBuf->Copy( s.GetArgStr() );

            bool bSuccess = false;

            if ( bChr )
                bSuccess = sWrite->Write(pcsWriteBuf->GetPtr(), 1);
            else
                bSuccess = sWrite->WriteString( pcsWriteBuf->GetPtr() );

            if ( !bSuccess )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Failed writing to \"%s\".\n", static_cast<lpctstr>(sWrite->GetFilePath()));
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
            sVal.FormatVal( bAppend );
        else if ( !strnicmp("CREATE",pszKey,6) )
            sVal.FormatVal( bCreate );
        else if ( !strnicmp("READFLAG",pszKey,8) )
            sVal.FormatVal( bRead );
        else if ( !strnicmp("WRITEFLAG",pszKey,9) )
            sVal.FormatVal( bWrite );
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
            sVal.Format("%s", sWrite->IsFileOpen() ? static_cast<lpctstr>(sWrite->GetFilePath()) : "" );
            break;

        case FO_INUSE:
            sVal.FormatVal( sWrite->IsFileOpen() );
            break;

        case FO_ISEOF:
            sVal.FormatVal( sWrite->IsEOF() );
            break;

        case FO_LENGTH:
            sVal.FormatSTVal( sWrite->IsFileOpen() ? sWrite->GetLength() : -1 );
            break;

        case FO_OPEN:
        {
            pszKey += strlen(sm_szLoadKeys[index]);
            GETNONWHITESPACE( pszKey );

            tchar * ppCmd = Str_TrimWhitespace(const_cast<tchar *>(pszKey));
            if ( !( ppCmd && strlen(ppCmd) ))
                return false;

            if ( sWrite->IsFileOpen() )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Cannot open file (%s). First close \"%s\".\n", ppCmd, static_cast<lpctstr>(sWrite->GetFilePath()));
                return false;
            }

            sVal.FormatVal( FileOpen(ppCmd) );
        } break;

        case FO_POSITION:
            sVal.FormatSTVal( sWrite->GetPosition() );
            break;

        case FO_READBYTE:
        case FO_READCHAR:
        {
            bool bChr = ( index == FO_READCHAR );
            size_t iRead = 1;

            if ( !bChr )
            {
                pszKey += strlen(sm_szLoadKeys[index]);
                GETNONWHITESPACE( pszKey );

                iRead = Exp_GetVal(pszKey);
                if ( iRead <= 0 || iRead >= SCRIPT_MAX_LINE_LEN)
                    return false;
            }

            if ( ( ( sWrite->GetPosition() + iRead ) > sWrite->GetLength() ) || ( sWrite->IsEOF() ) )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Failed reading %" PRIuSIZE_T " byte from \"%s\". Too near to EOF.\n", iRead, static_cast<lpctstr>(sWrite->GetFilePath()));
                return false;
            }

            tchar * psReadBuf = this->GetReadBuffer(true);

            if ( iRead != sWrite->Read(psReadBuf, iRead) )
            {
                g_Log.Event(LOGL_ERROR, "FILE: Failed reading %" PRIuSIZE_T " byte from \"%s\".\n", iRead, static_cast<lpctstr>(sWrite->GetFilePath()));
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

            size_t uiSeek = sWrite->GetPosition();
            sWrite->SeekToBegin();

            if ( iLines == 0 )
            {
                while ( ! sWrite->IsEOF() )
                    sWrite->ReadString( psReadBuf, SCRIPT_MAX_LINE_LEN );
            }
            else
            {
                for ( int64 x = 1; x <= iLines; ++x )
                {
                    if ( sWrite->IsEOF() )
                        break;

                    psReadBuf = this->GetReadBuffer();
                    sWrite->ReadString( psReadBuf, SCRIPT_MAX_LINE_LEN );
                }
            }

            sWrite->Seek(uiSeek);

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
                sVal.FormatSTVal( sWrite->Seek(0, SEEK_SET) );
            }
            else if ( !strnicmp("END", pszKey, 3) )
            {
                sVal.FormatSTVal( sWrite->Seek(0, SEEK_END) );
            }
            else
            {
                sVal.FormatSTVal( sWrite->Seek(Exp_GetSTVal(pszKey), SEEK_SET) );
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
            if ( sWrite->IsFileOpen() )
                sWrite->Close();
            break;

        case FOV_DELETEFILE:
        {
            if ( !s.GetArgStr() )
                return false;

            if ( sWrite->IsFileOpen() && !strcmp(s.GetArgStr(),sWrite->GetFileTitle()) )
                return false;

            STDFUNC_UNLINK(s.GetArgRaw());
        } break;

        case FOV_FLUSH:
            if ( sWrite->IsFileOpen() )
                sWrite->Flush();
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
    if ( sWrite->IsFileOpen() )
        return false;

    uint uMode = OF_SHARE_DENY_NONE | OF_TEXT;

    if ( bCreate )	// if we create, we can't append or read
        uMode |= OF_CREATE;
    else
    {
        if (( bRead && bWrite ) || bAppend )
            uMode |= OF_READWRITE;
        else
        {
            if ( bRead )
                uMode |= OF_READ;
            else if ( bWrite )
                uMode |= OF_WRITE;
        }
    }

    return( sWrite->Open(sPath, uMode) );
}
