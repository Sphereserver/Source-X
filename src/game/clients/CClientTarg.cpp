// An item is targetted.

#include "../../network/send.h"
#include "../../common/resource/sections/CItemTypeDef.h"
#include "../../common/sphere_library/CSRand.h"
#include "../../common/CExpression.h"
#include "../../common/CLog.h"
#include "../chars/CChar.h"
#include "../items/CItemCorpse.h"
#include "../items/CItemMulti.h"
#include "../items/CItemStone.h"
#include "../items/CItemVendable.h"
#include "../uo_files/CUOStaticItemRec.h"
#include "../uo_files/uofiles_enums_creid.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../CWorldSearch.h"
#include "../triggers.h"
#include "CClient.h"


////////////////////////////////////////////////////////
// Targetted GM functions.

bool CClient::OnTarg_Obj_Set( CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::OnTarg_Obj_Set");
	// CLIMODE_TARG_OBJ_SET
	// Targeted a command at an CObjBase object
	// ARGS:
	//  m_Targ_Text = new command and value.

	if ( pObj == nullptr )
	{
		SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_UNEXPECTED) );
		return false;
	}

	// Parse the command.
	tchar * pszLogMsg = Str_GetTemp();

	if ( pObj->IsItem() )
	{
		const CItem * pItem = static_cast <CItem*> (pObj);
		if ( pItem->GetAmount() > 1 )
			snprintf(pszLogMsg, Str_TempLength(), "'%s' commands uid=0%x (%s) [amount=%u] to '%s'",
                     GetName(), (dword)(pObj->GetUID()), pObj->GetName(), pItem->GetAmount(), static_cast<lpctstr>(m_Targ_Text));
		else
			snprintf(pszLogMsg, Str_TempLength(), "'%s' commands uid=0%x (%s) to '%s'", GetName(),
                     (dword)(pObj->GetUID()), pObj->GetName(), static_cast<lpctstr>(m_Targ_Text));
	}
	else
		snprintf(pszLogMsg, Str_TempLength(), "'%s' commands uid=0%x (%s) to '%s'", GetName(),
                 (dword)(pObj->GetUID()), pObj->GetName(), static_cast<lpctstr>(m_Targ_Text));

	// Check priv level for the new verb.
	if ( ! g_Cfg.CanUsePrivVerb( pObj, m_Targ_Text, this ))
	{
		SysMessageDefault( DEFMSG_MSG_CMD_LACKPRIV );
		g_Log.Event( LOGM_GM_CMDS, "%s=0\n", pszLogMsg);
		return false;
	}

	CScript sCmd( m_Targ_Text );

	if ( sCmd.IsKey("COLOR") )
	{
		// ".xCOLOR" command without arguments displays dye option
		if ( !sCmd.HasArgs() )
		{
			addDyeOption( pObj );
			return true;
		}
	}

	bool fRet = pObj->r_Verb( sCmd, this );
	if ( ! fRet )
	{
		SysMessageDefault( DEFMSG_MSG_ERR_INVSET );
	}

	if ( GetPrivLevel() >= g_Cfg.m_iCommandLog )
		g_Log.Event( LOGM_GM_CMDS, "%s=%d\n", pszLogMsg, fRet);
	return( fRet );
}


bool CClient::OnTarg_Obj_Function( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CClient::OnTarg_Obj_Function");
	m_Targ_p	= pt;
	lpctstr	pSpace	= strchr( m_Targ_Text, ' ' );
	if ( !pSpace )
		pSpace	= strchr( m_Targ_Text, '\t' );
	if ( pSpace )
		GETNONWHITESPACE( pSpace );

	CScriptTriggerArgs	Args( pSpace ? pSpace : "" );
	Args.m_VarsLocal.SetNum( "ID", id, true );
	Args.m_pO1	= pObj;
	CSString sVal;
	m_pChar->r_Call( static_cast<lpctstr>(m_Targ_Text), this, &Args, &sVal );
	return true;
}


bool CClient::OnTarg_Obj_Info( CObjBase * pObj, const CPointMap & pt, ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CClient::OnTarg_Obj_Info");
	// CLIMODE_TARG_OBJ_INFO "INFO"

	if ( pObj )
	{
		SetTargMode();
		addGumpDialogProps(pObj->GetUID());
	}
	else
	{
		tchar *pszTemp = Str_GetTemp();
		size_t len = 0;
		if ( id )
		{
			len = snprintf( pszTemp, Str_TempLength(), "[Static z=%d, 0%x=", pt.m_z, id );

			// static items have no uid's but we can still use them.
			CItemBase * pItemDef = CItemBase::FindItemBase(id);
			if ( pItemDef )
			{
				len += snprintf( pszTemp+len, Str_TempLength() - len, "%s->%s], ", pItemDef->GetResourceName(),
					g_Cfg.ResourceGetName( CResourceID( RES_TYPEDEF, pItemDef->GetType() )));
			}
			else
			{
				len += snprintf( pszTemp+len, Str_TempLength() - len, "NON scripted], " );
			}
		}
		else
		{
			// tile info for location.
			len = Str_CopyLimitNull( pszTemp, "[No static tile], ", Str_TempLength());
		}

		std::optional<CUOMapMeter> pMeter = CWorldMap::GetMapMeterAdjusted( pt );
		if ( pMeter )
		{
			len += snprintf( pszTemp+len, Str_TempLength() - len, "TERRAIN=0%x   TYPE=%s",
				pMeter->m_wTerrainIndex,
				CWorldMap::GetTerrainItemTypeDef( pMeter->m_wTerrainIndex )->GetResourceName() );
		}

		SysMessage(pszTemp);
	}

	return true;
}

bool CClient::Cmd_Control( CChar * pChar2 )
{
	ADDTOCALLSTACK("CClient::Cmd_Control");
	// I wish to control pChar2
	// Leave my own body behind.

	if ( pChar2 == nullptr )
		return false;
	if ( pChar2->IsDisconnected())
		return false;	// char is not on-line. (then we should make it so !)

	if ( GetPrivLevel() < pChar2->GetPrivLevel())
		return false;

	ASSERT(m_pChar);
	CChar * pChar1 = m_pChar;

	//Switch their home position to avoid the pChar1 corpse teleport to his home(far away)
	CPointMap homeP1 = pChar1->m_ptHome;
	pChar1->m_ptHome.Set(pChar2->m_ptHome);
	pChar2->m_ptHome.Set(homeP1);

	// Put my newbie equipped items on it.
	for (CSObjContRec* pObjRec : pChar1->GetIterationSafeContReverse())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		if ( !pItem->IsAttr(ATTR_MOVE_NEVER) )
			continue; // keep GM stuff.
		if ( !CItemBase::IsVisibleLayer(pItem->GetEquipLayer()) )
			continue;
		switch ( pItem->GetEquipLayer() )
		{
			case LAYER_BEARD:
			case LAYER_HAIR:
			case LAYER_PACK:
				continue;

			default:
				break;
		}
		pChar2->LayerAdd(pItem);	// add content
	}

	// Put my GM pack stuff in it's inventory.
	CItemContainer *pPack1 = pChar1->GetPack();
	CItemContainer *pPack2 = pChar2->GetPackSafe();
	if ( pPack1 && pPack2 )
	{
		for (CSObjContRec* pObjRec : pPack1->GetIterationSafeContReverse())
		{
			CItem* pItem = static_cast<CItem*>(pObjRec);
			if ( !pItem->IsAttr(ATTR_MOVE_NEVER) )	// keep newbie stuff.
				continue;
			pPack2->ContentAdd(pItem);	// add content
		}
	}

	pChar1->ClientDetach();
	m_pChar = nullptr;
	CClient * pClient2 = pChar2->GetClientActive();
	if ( pClient2 )	// controled char is a player/client.
	{
		pChar2->ClientDetach();
		pClient2->m_pChar = nullptr;
	}

	CCharPlayer * pPlayer1 = pChar1->m_pPlayer;
	if ( pPlayer1 )
	{
		pPlayer1->GetAccount()->DetachChar(pChar1);
	}
	CCharPlayer * pPlayer2 = pChar2->m_pPlayer;
	if ( pPlayer2 )
	{
		pPlayer2->GetAccount()->DetachChar(pChar2);
	}

	// swap m_pPlayer (if they even are both players.)

	pChar1->m_pPlayer = pPlayer2;
	pChar2->m_pPlayer = pPlayer1;

	ASSERT( pChar1->m_pNPC == nullptr );
	pChar1->m_pNPC = pChar2->m_pNPC;	// Turn my old body into a NPC. (if it was)
	pChar2->m_pNPC = nullptr;

	if ( pPlayer1 )
	{
		pPlayer1->GetAccount()->AttachChar(pChar2);
	}
	if ( pPlayer2 )
	{
		pPlayer2->GetAccount()->AttachChar(pChar1);
	}
	if ( pClient2 )
	{
		pClient2->addPlayerStart( pChar1 );
	}
	else
	{
		// delete my ghost.
		if ( pChar1->GetID() == CREID_EQUIP_GM_ROBE ||
			pChar1->GetID() == CREID_GHOSTMAN ||
			pChar1->GetID() == CREID_GHOSTWOMAN ||
			pChar1->GetID() == CREID_ELFGHOSTMAN ||
			pChar1->GetID() == CREID_ELFGHOSTWOMAN ||
			pChar1->GetID() == CREID_GARGGHOSTMAN ||
			pChar1->GetID() == CREID_GARGGHOSTWOMAN )	// CREID_EQUIP_GM_ROBE
		{
			pChar1->Delete();
		}
		else
		{
			pChar1->SetTimeout(1);	// must kick start the npc.
		}
	}
	addPlayerStart( pChar2 );
	return true;
}

bool CClient::OnTarg_UnExtract( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_UnExtract");
	UnreferencedParameter(pObj);
	// CLIMODE_TARG_UNEXTRACT
	// ??? Get rid of this in favor of a more .SCP file type approach.
	// result of the MULTI command.
	// Break a multi out of the multi.txt files and turn it into items.

	if ( !pt.GetRegion(REGION_TYPE_AREA) )
		return false;

	CScript s;	// It is not really a valid script type file.
	if ( ! g_Cfg.OpenResourceFind( s, m_Targ_Text ))
		return false;

	tchar *pszTemp = Str_GetTemp();
	sprintf(pszTemp, "%i template id", m_tmTile.m_id);
	if ( ! s.FindTextHeader(pszTemp))
		return false;
	if ( ! s.ReadKey())
		return false; // throw this one away
	if ( ! s.ReadKeyParse())
		return false; // this has the item count

	int iItemCount = atoi(s.GetKey()); // item count
	while (iItemCount > 0)
	{
		if ( ! s.ReadKeyParse())
			return false; // this has the item count

		int64 piCmd[4];		// Maximum parameters in one line
		Str_ParseCmds( s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd));

		CItem * pItem = CItem::CreateTemplate((ITEMID_TYPE)(atoi(s.GetKey())), nullptr, m_pChar);
		if ( pItem == nullptr )
			return false;

		CPointMap ptOffset( (word)(piCmd[0]), (word)(piCmd[1]), (char)(piCmd[2]) );
		ptOffset += pt;
		ptOffset.m_map = pt.m_map;
		pItem->MoveToUpdate( ptOffset );
		iItemCount --;
	}

	return true;
}

