
#include "../../common/CException.h"
#include "../../common/CLog.h"
#include "../../network/send.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"
#include "../components/CCPropsItemChar.h"
#include "../CServer.h"
#include "../triggers.h"
#include "CItem.h"
#include "CItemMulti.h"
#include "CItemContainer.h"

//----------------------------------------------------
// -CItemContainer

CItemContainer::CItemContainer( ITEMID_TYPE id, CItemBase *pItemDef ) :
    CTimedObject(PROFILE_ITEMS), CItemVendable( id, pItemDef )
{
}

CItemContainer::~CItemContainer()
{
	CItemContainer::DeletePrepare();
	CContainer::ClearContainer();		// get rid of my contents first to protect against weight calc errors.

    CItemMulti *pMulti = nullptr;
    if (_uidMultiSecured.IsValidUID())
    {
        pMulti = static_cast<CItemMulti*>(_uidMultiSecured.ItemFind());
        if (pMulti)
        {
            pMulti->Release(GetUID(), true);
        }
    }
    if (_uidMultiCrate.IsValidUID())
    {
        pMulti = static_cast<CItemMulti*>(_uidMultiCrate.ItemFind());
        if (pMulti)
        {
			pMulti->SetMovingCrate({});
        }
    }
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
	ADDTOCALLSTACK("CItemContainer::DeletePrepare");
	if ( IsType( IT_EQ_TRADE_WINDOW ))
		Trade_Delete();
	
	CContainer::ContentDelete(false);	// This object and its contents need to be deleted on the same tick
	CItem::DeletePrepare();
}

void CItemContainer::SetSecuredOfMulti(const CUID& uidMulti)
{
    _uidMultiSecured = uidMulti;
}

void CItemContainer::SetCrateOfMulti(const CUID& uidMulti)
{
    _uidMultiCrate = uidMulti;
}

void CItemContainer::r_Write( CScript &s )
{
	ADDTOCALLSTACK("CItemContainer::r_Write");
	CItemVendable::r_Write(s);
	r_WriteContent(s);
}

bool CItemContainer::r_GetRef( lpctstr &ptcKey, CScriptObj *&pRef )
{
	ADDTOCALLSTACK("CItemContainer::r_GetRef");
	if ( r_GetRefContainer(ptcKey, pRef) )
		return true;

	return CItemVendable::r_GetRef(ptcKey, pRef);
}

bool CItemContainer::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole *pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
	ADDTOCALLSTACK("CItemContainer::r_WriteVal");

	EXC_TRY("WriteVal");

	if ( r_WriteValContainer(ptcKey, sVal, pSrc) )
		return true;
	return (fNoCallParent ? false : CItemVendable::r_WriteVal(ptcKey, sVal, pSrc));

	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemContainer::IsItemInTrade() const
{
	// recursively get the item that is at "top" level.
	if ( IsType(IT_EQ_TRADE_WINDOW) )
		return true;
	CItemContainer *pItemCont = dynamic_cast<CItemContainer *>(GetContainer());
	if (pItemCont)
		return pItemCont->IsItemInTrade();
	return false;
}

