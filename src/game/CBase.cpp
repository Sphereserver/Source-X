
#include "../common/CException.h"
#include "../common/common.h"
#include "components/CCPropsItemChar.h"
#include "components/CCPropsItemWeapon.h"
#include "CServerConfig.h"
#include "CBase.h"
#include <algorithm>

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
	nullptr
};


CBaseBaseDef::CBaseBaseDef( CResourceID id ) :
	CResourceLink( id ),
	m_sName(false)
{
	_dwInstances = 0;
	m_dwDispIndex			= 0;	// Assume nothing til told differently.
	m_attackBase			= 0;
	m_attackRange			= 0;
	m_defenseBase			= 0;
	m_defenseRange			= 0;
	m_Height				= 0;
	m_Can					= 0;
    m_Expansion             = RDS_T2A;
	m_ResLevel				= RDS_T2A;
    _iEraLimitProps         = g_Cfg._iEraLimitProps;
	m_ResDispDnHue			= HUE_DEFAULT;
	m_ResDispDnId			= 0;
	m_BaseResources.setNoMergeOnLoad();
}

CBaseBaseDef::~CBaseBaseDef()
{
}

void CBaseBaseDef::DelInstance()
{
#ifdef _DEBUG
	ASSERT(_dwInstances != (dword)-1);    // catching underflows
#endif
	--_dwInstances;
}

CFactionDef CBaseBaseDef::GetFaction()
{
	return _pFaction;
}

lpctstr CBaseBaseDef::GetTypeName() const
{
	return m_sName.GetBuffer();
}

lpctstr CBaseBaseDef::GetName() const
{
	return GetTypeName();
}

bool CBaseBaseDef::HasTypeName() const	// some CItem may not.
{
	return (! m_sName.IsEmpty());	// default type name.
}

void CBaseBaseDef::SetTypeName( lpctstr pszName )
{
	GETNONWHITESPACE( pszName );
	m_sName = pszName;
}

bool CBaseBaseDef::Can( dword dwCan ) const
{
	return (m_Can & dwCan);
}

void CBaseBaseDef::UnLink()
{
    m_BaseResources.clear();
    m_TEvents.clear();
	CResourceLink::UnLink();
}

/*
bool CBaseBaseDef::r_Verb(CScript &s, CTextConsole * pSrc) // Execute command from script
{
    ADDTOCALLSTACK("CBaseBaseDef::r_Verb");
    EXC_TRY("Verb");
    lpctstr	ptcKey = s.GetKey();
    ASSERT(pSrc);

    //

    return false;
    EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}
*/

