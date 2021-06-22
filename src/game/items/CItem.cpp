
#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../../network/CClientIterator.h"
#include "../../network/send.h"
#include "../../sphere/ProfileTask.h"
#include "../components/CCChampion.h"
#include "../components/CCItemDamageable.h"
#include "../components/CCSpawn.h"
#include "../components/CCPropsItem.h"
#include "../components/CCPropsItemChar.h"
#include "../components/CCPropsItemEquippable.h"
#include "../components/CCPropsItemWeapon.h"
#include "../components/CCPropsItemWeaponRanged.h"
#include "../chars/CChar.h"
#include "../chars/CCharNPC.h"
#include "../clients/CClient.h"
#include "../CSector.h"
#include "../CServer.h"
#include "../CWorld.h"
#include "../CWorldTickingList.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../triggers.h"
#include "CItem.h"
#include "CItemCommCrystal.h"
#include "CItemContainer.h"
#include "CItemCorpse.h"
#include "CItemMap.h"
#include "CItemMemory.h"
#include "CItemMessage.h"
#include "CItemMultiCustom.h"
#include "CItemScript.h"
#include "CItemShip.h"
#include "CItemStone.h"
#include "CItemVendable.h"


/*
	If you add a new trigger here, be sure to also and ALWAYS add a corresponding
	character trigger (even if it does not make sense)! CTRIG_item* will be automatically
	called when one of these triggers fired, depending on their index. So NEVER unalign the two lists!
*/
lpctstr const CItem::sm_szTrigName[ITRIG_QTY+1] =	// static
{
	"@AAAUNUSED",
	"@AddRedCandle",
	"@AddWhiteCandle",
	"@AfterClick",
	"@Buy",
	"@CarveCorpse",			//I am a corpse and i am going to be carved.
	"@Click",
	"@ClientTooltip",	// Sending tooltip to a client
	"@ClientTooltip_AfterDefault",
	"@ContextMenuRequest",
	"@ContextMenuSelect",
	"@Create",
	"@DAMAGE",				// I have been damaged in some way
	"@DCLICK",				// I have been dclicked.
	"@Destroy",				//+I am nearly destroyed
	"@DropOn_Char",			// I have been dropped on this char
	"@DropOn_Ground",		// I have been dropped on the ground here
	"@DropOn_Item",			// I have been dropped on this item
	"@DropOn_Self",			// An item has been dropped here
	"@DropOn_Trade",
	"@Dye",					// My color has been changed
	"@EQUIP",		// I have been unequipped
    "@EQUIPTEST",
	"@MemoryEquip",
	"@PICKUP_GROUND",	// I was picked up off the ground.
	"@PICKUP_PACK",	// picked up from inside some container.
	"@PICKUP_SELF", // picked up from here
	"@PICKUP_STACK",	// was picked up from a stack
	"@PreSpawn",
    "@Redeed",
    "@RegionEnter",
    "@RegionLeave",
	"@SELL",
	"@Ship_Turn",
	"@Smelt",			// I am going to be smelted.
	"@Spawn",
	"@SpellEffect",		// cast some spell on me.
	"@Start",
	"@STEP",			// I have been walked on.
	"@TARGON_CANCEL",
	"@TARGON_CHAR",
	"@TARGON_GROUND",
	"@TARGON_ITEM",	// I am being combined with an item
	"@TIMER",		// My timer has expired.
	"@ToolTip",
	"@UNEQUIP",		// i have been unequipped (or try to unequip)
	nullptr
};

/////////////////////////////////////////////////////////////////
// -CItem

CUID CItem::GetComponentOfMulti() const	// I'm a CMultiComponent of a CMulti
{
	if (CVarDefCont* pVarDef = m_TagDefs.GetKey("MultiComponent"))
		return CUID((dword)pVarDef->GetValNum());
	return CUID();
}

CUID CItem::GetLockDownOfMulti() const	// I'm locked down in a CMulti
{
	if (CVarDefCont* pVarDef = m_TagDefs.GetKey("MultiLockDown"))
		return CUID((dword)pVarDef->GetValNum());
	return CUID();
}

void CItem::SetComponentOfMulti(const CUID& uidMulti)
{
	if (!uidMulti.IsValidUID())
		m_TagDefs.DeleteKey("MultiComponent");
	else
		m_TagDefs.SetNum("MultiComponent", uidMulti.GetObjUID(), false, false);
}

void CItem::SetLockDownOfMulti(const CUID& uidMulti)
{
	if (!uidMulti.IsValidUID())
		m_TagDefs.DeleteKey("MultiLockDown");
	else
		m_TagDefs.SetNum("MultiLockDown", uidMulti.GetObjUID(), false, false);
}

CItem::CItem( ITEMID_TYPE id, CItemBase * pItemDef ) :
	CTimedObject(PROFILE_ITEMS),
	CObjBase( true )
{
	ASSERT( pItemDef );

	g_Serv.StatInc(SERV_STAT_ITEMS);
    m_type = IT_NORMAL;
	m_Attr = 0;
	m_CanUse = pItemDef->m_CanUse;
	m_wAmount = 1;
	m_containedGridIndex = 0;
	m_dwDispIndex = ITEMID_NOTHING;


	m_itNormal.m_more1 = 0;
	m_itNormal.m_more2 = 0;
	m_itNormal.m_morep.ZeroPoint();

	SetBase( pItemDef );
	SetDispID( id );

	g_World.m_uidLastNewItem = GetUID();	// for script access.
	ASSERT( IsDisconnected() );


    // Manual CComponents addition

    /* CCItemDamageable is also added from CObjBase::r_LoadVal(OC_CANMASK) for manual override of can flags
    * but it's required to add it also on item's creation depending on it's CItemBase can flags.
    */
	if (CCItemDamageable::CanSubscribe(this))
    {
        SubscribeComponent(new CCItemDamageable(this));
    }
    if (CCFaction::CanSubscribe(this))
    {
        SubscribeComponent(new CCFaction());  // Adding it only to equippable items
    }

	TrySubscribeComponentProps<CCPropsItem>();
	TrySubscribeComponentProps<CCPropsItemChar>();
}

void CItem::DeleteCleanup(bool fForce)
{
	ADDTOCALLSTACK("CItem::DeleteCleanup");
	_fDeleting = true;

	// We don't want to have invalid pointers over there
	// Already called by CObjBase::DeletePrepare -> CObjBase::_GoSleep
	//CWorldTickingList::DelObjSingle(this);
	//CWorldTickingList::DelObjStatusUpdate(this, false);

	// Remove corpse map waypoint on enhanced clients
	if (IsType(IT_CORPSE) && m_uidLink.IsValidUID())
	{
		CChar* pChar = m_uidLink.CharFind();
		if (pChar && pChar->GetClientActive())
		{
			pChar->GetClientActive()->addMapWaypoint(this, MAPWAYPOINT_Remove);
		}
	}

	switch ( m_type )
	{
		case IT_FIGURINE:
		case IT_EQ_HORSE:
			{	// remove the ridden or linked char.
				CChar * pHorse = m_itFigurine.m_UID.CharFind();
				if ( pHorse && pHorse->IsDisconnected() && ! pHorse->m_pPlayer )
				{
                    pHorse->m_atRidden.m_uidFigurine.InitUID();
					pHorse->Delete(fForce);
				}
			}
			break;
		default:
			break;
	}

	CUID uidTest(GetComponentOfMulti());
    if (uidTest.IsValidUID())
    {
        if (auto* pMulti = static_cast<CItemMulti*>(uidTest.ItemFind(true)))
        {
            pMulti->DeleteComponent(GetUID(), true);
        }
    }
	uidTest = GetLockDownOfMulti();
    if (uidTest.IsValidUID())
    {
		if (auto* pMulti = static_cast<CItemMulti*>(uidTest.ItemFind(true)))
        {
            pMulti->UnlockItem(GetUID(), true);
        }
    }
}

bool CItem::NotifyDelete()
{
	ADDTOCALLSTACK("CItem::NotifyDelete");
	if ((IsTrigUsed(TRIGGER_DESTROY)) || (IsTrigUsed(TRIGGER_ITEMDESTROY)))
	{
		if (CItem::OnTrigger(ITRIG_DESTROY, &g_Serv) == TRIGRET_RET_TRUE)
			return false;
	}

	return true;
}

bool CItem::Delete(bool fForce)
{
	ADDTOCALLSTACK("CItem::Delete");
	if (( NotifyDelete() == false ) && !fForce)
		return false;

	DeletePrepare();	// Virtual -> Must remove early because virtuals will fail in child destructor.
	DeleteCleanup(fForce);

	return CObjBase::Delete(fForce);
}

CItem::~CItem()
{
	EXC_TRY("Cleanup in destructor");
	ADDTOCALLSTACK("CItem::~CItem");

	DeletePrepare();	// Using this in the destructor will fail to call virtuals, but it's better than nothing.
	CItem::DeleteCleanup(true);
	
	g_Serv.StatDec(SERV_STAT_ITEMS);

	EXC_CATCH;
}

CItem * CItem::CreateBase( ITEMID_TYPE id, IT_TYPE type )	// static
{
	ADDTOCALLSTACK("CItem::CreateBase");
	// All CItem creates come thru here.
	// NOTE: This is a free floating item.
	//  If not put on ground or in container after this = Memory leak !

	ITEMID_TYPE idErrorMsg = ITEMID_NOTHING;
	CItemBase * pItemDef = CItemBase::FindItemBase( id );
	if ( pItemDef == nullptr )
	{
		idErrorMsg = id;
		id = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, "DEFAULTITEM" ));
		if ( id <= 0 )
			id = ITEMID_GOLD_C1;

		pItemDef = CItemBase::FindItemBase( id );
		ASSERT(pItemDef);
	}

	CItem * pItem = nullptr;
    if (type == IT_INVALID)
        type = pItemDef->GetType();
	switch (type)
	{
		case IT_MAP:
		case IT_MAP_BLANK:
			pItem = new CItemMap( id, pItemDef );
			break;
		case IT_COMM_CRYSTAL:
			pItem = new CItemCommCrystal( id, pItemDef );
			break;
		case IT_GAME_BOARD:
		case IT_BBOARD:
		case IT_TRASH_CAN:
		case IT_CONTAINER_LOCKED:
		case IT_CONTAINER:
		case IT_EQ_TRADE_WINDOW:
		case IT_EQ_VENDOR_BOX:
		case IT_EQ_BANK_BOX:
		case IT_KEYRING:
		case IT_SHIP_HOLD_LOCK:
		case IT_SHIP_HOLD:
			pItem = new CItemContainer( id, pItemDef );
			break;
		case IT_CORPSE:
			pItem = new CItemCorpse( id, pItemDef );
			break;
		case IT_MESSAGE:
		case IT_BOOK:
			// A message for a bboard or book text.
			pItem = new CItemMessage( id, pItemDef );
			break;
		case IT_STONE_GUILD:
		case IT_STONE_TOWN:
			pItem = new CItemStone( id, pItemDef );
			break;
		case IT_MULTI:
        case IT_MULTI_ADDON:
			pItem = new CItemMulti( id, pItemDef );
			break;
		case IT_MULTI_CUSTOM:
			pItem = new CItemMultiCustom( id, pItemDef );
			break;
		case IT_SHIP:
			pItem = new CItemShip( id, pItemDef );
			break;
		case IT_EQ_MEMORY_OBJ:
			pItem = new CItemMemory( id, pItemDef );
			break;
		case IT_EQ_SCRIPT:	// pure script.with TAG(s) support
		case IT_SCRIPT:
			pItem = new CItemScript( id, pItemDef );
			break;
		default:
        {
            if (pItemDef->GetMakeValue(0))
                pItem = new CItemVendable(id, pItemDef);
            else
            {
                pItem = new CItem(id, pItemDef);
            }
            break;
        }
	}

    /*
    * Post-type CComponents check in SetType.
    * In the future, when all CItem* classes are moved to CComponents all will be passed
    * from here and the first switch can be removed, and so this comment.
    */
    ASSERT(pItem);
    pItem->SetType(type, false);

	if (idErrorMsg && idErrorMsg != -1)
		DEBUG_ERR(("CreateBase invalid item ID=0%" PRIx32 ", defaulting to ID=0%" PRIx32 ". Created UID=0%" PRIx32 "\n", idErrorMsg, id, (dword)pItem->GetUID()));

	return pItem;
}

CItem * CItem::CreateDupeItem( const CItem * pItem, CChar * pSrc, bool fSetNew )	// static
{
	ADDTOCALLSTACK("CItem::CreateDupeItem");
	// Dupe this item.
	if ( pItem == nullptr )
		return nullptr;
	CItem * pItemNew = CreateBase( pItem->GetID(), pItem->GetType() );
	ASSERT(pItemNew);
	pItemNew->DupeCopy( pItem );

	if ( pSrc )
		pSrc->m_Act_UID = pItemNew->GetUID();

	if ( fSetNew )
		g_World.m_uidNew = pItemNew->GetUID();

	return pItemNew;
}

CItem * CItem::CreateScript(ITEMID_TYPE id, CChar * pSrc, IT_TYPE type) // static
{
	ADDTOCALLSTACK("CItem::CreateScript");
	// Create item from the script id.

	CItem * pItem = CreateBase(id, type);
	ASSERT(pItem);
	pItem->GenerateScript(pSrc);
	return pItem;
}

CItem * CItem::GenerateScript( CChar * pSrc)
{
	// Calls to @Create and @ItemCreate
	// and some other default stuff for the item.
	switch ( GetType())
	{
		case IT_CONTAINER_LOCKED:
			{
				// At this level it just means to add a key for it.
				CItemContainer * pCont = dynamic_cast <CItemContainer *> ( this );
				ASSERT(pCont);
				pCont->MakeKey();
			}
			break;
		case IT_CORPSE:
			{
				//	initialize TAG.BLOOD as the amount of blood inside
				CItemBase	*pItemDef = Item_GetDef();
				int iBlood = 0;
				if ( pItemDef )
				{
					iBlood = (int)(pItemDef->m_TagDefs.GetKeyNum("MAXBLOOD"));
				}
				if ( !iBlood )
					iBlood = 5;
				else if ( iBlood < 0 )
					iBlood = 0;
				m_TagDefs.SetNum("BLOOD", iBlood, true);
			}
			break;
		default:
			break;
	}

	// call the ON=@Create trigger
	CItemBase * pItemDef = Item_GetDef();
	ASSERT( pItemDef );

	CResourceLock s;
	if ( pItemDef->ResourceLock(s))
	{
        OnTrigger(ITRIG_Create, pSrc ? static_cast<CTextConsole*>(pSrc) : static_cast<CTextConsole*>(&g_Serv), nullptr);
	}
	return this;
}

CItem * CItem::CreateHeader( tchar * pArg, CObjBase * pCont, bool fDupeCheck, CChar * pSrc )
{
	ADDTOCALLSTACK("CItem::CreateHeader");
	// Just read info on a single item carryed by a CChar.
	// ITEM=#id,#amount,R#chance

    tchar * pptcCmd[3];
    int iQty = Str_ParseCmds(pArg, pptcCmd, CountOf(pptcCmd), ",");
    if (iQty < 1)
        return nullptr;

	word amount = 1;
    for (int i = 1; i <= (iQty - 1); ++i)
    {
        if ( pptcCmd[i][0] != 'R' )
        {
            amount = Exp_GetWVal( pptcCmd[i] );
        }
        else if ( pptcCmd[i][0] == 'R' )
        {
            // 1 in x chance of creating this.
            if ( Calc_GetRandVal( atoi(pptcCmd[i] + 1) ))
                return nullptr;	// don't create it
        }
    }

	if ( amount == 0 )
		return nullptr;

    const CResourceID rid = g_Cfg.ResourceGetID(RES_ITEMDEF, pptcCmd[0]);
    if ( ! rid.IsValidUID() )
        return nullptr;
    const RES_TYPE iResType = rid.GetResType();
    if ( (iResType != RES_ITEMDEF) && (iResType != RES_TEMPLATE) )
        return nullptr;
    const int iResIndex = rid.GetResIndex();
    if ( iResIndex == 0 )
        return nullptr;

	const ITEMID_TYPE id = (ITEMID_TYPE)iResIndex;

	if ( fDupeCheck && (iResType == RES_ITEMDEF) && pCont )
	{
		// Check if they already have the item ? In the case of a regen.
		// This is just to keep NEWBIE items from being duped.
		const CContainer * pContBase = dynamic_cast <CContainer *> ( pCont );
		ASSERT(pContBase);
		if ( pContBase->ContentFind( rid ))
		{
			// We already have this.
			return nullptr;
		}
	}

	CItem * pItem = CItem::CreateTemplate( id, ((iResType == RES_ITEMDEF) ? nullptr : pCont), pSrc );
	if ( pItem != nullptr )
	{
		// Is the item movable ?
		if ( ! pItem->IsMovableType() && pCont && pCont->IsItem())
		{
			g_Log.EventWarn("Script Warning: Created item 0%x, which is not of a movable type, inside cont=0%x\n", id, (dword)(pCont->GetUID()));
            pItem->Delete();
			return nullptr;
		}

		if ( amount != 1 )
			pItem->SetAmount( amount );

		// Items should have their container set after their amount to
		// avoid stacking issues.
		if ( pCont && (iResType == RES_ITEMDEF) )
		{
			CContainer * pContBase = dynamic_cast <CContainer *> ( pCont );
			ASSERT(pContBase);
			pContBase->ContentAdd(pItem);
		}
	}
	return pItem;
}

lpctstr const CItem::sm_szTemplateTable[ITC_QTY+1] =
{
	"BREAK",
	"BUY",
	"CONTAINER",
	"FULLINTERP",
	"ITEM",
	"ITEMNEWBIE",
	"NEWBIESWAP",
	"SELL",
	nullptr
};

CItem * CItem::CreateTemplate( ITEMID_TYPE id, CObjBase * pCont, CChar * pSrc )	// static
{
	ADDTOCALLSTACK("CItem::CreateTemplate");
	// Create an item or a template.
	// A template is a collection of items. (not a single item)
	// But we can create (in this case) only the container item.
	// and return that.
	// ARGS:
	//  pCont = container item or char to put the new template or item in..
	// NOTE:
	//  This can possibly be reentrant under the current scheme.

	if ( id < ITEMID_TEMPLATE )
	{
		CItem * pItem = CreateScript( id, pSrc );
		if ( pCont )
		{
			CContainer * pContBase = dynamic_cast <CContainer *> ( pCont );
			ASSERT(pContBase);
			pContBase->ContentAdd(pItem);
		}
		return pItem;
	}

	CResourceLock s;
	if ( ! g_Cfg.ResourceLock( s, CResourceID( RES_TEMPLATE, id )))
		return nullptr;

	return ReadTemplate( s, pCont );
}

CItem * CItem::ReadTemplate( CResourceLock & s, CObjBase * pCont ) // static
{
	ADDTOCALLSTACK("CItem::ReadTemplate");
	CChar * pVendor = nullptr;
	CItemContainer * pVendorSell = nullptr;
	CItemContainer * pVendorBuy = nullptr;
	if ( pCont != nullptr )
	{
		pVendor = dynamic_cast <CChar*>( pCont->GetTopLevelObj());
		if ( pVendor != nullptr && pVendor->NPC_IsVendor())
		{
			pVendorSell = pVendor->GetBank( LAYER_VENDOR_STOCK );
			pVendorBuy = pVendor->GetBank( LAYER_VENDOR_BUYS );
		}
	}

	bool fItemAttrib = false;
	CItem * pNewTopCont = nullptr;
	CItem * pItem = nullptr;
	while ( s.ReadKeyParse())
	{
		if ( s.IsKeyHead( "ON", 2 ))
			break;

		int index = FindTableSorted( s.GetKey(), sm_szTemplateTable, CountOf( sm_szTemplateTable )-1 );
		switch (index)
		{
			case ITC_BUY: // "BUY"
			case ITC_SELL: // "SELL"
				fItemAttrib = false;
				if (pVendorBuy != nullptr)
				{
					pItem = CItem::CreateHeader( s.GetArgRaw(), (index==ITC_SELL)?pVendorSell:pVendorBuy, false );
					if ( pItem == nullptr )
						continue;
					if ( pItem->IsItemInContainer())
					{
						fItemAttrib = true;
						pItem->SetContainedLayer( (char)(pItem->GetAmount()));	// set the Restock amount.
					}
				}
				continue;

			case ITC_CONTAINER:
				fItemAttrib = false;
				{
					pItem = CItem::CreateHeader( s.GetArgRaw(), pCont, false, pVendor );
					if ( pItem == nullptr )
						continue;
					pCont = dynamic_cast <CItemContainer *> ( pItem );
					if ( pCont == nullptr )
						DEBUG_ERR(( "CreateTemplate: CContainer %s is not a container\n", pItem->GetResourceName() ));
					else
					{
						fItemAttrib = true;
						if ( ! pNewTopCont )
							pNewTopCont = pItem;
					}
				}
				continue;

			case ITC_ITEM:
			case ITC_ITEMNEWBIE:
				fItemAttrib = false;
				if ( pCont == nullptr && pItem != nullptr )
					continue;	// Don't create anymore items til we have some place to put them !
				pItem = CItem::CreateHeader( s.GetArgRaw(), pCont, false, pVendor );
				if ( pItem != nullptr )
					fItemAttrib = true;
				continue;
		}

		if ( pItem != nullptr && fItemAttrib )
			pItem->r_LoadVal( s );
	}

	if ( pNewTopCont )
		return pNewTopCont;
	return pItem;
}

bool CItem::IsTopLevelMultiLocked() const
{
	ADDTOCALLSTACK("CItem::IsTopLevelMultiLocked");
	// Is this item locked to the structure ?
	ASSERT( IsTopLevel());
	if ( ! m_uidLink.IsValidUID())	// linked to something.
		return false;
	if ( IsType(IT_KEY))	// keys cannot be locked down.
		return false;
	const CRegion * pArea = GetTopPoint().GetRegion( REGION_TYPE_MULTI );
	if ( pArea == nullptr )
		return false;
	if ( pArea->GetResourceID().GetObjUID() == m_uidLink.GetObjUID() )
		return true;
	return false;
}

bool CItem::IsMovableType() const
{
	ADDTOCALLSTACK("CItem::IsMovableType");
	if ( IsAttr( ATTR_MOVE_ALWAYS ))	// this overrides other flags.
		return true;
	if ( IsAttr( ATTR_MOVE_NEVER | ATTR_STATIC | ATTR_INVIS | ATTR_LOCKEDDOWN ))
		return false;
	const CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);
	if ( ! pItemDef->IsMovableType())
		return false;
	return true;
}

bool CItem::IsMovable() const
{
	ADDTOCALLSTACK("CItem::IsMovable");
	if ( ! IsTopLevel() && ! IsAttr( ATTR_MOVE_NEVER | ATTR_LOCKEDDOWN ))
	{
		// If it's in my pack (event though not normally movable) thats ok.
		return true;
	}
	return IsMovableType();
}


int CItem::GetVisualRange() const	// virtual
{
	if ( GetDispID() >= ITEMID_MULTI ) // ( IsTypeMulti() ) why not this?
		return UO_MAP_VIEW_RADAR;
	return UO_MAP_VIEW_SIZE_DEFAULT;
}

// Retrieve tag.override.speed for this CItem
// If no tag, then retrieve CItemBase::GetSpeed()
// Doing this way lets speed be changed for all created weapons from the script itself instead of rewriting one by one.
byte CItem::GetSpeed() const
{
    const CVarDefCont *pVarDef = GetKey("OVERRIDE.SPEED", true);
	if (pVarDef)
		return (byte)pVarDef->GetValNum();
	const CItemBase * pItemDef = static_cast<CItemBase *>(Base_GetDef());
	return pItemDef->GetSpeed();
}

byte CItem::GetRangeL() const
{
    return (byte)GetPropNum(COMP_PROPS_ITEMWEAPON, PROPIWEAP_RANGEL, true);
}

byte CItem::GetRangeH() const
{
    return (byte)GetPropNum(COMP_PROPS_ITEMWEAPON, PROPIWEAP_RANGEH, true);
}

int CItem::IsWeird() const
{
	ADDTOCALLSTACK_INTENSIVE("CItem::IsWeird");
	// Does item i have a "link to reality"?
	// (Is the container it is in still there)
	// RETURN: 0 = success ok
	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )
	{
		return iResultCode;
	}
	CItemBase * pItemDef = Item_GetDef();
	if ( pItemDef == nullptr )
	{
		iResultCode = 0x2102;
		return iResultCode;
	}
	if ( pItemDef->GetID() == 0 )
	{
		iResultCode = 0x2103;
		return iResultCode;
	}
	if ( IsDisconnected() )	// Should be connected to the world.
	{
		iResultCode = 0x2104;
		return iResultCode;
	}
	if ( IsTopLevel())
	{
		// no container = must be at valid point.
		if ( ! GetTopPoint().IsValidPoint())
		{
			iResultCode = 0x2105;
			return iResultCode;
		}
		return 0;
	}

	// The container must be valid.
	CObjBase * ptCont = GetContainer();
	return( ( ptCont == nullptr ) ? 0x2106 : ptCont->IsWeird() );
}

char CItem::GetFixZ( CPointMap pt, dword dwBlockFlags )
{
	height_t zHeight = CItemBase::GetItemHeight( GetDispID(), &dwBlockFlags );
	CServerMapBlockState block( dwBlockFlags, pt.m_z, pt.m_z + zHeight, pt.m_z + 2, zHeight );
	CWorldMap::GetFixPoint( pt, block );
	return block.m_Bottom.m_z;
}

