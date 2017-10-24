
#include "../common/CException.h"
#include "../common/common.h"
#include "CBase.h"
#include "CServerConfig.h"

enum OBC_TYPE
{
	#define ADD(a,b) OBC_##a,
	#include "../tables/CBaseBaseDef_props.tbl"
	#undef ADD
	OBC_QTY
};

lpctstr const CBaseBaseDef::sm_szLoadKeys[OBC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CBaseBaseDef_props.tbl"
	#undef ADD
	NULL
};


CBaseBaseDef::CBaseBaseDef( CResourceID id ) :
	CResourceLink( id )
{
	m_dwDispIndex			= 0;	// Assume nothing til told differently.
	m_attackBase			= 0;
	m_attackRange			= 0;
	m_defenseBase			= 0;
	m_defenseRange			= 0;
	m_Height				= 0;
	m_Can					= CAN_C_INDOORS;	// most things can cover us from the weather.
	SetDefNum("RANGE",1);	//m_range			= 1;
	m_ResLevel				= RDS_T2A;
	m_ResDispDnHue			= HUE_DEFAULT;
	m_ResDispDnId			= 0;
	m_BaseResources.setNoMergeOnLoad();
}

CBaseBaseDef::~CBaseBaseDef()
{
}

lpctstr CBaseBaseDef::GetTypeName() const
{
	return( m_sName );
}

lpctstr CBaseBaseDef::GetName() const
{
	return( GetTypeName());
}

bool CBaseBaseDef::HasTypeName() const	// some CItem may not.
{
	return( ! m_sName.IsEmpty());	// default type name.
}

void CBaseBaseDef::SetTypeName( lpctstr pszName )
{
	GETNONWHITESPACE( pszName );
	m_sName = pszName;
}

bool CBaseBaseDef::Can( word wCan ) const
{
	return(( m_Can & wCan ) ? true : false );
}

void CBaseBaseDef::UnLink()
{
	m_BaseResources.RemoveAll();
	m_TEvents.RemoveAll();
	CResourceLink::UnLink();
}

