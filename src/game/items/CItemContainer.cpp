
#include "../../common/CException.h"
#include "../../common/CUIDExtra.h"
#include "../../network/send.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"
#include "../CLog.h"
#include "../Triggers.h"
#include "CItem.h"
#include "CItemContainer.h"

//----------------------------------------------------
// -CItemContainer

CItemContainer::CItemContainer( ITEMID_TYPE id, CItemBase *pItemDef ) :	CItemVendable( id, pItemDef )
{
	// m_fTinkerTrapped = false;
}

bool CItemContainer::NotifyDelete()
{
	// notify destruction of the container before its contents
	if ( CItem::NotifyDelete() == false )
		return false;

	// ensure trade contents are moved out
	if ( IsType(IT_EQ_TRADE_WINDOW) )
		Trade_Delete();

	ContentNotifyDelete();
	return true;
}

void CItemContainer::DeletePrepare()
{
	if ( IsType( IT_EQ_TRADE_WINDOW ))
	{
		Trade_Delete();
	}
	CItem::DeletePrepare();
}

void CItemContainer::r_Write( CScript &s )
{
	ADDTOCALLSTACK_INTENSIVE("CItemContainer::r_Write");
	CItemVendable::r_Write(s);
	r_WriteContent(s);
}

bool CItemContainer::r_GetRef( lpctstr &pszKey, CScriptObj *&pRef )
{
	ADDTOCALLSTACK("CItemContainer::r_GetRef");
	if ( r_GetRefContainer(pszKey, pRef) )
		return true;

	return CItemVendable::r_GetRef(pszKey, pRef);
}

bool CItemContainer::r_WriteVal( lpctstr pszKey, CString &sVal, CTextConsole *pSrc )
{
	ADDTOCALLSTACK("CItemContainer::r_WriteVal");
	if ( r_WriteValContainer(pszKey, sVal, pSrc) )
		return true;
	return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
}

bool CItemContainer::IsItemInTrade()
{
	// recursively get the item that is at "top" level.
	CItemContainer *pObj = dynamic_cast<CItemContainer *>(this);
	//const CObjBase *pObj = pItem->GetContainer();
	if ( !pObj )
		return false;
	else if ( pObj->IsType(IT_EQ_TRADE_WINDOW) )
		return true;
	CItemContainer *pObj2 = dynamic_cast<CItemContainer *>(GetContainer());
	return pObj2->IsItemInTrade();
}

void CItemContainer::Trade_Status( bool bCheck )
{
	ADDTOCALLSTACK("CItemContainer::Trade_Status");
	// Update trade status check boxes to both sides.
	CItemContainer *pPartner = dynamic_cast<CItemContainer*>(m_uidLink.ItemFind());
	if ( !pPartner )
		return;

	CChar *pChar1 = dynamic_cast<CChar *>(GetParent());
	if ( !pChar1 || !pChar1->IsClient() )
		return;
	CChar *pChar2 = dynamic_cast<CChar *>(pPartner->GetParent());
	if ( !pChar2 || !pChar2->IsClient() )
		return;

	m_itEqTradeWindow.m_bCheck = bCheck ? 1 : 0;
	if ( !bCheck )
		pPartner->m_itEqTradeWindow.m_bCheck = 0;

	PacketTradeAction cmd(SECURE_TRADE_CHANGE);
	cmd.prepareReadyChange(this, pPartner);
	cmd.send(pChar1->GetClient());

	cmd.prepareReadyChange(pPartner, this);
	cmd.send(pChar2->GetClient());

	// Check if both clients had pressed the 'accept' buttom
	if ( pPartner->m_itEqTradeWindow.m_bCheck == 0 || m_itEqTradeWindow.m_bCheck == 0 )
		return;

	if ( IsTrigUsed(TRIGGER_TRADEACCEPTED) || IsTrigUsed(TRIGGER_CHARTRADEACCEPTED) )
	{
		CScriptTriggerArgs Args1(pChar1);
		ushort i = 1;
		for ( CItem *pItem = pPartner->GetContentHead(); pItem != NULL; pItem = pItem->GetNext(), i++ )
			Args1.m_VarObjs.Insert(i, pItem, true);
		Args1.m_iN1 = --i;

		CScriptTriggerArgs Args2(pChar2);
		i = 1;
		for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext(), i++ )
			Args2.m_VarObjs.Insert(i, pItem, true);
		Args2.m_iN2 = --i;

		Args1.m_iN2 = Args2.m_iN2;
		Args2.m_iN1 = Args1.m_iN1;
		if ( (pChar1->OnTrigger(CTRIG_TradeAccepted, pChar2, &Args1) == TRIGRET_RET_TRUE) || (pChar2->OnTrigger(CTRIG_TradeAccepted, pChar1, &Args2) == TRIGRET_RET_TRUE) )
			return;
	}

	// Transfer items
	CItem *pItemNext = NULL;
	for ( CItem *pItem = pPartner->GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pChar1->ItemBounce(pItem, false);
	}

	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pChar2->ItemBounce(pItem, false);
	}

	// Transfer gold/platinum
	if ( g_Cfg.m_iFeatureTOL & FEATURE_TOL_VIRTUALGOLD )
	{
		if ( m_itEqTradeWindow.m_iPlatinum && m_itEqTradeWindow.m_iGold )
		{
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_PLAT_GOLD), m_itEqTradeWindow.m_iPlatinum, m_itEqTradeWindow.m_iGold, pChar2->GetName());
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_PLAT_GOLD), m_itEqTradeWindow.m_iPlatinum, m_itEqTradeWindow.m_iGold, pChar1->GetName());
		}
		else if ( m_itEqTradeWindow.m_iPlatinum )
		{
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_PLAT), m_itEqTradeWindow.m_iPlatinum, pChar2->GetName());
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_PLAT), m_itEqTradeWindow.m_iPlatinum, pChar1->GetName());
		}
		else if ( m_itEqTradeWindow.m_iGold )
		{
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_GOLD), m_itEqTradeWindow.m_iGold, pChar2->GetName());
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_GOLD), m_itEqTradeWindow.m_iGold, pChar1->GetName());
		}

		if ( pPartner->m_itEqTradeWindow.m_iPlatinum && pPartner->m_itEqTradeWindow.m_iGold )
		{
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_PLAT_GOLD), pPartner->m_itEqTradeWindow.m_iPlatinum, pPartner->m_itEqTradeWindow.m_iGold, pChar1->GetName());
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_PLAT_GOLD), pPartner->m_itEqTradeWindow.m_iPlatinum, pPartner->m_itEqTradeWindow.m_iGold, pChar2->GetName());
		}
		else if ( pPartner->m_itEqTradeWindow.m_iPlatinum )
		{
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_PLAT), pPartner->m_itEqTradeWindow.m_iPlatinum, pChar1->GetName());
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_PLAT), pPartner->m_itEqTradeWindow.m_iPlatinum, pChar2->GetName());
		}
		else if ( pPartner->m_itEqTradeWindow.m_iGold )
		{
			pChar2->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_SENT_GOLD), pPartner->m_itEqTradeWindow.m_iGold, pChar1->GetName());
			pChar1->SysMessagef(g_Cfg.GetDefaultMsg(DEFMSG_MSG_TRADE_RECEIVED_GOLD), pPartner->m_itEqTradeWindow.m_iGold, pChar2->GetName());
		}

		int64 iGold1 = m_itEqTradeWindow.m_iGold + (m_itEqTradeWindow.m_iPlatinum * 1000000000);
		int64 iGold2 = pPartner->m_itEqTradeWindow.m_iGold + (pPartner->m_itEqTradeWindow.m_iPlatinum * 1000000000);
		pChar1->m_virtualGold += iGold2 - iGold1;
		pChar2->m_virtualGold += iGold1 - iGold2;
		pChar1->UpdateStatsFlag();
		pChar2->UpdateStatsFlag();
	}

	// done with trade.
	Delete();
}