int CItem::FixWeirdness()
{
    ADDTOCALLSTACK("CItem::FixWeirdness");
    // Check for weirdness and fix it if possible.
    // RETURN: 0 = i can't fix this.

    if (IsType(IT_EQ_MEMORY_OBJ) && !IsValidUID())
    {
        SetUID(UID_CLEAR, true);	// some cases we don't get our UID because we are created during load.
    }

    int iResultCode = CObjBase::IsWeird();
    if (iResultCode)
    {
        return iResultCode;
    }

    CItemBase * pItemDef = Item_GetDef();
    ASSERT(pItemDef);

    CChar * pChar = nullptr;
    if (IsItemEquipped())
    {
        pChar = dynamic_cast <CChar*>(GetParent());
        if (!pChar)
        {
            iResultCode = 0x2202;
            return iResultCode;
        }
    }

    // Make sure the link is valid.
    if (m_uidLink.IsValidUID())
    {
        // Make sure all my memories are valid !
        if (m_uidLink == GetUID() || m_uidLink.ObjFind() == nullptr)
        {
            if (m_type == IT_EQ_MEMORY_OBJ || m_type == IT_SPELL)
            {
                return 0; // get rid of it.	(this is not an ERROR per se)
            }
			if (IsAttr(ATTR_STOLEN))
			{
				// The item has been laundered.
				m_uidLink.InitUID();
			}
            else
            {
                DEBUG_ERR(("'%s' Bad Link to 0%x\n", GetName(), (dword)(m_uidLink)));
                m_uidLink.InitUID();
                iResultCode = 0x2205;
                return iResultCode;	// get rid of it.
            }
        }
    }

    // Check Amount - Only stackable items should set amount ?

    if (GetAmount() != 1 &&
        !IsType(IT_SPAWN_CHAR) &&
        !IsType(IT_SPAWN_ITEM) &&
        !IsStackableException())
    {
        // This should only exist in vendors restock boxes & spawns.
        if (!GetAmount())
        {
            SetAmount(1);
        }
        // stacks of these should not exist.
        else if (!pItemDef->IsStackableType() &&
            !IsType(IT_CORPSE) &&
            !IsType(IT_EQ_MEMORY_OBJ))
        {
            SetAmount(1);
        }
    }

    // Check Attributes

    //	allow bless/curse for memory items only, and of course deny both blessed+cursed
    if (!IsType(IT_EQ_MEMORY_OBJ))
    {
        ClrAttr(ATTR_CURSED | ATTR_CURSED2 | ATTR_BLESSED | ATTR_BLESSED2);
    }
    else if (IsAttr(ATTR_CURSED | ATTR_CURSED2) && IsAttr(ATTR_BLESSED | ATTR_BLESSED2))
    {
        ClrAttr(ATTR_CURSED | ATTR_CURSED2 | ATTR_BLESSED | ATTR_BLESSED2);
    }

    if (IsMovableType())
    {
        if (IsType(IT_WATER) || Can(CAN_I_WATER))
        {
		if ( ! IsAttr(ATTR_MOVE_ALWAYS))	// If item is explicitely set to always movable, it will not lock it.
		{
	            SetAttr(ATTR_MOVE_NEVER);
		}
        }
    }

    switch (GetID())
    {
        case ITEMID_SPELLBOOK2:	// weird client bug with these.
            SetDispID(ITEMID_SPELLBOOK);
            break;

        case ITEMID_DEATHSHROUD:	// be on a dead person only.
        case ITEMID_GM_ROBE:
        {
            pChar = dynamic_cast<CChar*>(GetTopLevelObj());
            if (pChar == nullptr)
            {
                iResultCode = 0x2206;
                return iResultCode;	// get rid of it.
            }
            if (GetID() == ITEMID_DEATHSHROUD)
            {
                if (IsAttr(ATTR_MAGIC) && IsAttr(ATTR_NEWBIE))
                {
                    break;	// special
                }
                if (!pChar->IsStatFlag(STATF_DEAD))
                {
                    iResultCode = 0x2207;
                    return iResultCode;	// get rid of it.
                }
            }
            else
            {
                // Only GM/counsel can have robe.
                // Not a GM til read *ACCT.SCP
                if (pChar->GetPrivLevel() < PLEVEL_Counsel)
                {
                    iResultCode = 0x2208;
                    return iResultCode;	// get rid of it.
                }
            }
        }
        break;

        case ITEMID_VENDOR_BOX:
            if (!IsItemEquipped())
            {
                SetID(ITEMID_BACKPACK);
            }
            else if (GetEquipLayer() == LAYER_BANKBOX)
            {
                SetID(ITEMID_BANK_BOX);
            }
            else
            {
                SetType(IT_EQ_VENDOR_BOX);
            }
            break;

        case ITEMID_BANK_BOX:
            // These should only be bank boxes and that is it !
            if (!IsItemEquipped())
            {
                SetID(ITEMID_BACKPACK);
            }
            else if (GetEquipLayer() != LAYER_BANKBOX)
            {
                SetID(ITEMID_VENDOR_BOX);
            }
            else
            {
                SetType(IT_EQ_BANK_BOX);
            }
            break;

        case ITEMID_MEMORY:
            // Memory or other flag in my head.
            // Should not exist except equipped.
            if (!IsItemEquipped())
            {
                iResultCode = 0x2222;
                return iResultCode;	// get rid of it.
            }
            break;

        default:
            break;
    }

    switch (GetType())
    {
        case IT_EQ_TRADE_WINDOW:
            // Should not exist except equipped. Commented the check on the layer because, right now, placing it on LAYER_SPECIAL is the only way to create script-side a trade window.
            if (!IsItemEquipped() || /*GetEquipLayer() != LAYER_NONE ||*/ !pChar || !pChar->m_pPlayer || !pChar->IsClientActive())
            {
                if (IsItemEquipped())
                {
                    CChar* pCharCont = dynamic_cast<CChar*>(GetContainer());
                    if (pCharCont)
                    {
                        CItemContainer* pTradeCont = dynamic_cast<CItemContainer*>(this);
                        ASSERT(pTradeCont);
						for (CSObjContRec* pObjRec : pTradeCont->GetIterationSafeContReverse())
						{
							CItem* pItem = static_cast<CItem*>(pObjRec);
                            pCharCont->ItemBounce(pItem, false);
                        }
                    }
                }
                iResultCode = 0x2220;
                return iResultCode;	// get rid of it.
            }
            break;

        case IT_EQ_CLIENT_LINGER:
            // Should not exist except equipped.
            if (!IsItemEquipped() || (GetEquipLayer() != LAYER_FLAG_ClientLinger) || !pChar || !pChar->m_pPlayer)
            {
                iResultCode = 0x2221;
                return iResultCode;	// get rid of it.
            }
            break;

        case IT_EQ_MEMORY_OBJ:
        {
            // Should not exist except equipped.
            CItemMemory * pItemTemp = dynamic_cast <CItemMemory*>(this);
            if (pItemTemp == nullptr)
            {
                iResultCode = 0x2222;
                return iResultCode;	// get rid of it.
            }
        }
        break;

        case IT_EQ_HORSE:
            // These should only exist eqipped.
            if (!IsItemEquipped() || (GetEquipLayer() != LAYER_HORSE))
            {
                iResultCode = 0x2226;
                return iResultCode;	// get rid of it.
            }
            break;

        case IT_HAIR:
        case IT_BEARD:	// 62 = just for grouping purposes.
            // Hair should only be on person or equipped. (can get lost in pack)
            // Hair should only be on person or on corpse.
            if (!IsItemEquipped())
            {
                CItemContainer * pCont = dynamic_cast<CItemContainer*>(GetParent());
                if (pCont == nullptr || (pCont->GetType() != IT_CORPSE && pCont->GetType() != IT_EQ_VENDOR_BOX))
                {
                    iResultCode = 0x2227;
                    return iResultCode;	// get rid of it.
                }
            }
            else
            {
                SetAttr(ATTR_MOVE_NEVER);
                if (GetEquipLayer() != LAYER_HAIR && GetEquipLayer() != LAYER_BEARD)
                {
                    iResultCode = 0x2228;
                    return iResultCode;	// get rid of it.
                }
            }
            break;

        case IT_GAME_PIECE:
            // This should only be in a game.
            if (!IsItemInContainer())
            {
                iResultCode = 0x2229;
                return iResultCode;	// get rid of it.
            }
            break;

        case IT_KEY:
            // blank unlinked keys.
            if (m_itKey.m_UIDLock.IsValidUID() && !IsValidUID())
            {
                g_Log.EventError("Key '%s' (UID=0%x) has bad link to 0%x: blanked out.\n", GetName(), (dword)GetUID(), (dword)(m_itKey.m_UIDLock));
                m_itKey.m_UIDLock.ClearUID();
            }
            break;

        case IT_SPAWN_CHAR:
        case IT_SPAWN_ITEM:
        {
            CCSpawn *pSpawn = GetSpawn();
            if (pSpawn)
            {
                pSpawn->FixDef();
                pSpawn->SetTrackID();
            }
        }
        break;

        case IT_CONTAINER_LOCKED:
        case IT_SHIP_HOLD_LOCK:
        case IT_DOOR_LOCKED:
            // Doors and containers must have a lock complexity set.
            if (!m_itContainer.m_dwLockComplexity)
            {
                m_itContainer.m_dwLockComplexity = 500 + Calc_GetRandVal(600);
            }
            break;

        case IT_POTION:
            if (m_itPotion.m_dwSkillQuality == 0) // store bought ?
            {
                m_itPotion.m_dwSkillQuality = Calc_GetRandVal(950);
            }
            break;
        case IT_MAP_BLANK:
            if (m_itNormal.m_more1 || m_itNormal.m_more2)
            {
                SetType(IT_MAP);
            }
            break;

        default:
        {
            const IT_TYPE iType = GetType();
            if ((iType > IT_QTY) && (iType < IT_TRIGGER))
            {
                SetType(pItemDef->GetType());
            }
        }
    }

    if (IsItemEquipped())
    {
        ASSERT(pChar);

        switch (GetEquipLayer())
        {
            case LAYER_NONE:
                // Only Trade windows should be equipped this way..
                if (m_type != IT_EQ_TRADE_WINDOW)
                {
                    iResultCode = 0x2230;
                    return iResultCode;	// get rid of it.
                }
                break;
            case LAYER_SPECIAL:
                switch (GetType())
                {
                    case IT_EQ_MEMORY_OBJ:
                    case IT_EQ_SCRIPT:	// pure script.
                        break;
                    default:
                        iResultCode = 0x2231;
                        return iResultCode;	// get rid of it.
                }
                break;
            case LAYER_VENDOR_STOCK:
            case LAYER_VENDOR_EXTRA:
            case LAYER_VENDOR_BUYS:
                if (pChar->m_pPlayer != nullptr)	// players never need carry these,
                {
                    return 0;
                }
                SetAttr(ATTR_MOVE_NEVER);
                break;

            case LAYER_PACK:
            case LAYER_BANKBOX:
                SetAttr(ATTR_MOVE_NEVER);
                break;

            case LAYER_HORSE:
                if (m_type != IT_EQ_HORSE)
                {
                    iResultCode = 0x2233;
                    return iResultCode;	// get rid of it.
                }
                SetAttr(ATTR_MOVE_NEVER);
                pChar->StatFlag_Set(STATF_ONHORSE);
                break;
            case LAYER_FLAG_ClientLinger:
                if (m_type != IT_EQ_CLIENT_LINGER)
                {
                    iResultCode = 0x2234;
                    return iResultCode;	// get rid of it.
                }
                break;

            case LAYER_FLAG_Murders:
                if (!pChar->m_pPlayer ||
                    pChar->m_pPlayer->m_wMurders <= 0)
                {
                    iResultCode = 0x2235;
                    return iResultCode;	// get rid of it.
                }
                break;

            default:
                break;
        }
    }

    else if (IsTopLevel())
    {
        if (IsAttr(ATTR_DECAY) && !_IsTimerSet() /*&& !_IsSleeping()*/)
        {
			// An item marked to decay but with an elapsed or immediately about to elapse timer.
			// // Ignore sleeping items. Maybe it's not broken and it will decay immediately after its sector goes awake.
            iResultCode = 0x2236;
            return iResultCode;	// get rid of it.
        }

        // unreasonably long for a top level item ?
        if (_GetTimerSAdjusted() > 90ll * 24 * 60 * 60)
        {
            g_Log.EventWarn("FixWeirdness on Item (UID=0%x): timer unreasonably long (> 90 days) on a top level object.\n", (uint)GetUID());
            _SetTimeoutS(60 * 60);
        }
    }

    // is m_BaseDef just set bad ?
    return IsWeird();
}

CItem * CItem::UnStackSplit( word amount, CChar * pCharSrc )
{
	ADDTOCALLSTACK("CItem::UnStackSplit");
	// Set this item to have this amount.
	// leave another pile behind.
	// can be 0 size if on vendor.
	// ARGS:
	//  amount = the amount that i want to set this pile to
	// RETURN:
	//  The newly created item.

    const word wAmount = GetAmount();
	if ( amount >= wAmount )
		return nullptr;

	ASSERT( amount <= wAmount );
	CItem * pItemNew = CreateDupeItem( this );
	pItemNew->SetAmount( wAmount - amount );
	SetAmountUpdate( amount );

    if ( ! pItemNew->MoveNearObj( this ))
	{
		if ( pCharSrc )
			pCharSrc->ItemBounce( pItemNew );
		else
		{
			// No place to put this item !
			pItemNew->Delete();
		}
	}
    else
    {
        pItemNew->Update();
    }

	return pItemNew;
}

bool CItem::IsSameType( const CObjBase * pObj ) const
{
	ADDTOCALLSTACK("CItem::IsSameType");
	const CItem * pItem = dynamic_cast <const CItem*> ( pObj );
	if ( pItem == nullptr )
		return false;

	if ( GetID() != pItem->GetID() )
		return false;
	if ( GetHue() != pItem->GetHue())
		return false;
	if ( m_type != pItem->m_type )
		return false;
	if ( m_uidLink != pItem->m_uidLink )
		return false;

	if ( m_itNormal.m_more1 != pItem->m_itNormal.m_more1 )
		return false;
	if ( m_itNormal.m_more2 != pItem->m_itNormal.m_more2 )
		return false;
	if ( m_itNormal.m_morep != pItem->m_itNormal.m_morep )
		return false;

	return true;
}

bool CItem::IsStackableException() const
{
	ADDTOCALLSTACK("CItem::IsStackableException");
	// IS this normally unstackable type item now stackable ?
	// NOTE: This means the m_vcAmount can be = 0

	if ( IsTopLevel() && IsAttr( ATTR_INVIS ))
		return true;	// resource tracker.

	// In a vendors box ?
	const CItemContainer * pCont = dynamic_cast <const CItemContainer*>( GetParent());
	if ( pCont == nullptr )
		return false;
	if ( ! pCont->IsType(IT_EQ_VENDOR_BOX) && ! pCont->IsAttr(ATTR_MAGIC))
		return false;
	return true;
}

bool CItem::IsStackable( const CItem * pItem ) const
{
	ADDTOCALLSTACK("CItem::IsStackable");
	CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);

	if ( ! pItemDef->IsStackableType())
	{
		// Vendor boxes can stack normally un-stackable stuff.
		if ( ! pItem->IsStackableException())
			return false;
	}

	// try object rotations ex. left and right hand hides ?
	if ( !IsSameType(pItem) )
		return false;

	// total should not add up to > 64K !!!
	/*if ( pItem->GetAmount() > ( UINT16_MAX - GetAmount()))
		return false;*/

	return true;
}

bool CItem::Stack( CItem * pItem )
{
	ADDTOCALLSTACK("CItem::Stack");
	// RETURN:
	//  true = the item got stacked. (it is gone)
	//  false = the item will not stack. (do somethjing else with it)
	// pItem is the item stacking on.

	if ( !pItem )
		return false;
	if ( pItem == this )
		return true;
	if ( !IsStackable(pItem) )
		return false;
	if ( (m_Attr & ~ATTR_DECAY) != (pItem->m_Attr & ~ATTR_DECAY) )
		return false;
	if ( !m_TagDefs.Compare(&pItem->m_TagDefs) )
		return false;
	if ( !m_BaseDefs.CompareAll(&pItem->m_BaseDefs) )
		return false;

	// Lost newbie status.
	if ( IsAttr(ATTR_NEWBIE) != pItem->IsAttr(ATTR_NEWBIE) )
		return false;
	if ( IsAttr(ATTR_MOVE_NEVER) != pItem->IsAttr(ATTR_MOVE_NEVER) )
		return false;
	if ( IsAttr(ATTR_STATIC) != pItem->IsAttr(ATTR_STATIC) )
		return false;
	if ( IsAttr(ATTR_LOCKEDDOWN) != pItem->IsAttr(ATTR_LOCKEDDOWN) )
		return false;

	const uint destMaxAmount = pItem->GetMaxAmount();
    const word destAmount = pItem->GetAmount();
    const word thisAmount = GetAmount();
	if ( ((uint)destAmount + thisAmount) > destMaxAmount )
	{
		word amount = (word)(destMaxAmount - destAmount);
		pItem->SetAmountUpdate(destAmount + amount);
		SetAmountUpdate(thisAmount - amount);
		UpdatePropertyFlag();
		pItem->UpdatePropertyFlag();
		return false;
	}

    SetAmount(destAmount + thisAmount);
    UpdatePropertyFlag();
    pItem->Delete();
	return true;
}

int64 CItem::GetDecayTime() const
{
	ADDTOCALLSTACK("CItem::GetDecayTime");
	// Return time in milliseconds that it will take to decay this item.
	// -1 = never decays.

	switch (GetType())
	{
		case IT_FOLIAGE:
		case IT_CROPS:		// crops "decay" as they grow
        {
            if (m_itCrop.m_Respawn_Sec > 0) // MORE1 override
                return (m_itCrop.m_Respawn_Sec * MSECS_PER_SEC);

            const int64 iTimeNextNewMoon = CWorldGameTime::GetNextNewMoon((GetTopPoint().m_map == 1) ? false : true);
            const int64 iMinutesDelay = Calc_GetRandLLVal(20) * g_Cfg.m_iGameMinuteLength;
			return (iTimeNextNewMoon - CWorldGameTime::GetCurrentTime().GetTimeRaw() + iMinutesDelay);
        }
		case IT_MULTI:
		case IT_SHIP:
		case IT_MULTI_CUSTOM:
			return ( 14 * 24 * 60 * 60 * MSECS_PER_SEC );		// very long decay updated as people use it
		case IT_TRASH_CAN:
			return ( 3 * 60 * MSECS_PER_SEC );		// empties in 3 minutes
		default:
			break;
	}

	if (IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC|ATTR_LOCKEDDOWN|ATTR_SECURE) || !IsMovableType())
		return -1;

	return g_Cfg.m_iDecay_Item;
}

void CItem::_SetTimeout( int64 iMsecs )
{
	ADDTOCALLSTACK("CItem::_SetTimeout");
	// PURPOSE:
	//  Set delay in msecs.
	//  -1 = never.
	// NOTE:
	//  It may be a decay timer or it might be a trigger timer

	CTimedObject::_SetTimeout(iMsecs);
}

bool CItem::MoveToUpdate(const CPointMap& pt, bool fForceFix)
{
	bool fReturn = MoveTo(pt, fForceFix);
	Update();
	return fReturn;
}

bool CItem::MoveToDecay(const CPointMap & pt, int64 iMsecsTimeout, bool fForceFix)
{
	SetDecayTime(iMsecsTimeout);
	return MoveToUpdate(pt, fForceFix);
}

void CItem::SetDecayTime(int64 iMsecsTimeout, bool fOverrideAlways)
{
	ADDTOCALLSTACK("CItem::SetDecayTime");
	// 0 = default (decay on the next tick)
	// -1 = set none. (clear it)

	if (!fOverrideAlways && _IsTimerSet() && !IsAttr(ATTR_DECAY))
	{
		// Already a timer here (and it's not a decay timer). Let it expire on it's own.
		return;
	}

	if (iMsecsTimeout == 0)
	{
		if (IsTopLevel())
            iMsecsTimeout = GetDecayTime();
		else
            iMsecsTimeout = -1;
	}
	_SetTimeout(iMsecsTimeout);

	if (iMsecsTimeout >= 0)
		SetAttr(ATTR_DECAY);
	else
		ClrAttr(ATTR_DECAY);
}

SOUND_TYPE CItem::GetDropSound( const CObjBase * pObjOn ) const
{
	ADDTOCALLSTACK("CItem::GetDropSound");
	// Get a special drop sound for the item.
	CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);
	SOUND_TYPE iSnd = 0;

	switch ( pItemDef->GetType())
	{
		case IT_COIN:
		case IT_GOLD:
			// depends on amount.
			switch ( GetAmount())
			{
				case 1: iSnd = 0x035; break;
				case 2: iSnd = 0x032; break;
				case 3:
				case 4:	iSnd = 0x036; break;
				default: iSnd = 0x037;
			}
			break;
		case IT_GEM:
			iSnd = (( GetID() > ITEMID_GEMS ) ? 0x034 : 0x032 );  // Small vs Large Gems
			break;
		case IT_INGOT:  // Any Ingot
			if ( pObjOn == nullptr )
			{
				iSnd = 0x033;
			}
			break;

		default:
			break;
	}

	CVarDefCont * pVar = GetDefKey("DROPSOUND", true);
	if ( pVar )
	{
		if (int64 iVal = pVar->GetValNum())
			iSnd = (SOUND_TYPE)iVal;
	}

	// normal drop sound for what dropped in/on.
	if ( iSnd == 0 )
		return( pObjOn ? 0x057 : 0x042 );
	else
		return ( iSnd );
}

bool CItem::MoveTo(const CPointMap& pt, bool fForceFix) // Put item on the ground here.
{
	ADDTOCALLSTACK("CItem::MoveTo");
	// Move this item to it's point in the world. (ground/top level)
	// NOTE: Do NOT do the decay timer here.

	if ( ! pt.IsValidPoint())
		return false;

	CSector * pSector = pt.GetSector();
	ASSERT( pSector );
	pSector->MoveItemToSector(this);	// This also awakes the item

	// Is this area too complex ?
	if ( ! g_Serv.IsLoading())
		pSector->CheckItemComplexity();

	SetTopPoint( pt );
	if ( fForceFix )
		SetTopZ(GetFixZ(pt));

	return true;
}

bool CItem::MoveToCheck( const CPointMap & pt, CChar * pCharMover )
{
	ADDTOCALLSTACK("CItem::MoveToCheck");
	// Make noise and try to pile it and such.

	CPointMap ptNewPlace;
	if (pt.IsValidPoint())
    {
		if (pCharMover && pCharMover->IsPriv(PRIV_GM))
		{
			ptNewPlace = pt;
		}
        else
        {
            const CPointMap ptWall(CWorldMap::FindItemTypeNearby(pt, IT_WALL, 0, true, true));
            if (!ptWall.IsValidPoint())
            {
                const CPointMap ptWindow(CWorldMap::FindItemTypeNearby(pt, IT_WINDOW, 0, true, true));
                if (!ptWindow.IsValidPoint())
                {
                    ptNewPlace = pt;
                }
            }
        }
    }
	if (pCharMover && !ptNewPlace.IsValidPoint())
	{
		pCharMover->ItemBounce(this);
		return false;
	}

	llong iDecayTime = GetDecayTime();
	if ( iDecayTime > 0 )
	{
		const CRegion * pRegion = ptNewPlace.GetRegion(REGION_TYPE_MULTI|REGION_TYPE_AREA|REGION_TYPE_ROOM);
		if ( pRegion != nullptr && pRegion->IsFlag(REGION_FLAG_NODECAY) )
			iDecayTime = -1 * MSECS_PER_SEC;
	}

    const CObjBase *pCont = GetContainer();
	TRIGRET_TYPE ttResult = TRIGRET_RET_DEFAULT;
    if (IsTrigUsed(TRIGGER_DROPON_GROUND) || IsTrigUsed(TRIGGER_ITEMDROPON_GROUND))
    {
        CScriptTriggerArgs args;
        args.m_iN1 = iDecayTime / MSECS_PER_TENTH;  // ARGN1 = Decay time for the dropped item (in tenths of second)
        //args.m_iN2 = 0;
        args.m_s1 = ptNewPlace.WriteUsed();
        ttResult = OnTrigger(ITRIG_DROPON_GROUND, pCharMover, &args);

        if (IsDeleted())
            return false;

        iDecayTime = args.m_iN1 * MSECS_PER_TENTH;

		// Warning: here we ignore the read-onlyness of CSString's buffer only because we know that CPointMap constructor won't write past the end, but only replace some characters with '\0'. It's not worth it to build another string just for that.
		tchar* ptcArgs = const_cast<tchar*>(args.m_s1.GetBuffer());
        const CPointMap ptChanged(ptcArgs);
        if (!ptChanged.IsValidPoint())
            g_Log.EventError("Trying to override item drop P with an invalid P. Using the original one.\n");
        else
            ptNewPlace = ptChanged;
    }
    else if (!ptNewPlace.IsValidPoint())
    {
        return false;
    }

    // If i have changed the container via scripts, we shouldn't change item's position again here
    if (pCont == GetContainer())
    {
        MoveTo(ptNewPlace);
    }
	Update();

	if ( ttResult == TRIGRET_RET_TRUE )
		return true;
	
	// Check if there's too many items on the same spot
	uint iItemCount = 0;
	const CItem * pItem = nullptr;
	CWorldSearch AreaItems(ptNewPlace);
	for (;;)
	{
		pItem = AreaItems.GetItem();
		if ( pItem == nullptr )
			break;

		++iItemCount;
		if ( iItemCount > g_Cfg.m_iMaxItemComplexity )
		{
			Speak(g_Cfg.GetDefaultMsg(DEFMSG_TOO_MANY_ITEMS));
			iDecayTime = 60 * MSECS_PER_SEC;		// force decay (even when REGION_FLAG_NODECAY is set)
			break;
		}
	}
	 
	/*  // From 56b
		// Too many items on the same spot!
        if ( iItemCount > g_Cfg.m_iMaxItemComplexity )
        {
            Speak("Too many items here!");
            if ( iItemCount > g_Cfg.m_iMaxItemComplexity + g_Cfg.m_iMaxItemComplexity/2 )
            {
                Speak("The ground collapses!");
                Delete();
            }
            // attempt to reject the move.
            return false;
        }
    */
	
	SetDecayTime(iDecayTime);
	Sound(GetDropSound(nullptr));
	return true;	
}

