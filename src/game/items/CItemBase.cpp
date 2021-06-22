
// define the base types of an item (rather than the instance)

#include "../../sphere/ProfileTask.h"
#include "../components/CCPropsItem.h"
#include "../components/CCPropsItemChar.h"
#include "../components/CCPropsItemEquippable.h"
#include "../components/CCPropsItemWeapon.h"
#include "../components/CCPropsItemWeaponRanged.h"
#include "../resource/CResourceLock.h"
#include "../uo_files/CUOItemInfo.h"
#include "../CUOInstall.h"
#include "../CLog.h"
#include "../CException.h"
#include "CItemBase.h"


/////////////////////////////////////////////////////////////////
// -CItemBase

CItemBase::CItemBase( ITEMID_TYPE id ) :
	CBaseBaseDef( CResourceID( RES_ITEMDEF, id ))
{
	m_weight		= 0;
	m_speed			= 0;
	m_iSkill		= SKILL_NONE;
	m_layer			= LAYER_NONE;
	m_CanUse		= CAN_U_ALL;
	// Just applies to equippable weapons/armor.
	m_ttNormal.m_tData1 = 0;
	m_ttNormal.m_tData2 = 0;
	m_ttNormal.m_tData3 = 0;
	m_ttNormal.m_tData4 = 0;

	if ( ! IsValidDispID( id ) )
	{
		// There should be an ID= in the scripts later.
		m_dwDispIndex = ITEMID_GOLD_C1; // until i hear otherwise from the script file.

        m_type = IT_NORMAL;
        m_qwFlags = 0;
        m_Can = 0;
        m_layer = 0;
        m_weight = 0;

		return;
	}

	// Set the artwork/display id.
	m_dwDispIndex = id;

	// I have it indexed but it needs to be loaded.
	// read it in from the script and *.mul files.

    CUOItemTypeRec_HS tiledata = {};
	if ( id < ITEMID_MULTI )
	{
		if ( ! CItemBase::GetItemData( id, &tiledata ) )	// some valid items don't show up here !
		{
			// warn? ignore? // return nullptr;
		}
	}
	else
	{
		tiledata.m_weight = 0xFF;
	}

	m_qwFlags = tiledata.m_flags;

    TrySubscribeComponentProps<CCPropsItem>();
    TrySubscribeComponentProps<CCPropsItemChar>();

	SetType(GetTypeBase( id, tiledata ));

	// Stuff read from .mul file.
	// Some items (like hair) have no names !
	// Get rid of the strange leading spaces in some of the names.
	tchar szName[ sizeof(tiledata.m_name)+1 ];
	ushort j = 0;
	for ( ushort i = 0; i < sizeof(tiledata.m_name) && tiledata.m_name[i]; ++i )
	{
		if ( j == 0 && ISWHITESPACE(tiledata.m_name[i]))
			continue;
		szName[j++] = tiledata.m_name[i];
	}

	szName[j] = '\0';
	m_sName = szName;	// default type name.

	SetHeight( GetItemHeightFlags( tiledata, &m_Can ));

    // Do some special processing for certain items.
	GetItemSpecificFlags( tiledata, &m_Can, m_type, id );

	if ( (tiledata.m_weight == 0xFF) ||	// not movable.
		( m_qwFlags & UFLAG1_WATER ) )  // water can't be picked up.
	{
		m_weight = UINT16_MAX;
	}
	else
	{
		m_weight = tiledata.m_weight * WEIGHT_UNITS;
	}

	if ( m_qwFlags & ( UFLAG1_EQUIP | UFLAG3_EQUIP2 ))
	{
		m_layer = tiledata.m_layer;
		if ( m_layer && ! IsMovableType())
		{
			// How am i supposed to equip something i can't pick up ?
			m_weight = WEIGHT_UNITS;
		}
	}

	// ResDisp
	SetResDispDnId(ITEMID_GOLD_C1);
}

word CItemBase::GetMaxAmount()
{
	ADDTOCALLSTACK("CItemBase::GetMaxAmount");
	if (!IsStackableType())
		return 0;

	int64 iMax = GetDefNum("MaxAmount");
	if (iMax)
		return (word)minimum(iMax, UINT16_MAX);
	else
		return (word)minimum(g_Cfg.m_iItemsMaxAmount, UINT16_MAX);
}

bool CItemBase::SetMaxAmount(word amount)
{
	ADDTOCALLSTACK("CItemBase::SetMaxAmount");
	if (!IsStackableType())
		return false;

	SetDefNum("MaxAmount", amount, false);
	return true;
}

void CItemBase::SetTypeName( lpctstr pszName )
{
	ADDTOCALLSTACK("CItemBase::SetTypeName");
	ASSERT(pszName);
	if ( ! strcmp( pszName, GetTypeName()) )
		return;
	m_qwFlags |= UFLAG2_ZERO1;	// we override the name
	CBaseBaseDef::SetTypeName( pszName );
}

lpctstr CItemBase::GetArticleAndSpace() const
{
	ADDTOCALLSTACK("CItemBase::GetArticleAndSpace");
	if ( IsSetOF(OF_NoPrefix) )
		return "";
	if ( m_qwFlags & UFLAG2_ZERO1 )	// Name has been changed from TILEDATA.MUL
		return Str_GetArticleAndSpace( GetTypeName() );
	if ( m_qwFlags & UFLAG2_AN )
		return "an ";
	if ( m_qwFlags & UFLAG2_A )
		return "a ";
	return "";
}

void CItemBase::UnLink()
{
    m_flip_id.clear();
    m_SkillMake.clear();
    CBaseBaseDef::UnLink();
}

void CItemBase::CopyBasic( const CItemBase * pBase )
{
	ADDTOCALLSTACK("CItemBase::CopyBasic");
	m_speed		= pBase->m_speed;
	m_weight	= pBase->m_weight;
	m_flip_id	= pBase->m_flip_id;
	m_layer		= pBase->m_layer;
    SetType(pBase->m_type);
    m_CanUse    = pBase->m_CanUse;
	// Just applies to weapons/armor.
	m_ttNormal.m_tData1 = pBase->m_ttNormal.m_tData1;
	m_ttNormal.m_tData2 = pBase->m_ttNormal.m_tData2;
	m_ttNormal.m_tData3 = pBase->m_ttNormal.m_tData3;
	m_ttNormal.m_tData4 = pBase->m_ttNormal.m_tData4;

	CBaseBaseDef::CopyBasic( pBase );	// This will overwrite the CResourceLink!!
}

void CItemBase::CopyTransfer( CItemBase * pBase )
{
	ADDTOCALLSTACK("CItemBase::CopyTransfer");
	CopyBasic( pBase );

	m_values = pBase->m_values;
	m_SkillMake = pBase->m_SkillMake;

	CBaseBaseDef::CopyTransfer( pBase );	// This will overwrite the CResourceLink!!
}

lpctstr CItemBase::GetName() const
{
	ADDTOCALLSTACK("CItemBase::GetName");
	// Get rid of the strange %s type stuff for pluralize rules of names.
	return GetNamePluralize( GetTypeName(), false );
}

tchar * CItemBase::GetNamePluralize( lpctstr pszNameBase, bool fPluralize )	// static
{
	ADDTOCALLSTACK("CItemBase::GetNamePluralize");
	tchar * pszName = Str_GetTemp();
	ushort j = 0;
	bool fInside = false;
	bool fPlural = false;
	for ( ushort i = 0; pszNameBase[i]; ++i )
	{
		if ( pszNameBase[i] == '%' )
		{
			fInside = ! fInside;
			fPlural = true;
			continue;
		}
		if ( fInside )
		{
			if ( pszNameBase[i] == '/' )
			{
				fPlural = false;
				continue;
			}
			if ( fPluralize )
			{
				if ( ! fPlural )
					continue;
			}
			else
			{
				if ( fPlural )
					continue;
			}
		}
		pszName[j++] = pszNameBase[i];
	}
	pszName[j] = '\0';
	return pszName;
}

CREID_TYPE CItemBase::FindCharTrack( ITEMID_TYPE trackID )	// static
{
	ADDTOCALLSTACK("CItemBase::FindCharTrack");
	// For figurines. convert to a creature.
	// IT_EQ_HORSE
	// IT_FIGURINE

	CItemBase * pItemDef = CItemBase::FindItemBase( trackID );
	if ( pItemDef == nullptr )
		return CREID_INVALID;
	if ( ! pItemDef->IsType(IT_EQ_HORSE) && ! pItemDef->IsType(IT_FIGURINE) )
		return CREID_INVALID;

	return (CREID_TYPE)(pItemDef->m_ttFigurine.m_idChar.GetResIndex());
}

bool CItemBase::IsTypeArmor( IT_TYPE type ) noexcept // static
{
	switch( type )
	{
		case IT_CLOTHING:
		case IT_ARMOR:
		case IT_ARMOR_LEATHER:
		case IT_SHIELD:
			return true;
	}
	return false;
}

bool CItemBase::IsTypeWeapon( IT_TYPE type ) noexcept // static
{
	// NOTE: a wand can be a weapon.
	switch( type )
	{
		case IT_WEAPON_MACE_STAFF:
		case IT_WEAPON_MACE_CROOK:
		case IT_WEAPON_MACE_PICK:
		case IT_WEAPON_AXE:
		case IT_WEAPON_XBOW:
		case IT_WEAPON_THROWING:
		case IT_WEAPON_MACE_SMITH:
		case IT_WEAPON_MACE_SHARP:
		case IT_WEAPON_SWORD:
		case IT_WEAPON_FENCE:
		case IT_WEAPON_BOW:
        case IT_WEAPON_WHIP:
		case IT_WAND:
			return true;

		default:
			return false;
	}
}

GUMP_TYPE CItemBase::IsTypeContainer() const noexcept
{
	// IT_CONTAINER
	// return the container gump id.

	switch ( m_type )
	{
		case IT_CONTAINER:
		case IT_SIGN_GUMP:
		case IT_SHIP_HOLD:
		case IT_BBOARD:
		case IT_CORPSE:
		case IT_TRASH_CAN:
		case IT_GAME_BOARD:
		case IT_EQ_BANK_BOX:
		case IT_EQ_VENDOR_BOX:
		case IT_KEYRING:
			return m_ttContainer.m_idGump;
		default:
			return GUMP_NONE;
	}
}