void CItemContainer::Trade_UpdateGold( dword platinum, dword gold )
{
	ADDTOCALLSTACK("CItemContainer::Trade_UpdateGold");
	// Update trade gold/platinum values on TOL clients
	CItemContainer *pPartner = dynamic_cast<CItemContainer*>(m_uidLink.ItemFind());
	if ( !pPartner )
		return;
	CChar *pChar1 = dynamic_cast<CChar *>(GetParent());
	if ( !pChar1 || !pChar1->IsClient() )
		return;
	CChar *pChar2 = dynamic_cast<CChar *>(pPartner->GetParent());
	if ( !pChar2 || !pChar2->IsClient() )
		return;

	bool bUpdateChar1 = false;
	bool bUpdateChar2 = pChar2->GetClient()->GetNetState()->isClientVersion(MINCLIVER_NEWSECURETRADE);

	// To prevent cheating, check if the char really have these gold/platinum values
	int64 iMaxValue = pChar1->m_virtualGold;
	if ( gold + (platinum * 1000000000) > iMaxValue )
	{
		gold = (dword)(iMaxValue % 1000000000);
		platinum = (dword)(iMaxValue / 1000000000);
		bUpdateChar1 = true;
	}

	m_itEqTradeWindow.m_iGold = gold;
	m_itEqTradeWindow.m_iPlatinum = platinum;

	PacketTradeAction cmd(SECURE_TRADE_UPDATEGOLD);
	cmd.prepareUpdateGold(this, gold, platinum);
	if ( bUpdateChar1 )
		cmd.send(pChar1->GetClient());
	if ( bUpdateChar2 )
		cmd.send(pChar2->GetClient());
}

void CItemContainer::Trade_Delete()
{
	ADDTOCALLSTACK("CItemContainer::Trade_Delete");
	// Called when object deleted.

	ASSERT(IsType(IT_EQ_TRADE_WINDOW));


	CChar *pChar = dynamic_cast<CChar *>(GetParent());
	if ( !pChar )
		return;

	if ( pChar->IsClient() )
	{
		// Send the cancel trade message.
		PacketTradeAction cmd(SECURE_TRADE_CLOSE);
		cmd.prepareClose(this);
		cmd.send(pChar->GetClient());
	}

	// Drop items back in my pack.
	CItem *pItemNext = NULL;
	for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItemNext )
	{
		pItemNext = pItem->GetNext();
		pChar->ItemBounce(pItem, false);
	}

	// Kill my trading partner.
	CItemContainer *pPartner = dynamic_cast<CItemContainer *>(m_uidLink.ItemFind());
	if ( !pPartner )
		return;

	if ( IsTrigUsed(TRIGGER_TRADECLOSE) )
	{
		CChar *pChar2 = dynamic_cast<CChar *>(pPartner->GetParent());
		CScriptTriggerArgs Args(pChar2);
		pChar->OnTrigger(CTRIG_TradeClose, pChar, &Args);
		CScriptTriggerArgs Args2(pChar);
		pChar2->OnTrigger(CTRIG_TradeClose, pChar, &Args2);
	}

	m_uidLink.InitUID();	// unlink.
	pPartner->m_uidLink.InitUID();
	pPartner->Delete();
}