bool CClient::OnTarg_Char_Add( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Char_Add");
	// CLIMODE_TARG_ADDCHAR
	// m_tmAdd.m_id = char id
	ASSERT(m_pChar);

	if ( !pt.GetRegion(REGION_TYPE_AREA) )
		return false;
	if ( pObj && pObj->IsItemInContainer() )
		return false;

	CChar *pChar = CChar::CreateBasic(static_cast<CREID_TYPE>(m_tmAdd.m_id));
	if ( !pChar )
		return false;

	pChar->NPC_LoadScript(true);
	pChar->MoveToChar(pt);
	pChar->NPC_CreateTrigger();		// removed from NPC_LoadScript() and triggered after char placement
	pChar->Update();
    pChar->UpdateAnimate(ANIM_SUMMON);
    pChar->SetTimeout(2000);
	pChar->SoundChar(CRESND_GETHIT);
	m_pChar->m_Act_UID = pChar->GetUID();		// for last target stuff. (trigger stuff)
	return true;
}

bool CClient::OnTarg_Item_Add( CObjBase * pObj, CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Item_Add");
	// CLIMODE_TARG_ADDITEM
	// m_tmAdd.m_id = item id
	ASSERT(m_pChar);

	if ( !pt.GetRegion(REGION_TYPE_AREA) )
		return false;
	if ( pObj && pObj->IsItemInContainer() )
		return false;

	CItem *pItem = CItem::CreateTemplate((ITEMID_TYPE)m_tmAdd.m_id, nullptr, m_pChar);
	if ( !pItem )
		return false;

	if ( pItem->IsTypeMulti() )
	{
		CItem *pMulti = OnTarg_Use_Multi(pItem->Item_GetDef(), pt, pItem);
		pItem->Delete();
		return pMulti ? true : false;
	}
	else
		pItem->SetAmount(m_tmAdd.m_vcAmount);

	pItem->MoveToCheck(pt, m_pChar);
	m_pChar->m_Act_UID = pItem->GetUID();		// for last target stuff (trigger stuff) and to make AxisII able to initialize placed spawn items.
	return true;
}

bool CClient::OnTarg_Item_Link( CObjBase * pObj2 )
{
	ADDTOCALLSTACK("CClient::OnTarg_Item_Link");
	// CLIMODE_LINK

	if ( pObj2 == nullptr )
	{
		SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_DYNAMIC) );
		return false;
	}

	CItem * pItem2 = dynamic_cast <CItem*>(pObj2);
	CItem * pItem1 = m_Targ_UID.ItemFind();
	if ( pItem1 == nullptr )
	{
		if ( pItem2 == nullptr )
		{
			m_Targ_UID.InitUID();
			addTarget( CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg( DEFMSG_MSG_TARG_LINK ) );
		}
		else
		{
			m_Targ_UID = pObj2->GetUID();
			addTarget( CLIMODE_TARG_LINK, g_Cfg.GetDefaultMsg( DEFMSG_MSG_TARG_LINK2 ) );
		}
		return true;
	}

	if ( pItem2 == pItem1 )
	{
		SysMessageDefault( DEFMSG_MSG_TARG_LINK_SAME );
		// Break any existing links.
		return false;
	}

	if ( pItem2 && ( pItem1->IsType(IT_KEY) || pItem2->IsType(IT_KEY)))
	{
		// Linking a key to something is a special case.
		if ( ! pItem1->IsType(IT_KEY))
		{
			CItem * pTmp = pItem1;
			pItem1 = pItem2;
			pItem2 = pTmp;
		}

		// pItem1 = the IT_KEY
		if (pItem2->m_itContainer.m_UIDLock.IsValidUID())
		{
			pItem1->m_itKey.m_UIDLock = pItem2->m_itContainer.m_UIDLock;
		}
		else if (pItem1->m_itKey.m_UIDLock.IsValidUID())
		{
			pItem2->m_itContainer.m_UIDLock = pItem1->m_itKey.m_UIDLock;
		}
		else
		{
			pItem1->m_itKey.m_UIDLock = pItem2->m_itContainer.m_UIDLock = pItem2->GetUID();
		}
	}
	else
	{
		pItem1->m_uidLink = pObj2->GetUID();
		if ( pItem2 && ! pItem2->m_uidLink.IsValidUID())
		{
			pItem2->m_uidLink = pItem1->GetUID();
		}
	}

	SysMessageDefault( DEFMSG_MSG_TARG_LINKS );
	return true;
}

int CClient::Cmd_Extract( CScript * pScript, const CRectMap &rect, int & zlowest )
{
	ADDTOCALLSTACK("CClient::Cmd_Extract");
	// RETURN: Number of statics here.
	CPointMap ptCtr = rect.GetCenter();

	int iCount = 0;
	for ( int mx = rect.m_left; mx <= rect.m_right; mx++)
	{
		for ( int my = rect.m_top; my <= rect.m_bottom; my++)
		{
			CPointMap ptCur((word)(mx), (word)(my), 0, (uchar)(rect.m_map));
			const CServerMapBlock * pBlock = CWorldMap::GetMapBlock( ptCur );
			if ( pBlock == nullptr )
				continue;
			size_t iQty = pBlock->m_Statics.GetStaticQty();
			if ( iQty <= 0 )  // no static items here.
				continue;

			int x2 = pBlock->GetOffsetX(mx);
			int y2 = pBlock->GetOffsetY(my);
			for ( uint i = 0; i < iQty; ++i )
			{
				if ( ! pBlock->m_Statics.IsStaticPoint( i, x2, y2 ))
					continue;
				const CUOStaticItemRec * pStatic = pBlock->m_Statics.GetStatic(i);
				ASSERT(pStatic);
				iCount ++;
				if ( pScript )
				{
					// This static is at the coordinates in question.
					pScript->Printf( "%u %i %i %i 0\n",
						pStatic->GetDispID(), mx - ptCtr.m_x, my - ptCtr.m_y, pStatic->m_z - zlowest);
				}
				else
				{
					if ( pStatic->m_z < zlowest)
					{
						zlowest = pStatic->m_z;
					}
				}
			}
		}
	}

	// Extract Multi's ???


	// Extract dynamics as well.

	int rx = 1 + abs( rect.m_right - rect.m_left ) / 2;
	int ry = 1 + abs( rect.m_bottom - rect.m_top ) / 2;

	auto AreaItem = CWorldSearchHolder::GetInstance( ptCtr, maximum( rx, ry ));
	AreaItem->SetSearchSquare( true );
	for (;;)
	{
		CItem * pItem = AreaItem->GetItem();
		if ( pItem == nullptr )
			break;
		if ( ! rect.IsInside2d( pItem->GetTopPoint()))
			continue;

		CPointMap pt = pItem->GetTopPoint();
		iCount ++;
		if ( pScript )
		{
			// This static is at the coordinates in question.
			pScript->Printf( "%u %i %i %i 0\n",
				pItem->GetDispID(), pt.m_x - ptCtr.m_x, pt.m_y - ptCtr.m_y, pt.m_z - zlowest );
		}
		else
		{
			if ( pt.m_z < zlowest)
			{
				zlowest = pt.m_z;
			}
		}
	}

	return iCount;
}

bool CClient::OnTarg_Tile( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Tile");
	// CLIMODE_TARG_TILE
	// m_tmTile.m_Code = CV_TILE etc
	// CV_NUKE. CV_NUKECHAR, CV_EXTRACT, CV_NUDGE
	//

	ASSERT(m_pChar);

	if ( pObj && ! pObj->IsTopLevel())
		return false;
	if ( ! pt.IsValidPoint())
		return false;

	if ( !m_tmTile.m_ptFirst.IsValidPoint())
	{
		m_tmTile.m_ptFirst = pt;
		addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_MSG_TARG_PC ), true );
		return true;
	}

	if ( pt == m_tmTile.m_ptFirst && m_tmTile.m_Code != CV_EXTRACT ) // Extract can work with one square
	{
		SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_TILE_SAME_POINT ) );
		addTarget( CLIMODE_TARG_TILE, g_Cfg.GetDefaultMsg( DEFMSG_MSG_TARG_PC ), true );
		return true;
	}

	CRectMap rect;
	rect.SetRect( m_tmTile.m_ptFirst.m_x, m_tmTile.m_ptFirst.m_y, pt.m_x, pt.m_y, pt.m_map);
	CPointMap ptCtr(rect.GetCenter());
	ptCtr.m_map = pt.m_map;

	int rx = 1 + abs( rect.m_right - rect.m_left ) / 2;
	int ry = 1 + abs( rect.m_bottom - rect.m_top ) / 2;
	int iRadius = maximum( rx, ry );

	int iCount = 0;

	switch ( m_tmTile.m_Code )
	{
	case CV_EXTRACT:
		{
		// "EXTRACT" all map statics in the region.
		// First find the lowest Z to use as a base
		// and count the statics
		int zlowest = UO_SIZE_Z;
		iCount = Cmd_Extract( nullptr, rect, zlowest );
		if ( iCount )
		{
			CScript s;
			if ( ! s.Open( m_Targ_Text, OF_WRITE|OF_TEXT|OF_DEFAULTMODE ))
				return false;

			// Write a header for this multi in XXX format
			// (i have no idea what most of this means)
			s.Printf("6 version\n");
			s.Printf("%d template id\n", m_tmTile.m_id );
			s.Printf("-1 item version\n");
			s.Printf("%i num components\n", iCount);

			Cmd_Extract( &s, rect, zlowest );
		}

		SysMessagef( "%d Statics Extracted to '%s', id=%d", iCount, static_cast<lpctstr>(m_Targ_Text), m_tmTile.m_id );
		}
		break;

	case CV_NUDGE:
		{
			tchar szTmp[512];
			Str_CopyLimitNull( szTmp, m_Targ_Text, ARRAY_COUNT(szTmp));

			int64 piArgs[3];		// Maximum parameters in one line
			Str_ParseCmds( szTmp, piArgs, ARRAY_COUNT( piArgs ));

			CPointMap ptNudge((word)(piArgs[0]),(word)(piArgs[1]),(char)(piArgs[2]) );

			auto Area = CWorldSearchHolder::GetInstance( ptCtr, iRadius );
			Area->SetAllShow( IsPriv( PRIV_ALLSHOW ));
			Area->SetSearchSquare( true );
			for (;;)
			{
				CItem * pItem = Area->GetItem();
				if ( pItem == nullptr )
					break;
				if ( ! rect.IsInside2d( pItem->GetTopPoint()))
					continue;
				CPointMap ptMove = pItem->GetTopPoint();
				ptMove += ptNudge;
				pItem->MoveToCheck( ptMove );
				++iCount;
			}

            Area->RestartSearch();
            for (;;)
			{
				CChar* pChar = Area->GetChar();
				if ( pChar == nullptr )
					break;
				if ( ! rect.IsInside2d( pChar->GetTopPoint()))
					continue;
				CPointMap ptMove = pChar->GetTopPoint();
				ptMove += ptNudge;
				pChar->MoveTo(ptMove);
				iCount++;
			}

			SysMessagef( "%d %s", iCount, g_Cfg.GetDefaultMsg( DEFMSG_NUDGED_OBJECTS ) );
		}
		break;

	case CV_NUKE:		// NUKE all items in the region.
		{
			auto AreaItem = CWorldSearchHolder::GetInstance( ptCtr, iRadius );
			AreaItem->SetAllShow( IsPriv( PRIV_ALLSHOW ));
			AreaItem->SetSearchSquare( true );
			for (;;)
			{
				CItem * pItem = AreaItem->GetItem();
				if ( pItem == nullptr )
					break;
				if ( ! rect.IsInside2d( pItem->GetTopPoint()))
					continue;

				if ( m_Targ_Text.IsEmpty())
				{
					pItem->Delete();
				}
				else
				{
					CScript script(m_Targ_Text);
					if ( ! pItem->r_Verb( script, this ))
						continue;
				}
				iCount++;
			}
			SysMessagef( "%d %s", iCount, g_Cfg.GetDefaultMsg( DEFMSG_NUKED_ITEMS ) );
		}
		break;

	case CV_NUKECHAR:
		{
			auto AreaChar = CWorldSearchHolder::GetInstance( ptCtr, iRadius );
			AreaChar->SetAllShow( IsPriv( PRIV_ALLSHOW ));
			AreaChar->SetSearchSquare( true );
			for (;;)
			{
				CChar* pChar = AreaChar->GetChar();
				if ( pChar == nullptr )
					break;
				if ( ! rect.IsInside2d( pChar->GetTopPoint()))
					continue;
				if ( pChar->m_pPlayer )
					continue;
				if ( m_Targ_Text.IsEmpty())
				{
					pChar->Delete();
				}
				else
				{
					CScript script(m_Targ_Text);
					if ( ! pChar->r_Verb( script, this ))
						continue;
				}
				iCount++;
			}
			SysMessagef( "%d %s", iCount, g_Cfg.GetDefaultMsg( DEFMSG_NUKED_CHARS ) );
		}
		break;

	case CV_TILE:
		{
			tchar szTmp[256];
			Str_CopyLimitNull( szTmp, m_Targ_Text, ARRAY_COUNT(szTmp));

			int64 piArgs[16];		// Maximum parameters in one line
			int iArgQty = Str_ParseCmds( szTmp, piArgs, ARRAY_COUNT( piArgs ));

			char z = (char)(piArgs[0]);	// z height is the first arg.
			int iArg = 0;
			for ( int mx = rect.m_left; mx <= rect.m_right; mx++)
			{
				for (int my = rect.m_top; my <= rect.m_bottom; my++)
				{
					if ( ++iArg >= iArgQty )
						iArg = 1;
                    CItem *pItem = CItem::CreateTemplate((ITEMID_TYPE)(ResGetIndex((dword)piArgs[iArg])), nullptr, m_pChar);
                    if (!pItem)
                        continue;
					pItem->SetAttr( ATTR_MOVE_NEVER );
					CPointMap ptCur((word)mx, (word)my, z, pt.m_map);
					pItem->MoveToUpdate( ptCur );
					++iCount;
				}
			}

			SysMessagef( "%d %s", iCount, g_Cfg.GetDefaultMsg( DEFMSG_TILED_ITEMS ) );
		}
		break;
	}

	return true;
}