bool CItemBase::IsTypeSpellbook( IT_TYPE type ) noexcept // static
{
	switch( type )
	{
		case IT_SPELLBOOK:
		case IT_SPELLBOOK_NECRO:
		case IT_SPELLBOOK_PALA:
		case IT_SPELLBOOK_EXTRA:
		case IT_SPELLBOOK_BUSHIDO:
		case IT_SPELLBOOK_NINJITSU:
		case IT_SPELLBOOK_ARCANIST:
		case IT_SPELLBOOK_MYSTIC:
		case IT_SPELLBOOK_MASTERY:
			return true;
		default:
			return false;
	}
}

bool CItemBase::IsTypeMulti( IT_TYPE type ) noexcept	// static
{
	switch( type )
	{
		case IT_MULTI:
		case IT_MULTI_CUSTOM:
		case IT_SHIP:
        case IT_MULTI_ADDON:
			return true;

		default:
			return false;
	}
}

bool CItemBase::IsTypeEquippable(IT_TYPE type, LAYER_TYPE layer) noexcept // static
{
    // Equippable on (possibly) visible layers.

    switch ( type )
    {
        case IT_LIGHT_LIT:
        case IT_LIGHT_OUT:	// Torches and lanterns.
        case IT_FISH_POLE:
        case IT_HAIR:
        case IT_BEARD:
        case IT_JEWELRY:
        case IT_EQ_HORSE:
            return true;
        default:
            break;
    }
    if ( IsTypeSpellbook( type ) )
        return true;
    if ( IsTypeArmor( type ) )
        return true;
    if ( IsTypeWeapon( type ) )
        return true;

    // Even not visible things.
    switch ( type )
    {
        case IT_EQ_BANK_BOX:
        case IT_EQ_VENDOR_BOX:
        case IT_EQ_CLIENT_LINGER:
        case IT_EQ_MURDER_COUNT:
        case IT_EQ_STUCK:
        case IT_EQ_TRADE_WINDOW:
        case IT_EQ_MEMORY_OBJ:
        case IT_EQ_SCRIPT:
            if (IsVisibleLayer( layer ))
                return false;
            return true;
        default:
            break;
    }

    return false;
}

bool CItemBase::IsTypeEquippable() const noexcept
{
	return IsTypeEquippable(m_type, (LAYER_TYPE)m_layer);
}

bool CItemBase::IsID_Multi( ITEMID_TYPE id ) noexcept // static
{
	// NOTE: Ships are also multi's
	return ( id >= ITEMID_MULTI && id < ITEMID_MULTI_MAX );
}

bool CItemBase::IsID_House(ITEMID_TYPE id) noexcept
{
    // IT_MULTI
    // IT_MULTI_CUSTOM
    return (((id >= ITEMID_HOUSE_SMALL_ST_PL) && (id <= ITEMID_HOUSE_SMALL_SHOP_MB)) || ((id >= ITEMID_HOUSEFOUNDATION_7x7) && (id <= ITEMID_HOUSEFOUNDATION_30x30)));
}

int CItemBase::IsID_Door( ITEMID_TYPE id ) noexcept // static
{
	// IT_DOOR
	static const ITEMID_TYPE sm_Item_DoorBase[] =
	{
		ITEMID_DOOR_SECRET_1,
		ITEMID_DOOR_SECRET_2,
		ITEMID_DOOR_SECRET_3,
		ITEMID_DOOR_SECRET_4,
		ITEMID_DOOR_SECRET_5,
		ITEMID_DOOR_SECRET_6,
		ITEMID_DOOR_METAL_S,
		ITEMID_DOOR_BARRED,
		ITEMID_DOOR_RATTAN,
		ITEMID_DOOR_WOODEN_1,
		ITEMID_DOOR_WOODEN_2,
		ITEMID_DOOR_METAL_L,
		ITEMID_DOOR_WOODEN_3,
		ITEMID_DOOR_WOODEN_4,
		ITEMID_DOOR_IRONGATE_1,
		ITEMID_DOOR_WOODENGATE_1,
		ITEMID_DOOR_IRONGATE_2,
		ITEMID_DOOR_WOODENGATE_2,
		ITEMID_DOOR_BAR_METAL,
		ITEMID_DOOR_WOOD_BLACK,
		ITEMID_DOOR_ELVEN_BARK,
		ITEMID_DOOR_ELVEN_SIMPLE,
		ITEMID_DOOR_ELVEN_ORNATE,
		ITEMID_DOOR_ELVEN_PLAIN,
		ITEMID_DOOR_MOON,
		ITEMID_DOOR_CRYSTAL,
		ITEMID_DOOR_SHADOW,
		ITEMID_DOOR_GARGISH_GREEN,
		ITEMID_DOOR_GARGISH_BROWN,
		ITEMID_DOOR_SUN,
		ITEMID_DOOR_GARGISH_GREY,
		ITEMID_DOOR_RUINED,
		ITEMID_DOOR_GARGISH_BLUE,
		ITEMID_DOOR_GARGISH_RED,
		ITEMID_DOOR_GARGISH_PRISON,
		ITEMID_DOOR_JUNGLE,
		ITEMID_DOOR_SHADOWGUARD
	};

	if ( id == 0x190e )
	{
		// anomoly door. bar door just has 2 pieces.
		return 1;
	}
	if ( id == 0x190f )
	{
		// anomoly door. bar door just has 2 pieces.
		return 2;
	}

	for ( uint i = 0; i < CountOf(sm_Item_DoorBase); ++i)
	{
		const int did = id - sm_Item_DoorBase[i];
		if ( did >= 0 && did <= 15 )
			return ( did+1 );
	}
	return 0;
}

bool CItemBase::IsID_DoorOpen( ITEMID_TYPE id ) noexcept // static
{
  	int doordir = IsID_Door(id)-1;
    if ( doordir < 0 )
		return false;
    if ( doordir & DOOR_OPENED )
		return true;
	return false;
}

bool CItemBase::IsID_Ship( ITEMID_TYPE id ) noexcept
{
	// IT_SHIP
	return ( id >= ITEMID_MULTI && id <= ITEMID_GALLEON_BRIT2_W );
}

bool CItemBase::IsID_GamePiece( ITEMID_TYPE id ) noexcept // static
{
	return ( id >= ITEMID_GAME1_CHECKER && id <= ITEMID_GAME_HI );
}

bool CItemBase::IsID_Track( ITEMID_TYPE id ) noexcept // static
{
	return ( id >= ITEMID_TRACK_BEGIN && id <= ITEMID_TRACK_END );
}

bool CItemBase::IsID_WaterFish( ITEMID_TYPE id ) noexcept // static
{
	// IT_WATER
	// Assume this means water we can fish in.
	// Not water we can wash in.
	if ( id >= 0x1796 && id <= 0x17b2 )
		return true;
	if ( id == 0x1559 )
		return true;
	return false;
}

bool CItemBase::IsID_WaterWash( ITEMID_TYPE id ) noexcept // static
{
	// IT_WATER_WASH
	if ( id >= ITEMID_WATER_TROUGH_1 && id <= ITEMID_WATER_TROUGH_2	)
		return true;
	return IsID_WaterFish( id );
}

bool CItemBase::IsID_Chair( ITEMID_TYPE id ) noexcept // static
{
	// Strangely there is not chair flag in the statics.mul file ??? !!!
	// IT_CHAIR

	// todo: consider enum values for these chairs
	switch ((word)id)
	{
		case 0x0459: // 'marble bench'
		case 0x045a: // 'marble bench'
		case 0x045b: // 'stone bench'
		case 0x045c: // 'stone bench'
		case 0x0b2c: // 'wooden bench'
		case 0x0b2d: // 'wooden bench'
		case 0x0b2e: // 'wooden chair'
		case 0x0b2f: // 'wooden chair'
		case 0x0b30: // 'wooden chair'
		case 0x0b31: // 'wooden chair'
		case 0x0b32: // 'throne'
		case 0x0b33: // 'throne'
		case 0x0b4e: // 'chair'
		case 0x0b4f: // 'chair'
		case 0x0b50: // 'chair'
		case 0x0b51: // 'chair'
		case 0x0b52: // 'chair'
		case 0x0b53: // 'chair'
		case 0x0b54: // 'chair'
		case 0x0b55: // 'chair'
		case 0x0b56: // 'chair'
		case 0x0b57: // 'chair'
		case 0x0b58: // 'chair'
		case 0x0b59: // 'chair'
		case 0x0b5a: // 'chair'
		case 0x0b5b: // 'chair'
		case 0x0b5c: // 'chair'
		case 0x0b5d: // 'chair'
		case 0x0b5e: // 'foot stool'
		case 0x0b5f: // 'bench'
		case 0x0b60: // 'bench'
		case 0x0b61: // 'bench'
		case 0x0b62: // 'bench'
		case 0x0b63: // 'bench'
		case 0x0b64: // 'bench'
		case 0x0b65: // 'bench'
		case 0x0b66: // 'bench'
		case 0x0b67: // 'bench'
		case 0x0b68: // 'bench'
		case 0x0b69: // 'bench'
		case 0x0b6a: // 'bench'
		case 0x0b91: // 'bench'
		case 0x0b92: // 'bench'
		case 0x0b93: // 'bench'
		case 0x0b94: // 'bench'
		case 0x0c17: // 'covered chair'
		case 0x0c18: // 'covered chair'
		case 0x1049: // 'loom bench'
		case 0x104a: // 'loom bench'
		case 0x1207: // 'stone bench'
		case 0x1208: // 'stone bench'
		case 0x1209: // 'stone bench'
		case 0x120a: // 'stone bench'
		case 0x120b: // 'stone bench'
		case 0x120c: // 'stone bench'
		case 0x1218: // 'stone chair'
		case 0x1219: // 'stone chair'
		case 0x121a: // 'stone chair'
		case 0x121b: // 'stone chair'
		case 0x1526: // 'throne'
		case 0x1527: // 'throne'
		case 0x19f1: // 'woodworker's bench'
		case 0x19f2: // 'woodworker's bench'
		case 0x19f3: // 'woodworker's bench'
		case 0x19f5: // 'woodworker's bench'
		case 0x19f6: // 'woodworker's bench'
		case 0x19f7: // 'woodworker's bench'
		case 0x19f9: // 'cooper's bench'
		case 0x19fa: // 'cooper's bench'
		case 0x19fb: // 'cooper's bench'
		case 0x19fc: // 'cooper's bench'
		case 0x1dc7: // 'sandstone bench'
		case 0x1dc8: // 'sandstone bench'
		case 0x1dc9: // 'sandstone bench'
		case 0x1dca: // 'sandstone bench'
		case 0x1dcb: // 'sandstone bench'
		case 0x1dcc: // 'sandstone bench'
		case 0x1dcd: // 'marble bench'
		case 0x1dce: // 'marble bench'
		case 0x1dcf: // 'marble bench'
		case 0x1dd0: // 'marble bench'
		case 0x1dd1: // 'marble bench'
		case 0x1dd2: // 'marble bench'
		case 0x1e6f: // 'chair'
		case 0x1e78: // 'chair'
		case 0x3dff: // 'bench'
		case 0x3e00: // 'bench'
			return true;

		default:
			return false;
	}
}

