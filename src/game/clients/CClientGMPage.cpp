
#include "../../common/CLog.h"
#include "../../network/CClientIterator.h"
#include "../chars/CChar.h"
#include "../CWorld.h"
#include "../CWorldGameTime.h"
#include "CClient.h"

/////////////////////////////////////////////

void CClient::Cmd_GM_Page( lpctstr pszReason ) // Help button (Calls GM Call Menus up)
{
	ADDTOCALLSTACK("CClient::Cmd_GM_Page");
	// Player pressed the help button.
	// m_Targ_Text = menu desc.
	// CLIMODE_PROMPT_GM_PAGE_TEXT

	if ( pszReason[0] == '\0' )
	{
		SysMessageDefault( DEFMSG_MSG_GMPAGE_CANCELED );
		return;
	}

	const CPointMap & ptPlayerLocation = m_pChar->GetTopPoint();

	tchar * pszMsg = Str_GetTemp();
	sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MSG_GMPAGE_REC ),
		    m_pChar->GetName(), (dword) m_pChar->GetUID(),
		    ptPlayerLocation.m_x, ptPlayerLocation.m_y, ptPlayerLocation.m_z, ptPlayerLocation.m_map,
			pszReason);

	g_Log.Event( LOGM_GM_PAGE, "%s\n", pszMsg);

	bool fFound = false;
	
	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( pClient->GetChar() && pClient->IsPriv( PRIV_GM_PAGE )) // found GM
		{
			fFound = true;
			pClient->SysMessage(pszMsg);
		}
	}

	if (fFound == false)
		SysMessageDefault( DEFMSG_MSG_GMPAGE_QUED );
	else
		SysMessageDefault( DEFMSG_MSG_GMPAGE_NOTIFIED );

	sprintf(pszMsg, g_Cfg.GetDefaultMsg( DEFMSG_MSG_GMPAGE_QNUM ), (int)(g_World.m_GMPages.GetContentCount()));
	SysMessage(pszMsg);

	// Already have a message in the queue ?
	// Find an existing GM page for this account.
	CGMPage * pPage = static_cast <CGMPage*>(g_World.m_GMPages.GetContainerHead());
	for ( ; pPage != nullptr; pPage = pPage->GetNext())
	{
		if (strcmpi( pPage->GetName(), GetAccount()->GetName()) == 0)
			break;
	}

	if ( pPage != nullptr )
	{
		SysMessageDefault( DEFMSG_MSG_GMPAGE_UPDATE );
		pPage->SetReason( pszReason );
		pPage->m_timePage = CWorldGameTime::GetCurrentTime().GetTimeRaw();
	}
	else
	{
		// Queue a GM page. (based on account)
		pPage = new CGMPage( GetAccount()->GetName());
		pPage->SetReason( pszReason );	// Description of reason for call.
	}

	pPage->m_ptOrigin = ptPlayerLocation; // Origin Point of call.
}

void CClient::Cmd_GM_PageClear()
{
	ADDTOCALLSTACK("CClient::Cmd_GM_PageClear");
	if ( m_pGMPage )
	{
		m_pGMPage->ClearGMHandler();
	}
}

