#include "../../../game/chars/CChar.h"
#include "../../../game/clients/CClient.h"
#include "../../../game/CObjBase.h"
#include "../../../sphere/threads.h"
#include "../../CException.h"
#include "../../CExpression.h"
#include "../../CScriptTriggerArgs.h"
#include "../CResourceLock.h"
#include "CDialogDef.h"

// endgroup, master, hue ????

enum GUMPCTL_TYPE // controls we can put in a gump.
{
    GUMPCTL_BUTTON = 0, // 7 = X,Y,Down gump,Up gump,pressable(1/0),page,id
    GUMPCTL_BUTTONTILEART, // NEW: 11 = X,Y,Down gump,Up gump,pressable(1/0),page,id,tileart,hue,X,Y
    GUMPCTL_CHECKBOX, // 6 = x,y,gumpcheck,gumpuncheck,starting state,checkid

    GUMPCTL_CHECKERTRANS, // NEW: x,y,w,h
    GUMPCTL_CROPPEDTEXT, // 6 = x,y,sx,sy,color?,startindex

    GUMPCTL_DCROPPEDTEXT,
    GUMPCTL_DHTMLGUMP,
    GUMPCTL_DORIGIN,
    GUMPCTL_DTEXT,
    GUMPCTL_DTEXTENTRY,
    GUMPCTL_DTEXTENTRYLIMITED,

    GUMPCTL_GROUP,

    GUMPCTL_GUMPPIC, // 3 = x,y,gumpID hue=color// put gumps in the dlg.
    GUMPCTL_GUMPPICTILED, // x, y, gumpID, w, h, hue=color
    GUMPCTL_HTMLGUMP, // 7 = x,y,sx,sy, 0 0 0

    GUMPCTL_ITEMPROPERTY,   // 1 = uid of the item in question.

    // Not really controls but more attributes.
    GUMPCTL_NOCLOSE, // 0 = The gump cannot be closed by right clicking.
    GUMPCTL_NODISPOSE, // 0 = The gump cannot be closed by gump-closing macro.
    GUMPCTL_NOMOVE, // 0 = The gump cannot be moved around.

    GUMPCTL_PAGE, // 1 = set current page number // for multi tab dialogs.

    GUMPCTL_PICINPIC, // x y gump spritex spritey width height
    GUMPCTL_RADIO, // 6 = x,y,gump1,gump2,starting state,id
    GUMPCTL_RESIZEPIC, // 5 = x,y,gumpback,sx,sy // can come first if multi page. put up some background gump
    GUMPCTL_TEXT, // 4 = x,y,color?,startstringindex // put some text here.
    GUMPCTL_TEXTENTRY,
    GUMPCTL_TEXTENTRYLIMITED,
    GUMPCTL_TILEPIC, // 3 = x,y,item // put item tiles in the dlg.
    GUMPCTL_TILEPICHUE, // NEW: x,y,item,color

    GUMPCTL_TOOLTIP, // From SE client. tooltip cliloc(1003000)

    GUMPCTL_XMFHTMLGUMP, // 7 = x,y,sx,sy, cliloc(1003000) hasBack canScroll
    GUMPCTL_XMFHTMLGUMPCOLOR, // NEW: x,y,w,h ???
    GUMPCTL_XMFHTMLTOK, // 9 = x y width height has_background has_scrollbar color cliloc_id @args

    GUMPCTL_QTY
};

lpctstr const CDialogDef::sm_szLoadKeys[GUMPCTL_QTY+1] =
{
    "BUTTON",
    "BUTTONTILEART",
    "CHECKBOX",

    "CHECKERTRANS",
    "CROPPEDTEXT",

    "DCROPPEDTEXT",
    "DHTMLGUMP",
    "DORIGIN",
    "DTEXT",
    "DTEXTENTRY",
    "DTEXTENTRYLIMITED",

    "GROUP",

    "GUMPPIC",
    "GUMPPICTILED",
    "HTMLGUMP",

    "ITEMPROPERTY",

    "NOCLOSE",
    "NODISPOSE",
    "NOMOVE",

    "PAGE",

    "PICINPIC",
    "RADIO",
    "RESIZEPIC",
    "TEXT",
    "TEXTENTRY",
    "TEXTENTRYLIMITED",
    "TILEPIC",
    "TILEPICHUE",

    "TOOLTIP",

    "XMFHTMLGUMP",
    "XMFHTMLGUMPCOLOR",
    "XMFHTMLTOK",

    nullptr
};