bool CBaseBaseDef::r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc )
{
	UNREFERENCED_PARAMETER(pSrc);
	ADDTOCALLSTACK("CBaseBaseDef::r_WriteVal");
	EXC_TRY("WriteVal");
	bool	fZero	= false;
	int index = FindTableHeadSorted( pszKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );

	switch ( index )
	{
		//return as string or hex number or NULL if not set
		case OBC_CATEGORY:
		case OBC_DESCRIPTION:
		case OBC_SUBSECTION:
		case OBC_HITSPELL:
		case OBC_SLAYER:
		case OBC_SLAYERLESSER:
		case OBC_SLAYERMISC:
		case OBC_SLAYERSUPER:
		case OBC_ABILITYPRIMARY:
		case OBC_ABILITYSECONDARY:
		case OBC_MANABURST:
			sVal = GetDefStr(pszKey, false);
			break;
		//return as decimal number or 0 if not set
		case OBC_CASTINGFOCUS:
		case OBC_DAMCHAOS:
		case OBC_DAMCOLD:
		case OBC_DAMDIRECT:
		case OBC_DAMENERGY:
		case OBC_DAMFIRE:
		case OBC_DAMMODIFIER:
		case OBC_DAMPHYSICAL:
		case OBC_DECREASEHITCHANCE:
		case OBC_DAMPOISON:
		case OBC_EATERCOLD:
		case OBC_EATERDAM:
		case OBC_EATERENERGY:
		case OBC_EATERFIRE:
		case OBC_EATERKINETIC:
		case OBC_EATERPOISON:
		case OBC_ENHANCEPOTIONS:
		case OBC_EXPANSION:
		case OBC_FASTERCASTING:
		case OBC_FASTERCASTRECOVERY:
		case OBC_HITAREACOLD:
		case OBC_HITAREAENERGY:
		case OBC_HITAREAFIRE:
		case OBC_HITAREAPHYSICAL:
		case OBC_HITAREAPOISON:
		case OBC_HITCURSE:
		case OBC_HITDISPEL:
		case OBC_HITFATIGUE:
		case OBC_HITFIREBALL:
		case OBC_HITHARM:
		case OBC_HITLEECHLIFE:
		case OBC_HITLEECHMANA:
		case OBC_HITLEECHSTAM:
		case OBC_HITLIGHTNING:
		case OBC_HITLOWERATK:
		case OBC_HITLOWERDEF:
		case OBC_HITMAGICARROW:
		case OBC_HITMANADRAIN:
		case OBC_INCREASEDAM:
		case OBC_INCREASEDEFCHANCE:
		case OBC_INCREASEDEFCHANCEMAX:
		case OBC_INCREASEHITCHANCE:
		case OBC_INCREASEGOLD:
		case OBC_INCREASEKARMALOSS:
		case OBC_INCREASESPELLDAM:
		case OBC_INCREASESWINGSPEED:
		case OBC_LOWERAMMOCOST:
		case OBC_LOWERMANACOST:
		case OBC_LOWERREAGENTCOST:
		case OBC_LOWERREQ:
		case OBC_LUCK:
		case OBC_MANABURSTFREQUENCY:
		case OBC_MANABURSTKARMA:
		case OBC_NIGHTSIGHT:
		case OBC_RAGEFOCUS:
		case OBC_REACTIVEPARALYZE:
		case OBC_REFLECTPHYSICALDAM:
		case OBC_REGENFOOD:
		case OBC_REGENHITS:
		case OBC_REGENMANA:
		case OBC_REGENSTAM:
		case OBC_REGENVALFOOD:
		case OBC_REGENVALHITS:
		case OBC_REGENVALMANA:
		case OBC_REGENVALSTAM:
		case OBC_RESCOLD:
		case OBC_RESFIRE:
		case OBC_RESENERGY:
		case OBC_RESPHYSICAL:
		case OBC_RESPOISON:
		case OBC_RESCOLDMAX:
		case OBC_RESFIREMAX:
		case OBC_RESENERGYMAX:
		case OBC_RESPHYSICALMAX:
		case OBC_RESPOISONMAX:
		case OBC_RESONANCECOLD:
		case OBC_RESONANCEENERGY:
		case OBC_RESONANCEFIRE:
		case OBC_RESONANCEKINETIC:
		case OBC_RESONANCEPOISON:
		case OBC_SOULCHARGE:
		case OBC_SOULCHARGECOLD:
		case OBC_SOULCHARGEENERGY:
		case OBC_SOULCHARGEFIRE:
		case OBC_SOULCHARGEKINETIC:
		case OBC_SOULCHARGEPOISON:
		case OBC_SPELLCONSUMPTION:
		case OBC_SPELLFOCUSING:
		case OBC_VELOCITY:
		case OBC_SPELLCHANNELING:
		case OBC_NAMELOC:
		case OBC_HITSPELLSTR:
		case OBC_WEIGHTREDUCTION:
		case OBC_COMBATBONUSSTAT:
		case OBC_COMBATBONUSPERCENT:
			sVal.FormatLLVal(GetDefNum(pszKey, true));
			break;

		case OBC_DEFNAME:
			sVal = GetResourceName();
			break;

		case OBC_ARMOR:
			{
				pszKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *pszKey == '.' )
				{
					SKIP_SEPARATORS( pszKey );

					if ( !strnicmp( pszKey, "LO", 2 ) )
					{
						sVal.Format( "%d", m_defenseBase );
					}
					else if ( !strnicmp( pszKey, "HI", 2 ) )
					{
						sVal.Format( "%d", m_defenseBase+m_defenseRange );
					}
				}
				else
				{
					sVal.Format( "%d,%d", m_defenseBase, m_defenseBase+m_defenseRange );
				}
			} break;
		case OBC_DAM:
			{
				pszKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *pszKey == '.' )
				{
					SKIP_SEPARATORS( pszKey );

					if ( !strnicmp( pszKey, "LO", 2 ) )
					{
						sVal.Format( "%d", m_attackBase );
					}
					else if ( !strnicmp( pszKey, "HI", 2 ) )
					{
						sVal.Format( "%d", m_attackBase+m_attackRange );
					}
				}
				else
				{
					sVal.Format( "%d,%d", m_attackBase, m_attackBase+m_attackRange );
				}
			} break;
		case OBC_BASEID:
			sVal = g_Cfg.ResourceGetName( GetResourceID());
			break;
        case OBC_CAN:
            sVal.FormatHex(m_Can);
            break;
        case OBC_CANUSE:
            sVal.FormatHex(m_CanUse);
            break;
		case OBC_HEIGHT:
			{
				//CBaseBaseDef * pBaseBaseDef = dynamic_cast<CBaseBaseDef*>(this);
				//DEBUG_ERR(("OBC_HEIGHT  -  m_dwDispIndex %d  GetHeight() %d  pBaseBaseDef->GetHeight() %d  pBaseBaseDef 0x%x\n",m_wDispIndex,GetHeight(),pBaseBaseDef->GetHeight(),pBaseBaseDef));
				sVal.FormatVal( GetHeight() );
			}
			break;
		case OBC_INSTANCES:
			sVal.FormatVal( GetRefInstances());
			break;
		case OBC_NAME:
			sVal = GetName();
			break;

		case OBC_RANGE:
			if ( RangeH() == 0 ) sVal.Format( "%d", RangeL() );
			else sVal.Format( "%d,%d", RangeH(), RangeL() );
			break;
		case OBC_RANGEL: // internally: rangel seems to be Range Highest value
			sVal.FormatVal( RangeH() );
			break;
		case OBC_RANGEH: // but rangeh seems to be the Range Lowest value.
			sVal.FormatVal( RangeL() );
			break;

		case OBC_RESOURCES:		// Print the resources
			{
				pszKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *pszKey == '.' )
				{
					SKIP_SEPARATORS( pszKey );

					if ( !strnicmp( pszKey, "COUNT", 5 ))
					{
						sVal.FormatSTVal(m_BaseResources.GetCount());
					}
					else
					{
						bool fQtyOnly = false;
						bool fKeyOnly = false;
						index = Exp_GetVal( pszKey );
						SKIP_SEPARATORS( pszKey );

						if ( !strnicmp( pszKey, "KEY", 3 ))
							fKeyOnly	= true;
						else if ( !strnicmp( pszKey, "VAL", 3 ))
							fQtyOnly	= true;

						tchar *pszTmp = Str_GetTemp();
						m_BaseResources.WriteKeys( pszTmp, index, fQtyOnly, fKeyOnly );
						if ( fQtyOnly && pszTmp[0] == '\0' )
							strcpy( pszTmp, "0" );

						sVal = pszTmp;
					}
				}
				else
				{
					tchar *pszTmp = Str_GetTemp();
					m_BaseResources.WriteKeys( pszTmp );
					sVal = pszTmp;
				}
			}
			break;
		case OBC_RESLEVEL:
			sVal.FormatVal( GetResLevel() );
			break;
		case OBC_RESDISPDNHUE:
			sVal.FormatHex( GetResDispDnHue() );
			break;
		case OBC_TAG0:
			fZero	= true;
			pszKey++;
		case OBC_TAG:			// "TAG" = get/set a local tag.
			if ( pszKey[3] != '.' )
				return false;
			pszKey += 4;
			sVal = m_TagDefs.GetKeyStr(pszKey, fZero );
			break;
		case OBC_TEVENTS:
			m_TEvents.WriteResourceRefList( sVal );
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CBaseBaseDef::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CBaseBaseDef::r_LoadVal");
	EXC_TRY("LoadVal");
	if ( s.IsKeyHead( "TAG.", 4 ))
	{
		bool fQuoted = false;
		m_TagDefs.SetStr( s.GetKey()+4, fQuoted, s.GetArgStr( &fQuoted ), false );
		return true;
	}
	if ( s.IsKeyHead( "TAG0.", 5 ))
	{
		bool fQuoted = false;
		m_TagDefs.SetStr( s.GetKey()+5, fQuoted, s.GetArgStr( &fQuoted ), true );
		return true;
	}

	switch ( FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		//Set as Strings
		case OBC_HITSPELL:
		case OBC_SLAYER:
		case OBC_SLAYERLESSER:
		case OBC_SLAYERMISC:
		case OBC_SLAYERSUPER:
		case OBC_ABILITYPRIMARY:
		case OBC_ABILITYSECONDARY:
		case OBC_MANABURST:
			{
				bool fQuoted = false;
				SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
			}
			break;
		//Set as number only
		case OBC_CASTINGFOCUS:
		case OBC_DAMCHAOS:
		case OBC_DAMCOLD:
		case OBC_DAMDIRECT:
		case OBC_DAMENERGY:
		case OBC_DAMFIRE:
		case OBC_DAMMODIFIER:
		case OBC_DAMPHYSICAL:
		case OBC_DECREASEHITCHANCE:
		case OBC_DAMPOISON:
		case OBC_EATERCOLD:
		case OBC_EATERDAM:
		case OBC_EATERENERGY:
		case OBC_EATERFIRE:
		case OBC_EATERKINETIC:
		case OBC_EATERPOISON:
		case OBC_ENHANCEPOTIONS:
		case OBC_EXPANSION:
		case OBC_FASTERCASTING:
		case OBC_FASTERCASTRECOVERY:
		case OBC_HITAREACOLD:
		case OBC_HITAREAENERGY:
		case OBC_HITAREAFIRE:
		case OBC_HITAREAPHYSICAL:
		case OBC_HITAREAPOISON:
		case OBC_HITCURSE:
		case OBC_HITDISPEL:
		case OBC_HITFATIGUE:
		case OBC_HITFIREBALL:
		case OBC_HITHARM:
		case OBC_HITLEECHLIFE:
		case OBC_HITLEECHMANA:
		case OBC_HITLEECHSTAM:
		case OBC_HITLIGHTNING:
		case OBC_HITLOWERATK:
		case OBC_HITLOWERDEF:
		case OBC_HITMAGICARROW:
		case OBC_HITMANADRAIN:
		case OBC_INCREASEDAM:
		case OBC_INCREASEDEFCHANCE:
		case OBC_INCREASEDEFCHANCEMAX:
		case OBC_INCREASEHITCHANCE:
		case OBC_INCREASEGOLD:
		case OBC_INCREASEKARMALOSS:
		case OBC_INCREASESPELLDAM:
		case OBC_INCREASESWINGSPEED:
		case OBC_LOWERAMMOCOST:
		case OBC_LOWERMANACOST:
		case OBC_LOWERREAGENTCOST:
		case OBC_LOWERREQ:
		case OBC_LUCK:
		case OBC_MANABURSTFREQUENCY:
		case OBC_MANABURSTKARMA:
		case OBC_NIGHTSIGHT:
		case OBC_RAGEFOCUS:
		case OBC_REACTIVEPARALYZE:
		case OBC_REFLECTPHYSICALDAM:
		case OBC_REGENFOOD:
		case OBC_REGENHITS:
		case OBC_REGENMANA:
		case OBC_REGENSTAM:
		case OBC_REGENVALFOOD:
		case OBC_REGENVALHITS:
		case OBC_REGENVALMANA:
		case OBC_REGENVALSTAM:
		case OBC_RESCOLD:
		case OBC_RESFIRE:
		case OBC_RESENERGY:
		case OBC_RESPHYSICAL:
		case OBC_RESPOISON:
		case OBC_RESCOLDMAX:
		case OBC_RESFIREMAX:
		case OBC_RESENERGYMAX:
		case OBC_RESPHYSICALMAX:
		case OBC_RESPOISONMAX:
		case OBC_RESONANCECOLD:
		case OBC_RESONANCEENERGY:
		case OBC_RESONANCEFIRE:
		case OBC_RESONANCEKINETIC:
		case OBC_RESONANCEPOISON:
		case OBC_SOULCHARGE:
		case OBC_SOULCHARGECOLD:
		case OBC_SOULCHARGEENERGY:
		case OBC_SOULCHARGEFIRE:
		case OBC_SOULCHARGEKINETIC:
		case OBC_SOULCHARGEPOISON:
		case OBC_SPELLCONSUMPTION:
		case OBC_SPELLFOCUSING:
		case OBC_VELOCITY:
		case OBC_SPELLCHANNELING:
		case OBC_NAMELOC:
		case OBC_HITSPELLSTR:
		case OBC_WEIGHTREDUCTION:
		case OBC_COMBATBONUSSTAT:
		case OBC_COMBATBONUSPERCENT:
			{
				SetDefNum(s.GetKey(),s.GetArgVal());
				return true;
			}
		case OBC_CATEGORY://*
		case OBC_SUBSECTION://*
		case OBC_DESCRIPTION://*
			{
				bool fQuoted = false;
				SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
				if ( !strcmpi(GetDefStr(sm_szLoadKeys[OBC_DESCRIPTION]), "@") )
					SetDefStr(sm_szLoadKeys[OBC_DESCRIPTION], GetDefStr(sm_szLoadKeys[OBC_SUBSECTION]));
			}
			return true;
		case OBC_ARMOR:
			{
				int64 piVal[2];
				size_t iQty = Str_ParseCmds( s.GetArgStr(), piVal, CountOf(piVal));
				m_defenseBase = (uchar)(piVal[0]);
				if ( iQty > 1 )
					m_defenseRange = (uchar)(piVal[1]) - m_defenseBase;
				else
					m_defenseRange = 0;
			}
			return true;
		case OBC_DAM:
			{
				int64 piVal[2];
				size_t iQty = Str_ParseCmds( s.GetArgStr(), piVal, CountOf(piVal));
				m_attackBase = (uchar)(piVal[0]);
				if ( iQty > 1 )
					m_attackRange = (uchar)(piVal[1]) - m_attackBase;
				else
					m_attackRange = 0;
			}
			return true;
		case OBC_BASEID:
			return false;
        case OBC_CAN:
            m_Can = s.GetArgVal();// | ( m_Can & ( CAN_C_INDOORS|CAN_C_EQUIP|CAN_C_USEHANDS|CAN_C_NONHUMANOID )); //Fixed #2326 ?
            return true;
        case OBC_CANUSE:
            m_CanUse = s.GetArgVal();
            return true;

		case OBC_DEFNAME:
		case OBC_DEFNAME2:
			return SetResourceName( s.GetArgStr());
		case OBC_HEIGHT:
			m_Height = static_cast<height_t>(s.GetArgVal());
			return true;
		case OBC_INSTANCES:
			return false;
		case OBC_NAME:
			SetTypeName( s.GetArgStr());
			return true;
		case OBC_RANGE:
			{
				int64 piVal[2];
				size_t iQty = Str_ParseCmds( s.GetArgStr(), piVal, CountOf(piVal));
				if ( iQty > 1 )
				{
					int64 iRange = ((piVal[0] & 0xff) << 8) & 0xff00;
					iRange |= (piVal[1] & 0xff);
					SetDefNum(s.GetKey(),iRange, false);
					//m_range	 = ((piVal[0] & 0xff) << 8) & 0xff00;
					//m_range	|= (piVal[1] & 0xff);
				}
				else
				{
					SetDefNum(s.GetKey(),piVal[0], false);
					//m_range	= (word)(piVal[0]);
				}
			}
			return true;
		case OBC_RESOURCES:
			m_BaseResources.Load( s.GetArgStr());
			return true;
		case OBC_RESLEVEL:
			return( SetResLevel((uchar)(s.GetArgVal())) );
		case OBC_RESDISPDNHUE:
			SetResDispDnHue(static_cast<HUE_TYPE>(s.GetArgVal()));
			return true;
		case OBC_TEVENTS:
			return( m_TEvents.r_LoadVal( s, RES_EVENTS ));
	}
	return( CScriptObj::r_LoadVal(s));
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