//-----------------------------------------------------------------------
// Targetted Informational skills

int CClient::OnSkill_AnimalLore( CUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_AnimalLore");
	UnreferencedParameter(iSkillLevel);
	// SKILL_ANIMALLORE
	// The creature is a "human" etc..
	// How happy.
	// Well trained.
	// Who owns it ?
	// What it eats.

	// Other "lore" type things about a creature ?
	// ex. liche = the remnants of a powerful wizard

	CChar * pChar = uid.CharFind();
	if ( pChar == nullptr )
	{
		SysMessageDefault( DEFMSG_NON_ALIVE );
		return -1;
	}

	if ( fTest )
	{
		if ( pChar == m_pChar )
			return( 2 );
		if ( m_pChar->IsStatFlag( STATF_ONHORSE ) )
		{
			CItem * pItem = m_pChar->LayerFind( LAYER_HORSE );
			if ( pItem && pItem->m_itFigurine.m_UID == uid)
				return 1;
		}
		if ( pChar->IsPlayableCharacter())
			return( g_Rand.GetVal(10));
		return g_Rand.GetVal(60);
	}

	lpctstr pszHe = pChar->GetPronoun();
	lpctstr pszHis = pChar->GetPossessPronoun();

	tchar *pszTemp = Str_GetTemp();

	// What kind of animal.
	if ( pChar->IsIndividualName())
	{
		snprintf(pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_RESULT), pChar->GetName(), pChar->Char_GetDef()->GetTradeName());
		addObjMessage(pszTemp, pChar);
	}

	// Who is master ?
	CChar * pCharOwner = pChar->NPC_PetGetOwner();
	if ( pCharOwner == nullptr )
	{
		snprintf(pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg( DEFMSG_ANIMALLORE_FREE ), pszHe, pszHis);
	}
	else
	{
		snprintf(pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg( DEFMSG_ANIMALLORE_MASTER ), pszHe, ( pCharOwner == m_pChar ) ? g_Cfg.GetDefaultMsg( DEFMSG_ANIMALLORE_MASTER_YOU ) : pCharOwner->GetName());
		// How loyal to master ?
	}
	addObjMessage(pszTemp, pChar );

	// How well fed ?
	// Food count = 30 minute intervals.
	lpctstr pszText = pChar->IsStatFlag(STATF_CONJURED) ?
						g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_CONJURED) :
						pChar->Food_GetLevelMessage(pCharOwner ? true : false, true);

	snprintf(pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_ANIMALLORE_FOOD), pszHe, pszText);
	addObjMessage(pszTemp, pChar);

	return 0;
}

int CClient::OnSkill_ItemID( CUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_ItemID");
	// SKILL_ITEMID

	CObjBase * pObj = uid.ObjFind();
	if ( pObj == nullptr )
	{
		return -1;
	}

	if ( pObj->IsChar())
	{
		CChar * pChar = static_cast <CChar*>(pObj);
		ASSERT(pChar);
		if ( fTest )
		{
			return 1;
		}

		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMID_RESULT ), pChar->GetName());
		return 1;
	}

	CItem * pItem = static_cast <CItem*>(pObj);
	ASSERT( pItem );

	if ( fTest )
	{
		if ( pItem->IsAttr( ATTR_IDENTIFIED ))
		{
			// already identified so easier.
			return g_Rand.GetVal(20);
		}
		return g_Rand.GetVal(60);
	}

	pItem->SetAttr(ATTR_IDENTIFIED);

	// ??? Estimate it's worth ?

	CItemVendable * pItemVend = dynamic_cast <CItemVendable *>(pItem);
	if ( pItemVend == nullptr )
	{
		SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_ITEMID_NOVAL ));
	}
	else
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_ITEMID_GOLD ),
			(pItemVend->GetVendorPrice(-15,0) * pItem->GetAmount()), pItemVend->GetNameFull(true));
	}

	// Whats it made of ?

	CItemBase * pItemDef = pItem->Item_GetDef();
	ASSERT(pItemDef);

	if ( (iSkillLevel > 40) && !pItemDef->m_BaseResources.empty())
	{
		tchar *pszTemp = Str_GetTemp();
		Str_CopyLimitNull(pszTemp, g_Cfg.GetDefaultMsg( DEFMSG_ITEMID_MADEOF ), Str_TempLength());

		pItemDef->m_BaseResources.WriteNames( pszTemp + strlen(pszTemp), Str_TempLength() );
		SysMessage( pszTemp );
	}

	// It required what skills to make ?
	// "It requires lots of mining skill"

	return iSkillLevel;
}

int CClient::OnSkill_EvalInt( CUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_EvalInt");
	// SKILL_EVALINT
	// iSkillLevel = 0 to 1000
	CChar * pChar = uid.CharFind();
	if ( pChar == nullptr )
	{
		SysMessageDefault( DEFMSG_NON_ALIVE );
		return -1;
	}

	if ( fTest )
	{
		if ( pChar == m_pChar )
			return( 2 );
		return g_Rand.GetVal(60);
	}

	static lpctstr const sm_szIntDesc[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_6 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_7 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_8 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_9 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_INT_10 )
	};

	int iIntVal = pChar->Stat_GetAdjusted(STAT_INT);
	int iIntEntry = (iIntVal-1) / 10;
	if ( iIntEntry < 0 )
		iIntEntry = 0;
	if ( (uint)iIntEntry >= ARRAY_COUNT( sm_szIntDesc ))
		iIntEntry = ARRAY_COUNT( sm_szIntDesc )-1;

	SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_RESULT ), pChar->GetName(), sm_szIntDesc[iIntEntry]);

	static lpctstr const sm_szMagicDesc[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAG_6 )
	};

	static lpctstr const sm_szManaDesc[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_5 ),	// 100 %
		g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_MAN_6 )
	};

	if ( iSkillLevel > 400 )	// magery skill and mana level ?
	{
		int iMagerySkill = pChar->Skill_GetAdjusted(SKILL_MAGERY);
		int iNecroSkill = pChar->Skill_GetAdjusted(SKILL_NECROMANCY);
		int iMagicSkill = maximum(iMagerySkill,iNecroSkill);

		int iMagicEntry = iMagicSkill / 200;
		if ( iMagicEntry < 0 )
			iMagicEntry = 0;
		if ( (uint)iMagicEntry >= ARRAY_COUNT(sm_szMagicDesc))
			iMagicEntry = ARRAY_COUNT(sm_szMagicDesc)-1;

		int iManaEntry = 0;
		if ( iIntVal )
			iManaEntry = IMulDiv( pChar->Stat_GetVal(STAT_INT), ARRAY_COUNT(sm_szManaDesc)-1, iIntVal );

		if ( iManaEntry < 0 )
			iManaEntry = 0;
		if ( (uint)iManaEntry >= ARRAY_COUNT(sm_szManaDesc))
			iManaEntry = ARRAY_COUNT(sm_szManaDesc)-1;

		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_EVALINT_RESULT_2 ), static_cast<lpctstr>(sm_szMagicDesc[iMagicEntry]), static_cast<lpctstr>(sm_szManaDesc[iManaEntry]));
	}

	return iSkillLevel;
}