int CItemContainer::GetWeight(word amount) const
{	// true weight == container item + contents.
	return( CItem::GetWeight(amount) + CContainer::GetTotalWeight());
}

void CItemContainer::OnWeightChange( int iChange )
{
	ADDTOCALLSTACK("CItemContainer::OnWeightChange");
	CContainer::OnWeightChange(iChange);
	UpdatePropertyFlag(AUTOTOOLTIP_FLAG_WEIGHT);

	if ( iChange == 0 )
		return;	// no change

				// some containers do not add weight to you.
	if ( !IsWeighed() )
		return;

	// Propagate the weight change up the stack if there is one.
	CContainer *pCont = dynamic_cast<CContainer *>(GetParent());
	if ( !pCont )
		return;	// on ground.
	pCont->OnWeightChange(iChange);
}

CPointMap CItemContainer::GetRandContainerLoc() const
{
	ADDTOCALLSTACK("CItemContainer::GetRandContainerLoc");
	// Max/Min Container Sizes.

	static const struct // we can probably get this from MUL file some place.
	{
		GUMP_TYPE m_gump;
		word m_minx;
		word m_miny;
		word m_maxx;
		word m_maxy;
	}

	sm_ContSize[] =
	{
		{ GUMP_RESERVED, 40, 50, 100, 100 },		// default.
		{ GUMP_CORPSE, 20, 85, 80, 185 },
		{ GUMP_PACK, 44, 65, 142, 150 },			// Open backpack
		{ GUMP_BAG, 29, 34, 93, 119 },				// Leather Bag
		{ GUMP_BARREL, 33, 36, 98, 139 },			// Barrel
		{ GUMP_BASKET_SQ, 19, 47, 138, 114 },		// Square picknick Basket
		{ GUMP_BOX_WOOD, 16, 51, 140, 115 },		// small wood box with a lock
		{ GUMP_BASKET_RO, 33, 36,  98, 134 },		// Round Basket
		{ GUMP_CHEST_GO_SI, 18, 105, 118, 169 },	// Gold and Silver Chest.
		{ GUMP_BOX_WOOD_OR, 16, 51, 140, 115 },		// Small wood box (ornate)(no lock)
		{ GUMP_CRATE, 20, 10, 126, 91 },			// Wood Crate
		{ GUMP_DRAWER_DK, 16, 17, 110, 85 },
		{ GUMP_CHEST_WO_GO, 18, 105, 118, 169 },	// Wood with gold trim.
		{ GUMP_CHEST_SI, 18, 105, 118, 169 },		// silver chest.
		{ GUMP_BOX_GO_LO, 16, 51, 140, 115 },		// Gold/Brass box with a lock.
		{ GUMP_SHIP_HOLD, 46, 74, 152, 175 },
		{ GUMP_BOOK_SHELF, 76, 12, 96, 59 },
		{ GUMP_CABINET_DK, 24, 96, 91, 143 },
		{ GUMP_CABINET_LT, 24, 96, 91, 143 },
		{ GUMP_DRAWER_LT, 16, 17, 110, 85 },
		{ GUMP_GIFT_BOX, 35, 10, 155, 85 },
		{ GUMP_ARMOIRE_RED, 10, 10, 150, 95 },
		{ GUMP_ARMOIRE_MAPLE, 10, 10, 150, 95 },
		{ GUMP_ARMOIRE_CHERRY, 10, 10, 150, 95 },
		{ GUMP_BASKET_TALL, 10, 10, 116, 71 },
		{ GUMP_CHEST_WOOD_PLAIN, 10, 10, 150, 95 },
		{ GUMP_CHEST_WOOD_GILDED, 10, 10, 150, 95 },
		{ GUMP_CHEST_WOOD_ORNATE, 10, 10, 150, 95 },
		{ GUMP_TALL_CABINET, 10, 10, 150, 95 },
		{ GUMP_CHEST_WOOD_FINISH, 10, 10, 150, 95 },
		{ GUMP_GIFT_BOX_CUBE, 23, 51, 140, 100 },
		{ GUMP_GIFT_BOX_CILINDER, 16, 51, 140, 115 },
		{ GUMP_GIFT_BOX_OCTOGON, 25, 51, 140, 115 },
		{ GUMP_GIFT_BOX_RECTANGLE, 16, 51, 140, 100 },
		{ GUMP_GIFT_BOX_ANGEL, 21, 51, 140, 100 },
		{ GUMP_HEART_SHAPED, 56, 30, 102, 74 },
		{ GUMP_SECURE_TRADE, 1, 1, 66, 26 },
		{ GUMP_GAME_BOARD,	 4, 10, 220, 185 },		// Chess or checker board.
		{ GUMP_GAME_BACKGAM, 4, 10, 220, 185 },
	};

	// ??? pItemDef->m_ttContainer.m_dwMinXY to m_dwMaxXY
	// Get a random location in the container.

	CItemBase *pItemDef = Item_GetDef();
	GUMP_TYPE gump = pItemDef->IsTypeContainer();

	// check for custom values in TDATA3/TDATA4
	if ( pItemDef->m_ttContainer.m_dwMaxXY )
	{
		int tmp_MinX = (pItemDef->m_ttContainer.m_dwMinXY & 0xFFFF0000) >> 16;
		int tmp_MinY = (pItemDef->m_ttContainer.m_dwMinXY & 0xFFFF);
		int tmp_MaxX = (pItemDef->m_ttContainer.m_dwMaxXY & 0xFFFF0000) >> 16;
		int tmp_MaxY = (pItemDef->m_ttContainer.m_dwMaxXY & 0xFFFF);
		DEBUG_WARN(("Custom container gump id %d for 0%x\n", gump, GetDispID()));
		return CPointMap(
			(word)(tmp_MinX + Calc_GetRandVal(tmp_MaxX - tmp_MinX)),
			(word)(tmp_MinY + Calc_GetRandVal(tmp_MaxY - tmp_MinY)),
			0);
	}

	uint i = 0;
	for ( ; ; i++ )
	{
		if ( i >= COUNTOF(sm_ContSize) )
		{
			i = 0;	// set to default
			DEBUG_WARN(("Unknown container gump id %d for 0%x\n", gump, GetDispID()));
			break;
		}
		if ( sm_ContSize[i].m_gump == gump )
			break;
	}

	return CPointMap(
		(word)(sm_ContSize[i].m_minx + Calc_GetRandVal(sm_ContSize[i].m_maxx - sm_ContSize[i].m_minx)),
		(word)(sm_ContSize[i].m_miny + Calc_GetRandVal(sm_ContSize[i].m_maxy - sm_ContSize[i].m_miny)),
		0);
}