bool CItem::MoveNearObj( const CObjBaseTemplate* pObj, ushort uiSteps )
{
	ADDTOCALLSTACK("CItem::MoveNearObj");
	// Put in the same container as another item.
	ASSERT(pObj);
	CItemContainer *pPack = dynamic_cast<CItemContainer *>(pObj->GetParent());
	if ( pPack )
	{
		// Put in same container (make sure they don't get re-combined)
		pPack->ContentAdd(this, pObj->GetContainedPoint());
		return true;
	}
	else
	{
		// Equipped or on the ground so put on ground nearby.
		return CObjBase::MoveNearObj(pObj, uiSteps);
	}
}

lpctstr CItem::GetName() const
{
	ADDTOCALLSTACK("CItem::GetName");
	// allow some items to go unnamed (just use type name).

	CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);

	lpctstr pszNameBase;
	if ( IsIndividualName())
	{
		pszNameBase = CObjBase::GetName();
	}
	else if ( pItemDef->HasTypeName())
	{

		pszNameBase	= nullptr;
		if ( IsSetOF( OF_Items_AutoName ) )
		{
			if ( IsType( IT_SCROLL ) || IsType( IT_POTION ) )
			{
				if ( RES_GET_INDEX(m_itPotion.m_Type) != SPELL_Explosion )
				{
					const CSpellDef * pSpell = g_Cfg.GetSpellDef((SPELL_TYPE)(m_itSpell.m_spell));
					if (pSpell != nullptr)
						pszNameBase	= pSpell->GetName();
				}
			}
		}
		if ( !pszNameBase )			// Just use it's type name instead.
			pszNameBase = pItemDef->GetTypeName();
	}
	else
	{
		// Get the name of the link.
		const CObjBase * pObj = m_uidLink.ObjFind();
		if ( pObj && pObj != this )
		{
			return( pObj->GetName());
		}
		return "unnamed";
	}

	// Watch for names that should be pluralized.
	// Get rid of the % in the names.
	return CItemBase::GetNamePluralize( pszNameBase, ((m_wAmount != 1) && ! IsType(IT_CORPSE)) );
}

lpctstr CItem::GetNameFull( bool fIdentified ) const
{
	ADDTOCALLSTACK("CItem::GetNameFull");
	// Should be lpctstr
	// Get a full descriptive name. Prefixing and postfixing.

	size_t len = 0;
	tchar * pTemp = Str_GetTemp();

	lpctstr pszTitle = nullptr;
	lpctstr pszName = GetName();

	CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);

	bool fSingular = (GetAmount()==1 || IsType(IT_CORPSE));
	if (fSingular) // m_corpse_DispID is m_vcAmount
	{
		if ( ! IsIndividualName())
			len += Str_CopyLen( pTemp+len, pItemDef->GetArticleAndSpace());
	}
	else
	{
		pszTitle = "%u ";
		len += snprintf(pTemp + len, STR_TEMPLENGTH - len, pszTitle, GetAmount());
	}

	if ( fIdentified && IsAttr(ATTR_CURSED|ATTR_CURSED2|ATTR_BLESSED|ATTR_BLESSED2|ATTR_MAGIC))
	{
		bool fTitleSet = false;
		switch ( m_Attr & ( ATTR_CURSED|ATTR_CURSED2|ATTR_BLESSED|ATTR_BLESSED2))
		{
			case (ATTR_CURSED|ATTR_CURSED2):
				pszTitle = g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_UNHOLY );
				fTitleSet = true;
				break;
			case ATTR_CURSED2:
				pszTitle = g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_DAMNED );
				fTitleSet = true;
				break;
			case ATTR_CURSED:
				pszTitle = g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_CURSED );
				fTitleSet = true;
				break;
			case (ATTR_BLESSED|ATTR_BLESSED2):
				pszTitle = g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_HOLY );
				fTitleSet = true;
				break;
			case ATTR_BLESSED2:
				pszTitle = g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_SACRED );
				fTitleSet = true;
				break;
			case ATTR_BLESSED:
				pszTitle = g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_BLESSED );
				fTitleSet = true;
				break;
		}
		if ( fTitleSet )
		{
			if ( fSingular && !IsSetOF(OF_NoPrefix) )
				len = Str_CopyLimitNull( pTemp, Str_GetArticleAndSpace(pszTitle), STR_TEMPLENGTH);
			len += Str_CopyLimitNull( pTemp+len, pszTitle, STR_TEMPLENGTH - len);
		}

		if ( IsAttr(ATTR_MAGIC))
		{
			if ( !pszTitle )
			{
				pszTitle = IsSetOF(OF_NoPrefix) ? "" : "a ";
				len = Str_CopyLimitNull( pTemp, pszTitle, STR_TEMPLENGTH);
			}

			if ( !IsTypeArmorWeapon() && (strnicmp( pszName, "MAGIC", 5 ) != 0))		// don't put "magic" prefix on armor/weapons and names already starting with "magic"
				len += Str_CopyLimitNull( pTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_MAGIC ), STR_TEMPLENGTH - len);
		}
	}

	// Prefix the name
	switch ( m_type )
	{
		case IT_STONE_GUILD:
			len += Str_CopyLimitNull( pTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_GUILDSTONE_FOR ), STR_TEMPLENGTH - len);
			break;
		case IT_STONE_TOWN:
			len += Str_CopyLimitNull( pTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_TOWN_OF ), STR_TEMPLENGTH - len);
			break;
		case IT_EQ_MEMORY_OBJ:
			len += Str_CopyLimitNull( pTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_MEMORY_OF ), STR_TEMPLENGTH - len);
			break;
		case IT_SPAWN_CHAR:
			if ( ! IsIndividualName())
				len += Str_CopyLimitNull( pTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_SPAWN ), STR_TEMPLENGTH - len);
			break;
		case IT_KEY:
			if ( ! m_itKey.m_UIDLock.IsValidUID())
				len += Str_CopyLimitNull( pTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_BLANK ), STR_TEMPLENGTH - len);
			break;
		case IT_RUNE:
			if ( ! m_itRune.m_ptMark.IsCharValid())
				len += Str_CopyLimitNull( pTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_BLANK ), STR_TEMPLENGTH - len);
			else if ( ! m_itRune.m_Strength )
				len += Str_CopyLimitNull( pTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_FADED ), STR_TEMPLENGTH - len);
			break;
		case IT_TELEPAD:
			if ( ! m_itTelepad.m_ptMark.IsValidPoint())
				len += Str_CopyLimitNull( pTemp+len, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_BLANK ), STR_TEMPLENGTH - len);
			break;
		default:
			break;
	}

	len += Str_CopyLimitNull( pTemp + len, pszName, STR_TEMPLENGTH - len);

	// Suffix the name.

	if ( fIdentified && IsAttr(ATTR_MAGIC) && IsTypeArmorWeapon())	// wand is also a weapon.
	{
		SPELL_TYPE spell = (SPELL_TYPE)(RES_GET_INDEX(m_itWeapon.m_spell));
		if ( spell )
		{
			const CSpellDef * pSpellDef = g_Cfg.GetSpellDef( spell );
			if ( pSpellDef )
			{
				len += snprintf(pTemp + len, STR_TEMPLENGTH - len, " of %s", pSpellDef->GetName());
				if (m_itWeapon.m_spellcharges)
					len += snprintf(pTemp + len, STR_TEMPLENGTH - len, " (%d %s)", m_itWeapon.m_spellcharges, g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_CHARGES ) );
			}
		}
	}

	switch ( m_type )
	{
		case IT_LOOM:
			if ( m_itLoom.m_iClothQty )
			{
				ITEMID_TYPE AmmoID = (ITEMID_TYPE)m_itLoom.m_ridCloth.GetResIndex();
				const CItemBase * pAmmoDef = CItemBase::FindItemBase(AmmoID);
				if ( pAmmoDef )
					len += snprintf(pTemp + len, STR_TEMPLENGTH - len, " (%d %ss)", m_itLoom.m_iClothQty, pAmmoDef->GetName());
			}
			break;

		case IT_ARCHERY_BUTTE:
			if ( m_itArcheryButte.m_iAmmoCount )
			{
				ITEMID_TYPE AmmoID = (ITEMID_TYPE)(m_itArcheryButte.m_ridAmmoType.GetResIndex());
				const CItemBase * pAmmoDef = CItemBase::FindItemBase(AmmoID);
				if ( pAmmoDef )
					len += snprintf(pTemp + len, STR_TEMPLENGTH - len, " %d %ss", m_itArcheryButte.m_iAmmoCount, pAmmoDef->GetName());
			}
			break;

		case IT_STONE_TOWN:
			{
				const CItemStone * pStone = dynamic_cast <const CItemStone*>(this);
				ASSERT(pStone);
				len += snprintf(pTemp + len, STR_TEMPLENGTH - len, " (pop:%" PRIuSIZE_T ")", pStone->GetContentCount());
			}
			break;

		case IT_MEAT_RAW:
		case IT_LEATHER:
		case IT_HIDE:
		case IT_FEATHER:
		case IT_FUR:
		case IT_WOOL:
		case IT_BLOOD:
		case IT_BONE:
			if ( fIdentified )
			{
				CREID_TYPE id = static_cast<CREID_TYPE>(RES_GET_INDEX(m_itSkin.m_creid));
				if ( id)
				{
					const CCharBase * pCharDef = CCharBase::FindCharBase( id );
					if (pCharDef != nullptr)
					{
						len += snprintf(pTemp + len, STR_TEMPLENGTH - len, " (%s)", pCharDef->GetTradeName());
					}
				}
			}
			break;

		case IT_LIGHT_LIT:
		case IT_LIGHT_OUT:
			// how many charges ?
			if ( !IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC) )
				len += snprintf(pTemp + len, STR_TEMPLENGTH - len, " (%d %s)", m_itLight.m_charges, g_Cfg.GetDefaultMsg(DEFMSG_ITEMTITLE_CHARGES));
			break;

		default:
			break;
	}


	if ( IsAttr(ATTR_STOLEN))
	{
		// Who is it stolen from ?
		const CChar * pChar = m_uidLink.CharFind();
		if ( pChar )
			len += snprintf(pTemp + len, STR_TEMPLENGTH - len, " (%s %s)", g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_STOLEN_FROM ), pChar->GetName());
		else
			len += snprintf(pTemp + len, STR_TEMPLENGTH - len, " (%s)", g_Cfg.GetDefaultMsg( DEFMSG_ITEMTITLE_STOLEN ) );
	}

	return pTemp;
}

bool CItem::SetName( lpctstr pszName )
{
	ADDTOCALLSTACK("CItem::SetName");
	// Can't be a dupe name with type name ?
	ASSERT(pszName);
	CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);

	lpctstr pszTypeName = pItemDef->GetTypeName();
	if ( ! strnicmp( pszName, "a ", 2 ) )
	{
		if ( ! strcmpi( pszTypeName, pszName+2 ))
			pszName = "";
	}
	else if ( ! strnicmp( pszName, "an ", 3 ) )
	{
		if ( ! strcmpi( pszTypeName, pszName+3 ))
			pszName = "";
	}
	return SetNamePool( pszName );
}

HUE_TYPE CItem::GetHueVisible() const
{
	if (g_Cfg.m_iColorInvisItem) //If setting ask a specific color
	{
		if (IsAttr(ATTR_INVIS))
		{
			if (!IsType(IT_SPAWN_CHAR) && !IsType(IT_SPAWN_ITEM))  //Spawn point always keep their m_wHue (HUE_RED_DARK)
				return g_Cfg.m_iColorInvisItem;
		}
	}
	
	return CObjBase::GetHue();
}

int CItem::GetWeight(word amount) const
{
	int iWeight = m_weight * (amount ? amount : GetAmount());
    const int iReduction = GetPropNum(COMP_PROPS_ITEMCHAR, PROPITCH_WEIGHTREDUCTION, true);
	if (iReduction)
	{
		iWeight -= (int)IMulDivLL( iWeight, iReduction, 100 );
		if ( iWeight < 0)
			iWeight = 0;
	}
	return iWeight;
}

height_t CItem::GetHeight() const
{
	ADDTOCALLSTACK("CItem::GetHeight");

    height_t tmpHeight;

    const ITEMID_TYPE iDispID = GetDispID();
    const CItemBase * pItemDef = CItemBase::FindItemBase(iDispID);
    ASSERT(pItemDef);
    tmpHeight = pItemDef->GetHeight();
    if (tmpHeight)
        return tmpHeight;

    const CItemBaseDupe * pDupeDef = CItemBaseDupe::GetDupeRef(iDispID);
    if (pDupeDef)
    {
        tmpHeight = pDupeDef->GetHeight();
        if (tmpHeight)
            return tmpHeight;
    }

	char * heightDef = Str_GetTemp();

	sprintf(heightDef, "itemheight_0%x", (uint)iDispID);
	tmpHeight = static_cast<height_t>(g_Exp.m_VarDefs.GetKeyNum(heightDef));
	//DEBUG_ERR(("2 tmpHeight %d\n",tmpHeight));
	if ( tmpHeight ) //set by a defname ([DEFNAME charheight]  height_0a)
		return tmpHeight;

	sprintf(heightDef, "itemheight_%u", (uint)iDispID);
	tmpHeight = static_cast<height_t>(g_Exp.m_VarDefs.GetKeyNum(heightDef));
	//DEBUG_ERR(("3 tmpHeight %d\n",tmpHeight));
	if ( tmpHeight ) //set by a defname ([DEFNAME charheight]  height_10)
		return tmpHeight;

	return 0; //if everything fails
}

bool CItem::SetBase( CItemBase * pItemDef )
{
	ADDTOCALLSTACK("CItem::SetBase");
	// Total change of type. (possibly dangerous)

	CItemBase * pItemOldDef = Item_GetDef();

	if ( pItemDef == nullptr || pItemDef == pItemOldDef )
		return false;

	// This could be a weight change for my parent !!!
	CContainer * pParentCont = dynamic_cast <CContainer*> (GetParent());
	int iWeightOld = 0;

	if ( pItemOldDef )
	{
		pItemOldDef->DelInstance();
		if ( pParentCont )
			iWeightOld = GetWeight();
	}

	pItemDef->AddInstance();	// Increase object instance counter (different from the resource reference counter!)
	m_BaseRef.SetRef(pItemDef);	// Among the other things, it increases the new resource reference counter and decreases the old, if any

	m_weight = pItemDef->GetWeight();

	// matex (moved here from constructor so armor/dam is copied too when baseid changes!)
	m_attackBase = pItemDef->m_attackBase;
	m_attackRange = pItemDef->m_attackRange;
	m_defenseBase = pItemDef->m_defenseBase;
	m_defenseRange = pItemDef->m_defenseRange;

	if (pParentCont)
	{
		ASSERT( IsItemEquipped() || IsItemInContainer());
		pParentCont->OnWeightChange( GetWeight() - iWeightOld );
	}

	m_type = pItemDef->GetType();
	return true;
}

bool CItem::SetBaseID( ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CItem::SetBaseID");
	// Converting the type of an existing item is possibly risky.
	// Creating invalid objects of any sort can crash clients.
	// User created items can be overwritten if we do this twice !
	CItemBase * pItemDef = CItemBase::FindItemBase( id );
	if ( pItemDef == nullptr )
	{
		DEBUG_ERR(( "SetBaseID 0%x invalid item uid=0%x\n",	id, (dword) GetUID()));
		return false;
	}
	// SetBase sets the type, but only SetType does the components check
	SetBase(pItemDef);
    SetType(pItemDef->GetType());
	return true;
}

void CItem::OnHear( lpctstr pszCmd, CChar * pSrc )
{
	// This should never be called directly. Normal items cannot hear. IT_SHIP and IT_COMM_CRYSTAL
	UNREFERENCED_PARAMETER(pszCmd);
	UNREFERENCED_PARAMETER(pSrc);
}

CItemBase * CItem::Item_GetDef() const
{
	return static_cast <CItemBase*>( Base_GetDef() );
}

ITEMID_TYPE CItem::GetID() const
{
	const CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);
	return pItemDef->GetID();
}

bool CItem::SetID( ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CItem::SetID");
	if ( ! IsSameDispID( id ))
	{
		if ( ! SetBaseID( id ))
			return false;
	}
	SetDispID( id );
	return true;
}

bool CItem::IsSameDispID( ITEMID_TYPE id ) const	// account for flipped types ?
{
	const CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);
	return pItemDef->IsSameDispID( id );
}

bool CItem::SetDispID( ITEMID_TYPE id )
{
	ADDTOCALLSTACK("CItem::SetDispID");
	// Just change what this item looks like.
	// do not change it's basic type.

	if ( id == GetDispID() )
		return true;	// no need to do this again or overwrite user created item types.

	if ( CItemBase::IsValidDispID(id) && id < ITEMID_MULTI )
	{
		m_dwDispIndex = id;
	}
	else
	{
		const CItemBase * pItemDef = Item_GetDef();
		ASSERT(pItemDef);
		m_dwDispIndex = pItemDef->GetDispID();
		ASSERT( CItemBase::IsValidDispID((ITEMID_TYPE)(m_dwDispIndex)));
	}
	return true;
}

void CItem::SetAmount(word amount )
{
	ADDTOCALLSTACK("CItem::SetAmount");
	// propagate the weight change.
	// Setting to 0 might be legal if we are deleteing it ?

	word oldamount = GetAmount();
	if ( oldamount == amount )
		return;

	m_wAmount = amount;
	// sometimes the diff graphics for the types are not in the client.
	if ( IsType(IT_ORE) )
	{
		static const ITEMID_TYPE sm_Item_Ore[] =
		{
			ITEMID_ORE_1,
			ITEMID_ORE_1,
			ITEMID_ORE_2,
			ITEMID_ORE_3
		};
		SetDispID( ( GetAmount() >= CountOf(sm_Item_Ore)) ? ITEMID_ORE_4 : sm_Item_Ore[GetAmount()] );
	}

	CContainer * pParentCont = dynamic_cast <CContainer*> (GetParent());
	if (pParentCont)
	{
		ASSERT( IsItemEquipped() || IsItemInContainer());
		pParentCont->OnWeightChange(GetWeight(amount) - GetWeight(oldamount));
	}

	UpdatePropertyFlag();
}

word CItem::GetMaxAmount()
{
	ADDTOCALLSTACK("CItem::GetMaxAmount");
	if ( !IsStackableType() )
		return 0;

	int64 iMax = GetDefNum("MaxAmount", true);
	if (iMax)
	{
		return (word)minimum(iMax, UINT16_MAX);
	}
	else
	{
		return (word)minimum(g_Cfg.m_iItemsMaxAmount, UINT16_MAX);
	}
}

bool CItem::SetMaxAmount(word amount)
{
	ADDTOCALLSTACK("CItem::SetMaxAmount");
	if (!IsStackableType())
		return false;

	SetDefNum("MaxAmount", amount);
	return true;
}

void CItem::SetAmountUpdate(word amount )
{
	ADDTOCALLSTACK("CItem::SetAmountUpdate");
	uint oldamount = GetAmount();
	SetAmount( amount );
	if ( oldamount < 5 || amount < 5 )	// beyond this make no visual diff.
	{
		// Strange client thing. You have to remove before it will update this.
		RemoveFromView();
	}
    if (CanSendAmount())
    {
	    Update();
    }
}

bool CItem::CanSendAmount() const noexcept
{
    // return false -> don't send to the client the real amount for this item (send 1 instead)
    const ITEMID_TYPE id = GetDispID();
    if (id == ITEMID_WorldGem) // it can be a natural resource worldgem bit (used for lumberjacking, mining...)
        return false;
    return true;
}

void CItem::WriteUOX( CScript & s, int index )
{
	ADDTOCALLSTACK("CItem::WriteUOX");
	s.Printf( "SECTION WORLDITEM %d\n", index );
	s.Printf( "{\n" );
	s.Printf( "SERIAL %u\n", (dword) GetUID());
	s.Printf( "NAME %s\n", GetName());
	s.Printf( "ID %d\n", GetDispID());
	s.Printf( "X %d\n", GetTopPoint().m_x );
	s.Printf( "Y %d\n", GetTopPoint().m_y );
	s.Printf( "Z %d\n", GetTopZ());
	s.Printf( "CONT %d\n", -1 );
	s.Printf( "TYPE %d\n", m_type );
	s.Printf( "AMOUNT %d\n", m_wAmount );
	s.Printf( "COLOR %d\n", GetHue());
	//ATT 5
	//VALUE 1
	s.Printf( "}\n\n" );
}

void CItem::r_WriteMore1(CSString & sVal)
{
    ADDTOCALLSTACK("CItem::r_WriteMore1");
    // do special processing to represent this.

	if (IsTypeSpellbook())
	{
		sVal.FormatHex(m_itNormal.m_more1);
		return;
	}

    switch (GetType())
    {
        case IT_TREE:
        case IT_GRASS:
        case IT_ROCK:
        case IT_WATER:
            sVal = g_Cfg.ResourceGetName(m_itResource.m_ridRes);
            return;

        case IT_FRUIT:
        case IT_FOOD:
        case IT_FOOD_RAW:
        case IT_MEAT_RAW:
            sVal = g_Cfg.ResourceGetName(m_itFood.m_ridCook);
            return;

        case IT_TRAP:
        case IT_TRAP_ACTIVE:
        case IT_TRAP_INACTIVE:
        case IT_ANIM_ACTIVE:
        case IT_SWITCH:
        case IT_DEED:
        case IT_LOOM:
        case IT_ARCHERY_BUTTE:
        case IT_ITEM_STONE:
            sVal = g_Cfg.ResourceGetName(CResourceID(RES_ITEMDEF, RES_GET_INDEX(m_itNormal.m_more1)));
            return;

        case IT_FIGURINE:
        case IT_EQ_HORSE:
            sVal = g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, RES_GET_INDEX(m_itNormal.m_more1)));
            return;

        case IT_POTION:
            sVal = g_Cfg.ResourceGetName(CResourceID(RES_SPELL, RES_GET_INDEX(m_itPotion.m_Type)));
            return;

        default:
            if (CResourceIDBase::IsValidResource(m_itNormal.m_more1))
                sVal = g_Cfg.ResourceGetName(CResourceID(m_itNormal.m_more1, 0));
            else
                sVal.FormatHex(m_itNormal.m_more1);
            return;
    }
}

void CItem::r_WriteMore2( CSString & sVal )
{
	ADDTOCALLSTACK_INTENSIVE("CItem::r_WriteMore2");
	// do special processing to represent this.

	if (IsTypeSpellbook())
	{
		sVal.FormatHex(m_itNormal.m_more2);
		return;
	}

	switch ( GetType())
	{
		case IT_FRUIT:
		case IT_FOOD:
		case IT_FOOD_RAW:
		case IT_MEAT_RAW:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_CHARDEF, m_itFood.m_MeatType ));
			return;

		case IT_CROPS:
		case IT_FOLIAGE:
			sVal = g_Cfg.ResourceGetName( m_itCrop.m_ridFruitOverride );
			return;

		case IT_LEATHER:
		case IT_HIDE:
		case IT_FEATHER:
		case IT_FUR:
		case IT_WOOL:
		case IT_BLOOD:
		case IT_BONE:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_CHARDEF, m_itNormal.m_more2 ));
			return;

		case IT_ANIM_ACTIVE:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_TYPEDEF, m_itAnim.m_PrevType ));
			return;

		default:
            if (CResourceIDBase::IsValidResource(m_itNormal.m_more2))
                sVal = g_Cfg.ResourceGetName(CResourceID(m_itNormal.m_more2, 0));
            else
                sVal.FormatHex(m_itNormal.m_more2);
			return;
	}
}

void CItem::r_Write( CScript & s )
{
	ADDTOCALLSTACK_INTENSIVE("CItem::r_Write");
	const CItemBase *pItemDef = Item_GetDef();
	if ( !pItemDef )
		return;

	s.WriteSection("WORLDITEM %s", GetResourceName());

	CObjBase::r_Write(s);

	const ITEMID_TYPE iDispID = GetDispID();
	if (iDispID != GetID())	// the item is flipped.
		s.WriteKeyStr("DISPID", g_Cfg.ResourceGetName(CResourceID(RES_ITEMDEF, iDispID)));

	const int iAmount = GetAmount();
	if (iAmount != 1)
		s.WriteKeyVal("AMOUNT", iAmount);

	if ( !pItemDef->IsType(m_type) )
		s.WriteKeyStr("TYPE", g_Cfg.ResourceGetName(CResourceID(RES_TYPEDEF, m_type)));
	if ( m_uidLink.IsValidUID() )
		s.WriteKeyHex("LINK", m_uidLink.GetObjUID());
	if ( m_Attr )
		s.WriteKeyHex("ATTR", m_Attr);
	if ( m_attackBase )
		s.WriteKeyFormat("DAM", "%hu,%hu", m_attackBase, m_attackBase + m_attackRange);
	if ( m_defenseBase )
		s.WriteKeyFormat("ARMOR", "%hu,%hu", m_defenseBase, m_defenseBase + m_defenseRange);
    if (!GetSpawn())
    {
        if ( m_itNormal.m_more1 )
        {
			CSString sVal(false);
            r_WriteMore1(sVal);
            s.WriteKeyStr("MORE1", sVal.GetBuffer());
        }
        if ( m_itNormal.m_more2 )
        {
			CSString sVal(false);
            r_WriteMore2(sVal);
            s.WriteKeyStr("MORE2", sVal.GetBuffer());
        }
        if ( m_itNormal.m_morep.m_x || m_itNormal.m_morep.m_y || m_itNormal.m_morep.m_z || m_itNormal.m_morep.m_map )
            s.WriteKeyStr("MOREP", m_itNormal.m_morep.WriteUsed());
    }

	if (const CObjBase* pCont = GetContainer())
	{
		if ( pCont->IsChar() )
		{
            const LAYER_TYPE iEqLayer = GetEquipLayer();
			if ( iEqLayer >= LAYER_HORSE )
				s.WriteKeyVal("LAYER", iEqLayer);
		}

		s.WriteKeyHex("CONT", pCont->GetUID().GetObjUID());
		if ( pCont->IsItem() )
		{
			s.WriteKeyStr("P", GetContainedPoint().WriteUsed());
            const uchar uiGridIdx = GetContainedGridIndex();
			if ( uiGridIdx )
				s.WriteKeyVal("CONTGRID", uiGridIdx);
		}
	}
	else
		s.WriteKeyStr("P", GetTopPoint().WriteUsed());

    CEntity::r_Write(s);
    CEntityProps::r_Write(s);
}