void CClient::Cmd_GM_PageMenu( uint uiEntryStart )
{
	ADDTOCALLSTACK("CClient::Cmd_GM_PageMenu");
	// Just put up the GM page menu.
	SetPrivFlags( PRIV_GM_PAGE );
	Cmd_GM_PageClear();

	CMenuItem item[10];	// only display x at a time.
	ASSERT( CountOf(item)<=CountOf(m_tmMenu.m_Item));

	item[0].m_sText = "GM Page Menu";

	dword entry = 0;
	word count = 0;
	CGMPage * pPage = static_cast <CGMPage*>( g_World.m_GMPages.GetContainerHead());
	for ( ; pPage!= nullptr; pPage = pPage->GetNext(), entry++ )
	{
		if ( entry < uiEntryStart )
			continue;

		CClient * pGM = pPage->FindGMHandler();	// being handled ?
		if ( pGM != nullptr )
			continue;

		if ( ++count >= (CountOf( item )-1) )
		{
			// Add the "MORE" option if there is more than 1 more.
			if ( pPage->GetNext() != nullptr )
			{
				ASSERT(count < CountOf(item));
				item[count].m_id = count-1;
				item[count].m_sText.Format( "MORE" );
				item[count].m_color = 0;
				m_tmMenu.m_Item[count] = 0xFFFF0000 | entry;
				break;
			}
		}

		const CClient * pClient = pPage->FindAccount()->FindClient(); // logged in ?

		ASSERT(count < CountOf(item));
		item[count].m_id = count-1;
		item[count].m_color = 0;
		item[count].m_sText.Format("%s %s %s", pPage->GetName(), pClient ? "ON " : "OFF", pPage->GetReason());
		m_tmMenu.m_Item[count] = entry;
	}

	if ( count <= 0 )
	{
		SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_MSG_GMPAGES_NONE) );
		return;
	}

	ASSERT(count < CountOf(item));
	addItemMenu( CLIMODE_MENU_GM_PAGES, item, count );
}

void CClient::Cmd_GM_PageInfo()
{
	ADDTOCALLSTACK("CClient::Cmd_GM_PageInfo");
	// Show the current page.
	// This should be a dialog !!!??? book or scroll.

	ASSERT(m_pGMPage);
	SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_GMPAGES_CURRENT), m_pGMPage->GetName(), m_pGMPage->GetAccountStatus(), m_pGMPage->GetReason(), m_pGMPage->GetAge());
}

enum GPV_TYPE
{
	GPV_BAN,
	GPV_CURRENT,
	GPV_D,
	GPV_DELETE,
	GPV_GO,
	GPV_JAIL,
	GPV_L,	// List
	GPV_LIST,
	GPV_OFF,
	GPV_ON,
	GPV_ORIGIN,
	GPV_U,
	GPV_UNDO,
	GPV_WIPE,
	GPV_QTY
};

static lpctstr constexpr sm_pszGMPageVerbs[GPV_QTY] =
{
	"BAN",
	"CURRENT",
	"D",
	"DELETE",
	"GO",
	"JAIL",
	"L",	// List
	"LIST",
	"OFF",
	"ON",
	"ORIGIN",
	"U",
	"UNDO",
	"WIPE"
};