bool CBaseBaseDef::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
	UNREFERENCED_PARAMETER(pSrc);
    UNREFERENCED_PARAMETER(fNoCallParent);
    UNREFERENCED_PARAMETER(fNoCallChildren);
	ADDTOCALLSTACK("CBaseBaseDef::r_WriteVal");
	EXC_TRY("WriteVal");

	bool fZero = false;
	int index = FindTableHeadSorted( ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );
	switch ( index )
	{
		//return as string or hex number or nullptr if not set
		case OBC_CATEGORY:
		case OBC_DESCRIPTION:
		case OBC_SUBSECTION:
		case OBC_ABILITYPRIMARY:
		case OBC_ABILITYSECONDARY:
			sVal = GetDefStr(ptcKey, false);
			break;
		//return as decimal number or 0 if not set
		case OBC_EXPANSION:
		case OBC_VELOCITY:
		case OBC_NAMELOC:
			sVal.FormatLLVal(GetDefNum(ptcKey));
			break;

		case OBC_DEFNAME:
			sVal = GetResourceName();
			break;

        case OBC_FACTION:
        case OBC_SLAYER:
            sVal.FormatHex((dword)GetFaction().GetFactionID());
            break;

		case OBC_ARMOR:
			{
				ptcKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *ptcKey == '.' )
				{
					SKIP_SEPARATORS( ptcKey );

					if ( !strnicmp( ptcKey, "LO", 2 ) )
					{
						sVal.Format( "%d", m_defenseBase );
					}
					else if ( !strnicmp( ptcKey, "HI", 2 ) )
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
				ptcKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *ptcKey == '.' )
				{
					SKIP_SEPARATORS( ptcKey );

					if ( !strnicmp( ptcKey, "LO", 2 ) )
					{
						sVal.Format( "%d", m_attackBase );
					}
					else if ( !strnicmp( ptcKey, "HI", 2 ) )
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

        case OBC_HASCOMPONENTPROPS:
        {
            ptcKey += 17;
            SKIP_SEPARATORS(ptcKey);
            GETNONWHITESPACE(ptcKey);

            COMPPROPS_TYPE id = (COMPPROPS_TYPE)Exp_GetVal(ptcKey);
            bool fRes = (id < COMP_PROPS_QTY) && (nullptr != CEntityProps::GetComponentProps(id));
            sVal.FormatVal(int(fRes));
            break;
        }

		case OBC_HEIGHT:
			sVal.FormatVal( GetHeight() );
			break;

		case OBC_INSTANCES:
			sVal.FormatVal( GetInstances() );
			break;
		case OBC_NAME:
			sVal = GetName();
			break;

		case OBC_RESOURCES:		// Print the resources
			{
				ptcKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *ptcKey == '.' )
				{
					SKIP_SEPARATORS( ptcKey );

					if ( !strnicmp( ptcKey, "COUNT", 5 ))
					{
						sVal.FormatSTVal(m_BaseResources.size());
					}
					else
					{
						bool fQtyOnly = false;
						bool fKeyOnly = false;
						index = Exp_GetVal( ptcKey );
						SKIP_SEPARATORS( ptcKey );

						if ( !strnicmp( ptcKey, "KEY", 3 ))
							fKeyOnly	= true;
						else if ( !strnicmp( ptcKey, "VAL", 3 ))
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
			fZero = true;
			++ptcKey;
			FALLTHROUGH;
		case OBC_TAG:			// "TAG" = get/set a local tag.
			if ( ptcKey[3] != '.' )
				return false;
			ptcKey += 4;
			sVal = m_TagDefs.GetKeyStr(ptcKey, fZero );
			break;
		case OBC_TEVENTS:
			m_TEvents.WriteResourceRefList( sVal );
			break;
        case OBC_ISTEVENT:
            if ( ptcKey[8] != '.' )
                return false;
            ptcKey += 9;
            sVal = m_TEvents.ContainsResourceName(RES_EVENTS, ptcKey) ? "1" : "0";
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
    lpctstr ptcKey = s.GetKey();
    if (!strnicmp("TAG", ptcKey, 3))
    {
        if ((ptcKey[3] == '.') || (ptcKey[3] == '0'))
        {
            const bool fZero = (ptcKey[3] == '0');
            ptcKey = ptcKey + (fZero ? 5 : 4);
            bool fQuoted = false;
            lpctstr ptcArg = s.GetArgStr(&fQuoted);
            m_TagDefs.SetStr(ptcKey, fQuoted, ptcArg, false); // don't change fZero to true! it would break some scripts!
            return true;
        }
    }

    int i = FindTableSorted(ptcKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );
	switch (i)
	{
		//Set as Strings
		case OBC_ABILITYPRIMARY:
		case OBC_ABILITYSECONDARY:
			{
				bool fQuoted = false;
				SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
			}
			break;
        case OBC_FACTION:
        case OBC_SLAYER:
            GetFaction().SetFactionID( (NPC_FACTION)s.GetArgVal() );
            return true;
		//Set as number only
		case OBC_EXPANSION:
		case OBC_VELOCITY:
		case OBC_NAMELOC:
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
					SetDefStr(sm_szLoadKeys[OBC_DESCRIPTION], GetDefStr(sm_szLoadKeys[OBC_SUBSECTION]), false, true, false);
			}
			return true;
		case OBC_ARMOR:
			{
				int64 piVal[2];
                int iQty = Str_ParseCmds( s.GetArgStr(), piVal, CountOf(piVal));
				m_defenseBase = (word)(piVal[0]);
				if ( iQty > 1 )
					m_defenseRange = (word)(piVal[1]) - m_defenseBase;
				else
					m_defenseRange = 0;
			}
			return true;
		case OBC_DAM:
			{
				int64 piVal[2];
				int iQty = Str_ParseCmds( s.GetArgStr(), piVal, CountOf(piVal));
				m_attackBase = (word)(piVal[0]);
				if ( iQty > 1 )
					m_attackRange = (word)(piVal[1]) - m_attackBase;
				else
					m_attackRange = 0;
			}
			return true;
		case OBC_BASEID:
			return false;
        case OBC_CAN:
            m_Can = s.GetArgDWVal(); // | ( m_Can & ( CAN_C_INDOORS|CAN_C_EQUIP|CAN_C_USEHANDS|CAN_C_NONHUMANOID )); //Fixed #2326 ?
            return true;

		case OBC_DEFNAME:
		case OBC_DEFNAME2:
			return SetResourceName(s.GetArgStr());
		case OBC_HEIGHT:
			m_Height = (height_t)s.GetArgVal();
			return true;
		case OBC_INSTANCES:
			return false;
		case OBC_NAME:
			SetTypeName( s.GetArgStr());
			return true;
		case OBC_RESOURCES:
			m_BaseResources.Load(s.GetArgStr());
			return true;
		case OBC_RESLEVEL:
			return( SetResLevel((uchar)(s.GetArgVal())) );
		case OBC_RESDISPDNHUE:
			SetResDispDnHue((HUE_TYPE)(s.GetArgVal()));
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
    m_Expansion = pBase->m_Expansion;
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
    _pFaction.SetFactionID(pBase->_pFaction.GetFactionID());
    CEntityProps::Copy(pBase);
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

ushort CBaseBaseDef::ConvertRangeStr(lpctstr ptcRange) // static
{
	int64 piVal[2];
	tchar* ptcTmp = Str_GetTemp();
	Str_CopyLimitNull(ptcTmp, ptcRange, STR_TEMPLENGTH);
	const int iQty = Str_ParseCmds(ptcTmp, piVal, CountOf(piVal));
	ushort iHi = 0, iLo = 0;
	if (iQty > 1)	// args: "min, max"
	{
		iHi = (ushort)piVal[1];
		iLo = (ushort)piVal[0];
		if (iLo > iHi)
		{
			std::swap(iHi, iLo);
		}
	}
	else			// arg: "max"
	{
		iHi = (ushort)piVal[0];
	}
	return RANGE_MAKE(iHi, iLo);
}