bool CItemBase::GetItemData( ITEMID_TYPE id, CUOItemTypeRec_HS * pData ) // static
{
	ADDTOCALLSTACK("CItemBase::GetItemData");
	// Read from g_Install.m_fTileData
	// Get an Item tiledata def data.
	// Invalid object id ?
	// NOTE: This data should already be read into the m_ItemBase table ???

    if (id >= g_Install.m_tiledata.GetItemMaxIndex())
    {
        g_Log.EventError("ITEMDEF has invalid ID=%d (0%x) (value is greater than the tiledata maximum index).\n", id, id);
        return false;
    }
    if (!IsValidDispID(id))
		return false;

	try
	{
		*pData = CUOItemInfo(id);
	}
    catch (const std::exception& e)
    {
        g_Log.CatchStdException(&e, "GetItemData");
        CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
        return false;
    }
	catch ( const CSError& e )
	{
		g_Log.CatchEvent( &e, "GetItemData" );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return false;
	}
	catch (...)
	{
		g_Log.CatchEvent(nullptr, "GetItemData" );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		return false;
	}

	// Unused tiledata I guess. Don't create it.
	if ( ! pData->m_flags &&
		! pData->m_weight &&
		! pData->m_layer &&
		! pData->m_dwUnk11 &&
		! pData->m_wAnim &&
		! pData->m_wHue &&
        ! pData->m_wLightIndex &&
		! pData->m_height &&
		! pData->m_name[0]
		)
	{
		// What are the exceptions to the rule ?
		if ( id == ITEMID_BBOARD_MSG ) // special
			return true;
		if ( IsID_GamePiece( id ))
			return true;
		if ( IsID_Track(id))	// preserve these
			return true;
		return false;
	}
	return true;
}

void CItemBase::GetItemSpecificFlags( const CUOItemTypeRec_HS & tiledata, dword *pdwBlockFlags, IT_TYPE type, ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemBase::GetItemSpecificFlags");
	if ( type == IT_DOOR )
	{
        *pdwBlockFlags &= ~CAN_I_BLOCK;
		if ( IsID_DoorOpen(id))
            *pdwBlockFlags &= ~CAN_I_DOOR;
		else
            *pdwBlockFlags |= CAN_I_DOOR;
	}

	if ( tiledata.m_flags & UFLAG3_LIGHT )	// this may actually be a moon gate or fire ?
        *pdwBlockFlags |= CAN_I_LIGHT;	// normally of type IT_LIGHT_LIT;

	if ( (tiledata.m_flags & UFLAG2_STACKABLE) || type == IT_REAGENT || id == ITEMID_EMPTY_BOTTLE )
        *pdwBlockFlags |= CAN_I_PILE;
}

void CItemBase::GetItemTiledataFlags( dword *pdwCanFlags, ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemBase::GetItemTiledataFlags");

    CUOItemTypeRec_HS tiledata{};
	if ( ! CItemBase::GetItemData( id, &tiledata ))
	{
        *pdwCanFlags = 0;
		return;
	}

	if ( tiledata.m_flags & UFLAG4_DOOR )
        *pdwCanFlags |= CAN_I_DOOR;
	if ( tiledata.m_flags & UFLAG1_WATER )
        *pdwCanFlags |= CAN_I_WATER;
	if ( tiledata.m_flags & UFLAG2_PLATFORM )
        *pdwCanFlags |= CAN_I_PLATFORM;
	if ( tiledata.m_flags & UFLAG1_BLOCK )
        *pdwCanFlags |= CAN_I_BLOCK;
	if ( tiledata.m_flags & UFLAG2_CLIMBABLE )
        *pdwCanFlags |= CAN_I_CLIMB;
	if ( tiledata.m_flags & UFLAG1_DAMAGE )
        *pdwCanFlags |= CAN_I_FIRE;
	if ( tiledata.m_flags & UFLAG4_ROOF )
        *pdwCanFlags |= CAN_I_ROOF;
	if ( tiledata.m_flags & UFLAG4_HOVEROVER )
        *pdwCanFlags |= CAN_I_HOVER;
}

height_t CItemBase::GetItemHeightFlags( const CUOItemTypeRec_HS & tiledata, dword *pdwCanFlags ) // static
{
	ADDTOCALLSTACK("CItemBase::GetItemHeightFlags");
	// Chairs are marked as blocking for some reason ?
	if ( tiledata.m_flags & UFLAG4_DOOR ) // door
	{
		*pdwCanFlags = CAN_I_DOOR;
		return tiledata.m_height;
	}

	if ( tiledata.m_flags & UFLAG1_BLOCK )
	{
		if ( tiledata.m_flags & UFLAG1_WATER )	// water
		{
            *pdwCanFlags = CAN_I_WATER;
			return tiledata.m_height;
		}
        *pdwCanFlags = CAN_I_BLOCK;
	}
	else
	{
		//DEBUG_ERR(("tiledata.m_flags 0x%x\n",tiledata.m_flags));
        *pdwCanFlags = 0;
		if ( ! ( tiledata.m_flags & (UFLAG2_PLATFORM|UFLAG4_ROOF|UFLAG4_HOVEROVER) ))
			return 0;	// have no effective height if it doesn't block.
	}

	if ( tiledata.m_flags & UFLAG4_ROOF)
        *pdwCanFlags |= CAN_I_ROOF;
	else if ( tiledata.m_flags & UFLAG2_PLATFORM )
        *pdwCanFlags |= CAN_I_PLATFORM;

	if ( tiledata.m_flags & UFLAG2_CLIMBABLE )
	{
		// actual standing height is height/2
        *pdwCanFlags |= CAN_I_CLIMB;
	}
	if ( tiledata.m_flags & UFLAG4_HOVEROVER )
        *pdwCanFlags |= CAN_I_HOVER;
	//DEBUG_WARN(("tiledata.m_height(%d)\n",tiledata.m_height));
	return tiledata.m_height;
}

height_t CItemBase::GetItemHeight( ITEMID_TYPE id, dword *pdwBlockFlags ) // static
{
	ADDTOCALLSTACK_INTENSIVE("CItemBase::GetItemHeight");
	// Get just the height and the blocking flags for the item by id.
	// used for walk block checking.

	const CResourceID rid( RES_ITEMDEF, id );
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index != SCONT_BADINDEX ) // already loaded ?
	{
		CResourceDef * pBaseStub = g_Cfg.m_ResHash.GetAt( rid, index );
		ASSERT(pBaseStub);
		CItemBase * pBase = dynamic_cast <CItemBase *>(pBaseStub);
		if ( pBase )
		{
			*pdwBlockFlags = pBase->m_Can & CAN_I_MOVEMASK;
			return pBase->GetHeight();
		}
	}

	// Not already loaded.
    CUOItemTypeRec_HS tiledata = {};
	if ( ! GetItemData( id, &tiledata ))
	{
		*pdwBlockFlags = CAN_I_MOVEMASK;
		return UO_SIZE_Z;
	}
	if ( IsID_Chair( id ) )
	{
		*pdwBlockFlags = 0;
		return 0;	// have no effective height if they don't block.
	}
	return GetItemHeightFlags( tiledata, pdwBlockFlags );
}

IT_TYPE CItemBase::GetTypeBase( ITEMID_TYPE id, const CUOItemTypeRec_HS &tiledata ) // static
{
	ADDTOCALLSTACK("CItemBase::GetTypeBase");
	if ( IsID_Ship( id ) )
		return IT_SHIP;
	if ( IsID_Multi( id ) )
		return IT_MULTI;

	if (( tiledata.m_flags & UFLAG1_BLOCK ) && (tiledata.m_flags & UFLAG1_WATER))
		return IT_WATER;
	if ( IsID_WaterFish( id ) ) // UFLAG1_WATER
		return IT_WATER;

	if (( tiledata.m_flags & UFLAG4_DOOR ) || IsID_Door( id ))
	{
		if ( IsID_DoorOpen( id ) )	// currently open
			return IT_DOOR_OPEN;
		return IT_DOOR;
	}

	if ( tiledata.m_flags & UFLAG3_CONTAINER )
		return IT_CONTAINER;

	if ( IsID_WaterWash( id ) )
		return IT_WATER_WASH;
	else if ( IsID_Track( id ) )
		return IT_FIGURINE;
	else if ( IsID_GamePiece( id ) )
		return IT_GAME_PIECE;

	// Get rid of the stuff below here !

	if (( tiledata.m_flags & UFLAG1_DAMAGE ) && ! ( tiledata.m_flags & UFLAG1_BLOCK ))
		return IT_TRAP_ACTIVE;

	if ( tiledata.m_flags & UFLAG3_LIGHT )	// this may actually be a moon gate or fire ?
		return IT_LIGHT_LIT;

	return IT_NORMAL;	// Get from script i guess.
}

