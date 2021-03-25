
#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../../common/CLog.h"
#include "../components/CCPropsChar.h"
#include "../components/CCPropsItemChar.h"
#include "../CServerConfig.h"
#include "CCharBase.h"

/////////////////////////////////////////////////////////////////
// -CCharBase

CCharBase::CCharBase( CREID_TYPE id ) :
	CBaseBaseDef( CResourceID( RES_CHARDEF, id ))
{
	m_iHireDayWage = 0;
	m_trackID = ITEMID_TRACK_WISP;
	m_soundBase = SOUND_NONE;
	m_soundIdle = m_soundNotice = m_soundHit = m_soundGetHit = m_soundDie = SOUND_NONE;
	m_defense = 0;
	m_defenseBase = 0;
	m_defenseRange = 0;
	m_Anims = 0xFFFFFF;
  	m_MaxFood = 0;			// Default value
	_wBloodHue = 0;
	m_wColor = 0;
	m_Str = 0;
	m_Dex = 0;
	m_Int = 0;
    _uiRange = 0;

	_iEraLimitGear = g_Cfg._iEraLimitGear;		// Always latest by default
	_iEraLimitLoot = g_Cfg._iEraLimitLoot;		// Always latest by default

	m_iMoveRate = (short)(g_Cfg.m_iMoveRate);

	if ( IsValidDispID(id))
		m_dwDispIndex = id;	// in display range.
	else
		m_dwDispIndex = 0;	// must read from SCP file later

	SetResDispDnId(CREID_MAN);

    // SubscribeComponent Prop Components
    TrySubscribeComponentProps<CCPropsChar>();
    TrySubscribeComponentProps<CCPropsItemChar>();
}


// From "Bill the carpenter" or "#HUMANMALE the Carpenter",
// Get "Carpenter"
lpctstr CCharBase::GetTradeName() const
{
	ADDTOCALLSTACK("CCharBase::GetTradeName");
	lpctstr pName = CBaseBaseDef::GetTypeName();
	if ( pName[0] != '#' )
		return pName;

	lpctstr pSpace = strchr( pName, ' ' );
	if ( pSpace == nullptr )
		return pName;

	++pSpace;
	if ( ! strnicmp( pSpace, "the ", 4 ))
		pSpace += 4;
	return pSpace;
}

void CCharBase::CopyBasic( const CCharBase * pCharDef )
{
	ADDTOCALLSTACK("CCharBase::CopyBasic");
	m_trackID = pCharDef->m_trackID;

	m_soundBase = pCharDef->m_soundBase;
	m_soundIdle = pCharDef->m_soundIdle;
	m_soundNotice = pCharDef->m_soundNotice;
	m_soundHit = pCharDef->m_soundHit;
	m_soundGetHit = pCharDef->m_soundGetHit;
	m_soundDie = pCharDef->m_soundDie;

	_wBloodHue = pCharDef->_wBloodHue;
	m_MaxFood = pCharDef->m_MaxFood;
	m_FoodType = pCharDef->m_FoodType;
	m_Desires = pCharDef->m_Desires;

	m_defense = pCharDef->m_defense;
	m_Anims = pCharDef->m_Anims;
    _uiRange = pCharDef->_uiRange;

	m_BaseResources = pCharDef->m_BaseResources;
    _pFaction = pCharDef->_pFaction;

	CBaseBaseDef::CopyBasic( pCharDef );	// This will overwrite the CResourceLink!!
}

void CCharBase::UnLink()
{
    // We are being reloaded .
    m_Speech.clear();
    m_FoodType.clear();
    m_Desires.clear();
    CBaseBaseDef::UnLink();
}

// Setting the visual "ID" for this.
bool CCharBase::SetDispID( CREID_TYPE id )
{
	ADDTOCALLSTACK("CCharBase::SetDispID");
	if ( id == GetID())
		return true;
	if ( id == GetDispID())
		return true;
	if ( ! IsValidDispID( id ))
	{
		DEBUG_ERR(( "Creating char SetDispID(0%x) > %d\n", id, CREID_QTY ));
		return false; // must be in display range.
	}

	// Copy the rest of the stuff from the display base.
	CCharBase * pCharDef = FindCharBase( id );
	if ( pCharDef == nullptr )
	{
		DEBUG_ERR(( "Creating char SetDispID(0%x) BAD\n", id ));
		return false;
	}

	CopyBasic( pCharDef );
	return true;
}