// do not copy CResourceLink
void CBaseBaseDef::CopyBasic( const CBaseBaseDef * pBase )
{
	ADDTOCALLSTACK("CBaseBaseDef::CopyBasic");

	if ( m_sName.IsEmpty())	// Base type name. should set this itself most times. (don't overwrite it!)
		m_sName = pBase->m_sName;

	m_dwDispIndex = pBase->m_dwDispIndex;

	m_Height = pBase->m_Height;
	// -------------- ResLevel -------------
	m_ResLevel = pBase->m_ResLevel;
	m_ResDispDnHue = pBase->m_ResDispDnHue;
	m_ResDispDnId = pBase->m_ResDispDnId;
	// -------------------------------------
	// m_BaseResources = pBase->m_BaseResources;	// items might not want this.
	m_attackBase = pBase->m_attackBase;
	m_attackRange = pBase->m_attackRange;
	m_defenseBase = pBase->m_defenseBase;
	m_defenseRange = pBase->m_defenseRange;
	m_Can = pBase->m_Can;
}

void CBaseBaseDef::CopyTransfer( CBaseBaseDef * pBase )
{
	ADDTOCALLSTACK("CBaseBaseDef::CopyTransfer");
	CResourceLink::CopyTransfer( pBase );

	m_sName = pBase->m_sName;
	m_BaseResources = pBase->m_BaseResources;

	CopyBasic( pBase );
}