void CItemContainer::ContentAdd( CItem *pItem, CPointMap pt, uchar gridIndex )
{
	ADDTOCALLSTACK("CItemContainer::ContentAdd");
	// Add to CItemContainer
	if ( !pItem )
		return;
	if ( pItem == this )
		return;	// infinite loop.

	if ( !g_Serv.IsLoading() )
	{
		if ( IsType(IT_EQ_TRADE_WINDOW) )
			Trade_Status(false);	// drop into a trade window

		switch ( pItem->GetType() )
		{
			case IT_LIGHT_LIT:
				// Douse the light in a pack ! (if possible)
				pItem->Use_Light();
				break;
			case IT_GAME_BOARD:
			{
				// Can't be put into any sort of a container.
				// delete all it's pieces.
				CItemContainer *pCont = dynamic_cast<CItemContainer *>(pItem);
				ASSERT(pCont);
				pCont->DeleteAll();
				break;
			}
			default:
				break;
		}
	}

	if ( pt.m_x <= 0 || pt.m_y <= 0 || pt.m_x > 512 || pt.m_y > 512 )	// invalid container location ?
	{
		bool fInsert = false;
		// Try to stack it.
		if ( !g_Serv.IsLoading() && pItem->Item_GetDef()->IsStackableType() )
		{
			for ( CItem *pTry = GetContentHead(); pTry != NULL; pTry = pTry->GetNext() )
			{
				pt = pTry->GetContainedPoint();
				if ( pItem->Stack(pTry) )
				{
					fInsert = true;
					break;
				}
			}
		}
		if ( !fInsert )
			pt = GetRandContainerLoc();
	}

	// check that the grid index isn't already in use
	bool bValidGrid = true;
	for ( CItem *pTry = GetContentHead(); pTry != NULL; pTry = pTry->GetNext() )
	{
		if ( pTry->GetContainedGridIndex() == gridIndex )
		{
			bValidGrid = false;
			break;
		}
	}

	if ( !bValidGrid )
	{
		// the grid index we've been given is already in use, so find the
		// first unused grid index
		for ( gridIndex = 0; (gridIndex < 255 && !bValidGrid); gridIndex++ )
		{
			bValidGrid = true;
			for ( CItem *pTry = GetContentHead(); pTry != NULL; pTry = pTry->GetNext() )
			{
				if ( pTry->GetContainedGridIndex() == gridIndex )
				{
					bValidGrid = false;
					break;
				}
			}

			if ( bValidGrid )
				break;
		}
	}

	CContainer::ContentAddPrivate(pItem);
	pItem->SetContainedPoint(pt);
	pItem->SetContainedGridIndex(gridIndex);

	// if an item needs OnTickStatusUpdate called on the next tick, it needs
	// to be added to a separate list since it won't receive ticks whilst in
	// this container
	if ( pItem->m_fStatusUpdate != 0 && g_World.m_ObjStatusUpdates.ContainsPtr(pItem) == false )
		g_World.m_ObjStatusUpdates.Add(pItem);

	switch ( GetType() )
	{
		case IT_KEYRING: // empty key ring.
			SetKeyRing();
			break;
		case IT_EQ_VENDOR_BOX:
			if ( !IsItemEquipped() )	// vendor boxes should ALWAYS be equipped !
			{
				DEBUG_ERR(("Un-equipped vendor box uid=0%x is bad\n", (dword)GetUID()));
				break;
			}
			{
				CItemVendable *pItemVend = dynamic_cast<CItemVendable *>(pItem);
				if ( !pItemVend )
				{
					g_Log.Event(LOGL_WARN, "Vendor non-vendable item: %s uid=0%x, vendor: %s uid=0%x\n", pItem->GetResourceName(), pItem->GetUID().GetObjUID(), GetContainer()->GetName(), GetContainer()->GetUID().GetObjUID());
					pItem->Delete();
					break;
				}

				pItemVend->SetPlayerVendorPrice(0);	// unpriced yet.
				pItemVend->SetContainedLayer((uchar)(pItem->GetAmount()));
			}
			break;
		case IT_GAME_BOARD:
			// Can only place IT_GAME_PIECE inside here
			if ( pItem->IsType(IT_GAME_PIECE) )
				break;
			g_Log.Event(LOGL_WARN, "Game board contains invalid item: %s uid=0%x, board: %s uid=0%x\n", pItem->GetResourceName(), pItem->GetUID().GetObjUID(), GetResourceName(), GetUID().GetObjUID());
			pItem->Delete();
			break;
		default:
			break;
	}

	switch ( pItem->GetID() )
	{
		case ITEMID_BEDROLL_O_EW:
		case ITEMID_BEDROLL_O_NS:
			// Close the bedroll
			pItem->SetDispID(ITEMID_BEDROLL_C);
			break;
		default:
			break;
	}

	pItem->Update();
}