bool CItem::LoadSetContainer(const CUID& uidCont, LAYER_TYPE layer )
{
	ADDTOCALLSTACK("CItem::LoadSetContainer");
	// Set the CItem in a container of some sort.
	// Used mostly during load.
	// "CONT" IC_CONT
	// NOTE: We don't have a valid point in the container yet probably. get that later.

	CObjBase * pObjCont = uidCont.ObjFind();
	if ( pObjCont == nullptr )
	{
		DEBUG_ERR(( "Invalid container 0%x\n", (dword)uidCont));
		return false;	// not valid object.
	}

	if ( IsTypeSpellbook() && pObjCont->GetTopLevelObj()->IsChar())	// Intercepting the spell's addition here for NPCs, they store the spells on vector <Spells>m_spells for better access from their AI.
	{
		CChar * pChar = static_cast <CChar*>(pObjCont->GetTopLevelObj());
		if (pChar->m_pNPC)
			pChar->NPC_AddSpellsFromBook(this);
	}
	if ( pObjCont->IsItem() )
	{
		// layer is not used here of course.

		CItemContainer * pCont = dynamic_cast <CItemContainer *> (pObjCont);
		if ( pCont != nullptr )
		{
			pCont->ContentAdd( this );
			return true;
		}
	}
	else
	{
		CChar * pChar = dynamic_cast <CChar *> (pObjCont);
		if ( pChar != nullptr )
		{
			// equip the item
			const CItemBase * pItemDef = Item_GetDef();
			ASSERT(pItemDef);
			if ( ! layer )
				layer = pItemDef->GetEquipLayer();

			pChar->LayerAdd( this, layer );
			return true;
		}
	}

	DEBUG_ERR(( "Non container uid=0%x,id=0%x\n", (dword)uidCont, pObjCont->GetBaseID() ));
	return false;		// not a container.
}

enum ICR_TYPE
{
	ICR_CONT,
	ICR_LINK,
	ICR_REGION,
	ICR_TOPCONT,
	ICR_QTY
};

lpctstr const CItem::sm_szRefKeys[ICR_QTY+1] =
{
	"CONT",
	"LINK",
	"REGION",
	"TOPCONT",
	nullptr
};

bool CItem::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CItem::r_GetRef");

    if (CEntity::r_GetRef(ptcKey, pRef))
    {
        return true;
    }

	int i = FindTableHeadSorted( ptcKey, sm_szRefKeys, CountOf(sm_szRefKeys)-1 );
	if ( i >= 0 )
	{
		ptcKey += strlen( sm_szRefKeys[i] );
		SKIP_SEPARATORS(ptcKey);
		switch (i)
		{
			case ICR_CONT:
				if ( ptcKey[-1] != '.' )
					break;
				pRef = GetContainer();
				return true;
			case ICR_LINK:
				if ( ptcKey[-1] != '.' )
					break;
				pRef = m_uidLink.ObjFind();
				return true;
			case ICR_REGION:
				pRef = GetTopLevelObj()->GetTopPoint().GetRegion(REGION_TYPE_MULTI |REGION_TYPE_AREA);
				return true;
			case ICR_TOPCONT:
				if (ptcKey[-1] != '.')
					break;
				pRef = GetTopContainer();
				return true;
		}
	}

	return CObjBase::r_GetRef( ptcKey, pRef );
}

enum IC_TYPE
{
	#define ADD(a,b) IC_##a,
	#include "../../tables/CItem_props.tbl"
	#undef ADD
	IC_QTY
};

lpctstr const CItem::sm_szLoadKeys[IC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../../tables/CItem_props.tbl"
	#undef ADD
	nullptr
};


bool CItem::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
	ADDTOCALLSTACK("CItem::r_WriteVal");
	EXC_TRY("WriteVal");

    if (!fNoCallChildren)
    {
        // Checking Props CComponents first (first check CItem props, if not found then check CItemBase)
        EXC_SET_BLOCK("EntityProp");
        if (CEntityProps::r_WritePropVal(ptcKey, sVal, this, Base_GetDef()))
        {
            return true;
        }

        // Now check regular CComponents
        EXC_SET_BLOCK("Entity");
        if (CEntity::r_WriteVal(ptcKey, sVal, pSrc))
        {
            return true;
        }
    }

    EXC_SET_BLOCK("Keyword");
	int index;
	if ( !strnicmp( CItem::sm_szLoadKeys[IC_ADDSPELL], ptcKey, 8 ) )
		index = IC_ADDSPELL;
	else
		index = FindTableSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );

	bool fDoDefault = false;

	switch ( index )
	{
		//return as string or hex number or 0 if not set
		case IC_DOORCLOSESOUND:
		case IC_DOOROPENSOUND:
		case IC_PORTCULISSOUND:
		case IC_DOOROPENID:
			GetDefStr(ptcKey, true);
			break;
		//return as string or hex number or nullptr if not set
		case IC_CRAFTEDBY:
		case IC_MAKERSNAME:
		case IC_OWNEDBY:
			sVal = GetDefStr(ptcKey);
			break;
		//On these ones, check BaseDef if not found on dynamic
		case IC_DROPSOUND:
		case IC_PICKUPSOUND:
		case IC_EQUIPSOUND:
		case IC_BONUSSKILL1:
		case IC_BONUSSKILL2:
		case IC_BONUSSKILL3:
		case IC_BONUSSKILL4:
		case IC_BONUSSKILL5:
		case IC_ITEMSETNAME:
		case IC_MATERIAL:
		case IC_NPCKILLER:
		case IC_NPCPROTECTION:
		case IC_OCOLOR:
		case IC_BONUSCRAFTING:
		case IC_BONUSCRAFTINGEXCEP:
		case IC_REMOVALTYPE:
		case IC_SUMMONING:
			{
				const CVarDefCont * pVar = GetDefKey(ptcKey, true);
				sVal = pVar ? pVar->GetValStr() : "";
			}
			break;
		//return as decimal number or 0 if not set
		//On these ones, check BaseDef if not found on dynamic
		case IC_BONUSSKILL1AMT:
		case IC_BONUSSKILL2AMT:
		case IC_BONUSSKILL3AMT:
		case IC_BONUSSKILL4AMT:
		case IC_BONUSSKILL5AMT:
		case IC_DURABILITY:
		case IC_ITEMSETAMTCUR:
		case IC_ITEMSETAMTMAX:
		case IC_ITEMSETCOLOR:
		case IC_LIFESPAN:
		case IC_RARITY:
		case IC_RECHARGE:
		case IC_RECHARGEAMT:
		case IC_RECHARGERATE:
		case IC_SELFREPAIR:
		case IC_USESCUR:
		case IC_USESMAX:
		case IC_BONUSCRAFTINGAMT:
		case IC_BONUSCRAFTINGEXCEPAMT:
		case IC_NPCKILLERAMT:
		case IC_NPCPROTECTIONAMT:
			{
				const CVarDefCont * pVar = GetDefKey(ptcKey, true);
				sVal.FormatLLVal(pVar ? pVar->GetValNum() : 0);
			}
			break;
		case IC_MAXAMOUNT:
		    sVal.FormatVal(GetMaxAmount() );
		    break;
		case IC_SPELLCOUNT:
			{
				if ( !IsTypeSpellbook() )
					return false;
				sVal.FormatLLVal(GetSpellcountInBook());
			}
			break;

        case IC_AC:
        case IC_AR:
            sVal.FormatVal(Armor_GetDefense());
            break;

		case IC_ADDSPELL:
			ptcKey	+= 8;
			SKIP_SEPARATORS( ptcKey );
			sVal.FormatVal( IsSpellInBook((SPELL_TYPE)(g_Cfg.ResourceGetIndexType( RES_SPELL, ptcKey ))));
			break;
		case IC_AMOUNT:
        {
            CCSpawn * pSpawn = static_cast<CCSpawn*>(GetComponent(COMP_SPAWN));
            if (pSpawn)
            {
                sVal.FormatVal(pSpawn->GetAmount());
            }
            else
            {
                sVal.FormatVal(GetAmount());
            }
            break;
        }
		case IC_BASEWEIGHT:
			sVal.FormatVal(m_weight);
			break;
		case IC_ATTR:
			sVal.FormatULLHex( m_Attr );
			break;
		case IC_CANUSE:
			sVal.FormatULLHex( m_CanUse );
			break;
		case IC_NODROP:
			sVal.FormatLLVal( m_Attr & ATTR_NODROP );
			break;
		case IC_NOTRADE:
			sVal.FormatLLVal( m_Attr & ATTR_NOTRADE );
			break;
		case IC_QUESTITEM:
			sVal.FormatLLVal( m_Attr & ATTR_QUESTITEM );
			break;
		case IC_CONT:
			{
				if ( ptcKey[4] == '.' )
					return CScriptObj::r_WriteVal( ptcKey, sVal, pSrc, false, false);

				const CObjBase* pCont = GetContainer();
				sVal.FormatHex(pCont ? (dword)pCont->GetUID() : 0);
			}
			break;
		case IC_TOPCONT:
		{
			if (ptcKey[7] == '.')
				return CScriptObj::r_WriteVal(ptcKey, sVal, pSrc, false, false);

			const CObjBase* pCont = GetTopContainer();
			sVal.FormatHex(pCont ? (dword)pCont->GetUID() : 0);
		}
		break;
		case IC_CONTGRID:
			if ( !IsItemInContainer() )
				return false;
			sVal.FormatVal(GetContainedGridIndex());
			break;
		case IC_CONTP:
			{
				const CObjBase * pContainer = GetContainer();
				if ( IsItem() && IsItemInContainer() && pContainer->IsValidUID() && pContainer->IsContainer() && pContainer->IsItem() )
					sVal = GetContainedPoint().WriteUsed();
				else
					return false;
				break;
			}
		case IC_DISPID:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_ITEMDEF, GetDispID()));
			break;
		case IC_DISPIDDEC:
			{
				int iVal;
	 			if ( IsType(IT_COIN)) // Fix money piles
				{
					const CItemBase * pItemDef = Item_GetDef();
					ASSERT(pItemDef);

					iVal = pItemDef->GetDispID();
                    const ushort uiAmount = GetAmount();
					if ( uiAmount >= 2 )
					{
						if ( uiAmount < 6)
							iVal = iVal + 1;
						else
							iVal = iVal + 2;
					}
				}
                else
                {
                    iVal = GetDispID();
                }
				sVal.FormatVal( iVal );
			}
			break;
		case IC_DUPEITEM:
			{
                const ITEMID_TYPE id = GetID();
				if ( id != GetDispID() )
					sVal.FormatHex( id );
				else
					sVal.FormatVal(0);
			}
			break;
		case IC_HEIGHT:
			sVal.FormatVal( GetHeight() );
			break;
		case IC_HITS:
			sVal.FormatVal(LOWORD(m_itNormal.m_more1));
			break;
		case IC_HITPOINTS:
			sVal.FormatVal( IsTypeArmorWeapon() ? m_itArmor.m_dwHitsCur : 0 );
			break;
		case IC_ID:
			fDoDefault = true;
			break;
		case IC_LAYER:
			if ( IsItemEquipped())
			{
				sVal.FormatVal( GetEquipLayer() );
				break;
			}
			fDoDefault = true;
			break;
		case IC_LINK:
			if (ptcKey[4] == '.')
			{
				if (!strnicmp("ISVALID", ptcKey+5, 7))
				{
					sVal.FormatVal(IsValidRef(m_uidLink));
					return true;
				}

				return CScriptObj::r_WriteVal(ptcKey, sVal, pSrc, false);
			}
			sVal.FormatHex( m_uidLink );
			break;
		case IC_MAXHITS:
			sVal.FormatVal(HIWORD(m_itNormal.m_more1));
			break;
		case IC_MORE:
		case IC_MORE1:
			r_WriteMore1(sVal);
			break;
		case IC_MORE1h:
			sVal.FormatVal( HIWORD( m_itNormal.m_more1 ));
			break;
		case IC_MORE1l:
			sVal.FormatVal( LOWORD( m_itNormal.m_more1 ));
			break;
        case IC_FRUIT:
            if (!IsType(IT_FRUIT))
                return false;
            FALLTHROUGH;
		case IC_MORE2:
			r_WriteMore2(sVal);
			break;
		case IC_MORE2h:
			sVal.FormatVal( HIWORD( m_itNormal.m_more2 ));
			break;
		case IC_MORE2l:
			sVal.FormatVal( LOWORD( m_itNormal.m_more2 ));
			break;
		case IC_MOREM:
			sVal.FormatVal( m_itNormal.m_morep.m_map );
			break;
		case IC_MOREP:
			sVal = m_itNormal.m_morep.WriteUsed();
			break;
		case IC_MOREX:
			sVal.FormatVal( m_itNormal.m_morep.m_x );
			break;
		case IC_MOREY:
			sVal.FormatVal( m_itNormal.m_morep.m_y );
			break;
		case IC_MOREZ:
			sVal.FormatVal( m_itNormal.m_morep.m_z );
			break;
		case IC_P:
			fDoDefault = true;
			break;
		case IC_TYPE:
			sVal = g_Cfg.ResourceGetName( CResourceID(RES_TYPEDEF, m_type) );
			break;
		default:
            if (!fNoCallParent)
            {
			    fDoDefault = true;
            }
	}
    if (fDoDefault)
    {
        return CObjBase::r_WriteVal(ptcKey, sVal, pSrc, false);
    }
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

void CItem::r_LoadMore1(dword dwVal)
{
    ADDTOCALLSTACK_INTENSIVE("CItem::r_LoadMore1");
    // Ensure that (when needed) the dwVal is stored as a CResourceIDBase,
    //  plus, do some extra checks for spawns

    const int iIndex = RES_GET_INDEX(dwVal);
    switch (GetType())
    {
    case IT_TREE:
    case IT_GRASS:
    case IT_ROCK:
    case IT_WATER:
        m_itResource.m_ridRes = CResourceIDBase(RES_REGIONRESOURCE, iIndex);
        return;

    case IT_FRUIT:
    case IT_FOOD:
    case IT_FOOD_RAW:
    case IT_MEAT_RAW:
    case IT_TRAP:
    case IT_TRAP_ACTIVE:
    case IT_TRAP_INACTIVE:
    case IT_ANIM_ACTIVE:
    case IT_SWITCH:
    case IT_DEED:
    case IT_LOOM:
    case IT_ARCHERY_BUTTE:
    case IT_ITEM_STONE:
        m_itNormal.m_more1 = CResourceIDBase(RES_ITEMDEF, iIndex);
        return;

    //case: IT_SHIP_TILLER
    //case: IT_KEY
    //case: IT_SIGN_GUMP
        // Allow arbitrary values, not necessarily valid UIDs (for backwards compatibility)

    case IT_SPAWN_CHAR:
    case IT_SPAWN_ITEM:
    case IT_SPAWN_CHAMPION:
        if (!g_Serv.IsLoading())
        {
            CCSpawn* pSpawn = GetSpawn();
            if (pSpawn)
            {
                pSpawn->FixDef();
                pSpawn->SetTrackID();
                RemoveFromView();
                Update();
            }
        }
        return;

    default:
        m_itNormal.m_more1 = dwVal;
        return;
    }
}

void CItem::r_LoadMore2(dword dwVal)
{
    ADDTOCALLSTACK_INTENSIVE("CItem::r_LoadMore2");
    // Ensure that (when needed) the dwVal is stored as a CResourceIDBase

    const int iIndex = RES_GET_INDEX(dwVal);
    switch (GetType())
    {
    case IT_CROPS:
    case IT_FOLIAGE:
        m_itCrop.m_ridFruitOverride = CResourceIDBase(RES_ITEMDEF, iIndex);
        return;

    case IT_LEATHER:
    case IT_HIDE:
    case IT_FEATHER:
    case IT_FUR:
    case IT_WOOL:
    case IT_BLOOD:
    case IT_BONE:
        m_itNormal.m_more2 = CResourceIDBase(RES_CHARDEF, iIndex);
        return;

    //case IT_CORPSE:
        // Be tolerant if a t_corpse has as MORE2 an invalid UID, does it really matter? People may use a custom value,
        //  and there's no hardcoded check for this MORE2 (if we add one, we have to ensure that MORE2 holds a valid UID).
    case IT_FIGURINE:
    case IT_EQ_HORSE:
        if (!CUID::IsValidUID(dwVal))
        {
            g_Log.EventWarn("Setting MORE2 (item type '%s') to invalid UID 0%x, defaulting to 0.\n",
                g_Cfg.ResourceGetName(CResourceID(RES_TYPEDEF, GetType())), dwVal);
            m_itFigurine.m_UID.ClearUID();
            return;
        }
        m_itNormal.m_more2 = dwVal;
        return;

    default:
        m_itNormal.m_more2 = dwVal;
        return;
    }
}

