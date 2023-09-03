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
    GUMPCTL_ITEMPROPERTY,   // 1 = uid of the item in question.

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
    "ITEMPROPERTY",

    "XMFHTMLGUMP",
    "XMFHTMLGUMPCOLOR",
    "XMFHTMLTOK",

    nullptr
};


uint CDialogDef::GumpAddText( lpctstr pszText )
{
    ADDTOCALLSTACK("CDialogDef::GumpAddText");
    m_sText[m_uiTexts] = pszText;
    ++m_uiTexts;
    return (m_uiTexts - 1);
}

#define SKIP_ALL( args )		SKIP_SEPARATORS( args ); GETNONWHITESPACE( args );
#define GET_ABSOLUTE( c )		SKIP_ALL( pszArgs );	int c = Exp_GetSingle( pszArgs );

#define GET_EVAL( c )		    SKIP_ALL( pszArgs );	int c = Exp_GetVal( pszArgs );

#define GET_RELATIVE( c, base )								\
	SKIP_ALL( pszArgs ); int c;								\
	if ( *pszArgs == '-' && IsSpace(pszArgs[1]))				\
		c	= base, ++pszArgs;								\
	else if ( *pszArgs == '+' )								\
		c = base + Exp_GetSingle( ++pszArgs );					\
	else if ( *pszArgs == '-' )								\
		c = base - Exp_GetSingle( ++pszArgs );					\
	else if ( *pszArgs == '*' )								\
		base = c	= base + Exp_GetSingle( ++pszArgs );		\
	else													\
		c = Exp_GetSingle( pszArgs );