int CClient::OnSkill_ArmsLore( CUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_ArmsLore");

	// SKILL_ARMSLORE
	CItem * pItem = uid.ItemFind();
	if ( pItem == nullptr || ! pItem->IsTypeArmorWeapon())
	{
		SysMessageDefault( DEFMSG_ARMSLORE_UNABLE );
		return -SKTRIG_QTY;
	}

	if ( fTest )
	{
		return g_Rand.GetVal(60);
	}

	tchar *pszTemp = Str_GetTemp();
	size_t len = 0;
	bool fWeapon;
	int iHitsCur;
	int iHitsMax;

	switch ( pItem->GetType() )
	{
		case IT_ARMOR:				// some type of armor. (no real action)
		case IT_SHIELD:
		case IT_ARMOR_BONE:
		case IT_ARMOR_CHAIN:
		case IT_ARMOR_LEATHER:
		case IT_ARMOR_RING:
		case IT_CLOTHING:
		case IT_JEWELRY:
			fWeapon = false;
			iHitsCur = pItem->m_itArmor.m_wHitsCur;
			iHitsMax = pItem->m_itArmor.m_wHitsMax;
			len += snprintf( pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DEF ), pItem->Armor_GetDefense());
			break;
		case IT_WEAPON_MACE_CROOK:
		case IT_WEAPON_MACE_PICK:
		case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
		case IT_WEAPON_SWORD:
		case IT_WEAPON_AXE:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_BOW:
		case IT_WEAPON_XBOW:
		case IT_WEAPON_THROWING:
			fWeapon = true;
			iHitsCur = pItem->m_itWeapon.m_wHitsCur;
			iHitsMax = pItem->m_itWeapon.m_wHitsMax;
			len += snprintf( pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_DAM ), pItem->Weapon_GetAttack());
			break;
		default:
			SysMessageDefault( DEFMSG_ARMSLORE_UNABLE );
			return -SKTRIG_QTY;
	}

	len += snprintf( pszTemp+len, Str_TempLength() - len, g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_REP ), pItem->Armor_GetRepairDesc());

	if ( iHitsCur <= 3 || iHitsMax <= 3 )
	{
		len += Str_CopyLimitNull( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ARMSLORE_REP_0 ), Str_TempLength() - len);
	}

	// Magical effects ?
	if ( pItem->IsAttr(ATTR_MAGIC))
	{
		len += Str_CopyLimitNull( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEM_MAGIC ), Str_TempLength() - len);
	}
	else if ( pItem->IsAttr(ATTR_NEWBIE|ATTR_MOVE_NEVER))
	{
		len += Str_CopyLimitNull( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEM_NEWBIE ), Str_TempLength() - len);
	}

	// Repairable ?
	if ( ! pItem->Armor_IsRepairable())
	{
		len += Str_CopyLimitNull( pszTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEM_REPAIR ), Str_TempLength() - len);
	}

	static const lpctstr sm_szPoisonMessages[] =
	{
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_1),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_2),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_3),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_4),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_5),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_6),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_7),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_8),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_9),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_10)
	};

	// Poisoned ?
	if ( fWeapon && pItem->m_itWeapon.m_poison_skill )
	{
		uint iLevel = (uint)IMulDiv(
			n_promote_n32(pItem->m_itWeapon.m_poison_skill),
			usize_narrow_u32(ARRAY_COUNT(sm_szPoisonMessages)),
			100);
		if ( iLevel >= ARRAY_COUNT(sm_szPoisonMessages))
			iLevel = usize_narrow_u32(ARRAY_COUNT(sm_szPoisonMessages)) - 1;
		len += snprintf( pszTemp+len, Str_TempLength() - len, " %s", sm_szPoisonMessages[iLevel] );
	}

	SysMessage(pszTemp);
	return iSkillLevel;
}

int CClient::OnSkill_Anatomy( CUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_Anatomy");
	// SKILL_ANATOMY
	CChar * pChar = uid.CharFind();
	if ( pChar == nullptr )
	{
		addObjMessage( g_Cfg.GetDefaultMsg( DEFMSG_NON_ALIVE ), pChar );
		return -1;
	}

	if ( fTest )
	{
		// based on rareity ?
		if ( pChar == m_pChar )
			return( 2 );
		return g_Rand.GetVal(60);
	}

	// Add in error cased on your skill level.

	static lpctstr const sm_szStrEval[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_6 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_7 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_8 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_9 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_STR_10 )
	};
	static lpctstr const sm_szDexEval[] =
	{
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_1 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_2 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_3 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_4 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_5 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_6 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_7 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_8 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_9 ),
		g_Cfg.GetDefaultMsg( DEFMSG_ANATOMY_DEX_10 )
	};

	int iStrVal = pChar->Stat_GetAdjusted(STAT_STR);
	int iStrEntry = (iStrVal-1)/10;
	if ( iStrEntry < 0 )
		iStrEntry = 0;
	if ( (uint)iStrEntry >= ARRAY_COUNT( sm_szStrEval ))
		iStrEntry = ARRAY_COUNT( sm_szStrEval )-1;

	int iDexVal = pChar->Stat_GetAdjusted(STAT_DEX);
	int iDexEntry = (iDexVal-1)/10;
	if ( iDexEntry < 0 )
		iDexEntry = 0;
	if ( (uint)iDexEntry >= ARRAY_COUNT( sm_szDexEval ))
		iDexEntry = ARRAY_COUNT( sm_szDexEval )-1;

	tchar * pszTemp = Str_GetTemp();
	snprintf(pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_RESULT), pChar->GetName(), sm_szStrEval[iStrEntry], sm_szDexEval[iDexEntry]);
	addObjMessage(pszTemp, pChar);

	if ( pChar->IsStatFlag(STATF_CONJURED) )
		addObjMessage(g_Cfg.GetDefaultMsg(DEFMSG_ANATOMY_MAGIC), pChar);

	// ??? looks hungry ?
	return iSkillLevel;
}

int CClient::OnSkill_Forensics( CUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_Forensics");
	// SKILL_FORENSICS
	// ! weird client issue targetting corpses !

	CItemCorpse * pCorpse = dynamic_cast<CItemCorpse *>(uid.ItemFind());
	if ( !pCorpse )
	{
		SysMessageDefault( DEFMSG_FORENSICS_CORPSE );
		return -SKTRIG_QTY;
	}
	if ( !m_pChar->CanTouch(pCorpse) )
	{
		SysMessageDefault( DEFMSG_FORENSICS_REACH );
		return -SKTRIG_QTY;
	}

	if (fTest)
		return (pCorpse->m_uidLink == m_pChar->GetUID()) ? 2 : g_Rand.GetVal(60);

	CChar * pCharKiller = pCorpse->m_itCorpse.m_uidKiller.CharFind();
	lpctstr pName = pCharKiller ? pCharKiller->GetName() : nullptr;

	if ( pCorpse->IsCorpseSleeping() )
	{
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_FORENSICS_ALIVE ), pName ? pName : "It" );
		return 1;
	}

	tchar * pszTemp = Str_GetTemp();
	if ( pCorpse->m_itCorpse.m_carved )
	{
		int len = snprintf( pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_CARVE_1), pCorpse->GetName() );
		if ( pName )
			snprintf( pszTemp + len, Str_TempLength() - len, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_CARVE_2), pName );
		else
			Str_CopyLimitNull( pszTemp + len, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_FAILNAME), Str_TempLength() - len);

	}
	else if ( pCorpse->GetTimeStampS() > 0 )
	{
		int len = snprintf( pszTemp, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_TIMER),
            pCorpse->GetName(),
            (CWorldGameTime::GetCurrentTime().GetTimeDiff(pCorpse->GetTimeStampS()) / MSECS_PER_SEC));

		if ( pName )
			snprintf( pszTemp + len, Str_TempLength() - len, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_NAME), pName );
		else
			Str_CopyLimitNull( pszTemp + len, g_Cfg.GetDefaultMsg(DEFMSG_FORENSICS_FAILNAME), Str_TempLength() - len );
	}
	SysMessage( pszTemp );
	return iSkillLevel;
}

int CClient::OnSkill_TasteID( CUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_TasteID");
	// SKILL_TASTEID
	// Try to taste for poisons I assume.
	// Maybe taste what it is made of for ingredients ?
	// Differntiate potion types ?

	CItem * pItem = uid.ItemFind();
	if ( pItem == nullptr )
	{
		if ( uid == m_pChar->GetUID())
		{
			SysMessageDefault( DEFMSG_TASTEID_SELF );
		}
		else
		{
			SysMessageDefault( DEFMSG_TASTEID_CHAR );
		}
		return -SKTRIG_QTY;
	}

	if ( ! m_pChar->CanUse( pItem, true ))
	{
		SysMessageDefault( DEFMSG_TASTEID_UNABLE );
		return -SKTRIG_QTY;
	}

	uint iPoisonLevel = 0;
	switch ( pItem->GetType())
	{
		case IT_POTION:
			if ( ResGetIndex(pItem->m_itPotion.m_Type) == SPELL_Poison )
			{
				iPoisonLevel = pItem->m_itPotion.m_dwSkillQuality;
			}
			break;
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
			iPoisonLevel = pItem->m_itFood.m_poison_skill * 10;
			break;
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_SWORD:		// 13 =
		case IT_WEAPON_FENCE:		// 14 = can't be used to chop trees. (make kindling)
		case IT_WEAPON_AXE:
			// pItem->m_itWeapon.m_poison_skill = pPoison->m_itPotion.m_SkillQuality / 10;
			iPoisonLevel = pItem->m_itWeapon.m_poison_skill * 10;
			break;
		default:
			if ( ! fTest )
			{
				SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_TASTEID_RESULT ), static_cast<lpctstr>(pItem->GetNameFull(false)));
			}
			return 1;
	}

	if ( fTest )
		return g_Rand.GetVal(60);


	static const lpctstr sm_szPoisonMessages[] =
	{
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_1),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_2),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_3),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_4),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_5),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_6),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_7),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_8),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_9),
			g_Cfg.GetDefaultMsg(DEFMSG_ARMSLORE_PSN_10)
	};

	if ( iPoisonLevel )
	{
		uint iLevel = (uint)IMulDiv( iPoisonLevel, ARRAY_COUNT(sm_szPoisonMessages), 1000 );
		if ( iLevel >= ARRAY_COUNT(sm_szPoisonMessages))
			iLevel = usize_narrow_u32(ARRAY_COUNT(sm_szPoisonMessages) - 1);
		SysMessage(sm_szPoisonMessages[iLevel] );
	}
	else
		SysMessagef( g_Cfg.GetDefaultMsg( DEFMSG_TASTEID_RESULT ), static_cast<lpctstr>(pItem->GetNameFull(false)));

	return iSkillLevel;
}

int CClient::OnSkill_Info( SKILL_TYPE skill, CUID uid, int iSkillLevel, bool fTest )
{
	ADDTOCALLSTACK("CClient::OnSkill_Info");
	// Skill timer has expired.
	// RETURN: difficulty credit. 0-100
	//  <0 = immediate failure.
	switch ( skill )
	{
		case SKILL_ANIMALLORE:	return OnSkill_AnimalLore( uid, iSkillLevel, fTest );
		case SKILL_ARMSLORE:	return OnSkill_ArmsLore( uid, iSkillLevel, fTest );
		case SKILL_ANATOMY:		return OnSkill_Anatomy( uid, iSkillLevel, fTest );
		case SKILL_ITEMID:		return OnSkill_ItemID( uid, iSkillLevel, fTest );
		case SKILL_EVALINT:		return OnSkill_EvalInt( uid, iSkillLevel, fTest );
		case SKILL_FORENSICS:	return OnSkill_Forensics( uid, iSkillLevel, fTest );
		case SKILL_TASTEID:		return OnSkill_TasteID( uid, iSkillLevel, fTest );
		default:				return -SKTRIG_QTY;
	}
}

////////////////////////////////////////
// Targeted skills and actions.

