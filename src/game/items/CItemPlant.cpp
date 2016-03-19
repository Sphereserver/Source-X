
#include "../chars/CChar.h"
#include "CItem.h"

void CItem::Plant_SetTimer()
{
	ADDTOCALLSTACK("CItem::Plant_SetTimer");
	SetTimeout(GetDecayTime());
}

// Pick cotton/hay/etc...
// use the
//  IT_CROPS = transforms into a "ripe" variety then is used up on reaping.
//  IT_FOLIAGE = is not consumed on reap (unless eaten then will regrow invis)
//
bool CItem::Plant_Use(CChar *pChar)
{
	ADDTOCALLSTACK("CItem::Plant_Use");

	if ( !pChar )
		return false;
	if ( !pChar->CanSeeItem(this) )		// might be invis underground
		return false;

	const CItemBase *pItemDef = Item_GetDef();
	ITEMID_TYPE iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttCrops.m_idFruit));	// if it's reapable at this stage
	if ( iFruitID <= 0 )
	{
		// not ripe. (but we could just eat it if we are herbivorous ?)
		pChar->SysMessageDefault(DEFMSG_CROPS_NOT_RIPE);
		return true;
	}

	if ( m_itCrop.m_ReapFruitID )
		iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(m_itCrop.m_ReapFruitID));
	if ( iFruitID )
	{
		CItem *pItemFruit = CItem::CreateScript(iFruitID, pChar);
		if ( pItemFruit )
			pChar->ItemBounce(pItemFruit);
	}
	else
		pChar->SysMessageDefault(DEFMSG_CROPS_NO_FRUIT);

	Plant_CropReset();
	pChar->UpdateAnimate(ANIM_BOW);
	pChar->Sound(0x13e);
	return true;
}

// timer expired, should I grow?
bool CItem::Plant_OnTick()
{
	ADDTOCALLSTACK("CItem::Plant_OnTick");
	ASSERT(IsType(IT_CROPS) || IsType(IT_FOLIAGE));
	// If it is in a container, kill it.
	if ( !IsTopLevel() )
		return false;

	// Make sure the darn thing isn't moveable
	SetAttr(ATTR_MOVE_NEVER);
	Plant_SetTimer();

	// No tree stuff below here
	if ( IsAttr(ATTR_INVIS) )	// if it's invis, take care of it here and return
	{
		SetHue(HUE_DEFAULT);
		ClrAttr(ATTR_INVIS);
		Update();
		return true;
	}

	const CItemBase *pItemDef = Item_GetDef();
	ITEMID_TYPE iGrowID = pItemDef->m_ttCrops.m_idGrow;

	if ( iGrowID == -1 )
	{
		// Some plants generate a fruit on the ground when ripe.
		ITEMID_TYPE iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttCrops.m_idGrow));
		if ( m_itCrop.m_ReapFruitID )
			iFruitID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(m_itCrop.m_ReapFruitID));
		if ( !iFruitID )
			return true;

		// Put a fruit on the ground if not already here.
		CWorldSearch AreaItems(GetTopPoint());
		for (;;)
		{
			CItem *pItem = AreaItems.GetItem();
			if ( !pItem )
			{
				CItem *pItemFruit = CItem::CreateScript(iFruitID);
				ASSERT(pItemFruit);
				pItemFruit->MoveToDecay(GetTopPoint(), 10 * g_Cfg.m_iDecay_Item);
				break;
			}
			if ( pItem->IsType(IT_FRUIT) || pItem->IsType(IT_REAGENT_RAW) )
				break;
		}

		// NOTE: ??? The plant should cycle here as well !
		iGrowID = pItemDef->m_ttCrops.m_idReset;
	}

	if ( iGrowID )
	{
		SetID(static_cast<ITEMID_TYPE>(RES_GET_INDEX(iGrowID)));
		Update();
		return true;
	}

	// some plants go dormant again ?
	// m_itCrop.m_Fruit_ID = iTemp;
	return true;
}

// Animals will eat crops before they are ripe, so we need a way to reset them prematurely
void CItem::Plant_CropReset()
{
	ADDTOCALLSTACK("CItem::Plant_CropReset");

	if ( !IsType(IT_CROPS) && !IsType(IT_FOLIAGE) )
	{
		// This isn't a crop, and since it just got eaten, we should delete it
		Delete();
		return;
	}

	const CItemBase *pItemDef = Item_GetDef();
	ITEMID_TYPE iResetID = static_cast<ITEMID_TYPE>(RES_GET_INDEX(pItemDef->m_ttCrops.m_idReset));
	if ( iResetID != ITEMID_NOTHING )
		SetID(iResetID);

	Plant_SetTimer();
	RemoveFromView();		// remove from most screens.
	SetHue(HUE_RED_DARK);	// Indicate to GM's that it is growing.
	SetAttr(ATTR_INVIS);	// regrown invis.
}