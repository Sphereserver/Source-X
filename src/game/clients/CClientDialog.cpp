
#include "../../common/resource/sections/CDialogDef.h"
#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../../common/CLog.h"
#include "../../network/receive.h"
#include "../../network/send.h"
#include "../chars/CChar.h"
#include "../CServer.h"
#include "CClient.h"


bool CClient::Dialog_Setup( CLIMODE_TYPE mode, const CResourceID& rid, int iPage, CObjBase * pObj, lpctstr Arguments )
{
	ADDTOCALLSTACK("CClient::Dialog_Setup");
	if ( pObj == nullptr )
		return false;

	CResourceDef * pRes = g_Cfg.RegisteredResourceGetDef( rid );
	CDialogDef *	pDlg	 = dynamic_cast <CDialogDef*>(pRes);
	if ( !pRes || !pDlg )
	{
		DEBUG_ERR(("Invalid RES_DIALOG.\n"));
		return false;
	}

	if ( !pDlg->GumpSetup( iPage, this, pObj, Arguments ) )
		return false;

	// Now pack it up to send,
	// m_tmGumpDialog.m_ResourceID = rid;
	dword context = rid.GetPrivateUID();
	if ( GetNetState()->isClientKR() )
	{
		// translate to KR's equivalent DialogID
		context = g_Cfg.GetKRDialog( context );

		if ( context == 0 )
		{
			g_Log.Event( LOGL_WARN, "A Kingdom Reborn equivalent of dialog '%s' has not been defined.\n", pDlg->GetResourceName());
		}
	}

	addGumpDialog(mode, &pDlg->m_sControls, &pDlg->m_sText, pDlg->m_x, pDlg->m_y, pObj, context);
    pDlg->m_sControls.clear();
    pDlg->m_sText.clear();
	return true;
}


void CClient::addGumpInputVal( bool fCancel, INPVAL_STYLE style,
	dword iMaxLength,
	lpctstr pszText1,
	lpctstr pszText2,
	CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::addGumpInputVal");
	// CLIMODE_INPVAL
	// Should result in PacketGumpValueInputResponse::onReceive
	// just input an objects attribute.

	// ARGS:
	// 	m_Targ_UID = pObj->GetUID();
	//  m_Targ_Text = verb

	if (pObj == nullptr)
		return;

	ASSERT( pObj );

	new PacketGumpValueInput(this, fCancel, style, iMaxLength, pszText1, pszText2, pObj);

	// m_tmInpVal.m_UID = pObj->GetUID();
	// m_tmInpVal.m_PrvGumpID = m_tmGumpDialog.m_ResourceID;

	m_Targ_UID = pObj->GetUID();
	// m_Targ_Text = verb
	SetTargMode( CLIMODE_INPVAL );
}

void
CClient::addGumpDialog( CLIMODE_TYPE mode, std::vector<CSString> const* vsControls, std::vector<CSString> const* vsText,
    int x, int y, CObjBase * pObj, dword dwRid)
{
	ADDTOCALLSTACK("CClient::addGumpDialog");
	// Add a generic GUMP menu.
	// Should return a PacketGumpDialogRet::onReceive
	// NOTE: These packets can get rather LARGE.
	// x,y = where on the screen ?

	if ( pObj == nullptr )
		pObj = m_pChar;

	uint context_mode = (uint)mode;
	if ( mode == CLIMODE_DIALOG && dwRid != 0 )
	{
		context_mode = dwRid;
	}

	PacketGumpDialog* cmd = new PacketGumpDialog(x, y, pObj, context_mode);
	cmd->writeControls(this, vsControls, vsText);
	cmd->push(this);

	if ( m_pChar )
	{
		++m_mapOpenedGumps[context_mode];
	}
}

bool CClient::addGumpDialogProps( const CUID& uid )
{
	ADDTOCALLSTACK("CClient::addGumpDialogProps");
	// put up a prop dialog for the object.
	CObjBase * pObj = uid.ObjFind();
	if ( pObj == nullptr )
		return false;
	if ( m_pChar == nullptr )
		return false;
	if ( ! m_pChar->CanTouch( pObj ))	// probably a security issue.
		return false;

	m_Prop_UID = m_Targ_UID = uid;
	if ( uid.IsChar() )
		addSkillWindow((SKILL_TYPE)(g_Cfg.m_iMaxSkill), true);

	tchar *pszMsg = Str_GetTemp();
	strcpy(pszMsg, pObj->IsItem() ? "D_ITEMPROP1" : "D_CHARPROP1" );

	CResourceID rid = g_Cfg.ResourceGetIDType(RES_DIALOG, pszMsg);
	if ( ! rid.IsValidUID())
		return false;

	Dialog_Setup( CLIMODE_DIALOG, rid, 0, pObj );
	return true;
}