ITEMID_TYPE CItemBase::GetNextFlipID( ITEMID_TYPE id ) const
{
	ADDTOCALLSTACK("CItemBase::GetNextFlipID");
	if (!m_flip_id.empty())
	{
		ITEMID_TYPE idprev = GetDispID();
		for ( size_t i = 0; i < m_flip_id.size(); ++i )
		{
			ITEMID_TYPE idnext = m_flip_id[i];
			if ( idprev == id )
				return idnext;
			idprev = idnext;
		}
	}
	return GetDispID();
}

bool CItemBase::IsSameDispID( ITEMID_TYPE id ) const
{
	ADDTOCALLSTACK("CItemBase::IsSameDispID");
	// Does this item look like the item we want ?
	// Take into account flipped items.

	if ( ! IsValidDispID( id ))	// this should really not be here but handle it anyhow.
		return ( GetID() == id );

	if ( id == GetDispID())
		return true;

	for ( size_t i = 0; i < m_flip_id.size(); ++i )
	{
		if ( m_flip_id[i] == id )
			return true;
	}
	return false;
}

bool CItemBase::IsDupedItem( ITEMID_TYPE id ) const
{
    ADDTOCALLSTACK("CItemBase::IsDupedItem");
    if (m_flip_id.empty())
        return false;
    for ( size_t i = 0; i < m_flip_id.size(); ++i )
    {
        if ( m_flip_id[i] == id)
            return true;
    }
    return false;
}

void CItemBase::Restock()
{
	ADDTOCALLSTACK("CItemBase::Restock");
	// Re-evaluate the base random value rate some time in the future.
	if ( m_values.m_iLo < 0 || m_values.m_iHi < 0 )
		m_values.Init();
}

int CItemBase::CalculateMakeValue( int iQualityLevel ) const
{
	ADDTOCALLSTACK("CItemBase::CalculateMakeValue");
	// Calculate the value in gold for this item based on its components.
	// NOTE: Watch out for circular RESOURCES= list in the scripts.
	// ARGS:
	//   iQualityLevel = 0-100

	static int sm_iReentrantCount = 0;
	if ( sm_iReentrantCount > 32 )
	{
		DEBUG_ERR(( "Too many RESOURCES at item '%s' to calculate a value with (circular resource list?).\n", GetResourceName() ));
		return 0;
	}

	++sm_iReentrantCount;
	int lValue = 0;

	// add value based on the base resources making this up.
	for ( size_t i = 0; i < m_BaseResources.size(); ++i )
	{
		const CResourceID rid = m_BaseResources[i].GetResourceID();
		if ( rid.GetResType() != RES_ITEMDEF )
			continue;

		CItemBase * pItemDef = CItemBase::FindItemBase( (ITEMID_TYPE)rid.GetResIndex() );
		if ( pItemDef == nullptr )
			continue;

		lValue += pItemDef->GetMakeValue( iQualityLevel ) * (int)(m_BaseResources[i].GetResQty());
	}

	// add some value based on the skill required to create it.
	for ( size_t i = 0; i < m_SkillMake.size(); ++i )
	{
		const CResourceID rid = m_SkillMake[i].GetResourceID();
		if ( rid.GetResType() != RES_SKILL )
			continue;
		const CSkillDef* pSkillDef = g_Cfg.GetSkillDef((SKILL_TYPE)(rid.GetResIndex()));
		if ( pSkillDef == nullptr )
			continue;

		// this is the normal skill required.
		// if iQuality is much less than iSkillNeed then something is wrong.
		int iSkillNeed = (int)(m_SkillMake[i].GetResQty());
		if ( iQualityLevel < iSkillNeed )
			iQualityLevel = iSkillNeed;

		lValue += pSkillDef->m_Values.GetLinear( iQualityLevel );
	}

	--sm_iReentrantCount;
	return lValue;
}

word CItemBase::GetWeight() const noexcept
{
    // Get weight in tenths of a stone.
    if ( ! IsMovableType())
        return WEIGHT_UNITS;	// If we can pick them up then we should be able to move them
    return m_weight;
}

byte CItemBase::GetSpeed() const
{
    const CVarDefCont *pVarDef = m_TagDefs.GetKey("OVERRIDE.SPEED");
	if (pVarDef)
		return (byte)pVarDef->GetValNum();
	return m_speed;
}

byte CItemBase::GetRangeL() const noexcept
{
	const auto pCCPItemWeapon = GetComponentProps<CCPropsItemWeapon>();
    return (byte)pCCPItemWeapon->GetPropertyNum(PROPIWEAP_RANGEL);
}

byte CItemBase::GetRangeH() const noexcept
{
    const auto pCCPItemWeapon = GetComponentProps<CCPropsItemWeapon>();
    return (byte)pCCPItemWeapon->GetPropertyNum(PROPIWEAP_RANGEH);
}

int CItemBase::GetMakeValue( int iQualityLevel )
{
	ADDTOCALLSTACK("CItemBase::GetMakeValue");
	// Set the items value based on the resources and skill used to make it.
	// ARGS:
	// iQualityLevel = 0-100

	CValueRangeDef values = m_values;

	if ( m_values.m_iLo == INT64_MIN || m_values.m_iHi == INT64_MIN )
	{
		values.m_iLo = CalculateMakeValue(0);		// low quality specimen
		m_values.m_iLo = -values.m_iLo;				// negative means they will float.
		values.m_iHi = CalculateMakeValue(100); 	// top quality specimen
		m_values.m_iHi = -values.m_iHi;
	}
	else
	{
		values.m_iLo = llabs(values.m_iLo);
		values.m_iHi = llabs(values.m_iHi);
	}

	return values.GetLinear(iQualityLevel*10);
}

void CItemBase::ResetMakeValue()
{
	ADDTOCALLSTACK("CItemBase::ResetMakeValue");
	m_values.m_iLo = INT32_MIN;
	m_values.m_iHi = INT32_MIN;
	GetMakeValue(0);
}

enum IBC_TYPE
{
	#define ADD(a,b) IBC_##a,
	#include "../../tables/CItemBase_props.tbl"
	#undef ADD
	IBC_QTY
};

lpctstr const CItemBase::sm_szLoadKeys[IBC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../../tables/CItemBase_props.tbl"
	#undef ADD
	nullptr
};