bool CItem::r_LoadVal( CScript & s ) // Load an item Script
{
	ADDTOCALLSTACK("CItem::r_LoadVal");
    EXC_TRY("LoadVal");
	
    // Checking Props CComponents first (first check CChar props, if not found then check CCharBase)
    EXC_SET_BLOCK("EntityProp");
    if (CEntityProps::r_LoadPropVal(s, this, Base_GetDef()))
    {
        return true;
    }

    // Now check regular CComponents
    EXC_SET_BLOCK("Entity");
    if (CEntity::r_LoadVal(s))
    {
        return true;
    }

    EXC_SET_BLOCK("Keyword");
    int index = FindTableSorted(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
	switch (index)
	{
		//Set as Strings
		case IC_CRAFTEDBY:
		case IC_DROPSOUND:
		case IC_PICKUPSOUND:
		case IC_EQUIPSOUND:
		case IC_BONUSSKILL1:
		case IC_BONUSSKILL2:
		case IC_BONUSSKILL3:
		case IC_BONUSSKILL4:
		case IC_BONUSSKILL5:
		case IC_ITEMSETNAME:
		case IC_MAKERSNAME:
		case IC_MATERIAL:
		case IC_NPCKILLER:
		case IC_NPCPROTECTION:
		case IC_OCOLOR:
		case IC_OWNEDBY:
		case IC_SUMMONING:
		case IC_BONUSCRAFTING:
		case IC_BONUSCRAFTINGEXCEP:
		case IC_REMOVALTYPE:
			{
				bool fQuoted = false;
				SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
			}
			break;
		//Set as number only
		case IC_BONUSSKILL1AMT:
		case IC_BONUSSKILL2AMT:
		case IC_BONUSSKILL3AMT:
		case IC_BONUSSKILL4AMT:
		case IC_BONUSSKILL5AMT:
		case IC_DURABILITY:
		case IC_ITEMSETAMTCUR:
		case IC_ITEMSETAMTMAX:
		case IC_ITEMSETCOLOR:
		case IC_LIFESPAN:
		case IC_RARITY:
		case IC_RECHARGE:
		case IC_RECHARGEAMT:
		case IC_RECHARGERATE:
		case IC_SELFREPAIR:
		case IC_USESCUR:
		case IC_BONUSCRAFTINGAMT:
		case IC_BONUSCRAFTINGEXCEPAMT:
		case IC_NPCKILLERAMT:
		case IC_NPCPROTECTIONAMT:
		case IC_DOORCLOSESOUND:
		case IC_DOOROPENSOUND:
		case IC_PORTCULISSOUND:
		case IC_DOOROPENID:
			SetDefNum(s.GetKey(),s.GetArgVal(), false);
			break;

		case IC_NODROP:
			if ( !s.HasArgs() )
				m_Attr |= ATTR_NODROP;
			else
				if ( s.GetArgVal() )
					m_Attr |= ATTR_NODROP;
				else
					m_Attr &= ~ATTR_NODROP;
			break;
		case IC_NOTRADE:
			if ( !s.HasArgs() )
				m_Attr |= ATTR_NOTRADE;
			else
				if ( s.GetArgVal() )
					m_Attr |= ATTR_NOTRADE;
				else
					m_Attr &= ~ATTR_NOTRADE;
			break;
		case IC_QUESTITEM:
			if ( !s.HasArgs() )
				m_Attr |= ATTR_QUESTITEM;
			else
				if ( s.GetArgVal() )
					m_Attr |= ATTR_QUESTITEM;
				else
					m_Attr &= ~ATTR_QUESTITEM;
			break;
		case IC_USESMAX:
		{
			int64 amount = s.GetArgLLVal();
			SetDefNum(s.GetKey(), amount, false);
			CVarDefCont * pVar = GetDefKey("Usescur", true);
			if (!pVar)
				SetDefNum("UsesCur", amount, false);
		}	break;
		case IC_MAXAMOUNT:
			if ( !SetMaxAmount(s.GetArgUSVal()) )
				return false;
			break;
		case IC_ADDCIRCLE:
			{
				if (m_type != IT_SPELLBOOK)
				{
					g_Log.EventError("ADDCIRCLE works only on t_spellbook items (mage spellbook).\n");
					return false;
				}

				tchar *ppVal[2];
				size_t amount = Str_ParseCmds(s.GetArgStr(), ppVal, CountOf(ppVal), " ,\t");
				bool includeLower = 0;	// should i add also the lower circles?
				int addCircle = 0;

				if ( amount <= 0 )
					return false;
				else if (amount > 1)
					includeLower = (atoi(ppVal[1]) != 0);

				for ( addCircle = atoi(ppVal[0]); addCircle; --addCircle )
				{
					for ( short i = 1; i < 9; ++i )
						AddSpellbookSpell((SPELL_TYPE)(RES_GET_INDEX(((addCircle - 1) * 8) + i)), false);

					if ( includeLower == false )
						break;
				}
                break;
			}
		case IC_ADDSPELL:
			// Add this spell to the i_spellbook.
			{
				SPELL_TYPE spell = (SPELL_TYPE)(RES_GET_INDEX(s.GetArgVal()));
				if (AddSpellbookSpell(spell, false))
					return false;
                break;
			}
		case IC_AMOUNT:
        {
            CCSpawn * pSpawn = static_cast<CCSpawn*>(GetComponent(COMP_SPAWN));
            if (pSpawn)
            {
                pSpawn->SetAmount(s.GetArgWVal());
            }
            else
            {
                SetAmountUpdate(s.GetArgWVal());
            }
            return true;
        }
		case IC_ATTR:
			m_Attr = s.GetArgU64Val();
			break;
		case IC_BASEWEIGHT:
			m_weight = s.GetArgWVal();
            break;
		case IC_CANUSE:
			m_CanUse = s.GetArgVal();
            break;
		case IC_CONT:	// needs special processing.
			{
				bool normcont = LoadSetContainer(CUID(s.GetArgDWVal()), (LAYER_TYPE)GetUnkZ());
				if (!normcont)
				{
					SERVMODE_TYPE iModeCode = g_Serv.GetServerMode();
					if ((iModeCode == SERVMODE_Loading) || (iModeCode == SERVMODE_GarbageCollection))
						Delete();	//	since the item is no longer in container, it should be deleted
				}
				else
				{
					RemoveFromView(nullptr, true);
				}
				return normcont;
			}
		case IC_CONTGRID:
			if ( !IsItemInContainer() )
				return false;
			SetContainedGridIndex(s.GetArgUCVal());
			return true;
		case IC_CONTP:
			{
				CPointMap pt;	// invalid point
				tchar *pszTemp = Str_GetTemp();
				Str_CopyLimitNull( pszTemp, s.GetArgStr(), STR_TEMPLENGTH );
				GETNONWHITESPACE( pszTemp );

				if ( IsDigit( pszTemp[0] ) || pszTemp[0] == '-' )
				{
					pt.m_map = 0; pt.m_z = 0;
					tchar * ppVal[2];
					size_t iArgs = Str_ParseCmds( pszTemp, ppVal, CountOf( ppVal ), " ,\t" );
					if ( iArgs < 2 )
					{
						DEBUG_ERR(( "Bad CONTP usage (not enough parameters)\n" ));
						return false;
					}
					switch ( iArgs )
					{
						default:
							FALLTHROUGH;
						case 2:
							pt.m_y = (short)(atoi(ppVal[1]));
							FALLTHROUGH;
						case 1:
							pt.m_x = (short)(atoi(ppVal[0]));
							FALLTHROUGH;
						case 0:
							break;
					}
				}
				CObjBase * pContainer = GetContainer();
				if ( IsItem() && IsItemInContainer() && pContainer->IsValidUID() && pContainer->IsContainer() && pContainer->IsItem() )
				{
					CItemContainer * pCont = dynamic_cast <CItemContainer *> ( pContainer );
					pCont->ContentAdd( this, pt );
				}
				else
				{
					DEBUG_ERR(( "Bad CONTP usage (item is not in a container)\n" ));
					return false;
				}
				break;
			}
		case IC_DISPID:
		case IC_DISPIDDEC:
			return SetDispID((ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr())));
		case IC_HITS:
			{
				int maxHits = HIWORD(m_itNormal.m_more1);
				if( maxHits == 0 )
					maxHits = s.GetArgVal();
				m_itNormal.m_more1 = MAKEDWORD(s.GetArgVal(), maxHits);
			}
			break;
		case IC_HITPOINTS:
			if ( !IsTypeArmorWeapon() )
			{
				DEBUG_ERR(("Item:Hitpoints assigned for non-weapon %s\n", GetResourceName()));
				return false;
			}
			m_itArmor.m_dwHitsCur = m_itArmor.m_wHitsMax = (word)(s.GetArgVal());
            break;
		case IC_ID:
		{
			lpctstr idstr = s.GetArgStr();
			const CResourceID rid = g_Cfg.ResourceGetID(RES_QTY, idstr);
			if (rid.GetResType() == RES_TEMPLATE)
			{
				// here we don't have to check if we are setting the ID in the script's header, because that is handled
				//	by IBC_ID in CItemBase; here we are under @Create trigger or loading from saves (?)

				CItem * pItemTemp = CItem::CreateTemplate((ITEMID_TYPE)rid.GetResIndex(), nullptr, nullptr);
				if (!pItemTemp)
					return false;

				const bool fCont = pItemTemp->IsContainer();
				const ITEMID_TYPE id = pItemTemp->GetID();
				pItemTemp->Delete();

				if (fCont)
				{
					g_Log.EventError("The template should not return a container-type item!\n");
					return false;	// not the kind of template we want...
				}
				else
					return SetID(id);
			}
			else
            {
                const RES_TYPE resType = rid.GetResType();
                if (resType == RES_ITEMDEF || resType == RES_QTY)
                    return SetID((ITEMID_TYPE)rid.GetResIndex());
            }
            return false;
		}
		case IC_LAYER:
			// used only during load (i'm reading the save files and placing again the items in the world in the right place).
			if ( ! IsDisconnected() && ! IsItemInContainer() && ! IsItemEquipped())
				return false;
			SetUnkZ( s.GetArgCVal() ); // GetEquipLayer()
            break;
		case IC_LINK:
			m_uidLink.SetObjUID(s.GetArgDWVal());
            break;

		case IC_FRUIT:	// m_more2
			m_itCrop.m_ridFruitOverride = CResourceIDBase(RES_ITEMDEF, RES_GET_INDEX(s.GetArgDWVal()));
            break;
		case IC_MAXHITS:
			m_itNormal.m_more1 = MAKEDWORD(LOWORD(m_itNormal.m_more1), s.GetArgVal());
			break;
		case IC_MORE:
		case IC_MORE1:
            r_LoadMore1(s.GetArgDWVal());
            break;
		case IC_MORE1h:
			m_itNormal.m_more1 = MAKEDWORD( LOWORD(m_itNormal.m_more1), s.GetArgVal());
			break;
		case IC_MORE1l:
			m_itNormal.m_more1 = MAKEDWORD( s.GetArgVal(), HIWORD(m_itNormal.m_more1));
			break;
		case IC_MORE2:
            r_LoadMore2(s.GetArgDWVal());
            break;
		case IC_MORE2h:
			m_itNormal.m_more2 = MAKEDWORD( LOWORD(m_itNormal.m_more2), s.GetArgVal());
			break;
		case IC_MORE2l:
			m_itNormal.m_more2 = MAKEDWORD( s.GetArgVal(), HIWORD(m_itNormal.m_more2));
			break;
		case IC_MOREM:
			m_itNormal.m_morep.m_map = s.GetArgUCVal();
			break;
		case IC_MOREP:
			{
				CPointBase pt;	// invalid point
				tchar *pszTemp = Str_GetTemp();
				Str_CopyLimitNull( pszTemp, s.GetArgStr(), STR_TEMPLENGTH );
				GETNONWHITESPACE( pszTemp );
				int iArgs = 0;
				if ( IsDigit( pszTemp[0] ) || pszTemp[0] == '-' )
				{
					pt.m_map = 0; pt.m_z = 0;
					tchar * ppVal[4];
					iArgs = Str_ParseCmds( pszTemp, ppVal, CountOf( ppVal ), " ,\t" );
					switch ( iArgs )
					{
						default:
						case 4:	// m_map
							if ( IsDigit(ppVal[3][0]))
								pt.m_map = (uchar)(atoi(ppVal[3]));
							FALLTHROUGH;
						case 3: // m_z
							if ( IsDigit(ppVal[2][0]) || ppVal[2][0] == '-' )
								pt.m_z = (char)(atoi(ppVal[2]));
							FALLTHROUGH;
						case 2:
							pt.m_y = (short)(atoi(ppVal[1]));
							FALLTHROUGH;
						case 1:
							pt.m_x = (short)(atoi(ppVal[0]));
							FALLTHROUGH;
						case 0:
							break;
					}
				}
				if ( iArgs < 2 )
					pt.InitPoint();

				m_itNormal.m_morep = pt;
				// m_itNormal.m_morep = g_Cfg.GetRegionPoint( s.GetArgStr() );
			}
			break;
		case IC_MOREX:
			m_itNormal.m_morep.m_x = s.GetArgSVal();
			return true;
		case IC_MOREY:
			m_itNormal.m_morep.m_y = s.GetArgSVal();
			break;
		case IC_MOREZ:
			m_itNormal.m_morep.m_z = s.GetArgCVal();
			break;
		case IC_P:
			// Loading or import ONLY ! others use the r_Verb
			if ( ! IsDisconnected() && ! IsItemInContainer() )
				return false;
			else
			{
				// Will be placed in the world later.
				CPointMap pt;
				pt.Read( s.GetArgStr());
                if (pt.IsValidPoint())
				    SetUnkPoint(pt);
                else
                    return false;
			}
            break;
		case IC_TYPE:
			if (! SetType( (IT_TYPE)(g_Cfg.ResourceGetIndexType( RES_TYPEDEF, s.GetArgStr() )) ))
                return false;
            // Need to UpdatePropertyFlag()
			break;
		default:
        {
            return CObjBase::r_LoadVal(s);
        }
	}
	if ((_iCallingObjTriggerId != ITRIG_CLIENTTOOLTIP) && (_iCallingObjTriggerId != ITRIG_CLIENTTOOLTIP_AFTERDEFAULT))
	{
		// Avoid @ClientTooltip calling TRIGGER @Create, and this calling again UpdatePropertyFlag() and the @ClientTooltip trigger
		UpdatePropertyFlag();
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

bool CItem::r_Load( CScript & s ) // Load an item from script
{
	ADDTOCALLSTACK("CItem::r_Load");
	CScriptObj::r_Load( s );

	if ( GetContainer() == nullptr )
	{
        // Actually place the item into the world.
		if ( GetTopPoint().IsCharValid())
			MoveToUpdate( GetTopPoint());
	}

	int iResultCode = CObjBase::IsWeird();
	if ( iResultCode )
	{
		DEBUG_ERR(( "Item 0%x Invalid, id=%s, code=0%x\n", (dword)GetUID(), GetResourceName(), iResultCode ));
		Delete();
		return true;
	}
	return true;
}

enum CIV_TYPE
{
	#define ADD(a,b) CIV_##a,
	#include "../../tables/CItem_functions.tbl"
	#undef ADD
	CIV_QTY
};

lpctstr const CItem::sm_szVerbKeys[CIV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../../tables/CItem_functions.tbl"
	#undef ADD
	nullptr
};

bool CItem::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CItem::r_Verb");
	ASSERT(pSrc);
	EXC_TRY("Pre-Verb");

    if (CEntity::r_Verb(s, pSrc))
    {
        return true;
    }

	EXC_SET_BLOCK("Verb-Statement");
	int index = FindTableSorted( s.GetKey(), sm_szVerbKeys, CountOf( sm_szVerbKeys )-1 );
	if ( index < 0 )
	{
		return CObjBase::r_Verb( s, pSrc );
	}

	CChar * pCharSrc = pSrc->GetChar();

	switch ( index )
	{
		case CIV_BOUNCE:
			if ( ! pCharSrc )
				return false;
			pCharSrc->ItemBounce( this );
			break;
		case CIV_CONSUME:
			ConsumeAmount( s.HasArgs() ? s.GetArgWVal() : 1 );
			break;
		case CIV_CONTCONSUME:
			{
				if ( IsContainer() )
				{
					CContainer * pCont = dynamic_cast<CContainer*>(this);
					CResourceQtyArray Resources;
					Resources.Load( s.GetArgStr() );
					pCont->ResourceConsume( &Resources, 1, false );
				}
			}
			break;
		case CIV_DECAY:
			SetDecayTimeD( s.GetArgVal());
			break;
		case CIV_DESTROY:	// remove this object now.
		{
			if (s.GetArgVal())
			{
				if ( g_Cfg.m_iEmoteFlags & EMOTEF_DESTROY )
					EmoteObj(g_Cfg.GetDefaultMsg(DEFMSG_ITEM_DMG_DESTROYED));
				else
					Emote(g_Cfg.GetDefaultMsg(DEFMSG_ITEM_DMG_DESTROYED));
			}
			Delete(true);
			return true;
		}
		case CIV_DROP:
			{
				CObjBaseTemplate * pObjTop = GetTopLevelObj();
				MoveToCheck( pObjTop->GetTopPoint(), pSrc->GetChar());
			}
			return true;
		case CIV_DUPE:
			{
				uint iCount = s.GetArgUVal();
				if ( iCount <= 0 )
					iCount = 1;
				if ( !GetContainer() && (iCount > g_Cfg.m_iMaxItemComplexity) )	// if top-level, obey the complexity
					iCount = g_Cfg.m_iMaxItemComplexity;
                bool fNoCont = IsTopLevel();
				while ( iCount-- )
				{
					CItem* pDupe = CItem::CreateDupeItem(this, dynamic_cast<CChar *>(pSrc), true);
					pDupe->_iCreatedResScriptIdx = s.m_iResourceFileIndex;
					pDupe->_iCreatedResScriptLine = s.m_iLineNum;
					pDupe->MoveNearObj(this, 1);
                    if (fNoCont)
                        pDupe->Update();
				}
			}
			break;
		case CIV_EQUIP:
			if ( ! pCharSrc )
				return false;
			return pCharSrc->ItemEquip( this );
		case CIV_UNEQUIP:
			if ( ! pCharSrc )
				return false;
			RemoveSelf();
			pCharSrc->ItemBounce(this);
			break;
		case CIV_USE:
			if ( pCharSrc == nullptr )
				return false;
			pCharSrc->Use_Obj( this, s.HasArgs() ? (s.GetArgVal() != 0) : true, true );
			break;
        case CIV_USEDOOR:   // bypasses key checks.
            if (!(IsType(IT_DOOR) || IsType(IT_DOOR_LOCKED)))
            {
                return false;
            }
            Use_DoorNew(false);
            break;
		case CIV_REPAIR:
			if (!pCharSrc)
				return false;
			return pCharSrc->Use_Repair(this);
		case CIV_SMELT:
		{
			if (!pCharSrc)
				return false;
			CItem * pTarg = CUID(s.GetArgVal()).ItemFind();
			return pCharSrc->Skill_Mining_Smelt(this, pTarg ? pTarg : nullptr);
		}

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

bool CItem::IsTriggerActive(lpctstr trig) const
{
    if (((_iRunningTriggerId == -1) && _sRunningTrigger.empty()) || (trig == nullptr))
        return false;
    if (_iRunningTriggerId != -1)
    {
        ASSERT(_iRunningTriggerId < ITRIG_QTY);
        int iAction = FindTableSorted( trig, CItem::sm_szTrigName, CountOf(CItem::sm_szTrigName)-1 );
        return (_iRunningTriggerId == iAction);
    }
    ASSERT(!_sRunningTrigger.empty());
    return (strcmpi(_sRunningTrigger.c_str(), trig) == 0);
}

void CItem::SetTriggerActive(lpctstr trig)
{
    if (trig == nullptr)
    {
        _iRunningTriggerId = -1;
        _sRunningTrigger.clear();
        return;
    }
    int iAction = FindTableSorted( trig, CItem::sm_szTrigName, CountOf(CItem::sm_szTrigName)-1 );
    if (iAction != -1)
    {
        _iRunningTriggerId = (short)iAction;
        _sRunningTrigger.clear();
        return;
    }
    _sRunningTrigger = trig;
    _iRunningTriggerId = -1;
}

TRIGRET_TYPE CItem::OnTrigger( lpctstr pszTrigName, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
	ADDTOCALLSTACK("CItem::OnTrigger");

	if (IsTriggerActive(pszTrigName)) //This should protect any item trigger from infinite loop
		return TRIGRET_RET_DEFAULT;

	if ( !pSrc )
		pSrc = &g_Serv;

    CItemBase * pItemDef = Item_GetDef();
    ASSERT(pItemDef);

    SetTriggerActive( pszTrigName );
    const ITRIG_TYPE iAction = (ITRIG_TYPE)_iRunningTriggerId;
    // Is there trigger code in the script file ?
    // RETURN:
    //   false = continue default process normally.
    //   true = don't allow this.  (halt further processing)
    TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	EXC_TRY("Trigger");

    // If i'm running the @Create trigger, i want first to run the trigger from the itemdef (normally ran for last), then from the other sources
    if (iAction == ITRIG_Create)
        goto from_itemdef_first;

standard_order:
	// 1) Triggers installed on character, sensitive to actions on all items
    {
		tchar ptcCharTrigName[TRIGGER_NAME_MAX_LEN] = "@ITEM";
		Str_ConcatLimitNull(ptcCharTrigName + 5, pszTrigName + 1, TRIGGER_NAME_MAX_LEN - 5);
        const CTRIG_TYPE iCharAction = (CTRIG_TYPE)FindTableSorted(ptcCharTrigName, CChar::sm_szTrigName, CountOf(CChar::sm_szTrigName) - 1);
        if ((iCharAction > XTRIG_UNKNOWN) && IsTrigUsed(ptcCharTrigName))
        {
            CChar* pChar = pSrc->GetChar();
            if (pChar != nullptr)
            {
                EXC_SET_BLOCK("chardef");
                const CUID uidOldAct = pChar->m_Act_UID;
                pChar->m_Act_UID = GetUID();
                iRet = pChar->OnTrigger(ptcCharTrigName, pSrc, pArgs);
                pChar->m_Act_UID = uidOldAct;
                if (iRet == TRIGRET_RET_TRUE)
                    goto stopandret; // Block further action.
            }
        }
    }

	if ( IsTrigUsed(pszTrigName) )
	{
		//	2) EVENTS (could be modified ingame!)
        {
            // Need this internal scope to prevent the goto jumps cross the initialization of origEvents and curEvents
            EXC_SET_BLOCK("events");
            size_t origEvents = m_OEvents.size();
            size_t curEvents = origEvents;
            for (size_t i = 0; i < curEvents; ++i)
            {
                CResourceLink* pLink = m_OEvents[i].GetRef();
                if (!pLink || !pLink->HasTrigger(iAction))
                    continue;
                CResourceLock s;
                if (!pLink->ResourceLock(s))
                    continue;

                iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
                if (iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT)
                    goto stopandret;

                curEvents = m_OEvents.size();
                if (curEvents < origEvents) // the event has been deleted, modify the counter for other trigs to work
                {
                    --i;
                    origEvents = curEvents;
                }
            }
        }

		// 3) TEVENTS on the item
		EXC_SET_BLOCK("tevents");
		for ( size_t i = 0; i < pItemDef->m_TEvents.size(); ++i )
		{
			CResourceLink * pLink = pItemDef->m_TEvents[i].GetRef();
			ASSERT(pLink);
			if ( !pLink->HasTrigger(iAction) )
				continue;
			CResourceLock s;
			if ( !pLink->ResourceLock(s) )
				continue;
			iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
			if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
				goto stopandret;
		}

		// 4) EVENTSITEM triggers (constant events of Items set from sphere.ini)
		EXC_SET_BLOCK("Item triggers - EVENTSITEM");
		for ( size_t i = 0; i < g_Cfg.m_iEventsItemLink.size(); ++i )
		{
			CResourceLink * pLink = g_Cfg.m_iEventsItemLink[i].GetRef();
			if ( !pLink || !pLink->HasTrigger(iAction) )
				continue;
			CResourceLock s;
			if ( !pLink->ResourceLock(s) )
				continue;
			iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
			if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
				goto stopandret;
		}

		// 5) TYPEDEF
		EXC_SET_BLOCK("typedef");
		{
			// It has an assigned trigger type.
			CResourceLink * pResourceLink = dynamic_cast <CResourceLink *>( g_Cfg.ResourceGetDef( CResourceID( RES_TYPEDEF, GetType() )));
			if ( pResourceLink == nullptr )
			{
                const CChar* pChar = pSrc->GetChar();
				if ( pChar )
					g_Log.EventError( "0%x '%s' has unhandled [TYPEDEF %d] for 0%x '%s'\n", (dword) GetUID(), GetName(), GetType(), (dword) pChar->GetUID(), pChar->GetName());
				else
					g_Log.EventError( "0%x '%s' has unhandled [TYPEDEF %d]\n", (dword) GetUID(), GetName(), GetType() );
				SetType(Item_GetDef()->GetType());
				iRet = TRIGRET_RET_DEFAULT;
				goto stopandret;
			}

			if ( pResourceLink->HasTrigger( iAction ))
			{
				CResourceLock s;
				if ( pResourceLink->ResourceLock(s))
				{
					iRet = CScriptObj::OnTriggerScript( s, pszTrigName, pSrc, pArgs );
					if ( iRet == TRIGRET_RET_TRUE )
						goto stopandret;
				}
			}
		}

        if (iAction != ITRIG_Create)
        {
from_itemdef_first:
            // 6) Look up the trigger in the RES_ITEMDEF. (default)
            EXC_SET_BLOCK("itemdef");
            CBaseBaseDef* pResourceLink = Base_GetDef();
            ASSERT(pResourceLink);
            if (pResourceLink->HasTrigger(iAction))
            {
                CResourceLock s;
                if (pResourceLink->ResourceLock(s))
                    iRet = CScriptObj::OnTriggerScript(s, pszTrigName, pSrc, pArgs);
            }

            // If i'm running the @Create trigger, i jumped here first, but i need to go back and try to run the trigger from the other sources
            if (iAction == ITRIG_Create) // This happens if i came here via the goto, so that the previous if was skipped
                goto standard_order;
        }
	}

stopandret:
    SetTriggerActive(nullptr);
    return iRet;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("trigger '%s' action '%d' char '0%x' [0%x]\n", pszTrigName, iAction, (pSrc && pSrc->GetChar()) ? (dword)pSrc->GetChar()->GetUID() : 0, (dword)GetUID());
	EXC_DEBUG_END;
	return iRet;
}

TRIGRET_TYPE CItem::OnTrigger( ITRIG_TYPE trigger, CTextConsole * pSrc, CScriptTriggerArgs * pArgs )
{
	ASSERT((trigger >= 0) && (trigger < ITRIG_QTY));
	return OnTrigger( CItem::sm_szTrigName[trigger], pSrc, pArgs );
}

// Item type specific stuff.
bool CItem::SetType(IT_TYPE type, bool fPreCheck)
{
	ADDTOCALLSTACK("CItem::SetType");

    if (fPreCheck)
    {
        // Pre-assignment checks
        if (type == IT_MULTI_CUSTOM)
        {
            g_Log.EventError("Can't dynamically assign type 't_multi_custom' to an item. This type can only be specified in the TYPEDEF.\n");
            return false;
        }
    }

    // Assign type
	m_type = type;

    // Post-assignment checks
    // CComponents sanity check.

    // Never unsubscribe Props Components, because if the type is changed to an unsubscribable type and then again to the previous type, the component will be deleted and created again.
    //  This means that all the properties (base and "dynamic") are lost.
    // Add first the most specific components, so that the tooltips will be better ordered
	TrySubscribeAllowedComponentProps<CCPropsItemWeaponRanged>(this);
	TrySubscribeAllowedComponentProps<CCPropsItemWeapon>(this);
	TrySubscribeAllowedComponentProps<CCPropsItemEquippable>(this);

	TrySubscribeComponentProps<CCPropsItemChar>();

    // Ensure that an item that should have a given component has it, and if the item shouldn't have a given component, check if it's subscribed, in that case unsubscribe it.
	CComponent* pComp = GetComponent(COMP_SPAWN);
    bool fIsSpawn = false;
    if ((type != IT_SPAWN_CHAR) && (type != IT_SPAWN_ITEM))
    {
        if (pComp)
            UnsubscribeComponent(pComp);
    }
    else
    {
        fIsSpawn = true;
        if (!pComp)
            SubscribeComponent(new CCSpawn(this));
    }

    if (type != IT_SPAWN_CHAMPION)
    {
        if (!fIsSpawn)
        {
            pComp = GetComponent(COMP_CHAMPION);
            if (pComp)
            {
                UnsubscribeComponent(pComp);
            }
            pComp = GetComponent(COMP_SPAWN);
            if (pComp)
            {
                UnsubscribeComponent(pComp);
            }
        }
    }
    else
    {
        if (!GetComponent(COMP_CHAMPION))
        {
            SubscribeComponent(new CCChampion(this));
        }
        if (!GetComponent(COMP_SPAWN))
        {
            SubscribeComponent(new CCSpawn(this, true));
        }
    }

    pComp = GetComponent(COMP_ITEMDAMAGEABLE);
    if (!CCItemDamageable::CanSubscribe(this))
    {
        if (pComp)
            UnsubscribeComponent(pComp);
    }
    else if (!pComp)
    {
        /* CCItemDamageable is also added from CObjBase::r_LoadVal(OC_CANMASK) for manual override of can flags
        * but it's required to add it also on item's creation depending on it's CItemBase can flags. */
        SubscribeComponent(new CCItemDamageable(this));
    }

    pComp = GetComponent(COMP_FACTION);
    if (!CCFaction::CanSubscribe(this))
    {
        if (pComp)
            UnsubscribeComponent(pComp);
    }
    else if (!pComp)
    {
        SubscribeComponent(new CCFaction());
    }

	return true;
}

bool CItem::IsTypeLit() const
{
	// has m_pattern arg
	switch(m_type)
	{
		case IT_SPELL: // a magic spell effect. (might be equipped)
		case IT_FIRE:
		case IT_LIGHT_LIT:
		case IT_CAMPFIRE:
		case IT_LAVA:
		case IT_WINDOW:
			return true;
		default:
			return false;
	}
}

bool CItem::IsTypeBook() const
{
	switch( m_type )
	{
		case IT_BOOK:
		case IT_MESSAGE:
			return true;
		default:
			return false;
	}
}

bool CItem::IsTypeSpellbook() const
{
	return ( CItemBase::IsTypeSpellbook(m_type));
}

bool CItem::IsTypeArmor() const
{
	return ( CItemBase::IsTypeArmor(m_type));
}

bool CItem::IsTypeWeapon() const
{
	return ( CItemBase::IsTypeWeapon(m_type));
}

bool CItem::IsTypeMulti() const
{
	return ( CItemBase::IsTypeMulti(m_type));
}

bool CItem::IsTypeArmorWeapon() const
{
	// Armor or weapon.
	return ( IsTypeArmor() || IsTypeWeapon());
}

bool CItem::IsTypeLocked() const
{
	switch ( m_type )
	{
		case IT_SHIP_SIDE_LOCKED:
		case IT_CONTAINER_LOCKED:
		case IT_SHIP_HOLD_LOCK:
		case IT_DOOR_LOCKED:
			return true;
		default:
			return false;
	}
}
bool CItem::IsTypeLockable() const
{
	switch( m_type )
	{
		case IT_CONTAINER:
		case IT_DOOR:
		case IT_DOOR_OPEN:
		case IT_SHIP_SIDE:
		case IT_SHIP_PLANK:
		case IT_SHIP_HOLD:
			//case IT_ROPE:
			return true;
		default:
			return( IsTypeLocked() );
	}
}
bool CItem::IsTypeSpellable() const
{
	// m_itSpell
	switch( m_type )
	{
		case IT_SCROLL:
		case IT_SPELL:
		case IT_FIRE:
			return true;
		default:
			return( IsTypeArmorWeapon() );
	}
}

bool CItem::IsTypeEquippable() const
{
    ADDTOCALLSTACK("CItem::IsTypeEquippable");
    return CItemBase::IsTypeEquippable(GetType(), GetEquipLayer());
}

void CItem::DupeCopy( const CItem * pItem )
{
	ADDTOCALLSTACK("CItem::DupeCopy");
	// Dupe this item.

	CObjBase::DupeCopy( pItem );

	m_dwDispIndex = pItem->m_dwDispIndex;
	SetBase( pItem->Item_GetDef() );
	_SetTimeout( pItem->_GetTimerAdjusted() );
	SetType(pItem->m_type);
	m_wAmount = pItem->m_wAmount;
	m_Attr  = pItem->m_Attr;
	m_CanMask = pItem->m_CanMask;
	m_CanUse = pItem->m_CanUse;
	m_uidLink = pItem->m_uidLink;

	m_itNormal.m_more1 = pItem->m_itNormal.m_more1;
	m_itNormal.m_more2 = pItem->m_itNormal.m_more2;
	m_itNormal.m_morep = pItem->m_itNormal.m_morep;

	m_TagDefs.Copy(&(pItem->m_TagDefs));
	m_BaseDefs.Copy(&(pItem->m_BaseDefs));
	m_OEvents = pItem->m_OEvents;
    CEntity::Copy(static_cast<const CEntity*>(pItem));
}

void CItem::SetAnim( ITEMID_TYPE id, int64 iTicksTimeout)
{
	ADDTOCALLSTACK("CItem::SetAnim");
	// Set this to an active anim that will revert to old form when done.
	// ??? use addEffect instead !!!
	m_itAnim.m_PrevID = GetDispID(); // save old id.
	m_itAnim.m_PrevType = m_type;
	SetDispID( id );
	m_type = IT_ANIM_ACTIVE;    // Do not change the components? SetType(IT_ANIM_ACTIVE);
	_SetTimeout(iTicksTimeout);
	//RemoveFromView();
	Update();
}

CObjBase * CItem::GetContainer() const
{
	// What is this CItem contained in ?
	// Container should be a CChar or CItemContainer
	return ( dynamic_cast <CObjBase*> (GetParent()));
}

const CItem* CItem::GetTopContainer() const
{
	//Get the top container
	const CItem* pItem = this;
	while (const CObjBase * pCont = pItem->GetContainer())
	{
		if (pCont->IsChar())
			break;
		ASSERT(pCont->IsItem());
		pItem = static_cast<const CItem *>(pItem->GetContainer());
	}
	return (pItem == this) ? nullptr : pItem;
}

CItem* CItem::GetTopContainer()
{
	//Get the top container
	CItem* pItem = this;
	while (CObjBase* pCont = pItem->GetContainer())
	{
		if (pCont->IsChar())
			break;
		ASSERT(pCont->IsItem());
		pItem = static_cast<CItem*>(pItem->GetContainer());
	}
	return (pItem == this) ? nullptr : pItem;
}

const CObjBaseTemplate* CItem::GetTopLevelObj() const
{
	// recursively get the item that is at "top" level.
	const CObjBase* pObj = GetContainer();
	if ( !pObj )
		return this;
	else if ( pObj == this )		// to avoid script errors setting same CONT
		return this;
	return pObj->GetTopLevelObj();
}

CObjBaseTemplate* CItem::GetTopLevelObj()
{
	// recursively get the item that is at "top" level.
	CObjBase* pObj = GetContainer();
	if (!pObj)
		return this;
	else if (pObj == this)		// to avoid script errors setting same CONT
		return this;
	return pObj->GetTopLevelObj();
}

uchar CItem::GetContainedGridIndex() const
{
	return m_containedGridIndex;
}

void CItem::SetContainedGridIndex(uchar index)
{
	m_containedGridIndex = index;
}

void CItem::Update(const CClient * pClientExclude)
{
	ADDTOCALLSTACK("CItem::Update");
	// Send this new item to all that can see it.

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( pClient == pClientExclude )
			continue;
		if ( ! pClient->CanSee( this ))
			continue;

		if ( IsItemEquipped())
		{
			if ( GetEquipLayer() == LAYER_DRAGGING )
			{
				pClient->addObjectRemove( this );
				continue;
			}
			if ( GetEquipLayer() >= LAYER_SPECIAL )	// nobody cares about this stuff.
				return;
		}
		else if ( IsItemInContainer())
		{
			CItemContainer* pCont = dynamic_cast <CItemContainer*> (GetParent());
			ASSERT(pCont);
			if ( pCont->IsAttr(ATTR_INVIS))
			{
				// used to temporary build corpses.
				pClient->addObjectRemove( this );
				continue;
			}
		}
		pClient->addItem( this );
	}
}

bool CItem::IsValidLockLink( CItem * pItemLock ) const
{
	ADDTOCALLSTACK("CItem::IsValidLockLink");
	// IT_KEY
	if ( pItemLock == nullptr )
		return false;
	if ( pItemLock->IsTypeLockable())
		return( pItemLock->m_itContainer.m_UIDLock == m_itKey.m_UIDLock );
	if ( CItemBase::IsID_Multi( pItemLock->GetDispID()))
	{
		// Is it linked to the item it is locking. (should be)
		// We can't match the lock uid this way tho.
		// Houses may have multiple doors. and multiple locks
		return true;
	}
	// not connected to anything i recognize.
	return false;
}

bool CItem::IsValidLockUID() const
{
	ADDTOCALLSTACK("CItem::IsValidLockUID");
	// IT_KEY
	// Keys must:
	//  1. have m_UIDLock == UID of the container.
	//  2. m_uidLink == UID of the multi.

	if ( ! m_itKey.m_UIDLock.IsValidUID() )	// blank
		return false;

	// or we are a key to a multi.
	// we can be linked to the multi.
	if ( IsValidLockLink( m_itKey.m_UIDLock.ItemFind()) )
		return true;
	if ( IsValidLockLink( m_uidLink.ItemFind()) )
		return true;

	return false;
}

bool CItem::IsKeyLockFit( dword dwLockUID ) const
{
	return ( m_itKey.m_UIDLock == dwLockUID );
}

void CItem::ConvertBolttoCloth()
{
	ADDTOCALLSTACK("CItem::ConvertBolttoCloth");
	// Cutting bolts of cloth with scissors will convert it to his RESOURCES (usually cloth)

	// We need to check all cloth_bolt items
	bool correctID = false;
	for (int i = (int)(ITEMID_CLOTH_BOLT1); i <= (int)(ITEMID_CLOTH_BOLT8); i++)
		if ( IsSameDispID((ITEMID_TYPE)i) )
			correctID = true;

	if ( !correctID )
		return;

	// Prevent the action if there's no resources to be created
	CItemBase * pDefCloth = Item_GetDef();
	if ( ! pDefCloth || !pDefCloth->m_BaseResources.size() )
		return;

	// Start the conversion
	word iOutAmount = GetAmount();
	CItemContainer * pCont = dynamic_cast <CItemContainer*> ( GetContainer() );
	Delete();

	for ( size_t i = 0; i < pDefCloth->m_BaseResources.size(); i++ )
	{
		CResourceID rid = pDefCloth->m_BaseResources[i].GetResourceID();
		if ( rid.GetResType() != RES_ITEMDEF )
			continue;

		const CItemBase * pBaseDef = CItemBase::FindItemBase((ITEMID_TYPE)(rid.GetResIndex()));
		if ( pBaseDef == nullptr )
			continue;

        int64 iTotalAmount = iOutAmount * pDefCloth->m_BaseResources[i].GetResQty();
        
        CItem* pItemNew = nullptr;
        while (iTotalAmount > 0)
        {
            pItemNew = CItem::CreateTemplate(pBaseDef->GetID());
            ASSERT(pItemNew);
            word iStack = (word)minimum(iTotalAmount, pItemNew->GetMaxAmount());
            pItemNew->SetAmount(iStack);

            if (pItemNew->IsType(IT_CLOTH))
            {
                pItemNew->SetHue(GetHue());
            }
            if (pCont)
            {
                pCont->ContentAdd(pItemNew);
            }
            else
            {
                pItemNew->MoveToDecay(GetTopPoint(), g_Cfg.m_iDecay_Item);
            }
            iTotalAmount -= iStack;
        }
	}
}

word CItem::ConsumeAmount( word iQty )
{
	ADDTOCALLSTACK("CItem::ConsumeAmount");
	// Eat or drink specific item. delete it when gone.
	// return: the amount we used.

	word iQtyMax = GetAmount();
	if ( iQty < iQtyMax )
	{
		SetAmountUpdate( iQtyMax - iQty );
		return iQty;
	}

	SetAmount( 0 );	// let there be 0 amount here til decay.
	if ( ! IsTopLevel() || ! IsAttr( ATTR_INVIS ))	// don't delete resource locators.
		Delete();

	return iQtyMax;
}


CREID_TYPE CItem::GetCorpseType() const
{
	return (CREID_TYPE)(GetAmount());	// What does the corpse look like ?
}

void CItem::SetCorpseType( CREID_TYPE id )
{
	// future: strongly typed enums will remove the need for this cast
    ASSERT(id <= UINT16_MAX);
	m_wAmount = (word)id;	// m_corpse_DispID
}

SPELL_TYPE CItem::GetScrollSpell() const
{
	ADDTOCALLSTACK("CItem::GetScrollSpell");
	// Given a scroll type. what spell is this ?
	for (size_t i = SPELL_Clumsy; i < g_Cfg.m_SpellDefs.size(); ++i)
	{
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef((SPELL_TYPE)i);
		if ( pSpellDef == nullptr || pSpellDef->m_idScroll == ITEMID_NOTHING )
			continue;
		if ( GetID() == pSpellDef->m_idScroll )
			return (SPELL_TYPE)i;
	}
	return( SPELL_NONE );
}

bool CItem::IsSpellInBook( SPELL_TYPE spell ) const
{
	ADDTOCALLSTACK("CItem::IsSpellInBook");
	CItemBase *pItemDef = Item_GetDef();
	if ( uint(spell) <= pItemDef->m_ttSpellbook.m_iOffset || spell < 0)
		return false;

	// Convert spell back to format of the book and check whatever it is in.
	const uint i = uint(spell) - (pItemDef->m_ttSpellbook.m_iOffset + 1u);
	if ( i < 32 ) // Replaced the <= with < because of the formula above, the first 32 spells have an i value from 0 to 31 and are stored in more1.
		return ((m_itSpellbook.m_spells1 & (1u << i)) != 0);
	else if ( i < 64 ) // Replaced the <= with < because of the formula above, the remaining 32 spells have an i value from 32 to 63 and are stored in more2.
		return ((m_itSpellbook.m_spells2 & (1u << (i-32))) != 0);
	//else if ( i <= 96 )
	//	return ((m_itSpellbook.m_spells2 & (1u << (i-64))) != 0);	//not used anymore?
	return false;
}

uint CItem::GetSpellcountInBook() const
{
	ADDTOCALLSTACK("CItem::GetSpellcountInBook");
	// -1 = can't count
	// n = number of spells

	if ( !IsTypeSpellbook() )
		return uint(-1);

	const CItemBase *pItemDef = Item_GetDef();
	if ( !pItemDef )
		return uint(-1);

	const uint min = pItemDef->m_ttSpellbook.m_iOffset + 1;
	const uint max = pItemDef->m_ttSpellbook.m_iOffset + pItemDef->m_ttSpellbook.m_iMaxSpells;

	uint count = 0;
	for ( uint i = min; i <= max; ++i )
	{
		if ( IsSpellInBook((SPELL_TYPE)i) )
			++count;
	}

	return count;
}

SKILL_TYPE CItem::GetSpellBookSkill()
{
	ADDTOCALLSTACK("CItem::GetSpellBookSkill");
	ASSERT(IsTypeSpellbook());
	switch (GetType())
	{
		case IT_SPELLBOOK:
			return SKILL_MAGERY;
		case IT_SPELLBOOK_NECRO:
			return SKILL_NECROMANCY;
		case IT_SPELLBOOK_PALA:
			return SKILL_CHIVALRY;
		case IT_SPELLBOOK_BUSHIDO:
			return SKILL_BUSHIDO;
		case IT_SPELLBOOK_NINJITSU:
			return SKILL_NINJITSU;
		case IT_SPELLBOOK_ARCANIST:
			return SKILL_SPELLWEAVING;
		case IT_SPELLBOOK_MYSTIC:
			return SKILL_MYSTICISM;
		case IT_SPELLBOOK_MASTERY:
		case IT_SPELLBOOK_EXTRA:
		default:
			break;
	}
	return SKILL_NONE;// SKILL_NONE returns 1000+ index in CChar::Spell_GetIndex()
}

uint CItem::AddSpellbookSpell( SPELL_TYPE spell, bool fUpdate )
{
	ADDTOCALLSTACK("CItem::AddSpellbookSpell");
	// Add  this scroll to the spellbook.
	// 0 = added.
	// 1 = already here.
	// 2 = not a scroll i know about.
	// 3 = not a valid spellbook

	if ( !IsTypeSpellbook() )
		return 3;
	const CItemBase *pBookDef = Item_GetDef();
	if ( uint(spell) <= pBookDef->m_ttSpellbook.m_iOffset )
		return 3;
	const CSpellDef *pSpellDef = g_Cfg.GetSpellDef(spell);
	if ( !pSpellDef )
		return 2;
	if ( IsSpellInBook(spell) )
		return 1;

	// Add spell to spellbook bitmask:
	const uint i = spell - (pBookDef->m_ttSpellbook.m_iOffset + 1);
	if ( i < 32u ) // Replaced the <= with < because of the formula above, the first 32 spells have an i value from 0 to 31 and are stored in more1.
		m_itSpellbook.m_spells1 |= (1 << i);
	else if ( i < 64u ) // Replaced the <= with < because of the formula above, the remaining 32 spells have an i value from 32 to 63 and are stored in more2.
		m_itSpellbook.m_spells2 |= (1 << (i-32u)); 
	//else if ( i <= 96 )
	//	m_itSpellbook.m_spells3 |= (1 << (i-64));	//not used anymore?
	else
		return 3;

	auto pTopObj = static_cast<CObjBase*>(GetTopLevelObj());
	if (pTopObj)
	{
		// Intercepting the spell's addition here for NPCs, they store the spells on vector <Spells>m_spells for better access from their AI
		CChar *pChar = dynamic_cast<CChar *>(pTopObj);
		if ( pChar && pChar->m_pNPC)
			pChar->m_pNPC->Spells_Add(spell);
	}

	if ( fUpdate )	// update the spellbook
	{
		PacketItemContainer cmd(this, pSpellDef);

		ClientIterator it;
		for ( CClient *pClient = it.next(); pClient != nullptr; pClient = it.next() )
		{
			if ( !pClient->CanSee(this) )
				continue;

			cmd.completeForTarget(pClient, this);
			cmd.send(pClient);
		}
	}

	UpdatePropertyFlag();
	return 0;
}

uint CItem::AddSpellbookScroll( CItem * pScroll )
{
	ADDTOCALLSTACK("CItem::AddSpellbookScroll");
	// Add  this scroll to the spellbook.
	// 0 = added.
	// 1 = already here.
	// 2 = not a scroll i know about.

	ASSERT(pScroll);
	uint iRet = AddSpellbookSpell( pScroll->GetScrollSpell(), true );
	if ( iRet > 0)
		return iRet;
	pScroll->ConsumeAmount(1);	// we only need 1 scroll.
	return 0;
}

void CItem::Flip()
{
	ADDTOCALLSTACK("CItem::Flip");
	// Equivelant rotational objects.
	// These objects are all the same except they are rotated.

	if ( Can(CAN_I_LIGHT) )		// with the standard WoldItem packet (0x1A) the item can be flippable OR a light source, not both
	{
		if ( ++m_itLight.m_pattern >= LIGHT_QTY )
			m_itLight.m_pattern = 0;
		Update();
		return;
	}

	// Doors are easy.
	if ( IsType( IT_DOOR ) || IsType( IT_DOOR_LOCKED ) || IsType(IT_DOOR_OPEN))
	{
		ITEMID_TYPE id = GetDispID();
		int doordir = CItemBase::IsID_Door( id ) - 1;
		int iNewID = (id - doordir) + (( doordir & ~DOOR_OPENED ) + 2 ) % 16; // next closed door type.
		SetDispID((ITEMID_TYPE)iNewID);
		Update();
		return;
	}

	if ( IsType( IT_CORPSE ))
	{
		m_itCorpse.m_facing_dir = GetDirTurn(m_itCorpse.m_facing_dir, 1);
		Update();
		return;
	}

	CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);

	// Try to rotate the object.
	ITEMID_TYPE id = pItemDef->GetNextFlipID( GetDispID());
	if ( id != GetDispID())
	{
		SetDispID( id );
		Update();
	}
}

bool CItem::Use_Portculis()
{
	ADDTOCALLSTACK("CItem::Use_Portculis");
	// Set to the new z location.
	if ( ! IsTopLevel())
		return false;

	CPointMap pt = GetTopPoint();
	if ( pt.m_z == m_itPortculis.m_z1 )
		pt.m_z = (char)(m_itPortculis.m_z2);
	else
		pt.m_z = (char)(m_itPortculis.m_z1);

	if ( pt.m_z == GetTopZ())
		return false;

	MoveToUpdate( pt );

	SOUND_TYPE iSnd = 0x21d;
	if (GetDefNum("PORTCULISSOUND"))
		iSnd = (SOUND_TYPE)(GetDefNum("PORTCULISSOUND"));
	/*CVarDefCont * pTagStorage = nullptr;
	pTagStorage = GetKey("OVERRIDE.PORTCULISSOUND", true);
	if ( pTagStorage )
	{
		if ( pTagStorage->GetValNum() )
			iSnd = (SOUND_TYPE)(pTagStorage->GetValNum());
		else
			iSnd = 0x21d;
	} else
		iSnd = 0x21d;*/

	Sound( iSnd );

	return true;
}

SOUND_TYPE CItem::Use_Music( bool fWell ) const
{
	ADDTOCALLSTACK("CItem::Use_Music");
	const CItemBase * pItemDef = Item_GetDef();
	return (SOUND_TYPE)(fWell ? ( pItemDef->m_ttMusical.m_iSoundGood ) : ( pItemDef->m_ttMusical.m_iSoundBad ));
}

bool CItem::IsDoorOpen() const
{
	ADDTOCALLSTACK("CItem::IsDoorOpen");
	return CItemBase::IsID_DoorOpen( GetDispID());
}

bool CItem::Use_DoorNew( bool bJustOpen )
{
	ADDTOCALLSTACK("CItem::Use_DoorNew");

	if (! IsTopLevel())
		return false;

	bool bClosing = IsAttr(ATTR_OPENED);
	if ( bJustOpen && bClosing )
		return true;	// links just open

	CItemBase * pItemDef = Item_GetDef();

	//default or override ID
	int64 idOverride = GetDefNum("DOOROPENID");
	ITEMID_TYPE idSwitch = idOverride ? (ITEMID_TYPE)idOverride : (ITEMID_TYPE)pItemDef->m_ttDoor.m_ridSwitch.GetResIndex();
	if (!idSwitch)
		return Use_Door(bJustOpen);

	//default or override locations
	short sDifX = m_itNormal.m_morep.m_x ? m_itNormal.m_morep.m_x : (short)(pItemDef->m_ttDoor.m_iXChange);
	short sDifY = m_itNormal.m_morep.m_y ? m_itNormal.m_morep.m_y : (short)(pItemDef->m_ttDoor.m_iYChange);


	//default sounds
	SOUND_TYPE iCloseSnd = pItemDef->m_ttDoor.m_iSoundClose ? pItemDef->m_ttDoor.m_iSoundClose : 0x00f1;
	SOUND_TYPE iOpenSnd = pItemDef->m_ttDoor.m_iSoundOpen ? pItemDef->m_ttDoor.m_iSoundOpen : 0x00ea;

	//override sounds
	int64 sndCloseOverride = GetDefNum("DOORCLOSESOUND");
	if (sndCloseOverride)
		iCloseSnd = (SOUND_TYPE)sndCloseOverride;
	int64 sndOpenOverride = GetDefNum("DOOROPENSOUND");
	if (sndOpenOverride)
		iOpenSnd = (SOUND_TYPE)sndOpenOverride;

	CPointMap pt = GetTopPoint();
	if (bClosing)
	{
		pt.m_x -= sDifX;
		pt.m_y -= sDifY;
	}
	else
	{
		pt.m_x += sDifX;
		pt.m_y += sDifY;
	}

	SetDefNum("DOOROPENID", GetDispID());
	SetDispID(idSwitch);

	MoveToUpdate(pt);
	Sound( bClosing ? iCloseSnd : iOpenSnd );
	_SetTimeoutS( bClosing ? -1 : 20);
	bClosing ? ClrAttr(ATTR_OPENED) : SetAttr(ATTR_OPENED);
	return( ! bClosing );
}

bool CItem::Use_Door( bool fJustOpen )
{
	ADDTOCALLSTACK("CItem::Use_Door");
	// don't call this directly but call CChar::Use_Item() instead.
	// don't check to see if it is locked here
	// RETURN:
	//  true = open

	ITEMID_TYPE id = GetDispID();
	int doordir = CItemBase::IsID_Door( id )-1;
	if ( doordir < 0 || ! IsTopLevel())
		return false;

	id = (ITEMID_TYPE)(id - doordir);
	// IT_TYPE typelock = m_type;

	bool fClosing = ( doordir & DOOR_OPENED );	// currently open
	if ( fJustOpen && fClosing )
		return true;	// links just open

	CPointMap pt = GetTopPoint();
	switch ( doordir )
	{
		case 0:
			pt.m_x--;
			pt.m_y++;
			doordir++;
			break;
		case DOOR_OPENED:
			pt.m_x++;
			pt.m_y--;
			doordir--;
			break;
		case DOOR_RIGHTLEFT:
			pt.m_x++;
			pt.m_y++;
			doordir++;
			break;
		case (DOOR_RIGHTLEFT+DOOR_OPENED):
			pt.m_x--;
			pt.m_y--;
			doordir--;
			break;
		case DOOR_INOUT:
			pt.m_x--;
			doordir++;
			break;
		case (DOOR_INOUT+DOOR_OPENED):
			pt.m_x++;
			doordir--;
			break;
		case (DOOR_INOUT|DOOR_RIGHTLEFT):
			pt.m_x++;
			pt.m_y--;
			doordir++;
			break;
		case (DOOR_INOUT|DOOR_RIGHTLEFT|DOOR_OPENED):
			pt.m_x--;
			pt.m_y++;
			doordir--;
			break;
		case 8:
			pt.m_x++;
			pt.m_y++;
			doordir++;
			break;
		case 9:
			pt.m_x--;
			pt.m_y--;
			doordir--;
			break;
		case 10:
			pt.m_x++;
			pt.m_y--;
			doordir++;
			break;
		case 11:
			pt.m_x--;
			pt.m_y++;
			doordir--;
			break;
		case 12:
			doordir++;
			break;
		case 13:
			doordir--;
			break;
		case 14:
			pt.m_y--;
			doordir++;
			break;
		case 15:
			pt.m_y++;
			doordir--;
			break;
	}

	SetDispID((ITEMID_TYPE)(id + doordir));
	// SetType( typelock );	// preserve the fact that it was locked.
	MoveToUpdate(pt);

	//CVarDefCont * pTagStorage = nullptr;
	SOUND_TYPE iCloseSnd = 0x00f1;
	SOUND_TYPE iOpenSnd = 0x00ea;

	switch ( id )
	{
		case ITEMID_DOOR_SECRET_1:
		case ITEMID_DOOR_SECRET_2:
		case ITEMID_DOOR_SECRET_3:
		case ITEMID_DOOR_SECRET_4:
		case ITEMID_DOOR_SECRET_5:
		case ITEMID_DOOR_SECRET_6:
			iCloseSnd = 0x002e;
			iOpenSnd = 0x002f;
			break;
		case ITEMID_DOOR_METAL_S:
		case ITEMID_DOOR_BARRED:
		case ITEMID_DOOR_METAL_L:
		case ITEMID_DOOR_IRONGATE_1:
		case ITEMID_DOOR_IRONGATE_2:
			iCloseSnd = 0x00f3;
			iOpenSnd = 0x00eb;
			break;
		default:
			break;
	}

	//override sounds
	int64 sndCloseOverride = GetDefNum("DOORCLOSESOUND");
	if (sndCloseOverride)
		iCloseSnd = (SOUND_TYPE)sndCloseOverride;
	int64 sndOpenOverride = GetDefNum("DOOROPENSOUND");
	if (sndOpenOverride)
		iOpenSnd = (SOUND_TYPE)sndOpenOverride;

	/*pTagStorage = GetKey("OVERRIDE.DOORSOUND_CLOSE", true);
	if ( pTagStorage )
		iCloseSnd = (SOUND_TYPE)(pTagStorage->GetValNum());
	pTagStorage = nullptr;
	pTagStorage = GetKey("OVERRIDE.DOORSOUND_OPEN", true);
	if ( pTagStorage )
		iOpenSnd = (SOUND_TYPE)(pTagStorage->GetValNum());*/

	Sound( fClosing ? iCloseSnd : iOpenSnd );

	// Auto close the door in n seconds.
	_SetTimeoutS( fClosing ? -1 : 60);
	return ( ! fClosing );
}

bool CItem::Armor_IsRepairable() const
{
	ADDTOCALLSTACK("CItem::Armor_IsRepairable");
	// We might want to check based on skills:
	// SKILL_BLACKSMITHING (armor)
	// SKILL_BOWERY (xbows)
	// SKILL_TAILORING (leather)
	//

	if ( Can( CAN_I_REPAIR ) )
		return true;

	switch ( m_type )
	{
		case IT_CLOTHING:
		case IT_ARMOR_LEATHER:
			return false;	// Not this way anyhow.
		case IT_SHIELD:
		case IT_ARMOR:				// some type of armor. (no real action)
			// ??? Bone armor etc is not !
			break;
		case IT_WEAPON_MACE_CROOK:
		case IT_WEAPON_MACE_PICK:
		case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
		case IT_WEAPON_SWORD:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_AXE:
		case IT_WEAPON_THROWING:
        case IT_WEAPON_WHIP:
			break;

		case IT_WEAPON_BOW:
			// wood Bows are not repairable !
			return false;
		case IT_WEAPON_XBOW:
			return true;
		default:
			return false;
	}

	return true;
}

int CItem::Armor_GetDefense() const
{
	ADDTOCALLSTACK("CItem::Armor_GetDefense");
	// Get the defensive value of the armor. plus magic modifiers.
	// Subtract for low hitpoints.
	if ( ! IsTypeArmor())
		return 0;

	int iVal = m_defenseBase + m_ModAr;
	if ( IsSetOF(OF_ScaleDamageByDurability) && m_itArmor.m_wHitsMax > 0 && m_itArmor.m_dwHitsCur < m_itArmor.m_wHitsMax )
	{
		int iRepairPercent = 50 + ((50 * m_itArmor.m_dwHitsCur) / m_itArmor.m_wHitsMax);
		iVal = (int)IMulDivLL( iVal, iRepairPercent, 100 );
	}
	if ( IsAttr(ATTR_MAGIC) )
		iVal += g_Cfg.GetSpellEffect( SPELL_Enchant, m_itArmor.m_spelllevel );
	if ( iVal < 0 )
		iVal = 0;
	return iVal;
}

int CItem::Weapon_GetAttack(bool fGetRange) const
{
	ADDTOCALLSTACK("CItem::Weapon_GetAttack");
	// Get the base attack for the weapon plus magic modifiers.
	// Subtract for low hitpoints.
	if ( ! IsTypeWeapon())	// anything can act as a weapon.
		return 1;

	int iVal = m_attackBase + m_ModAr;
	if ( fGetRange )
		iVal += m_attackRange;

	if ( IsSetOF(OF_ScaleDamageByDurability) && m_itArmor.m_wHitsMax > 0 && m_itArmor.m_dwHitsCur < m_itArmor.m_wHitsMax )
	{
		int iRepairPercent = 50 + ((50 * m_itArmor.m_dwHitsCur) / m_itArmor.m_wHitsMax);
		iVal = (int)IMulDivLL( iVal, iRepairPercent, 100 );
	}
	if ( IsAttr(ATTR_MAGIC) && ! IsType(IT_WAND))
		iVal += g_Cfg.GetSpellEffect( SPELL_Enchant, m_itArmor.m_spelllevel );
	if ( iVal < 0 )
		iVal = 0;
	return iVal;
}

SKILL_TYPE CItem::Weapon_GetSkill() const
{
	ADDTOCALLSTACK("CItem::Weapon_GetSkill");
	// assuming this is a weapon. What skill does it apply to.

	CItemBase * pItemDef = Item_GetDef();
	ASSERT(pItemDef);

	int iSkillOverride = (int)(m_TagDefs.GetKeyNum("OVERRIDE_SKILL") - 1);
	if ( iSkillOverride == -1)
		iSkillOverride = (int)(m_TagDefs.GetKeyNum("OVERRIDE.SKILL") - 1);
	if ( (iSkillOverride > SKILL_NONE) && (iSkillOverride < (int)g_Cfg.m_iMaxSkill) )
		return (SKILL_TYPE)(iSkillOverride);

	if ( (pItemDef->m_iSkill > SKILL_NONE) && (pItemDef->m_iSkill < (SKILL_TYPE)g_Cfg.m_iMaxSkill) )
		return pItemDef->m_iSkill;

	switch ( pItemDef->GetType() )
	{
		case IT_WEAPON_MACE_CROOK:
		case IT_WEAPON_MACE_PICK:
		case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
        case IT_WEAPON_WHIP:
			return SKILL_MACEFIGHTING;
		case IT_WEAPON_SWORD:
		case IT_WEAPON_AXE:
			return SKILL_SWORDSMANSHIP;
		case IT_WEAPON_FENCE:
			return SKILL_FENCING;
		case IT_WEAPON_BOW:
		case IT_WEAPON_XBOW:
			return SKILL_ARCHERY;
		case IT_WEAPON_THROWING:
			return SKILL_THROWING;
		default:
			return SKILL_WRESTLING;
	}
}

SOUND_TYPE CItem::Weapon_GetSoundHit() const
{
	ADDTOCALLSTACK("CItem::Weapon_GetSoundHit");
	// Get ranged weapon ammo hit sound if present.

	int iAmmoSoundHit = GetPropNum(COMP_PROPS_ITEMWEAPONRANGED, PROPIWEAPRNG_AMMOSOUNDHIT, true);
	if (iAmmoSoundHit > 0)
		return (SOUND_TYPE)iAmmoSoundHit;
	return SOUND_NONE;
}

SOUND_TYPE CItem::Weapon_GetSoundMiss() const
{
	ADDTOCALLSTACK("CItem::Weapon_GetSoundMiss");
	// Get ranged weapon ammo miss sound if present.

	int iAmmoSoundMiss = GetPropNum(COMP_PROPS_ITEMWEAPONRANGED, PROPIWEAPRNG_AMMOSOUNDMISS, true);
	if ( iAmmoSoundMiss > 0 )
		return (SOUND_TYPE)iAmmoSoundMiss;
	return SOUND_NONE;
}

void CItem::Weapon_GetRangedAmmoAnim(ITEMID_TYPE &id, dword &hue, dword &render)
{
	ADDTOCALLSTACK("CItem::Weapon_GetRangedAmmoAnim");
	// Get animation properties of this ranged weapon (archery/throwing)

	CSString sAmmoAnim = GetPropStr(COMP_PROPS_ITEMWEAPONRANGED, PROPIWEAPRNG_AMMOANIM, true, true);
	if (!sAmmoAnim.IsEmpty())
	{
		CResourceID rid(g_Cfg.ResourceGetID(RES_ITEMDEF, sAmmoAnim));
		id = (ITEMID_TYPE)rid.GetResIndex();
	}
	else
	{
		const CItemBase *pWeaponDef = Item_GetDef();
		if (pWeaponDef)
			id = (ITEMID_TYPE)(pWeaponDef->m_ttWeaponBow.m_ridAmmoX.GetResIndex());
	}

	dword dwAmmoHue = GetPropNum(COMP_PROPS_ITEMWEAPONRANGED, PROPIWEAPRNG_AMMOANIMHUE, true);

	if (dwAmmoHue > 0)
		hue = dwAmmoHue;

	dword dwAmmoRender = GetPropNum(COMP_PROPS_ITEMWEAPONRANGED, PROPIWEAPRNG_AMMOANIMRENDER, true);
	if (dwAmmoRender > 0)
		render = dwAmmoRender;
}

CResourceID CItem::Weapon_GetRangedAmmoRes()
{
	ADDTOCALLSTACK("CItem::Weapon_GetRangedAmmoRes");
	// Get ammo resource id of this ranged weapon (archery/throwing)

    CSString sAmmoID = GetPropStr(COMP_PROPS_ITEMWEAPONRANGED, PROPIWEAPRNG_AMMOTYPE, true, true);
	if ( !sAmmoID.IsEmpty() )
	{
		lpctstr pszAmmoID = sAmmoID.GetBuffer();
		return g_Cfg.ResourceGetID(RES_ITEMDEF, pszAmmoID);
	}

	CItemBase *pItemDef = Item_GetDef();
    pItemDef->m_ttWeaponBow.m_ridAmmo.FixRes();
	return pItemDef->m_ttWeaponBow.m_ridAmmo;
}

CItem *CItem::Weapon_FindRangedAmmo(const CResourceID& id)
{
	ADDTOCALLSTACK("CItem::Weapon_FindRangedAmmo");
	// Find ammo used by this ranged weapon (archery/throwing)

	// Get the container to search
	CContainer *pParent = dynamic_cast<CContainer *>(dynamic_cast<CObjBase *>(GetParent()));
	
	CSString sAmmoCont = GetPropStr(COMP_PROPS_ITEMWEAPONRANGED, PROPIWEAPRNG_AMMOCONT, true,true);
	if ( !sAmmoCont.IsEmpty())
	{
		// Search container using UID
		lpctstr  ptcAmmoCont = sAmmoCont.GetBuffer();
		CContainer *pCont = dynamic_cast<CContainer *>(CUID::ItemFindFromUID(Exp_GetDWVal(ptcAmmoCont)));
		if ( pCont )
		{
            //If the container exist that means the uid was a valid container uid.
			return pCont->ContentFind(id);
		}
		else
		{
            // Search container using ITEMID_TYPE
            if (!pParent)
                return nullptr;

			//Reassigned the value from sAmmoCont.GetBuffer() because Exp_GetDWal clears it 
			ptcAmmoCont = sAmmoCont.GetBuffer();
			const CResourceID ridCont(g_Cfg.ResourceGetID(RES_ITEMDEF, ptcAmmoCont));
			pCont = dynamic_cast<CContainer *>(pParent->ContentFind(ridCont));
			if (pCont)
				return pCont->ContentFind(id);
			return nullptr;
		}

		//CVarDefCont *pVarCont = GetDefKey("AMMOCONT", true);
		/*if ( pVarCont )
		{
			// Search container using UID
			CUID uidCont((dword)pVarCont->GetValNum());
			CContainer *pCont = dynamic_cast<CContainer *>(uidCont.ItemFind());
			if ( pCont )
				return pCont->ContentFind(id);

			// Search container using ITEMID_TYPE
			if ( pParent )
			{
				lpctstr pszContID = pVarCont->GetValStr();
				CResourceID ridCont(g_Cfg.ResourceGetID(RES_ITEMDEF, pszContID));
				pCont = dynamic_cast<CContainer *>(pParent->ContentFind(ridCont));
				if ( pCont )
					return pCont->ContentFind(id);
			}
			return nullptr;
		}*/
	}

	// Search on parent container if there's no specific container to search
	if ( pParent )
		return pParent->ContentFind(id);

	return nullptr;
}

bool CItem::IsMemoryTypes( word wType ) const
{
	if ( ! IsType( IT_EQ_MEMORY_OBJ ))
		return false;
	return (( GetHue() & wType ) ? true : false );
}

lpctstr CItem::Use_SpyGlass( CChar * pUser ) const
{
	ADDTOCALLSTACK("CItem::Use_SpyGlass");
	// IT_SPY_GLASS
	// Assume we are in water now ?

	CPointMap ptCoords = pUser->GetTopPoint();

#define BASE_SIGHT 26 // 32 (UO_MAP_VIEW_RADAR) is the edge of the radar circle (for the most part)
	WEATHER_TYPE wtWeather = ptCoords.GetSector()->GetWeather();
	byte iLight = ptCoords.GetSector()->GetLight();
	CSString sSearch;
	tchar	*pResult = Str_GetTemp();

	// Weather bonus
	double rWeatherSight = wtWeather == WEATHER_RAIN ? (0.25 * BASE_SIGHT) : 0.0;
	// Light level bonus
	double rLightSight = (1.0 - (static_cast<double>(iLight) / 25.0)) * BASE_SIGHT * 0.25;
	int iVisibility = (int) (BASE_SIGHT + rWeatherSight + rLightSight);

	// Check for the nearest land, only check every 4th square for speed
	const CUOMapMeter * pMeter = CWorldMap::GetMapMeter( ptCoords ); // Are we at sea?
	if ( pMeter == nullptr )
		return pResult;

	switch ( pMeter->m_wTerrainIndex )
	{
		case TERRAIN_WATER1:
		case TERRAIN_WATER2:
		case TERRAIN_WATER3:
		case TERRAIN_WATER4:
		case TERRAIN_WATER5:
		case TERRAIN_WATER6:
		{
			// Look for land if at sea
			CPointMap ptLand;
			for (int x = ptCoords.m_x - iVisibility; x <= (ptCoords.m_x + iVisibility); x += 2)
			{
				for (int y = ptCoords.m_y - iVisibility; y <= (ptCoords.m_y + iVisibility); y += 2)
				{
					CPointMap ptCur((word)(x), (word)(y), ptCoords.m_z);
					pMeter = CWorldMap::GetMapMeter( ptCur );
					if ( pMeter == nullptr )
						continue;

					switch ( pMeter->m_wTerrainIndex )
					{
						case TERRAIN_WATER1:
						case TERRAIN_WATER2:
						case TERRAIN_WATER3:
						case TERRAIN_WATER4:
						case TERRAIN_WATER5:
						case TERRAIN_WATER6:
							break;
						default:
							if (ptCoords.GetDist(ptCur) < ptCoords.GetDist(ptLand))
								ptLand = ptCur;
							break;
					}
				}
			}

			if ( ptLand.IsValidPoint())
				sSearch.Format( "%s %s. ", g_Cfg.GetDefaultMsg(DEFMSG_USE_SPYGLASS_LAND), CPointBase::sm_szDirs[ ptCoords.GetDir(ptLand) ] );
			else if (iLight > 3)
				sSearch = g_Cfg.GetDefaultMsg(DEFMSG_USE_SPYGLASS_DARK);
			else if (wtWeather == WEATHER_RAIN)
				sSearch = g_Cfg.GetDefaultMsg(DEFMSG_USE_SPYGLASS_WEATHER);
			else
				sSearch = g_Cfg.GetDefaultMsg(DEFMSG_USE_SPYGLASS_NO_LAND);
			Str_CopyLimitNull( pResult, sSearch, STR_TEMPLENGTH );
			break;
		}

		default:
			pResult[0] = '\0';
			break;
	}

	// Check for interesting items, like boats, carpets, etc.., ignore our stuff
	CItem * pItemSighted = nullptr;
	CItem * pBoatSighted = nullptr;
	int iItemSighted = 0;
	int iBoatSighted = 0;
	CWorldSearch ItemsArea( ptCoords, iVisibility );
	for (;;)
	{
		CItem * pItem = ItemsArea.GetItem();
		if ( pItem == nullptr )
			break;
		if ( pItem == this )
			continue;

		int iDist = ptCoords.GetDist(pItem->GetTopPoint());
		if ( iDist > iVisibility ) // See if it's beyond the "horizon"
			continue;
		if ( iDist <= 8 ) // spyglasses are fuzzy up close.
			continue;

		// Skip items linked to a ship or multi
		if ( pItem->m_uidLink.IsValidUID() )
		{
			CItem * pItemLink = pItem->m_uidLink.ItemFind();
			if (( pItemLink ) && ( pItemLink->IsTypeMulti() ))
					continue;
		}

		// Track boats separately from other items
		if ( iDist <= UO_MAP_VIEW_RADAR && // if it's visible in the radar window as a boat, report it
			pItem->m_type == IT_SHIP )
		{
			++iBoatSighted; // Keep a tally of how many we see
			if (!pBoatSighted || iDist < ptCoords.GetDist(pBoatSighted->GetTopPoint())) // Only find closer items to us
			{
				pBoatSighted = pItem;
			}
		}
		else
		{
			++ iItemSighted; // Keep a tally of how much we see
			if (!pItemSighted || iDist < ptCoords.GetDist(pItemSighted->GetTopPoint())) // Only find the closest item to us, give boats a preference
			{
				pItemSighted = pItem;
			}
		}
	}
	if (iBoatSighted) // Report boat sightings
	{
		DIR_TYPE dir = ptCoords.GetDir(pBoatSighted->GetTopPoint());
		if (iBoatSighted == 1)
			sSearch.Format(g_Cfg.GetDefaultMsg(DEFMSG_SHIP_SEEN_SHIP_SINGLE), pBoatSighted->GetName(), CPointBase::sm_szDirs[dir]);
		else
			sSearch.Format(g_Cfg.GetDefaultMsg(DEFMSG_SHIP_SEEN_SHIP_MANY), CPointBase::sm_szDirs[dir]);
		strcat( pResult, sSearch);
	}

	if (iItemSighted) // Report item sightings, also boats beyond the boat visibility range in the radar screen
	{
		int iDist = ptCoords.GetDist(pItemSighted->GetTopPoint());
		DIR_TYPE dir = ptCoords.GetDir(pItemSighted->GetTopPoint());
		if (iItemSighted == 1)
		{
			if ( iDist > UO_MAP_VIEW_RADAR) // if beyond ship visibility in the radar window, don't be specific
				sSearch.Format(g_Cfg.GetDefaultMsg(DEFMSG_SHIP_SEEN_STH_DIR), static_cast<lpctstr>(CPointBase::sm_szDirs[ dir ]) );
			else
				sSearch.Format(g_Cfg.GetDefaultMsg(DEFMSG_SHIP_SEEN_ITEM_DIR), static_cast<lpctstr>(pItemSighted->GetNameFull(false)), static_cast<lpctstr>(CPointBase::sm_szDirs[ dir ]) );
		}
		else
		{
			if ( iDist > UO_MAP_VIEW_RADAR) // if beyond ship visibility in the radar window, don't be specific
				sSearch.Format(g_Cfg.GetDefaultMsg(DEFMSG_SHIP_SEEN_ITEM_DIR_MANY), CPointBase::sm_szDirs[ dir ] );
			else
				sSearch.Format(g_Cfg.GetDefaultMsg(DEFMSG_SHIP_SEEN_SPECIAL_DIR), static_cast<lpctstr>(pItemSighted->GetNameFull(false)), static_cast<lpctstr>(CPointBase::sm_szDirs[ dir ]));
		}
		strcat( pResult, sSearch);
	}

	// Check for creatures
	CChar * pCharSighted = nullptr;
	int iCharSighted = 0;
	CWorldSearch AreaChar( ptCoords, iVisibility );
	for (;;)
	{
		CChar * pChar = AreaChar.GetChar();
		if ( pChar == nullptr )
			break;
		if ( pChar == pUser )
			continue;
		if ( pChar->m_pArea->IsFlag(REGION_FLAG_SHIP))
			continue; // skip creatures on ships, etc.
		int iDist = ptCoords.GetDist(pChar->GetTopPoint());
		if ( iDist > iVisibility ) // Can we see it?
			continue;
		++ iCharSighted;
		if ( !pCharSighted || iDist < ptCoords.GetDist(pCharSighted->GetTopPoint())) // Only find the closest char to us
		{
			pCharSighted = pChar;
		}
	}

	if (iCharSighted > 0) // Report creature sightings, don't be too specific (that's what tracking is for)
	{
		DIR_TYPE dir =  ptCoords.GetDir(pCharSighted->GetTopPoint());

		if (iCharSighted == 1)
			sSearch.Format(g_Cfg.GetDefaultMsg(DEFMSG_SHIP_SEEN_CREAT_SINGLE), CPointBase::sm_szDirs[dir] );
		else
			sSearch.Format(g_Cfg.GetDefaultMsg(DEFMSG_SHIP_SEEN_CREAT_MANY), CPointBase::sm_szDirs[dir] );
		strcat( pResult, sSearch);
	}
	return pResult;
}

lpctstr CItem::Use_Sextant( CPointMap pntCoords ) const
{
	ADDTOCALLSTACK("CItem::Use_Sextant");
	// IT_SEXTANT
	return g_Cfg.Calc_MaptoSextant(pntCoords);
}

bool CItem::IsBookWritable() const
{
	return ( (m_itBook.m_ResID.GetPrivateUID() == 0) && (GetTimeStamp() == 0) );
}

bool CItem::IsBookSystem() const	// stored in RES_BOOK
{
	return ( m_itBook.m_ResID.GetResType() == RES_BOOK );
}

bool CItem::Use_Light()
{
	ADDTOCALLSTACK("CItem::Use_Light");
	ASSERT(IsType(IT_LIGHT_OUT) || IsType(IT_LIGHT_LIT));

	if ( IsType(IT_LIGHT_OUT) && (m_itLight.m_burned || IsItemInContainer()) )
		return false;

	ITEMID_TYPE id = (ITEMID_TYPE)(m_TagDefs.GetKeyNum("OVERRIDE_LIGHTID"));
	if ( !id )
	{
		id = (ITEMID_TYPE)Item_GetDef()->m_ttLightSource.m_ridLight.GetResIndex();
		if ( !id )
			return false;
	}

	SetID(id);
	Update();
	UpdatePropertyFlag();

	if ( IsType(IT_LIGHT_LIT) )
	{
		Sound(0x47);
		_SetTimeoutS(60);
		if ( !m_itLight.m_charges )
			m_itLight.m_charges = 20;
	}
	else if ( IsType(IT_LIGHT_OUT) )
	{
		Sound(m_itLight.m_burned ? 0x4b8 : 0x3be);
		SetDecayTime(0);
	}

	return true;
}

int CItem::Use_LockPick( CChar * pCharSrc, bool fTest, bool fFail )
{
	ADDTOCALLSTACK("CItem::Use_LockPick");
	// This is the locked item.
	// Char is using a lock pick on it.
	// SKILL_LOCKPICKING
	// SKILL_MAGERY
	// RETURN:
	//  -1 not possible.
	//  0 = trivial
	//  difficulty of unlocking this (0 100)
	//

	if ( pCharSrc == nullptr )
		return -1;

	if ( ! IsTypeLocked() )
	{
		pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_UNLOCK_CANT_GENERAL ) );
		return -1;
	}

	CChar * pCharTop = dynamic_cast <CChar*>( GetTopLevelObj());
	if ( pCharTop && pCharTop != pCharSrc )
	{
		pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_UNLOCK_CANT_THERE ) );
		return -1;
	}

	// If we have the key allow unlock using spell.
	if ( pCharSrc->ContentFindKeyFor( this ))
	{
		if ( !fTest )
			pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_HAS_KEY ) );

		return 0;
	}

	if ( IsType(IT_DOOR_LOCKED) && g_Cfg.m_iMagicUnlockDoor != -1 )
	{
		if ( g_Cfg.m_iMagicUnlockDoor == 0 )
		{
			pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_UNLOCK_DOOR_NEEDS_KEY ) );
			return -1;
		}

		// you can get flagged criminal for this action.
		if ( ! fTest )
			pCharSrc->CheckCrimeSeen( SKILL_SNOOPING, nullptr, this, g_Cfg.GetDefaultMsg( DEFMSG_LOCK_PICK_CRIME ) );

		if ( Calc_GetRandVal( g_Cfg.m_iMagicUnlockDoor ))
			return 10000;	// plain impossible.
	}

	// If we don't have a key, we still have a *tiny* chance.
	// This is because players could have lockable chests too and we don't want it to be too easy
	if ( ! fTest )
	{
		if ( fFail )
		{
			if ( IsType(IT_DOOR_LOCKED))
				pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_UNLOCK_FAIL_DOOR ) );
			else
				pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_UNLOCK_FAIL_CONT ) );
			return -1;
		}

		// unlock it.
		pCharSrc->Use_KeyChange( this );
	}

	return( m_itContainer.m_dwLockComplexity / 10 );
}