bool CClient::OnTarg_Skill( CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill");
	// targetted skill now has it's target.
	// response to CLIMODE_TARG_SKILL
	// from Event_Skill_Use() select button from skill window

	if ( pObj == nullptr )
		return false;

	SetTargMode();	// just make sure last targ mode is gone.
	m_Targ_UID = pObj->GetUID();	// keep for 'last target' info.

	// did we target a scripted skill ?
	if ( g_Cfg.IsSkillFlag( m_tmSkillTarg.m_iSkill, SKF_SCRIPTED ) )
	{
		// is this scripted skill a targeted skill ?
		const CSkillDef * pSkillDef = g_Cfg.GetSkillDef(m_tmSkillTarg.m_iSkill);
		if (pSkillDef && pSkillDef->m_sTargetPrompt.IsEmpty() == false)
		{
			m_pChar->m_Act_UID = m_Targ_UID;
			return m_pChar->Skill_Start( m_tmSkillTarg.m_iSkill );
		}
		else
		{
			return true;
		}
	}

	// targetting what skill ?
	switch ( m_tmSkillTarg.m_iSkill )
	{
		// Delayed response type skills.
	case SKILL_BEGGING:
	case SKILL_STEALING:
	case SKILL_TAMING:
	case SKILL_ENTICEMENT:
	case SKILL_STEALTH:
	case SKILL_REMOVETRAP:

		// Informational skills. (instant return)
	case SKILL_ANIMALLORE:
	case SKILL_ARMSLORE:
	case SKILL_ANATOMY:
	case SKILL_ITEMID:
	case SKILL_EVALINT:
	case SKILL_FORENSICS:
	case SKILL_TASTEID:
		m_pChar->m_Act_UID = m_Targ_UID;
		return( m_pChar->Skill_Start( m_tmSkillTarg.m_iSkill ));

	case SKILL_PROVOCATION:
		if ( !pObj->IsChar() )
		{
			SysMessageDefault(DEFMSG_PROVOKE_UNABLE);
			return false;
		}
		addTarget(CLIMODE_TARG_SKILL_PROVOKE, g_Cfg.GetDefaultMsg(DEFMSG_PROVOKE_SELECT), false, true);
		break;

	case SKILL_POISONING:
		// We now have to find the poison.
		addTarget( CLIMODE_TARG_SKILL_POISON, g_Cfg.GetDefaultMsg( DEFMSG_POISONING_SELECT_1 ), false, true );
		break;

	default:
		// This is not a targetted skill !
		break;
	}

	return true;
}

bool CClient::OnTarg_Skill_Provoke( CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Provoke");
	// CLIMODE_TARG_SKILL_PROVOKE
	if ( !pObj || !pObj->IsChar() )
	{
		SysMessageDefault(DEFMSG_PROVOKE_UNABLE);
		return false;
	}
	m_pChar->m_Act_Prv_UID = m_Targ_UID;	// provoke him
	m_pChar->m_Act_UID = pObj->GetUID();	// against him
	return m_pChar->Skill_Start(SKILL_PROVOCATION);
}

bool CClient::OnTarg_Skill_Poison( CObjBase * pObj )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Poison");
	// CLIMODE_TARG_SKILL_POISON
	if ( pObj == nullptr )
		return false;
	m_pChar->m_Act_Prv_UID = m_Targ_UID;	// poison this
	m_pChar->m_Act_UID = pObj->GetUID();				// with this poison
	return m_pChar->Skill_Start( SKILL_POISONING );
}

bool CClient::OnTarg_Skill_Herd_Dest( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Herd_Dest");
	UnreferencedParameter(pObj);
	// CLIMODE_TARG_SKILL_HERD_DEST

	m_pChar->m_Act_p = pt;
	m_pChar->m_Act_UID = m_Targ_UID;	// Who to herd?
	m_pChar->m_Act_Prv_UID = m_Targ_Prv_UID; // crook ?

	return m_pChar->Skill_Start( SKILL_HERDING );
}

bool CClient::OnTarg_Skill_Magery( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Skill_Magery");
	// The client player has targeted a spell.
	// CLIMODE_TARG_SKILL_MAGERY

    const CChar *pTargChar = dynamic_cast<CChar*>(pObj);
    if (pTargChar && pTargChar->Can(CAN_C_NONSELECTABLE))
        return false;

	const CSpellDef * pSpell = g_Cfg.GetSpellDef( m_tmSkillMagery.m_iSpell );
	if ( ! pSpell )
		return false;

	if ( pObj )
	{
		if ( !pSpell->IsSpellType( SPELLFLAG_TARG_OBJ ) )
		{
			SysMessageDefault( DEFMSG_MAGERY_4 );
			return true;
		}

		if ( pObj->IsItem() && !pSpell->IsSpellType( SPELLFLAG_TARG_ITEM ) )
		{
			SysMessageDefault( DEFMSG_MAGERY_1 );
			return true;
		}

		if ( pObj->IsChar() )
		{
			if (!pSpell->IsSpellType( SPELLFLAG_TARG_CHAR ) )
			{
				SysMessageDefault( DEFMSG_MAGERY_2 );
				return true;
			}
			CChar * pChar = static_cast<CChar*>(pObj);
			if ( pSpell->IsSpellType( SPELLFLAG_TARG_NO_PLAYER ) && pChar->m_pPlayer )
			{
				SysMessageDefault( DEFMSG_MAGERY_7 );
				return true;
			}
			if ( pSpell->IsSpellType( SPELLFLAG_TARG_NO_NPC ) && !pChar->m_pPlayer )
			{
				SysMessageDefault( DEFMSG_MAGERY_8 );
				return true;
			}
		}

		if (pObj == m_pChar && pSpell->IsSpellType( SPELLFLAG_TARG_NOSELF ) && !IsPriv(PRIV_GM) )
		{
			SysMessageDefault( DEFMSG_MAGERY_3 );
			return true;
		}
	}

	m_pChar->m_atMagery.m_iSpell			= m_tmSkillMagery.m_iSpell;
	m_pChar->m_atMagery.m_uiSummonID		= m_tmSkillMagery.m_uiSummonID;

	m_pChar->m_Act_Prv_UID				= m_Targ_Prv_UID;	// Source (wand or you?)
	m_pChar->m_Act_UID					= pObj ? pObj->GetUID() : CUID(UID_PLAIN_CLEAR);
	m_pChar->m_Act_p					= pt;
	m_Targ_p							= pt;

	if ( IsSetMagicFlags( MAGICF_PRECAST ) && !pSpell->IsSpellType( SPELLFLAG_NOPRECAST ) && m_pChar->IsClientActive() )
	{
		if ( g_Cfg.IsSkillFlag(m_pChar->m_Act_SkillCurrent, SKF_MAGIC) )
		{
			this->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_MAGERY_5 ) ); // You have not yet finished preparing the spell.
			return false;
		}

		return m_pChar->Spell_CastDone();
	}

	int skill;
	if (!pSpell->GetPrimarySkill(&skill, nullptr))
		return false;

	return m_pChar->Skill_Start((SKILL_TYPE)skill);
}

bool CClient::OnTarg_Pet_Command( CObjBase * pObj, const CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Pet_Command");
	// CLIMODE_TARG_PET_CMD
	// Any pet command requiring a target.
	// m_Targ_UID = the pet i was instructing.
	// m_tmPetCmd = command index.
	// m_Targ_Text = the args to the command that got us here.

	if ( m_tmPetCmd.m_fAllPets )
	{
		// All the pets that could hear me.
		bool fGhostSpeak = m_pChar->IsSpeakAsGhost();

		auto AreaChars = CWorldSearchHolder::GetInstance( m_pChar->GetTopPoint(), UO_MAP_VIEW_SIGHT );
		for (;;)
		{
			CChar * pCharPet = AreaChars->GetChar();
			if ( pCharPet == nullptr )
				break;
			if ( pCharPet == m_pChar )
				continue;
			if ( !pCharPet->m_pNPC )
				continue;
			if ( fGhostSpeak && !pCharPet->CanUnderstandGhost())
				continue;
			if ( !pCharPet->NPC_IsOwnedBy( GetChar(), true ))
				continue;
			pCharPet->NPC_OnHearPetCmdTarg( m_tmPetCmd.m_iCmd, GetChar(), pObj, pt, m_Targ_Text );
		}
		return true;
	}
	else
	{
		CChar * pCharPet = m_Targ_UID.CharFind();
		if ( pCharPet == nullptr )
			return false;
		return pCharPet->NPC_OnHearPetCmdTarg( m_tmPetCmd.m_iCmd, GetChar(), pObj, pt, m_Targ_Text );
	}
}

bool CClient::OnTarg_Pet_Stable( CChar * pCharPet )
{
	ADDTOCALLSTACK("CClient::OnTarg_Pet_Stable");
	// CLIMODE_PET_STABLE
	// NOTE: You are only allowed to stable x creatures here.
	// m_Targ_Prv_UID = stable master.

	CChar * pCharMaster = m_Targ_Prv_UID.CharFind();
	if ( !pCharMaster || !pCharMaster->m_pNPC )
		return false;

	if ( (pCharPet == nullptr) || pCharPet->m_pPlayer || (pCharMaster == pCharPet) )
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TARG_FAIL ) );
		return false;
	}

	if ( ! pCharMaster->CanSeeLOS( pCharPet ))
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TARG_LOS ) );
		return false;
	}

	if ( ! pCharPet->NPC_IsOwnedBy( m_pChar ))
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TARG_OWNER ) );
		return false;
	}

	if ( pCharPet->IsStatFlag(STATF_CONJURED))
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TARG_SUMMON ) );
		return false;
	}

	CItemContainer * pPack = pCharPet->GetPack();
	if ( pPack )
	{
		if ( ! pPack->IsContainerEmpty() )
		{
			pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TARG_UNLOAD ) );
			return false;
		}
	}

	// Shrink the pet and put it in the bank box of the stable master.
	CItem * pPetItem = pCharPet->Make_Figurine( m_pChar->GetUID());
	if ( pPetItem == nullptr )
	{
		pCharMaster->Speak( g_Cfg.GetDefaultMsg( DEFMSG_NPC_STABLEMASTER_TARG_FAIL ) );
		return false;
	}

	if ( IsSetOF(OF_PetSlots) )
	{
        short iFollowerSlots = pCharPet->GetFollowerSlots();
		m_pChar->FollowersUpdate(pCharPet,(-maximum(0, iFollowerSlots)));
	}

	m_pChar->GetBank(LAYER_STABLE)->ContentAdd(pPetItem);
	pCharMaster->Speak(g_Cfg.GetDefaultMsg(DEFMSG_NPC_STABLEMASTER_CLAIM));
	return true;
}

//-----------------------------------------------------------------------
// Targetted items with special props.

bool CClient::OnTarg_Use_Deed( CItem * pDeed, CPointMap & pt )
{
	ADDTOCALLSTACK("CClient::OnTarg_Use_Deed");
	// Place the house/ship here. (if possible)
	// Can the structure go here ?
	// IT_DEED
	//

    if (!GetChar()->CanUse(pDeed, true))
    {
        return false;
    }

	const CItemBase * pItemDef = CItemBase::FindItemBase((ITEMID_TYPE)(ResGetIndex(pDeed->m_itDeed.m_Type)));
    if (!OnTarg_Use_Multi(pItemDef, pt, pDeed))
    {
        return false;
    }

	pDeed->ConsumeAmount(1);	// consume the deed.
	return true;
}