bool CItemBase::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
	ADDTOCALLSTACK("CItemBase::r_WriteVal");
    EXC_TRY("WriteVal");

    // Checking Props CComponents first
    EXC_SET_BLOCK("EntityProps");
    if (!fNoCallChildren && CEntityProps::r_WritePropVal(ptcKey, sVal, nullptr, this))
    {
        return true;
    }

    EXC_SET_BLOCK("Keyword");
	switch ( FindTableHeadSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		//return as string or hex number or nullptr if not set
		case IBC_ALTERITEM:
		case IBC_DROPSOUND:
		case IBC_PICKUPSOUND:
		case IBC_EQUIPSOUND:
		case IBC_BONUSSKILL1:
		case IBC_BONUSSKILL2:
		case IBC_BONUSSKILL3:
		case IBC_BONUSSKILL4:
		case IBC_BONUSSKILL5:
		case IBC_OCOLOR:
		case IBC_OWNEDBY:
		case IBC_ITEMSETNAME:
		case IBC_MATERIAL:
		case IBC_BONUSCRAFTING:
		case IBC_BONUSCRAFTINGEXCEP:
		case IBC_REMOVALTYPE:
		case IBC_NPCKILLER:
		case IBC_NPCPROTECTION:
		case IBC_SUMMONING:
			sVal = GetDefStr(ptcKey);
			break;
		//return as decimal number or 0 if not set
		case IBC_BONUSSKILL1AMT:
		case IBC_BONUSSKILL2AMT:
		case IBC_BONUSSKILL3AMT:
		case IBC_BONUSSKILL4AMT:
		case IBC_BONUSSKILL5AMT:
		case IBC_ITEMSETAMTCUR:
		case IBC_ITEMSETAMTMAX:
		case IBC_ITEMSETCOLOR:
		case IBC_RARITY:
		case IBC_LIFESPAN:
		case IBC_SELFREPAIR:
		case IBC_DURABILITY:
		case IBC_USESCUR:
		case IBC_USESMAX:
		case IBC_RECHARGE:
		case IBC_RECHARGEAMT:
		case IBC_RECHARGERATE:
		case IBC_BONUSCRAFTINGAMT:
		case IBC_BONUSCRAFTINGEXCEPAMT:
		case IBC_NPCKILLERAMT:
		case IBC_NPCPROTECTIONAMT:
			sVal.FormatLLVal(GetDefNum(ptcKey));
			break;


		case IBC_MAXAMOUNT:
			sVal.FormatVal(GetMaxAmount());
			break;

		case IBC_SPEEDMODE:
		{
			if (!IsType(IT_SHIP))
				return false;
			CItemBaseMulti * pItemMulti = dynamic_cast<CItemBaseMulti*>(this);
			ASSERT(pItemMulti);
			sVal.FormatVal(pItemMulti->m_SpeedMode);
		}
		break;
		case IBC_SHIPSPEED:
		{
			if (!IsType(IT_SHIP))
				return false;
			ptcKey += 9;
			CItemBaseMulti * pItemMulti = dynamic_cast<CItemBaseMulti*>(this);
			ASSERT(pItemMulti);

			if (*ptcKey == '.')
			{
				++ptcKey;
				if (!strnicmp(ptcKey, "TILES", 5))
				{
					sVal.FormatVal(pItemMulti->_shipSpeed.tiles);
					break;
				}
				else if (!strnicmp(ptcKey, "PERIOD", 6))
				{
					sVal.FormatVal(pItemMulti->_shipSpeed.period);
					break;
				}
				return false;
			}

			sVal.Format("%d,%d", pItemMulti->_shipSpeed.period, pItemMulti->_shipSpeed.tiles);
		} break;
        case IBC_MULTICOUNT:
        {
            if (!IsTypeMulti(GetType()))
            {
                return false;
            }
            const CItemBaseMulti * pItemMulti = dynamic_cast<const CItemBaseMulti*>(this);
            ASSERT(pItemMulti);
            sVal.FormatU8Val(pItemMulti->_iMultiCount);
        }break;
		case IBC_DISPID:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_ITEMDEF, GetDispID()));
			break;
		case IBC_DUPELIST:
			{
				tchar *pszTemp = Str_GetTemp();
				size_t iLen = 0;
				*pszTemp = '\0';
				for ( size_t i = 0; i < m_flip_id.size(); ++i )
				{
					if ( i > 0 )
						iLen += Str_CopyLimitNull( pszTemp + iLen, ",", SCRIPT_MAX_LINE_LEN - iLen);

					iLen += snprintf(pszTemp + iLen, SCRIPT_MAX_LINE_LEN - iLen, "0%x", (uint)m_flip_id[i]);
				}
				sVal = pszTemp;
			}
			break;
		case IBC_CANUSE:
			sVal.FormatHex( m_CanUse );
			break;
		case IBC_DYE:
			sVal.FormatVal( m_Can & CAN_I_DYE );
			break;
		case IBC_ENCHANT:
			sVal.FormatVal( m_Can & CAN_I_ENCHANT );
			break;
		case IBC_EXCEPTIONAL:
			sVal.FormatVal( m_Can & CAN_I_EXCEPTIONAL );
			break;
		case IBC_FLIP:
			sVal.FormatHex( m_Can & CAN_I_FLIP );
			break;
		case IBC_ID:
			sVal.FormatHex( GetDispID() );
			break;
		case IBC_IMBUE:
			sVal.FormatVal( m_Can & CAN_I_IMBUE );
			break;
		case IBC_ISARMOR:
			sVal.FormatVal( IsTypeArmor( m_type ) );
			break;
		case IBC_ISWEAPON:
			sVal.FormatVal( IsTypeWeapon( m_type ) );
			break;
		case IBC_MAKERSMARK:
			sVal.FormatVal( m_Can & CAN_I_MAKERSMARK );
			break;
		case IBC_RECYCLE:
			sVal.FormatVal( m_Can & CAN_I_RECYCLE );
			break;
		case IBC_REFORGE:
			sVal.FormatVal( m_Can & CAN_I_REFORGE );
			break;
		case IBC_RETAINCOLOR:
			sVal.FormatVal( m_Can & CAN_I_RETAINCOLOR );
			break;
		case IBC_SKILL:		// Skill to use.
			{
				if ( m_iSkill > SKILL_NONE && m_iSkill < (SKILL_TYPE)(g_Cfg.m_iMaxSkill) )
				{
					sVal.FormatVal(m_iSkill);
					break;
				}

				SKILL_TYPE skill;
				switch ( GetType() )
				{
					case IT_WEAPON_MACE_CROOK:
					case IT_WEAPON_MACE_PICK:
					case IT_WEAPON_MACE_SMITH:	// Can be used for smithing ?
					case IT_WEAPON_MACE_STAFF:
					case IT_WEAPON_MACE_SHARP:	// war axe can be used to cut/chop trees.
                    case IT_WEAPON_WHIP:
						skill = SKILL_MACEFIGHTING;
						break;
					case IT_WEAPON_SWORD:
					case IT_WEAPON_AXE:
						skill = SKILL_SWORDSMANSHIP;
						break;
					case IT_WEAPON_FENCE:
						skill = SKILL_FENCING;
						break;
					case IT_WEAPON_BOW:
					case IT_WEAPON_XBOW:
						skill = SKILL_ARCHERY;
						break;
					case IT_WEAPON_THROWING:
						skill = SKILL_THROWING;
						break;
					default:
						skill = SKILL_NONE;
						break;
				}
				sVal.FormatVal(skill);
				break;
			}
		case IBC_LAYER:
			sVal.FormatVal( m_layer );
			break;
		case IBC_REPAIR:
			sVal.FormatHex( m_Can & CAN_I_REPAIR );
			break;
		case IBC_REPLICATE:
			sVal.FormatHex( m_Can & CAN_I_REPLICATE );
			break;
		case IBC_REQSTR:
			// IsTypeEquippable()
			sVal.FormatVal( m_ttEquippable.m_iStrReq );
			break;
		case IBC_SKILLMAKE:		// Print the resources need to make in nice format.
			{
				ptcKey	+= 9;
				if ( *ptcKey == '.' )
				{
					bool	fQtyOnly	= false;
					bool	fKeyOnly	= false;
					SKIP_SEPARATORS( ptcKey );
					int		index	= Exp_GetVal( ptcKey );
					SKIP_SEPARATORS( ptcKey );

					if ( !strnicmp( ptcKey, "KEY", 3 ))
						fKeyOnly	= true;
					else if ( !strnicmp( ptcKey, "VAL", 3 ))
						fQtyOnly	= true;

					tchar *pszTmp = Str_GetTemp();
					if ( fKeyOnly || fQtyOnly )
						m_SkillMake.WriteKeys( pszTmp, index, fQtyOnly, fKeyOnly );
					else
						m_SkillMake.WriteNames( pszTmp, index );
					if ( fQtyOnly && pszTmp[0] == '\0' )
						strcpy( pszTmp, "0" );
					sVal = pszTmp;
				}
				else
				{
					tchar *pszTmp = Str_GetTemp();
					m_SkillMake.WriteNames( pszTmp );
					sVal = pszTmp;
				}
			}
			break;
		case IBC_RESDISPDNID:
			sVal = g_Cfg.ResourceGetName( CResourceID(RES_TYPEDEF, (int)GetResDispDnId()) );
			break;
		case IBC_RESMAKE:
			// Print the resources need to make in nice format.
			{
				tchar *pszTmp = Str_GetTemp();
				m_BaseResources.WriteNames( pszTmp );
				sVal = pszTmp;
			}
			break;
		case IBC_SPEED:
			sVal.FormatVal( m_speed );
			break;
		case IBC_TDATA1:
			sVal.FormatHex( m_ttNormal.m_tData1 );
			break;
		case IBC_TDATA2:
			sVal.FormatHex( m_ttNormal.m_tData2 );
			break;
		case IBC_TDATA3:
			sVal.FormatHex( m_ttNormal.m_tData3 );
			break;
		case IBC_TDATA4:
			sVal.FormatHex( m_ttNormal.m_tData4 );
			break;
		case IBC_TFLAGS:
			sVal.FormatULLHex( GetTFlags() );
			break;
		case IBC_TWOHANDS:
			if ( ! IsTypeEquippable() )
				return false;

			if ( !CCPropsItemWeapon::CanSubscribe(this))
				sVal.FormatVal(0);
			else
				sVal.FormatVal( m_layer == LAYER_HAND2 );
			break;
		case IBC_TYPE:
			// sVal.FormatVal( m_type );
			{
				CResourceID	rid( RES_TYPEDEF, m_type );
				CResourceDef *pRes = g_Cfg.ResourceGetDef( rid );
				if ( !pRes )
					sVal.FormatVal( m_type );
				else
					sVal = pRes->GetResourceName();
			}
			break;
		case IBC_VALUE:
			if ( m_values.GetRange() )
				sVal.Format( "%d,%d", GetMakeValue(0), GetMakeValue(100) );
			else
				sVal.Format( "%d", GetMakeValue(0));
			break;
		case IBC_WEIGHT:
			sVal.FormatVal( m_weight / WEIGHT_UNITS );
			break;
		default:
        {
            return ( fNoCallParent ? false : CBaseBaseDef::r_WriteVal(ptcKey, sVal) );
        }
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CItemBase::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CItemBase::r_LoadVal");
    EXC_TRY("LoadVal");

    // Checking Props CComponents first
    EXC_SET_BLOCK("EntityProps");
    if (CEntityProps::r_LoadPropVal(s, nullptr, this))
    {
        return true;
    }

    EXC_SET_BLOCK("Keyword");
	lpctstr	ptcKey = s.GetKey();
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ) )
	{
		//Set as Strings
		case IBC_ALTERITEM:
		case IBC_DROPSOUND:
		case IBC_PICKUPSOUND:
		case IBC_EQUIPSOUND:
		case IBC_BONUSSKILL1:
		case IBC_BONUSSKILL2:
		case IBC_BONUSSKILL3:
		case IBC_BONUSSKILL4:
		case IBC_BONUSSKILL5:
		case IBC_OCOLOR:
		case IBC_OWNEDBY:
		case IBC_ITEMSETNAME:
		case IBC_MATERIAL:
		case IBC_BONUSCRAFTING:
		case IBC_BONUSCRAFTINGEXCEP:
		case IBC_REMOVALTYPE:
		case IBC_NPCKILLER:
		case IBC_NPCPROTECTION:
		case IBC_SUMMONING:
			{
				bool fQuoted = false;
				SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
			}
			break;
		//Set as number only
		case IBC_BONUSSKILL1AMT:
		case IBC_BONUSSKILL2AMT:
		case IBC_BONUSSKILL3AMT:
		case IBC_BONUSSKILL4AMT:
		case IBC_BONUSSKILL5AMT:
		case IBC_ITEMSETAMTCUR:
		case IBC_ITEMSETAMTMAX:
		case IBC_ITEMSETCOLOR:
		case IBC_RARITY:
		case IBC_LIFESPAN:
		case IBC_SELFREPAIR:
		case IBC_DURABILITY:
		case IBC_USESCUR:
		case IBC_USESMAX:
		case IBC_RECHARGE:
		case IBC_RECHARGEAMT:
		case IBC_RECHARGERATE:
		case IBC_BONUSCRAFTINGAMT:
		case IBC_BONUSCRAFTINGEXCEPAMT:
		case IBC_NPCKILLERAMT:
		case IBC_NPCPROTECTIONAMT:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			break;
		case IBC_MAXAMOUNT:
			if (!SetMaxAmount(s.GetArgUSVal()))
				return false;
			break;
		case IBC_SPEEDMODE:
		{
			if (!IsType(IT_SHIP))
				return false;
			CItemBaseMulti *pItemMulti = dynamic_cast<CItemBaseMulti*>(this);
			ASSERT(pItemMulti);
            ShipMovementSpeed speed = (ShipMovementSpeed)s.GetArgBVal();
			if (speed > SMS_FAST)
				speed = SMS_FAST;
			else if (speed < SMS_NORMAL)
				speed = SMS_NORMAL;	//Max = 4, Min = 1.
			pItemMulti->m_SpeedMode = speed;
		}break;
		case IBC_SHIPSPEED:
		{
			ptcKey += 9;
			if (*ptcKey == '.')
			{
				++ptcKey;
				CItemBaseMulti *pItemMulti = dynamic_cast<CItemBaseMulti*>(dynamic_cast<CItemBase*>(this));
				ASSERT(pItemMulti);

				if (!strnicmp(ptcKey, "TILES", 5))
				{
					pItemMulti->_shipSpeed.tiles = (uchar)(s.GetArgVal());
					return true;
				}
				else if (!strnicmp(ptcKey, "PERIOD", 6))
				{
					pItemMulti->_shipSpeed.tiles = (uchar)(s.GetArgVal());
					return true;
				}

				int64 piVal[2];
				size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, CountOf(piVal));
				if (iQty == 2)
				{
					pItemMulti->_shipSpeed.period = (uchar)(piVal[0]);
					pItemMulti->_shipSpeed.tiles = (uchar)(piVal[1]);
					return true;
				}
				else
					return false;
			}
		} break;
        case IBC_MULTICOUNT:
        {
            if (!IsTypeMulti(GetType()))
            {
                return false;
            }
            CItemBaseMulti * pItemMulti = dynamic_cast<CItemBaseMulti*>(this);
			ASSERT(pItemMulti);
            pItemMulti->_iMultiCount = s.GetArgU8Val();
        }
		break;
		case IBC_CANUSE:
			m_CanUse = s.GetArgVal();
			break;
		case IBC_DISPID:
			// Can't set this.
			return false;
		case IBC_DUPEITEM:
			// Just ignore these.
			return true;

		case IBC_DUPELIST:
			{
				tchar * ppArgs[512];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), ppArgs, CountOf(ppArgs));
				if ( iArgQty <= 0 )
					return false;
				m_flip_id.clear();
				for ( int i = 0; i < iArgQty; ++i )
				{
					ITEMID_TYPE id = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, ppArgs[i] ));
					if ( ! IsValidDispID( id ))
						continue;
					if ( IsSameDispID(id))
						continue;
					m_flip_id.emplace_back(id);
				}
			}
			break;
		case IBC_DYE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_DYE;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_DYE;
				else
					m_Can &= ~CAN_I_DYE;
			break;
		case IBC_FLIP:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_FLIP;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_FLIP;
				else
					m_Can &= ~CAN_I_FLIP;
			break;
		case IBC_ENCHANT:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_ENCHANT;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_ENCHANT;
				else
					m_Can &= ~CAN_I_ENCHANT;
			break;
		case IBC_EXCEPTIONAL:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_EXCEPTIONAL;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_EXCEPTIONAL;
				else
					m_Can &= ~CAN_I_EXCEPTIONAL;
			break;
		case IBC_IMBUE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_IMBUE;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_IMBUE;
				else
					m_Can &= ~CAN_I_IMBUE;
			break;
		case IBC_REFORGE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_REFORGE;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_REFORGE;
				else
					m_Can &= ~CAN_I_REFORGE;
			break;
		case IBC_RETAINCOLOR:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_RETAINCOLOR;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_RETAINCOLOR;
				else
					m_Can &= ~CAN_I_RETAINCOLOR;
			break;
		case IBC_MAKERSMARK:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_MAKERSMARK;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_MAKERSMARK;
				else
					m_Can &= ~CAN_I_MAKERSMARK;
			break;
		case IBC_RECYCLE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_RECYCLE;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_RECYCLE;
				else
					m_Can &= ~CAN_I_RECYCLE;
			break;
		case IBC_REPAIR:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_REPAIR;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_REPAIR;
				else
					m_Can &= ~CAN_I_REPAIR;
			break;
		case IBC_REPLICATE:
			if ( !s.HasArgs() )
				m_Can |= CAN_I_REPLICATE;
			else
				if ( s.GetArgVal() )
					m_Can |= CAN_I_REPLICATE;
				else
					m_Can &= ~CAN_I_REPLICATE;
			break;
		case IBC_ID:
			{
            if ( GetID() < ITEMID_MULTI )
                {
                    g_Log.EventError( "Setting new ID for base type %s not allowed\n", GetResourceName());
                    return false;
                }
                
                ITEMID_TYPE id = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
                CItemBase * pItemDef = FindItemBase( id );	// make sure the base is loaded.
                if ( ! pItemDef )
                {
                    g_Log.EventError( "Setting unknown base ID=0%x for base type %s\n", id, GetResourceName());
                    return false;
				}
                
                /*
                 * I add Is Duped Item check to check if item is from DUPELIST of base item, and ID won't change to baseid for unnecessarily.
                 * I made this change to fix issue #512 (https://github.com/Sphereserver/Source-X/issues/512)
                 * I leave a note here to know developers why I did this changed
                 * xwerswoodx
                 */
                 if (!pItemDef->IsDupedItem(id))
                    id = ITEMID_TYPE(pItemDef->m_dwDispIndex);

                if ( ! IsValidDispID(id) )
                {
                    if (id >= g_Install.m_tiledata.GetItemMaxIndex())
                        g_Log.EventError("Setting invalid ID=%s for base type %s (value is greater than the tiledata maximum index).\n", s.GetArgStr(), GetResourceName());
                    else
                        g_Log.EventError( "Setting invalid ID=%s for base type %s\n", s.GetArgStr(), GetResourceName());
                    return false;
                }

				CopyBasic( pItemDef );
				m_dwDispIndex = id;	// Might not be the default of a DUPEITEM
			}
			break;

		case IBC_LAYER:
			m_layer = s.GetArgUCVal();
			break;
		case IBC_PILE:
			break;
		case IBC_REQSTR:
			if ( ! IsTypeEquippable())
				return false;
			m_ttEquippable.m_iStrReq = s.GetArgDWVal();
			break;

		case IBC_RESDISPDNID:
			SetResDispDnId((word)(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, s.GetArgStr())));
			break;

		case IBC_SPEED:
			m_speed = s.GetArgBVal();
			break;

		case IBC_SKILL:		// Skill to use.
			m_iSkill = g_Cfg.FindSkillKey( s.GetArgStr() );
			break;

		case IBC_SKILLMAKE:
			// Skill required to make this.
			m_SkillMake.Load( s.GetArgStr() );
            for (const CResourceQty& res : m_SkillMake)
            {
                RES_TYPE type = res.GetResType();
                if ((type != RES_SKILL) && (type != RES_TYPEDEF) && (type != RES_ITEMDEF))
                {
                    g_Log.EventWarn("Invalid requirement in SKILLMAKE (allowed: skill, typedef, itemdef).\n");
                    break;
                }
            }
			break;

		case IBC_TDATA1:
			m_ttNormal.m_tData1 = s.GetArgVal();
			break;
		case IBC_TDATA2:
			m_ttNormal.m_tData2 = s.GetArgVal();
			break;
		case IBC_TDATA3:
			m_ttNormal.m_tData3 = s.GetArgVal();
			break;
		case IBC_TDATA4:
			m_ttNormal.m_tData4 = s.GetArgVal();
			break;

		case IBC_TWOHANDS:
			// In some cases the layer is not set right.
			// override the layer here.
			if ( s.GetArgStr()[0] == '1' || s.GetArgStr()[0] == 'Y' || s.GetArgStr()[0] == 'y' )
			{
				m_layer = LAYER_HAND2;
			}
			break;
		case IBC_TYPE:
			SetType((IT_TYPE)(g_Cfg.ResourceGetIndexType( RES_TYPEDEF, s.GetArgStr())));
			if ( m_type == IT_CONTAINER_LOCKED )
			{
				// At this level it just means to add a key for it.
				SetType(IT_CONTAINER);
			}
			break;
		case IBC_VALUE:
			m_values.Load( s.GetArgRaw() );
			break;
		case IBC_WEIGHT:
			// Read in the weight but it may not be decimalized correctly
			{
				bool fDecimal = ( strchr( s.GetArgStr(), '.' ) != nullptr );
				m_weight = s.GetArgWVal();
				if ( ! fDecimal )
					m_weight *= WEIGHT_UNITS;
			}
			break;
		default:
        {
            return CBaseBaseDef::r_LoadVal(s);
        }
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CItemBase::ReplaceItemBase( CItemBase * pOld, CResourceDef * pNew ) // static
{
	ADDTOCALLSTACK("CItemBase::ReplaceItemBase");
	ASSERT(pOld);
	ASSERT(pOld->GetRefInstances() == 0);
	CResourceID const& rid = pOld->GetResourceID();
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	ASSERT( index != SCONT_BADINDEX );
	g_Cfg.m_ResHash.SetAt( rid, index, pNew );
}