void CItem::SetSwitchState()
{
	ADDTOCALLSTACK("CItem::SetSwitchState");
	if ( m_itSwitch.m_SwitchID )
	{
		ITEMID_TYPE id = m_itSwitch.m_SwitchID;
		m_itSwitch.m_SwitchID = GetDispID();
		SetDispID( id );
	}
}

void CItem::SetTrapState( IT_TYPE state, ITEMID_TYPE id, int iTimeSec )
{
	ADDTOCALLSTACK("CItem::SetTrapState");
	ASSERT( IsType(IT_TRAP) || IsType(IT_TRAP_ACTIVE) || IsType(IT_TRAP_INACTIVE) );
	ASSERT( state == IT_TRAP || state == IT_TRAP_ACTIVE || state == IT_TRAP_INACTIVE );

	if ( ! id )
	{
		id = m_itTrap.m_AnimID;
		if ( ! id )
			id = (ITEMID_TYPE)( GetDispID() + 1 );
	}
	if ( ! iTimeSec )
		iTimeSec = 3* MSECS_PER_SEC;
	else if ( iTimeSec > 0 && iTimeSec < UINT16_MAX )
		iTimeSec *= MSECS_PER_SEC;
	else
		iTimeSec = -1;

	if ( id != GetDispID())
	{
		m_itTrap.m_AnimID = GetDispID(); // save old id.
		SetDispID( id );
	}

	SetType( state );
	_SetTimeout( iTimeSec );
	Update();
}