void CItemContainer::Trade_Status( bool bCheck )
{
	ADDTOCALLSTACK("CItemContainer::Trade_Status");
	// Update trade status check boxes to both sides.
	CItemContainer *pPartner = dynamic_cast<CItemContainer*>(m_uidLink.ItemFind());
	if ( !pPartner )
		return;

	CChar *pChar1 = dynamic_cast<CChar *>(GetParent());
	if ( !pChar1 || !pChar1->IsClientActive() )
		return;
	CChar *pChar2 = dynamic_cast<CChar *>(pPartner->GetParent());
	if ( !pChar2 || !pChar2->IsClientActive() )
		return;

	m_itEqTradeWindow.m_bCheck = bCheck ? 1 : 0;
	if ( !bCheck )
		pPartner->m_itEqTradeWindow.m_bCheck = 0;

	PacketTradeAction cmd(SECURE_TRADE_CHANGE);
	cmd.prepareReadyChange(this, pPartner);
	cmd.send(pChar1->GetClientActive());

	cmd.prepareReadyChange(pPartner, this);
	cmd.send(pChar2->GetClientActive());

	// Check if both clients had pressed the 'accept' buttom
	if ( pPartner->m_itEqTradeWindow.m_bCheck == 0 || m_itEqTradeWindow.m_bCheck == 0 )
		return;

	if ( IsTrigUsed(TRIGGER_TRADEACCEPTED) || IsTrigUsed(TRIGGER_CHARTRADEACCEPTED) )
	{
		CScriptTriggerArgs Args1(pChar1);
		ushort i = 1;
		for (CSObjContRec* pObjRec : *pPartner)
		{
			CItem* pItem = static_cast<CItem*>(pObjRec);
			Args1.m_VarObjs.Insert(i, pItem, true);
			++i;
		}
		Args1.m_iN1 = --i;

		CScriptTriggerArgs Args2(pChar2);
		i = 1;
		for (CSObjContRec * pObjRec : *this)
		{
			CItem* pItem = static_cast<CItem*>(pObjRec);
			Args2.m_VarObjs.Insert(i, pItem, true);
			++i;
		}
		Args2.m_iN1 = --i;

		Args1.m_iN2 = Args2.m_iN1;
		Args2.m_iN2 = Args1.m_iN1;
		if ( (pChar1->OnTrigger(CTRIG_TradeAccepted, pChar2, &Args1) == TRIGRET_RET_TRUE) || (pChar2->OnTrigger(CTRIG_TradeAccepted, pChar1, &Args2) == TRIGRET_RET_TRUE) )
			return;
	}

	// Transfer items
	for (CSObjContRec* pObjRec : pPartner->GetIterationSafeContReverse())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		pChar1->ItemBounce(pItem, false);
	}

	for (CSObjContRec* pObjRec : GetIterationSafeContReverse())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
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

		const int64 iGold1 = m_itEqTradeWindow.m_iGold           + (          m_itEqTradeWindow.m_iPlatinum * 1000000000LL);
		const int64 iGold2 = pPartner->m_itEqTradeWindow.m_iGold + (pPartner->m_itEqTradeWindow.m_iPlatinum * 1000000000LL);
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
	if ( !pChar1 || !pChar1->IsClientActive() )
		return;
	CChar *pChar2 = dynamic_cast<CChar *>(pPartner->GetParent());
	if ( !pChar2 || !pChar2->IsClientActive() )
		return;

	bool bUpdateChar1 = false;
	bool bUpdateChar2 = pChar2->GetClientActive()->GetNetState()->isClientVersion(MINCLIVER_NEWSECURETRADE);

	// To prevent cheating, check if the char really have these gold/platinum values
	const int64 iMaxValue = pChar1->m_virtualGold;
	if ( gold + (platinum * 1000000000LL) > iMaxValue )
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
		cmd.send(pChar1->GetClientActive());
	if ( bUpdateChar2 )
		cmd.send(pChar2->GetClientActive());
}

bool CItemContainer::Trade_Delete()
{
	ADDTOCALLSTACK("CItemContainer::Trade_Delete");
	// Called when object deleted.

	ASSERT(IsType(IT_EQ_TRADE_WINDOW));


	CChar *pChar = dynamic_cast<CChar *>(GetParent());
	if ( !pChar )
		return false;

	if ( pChar->IsClientActive() )
	{
		// Send the cancel trade message.
		PacketTradeAction cmd(SECURE_TRADE_CLOSE);
		cmd.prepareClose(this);
		cmd.send(pChar->GetClientActive());
	}

	// Drop items back in my pack.
	for (CSObjContRec* pObjRec : GetIterationSafeContReverse())
	{
		CItem* pItem = static_cast<CItem*>(pObjRec);
		pChar->ItemBounce(pItem, false);
	}

	// Kill my trading partner.
	CItemContainer *pPartner = dynamic_cast<CItemContainer *>(m_uidLink.ItemFind());
	if ( !pPartner )
		return false;

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
	return pPartner->Delete();
}