void CItemContainer::ContentAdd( CItem *pItem )
{
	ADDTOCALLSTACK("CItemContainer::ContentAdd");
	if ( !pItem )
		return;
	if ( pItem->GetParent() == this )
		return;	// already here.
	CPointMap pt;	// invalid point.
	if ( g_Serv.IsLoading() )
		pt = pItem->GetUnkPoint();

	ContentAdd(pItem, pt);
}

bool CItemContainer::IsWeighed() const
{
	if ( IsType(IT_EQ_BANK_BOX ))
		return false;
	if ( IsType(IT_EQ_VENDOR_BOX))
		return false;
	if ( IsAttr(ATTR_MAGIC))	// magic containers have no weight.
		return false;
	return true;
}

bool CItemContainer::IsSearchable() const
{
	if ( IsType(IT_EQ_BANK_BOX) )
		return false;
	if ( IsType(IT_EQ_VENDOR_BOX) )
		return false;
	if ( IsType(IT_CONTAINER_LOCKED) )
		return false;
	if ( IsType(IT_EQ_TRADE_WINDOW) )
		return false;
	return true;
}

bool CItemContainer::IsItemInside( const CItem *pItem ) const
{
	ADDTOCALLSTACK("CItemContainer::IsItemInside");
	// Checks if a particular item is in a container or one of
	// it's subcontainers.

	for (;;)
	{
		if ( !pItem )
			return false;
		if ( pItem == this )
			return true;
		pItem = dynamic_cast<CItemContainer *>(pItem->GetParent());
	}
}

void CItemContainer::OnRemoveOb( CGObListRec *pObRec )	// Override this = called when removed from list.
{
	ADDTOCALLSTACK("CItemContainer::OnRemoveOb");
	// remove this object from the container list.
	CItem *pItem = static_cast<CItem *>(pObRec);
	ASSERT(pItem);
	if ( IsType(IT_EQ_TRADE_WINDOW) )
	{
		// Remove from a trade window.
		Trade_Status(false);
		m_itEqTradeWindow.m_iWaitTime = g_World.GetCurrentTime().GetTimeRaw() + (10 * TICK_PER_SEC);
	}
	if ( IsType(IT_EQ_VENDOR_BOX) && IsItemEquipped() )	// vendor boxes should ALWAYS be equipped !
	{
		CItemVendable *pItemVend = dynamic_cast<CItemVendable *>(pItem);
		if ( pItemVend )
			pItemVend->SetPlayerVendorPrice(0);
	}

	CContainer::OnRemoveOb(pObRec);
	if ( IsType(IT_KEYRING) )	// key ring.
		SetKeyRing();
}

void CItemContainer::DupeCopy( const CItem *pItem )
{
	ADDTOCALLSTACK("CItemContainer::DupeCopy");
	// Copy the contents of this item.

	CItemVendable::DupeCopy(pItem);

	const CItemContainer *pContItem = dynamic_cast<const CItemContainer *>(pItem);
	if ( !pContItem )
		return;

	for ( CItem *pContent = pContItem->GetContentHead(); pContent != NULL; pContent = pContent->GetNext() )
		ContentAdd(CreateDupeItem(pContent), pContent->GetContainedPoint());
}

void CItemContainer::MakeKey()
{
	ADDTOCALLSTACK("CItemContainer::MakeKey");
	SetType(IT_CONTAINER);
	m_itContainer.m_lockUID = GetUID();
	m_itContainer.m_lock_complexity = 500 + Calc_GetRandVal(600);

	CItem *pKey = CreateScript(ITEMID_KEY_COPPER);
	ASSERT(pKey);
	pKey->m_itKey.m_lockUID = GetUID();
	ContentAdd(pKey);
}

void CItemContainer::SetKeyRing()
{
	ADDTOCALLSTACK("CItemContainer::SetKeyRing");
	// Look of a key ring depends on how many keys it has in it.
	static const ITEMID_TYPE sm_Item_Keyrings[] =
	{
		ITEMID_KEY_RING0, // empty key ring.
		ITEMID_KEY_RING1,
		ITEMID_KEY_RING1,
		ITEMID_KEY_RING3,
		ITEMID_KEY_RING3,
		ITEMID_KEY_RING5
	};

	size_t iQty = GetCount();
	if ( iQty >= COUNTOF(sm_Item_Keyrings) )
		iQty = COUNTOF(sm_Item_Keyrings) - 1;

	ITEMID_TYPE id = sm_Item_Keyrings[iQty];
	if ( id != GetID() )
	{
		SetID(id);	// change the type as well.
		Update();
	}
}