int CItem::Use_Trap()
{
	ADDTOCALLSTACK("CItem::Use_Trap");
	// We activated a trap somehow.
	// We might not be close to it tho.
	// RETURN:
	//   The amount of damage.

	ASSERT( m_type == IT_TRAP || m_type == IT_TRAP_ACTIVE );
	if ( m_type == IT_TRAP )
	{
		SetTrapState( IT_TRAP_ACTIVE, m_itTrap.m_AnimID, m_itTrap.m_wAnimSec );
	}

	if ( ! m_itTrap.m_iDamage ) m_itTrap.m_iDamage = 2;
	return m_itTrap.m_iDamage;	// base damage done.
}

bool CItem::SetMagicLock( CChar * pCharSrc, int iSkillLevel )
{
	ADDTOCALLSTACK("CItem::SetMagicLock");
	UNREFERENCED_PARAMETER(iSkillLevel);
	if ( pCharSrc == nullptr )
		return false;

	if ( IsTypeLocked() )
	{
		pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_ALREADY_IS ) );
		return false;
	}
	if ( ! IsTypeLockable() )
	{
		// Maybe lock items to the ground ?
		pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_UNLOCKEABLE ) );
		return false;
	}
   if ( IsAttr( ATTR_OWNED ) )
	{
		pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_MAY_NOT ) );
		return false;
	}
	switch ( m_type )
	{
		// Have to check for keys here or people would be
		// locking up dungeon chests.
		case IT_CONTAINER:
			if ( pCharSrc->ContentFindKeyFor( this ))
			{
				SetType(IT_CONTAINER_LOCKED);
				pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_CONT_OK ) );
			}
			else
			{
				pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_CONT_NO_KEY ) );
				return false;
			}
			break;

			// Have to check for keys here or people would be
			// locking all the doors in towns
		case IT_DOOR:
		case IT_DOOR_OPEN:
			if ( pCharSrc->ContentFindKeyFor( this ))
			{
				SetType(IT_DOOR_LOCKED);
				pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_DOOR_OK ) );
			}
			else
			{
				pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_DOOR_NO_KEY ) );
				return false;
			}
			break;
		case IT_SHIP_HOLD:
			if ( pCharSrc->ContentFindKeyFor( this ))
			{
				SetType(IT_SHIP_HOLD_LOCK);
				pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_HOLD_OK ) );
			}
			else
			{
				pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_HOLD_NO_KEY ) );
				return false;
			}
			break;
		case IT_SHIP_SIDE:
			SetType(IT_SHIP_SIDE_LOCKED);
			pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_SHIP_OK ) );
			break;
		default:
			pCharSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_LOCK_CAN_NOT ) );
			return false;
	}

	return true;
}