TRIGRET_TYPE CClient::Dialog_OnButton( const CResourceID& rid, dword dwButtonID, CObjBase * pObj, CDialogResponseArgs * pArgs )
{
	ADDTOCALLSTACK("CClient::Dialog_OnButton");
	// one of the gump dialog buttons was pressed.
	if ( pObj == nullptr )		// object is gone ?
		return TRIGRET_ENDIF;

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, CResourceID( RES_DIALOG, rid.GetResIndex(), RES_DIALOG_BUTTON )))
	{
		return TRIGRET_ENDIF;
	}

	// Set the auxiliary stuff for INPDLG here
	// m_tmInpVal.m_PrvGumpID	= rid;
	// m_tmInpVal.m_UID		= pObj ? pObj->GetUID() : (CUID) 0;

	int64 piCmd[3];
	while ( s.ReadKeyParse())
	{
		if ( ! s.IsKeyHead( "ON", 2 ))
			continue;

		size_t iArgs = Str_ParseCmds( s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd) );
		if ( iArgs == 0 )
			continue;

		if ( iArgs == 1 )
		{
			// single button value
			if ( (dword)piCmd[0] != dwButtonID )
				continue;
		}
		else
		{
			// range of button values
			if ( dwButtonID < (dword)piCmd[0] || dwButtonID > (dword)piCmd[1] )
				continue;
		}

		pArgs->m_iN1 = dwButtonID;

		auto stopPrebutton = TRIGRET_RET_FALSE;

		CResourceLock prebutton;
		if (g_Cfg.ResourceLock(prebutton, CResourceID(RES_DIALOG, rid.GetResIndex(), RES_DIALOG_PREBUTTON)))
		stopPrebutton = pObj->OnTriggerRun(prebutton, TRIGRUN_SECTION_TRUE, m_pChar, pArgs, NULL);

		if (stopPrebutton != TRIGRET_RET_TRUE)
		return pObj->OnTriggerRunVal(s, TRIGRUN_SECTION_TRUE, m_pChar, pArgs);
	}

	return TRIGRET_ENDIF;
}

bool CClient::Dialog_Close( CObjBase * pObj, dword dwRid, int buttonID )
{
	ADDTOCALLSTACK("CClient::Dialog_Close");

	new PacketGumpChange(this, dwRid, buttonID);

	if ( GetNetState()->isClientVersionNumber(MINCLIVER_CLOSEDIALOG) )
	{
		CChar * pSrc = dynamic_cast<CChar*>( pObj );
		if ( pSrc )
		{
			OpenedGumpsMap_t::const_iterator itGumpFound = m_mapOpenedGumps.find(dwRid);
			if (( itGumpFound != m_mapOpenedGumps.end() ) && ( itGumpFound->second > 0 ))
			{
				PacketGumpDialogRet packet;
				packet.writeByte(XCMD_GumpDialogRet);
				packet.writeInt16(27);
				packet.writeInt32(pObj->GetUID());
				packet.writeInt32(dwRid);	// gump context
				packet.writeInt32(buttonID);
				packet.writeInt32(0);
				packet.writeInt32(0);
				packet.writeInt32(0);

				packet.seek(1);
				packet.onReceive(GetNetState());
			}
		}
	}

	return true;
}