CItem * CClient::OnTarg_Use_Multi(const CItemBase * pItemDef, CPointMap & pt, CItem *pDeed)
{
    ADDTOCALLSTACK("CClient::OnTarg_Use_Multi");
    // Might be a IT_MULTI or it might not. place it anyhow.

    if ((pItemDef == nullptr) || !pt.GetRegion(REGION_TYPE_AREA))
        return nullptr;

    return CItemMulti::Multi_Create(GetChar(), pItemDef, pt, pDeed);
}

bool CClient::OnTarg_Use_Item( CObjBase * pObjTarg, CPointMap & pt, ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CClient::OnTarg_Use_Item");
	// CLIMODE_TARG_USE_ITEM started from Event_DoubleClick()
	// Targetted an item to be used on some other object (char or item).
	// Solve for the intersection of the items.
	// ARGS:
	//  id = static item id
	//  uid = the target.
	//  m_Targ_UID = what is the used object on the target
	//
	// NOTE:
  	//  Assume we can see the target.
	//
	// RETURN:
	//  true = success.

	CItem * pItemUse = m_Targ_UID.ItemFind();
	if ( pItemUse == nullptr )
	{
		SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_GONE) );
		return false;
	}
	if ( pItemUse->GetParent() != m_tmUseItem.m_pParent )
	{
		// Watch out for cheating.
		// Is the source item still in the same place as it was.
		SysMessage(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TARG_MOVED));
		return false;
	}

	m_Targ_Prv_UID = m_Targ_UID;	// used item.
	m_Targ_p = pt;

	ITRIG_TYPE trigtype;
    if ( pObjTarg )
    {
        trigtype = pObjTarg->IsChar() ? ITRIG_TARGON_CHAR : ITRIG_TARGON_ITEM;
        m_Targ_UID = pObjTarg->GetUID();
        if (!IsSetOF(OF_NoTargTurn))
            m_pChar->UpdateDir(pObjTarg);
    }
    else
    {
        trigtype = ITRIG_TARGON_GROUND;
        m_Targ_UID.InitUID();
        if ( !IsSetOF(OF_NoTargTurn) && pt.IsValidPoint() )
            m_pChar->UpdateDir(pt);
    }

	if (( IsTrigUsed(CItem::sm_szTrigName[trigtype]) ) || ( IsTrigUsed(CChar::sm_szTrigName[(CTRIG_itemAfterClick - 1) + trigtype]) )) //ITRIG_TARGON_GROUND, ITRIG_TARGON_CHAR, ITRIG_TARGON_ITEM
	{
		CScriptTriggerArgs	Args( id, 0, pObjTarg );
		if ( pItemUse->OnTrigger( trigtype, m_pChar, &Args ) == TRIGRET_RET_TRUE )
			return true;
	}

	// NOTE: We have NOT checked to see if the targetted object is within reasonable distance.
	// Call CanUse( pItemTarg )

	// What did i target it on ? this could be null if ground is the target.
	CChar * pCharTarg = dynamic_cast <CChar*>(pObjTarg);
	CItem * pItemTarg = dynamic_cast <CItem*>(pObjTarg);

	switch ( pItemUse->GetType() )
	{
	case IT_COMM_CRYSTAL:
		if ( !pItemTarg || !pItemTarg->IsType(IT_COMM_CRYSTAL) )
			return false;
		pItemUse->m_uidLink = pItemTarg->GetUID();
		pItemUse->Speak("Linked");
		pItemUse->UpdatePropertyFlag();
		return true;

	case IT_POTION:
		// Use a potion on something else.
		if ( ResGetIndex(pItemUse->m_itPotion.m_Type) == SPELL_Explosion )
		{
			// Throw explosion potion
			if ( !pItemUse->IsItemEquipped() || pItemUse->GetEquipLayer() != LAYER_DRAGGING )
				return false;

			// Check if we have clear LOS to target location and also if it's not over a wall (to prevent hit chars on the other side of the wall)
			if ( m_pChar->CanSeeLOS(pt, nullptr, UO_MAP_VIEW_SIGHT, LOS_NB_WINDOWS) && !CWorldMap::IsItemTypeNear(pt, IT_WALL, 0, true) )
			{
				pItemUse->SetAttr(ATTR_MOVE_NEVER);
				pItemUse->MoveToUpdate(pt);
				pItemUse->Effect(EFFECT_BOLT, pItemUse->GetDispID(), m_pChar, 7, 0, false, maximum(0, pItemUse->GetHue()-1));
			}
		}
		return true;

	case IT_KEY:
		return( m_pChar->Use_Key( pItemUse, pItemTarg ));

	case IT_FORGE:
		// target the ore.
		return( m_pChar->Skill_Mining_Smelt( pItemTarg, pItemUse ));

	case IT_CANNON_MUZZLE:
		// We have targetted the cannon to something.
		if ( ( pItemUse->m_itCannon.m_Load & 3 ) != 3 )
		{
			if ( m_pChar->Use_Cannon_Feed( pItemUse, pItemTarg ))
			{
				pItemTarg->ConsumeAmount();
			}
			return true;
		}

		// Fire!
		pItemUse->m_itCannon.m_Load = 0;
		pItemUse->Sound( 0x207 );
		pItemUse->Effect( EFFECT_OBJ, ITEMID_FX_TELE_VANISH, pItemUse, 9, 6 );

		// just explode on the ground ?
		if ( pObjTarg != nullptr )
		{
			// Check distance and LOS.
			if ( pItemUse->GetDist( pObjTarg ) > UO_MAP_VIEW_SIGHT )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_TOOFAR );
				return true;
			}

			// Hit the Target !
			pObjTarg->Sound( 0x207 );
			pObjTarg->Effect( EFFECT_BOLT, ITEMID_Cannon_Ball, pItemUse, 8, 0, true );

			CChar *pChar = dynamic_cast<CChar *>(this);
			CItem *pItem = dynamic_cast<CItem *>(this);
			if ( pChar )
				pChar->OnTakeDamage( 80 + g_Rand.GetVal(150), m_pChar, DAMAGE_HIT_BLUNT|DAMAGE_FIRE );
			else if ( pItem )
				pItem->OnTakeDamage( 80 + g_Rand.GetVal(150), m_pChar, DAMAGE_HIT_BLUNT|DAMAGE_FIRE );
		}
		return true;

	case IT_WEAPON_MACE_PICK:
		// Mine at the location. (shovel)
		m_pChar->m_Act_p = pt;
		m_pChar->m_Act_Prv_UID = m_Targ_Prv_UID;
		m_pChar->m_atResource.m_ridType = CResourceIDBase(RES_TYPEDEF, IT_ROCK);
		return( m_pChar->Skill_Start( SKILL_MINING ));

	case IT_WEAPON_MACE_CROOK:
		// SKILL_HERDING
		// Selected a creature to herd
		// Move the selected item or char to this location.
		if ( pCharTarg == nullptr )
		{
			// hit it ?
			SysMessageDefault( DEFMSG_ITEMUSE_CROOK_TRY );
			return false;
		}
		addTarget(CLIMODE_TARG_SKILL_HERD_DEST, g_Cfg.GetDefaultMsg(DEFMSG_ITEMUSE_CROOK_TARGET), true);
		return true;

	case IT_WEAPON_MACE_SMITH:		// Can be used for smithing.
		// Near a forge ? smithing ?
		if ( pItemTarg == nullptr )
			break;
		if ( pItemTarg->IsType(IT_INGOT) )
		{
			return Cmd_Skill_Smith( pItemTarg );
		}
		else if ( pItemTarg->Armor_IsRepairable())
		{
			// Near an anvil ? repair ?
			m_pChar->Use_Repair( pItemTarg );
			return true;
		}
		break;

	case IT_CARPENTRY_CHOP:		// Carpentry type tool
	case IT_WEAPON_MACE_SHARP:// 22 = war axe can be used to cut/chop trees.
	case IT_WEAPON_FENCE:		// 24 = can't be used to chop trees.
	case IT_WEAPON_AXE:
	case IT_WEAPON_SWORD:		// 23 =

		// Use sharp weapon on something.
		if ( pCharTarg != nullptr )
		{
			// on some person ?
			if ( ! m_pChar->CanTouch(pCharTarg) )
				return false;
			switch ( pCharTarg->GetID())
			{
				case CREID_SHEEP: // Sheep have wool.
					// Get the wool.
					{
						CItem *pWool = CItem::CreateBase( ITEMID_WOOL );
						ASSERT(pWool);
						m_pChar->ItemBounce(pWool);
						pCharTarg->SetID(CREID_SHEEP_SHORN);
						// Set wool to regrow.
						pWool = CItem::CreateBase( ITEMID_WOOL );
						ASSERT(pWool);
						pWool->SetTimeout( g_Cfg.m_iWoolGrowthTime );
						pCharTarg->LayerAdd( pWool, LAYER_FLAG_Wool );
					}
					return true;
				case CREID_SHEEP_SHORN:
					SysMessageDefault( DEFMSG_ITEMUSE_WEAPON_WWAIT );
					return true;
				default:
					// I suppose this is an attack ?
					break;
			}
			break;
		}


		switch ( m_pChar->CanTouchStatic( &pt, id, pItemTarg ))
		{
		case IT_JUNK:
			SysMessageDefault( DEFMSG_ITEMUSE_JUNK_REACH );
			return false;

		case IT_FOLIAGE:
		case IT_TREE:
			// Just targetted a tree type
			m_pChar->m_Act_Prv_UID = m_Targ_Prv_UID;
			m_pChar->m_Act_UID = m_Targ_UID;
			m_pChar->m_Act_p = pt;
			m_pChar->m_atResource.m_ridType = CResourceIDBase(RES_TYPEDEF, IT_TREE);
			return( m_pChar->Skill_Start( SKILL_LUMBERJACKING ));

		case IT_LOG:
			if ( ! m_pChar->CanUse( pItemTarg, true ))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_LOG_UNABLE );
				return false;
			}
			if ( pItemUse->IsType(IT_CARPENTRY_CHOP) )
			{
				return Skill_Menu(SKILL_CARPENTRY, "sm_carpentry", pItemUse->GetID());
			}
			if ( pItemUse->IsSameDispID( ITEMID_DAGGER ))
			{
				// set the target item
                if (pItemTarg)
                    m_Targ_UID = pItemTarg->GetUID();
                else
                    m_Targ_UID.InitUID();

				return Skill_Menu(SKILL_BOWCRAFT, "sm_bowcraft", pItemUse->GetID());
			}
			SysMessageDefault( DEFMSG_ITEMUSE_LOG_USE );
			return false;

		case IT_FISH:
			if ( !pItemTarg || !m_pChar->CanUse(pItemTarg, true) )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_FISH_UNABLE );
				return false;
			}

			// Carve up Fish parts.
			pItemTarg->SetID( ITEMID_FOOD_FISH_RAW );
			pItemTarg->SetHue( HUE_DEFAULT );
			pItemTarg->SetAmount( 4 * pItemTarg->GetAmount());
			pItemTarg->Update();
			return true;

		case IT_CORPSE:
			if ( ! m_pChar->CanUse( pItemTarg, false ))
				return false;
			m_pChar->Use_CarveCorpse( dynamic_cast <CItemCorpse*>( pItemTarg ), pItemUse);
			return true;

		case IT_FRUIT:
		case IT_REAGENT_RAW:
			// turn the fruit into a seed.
			if ( !pItemTarg || !m_pChar->CanUse( pItemTarg, true ))
				return false;
			{
				CResourceID defaultseed = g_Cfg.ResourceGetIDType( RES_ITEMDEF, "DEFAULTSEED" );
				pItemTarg->SetDispID((ITEMID_TYPE)(defaultseed.GetResIndex()));
				pItemTarg->SetType(IT_SEED);
				tchar *pszTemp = Str_GetTemp();
				snprintf(pszTemp, Str_TempLength(), "%s seed", pItemTarg->GetName());
				pItemTarg->SetName(pszTemp);
				pItemTarg->Update();
			}
			return true;

		// case IT_FOLIAGE: // trim back ?
		case IT_CROPS:
			if ( pItemTarg )
				pItemTarg->Plant_CropReset();
			return true;

		default:
			// Item to smash ? furniture ???

			if ( ! m_pChar->CanMoveItem(pItemTarg) )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_WEAPON_IMMUNE );
				return false;	// ? using does not imply moving in all cases ! such as reading ?
			}

			// Is breaking this a crime ?
			if ( m_pChar->IsTakeCrime( pItemTarg ))
			{
				SysMessageDefault( DEFMSG_ITEMUSE_STEAL );
				return false;
			}

			if ( pItemTarg )
				pItemTarg->OnTakeDamage( 1, m_pChar, DAMAGE_HIT_BLUNT );
			return true;
		}
		break;

	case IT_BANDAGE:	// SKILL_HEALING, or SKILL_VETERINARY
		// Use bandages on some creatures or on a corpse item.
		if ( pCharTarg == nullptr && pItemTarg == nullptr )
			return false;

		if (pItemTarg)
		{
			if (pItemTarg->GetType() == IT_CORPSE)
			{
				CItemCorpse* pCorpse = static_cast<CItemCorpse*>(pItemTarg);
				pCharTarg = pCorpse->m_uidLink.CharFind();
				if (pCharTarg == nullptr || pCharTarg->IsNPC())
					return false;
			}
			else
			{
				return false;
			}
		}
		m_pChar->m_Act_Prv_UID = m_Targ_Prv_UID; //The bandage item
		m_pChar->m_Act_UID = m_Targ_UID;  //The target.

		/*An NPC will be healed by the Veterinary skill if the following conditions are satisfied:
		  It has a Taming value above > 0 AND its ID is not the ID one of the playable races (Human, Elf or Gargoyle)
		*/
		if (pCharTarg->m_pNPC && pCharTarg->Skill_GetBase(SKILL_TAMING) > 0 &&
			!pCharTarg->IsPlayableCharacter() )
		{
			switch (pCharTarg->GetNPCBrain())
			{
			case NPCBRAIN_ANIMAL:
			case NPCBRAIN_DRAGON:
			case NPCBRAIN_MONSTER:
				return m_pChar->Skill_Start(SKILL_VETERINARY);
			default:
				return m_pChar->Skill_Start(SKILL_HEALING);
			}
		}
		return m_pChar->Skill_Start(SKILL_HEALING);

	case IT_SEED:
		return m_pChar->Use_Seed( pItemUse, &pt );

	case IT_DEED:
		return OnTarg_Use_Deed( pItemUse, pt );

	case IT_WOOL:
	case IT_COTTON:
		// Use on a spinning wheel.
		if ( pItemTarg == nullptr )
			break;
		if ( ! pItemTarg->IsType(IT_SPINWHEEL))
			break;
		if ( ! m_pChar->CanUse( pItemTarg, false ))
			return false;

		pItemTarg->SetAnim((ITEMID_TYPE)( pItemTarg->GetID() + 1 ), 2 * MSECS_PER_SEC);
		pItemUse->ConsumeAmount( 1 );

		{
			CItem * pNewItem;
			if ( pItemUse->IsType(IT_WOOL))
			{
				// 1 pile of wool yields three balls of yarn
				SysMessageDefault( DEFMSG_ITEMUSE_WOOL_CREATE );
				pNewItem = CItem::CreateScript( ITEMID_YARN1, m_pChar );
				if ( pNewItem->GetAmount() == 1 )
					pNewItem->SetAmountUpdate( 3 );
			}
			else
			{
				// 1 pile of cotton yields six spools of thread
				SysMessageDefault( DEFMSG_ITEMUSE_COTTON_CREATE );
				pNewItem = CItem::CreateScript( ITEMID_THREAD1, m_pChar );
				if ( pNewItem->GetAmount() == 1 )
					pNewItem->SetAmountUpdate( 6 );
			}
			m_pChar->ItemBounce( pNewItem );
		}
		return true;

	case IT_KEYRING:
		// it acts as a key.
		{
		if ( pItemTarg == nullptr )
			return false;
		CItemContainer* pKeyRing = dynamic_cast <CItemContainer*>(pItemUse);
		if ( pKeyRing == nullptr )
			return false;

		if ( pItemTarg == pItemUse )
		{
			// Use a keyring on self = remove all keys.
			pKeyRing->ContentsTransfer( m_pChar->GetPack(), false );
			return true;
		}

		CItem * pKey = nullptr;
		bool fLockable = pItemTarg->IsTypeLockable();

		if ( fLockable && pItemTarg->m_itContainer.m_UIDLock.IsValidUID())
		{
			// try all the keys on the object.
			pKey = pKeyRing->ContentFind( CResourceID(RES_TYPEDEF,IT_KEY), pItemTarg->m_itContainer.m_UIDLock );
		}
		if ( pKey == nullptr )
		{
			// are we trying to lock it down ?
			if ( m_pChar->m_pArea->GetResourceID().IsUIDItem())
			{
				pKey = pKeyRing->ContentFind( CResourceID(RES_TYPEDEF,IT_KEY), m_pChar->m_pArea->GetResourceID() );
				if ( pKey )
				{
					if ( m_pChar->Use_MultiLockDown( pItemTarg ))
						return true;
				}
			}

			if ( ! fLockable || ! pItemTarg->m_itContainer.m_UIDLock.IsValidUID())
				SysMessageDefault( DEFMSG_ITEMUSE_KEY_NOLOCK );
			else
				SysMessageDefault( DEFMSG_ITEMUSE_KEY_NOKEY );
			return false;
		}

		return( m_pChar->Use_Key( pKey, pItemTarg ));
		}

	case IT_SCISSORS:
		if ( pItemTarg != nullptr )
		{
			if ( ! m_pChar->CanUse( pItemTarg, true ))
				return false;

			// Cut up cloth.
			word iOutQty = 0;
			ITEMID_TYPE iOutID = ITEMID_BANDAGES1;

			switch ( pItemTarg->GetType())
			{
				case IT_CLOTH_BOLT:
					// Just make cut cloth here !
					pItemTarg->ConvertBolttoCloth();
					m_pChar->Sound( SOUND_SNIP );	// snip noise.
					return true;
				case IT_CLOTH:
					iOutQty = pItemTarg->GetAmount();
					break;
				case IT_CLOTHING:
					// Cut up for bandages.
					iOutQty = (word)(pItemTarg->GetWeight()/WEIGHT_UNITS);
					break;
				case IT_HIDE:
					// IT_LEATHER
					// Cut up the hides and create strips of leather
					iOutID = (ITEMID_TYPE)(ResGetIndex(pItemTarg->Item_GetDef()->m_ttNormal.m_tData1));
					if ( ! iOutID )
						iOutID = ITEMID_LEATHER_1;
					iOutQty = pItemTarg->GetAmount();
					break;
				default:
					break;
			}
			if ( iOutQty )
			{
				CItem * pItemNew = CItem::CreateBase( iOutID );
				ASSERT(pItemNew);
				HUE_TYPE hue = pItemTarg->GetHue();
				pItemTarg->Delete();
				pItemNew->SetHue( hue ) ;
				pItemNew->SetAmount( iOutQty );
				m_pChar->ItemBounce( pItemNew );
				m_pChar->Sound( SOUND_SNIP );	// snip noise.
				return true;
			}
		}
		SysMessageDefault( DEFMSG_ITEMUSE_SCISSORS_USE );
		return false;

	case IT_YARN:
	case IT_THREAD:
		// Use this on a loom.
		// use on a spinning wheel.
		if ( pItemTarg == nullptr )
			break;
		if ( ! pItemTarg->IsType( IT_LOOM ))
			break;
		if ( ! m_pChar->CanUse( pItemTarg, false ))
			return false;

		{
static lpctstr const sm_Txt_LoomUse[] =
{
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_1 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_2 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_3 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_4 ),
	g_Cfg.GetDefaultMsg( DEFMSG_ITEMUSE_BOLT_5 )
};

		// pItemTarg->SetAnim((ITEMID_TYPE)(pItemTarg->GetID() + 1), 2 * MSECS_PER_SEC );

		// Use more1 to record the type of resource last used on this object
		// Use more2 to record the number of resources used so far
		// Check what was used last.
		ITEMID_TYPE ClothID = (ITEMID_TYPE)pItemTarg->m_itLoom.m_ridCloth.GetResIndex();
		if ( ClothID && (ClothID != pItemUse->GetDispID()) )
		{
			// throw away what was on here before
			SysMessageDefault( DEFMSG_ITEMUSE_LOOM_REMOVE );
			CItem * pItemCloth = CItem::CreateTemplate( ClothID, nullptr, m_pChar );
            if (!pItemCloth)
                return false;
			pItemCloth->SetAmount( (word)pItemTarg->m_itLoom.m_iClothQty );
			pItemTarg->m_itLoom.m_iClothQty = 0;
			pItemTarg->m_itLoom.m_ridCloth.Clear();
			m_pChar->ItemBounce( pItemCloth );
			return true;
		}

		pItemTarg->m_itLoom.m_ridCloth = CResourceIDBase(RES_ITEMDEF, pItemUse->GetDispID());

		int iUsed = 0;
		int iNeed = int(ARRAY_COUNT( sm_Txt_LoomUse ) - 1);
		int iHave = pItemTarg->m_itLoom.m_iClothQty;
		if ( iHave < iNeed )
		{
			iNeed -= iHave;
			iUsed = pItemUse->ConsumeAmount( (word)iNeed );
		}

		if ( (iHave + iUsed) < int(ARRAY_COUNT( sm_Txt_LoomUse ) - 1) )
		{
			pItemTarg->m_itLoom.m_iClothQty += iUsed;
			SysMessage( sm_Txt_LoomUse[ pItemTarg->m_itLoom.m_iClothQty ] );
		}
		else
		{
			SysMessage( sm_Txt_LoomUse[ ARRAY_COUNT( sm_Txt_LoomUse ) - 1 ] );
			pItemTarg->m_itLoom.m_iClothQty = 0;
			pItemTarg->m_itLoom.m_ridCloth.Clear();

/*
			CItemBase * pItemDef = pItemTarg->Item_GetDef();

			if ( pItemDef->m_ttNormal.m_tData3 != 0 )
			{
				for ( int clothcount=1; clothcount < pItemDef->m_ttNormal.m_tData3; clothcount++)
				{
					m_pChar->ItemBounce( CItem::CreateScript(ITEMID_CLOTH_BOLT1, m_pChar ));
				}
			} else {
*/

				m_pChar->ItemBounce( CItem::CreateScript(ITEMID_CLOTH_BOLT1, m_pChar ));
/*			}
*/
		}
		}
		return true;

	case IT_BANDAGE_BLOOD:
		// Use these on water to clean them.
		switch ( m_pChar->CanTouchStatic( &pt, id, pItemTarg ))
		{
			case IT_WATER:
			case IT_WATER_WASH:
				// Make clean.
				pItemUse->SetID( ITEMID_BANDAGES1 );
				pItemUse->Update();
				return true;
			case IT_JUNK:
				SysMessageDefault( DEFMSG_ITEMUSE_BANDAGE_REACH );
				break;
			default:
				SysMessageDefault( DEFMSG_ITEMUSE_BANDAGE_CLEAN );
				break;
		}
		return false;

	case IT_FISH_POLE:
		m_pChar->m_Act_p = pt;
		m_pChar->m_atResource.m_ridType = CResourceIDBase(RES_TYPEDEF, IT_WATER);
		return( m_pChar->Skill_Start( SKILL_FISHING ));

	case IT_LOCKPICK:
		// Using a lock pick on something.
		if ( pItemTarg== nullptr )
			return false;
		m_pChar->m_Act_UID = m_Targ_UID;	// the locked item to be picked
		m_pChar->m_Act_Prv_UID = m_Targ_Prv_UID;	// the pick
		if ( ! m_pChar->CanUse( pItemTarg, false ))
			return false;
		return( m_pChar->Skill_Start( SKILL_LOCKPICKING ));

	case IT_CANNON_BALL:
		if ( m_pChar->Use_Cannon_Feed( pItemTarg, pItemUse ))
		{
			pItemUse->Delete();
			return true;
		}
		break;

	case IT_DYE:
		if (( pItemTarg != nullptr && pItemTarg->IsType(IT_DYE_VAT)) ||
			( pCharTarg != nullptr && ( pCharTarg == m_pChar || IsPriv( PRIV_GM ))))	// Change skin color.
		{
			addDyeOption( pObjTarg );
			return true;
		}
		SysMessageDefault( DEFMSG_ITEMUSE_DYE_FAIL );
		return false;

	case IT_DYE_VAT:
		{
		// Use the dye vat on some object.
		if ( pObjTarg == nullptr )
			return false;

		if ( pObjTarg->GetTopLevelObj() != m_pChar &&
			! IsPriv( PRIV_GM ))	// Change hair wHue.
		{
			SysMessageDefault( DEFMSG_ITEMUSE_DYE_REACH );
			return false;
		}
		if ( pCharTarg != nullptr )
		{
			// Dye hair.
			pObjTarg = pCharTarg->LayerFind( LAYER_HAIR );
			if ( pObjTarg != nullptr )
			{
				pObjTarg->SetHue( pItemUse->GetHue(), false, this->GetChar(), pItemUse, 0x23e);
				pObjTarg->Update();
			}
			pObjTarg = pCharTarg->LayerFind( LAYER_BEARD );
			if ( pObjTarg == nullptr )
				return true;
			// fall through
		}
		else
		{
			if ( ! m_pChar->CanUse( pItemTarg, false ))
				return false;
			if ( ! IsPriv( PRIV_GM ) &&
				! pItemTarg->Can( CAN_I_DYE ) &&
				! pItemTarg->IsType(IT_CLOTHING) )
			{
				SysMessageDefault( DEFMSG_ITEMUSE_DYE_FAIL );
				return false;
			}
		}

		pObjTarg->SetHue( pItemUse->GetHue(), false, this->GetChar(), pItemUse, 0x23e );
		pObjTarg->Update();
		return true;
		}
		break;

	case IT_PITCHER_EMPTY:
		// Fill it up with water.
		switch ( m_pChar->CanTouchStatic( &pt, id, pItemTarg ))
		{
			case IT_JUNK:
				SysMessageDefault( DEFMSG_ITEMUSE_PITCHER_REACH );
				return false;
			case IT_WATER:
			case IT_WATER_WASH:
				pItemUse->SetID( ITEMID_PITCHER_WATER );
				pItemUse->Update();
				return true;
			default:
				SysMessageDefault( DEFMSG_ITEMUSE_PITCHER_FILL );
				return false;
		}
		break;

	case IT_SEWING_KIT:
		// Use on cloth or hides
		if ( pItemTarg == nullptr)
			break;
		if ( ! m_pChar->CanUse( pItemTarg, true ))
			return false;

		switch ( pItemTarg->GetType())
		{
			case IT_LEATHER:
			case IT_HIDE:
				return Skill_Menu(SKILL_TAILORING, "sm_tailor_leather", pItemTarg->GetID());
			case IT_CLOTH:
				return Skill_Menu(SKILL_TAILORING, "sm_tailor_cloth", pItemTarg->GetID());
			default:
				break;
		}

		SysMessageDefault( DEFMSG_ITEMUSE_SKIT_UNABLE );
		return (false);

	default:
		break;
	}

	SysMessageDefault( DEFMSG_ITEMUSE_UNABLE );
	return false;
}