bool CBaseBaseDef::IsValid() const
{
	return( m_sName.IsValid());
}

byte CBaseBaseDef::RangeL() const
{
	return (byte)(GetDefNum("RANGE",true) & 0xff);
	//return (m_range & 0xff);
}

byte CBaseBaseDef::RangeH() const
{
	return (byte)((GetDefNum("RANGE",true)>>8) & 0xff);
	//return ((m_range>>8) & 0xff);
}

height_t CBaseBaseDef::GetHeight() const
{
	return( m_Height );
}

void CBaseBaseDef::SetHeight( height_t Height )
{
	m_Height = Height;
}


byte CBaseBaseDef::GetResLevel() const
{
	return( m_ResLevel );
}

bool CBaseBaseDef::SetResLevel( byte ResLevel )
{
	if ( ResLevel >= RDS_T2A && ResLevel < RDS_QTY )
	{
		m_ResLevel = ResLevel;
		return true;
	}
	return false;
}

HUE_TYPE CBaseBaseDef::GetResDispDnHue() const
{
	return( m_ResDispDnHue );
}

void CBaseBaseDef::SetResDispDnHue( HUE_TYPE ResDispDnHue )
{
	m_ResDispDnHue = ResDispDnHue;
}

word CBaseBaseDef::GetResDispDnId() const
{
	return( m_ResDispDnId );
}

void CBaseBaseDef::SetResDispDnId( word ResDispDnId )
{
	m_ResDispDnId = ResDispDnId;
}