bool CDialogDef::r_Verb( CScript & s, CTextConsole * pSrc )	// some command on this object as a target
{
    ADDTOCALLSTACK("CDialogDef::r_Verb");
    EXC_TRY("Verb");
    // The first part of the key is GUMPCTL_TYPE
    lpctstr ptcKey = s.GetKey();

    int index = FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys)-1 );
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

    lpctstr pszArgs	= s.GetArgStr();

    switch( index )
    {
        case GUMPCTL_PAGE:
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;

            GET_ABSOLUTE( page );

            if ( page <= 0 )		return true;

            int	iNewPage;
            if ( m_wPage == 0 || page > m_wPage || page == 0 )
                iNewPage	= page;
            else if ( page == m_wPage  )
                iNewPage	= 1;
            else
                iNewPage	= page + 1;

            m_sControls[m_uiControls].Format( "page %d", iNewPage );
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_BUTTON:			// 7 = X,Y,Down gump,Up gump,pressable(1/0),page,id
        case GUMPCTL_BUTTONTILEART:		// 11 = X,Y,Down gump,Up gump,pressable(1/0),page,id,tileart,hue,X,Y
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;

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
                m_sControls[m_uiControls].Format( "button %d %d %d %d %d %d %d", x, y, down, up, press, iNewPage, id );
            else
            {
                GET_ABSOLUTE( tileId );
                GET_ABSOLUTE( tileHue );
                GET_ABSOLUTE( tileX );
                GET_ABSOLUTE( tileY );

                m_sControls[m_uiControls].Format( "buttontileart %d %d %d %d %d %d %d %d %d %d %d", x, y, down, up, press, iNewPage, id, tileId, tileHue, tileX, tileY );
            }

            ++m_uiControls;
            return true;
        }
        case GUMPCTL_GUMPPIC:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( id );
            SKIP_ALL( pszArgs );

            m_sControls[m_uiControls].Format( "gumppic %d %d %d%s%s", x, y, id, *pszArgs ? " hue=" : "", *pszArgs ? pszArgs : "" );
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_GUMPPICTILED:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( sX );
            GET_ABSOLUTE( sY );
            GET_ABSOLUTE( id );

            m_sControls[m_uiControls].Format( "gumppictiled %d %d %d %d %d", x, y, sX, sY, id );
            ++m_uiControls;
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

            m_sControls[m_uiControls].Format("picinpic %d %d %d %d %d %d %d", x, y, id, w, h, sX, sY);
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_RESIZEPIC:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( id );
            GET_ABSOLUTE( sX );
            GET_ABSOLUTE( sY );

            m_sControls[m_uiControls].Format( "resizepic %d %d %d %d %d", x, y, id, sX, sY );
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_TILEPIC:
        case GUMPCTL_TILEPICHUE:
        {
            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( id );
            SKIP_ALL( pszArgs );

            // TilePic don't use args, TilePicHue yes :)
            if ( index == GUMPCTL_TILEPIC )
                m_sControls[m_uiControls].Format( "tilepic %d %d %d", x, y, id );
            else
                m_sControls[m_uiControls].Format( "tilepichue %d %d %d%s%s", x, y, id, *pszArgs ? " " : "", *pszArgs ? pszArgs : "" );

            ++m_uiControls;
            return true;
        }
        case GUMPCTL_DTEXT:
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;
            if ( m_uiTexts >= (ARRAY_COUNT(m_sText) - 1) )
                return false;

            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( hue );
            SKIP_ALL( pszArgs )
                if ( *pszArgs == '.' )			pszArgs++;

            uint iText = GumpAddText( *pszArgs ? pszArgs : "" );
            m_sControls[m_uiControls].Format( "text %d %d %d %u", x, y, hue, iText );
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_DCROPPEDTEXT:
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;
            if ( m_uiTexts >= (ARRAY_COUNT(m_sText) - 1) )
                return false;

            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( w );
            GET_ABSOLUTE( h );
            GET_ABSOLUTE( hue );
            SKIP_ALL( pszArgs )
                if ( *pszArgs == '.' )			pszArgs++;

			uint iText = GumpAddText( *pszArgs ? pszArgs : "" );
            m_sControls[m_uiControls].Format( "croppedtext %d %d %d %d %d %u", x, y, w, h, hue, iText );
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_DHTMLGUMP:
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;
            if ( m_uiTexts >= (ARRAY_COUNT(m_sText) - 1) )
                return false;

            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( w );
            GET_ABSOLUTE( h );
            GET_ABSOLUTE( bck );
            GET_ABSOLUTE( options );
            SKIP_ALL( pszArgs )

            uint iText = GumpAddText( *pszArgs ? pszArgs : "" );
            m_sControls[m_uiControls].Format( "htmlgump %d %d %d %d %u %d %d", x, y, w, h, iText, bck, options );
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_DTEXTENTRY:
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;
            if ( m_uiTexts >= (ARRAY_COUNT(m_sText) - 1) )
                return false;

            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( w );
            GET_ABSOLUTE( h );
            GET_ABSOLUTE( hue );
            GET_ABSOLUTE( id );
            SKIP_ALL( pszArgs )

            uint iText = GumpAddText( *pszArgs ? pszArgs : "" );
            m_sControls[m_uiControls].Format( "textentry %d %d %d %d %d %d %u", x, y, w, h, hue, id, iText );
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_DTEXTENTRYLIMITED:
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;
            if ( m_uiTexts >= (ARRAY_COUNT(m_sText) - 1) )
                return false;

            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( w );
            GET_ABSOLUTE( h );
            GET_ABSOLUTE( hue );
            GET_ABSOLUTE( id );
            GET_ABSOLUTE( charLimit );
            SKIP_ALL( pszArgs )

            uint iText = GumpAddText( *pszArgs ? pszArgs : "" );
            m_sControls[m_uiControls].Format( "textentrylimited %d %d %d %d %d %d %u %d", x, y, w, h, hue, id, iText, charLimit );
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_CHECKBOX:
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;

            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( down );
            GET_ABSOLUTE( up );
            GET_ABSOLUTE( state );
            GET_ABSOLUTE( id );

            m_sControls[m_uiControls].Format( "checkbox %d %d %d %d %d %d", x, y, down, up, state, id );

            ++m_uiControls;
            return true;
        }
        case GUMPCTL_RADIO:
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;

            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( down );
            GET_ABSOLUTE( up );
            GET_ABSOLUTE( state );
            GET_ABSOLUTE( id );

            m_sControls[m_uiControls].Format( "radio %d %d %d %d %d %d", x, y, down, up, state, id );

            ++m_uiControls;
            return true;
        }
        case GUMPCTL_CHECKERTRANS:
        {
            if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
                return false;

            GET_RELATIVE( x, m_iOriginX );
            GET_RELATIVE( y, m_iOriginY );
            GET_ABSOLUTE( width );
            GET_ABSOLUTE( height );

            m_sControls[m_uiControls].Format( "checkertrans %d %d %d %d", x, y, width, height );
            ++m_uiControls;
            return true;
        }
        case GUMPCTL_DORIGIN:
        {
            // GET_RELATIVE( x, m_iOriginX );
            // GET_RELATIVE( y, m_iOriginY );
            // m_iOriginX	= x;
            // m_iOriginY	= y;

            SKIP_ALL( pszArgs );
            if ( *pszArgs == '-' && (IsSpace( pszArgs[1] ) || !pszArgs[1]) )		pszArgs++;
            else  if ( *pszArgs == '*' )	m_iOriginX	+= Exp_GetSingle( ++pszArgs );
            else							m_iOriginX	 = Exp_GetSingle( pszArgs );

            SKIP_ALL( pszArgs );
            if ( *pszArgs == '-' && (IsSpace( pszArgs[1] ) || !pszArgs[1]) )		pszArgs++;
            else  if ( *pszArgs == '*' )	m_iOriginY	+= Exp_GetSingle( ++pszArgs );
            else							m_iOriginY	= Exp_GetSingle( pszArgs );

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
            //SKIP_ALL( pszArgs )

            if ( index == GUMPCTL_XMFHTMLGUMP ) // xmfhtmlgump doesn't use color
                m_sControls[m_uiControls].Format( "xmfhtmlgump %d %d %d %d %d %d %d" , x, y, sX, sY, cliloc, hasBack, canScroll );
            else
                m_sControls[m_uiControls].Format( "xmfhtmlgumpcolor %d %d %d %d %d %d %d%s%s", x, y, sX, sY, cliloc, hasBack, canScroll, *pszArgs ? " " : "", *pszArgs ? pszArgs : "" );

            ++m_uiControls;
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
            SKIP_ALL(pszArgs);

            m_sControls[m_uiControls].Format("xmfhtmltok %d %d %d %d %d %d %d %d %s", x, y, sX, sY, hasBack, canScroll, color, cliloc, *pszArgs ? pszArgs : "");

            ++m_uiControls;
            return true;
        }
        default:
            break;
    }

    if ( m_uiControls >= (ARRAY_COUNT(m_sControls) - 1) )
        return false;

    m_sControls[m_uiControls].Format("%s %s", ptcKey, pszArgs);
    ++m_uiControls;
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPTSRC;
    EXC_DEBUG_END;
    return false;
}


CDialogDef::CDialogDef( CResourceID rid ) :
    CResourceLink( rid )
{
    m_uiControls = 0;
    m_uiTexts = 0;
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

    m_uiControls	= 0;
    m_uiTexts		= 0;
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
            if ( m_uiTexts >= (ARRAY_COUNT(m_sText) - 1) )
                break;
            m_pObj->ParseScriptText( s.GetKeyBuffer(), pClient->GetChar() );
            m_sText[m_uiTexts] = s.GetKey();
            ++m_uiTexts;
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
    m_x		= (int)(iSizes[0]);
    m_y		= (int)(iSizes[1]);

    if ( OnTriggerRunVal( s, TRIGRUN_SECTION_TRUE, pClient->GetChar(), &Args ) == TRIGRET_RET_TRUE )
        return false;
    return true;
}