CItemBase * CItemBase::MakeDupeReplacement( CItemBase * pBase, ITEMID_TYPE idmaster ) // static
{
	ADDTOCALLSTACK("CItemBase::MakeDupeReplacement");
	ITEMID_TYPE id = pBase->GetID();
	if ( idmaster == id || ! IsValidDispID( idmaster ))
	{
		DEBUG_ERR(( "CItemBase:DUPEITEM weirdness 0%x==0%x\n", id, idmaster ));
		return pBase;
	}

	CItemBase * pBaseNew = FindItemBase( idmaster );
	if ( pBaseNew == nullptr )
	{
		DEBUG_ERR(( "CItemBase:DUPEITEM not exist 0%x==0%x\n", id, idmaster ));
		return pBase;
	}

	if ( pBaseNew->GetID() != idmaster )
	{
		DEBUG_ERR(( "CItemBase:DUPEITEM circle 0%x==0%x\n", id, idmaster ));
		return pBase;
	}

	if ( ! pBaseNew->IsSameDispID(id))	// already here ?!
		pBaseNew->m_flip_id.emplace_back(id);

	// create the dupe stub.
	CItemBaseDupe * pBaseDupe = new CItemBaseDupe( id, pBaseNew );;
    CUOItemTypeRec_HS tiledata = {};
	if ( CItemBase::GetItemData( id, &tiledata ) )
	{
		pBaseDupe->SetTFlags( tiledata.m_flags );
		height_t Height = CItemBase::GetItemHeightFlags( tiledata, &pBaseDupe->m_Can );
		//Height = ( pBaseDupe->GetTFlags() & 0x400 ) ? ( Height / 2 ) : ( Height ); //should not be done here
		Height = IsID_Chair( id ) ? 0 : Height;
		pBaseDupe->SetHeight( Height );
	}
	ReplaceItemBase( pBase, pBaseDupe );
    delete pBase;   // replaced the old base, delete the old one since it's not used anymore

	return pBaseNew;
}