void CClient::Cmd_GM_PageCmd( lpctstr pszCmd )
{
	ADDTOCALLSTACK("CClient::Cmd_GM_PageCmd");
	static lpctstr constexpr sm_pszGMPageVerbsHelp[] =
	{
		".PAGE on/off\n",
		".PAGE list = list of pages.\n",
		".PAGE delete = dispose of this page. Assume it has been handled.\n",
		".PAGE origin = go to the origin point of the page\n",
		".PAGE undo/queue = put back in the queue\n",
		".PAGE current = info on the current selected page.\n",
		".PAGE go/player = go to the player that made the page. (if they are logged in)\n",
		".PAGE jail\n",
		".PAGE ban/kick\n",
		".PAGE wipe (gm only)"
	};
	// A GM page command.
	// Put up a list of GM pages pending.

	if ( pszCmd == nullptr || pszCmd[0] == '?' )
	{
		for ( size_t i = 0; i < CountOf(sm_pszGMPageVerbsHelp); ++i )
		{
			SysMessage( sm_pszGMPageVerbsHelp[i] );
		}
		return;
	}
	if ( pszCmd[0] == '\0' )
	{
		if ( m_pGMPage )
			Cmd_GM_PageInfo();
		else
			Cmd_GM_PageMenu();
		return;
	}

	int index = FindTableHeadSorted( pszCmd, sm_pszGMPageVerbs, CountOf(sm_pszGMPageVerbs) );
	if ( index < 0 )
	{
		Cmd_GM_PageCmd(nullptr);
		return;
	}

	switch ( index )
	{
		case GPV_OFF:
			if ( GetPrivLevel() < PLEVEL_Counsel )
				return;	// cant turn off.
			ClearPrivFlags( PRIV_GM_PAGE );
			Cmd_GM_PageClear();
			SysMessage( "GM pages off" );
			return;
		case GPV_ON:
			SetPrivFlags( PRIV_GM_PAGE );
			SysMessage( "GM pages on" );
			return;
		case GPV_WIPE:
			if ( ! IsPriv( PRIV_GM ))
				return;
			g_World.m_GMPages.ClearContainer();
			return;
	}

	if ( m_pGMPage == nullptr )
	{
		// No gm page has been selected yet.
		Cmd_GM_PageMenu();
		return;
	}

	// We must have a select page for these commands.
	switch ( index )
	{
		case GPV_L:	// List
		case GPV_LIST:
			Cmd_GM_PageMenu();
			return;
		case GPV_D:
		case GPV_DELETE:
			// /PAGE delete = dispose of this page. We assume it has been handled.
			SysMessage( "GM Page deleted" );
			delete m_pGMPage;
			ASSERT( m_pGMPage == nullptr );
			return;
		case GPV_ORIGIN:
			// /PAGE origin = go to the origin point of the page
			m_pChar->Spell_Teleport( m_pGMPage->m_ptOrigin, true, false );
			return;
		case GPV_U:
		case GPV_UNDO:
			// /PAGE queue = put back in the queue
			SysMessage( "GM Page re-queued." );
			Cmd_GM_PageClear();
			return;
		case GPV_BAN:
			// This should work even if they are not logged in.
			{
				CAccount * pAccount = m_pGMPage->FindAccount();
				if ( pAccount )
				{
					if ( ! pAccount->Kick( this, true ))
						return;
				}
				else
				{
					SysMessage( "Invalid account for page !?" );
				}
			}
			break;
		case GPV_CURRENT:
			// What am i servicing ?
			Cmd_GM_PageInfo();
			return;
	}

	// Manipulate the character only if logged in.

	CClient * pClient = m_pGMPage->FindAccount()->FindClient();
	if ( pClient == nullptr || pClient->GetChar() == nullptr )
	{
		SysMessage( "The account is not logged in." );
		if ( index == GPV_GO )
		{
			m_pChar->Spell_Teleport( m_pGMPage->m_ptOrigin, true, false );
		}
		return;
	}

	switch ( index )
	{
		case GPV_GO: // /PAGE player = go to the player that made the page. (if they are logged in)
			m_pChar->Spell_Teleport( pClient->GetChar()->GetTopPoint(), true, false );
			return;
		case GPV_BAN:
			pClient->addKick( m_pChar );
			return;
		case GPV_JAIL:
			pClient->GetChar()->Jail( this, true, 0 );
			return;
		default:
			return;
	}
}

void CClient::Cmd_GM_PageSelect( uint iSelect )
{
	ADDTOCALLSTACK("CClient::Cmd_GM_PageSelect");
	// 0 = cancel.
	// 1 based.

	if ( m_pGMPage != nullptr )
	{
		SysMessage( "Current message sent back to the queue" );
		Cmd_GM_PageClear();
	}

	if ( iSelect <= 0 || iSelect >= CountOf(m_tmMenu.m_Item))
	{
		return;
	}

	if ( m_tmMenu.m_Item[iSelect] & 0xFFFF0000 )
	{
		// "MORE" selected
		Cmd_GM_PageMenu( m_tmMenu.m_Item[iSelect] & 0xFFFF );
		return;
	}

	CGMPage * pPage = static_cast <CGMPage*>( g_World.m_GMPages.GetContentAt( m_tmMenu.m_Item[iSelect] ));
	if ( pPage != nullptr )
	{
		if ( pPage->FindGMHandler())
		{
			SysMessage( "GM Page already being handled." );
			return;	// someone already has this.
		}

		pPage->SetGMHandler( this );
		Cmd_GM_PageInfo();
		Cmd_GM_PageCmd( "GO" );	// go there.
	}
}