int CItemContainer::GetWeight(word amount) const
{	// true weight == container item + contents.
	return( CItem::GetWeight(amount) + CContainer::GetTotalWeight());
}

void CItemContainer::OnWeightChange( int iChange )
{
	ADDTOCALLSTACK("CItemContainer::OnWeightChange");
	if ( iChange == 0 )
		return;

	// Use WeightReduction property
	if (GetPropNum(COMP_PROPS_ITEMCHAR, PROPITCH_WEIGHTREDUCTION, true))
		iChange = iChange * (100 - (GetPropNum(COMP_PROPS_ITEMCHAR, PROPITCH_WEIGHTREDUCTION, true))) / 100;

	// Apply the weight change on the container
	CContainer::OnWeightChange(iChange);
	UpdatePropertyFlag();

	// Some containers do not add weight to you.
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
	} sm_ContSize[] =
	{
        { GUMP_SCROLL, 30, 30, 270, 170 },
        { GUMP_CORPSE, 20, 85, 124, 196 },
        { GUMP_BACKPACK, 44, 65, 186, 159 },
        { GUMP_BAG, 29, 34, 137, 128 },
        { GUMP_BARREL, 33, 36, 142, 148 },
        { GUMP_BASKET_SQUARE, 19, 47, 182, 123 },
        { GUMP_BOX_WOOD, 16, 38, 152, 125 },
        { GUMP_BASKET_ROUND, 35, 38, 145, 116 },
        { GUMP_CHEST_METAL_GOLD, 18, 105, 162, 178 },
        { GUMP_BOX_WOOD_ORNATE, 16, 51, 184, 124 },
        { GUMP_CRATE, 20, 10, 170, 100 },
        { GUMP_LEATHER, 16, 10, 148, 138 },
        { GUMP_DRAWER_DARK, 16, 10, 154, 94 },
        { GUMP_CHEST_WOOD, 18, 105, 162, 178 },
        { GUMP_CHEST_METAL, 18, 105, 162, 178 },
        { GUMP_BOX_METAL, 16, 51, 184, 124 },
        { GUMP_SHIP_HATCH, 46, 74, 196, 184 },
        { GUMP_BOOK_SHELF, 76, 12, 110, 68 },
        { GUMP_CABINET_DARK, 24, 96, 96, 152 },
        { GUMP_CABINET_LIGHT, 24, 96, 96, 152 },
        { GUMP_DRAWER_LIGHT, 16, 10, 154, 94 },
        { GUMP_BULLETIN_BOARD, 0, 0, 110, 62 },
        { GUMP_GIFT_BOX, 35, 10, 190, 95 },
        { GUMP_STOCKING, 41, 21, 186, 111 },
        { GUMP_ARMOIRE_ELVEN, 10, 10, 170, 115 },
        { GUMP_ARMOIRE_RED, 10, 10, 170, 115 },
        { GUMP_ARMOIRE_MAPLE, 10, 10, 170, 115 },
        { GUMP_ARMOIRE_CHERRY, 10, 10, 170, 115 },
        { GUMP_BASKET_TALL, 10, 30, 170, 145 },
        { GUMP_CHEST_WOOD_PLAIN, 10, 10, 170, 115 },
        { GUMP_CHEST_WOOD_GILDED, 10, 10, 170, 115 },
        { GUMP_CHEST_WOOD_ORNATE, 10, 10, 170, 115 },
        { GUMP_TALL_CABINET, 10, 10, 170, 115 },
        { GUMP_CHEST_WOOD_FINISH, 10, 10, 170, 115 },
        { GUMP_DRAWER_RED, 10, 10, 170, 115 },
        { GUMP_BLESSED_STATUE, 44, 29, 128, 103 },
        { GUMP_MAILBOX, 19, 61, 119, 155 },
        { GUMP_GIFT_BOX_CUBE, 23, 51, 163, 151 },
        { GUMP_GIFT_BOX_CYLINDER, 16, 51, 156, 166 },
        { GUMP_GIFT_BOX_OCTOGON, 25, 51, 165, 166 },
        { GUMP_GIFT_BOX_RECTANGLE, 16, 51, 156, 151 },
        { GUMP_GIFT_BOX_ANGEL, 21, 51, 161, 151 },
        { GUMP_GIFT_BOX_HEART_SHAPED, 56, 30, 158, 104 },
        { GUMP_GIFT_BOX_TALL, 77, 44, 161, 105 },
        { GUMP_GIFT_BOX_CHRISTMAS, 16, 51, 156, 166 },
        { GUMP_WALL_SAFE, 35, 13, 112, 165 },
        { GUMP_CHEST_HUGE, 61, 86, 574, 382 },
        { GUMP_CHEST_PIRATE, 49, 76, 141, 114 },
        { GUMP_FOUNTAIN_LIFE, 4, 42, 162, 110 },
		{ GUMP_COMBINATION_CHEST_OPEN, 49, 121, 349, 236 },
		{ GUMP_MAILBOX_DOLPHIN, 16, 84, 119, 153 },
		{ GUMP_MAILBOX_SQUIRREL, 16, 66, 116, 149 },
		{ GUMP_MAILBOX_BARREL, 13, 71, 122, 147 },
		{ GUMP_MAILBOX_LANTERN, 17, 63, 117, 152 },
		{ GUMP_WARDROBE_YELLOW, 66, 73, 308, 542 },
		{ GUMP_WARDROBE_BROWN, 66, 73, 308, 542 },
		{ GUMP_DRAWER_YELLOW, 57, 54, 545, 300 },
		{ GUMP_DRAWER_BROWN, 57, 54, 545, 300 },
		{ GUMP_BARREL_SHORT, 48, 78, 363, 316 },
		{ GUMP_BOOKCASE_BROWN, 93, 32, 567, 344 },
        { GUMP_SECURE_TRADE, 20, 30, 380, 180 },
        { GUMP_SEED_BOX, 16, 29, 283, 338 },
        { GUMP_SECURE_TRADE_TOL, 19, 108, 154, 183 },
        { GUMP_BOARD_CHECKER, 0, 0, 282, 230 },
        { GUMP_BOARD_BACKGAMMON, 0, 0, 282, 210 },
        { GUMP_CHEST_WEDDING, 16, 51, 184, 124 },
        { GUMP_STONE_BASE, 16, 51, 184, 124 },
        { GUMP_PLAGUE_BEAST, 60, 33, 460, 348 },
        { GUMP_REGAL_CASE, 32, 72, 226, 150 },
        { GUMP_BACKPACK_SUEDE, 44, 65, 186, 159 },
        { GUMP_BACKPACK_POLAR_BEAR, 44, 65, 186, 159 },
        { GUMP_BACKPACK_GHOUL_SKIN, 44, 65, 186, 159 },
        { GUMP_GIFT_BOX_CHRISTMAS_2, 25, 36, 194, 120 },
        { GUMP_WALL_SAFE_COMBINATION, 9, 13, 117, 176 },
        { GUMP_CRATE_FLETCHING, 24, 96, 196, 152 },
        { GUMP_DRAWER_ROYAL, 50, 90, 552, 343 },
        { GUMP_CHEST_WOODEN, 10, 10, 170, 115 },
        //{ GUMP_PILLOW_HEART, 0, 0, 0, 0 },		// TO-DO: confirm gump size
        { GUMP_CHEST_METAL_LARGE, 50, 60, 500, 300 },
		{ GUMP_CHEST_METAL_GOLD_LARGE, 50, 60, 500, 300 },
		{ GUMP_CHEST_WOOD_LARGE, 50, 60, 500, 300 },
		{ GUMP_CHEST_CRATE_LARGE, 50, 60, 500, 300 },
		{ GUMP_MINERS_SATCHEL, 44, 65, 186, 159 },
		{ GUMP_LUMBERJACKS_SATCHEL, 44, 65, 186, 159 },
		{ GUMP_MAILBOX_WOOD, 17, 63, 116, 153 },
		{ GUMP_MAILBOX_BIRD, 18, 68, 117, 154 },
		{ GUMP_MAULBOX_IRON, 19, 81, 117, 152 },
		{ GUMP_MAILBOX_GOLDEN, 23, 65, 112, 155 },
        { GUMP_CHEST_METAL2, 18, 105, 162, 178 }
	};

	// Get a random location in the container.

	const CItemBase *pItemDef = Item_GetDef();
	GUMP_TYPE gump = pItemDef->m_ttContainer.m_idGump;	// Get the TDATA2

	// check for custom values in TDATA3/TDATA4
	if ( pItemDef->m_ttContainer.m_dwMinXY || pItemDef->m_ttContainer.m_dwMaxXY )
	{
		int tmp_MinX = pItemDef->m_ttContainer.m_dwMinXY >> 16;
		int tmp_MinY = (pItemDef->m_ttContainer.m_dwMinXY & 0x0000FFFF);
		int tmp_MaxX = pItemDef->m_ttContainer.m_dwMaxXY >> 16;
		int tmp_MaxY = (pItemDef->m_ttContainer.m_dwMaxXY & 0x0000FFFF);
		//DEBUG_WARN(("Custom container gump id %d for 0%x\n", gump, GetDispID()));
		return CPointMap(
			(word)(tmp_MinX + Calc_GetRandVal(tmp_MaxX - tmp_MinX)),
			(word)(tmp_MinY + Calc_GetRandVal(tmp_MaxY - tmp_MinY)),
			0);
	}

	// No TDATA3 or no TDATA4: check if we have hardcoded in sm_ContSize the size of the gump indicated by TDATA2

	uint i = 0;
	// We may want a keyring with no gump, so no need to show the warning.
	if (!IsType(IT_KEYRING))
	{
		for (; ; ++i)
		{
			if (i >= CountOf(sm_ContSize))
			{
				i = 0;	// set to default
				g_Log.EventWarn("Unknown container gump id %d for 0%x\n", gump, GetDispID());
				break;
			}
			if (sm_ContSize[i].m_gump == gump)
				break;
		}
	}

	const int iRandOnce = (int)Calc_GetRandVal(UINT16_MAX);
	return {
		(short)(sm_ContSize[i].m_minx + (iRandOnce % (sm_ContSize[i].m_maxx - sm_ContSize[i].m_minx))),
		(short)(sm_ContSize[i].m_miny + (iRandOnce % (sm_ContSize[i].m_maxy - sm_ContSize[i].m_miny))),
		0 };
}