void CItemBase::SetType(IT_TYPE type)
{
    m_type = type;

    // Add first the most specific components, so that the tooltips will be better ordered
	TrySubscribeAllowedComponentProps<CCPropsItemWeaponRanged>(this);
	TrySubscribeAllowedComponentProps<CCPropsItemWeapon>(this);
	TrySubscribeAllowedComponentProps<CCPropsItemEquippable>(this);
}

//**************************************************
// -CItemBaseMulti

CItemBase * CItemBaseMulti::MakeMultiRegion( CItemBase * pBase, CScript & s ) // static
{
	ADDTOCALLSTACK("CItemBaseMulti::MakeMultiRegion");
	// "MULTIREGION"
	// We must transform this object into a CItemBaseMulti

	if ( !pBase )
		return nullptr;

	if ( ! pBase->IsTypeMulti(pBase->GetType()) )
	{
		DEBUG_ERR(( "MULTIREGION defined for NON-MULTI type 0%x\n", pBase->GetID()));
		return pBase;
	}

	CItemBaseMulti * pMultiBase = dynamic_cast <CItemBaseMulti *>(pBase);
	if ( pMultiBase == nullptr )
	{
		if ( pBase->GetRefInstances() > 0 )
		{
			DEBUG_ERR(( "MULTIREGION defined for IN USE NON-MULTI type 0%x\n", pBase->GetID()));
			return pBase;
		}

		pMultiBase = new CItemBaseMulti( pBase );
		ReplaceItemBase( pBase, pMultiBase );
	}

	pMultiBase->SetMultiRegion( s.GetArgStr());
	return pMultiBase;
}

CItemBaseMulti::CItemBaseMulti( CItemBase* pBase ) :
	CItemBase( pBase->GetID() )
{
    m_dwRegionFlags = REGION_FLAG_NODECAY | REGION_ANTIMAGIC_TELEPORT | REGION_ANTIMAGIC_RECALL_IN | REGION_FLAG_NOBUILDING;
	m_rect.SetRectEmpty();
	_shipSpeed.period = 0;
	_shipSpeed.tiles = 0;
	m_SpeedMode = SMS_SLOW;

	m_Offset.m_dx = 0;
	m_Offset.m_dy = -1;
	m_Offset.m_dz = 0;

    _iBaseStorage = 489;    // Minimum possible value from 7x7 houses.
    _iBaseVendors = 10;     // Minimum possible value from 7x7 houses.
    _iLockdownsPercent = 50;// Default value
    _iMultiCount = 1;       // All Multis have a default 'weight' of 1.

	// copy the stuff from the pBase
	CopyTransfer(pBase);
}

bool CItemBaseMulti::AddComponent( ITEMID_TYPE id, short dx, short dy, char dz )
{
	ADDTOCALLSTACK("CItemBaseMulti::AddComponent");
	m_rect.UnionPoint( dx, dy );
	if ( id > 0 )
	{
		const CItemBase * pItemBase = FindItemBase(id);
		if ( pItemBase == nullptr )	// make sure the item is valid
		{
			g_Log.EventError( "Bad Multi COMPONENT 0%x\n", id );
			return false;
		}

		CItemBaseMulti::CMultiComponentItem comp;
		comp.m_id = id;
		comp.m_dx = dx;
		comp.m_dy = dy;
		comp.m_dz = dz;
		m_Components.emplace_back(std::move(comp));
	}

	return true;
}

//	TODO: (Vjaka) do i really need map plane here?
void CItemBaseMulti::SetMultiRegion( tchar * pArgs )
{
	ADDTOCALLSTACK("CItemBaseMulti::SetMultiRegion");
	// inclusive region.
	int64 piArgs[5];
	size_t iQty = Str_ParseCmds( pArgs, piArgs, CountOf(piArgs));
	if ( iQty <= 1 )
		return;
	m_Components.clear();	// might be after a resync
	m_rect.SetRect( (int)(piArgs[0]), (int)(piArgs[1]), (int)(piArgs[2]+1), (int)(piArgs[3]+1), (int)(piArgs[4]) );
}

bool CItemBaseMulti::AddComponent( tchar * pArgs )
{
	ADDTOCALLSTACK("CItemBaseMulti::AddComponent");
	int64 piArgs[4];
	size_t iQty = Str_ParseCmds( pArgs, piArgs, CountOf(piArgs));
	if ( iQty <= 1 )
		return false;
	return AddComponent((ITEMID_TYPE)(RES_GET_INDEX(piArgs[0])), (short)piArgs[1], (short)piArgs[2], (char)piArgs[3] );
}

int CItemBaseMulti::GetDistanceMax() const
{
	ADDTOCALLSTACK("CItemBaseMulti::GetDistanceMax");
	int iDist = abs( m_rect.m_left );
	int iDistTmp = abs( m_rect.m_top );
	if ( iDistTmp > iDist )
		iDist = iDistTmp;
	iDistTmp = abs( m_rect.m_right + 1 );
	if ( iDistTmp > iDist )
		iDist = iDistTmp;
	iDistTmp = abs( m_rect.m_bottom + 1 );
	if ( iDistTmp > iDist )
		iDist = iDistTmp;
	return (iDist + 1);
}

int CItemBaseMulti::GetDistanceDir(DIR_TYPE dir) const
{
	ADDTOCALLSTACK("CItemBaseMulti::GetDistanceDir");
	ASSERT(dir <= DIR_QTY);

	int iDist = 0;
	switch (dir)
	{
	case DIR_N:
		iDist = m_rect.m_top;
		break;
	case DIR_NE:
		iDist = (m_rect.m_top + m_rect.m_right) / 2;
		break;
	case DIR_E:
		iDist = m_rect.m_right;
		break;
	case DIR_SE:
		iDist = (m_rect.m_right + m_rect.m_bottom) / 2;
		break;
	case DIR_S:
		iDist = m_rect.m_bottom;
		break;
	case DIR_SW:
		iDist = (m_rect.m_bottom + m_rect.m_left) / 2;
		break;
	case DIR_W:
		iDist = m_rect.m_left;
		break;
	case DIR_NW:
		iDist = (m_rect.m_left + m_rect.m_top) / 2;
		break;
	default:
		return 0;
	}
	return abs(iDist);
}

enum MLC_TYPE
{
	MLC_BASECOMPONENT,
    MLC_BASESTORAGE,
    MLC_BASEVENDORS,
	MLC_COMPONENT,
    MLC_LOCKDOWNSPERCENT,
	MLC_MULTIOFFSET,
	MLC_MULTIREGION,
	MLC_REGIONFLAGS,
	MLC_SHIPSPEED,
	MLC_TSPEECH,
	MLC_QTY
};

lpctstr const CItemBaseMulti::sm_szLoadKeys[] =
{
	"BASECOMPONENT",
    "BASESTORAGE",
    "BASEVENDORS",
	"COMPONENT",
    "LOCKDOWNSPERCENT",
	"MULTIOFFSET",
	"MULTIREGION",
	"REGIONFLAGS",
	"SHIPSPEED",
	"TSPEECH",
	nullptr
};

bool CItemBaseMulti::r_LoadVal(CScript &s)
{
    ADDTOCALLSTACK("CItemBaseMulti::r_LoadVal");
    EXC_TRY("LoadVal");
    switch (FindTableSorted(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1))
    {
        case MLC_BASESTORAGE:
            _iBaseStorage = s.GetArgU16Val();
            break;
        case MLC_BASEVENDORS:
            _iBaseVendors = s.GetArgU8Val();
            break;
        case MLC_LOCKDOWNSPERCENT:
            _iLockdownsPercent = s.GetArgU8Val();
            break;
        case MLC_COMPONENT:
            return AddComponent(s.GetArgStr());
		case MLC_MULTIOFFSET:
		{
			int64 ppArgs[3];
			size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, CountOf(ppArgs));
			if (iQty < 1)
				return false;

			m_Offset.m_dx = (short)(ppArgs[0]);
			m_Offset.m_dy = (short)(ppArgs[1]);
			m_Offset.m_dz = (char)(ppArgs[2]);
		} break;
        case MLC_MULTIREGION:
            MakeMultiRegion(this, s);
            break;
        case MLC_REGIONFLAGS:
            m_dwRegionFlags = s.GetArgDWVal();
            break;
        case MLC_SHIPSPEED:
        {
            if (!IsType(IT_SHIP))	// only valid for ships
                return false;

            // SHIPSPEED x[,y]
            int64 ppArgs[2];
            size_t iQty = Str_ParseCmds(s.GetArgRaw(), ppArgs, CountOf(ppArgs));
            if (iQty < 1)
                return false;

            _shipSpeed.period = (uchar)(ppArgs[0]);

            if (iQty >= 2)
                _shipSpeed.tiles = (uchar)(ppArgs[1]);
        }
        break;
        case MLC_TSPEECH:
            return(m_Speech.r_LoadVal(s, RES_SPEECH));
        default:
            return(CItemBase::r_LoadVal(s));
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_SCRIPT;
    EXC_DEBUG_END;
    return false;
}