bool CItem::OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool bReflecting )
{
	ADDTOCALLSTACK("CItem::OnSpellEffect");
	UNREFERENCED_PARAMETER(bReflecting);	// items are not affected by Magic Reflection
    // A spell is cast on this item.
    // ARGS:
    //  iSkillLevel = 0-1000 = difficulty. may be slightly larger . how advanced is this spell (might be from a wand)

	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	CScriptTriggerArgs Args( spell, iSkillLevel, pSourceItem );

	if ( IsTrigUsed(TRIGGER_SPELLEFFECT) || IsTrigUsed(TRIGGER_ITEMSPELL) )
	{
		switch ( OnTrigger(ITRIG_SPELLEFFECT, pCharSrc, &Args) )
		{
			case TRIGRET_RET_TRUE:
				return false;
			case TRIGRET_RET_FALSE:
				if ( pSpellDef && pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) )
					return true;
				break;
			default:
				break;
		}
	}

	if ( IsTrigUsed(TRIGGER_EFFECT) )
	{
		switch (Spell_OnTrigger(spell, SPTRIG_EFFECT, pCharSrc, &Args))
		{
			case TRIGRET_RET_TRUE:
				return false;
			case TRIGRET_RET_FALSE:
				if ( pSpellDef && pSpellDef->IsSpellType(SPELLFLAG_SCRIPTED) )
					return true;
				break;
			default:
				break;
		}
	}

	spell = (SPELL_TYPE)(Args.m_iN1);
	iSkillLevel = (int)(Args.m_iN2);
	pSpellDef = g_Cfg.GetSpellDef( spell );


	if ( IsType(IT_WAND) )	// try to recharge the wand.
	{
		if ( !m_itWeapon.m_spell || RES_GET_INDEX(m_itWeapon.m_spell) == (word)spell )
		{
			SetAttr(ATTR_MAGIC);
			if ( !m_itWeapon.m_spell || ( pCharSrc && pCharSrc->IsPriv(PRIV_GM) ) )
			{
				m_itWeapon.m_spell = (word)(spell);
				m_itWeapon.m_spelllevel = (word)(iSkillLevel);
				m_itWeapon.m_spellcharges = 0;
			}

			++m_itWeapon.m_spellcharges;
			UpdatePropertyFlag();
		}
	}

	if ( pCharSrc && (!pCharSrc->IsPriv(PRIV_GM) && pCharSrc->GetRegion()->CheckAntiMagic(spell)) )
	{
		pCharSrc->SysMessageDefault( DEFMSG_SPELL_TRY_AM );
		return false;
	}

	ITEMID_TYPE iEffectID = !pSpellDef ? ITEMID_NOTHING : pSpellDef->m_idEffect;
	DAMAGE_TYPE uiDamageType = 0;
	switch ( spell )
	{
		case SPELL_Dispel_Field:
			if ( GetType() == IT_SPELL )
			{
				if ( IsTopLevel() )
					Effect( EFFECT_XYZ, iEffectID, pCharSrc, 9, 20 );
				Delete();
                return true;
			}
			break;
		case SPELL_Dispel:
		case SPELL_Mass_Dispel:
			if ( GetType() == IT_SPELL )
			{
				if ( IsTopLevel() )
					Effect( EFFECT_XYZ, iEffectID, pCharSrc, 8, 20 );
				Delete();
                return true;
			}
			break;
		case SPELL_Bless:
		case SPELL_Curse:
			return false;
		case SPELL_Lightning:
			Effect( EFFECT_LIGHTNING, ITEMID_NOTHING, pCharSrc );
			break;
		case SPELL_Explosion:
		case SPELL_Fireball:
		case SPELL_Fire_Bolt:
		case SPELL_Fire_Field:
		case SPELL_Flame_Strike:
		case SPELL_Meteor_Swarm:
            uiDamageType = DAMAGE_FIRE;
			break;

		case SPELL_Magic_Lock:
			if ( !SetMagicLock( pCharSrc, iSkillLevel ) )
				return false;
			break;

		case SPELL_Unlock:
		{
			if ( !pCharSrc )
				return false;

			int iDifficulty = Use_LockPick( pCharSrc, true, false );
			if ( iDifficulty < 0 )
				return false;
			bool fSuccess = pCharSrc->Skill_CheckSuccess( SKILL_MAGERY, iDifficulty );
			Use_LockPick( pCharSrc, false, ! fSuccess );
			return fSuccess;
		}

		case SPELL_Mark:
			if ( !pCharSrc )
				return false;

			if ( !pCharSrc->IsPriv(PRIV_GM))
			{
				if ( !IsType(IT_RUNE) && ! IsType(IT_TELEPAD) )
				{
					pCharSrc->SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_SPELL_RECALL_NOTRUNE) );
					return false;
				}
				if ( GetTopLevelObj() != pCharSrc )
				{
					// Prevent people from remarking GM teleport pads if they can't pick it up.
					pCharSrc->SysMessage( g_Cfg.GetDefaultMsg(DEFMSG_SPELL_MARK_CONT) );
					return false;
				}
			}

			m_itRune.m_ptMark = pCharSrc->GetTopPoint();
			if ( IsType(IT_RUNE) )
			{
				m_itRune.m_Strength = pSpellDef->m_vcEffect.GetLinear( iSkillLevel );
				SetName( pCharSrc->m_pArea->GetName() );
			}
			break;
		default:
			break;
	}

	// ??? Potions should explode when hit (etc..)
	if ( pSpellDef->IsSpellType( SPELLFLAG_HARM ))
		OnTakeDamage( 1, pCharSrc, DAMAGE_MAGIC|uiDamageType );

    if ( (iEffectID > ITEMID_NOTHING) && (iEffectID < ITEMID_QTY) )
    {
        bool fExplode = (pSpellDef->IsSpellType(SPELLFLAG_FX_BOLT) && !pSpellDef->IsSpellType(SPELLFLAG_GOOD));		// bolt (chasing) spells have explode = 1 by default (if not good spell)
        dword dwColor = (dword)(Args.m_VarsLocal.GetKeyNum("EffectColor"));
        dword dwRender = (dword)(Args.m_VarsLocal.GetKeyNum("EffectRender"));

        if ( pSpellDef->IsSpellType(SPELLFLAG_FX_BOLT) )
            Effect(EFFECT_BOLT, iEffectID, pCharSrc, 5, 1, fExplode, dwColor, dwRender);
        if ( pSpellDef->IsSpellType(SPELLFLAG_FX_TARG) )
            Effect(EFFECT_OBJ, iEffectID, this, 0, 15, fExplode, dwColor, dwRender);
    }

	return true;
}

int CItem::Armor_GetRepairPercent() const
{
	ADDTOCALLSTACK("CItem::Armor_GetRepairPercent");

	if ( !m_itArmor.m_wHitsMax || ( m_itArmor.m_wHitsMax < m_itArmor.m_dwHitsCur ))
		return 100;
 	return IMulDiv( m_itArmor.m_dwHitsCur, 100, m_itArmor.m_wHitsMax );
}

lpctstr CItem::Armor_GetRepairDesc() const
{
	ADDTOCALLSTACK("CItem::Armor_GetRepairDesc");
	if ( m_itArmor.m_dwHitsCur > m_itArmor.m_wHitsMax )
		return g_Cfg.GetDefaultMsg( DEFMSG_ITEMSTATUS_PERFECT );
	else if ( m_itArmor.m_dwHitsCur == m_itArmor.m_wHitsMax )
		return g_Cfg.GetDefaultMsg( DEFMSG_ITEMSTATUS_FULL );
	else if ( m_itArmor.m_dwHitsCur > m_itArmor.m_wHitsMax / 2 )
		return g_Cfg.GetDefaultMsg( DEFMSG_ITEMSTATUS_SCRATCHED );
	else if ( m_itArmor.m_dwHitsCur > m_itArmor.m_wHitsMax / 3 )
		return g_Cfg.GetDefaultMsg( DEFMSG_ITEMSTATUS_WELLWORN );
	else if ( m_itArmor.m_dwHitsCur > 3 )
		return g_Cfg.GetDefaultMsg( DEFMSG_ITEMSTATUS_BADLY );
	else
		return g_Cfg.GetDefaultMsg( DEFMSG_ITEMSTATUS_FALL_APART );
}

int CItem::OnTakeDamage( int iDmg, CChar * pSrc, DAMAGE_TYPE uType )
{
	ADDTOCALLSTACK("CItem::OnTakeDamage");
	// This will damage the item durability, break stuff, explode potions, etc.
	// Any chance to do/avoid the damage must be checked before OnTakeDamage().
	//
	// pSrc = The source of the damage. May not be the person wearing the item.
	//
	// RETURN:
	//  Amount of damage done.
	//  INT32_MAX = destroyed !!!
	//  -1 = invalid target ?

	if ( iDmg <= 0 )
		return 0;

    const bool fHasMaxHits = IsTypeArmorWeapon();
    if (fHasMaxHits && (m_itArmor.m_wHitsMax > 0))
    {
        const int64 iSelfRepair = GetDefNum("SELFREPAIR", true);
        if (iSelfRepair > Calc_GetRandVal(10))
        {
            const ushort uiOldHits = m_itArmor.m_dwHitsCur;
            m_itArmor.m_dwHitsCur += 2;
            if (m_itArmor.m_dwHitsCur > m_itArmor.m_wHitsMax)
                m_itArmor.m_dwHitsCur = m_itArmor.m_wHitsMax;

            if (uiOldHits != m_itArmor.m_dwHitsCur)
                UpdatePropertyFlag();

            return 0;
        }
    }

	if ( IsTrigUsed(TRIGGER_DAMAGE) || IsTrigUsed(TRIGGER_ITEMDAMAGE) )
	{
		CScriptTriggerArgs Args(iDmg, (int)(uType));
		if ( OnTrigger( ITRIG_DAMAGE, pSrc, &Args ) == TRIGRET_RET_TRUE )
			return 0;
	}

	switch ( GetType() )
	{
	case IT_CLOTHING:
		if ( uType & DAMAGE_FIRE )
		{
			// normal cloth takes special damage from fire.
			goto forcedamage;
		}
		break;

	case IT_WEAPON_ARROW:
	case IT_WEAPON_BOLT:
		if ( iDmg == 1 )
		{
			// Miss - They will usually survive.
			if ( Calc_GetRandVal(5))
				return 0;
		}
		else
		{
			// Must have hit.
			if ( ! Calc_GetRandVal(3))
				return 1;
		}
		Delete();
		return INT32_MAX;

	case IT_POTION:
		if ( RES_GET_INDEX(m_itPotion.m_Type) == SPELL_Explosion )
		{
			CSpellDef *pSpell = g_Cfg.GetSpellDef(SPELL_Explosion);
			if (!pSpell)
				return 0;

			CItem *pItem = CItem::CreateBase(ITEMID_FX_EXPLODE_3);
			if ( !pItem )
				return 0;

			if ( pSrc )
				pItem->m_uidLink = pSrc->GetUID();
			pItem->m_itExplode.m_iDamage = (word)(g_Cfg.GetSpellEffect(SPELL_Explosion, m_itPotion.m_dwSkillQuality));
			pItem->m_itExplode.m_wFlags = pSpell->IsSpellType(SPELLFLAG_NOUNPARALYZE) ? DAMAGE_FIRE|DAMAGE_NOUNPARALYZE : DAMAGE_FIRE;
			pItem->m_itExplode.m_iDist = 2;
			pItem->SetType(IT_EXPLOSION);
			pItem->SetAttr(ATTR_MOVE_NEVER|ATTR_DECAY);
			pItem->MoveToDecay(GetTopLevelObj()->GetTopPoint(), 1);		// almost immediate decay

			ConsumeAmount();
			return( INT32_MAX );
		}
		return 1;

	case IT_WEB:
		if ( ! ( uType & (DAMAGE_FIRE|DAMAGE_HIT_BLUNT|DAMAGE_HIT_SLASH|DAMAGE_GOD)))
		{
			if ( pSrc )
				pSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_WEB_NOEFFECT ) );
			return 0;
		}

		if ( (dword)iDmg > m_itWeb.m_dwHitsCur || ( uType & DAMAGE_FIRE ))
		{
			if ( pSrc )
				pSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_WEB_DESTROY ) );
			Delete();
			return( INT32_MAX );
		}

		if ( pSrc )
			pSrc->SysMessage( g_Cfg.GetDefaultMsg( DEFMSG_WEB_WEAKEN ) );
		m_itWeb.m_dwHitsCur -= iDmg;
		return 1;

	default:
		break;
	}

	// Break armor etc..
	if (fHasMaxHits)
	{
forcedamage:
		CChar * pChar = dynamic_cast <CChar*> ( GetTopLevelObj());

		if ( m_itArmor.m_dwHitsCur <= 1 )
		{
			m_itArmor.m_dwHitsCur = 0;
			if ( g_Cfg.m_iEmoteFlags & EMOTEF_DESTROY )
				EmoteObj( g_Cfg.GetDefaultMsg( DEFMSG_ITEM_DMG_DESTROYED ) );
			else
				Emote( g_Cfg.GetDefaultMsg( DEFMSG_ITEM_DMG_DESTROYED ) );
			Delete();
			return( INT32_MAX );
		}

		const int previousDefense = Armor_GetDefense();
		const int previousDamage = Weapon_GetAttack();

		--m_itArmor.m_dwHitsCur;
		UpdatePropertyFlag();

		if (pChar != nullptr && IsItemEquipped() )
		{
            bool fUpdateStatsFlag = false;

			if ( previousDefense != Armor_GetDefense() )
			{
				pChar->m_defense = (word)(pChar->CalcArmorDefense());
                fUpdateStatsFlag = true;
			}
			else if ( previousDamage != Weapon_GetAttack() )
			{
                fUpdateStatsFlag = true;
			}

            if (fUpdateStatsFlag)
                pChar->UpdateStatsFlag();
		}

		tchar *pszMsg = Str_GetTemp();
		if ( pSrc != nullptr )
		{
			if (pSrc->IsPriv(PRIV_DETAIL))
			{
				// Tell hitter they scored !
				if (pChar && pChar != pSrc)
					snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_ITEM_DMG_DAMAGE1), pChar->GetName(), GetName());
				else
					snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_ITEM_DMG_DAMAGE2), GetName());
				pSrc->SysMessage(pszMsg);
			}
		}
		if ( pChar && pChar != pSrc )
		{
			if (pChar->IsPriv(PRIV_DETAIL))
			{
				// Tell target they got damaged.
				*pszMsg = 0;
				if (m_itArmor.m_dwHitsCur < m_itArmor.m_wHitsMax / 2)
				{
					const int iPercent = Armor_GetRepairPercent();
					if (pChar->Skill_GetAdjusted(SKILL_ARMSLORE) / 10 > iPercent)
						snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_ITEM_DMG_DAMAGE3), GetName(), Armor_GetRepairDesc());
				}
				if (!*pszMsg)
					snprintf(pszMsg, STR_TEMPLENGTH, g_Cfg.GetDefaultMsg(DEFMSG_ITEM_DMG_DAMAGE4), GetName());
				pChar->SysMessage(pszMsg);
			}
		}
		return 2;
	}

	// don't know how to calc damage for this.
	return 0;
}

void CItem::OnExplosion()
{
	ADDTOCALLSTACK("CItem::OnExplosion");
	// IT_EXPLOSION
	// Async explosion.
	// RETURN: true = done. (delete the animation)

	// It can explode both on the ground and on the player's hand, if he doesn't throw it in time.
	ASSERT( m_type == IT_EXPLOSION );

	// AOS damage types (used by COMBAT_ELEMENTAL_ENGINE)
	int iDmgPhysical = 0, iDmgFire = 0, iDmgCold = 0, iDmgPoison = 0, iDmgEnergy = 0;
	if ( m_itExplode.m_wFlags & DAMAGE_FIRE )
		iDmgFire = 100;
	else if ( m_itExplode.m_wFlags & DAMAGE_COLD )
		iDmgCold = 100;
	else if ( m_itExplode.m_wFlags & DAMAGE_POISON )
		iDmgPoison = 100;
	else if ( m_itExplode.m_wFlags & DAMAGE_ENERGY )
		iDmgEnergy = 100;
	else
		iDmgPhysical = 100;

	CChar * pSrc = m_uidLink.CharFind();
	CWorldSearch AreaChars( GetTopPoint(), m_itExplode.m_iDist );
	for (;;)
	{
		CChar * pChar = AreaChars.GetChar();
		if ( pChar == nullptr )
			break;
		if ( pChar->CanSeeLOS(this) )
			pChar->OnTakeDamage( m_itExplode.m_iDamage, pSrc, m_itExplode.m_wFlags, iDmgPhysical, iDmgFire, iDmgCold, iDmgPoison, iDmgEnergy );
	}

	Effect(EFFECT_XYZ, ITEMID_FX_EXPLODE_3, this, 9, 10);
	Sound(0x307);
}

bool CItem::IsResourceMatch( const CResourceID& rid, dword dwArg ) const
{
	ADDTOCALLSTACK("CItem::IsResourceMatch");
	// Check for all the matching special cases.
	// ARGS:
	//  dwArg = specific key or map (for typedefs)

	const CItemBase *pItemDef = Item_GetDef();
	ASSERT(pItemDef);

	if ( rid == pItemDef->GetResourceID() )
		return true;

	RES_TYPE restype = rid.GetResType();

	if ( restype == RES_ITEMDEF )
	{
		if ( IsSetEF(EF_Item_Strict_Comparison) )
			return false;

		ITEMID_TYPE itemid = pItemDef->GetID();
		ITEMID_TYPE index = (ITEMID_TYPE)(rid.GetResIndex());

		switch ( index )
		{
			case ITEMID_LOG_1:		// boards can be used as logs (but logs can't be used as boards)
			{
				if ( itemid == ITEMID_BOARD1 )
					return true;
				break;
			}
			case ITEMID_HIDES:		// leather can be used as hide (but hide can't be used as leather)
			{
				if ( itemid == ITEMID_LEATHER_1 )
					return true;
				break;
			}
			default:
				break;
		}
		return false;
	}
	else if ( restype == RES_TYPEDEF )
	{
		IT_TYPE index = (IT_TYPE)(rid.GetResIndex());
		if ( !IsType(index) )
			return false;

		if ( dwArg )
		{
			switch ( index )
			{
				case IT_MAP:		// different map types are not the same resource
				{
					if ( LOWORD(dwArg) != m_itMap.m_top || HIWORD(dwArg) != m_itMap.m_left )
						return false;
					break;
				}
				case IT_KEY:		// keys with different links are not the same resource
				{
					if ( m_itKey.m_UIDLock != dwArg )
						return false;
					break;
				}
				default:
					break;
			}
		}
		return true;
	}

	return false;
}

CCFaction * CItem::GetSlayer() const
{
    return static_cast<CCFaction*>(GetComponent(COMP_FACTION));
}


void CItem::_GoAwake()
{
	ADDTOCALLSTACK("CItem::_GoAwake");
	CObjBase::_GoAwake();
	
	// Items equipped or inside containers don't receive ticks and need to be added to a list of items to be processed separately
	if (!IsTopLevel())
	{
		CWorldTickingList::AddObjStatusUpdate(this, false);
	}
}

void CItem::_GoSleep()
{
	ADDTOCALLSTACK("CItem::_GoSleep");
	CObjBase::_GoSleep();

	// Items equipped or inside containers don't receive ticks and need to be added to a list of items to be processed separately
	if (IsTopLevel())
	{
		CWorldTickingList::DelObjStatusUpdate(this, false);
	}
}

bool CItem::_OnTick()
{
	ADDTOCALLSTACK("CItem::_OnTick");
	// Timer expired. Time to do something.
	// RETURN: false = delete it.

	EXC_TRY("Tick");

    EXC_SET_BLOCK("sleep check");

	const CSector* pSector = GetTopSector();	// It prints an error if it belongs to an invalid sector.
    if (pSector && pSector->IsSleeping())
    {
		//Make it tick after sector's awakening.
		if (!_IsSleeping())
		{
			_GoSleep();
		}
		_SetTimeout(1);
        return true;
    }


    EXC_SET_BLOCK("timer trigger");
	TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;

	if (( IsTrigUsed(TRIGGER_TIMER) ) || ( IsTrigUsed(TRIGGER_ITEMTIMER) ))
	{
		iRet = OnTrigger( ITRIG_TIMER, &g_Serv );
        if (iRet == TRIGRET_RET_TRUE)
        {
            return true;
        }
	}

    EXC_SET_BLOCK("components ticking");

    /*
    * CComponents ticking:
    * Note that CCRET_TRUE will force a return true and skip default behaviour,
    * CCRET_FALSE will force a return false and delete the item without it's default
    * timer checks.
    * CCRET_CONTINUE will allow the normal ticking proccess to happen.
    */
    const CCRET_TYPE iCompRet = CEntity::_OnTick();
    if (iCompRet != CCRET_CONTINUE) // if return = CCRET_TRUE or CCRET_FALSE
    {
        return !iCompRet;    // Stop here
    }

	EXC_SET_BLOCK("GetType");
	EXC_SET_BLOCK("default behaviour");
	switch ( m_type )
	{
		case IT_CORPSE:
			{
				EXC_SET_BLOCK("default behaviour::IT_CORPSE");
				// turn player corpse into bones
				const CChar * pSrc = m_uidLink.CharFind();
				if ( pSrc && pSrc->m_pPlayer )
				{
                    const CClient* pClient = pSrc->GetClientActive();
                    if (pClient)
                    {
                        pClient->addMapWaypoint(this, MAPWAYPOINT_Remove);	// remove corpse map waypoint on enhanced clients
                    }
					SetID((ITEMID_TYPE)(Calc_GetRandVal2(ITEMID_SKELETON_1, ITEMID_SKELETON_9)));
					SetHue((HUE_TYPE)(HUE_DEFAULT));
					_SetTimeout(g_Cfg.m_iDecay_CorpsePlayer);
					m_itCorpse.m_carved = 1;	// the corpse can't be carved anymore
					m_uidLink.InitUID();		// and also it's not linked to the char anymore (others players can loot it without get flagged criminal)
					RemoveFromView();
					Update();
					return true;
				}
			}
			break;

		case IT_LIGHT_LIT:
			{
				EXC_SET_BLOCK("default behaviour::IT_LIGHT_LIT");
				if ( IsAttr(ATTR_MOVE_NEVER|ATTR_STATIC) )	// infinite charges
					return true;

				--m_itLight.m_charges;
				if (m_itLight.m_charges > 0)
				{
					_SetTimeoutS(60);
				}
				else
				{
					// Burn out the light
					Emote(g_Cfg.GetDefaultMsg(DEFMSG_LIGHTSRC_BURN_OUT));
					m_itLight.m_burned = 1;
					Use_Light();
				}
			}
			return true;

		case IT_SHIP_PLANK:
			{
				EXC_SET_BLOCK("default behaviour::IT_SHIP_PLANK");
				Ship_Plank( false );
			}
			return true;

		case IT_EXPLOSION:
			{
				EXC_SET_BLOCK("default behaviour::IT_EXPLOSION");
				OnExplosion();
			}
			break;

		case IT_TRAP_ACTIVE:
			{
				EXC_SET_BLOCK("default behaviour::IT_TRAP_ACTIVE");
				SetTrapState( IT_TRAP_INACTIVE, m_itTrap.m_AnimID, m_itTrap.m_wAnimSec );
			}
			return true;

		case IT_TRAP_INACTIVE:
			{
				EXC_SET_BLOCK("default behaviour::IT_TRAP_INACTIVE");
				// Set inactive til someone triggers it again.
				if ( m_itTrap.m_bPeriodic )
					SetTrapState( IT_TRAP_ACTIVE, m_itTrap.m_AnimID, m_itTrap.m_wResetSec );
				else
					SetTrapState( IT_TRAP, GetDispID(), -1 );
			}
			return true;

		case IT_ANIM_ACTIVE:
			{
				// reset the anim
				EXC_SET_BLOCK("default behaviour::IT_ANIM_ACTIVE");
				RemoveFromView();
				SetDispID( m_itAnim.m_PrevID );
				m_type = m_itAnim.m_PrevType;   // don't change the components SetType(m_itAnim.m_PrevType);
				_SetTimeout( -1 );
				Update();
			}
			return true;

		case IT_DOOR:
		case IT_DOOR_OPEN:
		case IT_DOOR_LOCKED:	// Temporarily opened locked door.
			{
				// Doors should close.
				EXC_SET_BLOCK("default behaviour::IT_DOOR");
				//Use_Door( false );
				Use_DoorNew( false );
			}
			return true;

		case IT_POTION:
			{
				EXC_SET_BLOCK("default behaviour::IT_POTION");
				// This is a explosion potion?
				if ( (RES_GET_INDEX(m_itPotion.m_Type) == SPELL_Explosion) && m_itPotion.m_ignited )
				{
					if ( m_itPotion.m_tick <= 1 )
					{
						OnTakeDamage( 1, m_uidLink.CharFind(), DAMAGE_FIRE );
					}
					else
					{
						--m_itPotion.m_tick;
						CObjBase* pObj = static_cast<CObjBase*>(GetTopLevelObj());
						ASSERT(pObj);
						tchar* pszMsg = Str_GetTemp();
						pObj->Speak(Str_FromI_Fast(m_itPotion.m_tick, pszMsg, STR_TEMPLENGTH, 10), HUE_RED);
						_SetTimeoutS(1);
					}
					return true;
				}
			}
			break;

		case IT_CROPS:
		case IT_FOLIAGE:
			{
				EXC_SET_BLOCK("default behaviour::IT_CROPS");
				if ( Plant_OnTick())
					return true;
			}
			break;

		case IT_BEE_HIVE:
			{
				EXC_SET_BLOCK("default behaviour::IT_BEE_HIVE");
				// Regenerate honey count
				if ( m_itBeeHive.m_iHoneyCount < 5 )
					++m_itBeeHive.m_iHoneyCount;
				_SetTimeoutS( 15 * 60);
			}
			return true;

		case IT_CAMPFIRE:
			{
				EXC_SET_BLOCK("default behaviour::IT_CAMPFIRE");
				if ( GetID() == ITEMID_EMBERS )
					break;
				SetID( ITEMID_EMBERS );
				SetDecayTime( 2 * 60 * MSECS_PER_SEC);
				Update();
			}
			return true;

		case IT_SIGN_GUMP:	// signs never decay
			{
				EXC_SET_BLOCK("default behaviour::IT_SIGN_GUMP");
			}
			return true;

		default:
			break;
	}

	EXC_SET_BLOCK("default behaviour2");
	if ( IsAttr(ATTR_DECAY) )
		return false;

	EXC_SET_BLOCK("default behaviour3");
	if ( iRet == TRIGRET_RET_FALSE )
		return false;

	EXC_SET_BLOCK("default behaviour4");
	DEBUG_ERR(( "Timer expired without DECAY flag '%s' (UID=0%x)?\n", GetName(), (dword)GetUID()));

    EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("CItem::_OnTick: '%s' item [0%x]\n", GetName(), (dword)GetUID());
	//g_Log.EventError("'%s' item [0%x]\n", GetName(), GetUID());
	EXC_DEBUG_END;

	return true;
}