// Setting what do I eat
void CCharBase::SetFoodType( lpctstr pszFood )
{
	ADDTOCALLSTACK("CCharBase::SetFoodType");
  	m_FoodType.Load( pszFood );

  	// Try to determine the real value
	m_MaxFood = 0;
	for ( size_t i = 0; i < m_FoodType.size(); ++i )
	{
		if ( m_MaxFood < m_FoodType[i].GetResQty())
			m_MaxFood = (ushort)(m_FoodType[i].GetResQty());
	}
}

enum CBC_TYPE
{
	#define ADD(a,b) CBC_##a,
	#include "../../tables/CCharBase_props.tbl"
	#undef ADD
	CBC_QTY
};

lpctstr const CCharBase::sm_szLoadKeys[CBC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../../tables/CCharBase_props.tbl"
	#undef ADD
	nullptr
};

bool CCharBase::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
	ADDTOCALLSTACK("CCharBase::r_WriteVal");
    EXC_TRY("WriteVal");

    // Checking Props CComponents first
    EXC_SET_BLOCK("EntityProps");
    if (!fNoCallChildren && CEntityProps::r_WritePropVal(ptcKey, sVal, nullptr, this))
    {
        return true;
    }

    EXC_SET_BLOCK("Keyword");
	switch ( FindTableSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		//return as string or hex number or nullptr if not set
		case CBC_THROWDAM:
		case CBC_THROWDAMTYPE:
		case CBC_THROWOBJ:
		case CBC_THROWRANGE:
			sVal = GetDefStr(ptcKey, false);
			break;
		//return as decimal number or 0 if not set
		case CBC_FOLLOWERSLOTS:
		case CBC_MAXFOLLOWER:
		case CBC_BONDED:
		case CBC_TITHING:
			sVal.FormatLLVal(GetDefNum(ptcKey));
			break;
		case CBC_ANIM:
			sVal.FormatLLHex( m_Anims );
			break;
		case CBC_AVERSIONS:
			{
				tchar *pszTmp = Str_GetTemp();
				m_Aversions.WriteKeys(pszTmp);
				sVal = pszTmp;
			}
			break;
		case CBC_BLOODCOLOR:
			sVal.FormatHex( _wBloodHue );
			break;
		case CBC_ARMOR:
			sVal.FormatVal( m_defense );
			break;
		case CBC_COLOR:
			sVal.FormatHex( m_wColor );
			break;
		case CBC_DESIRES:
			{
				tchar *pszTmp = Str_GetTemp();
				m_Desires.WriteKeys(pszTmp);
				sVal = pszTmp;
			}
			break;
		case CBC_DEX:
			sVal.FormatVal( m_Dex );
			break;
		case CBC_DISPID:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_CHARDEF, GetDispID()));
			break;
		case CBC_ERALIMITGEAR:
			sVal.FormatVal(_iEraLimitGear);
			break;
		case CBC_ERALIMITLOOT:
			sVal.FormatVal(_iEraLimitLoot);
			break;
		case CBC_ERALIMITPROPS:
			sVal.FormatVal(_iEraLimitProps);
			break;
		case CBC_ID:
			sVal.FormatHex( GetDispID() );
			break;
		case CBC_FOODTYPE:
			{
				tchar *pszTmp = Str_GetTemp();
				m_FoodType.WriteKeys(pszTmp);
				sVal = pszTmp;
			}
			break;
		case CBC_HIREDAYWAGE:
			sVal.FormatVal( m_iHireDayWage );
			break;
		case CBC_ICON:
			sVal.FormatHex( m_trackID );
			break;
		case CBC_INT:
			sVal.FormatVal( m_Int );
			break;
		case CBC_JOB:
			sVal = GetTradeName();
			break;
		case CBC_MAXFOOD:
			sVal.FormatVal( m_MaxFood );
			break;
		case CBC_MOVERATE:
			sVal.FormatVal(m_iMoveRate);
			break;
        case CBC_RANGE:
        {
            const uchar iRangeH = GetRangeH(), iRangeL = GetRangeL();
            if ( iRangeL == 0 )
                sVal.Format( "%hhd", iRangeH );
            else
                sVal.Format( "%hhd,%hhd", iRangeH, iRangeL );
            break;
        }
        case CBC_RANGEH:
            sVal.FormatBVal(GetRangeH());
            break;
        case CBC_RANGEL:
            sVal.FormatBVal(GetRangeL());
            break;
		case CBC_RESDISPDNID:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_CHARDEF, (int)GetResDispDnId()) );
			break;
		case CBC_SOUND:
			sVal.FormatHex( m_soundBase );
			break;
		case CBC_SOUNDDIE:
			sVal.FormatHex( m_soundDie );
			break;
		case CBC_SOUNDGETHIT:
			sVal.FormatHex( m_soundGetHit );
			break;
		case CBC_SOUNDHIT:
			sVal.FormatHex( m_soundHit );
			break;
		case CBC_SOUNDIDLE:
			sVal.FormatHex( m_soundIdle );
			break;
		case CBC_SOUNDNOTICE:
			sVal.FormatHex( m_soundNotice );
			break;
		case CBC_STR:
			sVal.FormatVal( m_Str );
			break;
		case CBC_TSPEECH:
			m_Speech.WriteResourceRefList( sVal );
			break;
		default:
        {
            return (fNoCallParent ? false : CBaseBaseDef::r_WriteVal(ptcKey, sVal, pSrc, false));
        }
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CCharBase::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CCharBase::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( ! s.HasArgs())
		return false;

    // Checking Props CComponents first
    EXC_SET_BLOCK("EntityProps");
    if (CEntityProps::r_LoadPropVal(s, nullptr, this))
    {
        return true;
    }

    EXC_SET_BLOCK("Keyword");
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		//Set as Strings
		case CBC_THROWDAM:
		case CBC_THROWDAMTYPE:
		case CBC_THROWOBJ:
		case CBC_THROWRANGE:
			{
				bool fQuoted = false;
				SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
			}
			break;
		//Set as number only
		case CBC_FOLLOWERSLOTS:
		case CBC_MAXFOLLOWER:
		case CBC_BONDED:
		case CBC_TITHING:
			SetDefNum(s.GetKey(), s.GetArgVal(), false);
			break;

		case CBC_ANIM:
			m_Anims = s.GetArgVal();
			break;
		case CBC_AVERSIONS:
			m_Aversions.Load( s.GetArgStr() );
			break;
		case CBC_BLOODCOLOR:
			_wBloodHue = (HUE_TYPE)(s.GetArgVal());
			break;
		case CBC_ARMOR:
			m_defense = s.GetArgWVal();
			break;
		case CBC_COLOR:
			m_wColor = (HUE_TYPE)(s.GetArgVal());
			break;
		case CBC_DESIRES:
			m_Desires.Load( s.GetArgStr() );
			break;
		case CBC_DEX:
			m_Dex = s.GetArgUSVal();
			break;
		case CBC_DISPID:
			return false;
		case CBC_ERALIMITGEAR:
			_iEraLimitGear = (RESDISPLAY_VERSION)s.GetArgVal();
			break;
		case CBC_ERALIMITLOOT:
			_iEraLimitLoot = (RESDISPLAY_VERSION)s.GetArgVal();
			break;
		case CBC_ERALIMITPROPS:
			_iEraLimitProps = (RESDISPLAY_VERSION)s.GetArgVal();
			break;
		case CBC_FOODTYPE:
			SetFoodType( s.GetArgStr());
			break;
		case CBC_HIREDAYWAGE:
			m_iHireDayWage = s.GetArgVal();
			break;
		case CBC_ICON:
			{
				ITEMID_TYPE id = (ITEMID_TYPE)(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
				if ( (id < 0) || (id >= ITEMID_MULTI) )
					return false;
				m_trackID = id;
			}
			break;
		case CBC_ID:
			return SetDispID((CREID_TYPE)(g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr())));
		case CBC_INT:
			m_Int = s.GetArgUSVal();
			break;
		case CBC_MAXFOOD:
			m_MaxFood = s.GetArgUSVal();
			break;
		case CBC_MOVERATE:
			m_iMoveRate = s.GetArgUSVal();
			break;
        case CBC_RANGE:
        {
            _uiRange = ConvertRangeStr(s.GetArgStr());
            break;
        }
        case CBC_RANGEH:
        case CBC_RANGEL:
            return false;
		case CBC_RESDISPDNID:
			SetResDispDnId((word)(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr())));
			break;
		case CBC_SOUND:
			m_soundBase = (SOUND_TYPE)(s.GetArgVal());
			break;
		case CBC_SOUNDDIE:
			m_soundDie = (SOUND_TYPE)(s.GetArgVal());
			break;
		case CBC_SOUNDGETHIT:
			m_soundGetHit = (SOUND_TYPE)(s.GetArgVal());
			break;
		case CBC_SOUNDHIT:
			m_soundHit = (SOUND_TYPE)(s.GetArgVal());
			break;
		case CBC_SOUNDIDLE:
			m_soundIdle = (SOUND_TYPE)(s.GetArgVal());
			break;
		case CBC_SOUNDNOTICE:
			m_soundNotice = (SOUND_TYPE)(s.GetArgVal());
			break;
		case CBC_STR:
			m_Str = s.GetArgUSVal();
			break;
		case CBC_TSPEECH:
			return m_Speech.r_LoadVal( s, RES_SPEECH );
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