bool CItemBaseMulti::r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pChar, bool fNoCallParent, bool fNoCallChildren)
{
    UNREFERENCED_PARAMETER(fNoCallChildren);
    ADDTOCALLSTACK("CItemBaseMulti::r_WriteVal");
    EXC_TRY("WriteVal");
    switch (FindTableHeadSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1))
    {
        case MLC_BASESTORAGE:
            sVal.FormatU16Val(_iBaseStorage);
            break;
        case MLC_BASEVENDORS:
            sVal.FormatU8Val(_iBaseVendors);
            break;
        case MLC_LOCKDOWNSPERCENT:
            sVal.FormatU8Val(_iLockdownsPercent);
            break;
        case MLC_BASECOMPONENT:
        {
            ptcKey += 13;
            const CUOMulti* pMulti = g_Cfg.GetMultiItemDefs(GetDispID());
            if (pMulti == nullptr)
                return false;

            if (*ptcKey == '\0')
            {
                sVal.FormatSTVal(pMulti->GetItemCount());
            }
            else if (*ptcKey == '.')
            {
                SKIP_SEPARATORS(ptcKey);
                size_t index = Exp_GetVal(ptcKey);
                if (index >= pMulti->GetItemCount())
                    return false;
                SKIP_SEPARATORS(ptcKey);
                const CUOMultiItemRec_HS* item = pMulti->GetItem(index);

                if (*ptcKey == '\0') { sVal.Format("%u,%i,%i,%i", item->m_wTileID, item->m_dx, item->m_dy, item->m_dz); return true; }
                else if (!strnicmp(ptcKey, "ID", 2)) { sVal.FormatVal(item->m_wTileID); return true; }
                else if (!strnicmp(ptcKey, "DX", 2)) { sVal.FormatVal(item->m_dx); return true; }
                else if (!strnicmp(ptcKey, "DY", 2)) { sVal.FormatVal(item->m_dy); return true; }
                else if (!strnicmp(ptcKey, "DZ", 2)) { sVal.FormatVal(item->m_dz); return true; }
                else if (!strnicmp(ptcKey, "D", 1)) { sVal.Format("%i,%i,%i", item->m_dx, item->m_dy, item->m_dz); return true; }
                else if (!strnicmp(ptcKey, "VISIBLE", 7)) { sVal.FormatVal(item->m_visible); return true; }
                else return false;
            }
            else
                return false;

            return true;
        }
        case MLC_COMPONENT:
        {
            ptcKey += 9;
            if (*ptcKey == '\0')
            {
                sVal.FormatSTVal(m_Components.size());
            }
            else if (*ptcKey == '.')
            {
                SKIP_SEPARATORS(ptcKey);
				const llong iIndex = Exp_GetLLVal(ptcKey);
				if (iIndex < 0)
					return false;
                if ((size_t)iIndex >= m_Components.size())
                    return false;

                SKIP_SEPARATORS(ptcKey);
                const CMultiComponentItem& item = m_Components[iIndex];

                if (!strnicmp(ptcKey, "ID", 2)) sVal.FormatVal(item.m_id);
                else if (!strnicmp(ptcKey, "DX", 2)) sVal.FormatVal(item.m_dx);
                else if (!strnicmp(ptcKey, "DY", 2)) sVal.FormatVal(item.m_dy);
                else if (!strnicmp(ptcKey, "DZ", 2)) sVal.FormatVal(item.m_dz);
                else if (!strnicmp(ptcKey, "D", 1)) sVal.Format("%i,%i,%i", item.m_dx, item.m_dy, item.m_dz);
                else sVal.Format("%u,%i,%i,%i", item.m_id, item.m_dx, item.m_dy, item.m_dz);
            }
            else
                return false;
            return true;
        }
		case MLC_MULTIOFFSET:
			sVal.Format("%d,%d,%d", m_Offset.m_dx, m_Offset.m_dy, m_Offset.m_dz);
			return true;
        case MLC_MULTIREGION:
            sVal.Format("%d,%d,%d,%d", m_rect.m_left, m_rect.m_top, m_rect.m_right - 1, m_rect.m_bottom - 1);
            return true;
        case MLC_REGIONFLAGS:
            sVal.FormatHex(m_dwRegionFlags);
            return true;
        case MLC_SHIPSPEED:
        {
            if (!IsType(IT_SHIP))
                return false;

            ptcKey += 9;
            if (*ptcKey == '.')
            {
                ++ptcKey;
                if (!strnicmp(ptcKey, "TILES", 5))
                {
                    sVal.FormatVal(_shipSpeed.tiles);
                    break;
                }
                else if (!strnicmp(ptcKey, "PERIOD", 6))
                {
                    sVal.FormatVal(_shipSpeed.period);
                    break;
                }
                return false;
            }

            sVal.Format("%d,%d", _shipSpeed.period, _shipSpeed.tiles);
            break;
        }
        default:
            return (fNoCallParent ? false : CItemBase::r_WriteVal(ptcKey, sVal, pChar));
    }
    return true;
    EXC_CATCH;

    EXC_DEBUG_START;
    EXC_ADD_KEYRET(pChar);
    EXC_DEBUG_END;
    return false;
}

//**************************************************

CItemBase * CItemBase::FindItemBase( ITEMID_TYPE id ) // static
{
    ADDTOCALLSTACK_INTENSIVE("CItemBase::FindItemBase");
	// CItemBase is a like item already loaded.
	if ( id <= ITEMID_NOTHING )
		return nullptr;

	CResourceID rid = CResourceID( RES_ITEMDEF, id );
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index == SCONT_BADINDEX )
		return nullptr;

	CResourceDef * pBaseStub = g_Cfg.m_ResHash.GetAt( rid, index );
	ASSERT(pBaseStub);

	CItemBase * pBase = dynamic_cast <CItemBase *>(pBaseStub);
	if ( pBase )
		return pBase;	// already loaded all base info.

	const CItemBaseDupe * pBaseDupe = dynamic_cast <const CItemBaseDupe *>(pBaseStub);
	if ( pBaseDupe )
		return( pBaseDupe->GetItemDef() );	// this is just a dupeitem

    // The rid was added to the ResourceHash, but it's not linked yet to a CItemBase (we do it on the first request).
	CResourceLink * pBaseLink = dynamic_cast <CResourceLink *>(pBaseStub);
	ASSERT(pBaseLink);

	pBase = new CItemBase( id );
	pBase->CResourceLink::CopyTransfer( pBaseLink );
	g_Cfg.m_ResHash.SetAt( rid, index, pBase );	// Replace with new in sorted order.

	// Find the previous one in the series if any.
	// Find it's script section offset.

	CResourceLock s;
	if ( ! pBase->ResourceLock(s))
	{
		// must be scripted. not in the artwork set.
		g_Log.Event( LOGL_ERROR, "UN-scripted item 0%0x NOT allowed.\n", id );
		return nullptr;
	}

	// Scan the item definition for keywords such as DUPEITEM and
	// MULTIREGION, as these will adjust how our definition is processed
	CScriptLineContext scriptStartContext = s.GetContext();
	while ( s.ReadKeyParse())
	{
		if (s.IsKey("DUPEITEM"))
		{
			return MakeDupeReplacement(pBase, (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, s.GetArgStr())));
		}
		else if ( s.IsKey( "MULTIREGION" ))
		{
			// Upgrade the CItemBase::pBase to the CItemBaseMulti.
			pBase = CItemBaseMulti::MakeMultiRegion( pBase, s );
			continue;
		}
		else if (s.IsKeyHead("ON", 2))	// trigger scripting marks the end
		{
			break;
		}
		else if (s.IsKey("ID") || s.IsKey("TYPE"))	// These are required for CItemBaseMulti::MakeMultiRegion to function correctly
		{
			pBase->r_LoadVal(s);
		}
	}

	// Return to the start of the item script
	s.SeekContext(scriptStartContext);

	// Read the Script file preliminary.
	while ( s.ReadKeyParse() )
	{
		if ( s.IsKey( "DUPEITEM" ) || s.IsKey( "MULTIREGION" ))
			continue;
		else if ( s.IsKeyHead( "ON", 2 ))	// trigger scripting marks the end
			break;

		pBase->r_LoadVal( s );
	}

	return pBase;
}


//**************************************************

CItemBaseDupe::CItemBaseDupe(ITEMID_TYPE id, CItemBase* pMasterItem) :
	CResourceDef(CResourceID(RES_ITEMDEF, id)),
	m_MasterItem(pMasterItem),
	m_qwFlags(0), m_Height(0), m_Can(0)
{
	ASSERT(pMasterItem);
	ASSERT(pMasterItem->GetResourceID().GetResIndex() != id);
}

CItemBaseDupe * CItemBaseDupe::GetDupeRef( ITEMID_TYPE id ) // static
{
	ADDTOCALLSTACK("CItemBaseDupe::GetDupeRef");
	if ( id <= 0 )
		return nullptr;

	CResourceID rid = CResourceID( RES_ITEMDEF, id );
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index == SCONT_BADINDEX )
		return nullptr;

	CResourceDef * pBaseStub = g_Cfg.m_ResHash.GetAt( rid, index );

	CItemBase * pBase = dynamic_cast <CItemBase *>(pBaseStub);
	if ( pBase )
		return nullptr; //We want to return Dupeitem, not Baseitem

	CItemBaseDupe * pBaseDupe = dynamic_cast <CItemBaseDupe *>(pBaseStub);
	if ( pBaseDupe )
		return pBaseDupe;	// this is just a dupeitem

	return nullptr; //we suspect item is loaded
}

void CItemBaseDupe::UnLink()
{
	m_MasterItem.SetRef(nullptr);
	CResourceDef::UnLink();
}

CItemBase* CItemBaseDupe::GetItemDef() const
{
	CResourceLink* pLink = m_MasterItem.GetRef();
	ASSERT(pLink);
	CItemBase* pItemDef = dynamic_cast <CItemBase*>(pLink);
	ASSERT(pItemDef);
	return pItemDef;
}
