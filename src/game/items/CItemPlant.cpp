
#include "../chars/CChar.h"
#include "../CWorldMap.h"
#include "CItem.h"

void CItem::Plant_SetTimer()
{
	ADDTOCALLSTACK("CItem::Plant_SetTimer");
	_SetTimeout(GetDecayTime());
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

	const CItemBase* pItemDef = Item_GetDef();

	ITEMID_TYPE iGrowID = (ITEMID_TYPE)pItemDef->m_ttCrops.m_ridGrow.GetResIndex();
	ITEMID_TYPE iFruitIDOverride = (ITEMID_TYPE)m_itCrop.m_ridFruitOverride.GetResIndex();
	if ( (iGrowID == ITEMID_NOTHING) && (iFruitIDOverride != ITEMID_NOTHING) )	// If we set an override, we can reap this at every stage
	{
		// not ripe. (but we could just eat it if we are herbivorous ?)
		pChar->SysMessageDefault(DEFMSG_CROPS_NOT_RIPE);
		return true;
	}

	ITEMID_TYPE iFruitID = ITEMID_NOTHING;
	if ( iFruitIDOverride != ITEMID_NOTHING )
		iFruitID = iFruitIDOverride;
	else
		iFruitID = (ITEMID_TYPE)pItemDef->m_ttCrops.m_ridFruit.GetResIndex();
	
	if ( iFruitID == ITEMID_NOTHING )
		pChar->SysMessageDefault(DEFMSG_CROPS_NO_FRUIT);
	else
	{
		CItem *pItemFruit = CItem::CreateScript(iFruitID, pChar);
		if ( pItemFruit )
			pChar->ItemBounce(pItemFruit);
	}	

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
	ITEMID_TYPE iGrowID = (ITEMID_TYPE)pItemDef->m_ttCrops.m_ridGrow.GetResIndex();

	if ( iGrowID == RES_INDEX_MASK ) // if TDATA2 == -1
	{
		// Some plants generate a fruit on the ground when ripe.
		ITEMID_TYPE iFruitID = ITEMID_NOTHING;
		if ( m_itCrop.m_ridFruitOverride.IsValidUID())
			iFruitID = (ITEMID_TYPE)m_itCrop.m_ridFruitOverride.GetResIndex();
		else
			iFruitID = (ITEMID_TYPE)pItemDef->m_ttCrops.m_ridFruit.GetResIndex();
		
		if (iFruitID != ITEMID_NOTHING)
		{
			// Put a fruit on the ground if not already here.
			CWorldSearch AreaItems(GetTopPoint());
			for (;;)
			{
				CItem *pItem = AreaItems.GetItem();
				if ( !pItem )
				{
					CItem *pItemFruit = CItem::CreateScript(iFruitID);
					ASSERT(pItemFruit);
					pItemFruit->MoveToDecay(GetTopPoint(), g_Cfg.m_iDecay_Item);
					break;
				}
				else if ( pItem->IsType(IT_FRUIT) || pItem->IsType(IT_REAGENT_RAW) )
					break;
			}
		}

		Plant_CropReset();
		return true;
	}
	else if ( iGrowID )
	{
		SetID(iGrowID);
		Update();
	}

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
	ITEMID_TYPE iResetID = (ITEMID_TYPE)pItemDef->m_ttCrops.m_ridReset.GetResIndex();
	if ( iResetID != ITEMID_NOTHING )
		SetID(iResetID);

	Plant_SetTimer();
	RemoveFromView();		// remove from most screens.
	SetHue(HUE_RED_DARK);	// Indicate to GM's that it is growing.
	SetAttr(ATTR_INVIS);	// regrown invis.
    Update();
}