bool CCharBase::r_Load( CScript & s )
{
	ADDTOCALLSTACK("CCharBase::r_Load");
	// Do a prelim read from the script file.
	CScriptObj::r_Load(s);

	if ( m_sName.IsEmpty() )
	{
		g_Log.EventError("Char script '%s' has no name!\n", GetResourceName());
		return false;
	}

	if ( !IsValidDispID(GetDispID()) )
	{
 		g_Log.Event(LOGL_WARN, "Char script '%s' has bad DISPID 0%x. Defaulting to 0%x.\n", GetResourceName(), GetDispID(), (int)(CREID_MAN));
		m_dwDispIndex = CREID_MAN;
	}
	if ( m_Can == 0 )
		g_Log.Event(LOGL_WARN, "Char script '%s' has no CAN flags specified!\n", GetResourceName());

	return true;
}


byte CCharBase::GetRangeL() const noexcept
{
    return (byte)(RANGE_GET_LO(_uiRange));
}

byte CCharBase::GetRangeH() const noexcept
{
    return (byte)(RANGE_GET_HI(_uiRange));
}


////////////////////////////////////////////

// find it (or near it) if already loaded.
CCharBase * CCharBase::FindCharBase( CREID_TYPE baseID ) // static
{
	ADDTOCALLSTACK("CCharBase::FindCharBase");
    if (baseID <= CREID_INVALID)
        return nullptr;

	const CResourceID rid( RES_CHARDEF, baseID );
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index == SCONT_BADINDEX )
		return nullptr;

	CResourceLink * pBaseLink = static_cast <CResourceLink *> ( g_Cfg.m_ResHash.GetAt(rid,index));
	ASSERT(pBaseLink);
	CCharBase * pBase = dynamic_cast <CCharBase *> (pBaseLink);
	if ( pBase )
		return pBase;	// already loaded.

	// create a new base.
	pBase = new CCharBase(baseID);
	ASSERT(pBase);
	pBase->CResourceLink::CopyTransfer(pBaseLink);

	// replace existing one
	g_Cfg.m_ResHash.SetAt(rid, index, pBase);

	// load it's data on demand.
	CResourceLock s;
	if ( !pBase->ResourceLock(s))
		return nullptr;
	if ( !pBase->r_Load(s))
		return nullptr;

	return pBase;
}