TRIGRET_TYPE CClient::Menu_OnSelect( const CResourceID& rid, int iSelect, CObjBase * pObj ) // Menus for general purpose
{
	ADDTOCALLSTACK("CClient::Menu_OnSelect");
	// A select was made. so run the script.
	// iSelect = 0 = cancel.

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, rid ))
	{
		// Can't find the resource ?
		return TRIGRET_ENDIF;
	}

	if ( pObj == nullptr )
		pObj = m_pChar;

	// execute the menu script.
	if ( iSelect == 0 )	// Cancel button
	{

		while ( s.ReadKeyParse() )
		{
            lpctstr ptcStr = s.GetArgStr();
			if ( !s.IsKey( "ON" ) || ( *ptcStr != '@' ) )
				continue;

			if (strnicmp(ptcStr, "@CANCEL", 7 ) )
				continue;

			return pObj->OnTriggerRunVal( s, TRIGRUN_SECTION_TRUE, m_pChar, nullptr );
		}
	}
	else
	{
		int i=0;	// 1 based selection.
		while ( s.ReadKeyParse())
		{
			if ( !s.IsKey( "ON" ) || ( *s.GetArgStr() == '@' ) )
				continue;

			++i;
			if ( i < iSelect )
				continue;
			if ( i > iSelect )
				break;

			return pObj->OnTriggerRunVal( s, TRIGRUN_SECTION_TRUE, m_pChar, nullptr );
		}
	}

	// No selection ?
	return TRIGRET_ENDIF;
}

bool CMenuItem::ParseLine( tchar * pszArgs, CScriptObj * pObjBase, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CMenuItem::ParseLine");

	if ( *pszArgs == '@' )
	{
		// This allows for triggers in menus
		return false;
	}

	tchar * pszArgStart = pszArgs;
	while ( _ISCSYM( *pszArgs ))
		++pszArgs;

	if ( *pszArgs )
	{
		*pszArgs = '\0';
        ++pszArgs;
		GETNONWHITESPACE( pszArgs );
	}

	// The item id (if we want to have an item type menu) or 0
	if ( strcmp( pszArgStart, "0" ) != 0 )
	{
		CItemBase * pItemBase = CItemBase::FindItemBase((ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, pszArgStart )));
		if ( pItemBase != nullptr )
		{
			m_id = (word)(pItemBase->GetDispID());
			pObjBase = pItemBase;
		}
		else
		{
			DEBUG_ERR(( "Bad MENU item id '%s'\n", pszArgStart ));
			return false;	// skip this.
		}
	}
	else
	{
		m_id = 0;
	}

	if ( pObjBase != nullptr )
		pObjBase->ParseScriptText( pszArgs, pSrc );
	else
		g_Serv.ParseScriptText( pszArgs, pSrc );

	// Parsing @color
	if ( *pszArgs == '@' )
	{
		++pszArgs;
		HUE_TYPE wHue = (HUE_TYPE)(Exp_GetVal( pszArgs ));
		if ( wHue != 0 )
			wHue = (wHue == 1? 0x7FF: wHue-1);

		m_color = wHue;
		SKIP_ARGSEP( pszArgs );
	}
	else
		m_color = 0;

	m_sText = pszArgs;

	if ( m_sText.IsEmpty())
	{
		if ( pObjBase )	// use the objects name by default.
		{
			m_sText = pObjBase->GetName();
			if ( ! m_sText.IsEmpty())
				return true;
		}
		DEBUG_ERR(( "Bad MENU item text '%s'\n", pszArgStart ));
	}

	return !m_sText.IsEmpty();
}

void CClient::Menu_Setup( CResourceID rid, CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::Menu_Setup");
	// Menus for general purpose
	// Expect PacketMenuChoice::onReceive() back.

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, rid ))
	{
        DEBUG_ERR(("Invalid RES_MENU.\n"));
		return;
	}

	if ( pObj == nullptr )
	{
		pObj = m_pChar;
	}

	if (!s.ReadKey())	// get title for the menu.
	{
		DEBUG_ERR(("Error getting the menu title.\n"));
		return;
	}
	pObj->ParseScriptText( s.GetKeyBuffer(), m_pChar );

	CMenuItem item[MAX_MENU_ITEMS];
	item[0].m_sText = s.GetKey();
	// item[0].m_id = rid.m_internalrid;	// general context id

	uint i = 0;
	while (s.ReadKeyParse())
	{
		if ( ! s.IsKey( "ON" ))
			continue;

		++i;
		if ( ! item[i].ParseLine( s.GetArgRaw(), pObj, m_pChar ))
			--i;

		if ( i >= (ARRAY_COUNT( item ) - 1))
			break;
	}

	m_tmMenu.m_ResourceID = rid;

	ASSERT(i < ARRAY_COUNT(item));
	addItemMenu( CLIMODE_MENU, item, i, pObj );
}