bool CClient::OnTarg_Stone_Recruit(CChar* pChar, bool bFull)
{
	ADDTOCALLSTACK("CClient::OnTarg_Stone_Recruit");
	// CLIMODE_TARG_STONE_RECRUIT / CLIMODE_TARG_STONE_RECRUITFULL
	CItemStone * pStone = dynamic_cast <CItemStone*> (m_Targ_UID.ItemFind());
	if ( !pStone )
		return false;
	return ( pStone->AddRecruit(pChar, STONEPRIV_CANDIDATE, bFull) != nullptr );
}

bool CClient::OnTarg_Party_Add( CChar * pChar )
{
	ADDTOCALLSTACK("CClient::OnTarg_Party_Add");
	// CLIMODE_TARG_PARTY_ADD
	// Invite this person to join our party. PARTYMSG_Add

	if ( pChar == nullptr )
	{
		SysMessageDefault( DEFMSG_PARTY_SELECT );
		return false;
	}

	if ( pChar == m_pChar )
	{
		SysMessageDefault( DEFMSG_PARTY_NO_SELF_ADD );
		return false;
	}

	if ( !pChar->IsClientActive() )
	{
		SysMessageDefault( DEFMSG_PARTY_NONPCADD );
		return false;
	}

	if ( m_pChar->m_pParty )
	{
		if ( !(m_pChar->m_pParty->IsPartyMaster(m_pChar)) )
		{
			SysMessageDefault( DEFMSG_PARTY_NOTLEADER );
			return false;
		}

		if ( m_pChar->m_pParty->IsPartyFull() )
		{
			SysMessageDefault( DEFMSG_PARTY_IS_FULL );
			return false;
		}
	}

	if (IsPriv(PRIV_GM) && (pChar->GetClientActive()->GetPrivLevel() < GetPrivLevel()))
	{
		CPartyDef::AcceptEvent(pChar, m_pChar->GetUID(), true);
		return true;
	}

	if ( pChar->m_pParty != nullptr )	// Already in a party !
	{
		if ( m_pChar->m_pParty == pChar->m_pParty )	// already in this party
		{
			SysMessageDefault( DEFMSG_PARTY_ALREADY_IN_THIS );
			return true;
		}

		SysMessageDefault( DEFMSG_PARTY_ALREADY_IN );
		return false;
	}

	if ( pChar->GetKeyNum("PARTY_AUTODECLINEINVITE") )
	{
		SysMessageDefault( DEFMSG_PARTY_AUTODECLINE );
		return false;
	}

	CVarDefCont * pTagInvitetime = m_pChar->m_TagDefs.GetKey("PARTY_LASTINVITETIME");
	if ( pTagInvitetime && (CWorldGameTime::GetCurrentTime().GetTimeDiff(pTagInvitetime->GetValNum()) <= 0) )
	{
		SysMessageDefault( DEFMSG_PARTY_ADD_TOO_FAST );
		return false;
	}

	if ( IsTrigUsed(TRIGGER_PARTYINVITE) )
	{
		CScriptTriggerArgs args;
		if ( pChar->OnTrigger(CTRIG_PartyInvite, m_pChar, &args) == TRIGRET_RET_TRUE )
			return false;
	}

	tchar * sTemp = Str_GetTemp();
	snprintf(sTemp, Str_TempLength(), g_Cfg.GetDefaultMsg( DEFMSG_PARTY_INVITE ), pChar->GetName());
	m_pChar->SysMessage( sTemp );
	sTemp = Str_GetTemp();
	snprintf(sTemp, Str_TempLength(), g_Cfg.GetDefaultMsg( DEFMSG_PARTY_INVITE_TARG ), m_pChar->GetName());
	pChar->SysMessage( sTemp );

	m_pChar->SetKeyNum("PARTY_LASTINVITE", (dword)(pChar->GetUID()));
	m_pChar->SetKeyNum("PARTY_LASTINVITETIME", CWorldGameTime::GetCurrentTime().GetTimeRaw() + (g_Rand.GetVal2(2,5) * MSECS_PER_SEC));

	new PacketPartyInvite(pChar->GetClientActive(), m_pChar);

	// Now up to them to decide to accept.
	return true;
}