bool CItemContainer::CanContainerHold( const CItem *pItem, const CChar *pCharMsg )
{
	ADDTOCALLSTACK("CItemContainer::CanContainerHold");
	if ( !pCharMsg || !pItem )
		return false;
	if ( pCharMsg->IsPriv(PRIV_GM) )	// a gm can doing anything.
		return true;

	if ( IsAttr(ATTR_MAGIC) )
	{
		// Put stuff in a magic box
		pCharMsg->SysMessageDefault(DEFMSG_CONT_MAGIC);
		return false;
	}

	size_t pTagTmp = static_cast<size_t>(GetKeyNum("OVERRIDE.MAXITEMS", true));
	size_t tMaxItemsCont = pTagTmp ? pTagTmp : MAX_ITEMS_CONT;
	if ( GetCount() >= tMaxItemsCont )
	{
		pCharMsg->SysMessageDefault(DEFMSG_CONT_FULL);
		return false;
	}

	pTagTmp = static_cast<size_t>(GetKeyNum("OVERRIDE.MAXWEIGHT", true));
	if ( pTagTmp )	// weightcheck does ALSO apply on backpack if tag is set!
	{
		if ( (GetWeight() + pItem->GetWeight()) > ((int)(pTagTmp) * WEIGHT_UNITS) )
		{
			pCharMsg->SysMessageDefault(DEFMSG_CONT_FULL_WEIGHT);
			return false;
		}
	}

	if ( !IsItemEquipped() &&	// does not apply to my pack.
		pItem->IsContainer() &&
		pItem->Item_GetDef()->GetVolume() >= Item_GetDef()->GetVolume() )
	{
		// is the container too small ? can't put barrels in barrels.
		pCharMsg->SysMessageDefault(DEFMSG_CONT_TOOSMALL);
		return false;
	}

	switch ( GetType() )
	{
		case IT_EQ_BANK_BOX:
		{
			// Check if the bankbox will allow this item to be dropped into it.
			// Too many items or too much weight?

			int iBankIMax = g_Cfg.m_iBankIMax;
			CVarDefCont *pTagTemp = GetKey("OVERRIDE.MAXITEMS", false);
			if ( pTagTemp )
				iBankIMax = (int)(pTagTemp->GetValNum());

			if ( iBankIMax >= 0 )
			{
				// Check if the item dropped in the bank is a container. If it is
				// we need to calculate the number of items in that too.
				int iItemsInContainer = 0;
				const CItemContainer *pContItem = dynamic_cast<const CItemContainer *>(pItem);
				if ( pContItem )
					iItemsInContainer = pContItem->ContentCountAll();

				// Check the total number of items in the bankbox and the ev.
				// container put into it.
				if ( (ContentCountAll() + iItemsInContainer) > iBankIMax )
				{
					pCharMsg->SysMessageDefault(DEFMSG_BVBOX_FULL_ITEMS);
					return false;
				}
			}

			int iBankWMax = g_Cfg.m_iBankWMax;
			pTagTemp = GetKey("OVERRIDE.MAXWEIGHT", false);
			if ( pTagTemp )
				iBankWMax = (int)(pTagTemp->GetValNum()) * WEIGHT_UNITS;

			if ( iBankWMax >= 0 )
			{
				// Check the weightlimit on bankboxes.
				if ( (GetWeight() + pItem->GetWeight()) > iBankWMax )
				{
					pCharMsg->SysMessageDefault(DEFMSG_BVBOX_FULL_WEIGHT);
					return false;
				}
			}
		}
		break;

		case IT_GAME_BOARD:
			if ( !pItem->IsType(IT_GAME_PIECE) )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOTGAMEPIECE);
				return false;
			}
			break;
		case IT_BBOARD:		// This is a trade window or a bboard.
			return false;
		case IT_EQ_TRADE_WINDOW:
			// BBoards are containers but you can't put stuff in them this way.
			return true;

		case IT_KEYRING: // empty key ring.
			if ( !pItem->IsType(IT_KEY) )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOTKEY);
				return false;
			}
			if ( !pItem->m_itKey.m_lockUID )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOBLANKRING);
				return false;
			}
			break;

		case IT_EQ_VENDOR_BOX:
			if ( pItem->IsTimerSet() && !pItem->IsAttr(ATTR_DECAY) )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOT4SALE);
				return false;
			}

			// Fix players losing their items when attempting to sell them with no VALUE property scripted
			if ( !pItem->Item_GetDef()->GetMakeValue(0) )
			{
				pCharMsg->SysMessageDefault(DEFMSG_MSG_ERR_NOT4SALE);
				return false;
			}

			// Check that this vendor box hasn't already reached its content limit
			if ( GetCount() >= MAX_ITEMS_CONT )
			{
				pCharMsg->SysMessageDefault(DEFMSG_CONT_FULL);
				return false;
			}
			break;

		case IT_TRASH_CAN:
			Sound(0x235); // a little sound so we know it "ate" it.
			pCharMsg->SysMessageDefault(DEFMSG_ITEMUSE_TRASHCAN);
			SetTimeout(15 * TICK_PER_SEC);
			break;

		default:
			break;
	}

	return true;
}