uint CDialogDef::GumpAddText( lpctstr ptcText )
{
    ADDTOCALLSTACK("CDialogDef::GumpAddText");
    m_sText.emplace_back(ptcText);
    return uint(m_sText.size() - 1);
}


bool CDialogDef::r_Verb( CScript & s, CTextConsole * pSrc )	// some command on this object as a target
{
    ADDTOCALLSTACK("CDialogDef::r_Verb");
    EXC_TRY("Verb");

    // The first part of the key is GUMPCTL_TYPE
    lpctstr ptcKey = s.GetKey();
    const int index = FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys)-1 );
    if ( index < 0 )
    {
        const size_t uiFunctionIndex = r_GetFunctionIndex(ptcKey);
        if (r_CanCall(uiFunctionIndex))
        {
            // RES_FUNCTION call
            CSString sVal;
            CScriptTriggerArgs Args(s.GetArgRaw());
            if ( r_Call(uiFunctionIndex, pSrc, &Args, &sVal) )
                return true;
        }

        if (!m_pObj)
            return CResourceLink::r_Verb(s, pSrc);
        return m_pObj->r_Verb(s, pSrc);
    }

    lptstr ptcArgs = s.GetArgStr();
    //g_Log.EventDebug("Dialog index %d, KEY %s ARG %s.\n", index, ptcKey, ptcArgs);

    const auto _SkipAll = [](lptstr& ptcArgs_) noexcept -> void
    {
        SKIP_SEPARATORS(ptcArgs_);
        GETNONWHITESPACE(ptcArgs_);
    };

    const auto _CalcRelative = [](lptstr& ptcArgs_, int &iCoordBase_) -> int
    {
        int c;
            if ( *ptcArgs_ == '-' && IsSpace(ptcArgs_[1]))
            c = iCoordBase_, ++ptcArgs_;
        else if ( *ptcArgs_ == '+' )
            c = iCoordBase_ + Exp_GetSingle( ++ptcArgs_ );
        else if ( *ptcArgs_ == '-' )
            c = iCoordBase_ - Exp_GetSingle( ++ptcArgs_ );
        else if ( *ptcArgs_ == '*' )
            iCoordBase_ = c = iCoordBase_ + Exp_GetSingle( ++ptcArgs_ );
        else
            c = Exp_GetSingle( ptcArgs_ );
        return c;
    };