bool CClient::OnTarg_GlobalChat_Add(CChar* pChar)
{
	ADDTOCALLSTACK("CClient::OnTarg_GlobalChat_Add");
	// CLIMODE_TARG_GLOBALCHAT_ADD
	// Invite this person to join our global chat friend list

	if (!CGlobalChatChanMember::IsVisible())
	{
		SysMessage("You must enable Global Chat to request a friend.");
		return false;
	}
	if (!pChar || !pChar->m_pPlayer || (pChar == m_pChar))
	{
		SysMessage("Invalid target.");
		return false;
	}
	if (!pChar->GetClientActive())
	{
		SysMessage("Player currently unavailable.");
		return false;
	}
	if (pChar->m_pPlayer->m_fRefuseGlobalChatRequests)		// TO-DO: also check if pChar is online on global chat -> CGlobalChat::IsVisible()
	{
		SysMessage("This user is not accepting Global Chat friend requests at this time.");
		return false;
	}
	/*if ( iFriendsCount >= 50 )		// TO-DO
	{
		SysMessage("You have reached your global chat friend limit.");
		return false;
	}*/

	if (IsPriv(PRIV_GM) && (pChar->GetClientActive()->GetPrivLevel() < GetPrivLevel()))
	{
		// TO-DO: auto-accept the request without send 'friend request' dialog
		return true;
	}

	CVarDefCont* pTagInviteTime = m_pChar->m_TagDefs.GetKey("GLOBALCHAT_LASTINVITETIME");
	if (pTagInviteTime && (CWorldGameTime::GetCurrentTime().GetTimeRaw() < pTagInviteTime->GetValNum()))
	{
		SysMessage("You are unable to add new friends at this time. Please try again in a moment.");
		return false;
	}
	m_pChar->SetKeyNum("GLOBALCHAT_LASTINVITETIME", CWorldGameTime::GetCurrentTime().GetTimeRaw() + (30 * TICKS_PER_SEC));

	// TO-DO: send 'friend request' dialog
	return true;
}