void CItemContainer::Restock()
{
	ADDTOCALLSTACK("CItemContainer::Restock");
	// Check for vendor type containers.

	if ( IsItemEquipped() )
	{
		// Part of a vendor.
		CChar *pChar = dynamic_cast<CChar *>(GetParent());
		if ( pChar && !pChar->IsStatFlag(STATF_Pet) )
		{
			switch ( GetEquipLayer() )
			{
				case LAYER_VENDOR_STOCK:
					// Magic restock the vendors container.
				{
					for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
					{
						CItemVendable *pVendItem = dynamic_cast<CItemVendable *>(pItem);
						if ( pVendItem )
							pVendItem->Restock(true);
					}
				}
				break;

				case LAYER_VENDOR_EXTRA:
					// clear all this junk periodically.
					// sell it back for cash value ?
					DeleteAll();
					break;

				case LAYER_VENDOR_BUYS:
				{
					// Reset what we will buy from players.
					for ( CItem *pItem = GetContentHead(); pItem != NULL; pItem = pItem->GetNext() )
					{
						CItemVendable *pVendItem = dynamic_cast<CItemVendable *>(pItem);
						if ( pVendItem )
							pVendItem->Restock(false);
					}
				}
				break;

				case LAYER_BANKBOX:
					// Restock petty cash.
					if ( !m_itEqBankBox.m_Check_Restock )
						m_itEqBankBox.m_Check_Restock = 10000;
					if ( m_itEqBankBox.m_Check_Amount < m_itEqBankBox.m_Check_Restock )
						m_itEqBankBox.m_Check_Amount = m_itEqBankBox.m_Check_Restock;
					return;

				default:
					break;
			}
		}
	}
}

void CItemContainer::OnOpenEvent( CChar *pCharOpener, const CObjBaseTemplate *pObjTop )
{
	ADDTOCALLSTACK("CItemContainer::OnOpenEvent");
	// The container is being opened. Explode ? etc ?

	if ( !pCharOpener )
		return;

	if ( IsType(IT_EQ_BANK_BOX) || IsType(IT_EQ_VENDOR_BOX) )
	{
		const CChar *pCharTop = dynamic_cast<const CChar *>(pObjTop);
		if ( !pCharTop )
			return;

		int iStones = GetWeight() / WEIGHT_UNITS;
		tchar *pszMsg = Str_GetTemp();
		if ( pCharTop == pCharOpener )
			sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_BVBOX_OPEN_SELF), iStones, GetName());
		else
			sprintf(pszMsg, g_Cfg.GetDefaultMsg(DEFMSG_BVBOX_OPEN_OTHER), pCharTop->GetPronoun(), iStones, pCharTop->GetPossessPronoun(), GetName());

		pCharOpener->SysMessage(pszMsg);

		// these are special. they can only be opened near the designated opener.
		// see CanTouch
		m_itEqBankBox.m_pntOpen = pCharOpener->GetTopPoint();
	}
}