#   define GET_ABSOLUTE(c)          _SkipAll(ptcArgs);   int c = Exp_GetSingle(ptcArgs);
#   define GET_EVAL(c)              _SkipAll(ptcArgs);   int c = Exp_GetVal(ptcArgs);
#   define GET_RELATIVE(c, base)    _SkipAll(ptcArgs);   int c = _CalcRelative(ptcArgs, base);

    switch( index )
    {
        case GUMPCTL_PAGE:
        {
            GET_ABSOLUTE( page );
            if ( page <= 0 )
                return true;

            int	iNewPage;
            if ( m_wPage == 0 || page > m_wPage || page == 0 )
                iNewPage	= page;
            else if ( page == m_wPage  )
                iNewPage	= 1;
            else
                iNewPage	= page + 1;

            m_sControls.emplace_back(false).Format( "page %d", iNewPage );
            return true;
        }

        case GUMPCTL_BUTTON:			// 7 = X,Y,Down gump,Up gump,pressable(1/0),page,id
        case GUMPCTL_BUTTONTILEART:		// 11 = X,Y,Down gump,Up gump,pressable(1/0),page,id,tileart,hue,X,Y
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( down );
            GET_ABSOLUTE( up );
            GET_ABSOLUTE( press );
            GET_ABSOLUTE( page );
            GET_ABSOLUTE( id );

            int	iNewPage;
            if ( m_wPage == 0 || page > m_wPage || page == 0 )
                iNewPage	= page;
            else if ( page == m_wPage  )
                iNewPage	= 1;
            else
                iNewPage	= page + 1;

            if (index == GUMPCTL_BUTTON)
                m_sControls.emplace_back(false).Format( "button %d %d %d %d %d %d %d", x, y, down, up, press, iNewPage, id );
            else
            {
                GET_ABSOLUTE( tileId );
                GET_ABSOLUTE( tileHue );
                GET_ABSOLUTE( tileX );
                GET_ABSOLUTE( tileY );

                m_sControls.emplace_back(false).Format( "buttontileart %d %d %d %d %d %d %d %d %d %d %d", x, y, down, up, press, iNewPage, id, tileId, tileHue, tileX, tileY );
            }

            return true;
        }

        case GUMPCTL_GUMPPIC:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( id );
            _SkipAll( ptcArgs );

            m_sControls.emplace_back(false).Format( "gumppic %d %d %d%s%s", x, y, id, *ptcArgs ? " hue=" : "", *ptcArgs ? ptcArgs : "" );
            return true;
        }

        case GUMPCTL_GUMPPICTILED:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( sX );
            GET_ABSOLUTE( sY );
            GET_ABSOLUTE( id );

            m_sControls.emplace_back(false).Format( "gumppictiled %d %d %d %d %d", x, y, sX, sY, id );
            return true;
        }

        case GUMPCTL_PICINPIC:
        {
            GET_RELATIVE(x, m_iOriginX);
            GET_RELATIVE(y, m_iOriginY);
            GET_ABSOLUTE(id);
            GET_ABSOLUTE(w);
            GET_ABSOLUTE(h);
            GET_ABSOLUTE(sX);
            GET_ABSOLUTE(sY);

            m_sControls.emplace_back(false).Format("picinpic %d %d %d %d %d %d %d", x, y, id, w, h, sX, sY);
            return true;
        }

        case GUMPCTL_RESIZEPIC:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( id );
            GET_ABSOLUTE( sX );
            GET_ABSOLUTE( sY );

            m_sControls.emplace_back(false).Format( "resizepic %d %d %d %d %d", x, y, id, sX, sY );
            return true;
        }

        case GUMPCTL_TILEPIC:
        case GUMPCTL_TILEPICHUE:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( id );
            _SkipAll( ptcArgs );

            // TilePic don't use args, TilePicHue yes :)
            if ( index == GUMPCTL_TILEPIC )
                m_sControls.emplace_back(false).Format( "tilepic %d %d %d", x, y, id );
            else
                m_sControls.emplace_back(false).Format( "tilepichue %d %d %d%s%s", x, y, id, *ptcArgs ? " " : "", *ptcArgs ? ptcArgs : "" );

            return true;
        }

        case GUMPCTL_DTEXT:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( hue );
            _SkipAll( ptcArgs );
            if ( *ptcArgs == '.' )
                ++ptcArgs;

            const uint uiText = GumpAddText( *ptcArgs ? ptcArgs : "" );
            m_sControls.emplace_back(false).Format( "text %d %d %d %u", x, y, hue, uiText );
            return true;
        }

        case GUMPCTL_DCROPPEDTEXT:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( w );
            GET_ABSOLUTE( h );
            GET_ABSOLUTE( hue );
            _SkipAll( ptcArgs );
            if ( *ptcArgs == '.' )
                ptcArgs += 1;

            const uint uiText = GumpAddText( *ptcArgs ? ptcArgs : "" );
            m_sControls.emplace_back(false).Format( "croppedtext %d %d %d %d %d %u", x, y, w, h, hue, uiText );
            return true;
        }

        case GUMPCTL_DHTMLGUMP:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( w );
            GET_ABSOLUTE( h );
            GET_ABSOLUTE( bck );
            GET_ABSOLUTE( options );
            _SkipAll( ptcArgs );

            const uint uiText = GumpAddText( *ptcArgs ? ptcArgs : "" );
            m_sControls.emplace_back(false).Format( "htmlgump %d %d %d %d %u %d %d", x, y, w, h, uiText, bck, options );
            return true;
        }

        case GUMPCTL_DTEXTENTRY:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( w );
            GET_ABSOLUTE( h );
            GET_ABSOLUTE( hue );
            GET_ABSOLUTE( id );
            _SkipAll( ptcArgs );

            const uint uiText = GumpAddText( *ptcArgs ? ptcArgs : "" );
            m_sControls.emplace_back(false).Format( "textentry %d %d %d %d %d %d %u", x, y, w, h, hue, id, uiText );
            return true;
        }

        case GUMPCTL_DTEXTENTRYLIMITED:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( w );
            GET_ABSOLUTE( h );
            GET_ABSOLUTE( hue );
            GET_ABSOLUTE( id );
            GET_ABSOLUTE( charLimit );
            _SkipAll( ptcArgs );

            const uint uiText = GumpAddText( *ptcArgs ? ptcArgs : "" );
            m_sControls.emplace_back(false).Format( "textentrylimited %d %d %d %d %d %d %u %d", x, y, w, h, hue, id, uiText, charLimit );
            return true;
        }

        case GUMPCTL_CHECKBOX:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( down );
            GET_ABSOLUTE( up );
            GET_ABSOLUTE( state );
            GET_ABSOLUTE( id );

            m_sControls.emplace_back(false).Format( "checkbox %d %d %d %d %d %d", x, y, down, up, state, id );
            return true;
        }

        case GUMPCTL_RADIO:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( down );
            GET_ABSOLUTE( up );
            GET_ABSOLUTE( state );
            GET_ABSOLUTE( id );

            m_sControls.emplace_back(false).Format( "radio %d %d %d %d %d %d", x, y, down, up, state, id );
            return true;
        }

        case GUMPCTL_TOOLTIP:
        {
            tchar* pptcArgs[2];
            const int iArgs = Str_ParseCmds(ptcArgs, pptcArgs, ARRAY_COUNT(pptcArgs));
            if (iArgs < 2)
                return false;

            // The client expects this to be sent as a string in decimal notation. Ensure that.
            std::optional<uint> conv = Str_ToU(pptcArgs[0]);
            if (!conv.has_value())
                return false;

            m_sControls.emplace_back(false).Format( "tooltip %u %s", conv.value(), pptcArgs[1] );
            return true;
        }

        case GUMPCTL_CHECKERTRANS:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( width );
            GET_ABSOLUTE( height );

            m_sControls.emplace_back(false).Format( "checkertrans %d %d %d %d", x, y, width, height );
            return true;
        }

        case GUMPCTL_DORIGIN:
        {
            // GET_RELATIVE( x, m_iOriginX );
            // GET_RELATIVE( y, m_iOriginY );
            // m_iOriginX	= x;
            // m_iOriginY	= y;

            _SkipAll( ptcArgs );
            if ( *ptcArgs == '-' && (IsSpace( ptcArgs[1] ) || !ptcArgs[1]) )
                ++ptcArgs;
            else if ( *ptcArgs == '*' )
                m_iOriginX += Exp_GetSingle( ++ptcArgs );
            else
            	m_iOriginX	= Exp_GetSingle( ptcArgs );

            _SkipAll( ptcArgs );
            if ( *ptcArgs == '-' && (IsSpace( ptcArgs[1] ) || !ptcArgs[1]) )
                ++ptcArgs;
            else if ( *ptcArgs == '*' )
                m_iOriginY += Exp_GetSingle( ++ptcArgs );
            else
                m_iOriginY  = Exp_GetSingle( ptcArgs );

            return true;
        }

        case GUMPCTL_NODISPOSE:
            m_fNoDispose = true;
            break;

        case GUMPCTL_CROPPEDTEXT:
        case GUMPCTL_TEXT:
        case GUMPCTL_TEXTENTRY:
        case GUMPCTL_TEXTENTRYLIMITED:
            break;

        case GUMPCTL_XMFHTMLGUMP:		// 7 = x,y,sx,sy, cliloc(1003000) hasBack canScroll
        case GUMPCTL_XMFHTMLGUMPCOLOR: // 7 + color.
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( sX );
            GET_ABSOLUTE( sY );
            GET_ABSOLUTE( cliloc );
            GET_ABSOLUTE( hasBack );
            GET_ABSOLUTE( canScroll );
            //_SkipAll( ptcArgs )

            if ( index == GUMPCTL_XMFHTMLGUMP ) // xmfhtmlgump doesn't use color
                m_sControls.emplace_back(false).Format( "xmfhtmlgump %d %d %d %d %d %d %d", x, y, sX, sY, cliloc, hasBack, canScroll );
            else
                m_sControls.emplace_back(false).Format( "xmfhtmlgumpcolor %d %d %d %d %d %d %d%s%s", x, y, sX, sY, cliloc, hasBack, canScroll, *ptcArgs ? " " : "", *ptcArgs ? ptcArgs : "" );
            return true;
        }

        case GUMPCTL_XMFHTMLTOK: // 9 = x y width height has_background has_scrollbar color cliloc_id @args
        {
            GET_RELATIVE(x, m_iOriginX);
            GET_RELATIVE(y, m_iOriginY);
            GET_ABSOLUTE(sX);
            GET_ABSOLUTE(sY);
            GET_ABSOLUTE(hasBack);
            GET_ABSOLUTE(canScroll);
            GET_ABSOLUTE(color);
            GET_ABSOLUTE(cliloc);
            _SkipAll(ptcArgs);

            m_sControls.emplace_back(false).Format("xmfhtmltok %d %d %d %d %d %d %d %d %s", x, y, sX, sY, hasBack, canScroll, color, cliloc, *ptcArgs ? ptcArgs : "");
            return true;
        }

        default:
            break;
    }

    m_sControls.emplace_back(false).Format("%s %s", ptcKey, ptcArgs);
    return true;
    EXC_CATCH;

