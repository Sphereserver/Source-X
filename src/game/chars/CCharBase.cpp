
#include "../../common/CException.h"
#include "../../common/CLog.h"
#include "../CServerConfig.h"
#include "CCharBase.h"

/////////////////////////////////////////////////////////////////
// -CCharBase

CCharBase::CCharBase( CREID_TYPE id ) :
	CBaseBaseDef( CResourceID( RES_CHARDEF, id ))
{
	m_iHireDayWage = 0;
	m_trackID = ITEMID_TRACK_WISP;
	m_soundbase = 0;
	m_defense = 0;
	m_defenseBase = 0;
	m_defenseRange = 0;
	m_Anims = 0xFFFFFF;
  	m_MaxFood = 0;			// Default value
	m_wBloodHue = 0;
	m_wColor = 0;
	m_Str = 0;
	m_Dex = 0;
	m_Int = 0;

	m_iMoveRate = (short)(g_Cfg.m_iMoveRate);

	if ( IsValidDispID(id))
		m_dwDispIndex = id;	// in display range.
	else
		m_dwDispIndex = 0;	// must read from SCP file later

	SetResDispDnId(CREID_MAN);
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
	if ( pSpace == NULL )
		return pName;

	pSpace++;
	if ( ! strnicmp( pSpace, "the ", 4 ))
		pSpace += 4;
	return pSpace;
}

void CCharBase::CopyBasic( const CCharBase * pCharDef )
{
	ADDTOCALLSTACK("CCharBase::CopyBasic");
	m_trackID = pCharDef->m_trackID;
	m_soundbase = pCharDef->m_soundbase;

	m_wBloodHue = pCharDef->m_wBloodHue;
	m_MaxFood = pCharDef->m_MaxFood;
	m_FoodType = pCharDef->m_FoodType;
	m_Desires = pCharDef->m_Desires;

	m_defense = pCharDef->m_defense;
	m_Anims = pCharDef->m_Anims;

	m_BaseResources = pCharDef->m_BaseResources;

	CBaseBaseDef::CopyBasic( pCharDef );	// This will overwrite the CResourceLink!!
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
	if ( pCharDef == NULL )
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
	for ( size_t i = 0; i < m_FoodType.GetCount(); i++ )
	{
		if ( m_MaxFood < m_FoodType[i].GetResQty())
			m_MaxFood = (short)(m_FoodType[i].GetResQty());
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
	NULL
};

bool CCharBase::r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc )
{
	UNREFERENCED_PARAMETER(pSrc);
	ADDTOCALLSTACK("CCharBase::r_WriteVal");
	EXC_TRY("WriteVal");
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		//return as string or hex number or NULL if not set
		case CBC_THROWDAM:
		case CBC_THROWOBJ:
		case CBC_THROWRANGE:
			sVal = GetDefStr(pszKey, false);
			break;
		//return as decimal number or 0 if not set
		case CBC_FOLLOWERSLOTS:
		case CBC_MAXFOLLOWER:
		case CBC_BONDED:
		case CBC_TITHING:
			sVal.FormatLLVal(GetDefNum(pszKey, true));
			break;
		case CBC_ANIM:
			sVal.FormatHex( m_Anims );
			break;
		case CBC_AVERSIONS:
			{
				tchar *pszTmp = Str_GetTemp();
				m_Aversions.WriteKeys(pszTmp);
				sVal = pszTmp;
			}
			break;
		case CBC_CAN:
			sVal.FormatHex( m_Can);
			break;
		case CBC_BLOODCOLOR:
			sVal.FormatHex( m_wBloodHue );
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
		case CBC_RESDISPDNID:
			sVal = g_Cfg.ResourceGetName( CResourceID( RES_CHARDEF, GetResDispDnId()));
			break;
		case CBC_SOUND:
			sVal.FormatHex( m_soundbase );
			break;
		case CBC_STR:
			sVal.FormatVal( m_Str );
			break;
		case CBC_TSPEECH:
			m_Speech.WriteResourceRefList( sVal );
			break;
		default:
			return( CBaseBaseDef::r_WriteVal( pszKey, sVal ));
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
	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		//Set as Strings
		case CBC_THROWDAM:
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
		case CBC_CAN:
			m_Can = s.GetArgVal();
			break;
		case CBC_BLOODCOLOR:
			m_wBloodHue = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case CBC_ARMOR:
			m_defense = (word)(s.GetArgVal());
			break;
		case CBC_COLOR:
			m_wColor = static_cast<HUE_TYPE>(s.GetArgVal());
			break;
		case CBC_DESIRES:
			m_Desires.Load( s.GetArgStr() );
			break;
		case CBC_DEX:
			m_Dex = (short)(s.GetArgVal());
			break;
		case CBC_DISPID:
			return false;
		case CBC_FOODTYPE:
			SetFoodType( s.GetArgStr());
			break;
		case CBC_HIREDAYWAGE:
			m_iHireDayWage = s.GetArgVal();
			break;
		case CBC_ICON:
			{
				ITEMID_TYPE id = static_cast<ITEMID_TYPE>(g_Cfg.ResourceGetIndexType( RES_ITEMDEF, s.GetArgStr()));
				if ( id < 0 || id >= ITEMID_MULTI )
				{
					return false;
				}
				m_trackID = id;
			}
			break;
		case CBC_ID:
			{
				return SetDispID(static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType( RES_CHARDEF, s.GetArgStr())));
			}
		case CBC_INT:
			m_Int = (short)(s.GetArgVal());
			break;
		case CBC_MAXFOOD:
			m_MaxFood = (short)(s.GetArgVal());
			break;
		case CBC_MOVERATE:
			m_iMoveRate = (short)(s.GetArgVal());
			break;
		case CBC_RESDISPDNID:
			SetResDispDnId((word)(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr())));
			break;
		case CBC_SOUND:
			m_soundbase = static_cast<SOUND_TYPE>(s.GetArgVal());
			break;
		case CBC_STR:
			m_Str = (short)(s.GetArgVal());
			break;
		case CBC_TSPEECH:
			return( m_Speech.r_LoadVal( s, RES_SPEECH ));
		default:
			return( CBaseBaseDef::r_LoadVal( s ));
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
	if ( m_Can == CAN_C_INDOORS )
		g_Log.Event(LOGL_WARN, "Char script '%s' has no CAN flags specified!\n", GetResourceName());

	return true;
}

////////////////////////////////////////////

// find it (or near it) if already loaded.
CCharBase * CCharBase::FindCharBase( CREID_TYPE baseID ) // static
{
	ADDTOCALLSTACK("CCharBase::FindCharBase");
	CResourceID rid = CResourceID( RES_CHARDEF, baseID );
	size_t index = g_Cfg.m_ResHash.FindKey(rid);
	if ( index == g_Cfg.m_ResHash.BadIndex() )
		return NULL;

	CResourceLink * pBaseLink = static_cast <CResourceLink *> ( g_Cfg.m_ResHash.GetAt(rid,index));
	ASSERT(pBaseLink);
	CCharBase * pBase = dynamic_cast <CCharBase *> (pBaseLink);
	if ( pBase )
		return( pBase );	// already loaded.

	// create a new base.
	pBase = new CCharBase(baseID);
	ASSERT(pBase);
	pBase->CResourceLink::CopyTransfer(pBaseLink);

	// replace existing one
	g_Cfg.m_ResHash.SetAt(rid, index, pBase);

	// load it's data on demand.
	CResourceLock s;
	if ( !pBase->ResourceLock(s))
		return NULL;
	if ( !pBase->r_Load(s))
		return NULL;

	return pBase;
}