void CItemContainer::ContentAdd( CItem *pItem, CPointMap pt, bool bForceNoStack, uchar gridIndex )
{
	ADDTOCALLSTACK("CItemContainer::ContentAdd");
	// Add to CItemContainer
	if ( !pItem )
		return;
	if ( pItem == this )
		return;	// infinite loop.

	if ( !g_Serv.IsLoading() )
	{
        switch (GetType())
        {
            case IT_EQ_TRADE_WINDOW:
                Trade_Status(false);
                break;

            default:
                break;
        }

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
				pCont->ClearContainer();
				break;
			}
            default:
                break;
		}
	}

	// check for custom values in TDATA3/TDATA4
	CItemBase *pContDef = Item_GetDef();
	if (pContDef->m_ttContainer.m_dwMinXY || pContDef->m_ttContainer.m_dwMaxXY)
	{
		const short tmp_MinX = (short)( pContDef->m_ttContainer.m_dwMinXY >> 16 );
        const short tmp_MinY = (short)( (pContDef->m_ttContainer.m_dwMinXY & 0x0000FFFF) );
        const short tmp_MaxX = (short)( pContDef->m_ttContainer.m_dwMaxXY >> 16 );
        const short tmp_MaxY = (short)( (pContDef->m_ttContainer.m_dwMaxXY & 0x0000FFFF) );
		if (pt.m_x < tmp_MinX)
			pt.m_x = tmp_MinX;
		if (pt.m_x > tmp_MaxX)
			pt.m_x = tmp_MaxX;
		if (pt.m_y < tmp_MinY)
			pt.m_y = tmp_MinY;
		if (pt.m_y > tmp_MaxY)
			pt.m_y = tmp_MaxY;
	}

    bool fStackInsert = false;
	if ( pt.m_x <= 0 || pt.m_y <= 0 || pt.m_x > 512 || pt.m_y > 512 )	// invalid container location ?
	{
		// Try to stack it.
		if ( !g_Serv.IsLoading() && pItem->Item_GetDef()->IsStackableType() && !bForceNoStack )
		{
			for (CSObjContRec* pObjRec : *this)
			{
				CItem* pTry = static_cast<CItem*>(pObjRec);
				pt = pTry->GetContainedPoint();
				if ( pItem->Stack(pTry) )
				{
                    fStackInsert = true;
					break;
				}
			}
		}
		if ( !fStackInsert)
			pt = GetRandContainerLoc();
	}

    // Try drop it on given container grid index (if not available, drop it on next free index)
	{
		bool fGridCellUsed[UCHAR_MAX] {false};
		for (const CSObjContRec* pObjRec : *this)
		{
			const CItem* pTry = static_cast<const CItem*>(pObjRec);
			const auto idxGridTest = pTry->GetContainedGridIndex();

			static_assert(sizeof(idxGridTest) == sizeof(uchar));
			fGridCellUsed[idxGridTest] = true;
		}

		static_assert(sizeof(gridIndex) == sizeof(uchar));
		if (fGridCellUsed[gridIndex])
		{
			gridIndex = 0;
			for (uint i = 0; i < UCHAR_MAX; ++i)
			{
				if (!fGridCellUsed[i])
					break;

				if (++gridIndex >= g_Cfg.m_iContainerMaxItems)
				{
					gridIndex = 0;
					break;
				}
			}
		}
	}

	CContainer::ContentAddPrivate(pItem);
	pItem->SetContainedPoint(pt);
	pItem->SetContainedGridIndex(gridIndex);

	switch ( GetType() )
	{
		case IT_KEYRING: // empty key ring.
			SetKeyRing();
			break;
		case IT_EQ_VENDOR_BOX:
			/*
            if ( !IsItemEquipped() )	// vendor boxes should ALWAYS be equipped! Unless we're duping the container, in this case first we dupe (and so the items inside), then we equip it
			{
				DEBUG_ERR(("Un-equipped vendor box uid=0%x is bad\n", (dword)GetUID()));
				break;
			}*/
			{
				CItemVendable *pItemVend = dynamic_cast<CItemVendable *>(pItem);
				if ( !pItemVend )
				{
					g_Log.Event(LOGL_WARN, "Vendor: deleting non-vendable item %s uid=0%x, vendor: %s, uid=0%x\n", pItem->GetResourceName(), pItem->GetUID().GetObjUID(), GetContainer()->GetName(), GetContainer()->GetUID().GetObjUID());
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
    if (!fStackInsert)
        pItem->UpdatePropertyFlag();
    // UpdatePropertyFlag for this item is called by CContainer::ContentAddPrivate -> CItemContainer::OnWeightChange
}

void CItemContainer::ContentAdd( CItem *pItem, bool bForceNoStack )
{
	ADDTOCALLSTACK("CItemContainer::ContentAdd");
	if ( !pItem )
		return;
	if ( pItem->GetParent() == this )
		return;	// already here.

	CPointMap pt;	// invalid point.
	if ( g_Serv.IsLoading() )
		pt = pItem->GetUnkPoint();

	ContentAdd(pItem, pt, bForceNoStack);
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

void CItemContainer::OnRemoveObj( CSObjContRec *pObjRec )	// Override this = called when removed from list.
{
	ADDTOCALLSTACK("CItemContainer::OnRemoveObj");
	// remove this object from the container list.
	CItem *pItem = static_cast<CItem *>(pObjRec);
	ASSERT(pItem);
	if ( IsType(IT_EQ_TRADE_WINDOW) )
	{
		// Remove from a trade window.
		Trade_Status(false);
	}
	if ( IsType(IT_EQ_VENDOR_BOX) && IsItemEquipped() )	// vendor boxes should ALWAYS be equipped !
	{
		CItemVendable *pItemVend = dynamic_cast<CItemVendable *>(pItem);
		if ( pItemVend )
			pItemVend->SetPlayerVendorPrice(0);
	}
	CContainer::OnRemoveObj(pObjRec);
    UpdatePropertyFlag();

	if ( IsType(IT_KEYRING) )	// key ring.
		SetKeyRing();
    pItem->GoAwake();
}

void CItemContainer::_GoAwake()
{
	ADDTOCALLSTACK("CItemContainer::_GoAwake");

	CItem::_GoAwake();
	CContainer::_GoAwake();	// This method isn't virtual
}

void CItemContainer::_GoSleep()
{
	ADDTOCALLSTACK("CItemContainer::_GoSleep");

	CContainer::_GoSleep(); // This method isn't virtual
	CItem::_GoSleep();
}

void CItemContainer::DupeCopy( const CItem *pItem )
{
	ADDTOCALLSTACK("CItemContainer::DupeCopy");
	// Copy the contents of this item.

	CItemVendable::DupeCopy(pItem);

	const CItemContainer *pContItem = dynamic_cast<const CItemContainer *>(pItem);
	if ( !pContItem )
		return;

	for (const CSObjContRec* pObjRec : *pContItem)
	{
		const CItem* pContent = static_cast<const CItem*>(pObjRec);
		ContentAdd(CreateDupeItem(pContent), pContent->GetContainedPoint());
	}
}

void CItemContainer::MakeKey()
{
	ADDTOCALLSTACK("CItemContainer::MakeKey");
	SetType(IT_CONTAINER);
	m_itContainer.m_UIDLock = GetUID();
	m_itContainer.m_dwLockComplexity = 500 + Calc_GetRandVal(600);

	CItem *pKey = CreateScript(ITEMID_KEY_COPPER);
	ASSERT(pKey);
	pKey->m_itKey.m_UIDLock = GetUID();
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

	size_t iQty = GetContentCount();
	if ( iQty >= CountOf(sm_Item_Keyrings) )
		iQty = CountOf(sm_Item_Keyrings) - 1;

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

	size_t pTagTmp = (size_t)(GetKeyNum("OVERRIDE.MAXITEMS"));
	size_t tMaxItemsCont = pTagTmp ? pTagTmp : g_Cfg.m_iContainerMaxItems;
	if ( GetContentCount() >= tMaxItemsCont )
	{
		pCharMsg->SysMessageDefault(DEFMSG_CONT_FULL);
		return false;
	}

	if (m_ModMaxWeight)
	{
		if ( (GetTotalWeight() + pItem->GetWeight()) > m_ModMaxWeight )
		{
			pCharMsg->SysMessageDefault(DEFMSG_CONT_FULL_WEIGHT);
			return false;
		}
	}

	if ( !IsItemEquipped() &&	// does not apply to my pack.
		pItem->IsContainer() &&
		(pItem->Item_GetDef()->GetVolume() >= Item_GetDef()->GetVolume()) )
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
				size_t iItemsInContainer = 0;
				const CItemContainer *pContItem = dynamic_cast<const CItemContainer *>(pItem);
				if ( pContItem )
					iItemsInContainer = pContItem->ContentCountAll();

				// Check the total number of items in the bankbox and the ev.
				// container put into it.
				if ( (ContentCountAll() + iItemsInContainer) > (size_t)iBankIMax )
				{
					pCharMsg->SysMessageDefault(DEFMSG_BVBOX_FULL_ITEMS);
					return false;
				}
			}

			int iBankWMax = g_Cfg.m_iBankWMax;
			iBankWMax += m_ModMaxWeight * WEIGHT_UNITS;

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
			if ( !pItem->m_itKey.m_UIDLock.IsValidUID())
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
			if ( GetContentCount() >= g_Cfg.m_iContainerMaxItems )
			{
				pCharMsg->SysMessageDefault(DEFMSG_CONT_FULL);
				return false;
			}
			break;

		case IT_TRASH_CAN:
			Sound(0x235); // a little sound so we know it "ate" it.
			pCharMsg->SysMessageDefault(DEFMSG_ITEMUSE_TRASHCAN);
			_SetTimeoutS(15);
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
		if ( pChar && !pChar->IsStatFlag(STATF_PET) )
		{
			switch ( GetEquipLayer() )
			{
				case LAYER_VENDOR_STOCK:
					// Magic restock the vendors container.
				{
					for (CSObjContRec* pObjRec : *this)
					{
						CItem* pItem = static_cast<CItem*>(pObjRec);
						CItemVendable *pVendItem = dynamic_cast<CItemVendable *>(pItem);
						if ( pVendItem )
							pVendItem->Restock(true);
					}
				}
				break;

				case LAYER_VENDOR_EXTRA:
					// clear all this junk periodically.
					// sell it back for cash value ?
					ClearContainer();
					break;

				case LAYER_VENDOR_BUYS:
				{
					// Reset what we will buy from players.
					for (CSObjContRec* pObjRec : *this)
					{
						CItem* pItem = static_cast<CItem*>(pObjRec);
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
			snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_BVBOX_OPEN_SELF), iStones, GetName());
		else
			snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_BVBOX_OPEN_OTHER), pCharTop->GetPronoun(), iStones, pCharTop->GetPossessPronoun(), GetName());

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

	if ( !IsContainerEmpty() )
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
		for ( size_t i = 0; i < CountOf(sm_Item_ChessPieces); ++i )
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
		for ( int i = 0; i < 24; ++i )
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
	nullptr
};

bool CItemContainer::r_Verb( CScript &s, CTextConsole *pSrc )
{
	ADDTOCALLSTACK("CItemContainer::r_Verb");
	EXC_TRY("Verb");
	ASSERT(pSrc);
	switch ( FindTableSorted(s.GetKey(), sm_szVerbKeys, CountOf(sm_szVerbKeys) - 1) )
	{
		case ICV_DELETE:
			if ( s.HasArgs() )
			{
				// 1 based pages.
				size_t index = s.GetArgVal();
				if ( index > 0 && index <= GetContentCount() )
				{
					CItem *pItem = static_cast<CItem*>(GetContentIndex(index - 1));
					ASSERT(pItem);
					pItem->Delete();
					return true;
				}
			}
			return false;
		case ICV_EMPTY:
		{
			ClearContainer();
			return true;
		}
		case ICV_FIXWEIGHT:
			FixWeight();
			return true;
		case ICV_OPEN:
			if ( pSrc->GetChar() )
			{
				CChar *pChar = pSrc->GetChar();
				if ( pChar->IsClientActive() )
				{
					CClient *pClient = pChar->GetClientActive();
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
				if ( pChar->IsClientActive() )
				{
					CClient *pClient = pChar->GetClientActive();
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

bool CItemContainer::_OnTick()
{
	ADDTOCALLSTACK("CItemContainer::_OnTick");
	// equipped or not.
	switch ( GetType() )
	{
		case IT_TRASH_CAN:
			// Empty it !
			ClearContainer();
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
			_SetTimeout(-1);
			return true;
		default:
			break;
	}
	return CItemVendable::_OnTick();
}