bool CCharBase::IsValidDispID( CREID_TYPE id ) noexcept //  static
{
    return( id > 0 && id < CREID_QTY );
}

bool CCharBase::IsPlayableID( CREID_TYPE id, bool bCheckGhost) noexcept
{
    return ( CCharBase::IsHumanID( id, bCheckGhost) || CCharBase::IsElfID( id, bCheckGhost) || CCharBase::IsGargoyleID( id, bCheckGhost));
}

bool CCharBase::IsHumanID( CREID_TYPE id, bool bCheckGhost ) noexcept // static
{
    if ( bCheckGhost == true)
        return( id == CREID_MAN || id == CREID_WOMAN || id == CREID_EQUIP_GM_ROBE  || id == CREID_GHOSTMAN || id == CREID_GHOSTWOMAN);
    else
        return( id == CREID_MAN || id == CREID_WOMAN || id == CREID_EQUIP_GM_ROBE);
}

bool CCharBase::IsElfID( CREID_TYPE id, bool bCheckGhost ) noexcept // static
{
    if ( bCheckGhost == true)
        return( id == CREID_ELFMAN || id == CREID_ELFWOMAN || id == CREID_ELFGHOSTMAN || id == CREID_ELFGHOSTWOMAN);
    else
        return( id == CREID_ELFMAN || id == CREID_ELFWOMAN );
}

bool CCharBase::IsGargoyleID( CREID_TYPE id, bool bCheckGhost ) noexcept // static
{
    if ( bCheckGhost == true)
        return( id == CREID_GARGMAN || id == CREID_GARGWOMAN || id == CREID_GARGGHOSTMAN || id == CREID_GARGGHOSTWOMAN );
    else
        return( id == CREID_GARGMAN || id == CREID_GARGWOMAN );
}