void CItemContainer::Game_Create()
{
	ADDTOCALLSTACK("CItemContainer::Game_Create");
	ASSERT(IsType(IT_GAME_BOARD));

	if ( GetCount() > 0 )
		return;	// already here.

	static const ITEMID_TYPE sm_Item_ChessPieces[] =
	{
		ITEMID_GAME1_ROOK,		// 42,4
		ITEMID_GAME1_KNIGHT,
		ITEMID_GAME1_BISHOP,
		ITEMID_GAME1_QUEEN,
		ITEMID_GAME1_KING,
		ITEMID_GAME1_BISHOP,
		ITEMID_GAME1_KNIGHT,
		ITEMID_GAME1_ROOK,		// 218,4

		ITEMID_GAME1_PAWN,		// 42,41
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,
		ITEMID_GAME1_PAWN,		// 218, 41

		ITEMID_GAME2_PAWN,		// 44, 167
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,
		ITEMID_GAME2_PAWN,		// 218, 164

		ITEMID_GAME2_ROOK,		// 42, 185
		ITEMID_GAME2_KNIGHT,
		ITEMID_GAME2_BISHOP,
		ITEMID_GAME2_QUEEN,
		ITEMID_GAME2_KING,
		ITEMID_GAME2_BISHOP,
		ITEMID_GAME2_KNIGHT,
		ITEMID_GAME2_ROOK		// 218, 183
	};

	CPointMap pt;

	// Chess has 2 rows of 8 pieces on each side
	static const word sm_ChessRow[] =
	{
		5,
		40,
		160,
		184
	};

	// Checkers has 3 rows of 4 pieces on each side
	static const word sm_CheckerRow[] =
	{
		30,
		55,
		80,
		155,
		180,
		205
	};

	// Backgammon has an odd setup
	static const word sm_BackgammonRow[] =
	{
		// White Side
		// Position 13
		8,
		23,
		38,
		53,
		68,
		// Position 6
		128,
		143,
		158,
		173,
		188,
		// Position 24
		8,
		23,
		// Position 8
		158,
		173,
		188,

		// Black Side
		// Position 12
		128,
		143,
		158,
		173,
		188,
		// Position 19
		8,
		23,
		38,
		53,
		68,
		// Position 1
		173,
		188,
		// Position 17
		8,
		23,
		38
	};

	if ( m_itGameBoard.m_GameType == 0 )	// Chess
	{
		for ( size_t i = 0; i < COUNTOF(sm_Item_ChessPieces); i++ )
		{
			// Add all it's pieces. (if not already added)
			CItem *pPiece = CItem::CreateBase(sm_Item_ChessPieces[i]);
			if ( !pPiece )
				break;
			pPiece->SetType(IT_GAME_PIECE);

			// Move to the next row after 8 pieces
			if ( (i & 7) == 0 )
			{
				pt.m_x = 42;
				pt.m_y = sm_ChessRow[i / 8];
			}
			else
			{
				pt.m_x += 25;
			}
			ContentAdd(pPiece, pt);
		}
	}
	else if ( m_itGameBoard.m_GameType == 1 )	// Checkers
	{
		for ( int i = 0; i < 24; i++ )
		{
			// Add all it's pieces. (if not already added)
			CItem *pPiece = CItem::CreateBase(((i >= (3 * 4)) ? ITEMID_GAME1_CHECKER : ITEMID_GAME2_CHECKER));
			if ( !pPiece )
				break;
			pPiece->SetType(IT_GAME_PIECE);

			// Move to the next row after 4 pieces
			// In checkers the pieces are placed on the black squares
			if ( (i & 3) == 0 )
			{
				pt.m_x = ((i / 4) & 1) ? 67 : 42;
				pt.m_y = sm_CheckerRow[i / 4];
			}
			else
			{
				// Skip white squares
				pt.m_x += 50;
			}
			ContentAdd(pPiece, pt);
		}
	}
	else if ( m_itGameBoard.m_GameType == 2 )	// Backgammon
	{
		for ( int i = 0; i < 30; i++ )
		{
			// Add all it's pieces. (if not already added)
			CItem *pPiece = CItem::CreateBase(((i >= 15) ? ITEMID_GAME1_CHECKER : ITEMID_GAME2_CHECKER));
			if ( !pPiece )
				break;
			pPiece->SetType(IT_GAME_PIECE);

			// Backgammon has a strange setup.
			// Someone more familiar with the game may be able
			// to improve the code for this
			if ( (i == 12) || (i == 27) )
				pt.m_x = 107;
			else if ( (i == 10) || (i == 25) )
				pt.m_x = 224;
			else if ( (i == 5) || (i == 20) )
				pt.m_x = 141;
			else if ( (i == 0) || (i == 15) )
				pt.m_x = 41;
			pt.m_y = sm_BackgammonRow[i];
			ContentAdd(pPiece, pt);
		}
	}
}

enum ICV_TYPE
{
	ICV_CLOSE,
	ICV_DELETE,
	ICV_EMPTY,
	ICV_FIXWEIGHT,
	ICV_OPEN,
	ICV_QTY
};

lpctstr const CItemContainer::sm_szVerbKeys[ICV_QTY+1] =
{
	"CLOSE",
	"DELETE",
	"EMPTY",
	"FIXWEIGHT",
	"OPEN",
	NULL
};

bool CItemContainer::r_Verb( CScript &s, CTextConsole *pSrc )
{
	ADDTOCALLSTACK("CItemContainer::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	switch ( FindTableSorted(s.GetKey(), sm_szVerbKeys, COUNTOF(sm_szVerbKeys) - 1) )
	{
		case ICV_DELETE:
			if ( s.HasArgs() )
			{
				// 1 based pages.
				size_t index = s.GetArgVal();
				if ( index > 0 && index <= GetCount() )
				{
					delete GetAt(index - 1);
					return true;
				}
			}
			return false;
		case ICV_EMPTY:
		{
			DeleteAll();
			return true;
		}
		case ICV_FIXWEIGHT:
			FixWeight();
			return true;
		case ICV_OPEN:
			if ( pSrc->GetChar() )
			{
				CChar *pChar = pSrc->GetChar();
				if ( pChar->IsClient() )
				{
					CClient *pClient = pChar->GetClient();
					ASSERT(pClient);

					if ( s.HasArgs() )
					{
						pClient->addItem(this);
						pClient->addCustomSpellbookOpen(this, s.GetArgVal());
						return true;
					}
					pClient->addItem(this);	// may crash client if we dont do this.
					pClient->addContainerSetup(this);
					OnOpenEvent(pChar, GetTopLevelObj());
				}
			}
			return true;
		case ICV_CLOSE:
			if ( pSrc->GetChar() )
			{
				CChar *pChar = pSrc->GetChar();
				if ( pChar->IsClient() )
				{
					CClient *pClient = pChar->GetClient();
					ASSERT(pClient);
					pClient->closeContainer(this);
				}
			}
			return true;
	}
	return CItemVendable::r_Verb(s, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

bool CItemContainer::OnTick()
{
	ADDTOCALLSTACK("CItemContainer::OnTick");
	// equipped or not.
	switch ( GetType() )
	{
		case IT_TRASH_CAN:
			// Empty it !
			DeleteAll();
			return true;
		case IT_CONTAINER:
			if ( IsAttr(ATTR_MAGIC) )
			{
				// Magic restocking container.
				Restock();
				return true;
			}
			break;
		case IT_EQ_BANK_BOX:
		case IT_EQ_VENDOR_BOX:
			// Restock this box.
			// Magic restocking container.
			Restock();
			SetTimeout(-1);
			return true;
		default:
			break;
	}
	return CItemVendable::OnTick();
}