#   undef GET_ABSOLUTE
#   undef GET_EVAL
#   undef GET_RELATIVE

    EXC_DEBUG_START;
    EXC_ADD_SCRIPTSRC;
    EXC_DEBUG_END;
    return false;
}


CDialogDef::CDialogDef( CResourceID rid ) :
    CResourceLink( rid )
{
    m_pObj = nullptr;
    m_x = 0;
    m_y = 0;
    m_iOriginX = 0;
    m_iOriginY = 0;
    m_wPage = 0;
    m_fNoDispose = false;
}


bool CDialogDef::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallChildren);
    ADDTOCALLSTACK("CDialogDef::r_WriteVal");
    if ( !m_pObj )
        return false;
    return (fNoCallParent ? false : m_pObj->r_WriteVal( ptcKey, sVal, pSrc ));
}


bool CDialogDef::r_LoadVal( CScript & s )
{
    ADDTOCALLSTACK("CDialogDef::r_LoadVal");
    if ( !m_pObj )
        return false;
    return m_pObj->r_LoadVal( s );
}


bool CDialogDef::GumpSetup( int iPage, CClient * pClient, CObjBase * pObjSrc, lpctstr Arguments )
{
    ADDTOCALLSTACK("CDialogDef::GumpSetup");
    CResourceLock	s;

    m_pObj			= pObjSrc;
    m_iOriginX		= 0;
    m_iOriginY		= 0;
    m_wPage			= (word)(iPage);
    m_fNoDispose	= false;

    CScriptTriggerArgs	Args(iPage, 0, pObjSrc);
    //DEBUG_ERR(("Args.m_s1_buf_vec %s  Args.m_s1 %s  Arguments 0x%x\n",Args.m_s1_buf_vec, Args.m_s1, Arguments));
    Args.m_s1_buf_vec = Args.m_s1 = Arguments;

    // read text first
    if ( g_Cfg.ResourceLock( s, CResourceID( RES_DIALOG, GetResourceID().GetResIndex(), RES_DIALOG_TEXT ) ) )
    {
        while ( s.ReadKey())
        {
            m_pObj->ParseScriptText( s.GetKeyBuffer(), pClient->GetChar() );
            m_sText.emplace_back(false) = s.GetKey();
        }
    }
    else
    {
        // no gump text?
    }

    // read the main dialog
    if ( !ResourceLock( s ) )
        return false;
    if ( !s.ReadKey() )		// read the size.
        return false;

    // starting x,y location.
    int64 iSizes[2];
    tchar * pszBuf = s.GetKeyBuffer();
    m_pObj->ParseScriptText( pszBuf, pClient->GetChar() );

    Str_ParseCmds( pszBuf, iSizes, ARRAY_COUNT(iSizes) );
    m_x	= (int)(iSizes[0]);
    m_y	= (int)(iSizes[1]);

    const auto trigRet = OnTriggerRunVal( s, TRIGRUN_SECTION_TRUE, pClient->GetChar(), &Args );
    m_sText.shrink_to_fit();
    m_sControls.shrink_to_fit();

    if ( trigRet == TRIGRET_RET_TRUE )
        return false;
    return true;
}
