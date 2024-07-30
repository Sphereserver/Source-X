// Common for client and server.

#include "../common/resource/sections/CRandGroupDef.h"
#include "../common/resource/CResourceLock.h"
#include "../common/CScript.h"
#include "../network/CClientIterator.h"
#include "../sphere/threads.h"
#include "chars/CChar.h"
#include "clients/CClient.h"
#include "CSector.h"
#include "CSectorList.h"
#include "CServer.h"



//*************************************************************************
// -CRegion

CRegion::CRegion( CResourceID rid, lpctstr pszName ) :
	CResourceDef( rid )
{
	ADDTOCALLSTACK("CRegion::CRegion()");
	m_dwFlags	= 0;
	m_dwModifiedFlags	= 0;
	m_iLinkedSectors = 0;
	if ( pszName )
		SetName( pszName );
    _pMultiLink = nullptr;
}

CRegion::~CRegion()
{
	ADDTOCALLSTACK("CRegion::~CRegion");
	// RemoveSelf();
	UnRealizeRegion();
}

void CRegion::SetModified( dword dwModFlag ) noexcept
{
	if ( !m_iLinkedSectors )
        return;

	m_dwModifiedFlags = m_dwModifiedFlags | dwModFlag;
}

void CRegion::UnRealizeRegion()
{
	ADDTOCALLSTACK("CRegion::UnRealizeRegion");
	// remove myself from the world.
	// used in the case of a ship where the region will move.

	for ( int i = 0; ; ++i )
	{
		CSector * pSector = GetSector(i);
		if ( pSector == nullptr )
			break;
		// Does the rect overlap ?
		if ( ! IsOverlapped( pSector->GetRect()))
			continue;
		if ( pSector->UnLinkRegion( this ))
			--m_iLinkedSectors;
	}

}

bool CRegion::RealizeRegion()
{
	ADDTOCALLSTACK("CRegion::RealizeRegion");
	// Link the region to the world. RETURN: false = not valid.
	if ( IsRegionEmpty() )
		return false;
	if ( !m_pt.IsValidPoint() )
		m_pt = GetRegionCorner( DIR_QTY );	// center

	// Attach to all sectors that i overlap.
	ASSERT( m_iLinkedSectors == 0 );
	const CSectorList* pSectors = CSectorList::Get();
	for ( int i = 0, iMax = pSectors->GetSectorQty(m_pt.m_map); i < iMax; ++i )
	{
		CSector *pSector = pSectors->GetSector(m_pt.m_map, i);

		if ( pSector && IsOverlapped(pSector->GetRect()) )
		{
			//	Yes, this sector overlapped, so add it to the sector list
			if ( !pSector->LinkRegion(this) )
			{
				g_Log.EventError("Linking sector #%d (map %d) for region %s failed (fatal for this region).\n", i, int(m_pt.m_map), GetName());
				return false;
			}
			++m_iLinkedSectors;
		}
	}
	return true;
}

bool CRegion::AddRegionRect( const CRectMap & rect )
{
	ADDTOCALLSTACK("CRegion::AddRegionRect");
	// Add an included rectangle to this region.
	if ( ! rect.IsValid())
	{
		return false;
	}
	if ( ! CRegionBase::AddRegionRect( rect ))
		return false;

	// Need to call RealizeRegion now.?
	return true;
}

void CRegion::SetName( lpctstr pszName )
{
	ADDTOCALLSTACK("CRegion::SetName");
	if ( pszName == nullptr || pszName[0] == '%' )
	{
		m_sName = g_Serv.GetName();
	}
	else
	{
		m_sName = pszName;
	}
}

bool CRegion::MakeRegionDefname()
{
    ADDTOCALLSTACK("CRegion::MakeRegionDefname");
    if ( m_pDefName )
        return true;

    tchar ch;
    lpctstr ptcKey = nullptr;	// auxiliary, the key of a similar CVarDef, if any found
    tchar * pbuf = Str_GetTemp();
    tchar * pszDef = pbuf + 2;
    strcpy(pbuf, "a_");

    lpctstr pszName = GetName();
    GETNONWHITESPACE( pszName );
    g_Log.EventWarn("Missing DEFNAME for Region named '%s'. Auto-generating one.\n", pszName);

    if ( !strnicmp( "the ", pszName, 4 ) )
        pszName	+= 4;
    else if ( !strnicmp( "a ", pszName, 2 ) )
        pszName	+= 2;
    else if ( !strnicmp( "an ", pszName, 3 ) )
        pszName	+= 3;
    else if ( !strnicmp( "ye ", pszName, 3 ) )
        pszName	+= 3;

    for ( ; *pszName; ++pszName )
    {
        if ( !strnicmp( " of ", pszName, 4 ) || !strnicmp( " in ", pszName, 4 ) )
        {	pszName	+= 4;	continue;	}
        if ( !strnicmp( " the ", pszName, 5 )  )
        {	pszName	+= 5;	continue;	}

        ch	= *pszName;
        if ( ch == ' ' || ch == '\t' || ch == '-' )
            ch	= '_';
        else if ( !IsAlnum( ch ) )
            continue;
        // collapse multiple spaces together
        if ( ch == '_' && *(pszDef-1) == '_' )
            continue;
        *pszDef = static_cast<tchar>(tolower(ch));
        ++pszDef;
    }
    *pszDef	= '_';
    *(++pszDef)	= '\0';


    size_t iMax = g_Cfg.m_RegionDefs.size();
    int iVar = 1;
    size_t iLen = strlen( pbuf );

    for ( size_t i = 0; i < iMax; ++i )
    {
        CRegion * pRegion = dynamic_cast <CRegion*> (g_Cfg.m_RegionDefs[i]);
        if ( !pRegion )
            continue;
        ptcKey = pRegion->GetResourceName();
        if ( !ptcKey )
            continue;

        // Is this a similar key?
        if ( strnicmp( pbuf, ptcKey, iLen ) != 0 )
            continue;

        // skip underscores
        ptcKey = ptcKey + iLen;
        while ( *ptcKey	== '_' )
            ++ptcKey;

        // Is this is subsequent key with a number? Get the highest (plus one)
        if ( IsStrNumericDec( ptcKey ) )
        {
            int iVarThis = Str_ToI( ptcKey );
            if ( iVarThis >= iVar )
                iVar = iVarThis + 1;
        }
        else
            ++iVar;
    }

    // Only one, no need for the extra "_"
    pszDef = Str_FromI_Fast(iVar, pszDef, Str_TempLength(), 10);
    SetResourceName( pbuf );
    // Assign name
    return true;
}

enum RC_TYPE : int // Even if it would implicitly be set to int, specify it to silence UBSanitizer.
{
	RC_ANNOUNCE,
	RC_ARENA,
	RC_BUILDABLE,
	RC_CLIENTS,
	RC_COLDCHANCE,
    RC_DEFNAME,
	RC_EVENTS,
	RC_FLAGS,
	RC_GATE,
	RC_GROUP,
	RC_GUARDED,
	RC_ISEVENT,
	RC_MAGIC,
	RC_MAP,
	RC_MARK,		// recall in teleport as well.
	RC_NAME,
	RC_NOBUILD,
	RC_NODECAY,
	RC_NOPVP,
	RC_P,
	RC_RAINCHANCE,
	RC_RECALL,	// recall out
	RC_RECALLIN,
	RC_RECALLOUT,
	RC_RECT,
	RC_SAFE,
	RC_TAG,
	RC_TAG0,
	RC_TAGAT,
	RC_TAGCOUNT,
	RC_TYPEREGION,
	RC_UID,
	RC_UNDERGROUND,
	RC_QTY
};

lpctstr const CRegion::sm_szLoadKeys[RC_QTY+1] =	// static (Sorted)
{
	"ANNOUNCE",
	"ARENA",
	"BUILDABLE",
	"CLIENTS",
	"COLDCHANCE",
    "DEFNAME",
	"EVENTS",
	"FLAGS",
	"GATE",
	"GROUP",
	"GUARDED",
	"ISEVENT",
	"MAGIC",
	"MAP",
	"MARK",		// recall in teleport as well.
	"NAME",
	"NOBUILD",
	"NODECAY",
	"NOPVP",
	"P",
	"RAINCHANCE",
	"RECALL",	// recall out
	"RECALLIN",
	"RECALLOUT",
	"RECT",
	"SAFE",
	"TAG",
	"TAG0",
	"TAGAT",
	"TAGCOUNT",
	"TYPE",
	"UID",
	"UNDERGROUND",
	nullptr
};

bool CRegion::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallChildren);
	ADDTOCALLSTACK("CRegion::r_WriteVal");
	EXC_TRY("WriteVal");
	bool fZero = false;
	RC_TYPE index = (RC_TYPE) FindTableHeadSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );
	if ( index < 0 )
	{
		return (fNoCallParent ? false : CScriptObj::r_WriteVal( ptcKey, sVal, pSrc ));
	}

	switch ( index )
	{
		case RC_ANNOUNCE:
			sVal.FormatVal( IsFlag(REGION_FLAG_ANNOUNCE));
			break;
		case RC_ARENA:
			sVal.FormatVal( IsFlag(REGION_FLAG_ARENA));
			break;
		case RC_BUILDABLE:
			sVal.FormatVal( ! IsFlag(REGION_FLAG_NOBUILDING));
			break;
		case RC_CLIENTS:
        {
            int iClients = 0;
            for (int i = 0; ; ++i)
            {
                CSector	*pSector = GetSector(i);
                if (pSector == nullptr)
                    break;
                iClients += pSector->m_Chars_Active.GetClientsNumber();
            }
            sVal.FormatVal(iClients);
            break;
        }
        case RC_DEFNAME: // "DEFNAME" = for the speech system.
            sVal = GetResourceName();
            break;
		case RC_EVENTS:
			m_Events.WriteResourceRefList( sVal );
			break;
		case RC_ISEVENT:
			if ( ptcKey[7] != '.' )
				return false;
			ptcKey += 8;
			sVal = m_Events.ContainsResourceName(RES_EVENTS, ptcKey) ? "1" : "0";
            break;
		case RC_FLAGS:
			sVal.FormatHex( GetRegionFlags() );
			break;
		case RC_GATE:
			sVal.FormatVal( ! IsFlag(REGION_ANTIMAGIC_GATE));
			break;
		case RC_GROUP:
			sVal = m_sGroup;
			break;
		case RC_GUARDED:
			sVal.FormatVal( IsFlag(REGION_FLAG_GUARDED));
			break;
		case RC_MAGIC:
			sVal.FormatVal( ! IsFlag(REGION_ANTIMAGIC_ALL));
			break;
		case RC_MAP:
			sVal.FormatVal( m_pt.m_map );
			break;
		case RC_MARK:
		case RC_RECALLIN:
			sVal.FormatVal( ! IsFlag(REGION_ANTIMAGIC_RECALL_IN));
			break;
		case RC_NAME:
			// The previous name was really the DEFNAME ???
			sVal = GetName();
			break;
		case RC_NOBUILD:
			sVal.FormatVal( IsFlag(REGION_FLAG_NOBUILDING));
			break;
		case RC_NODECAY:
			sVal.FormatVal( IsFlag(REGION_FLAG_NODECAY));
			break;
		case RC_NOPVP:
			sVal.FormatVal( IsFlag(REGION_FLAG_NO_PVP));
			break;
		case RC_P:
			sVal = m_pt.WriteUsed();
			break;
		case RC_RECALL:
		case RC_RECALLOUT:
			sVal.FormatVal( ! IsFlag(REGION_ANTIMAGIC_RECALL_OUT));
			break;
		case RC_RECT:
			{
				size_t iQty = m_Rects.size();
				ptcKey += 4;
				if ( *ptcKey == '\0' )
				{
					sVal.FormatVal( (int)(iQty));
					return true;
				}
				SKIP_SEPARATORS( ptcKey );
				size_t iRect = Exp_GetVal( ptcKey );
				if ( iRect <= 0 )
				{
					sVal = m_rectUnion.Write();
					return true;
				}

				iRect -= 1;
				if ( !m_Rects.IsValidIndex( iRect ) )
				{
					sVal.FormatVal( 0 );
					return true;
				}
				sVal = m_Rects[iRect].Write();
				return true;
			}
		case RC_SAFE:
			sVal.FormatVal( IsFlag(REGION_FLAG_SAFE));
			break;
		case RC_TAGCOUNT:
			sVal.FormatSTVal( m_TagDefs.GetCount() );
			break;
		case RC_TAGAT:
			{
				ptcKey += 5; // eat the 'TAGAT'
				if ( *ptcKey == '.' ) // do we have an argument?
				{
					SKIP_SEPARATORS( ptcKey );
					size_t uiQty = Exp_GetSTVal( ptcKey );
					if ( uiQty >= m_TagDefs.GetCount() )
						return false; // trying to get non-existant tag

					const CVarDefCont * pTagAt = m_TagDefs.GetAt(uiQty);
					if ( !pTagAt )
						return false; // trying to get non-existant tag

					SKIP_SEPARATORS( ptcKey );
					if ( ! *ptcKey )
					{
						sVal.Format("%s=%s", pTagAt->GetKey(), pTagAt->GetValStr());
						return true;
					}
					else if ( !strnicmp( ptcKey, "KEY", 3 )) // key?
					{
						sVal = pTagAt->GetKey();
						return true;
					}
					else if ( !strnicmp( ptcKey, "VAL", 3 )) // val?
					{
						sVal = pTagAt->GetValStr();
						return true;
					}
				}

				return false;
			}
			break;
		case RC_TAG0:
			fZero = true;
			++ptcKey;
			FALLTHROUGH;
		case RC_TAG:	// "TAG" = get/set a local tag.
			{
				if ( ptcKey[3] != '.' )
					return false;
				ptcKey += 4;
				sVal = m_TagDefs.GetKeyStr( ptcKey, fZero );
				return true;
			}
		case RC_TYPEREGION:
			{
				const CItemBase * pBase = nullptr;
				const CItem * pItem = GetResourceID().ItemFindFromResource();
				if (pItem != nullptr)
					pBase = pItem->Item_GetDef();

				if (pBase != nullptr)
					sVal = pBase->GetResourceName();
				else
					sVal.Clear();
			} break;
		case RC_UID:
			// Allow use of UID.x.KEY on the REGION object
			if ( ptcKey[3] == '.' )
				return CScriptObj::r_WriteVal( ptcKey, sVal, pSrc );

			sVal.FormatHex( GetResourceID() );
			break;
		case RC_UNDERGROUND:
			sVal.FormatVal( IsFlag(REGION_FLAG_UNDERGROUND));
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", ptcKey, static_cast<lpctstr>(sVal), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CRegion::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CRegion::r_LoadVal");
	EXC_TRY("LoadVal");

    lpctstr ptcKey = s.GetKey();
    if (!strnicmp("TAG", ptcKey, 3))
    {
        if ((ptcKey[3] == '.') || (ptcKey[3] == '0'))
        {
            SetModified(REGMOD_TAGS);
            const bool fZero = (ptcKey[3] == '0');
            ptcKey = ptcKey + (fZero ? 5 : 4);
            bool fQuoted = false;
            lpctstr ptcArg = s.GetArgStr(&fQuoted);
            m_TagDefs.SetStr(ptcKey, fQuoted, ptcArg, false); // don't change fZero to true! it would break some scripts!
            return true;
        }
    }

	RC_TYPE index = (RC_TYPE) FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );
	if ( index < 0 )
		return false;

	switch ( index )
	{
		case RC_ANNOUNCE:
			TogRegionFlags( REGION_FLAG_ANNOUNCE, !s.HasArgs() || s.GetArgVal() );
			break;
		case RC_ARENA:
			TogRegionFlags( REGION_FLAG_ARENA, s.GetArgVal() != 0 );
			break;
		case RC_BUILDABLE:
			TogRegionFlags( REGION_FLAG_NOBUILDING, !s.GetArgVal() );
			break;
		case RC_COLDCHANCE:
			SendSectorsVerb( s.GetKey(), s.GetArgStr(), &g_Serv );
			break;
        case RC_DEFNAME: // "DEFNAME" = for the speech system.
            return SetResourceName( s.GetArgStr());
		case RC_EVENTS:
			SetModified( REGMOD_EVENTS );
			return m_Events.r_LoadVal( s, RES_REGIONTYPE );
		case RC_FLAGS:
			m_dwFlags = ( s.GetArgDWVal() & ~REGION_FLAG_SHIP ) | ( m_dwFlags & REGION_FLAG_SHIP );
			SetModified( REGMOD_FLAGS );
			break;
		case RC_GATE:
			TogRegionFlags( REGION_ANTIMAGIC_GATE, ! s.GetArgVal());
			break;
		case RC_GROUP:
			m_sGroup = s.GetArgStr();
			SetModified( REGMOD_GROUP );
			break;
		case RC_GUARDED:
			TogRegionFlags( REGION_FLAG_GUARDED, !s.HasArgs() || s.GetArgVal());
			break;
		case RC_MAGIC:
			TogRegionFlags( REGION_ANTIMAGIC_ALL, !s.GetArgVal());
			break;
		case RC_MAP:
			m_pt.m_map = s.GetArgUCVal();
			break;
		case RC_MARK:
		case RC_RECALLIN:
			TogRegionFlags( REGION_ANTIMAGIC_RECALL_IN, ! s.GetArgVal());
			break;
		case RC_NAME:
			SetName( s.GetArgStr());
			SetModified( REGMOD_NAME );
			break;
		case RC_NODECAY:
			TogRegionFlags( REGION_FLAG_NODECAY, s.GetArgVal() != 0);
			break;
		case RC_NOBUILD:
			TogRegionFlags( REGION_FLAG_NOBUILDING, s.GetArgVal() != 0);
			break;
		case RC_NOPVP:
			TogRegionFlags( REGION_FLAG_NO_PVP, s.GetArgVal() != 0);
			break;
		case RC_P:
			m_pt.Read(s.GetArgStr());
            if (!m_pt.IsValidPoint())
                return false;
			break;
		case RC_RAINCHANCE:
			SendSectorsVerb( s.GetKey(), s.GetArgStr(), &g_Serv );
			break;
		case RC_RECALL:
		case RC_RECALLOUT:
			TogRegionFlags( REGION_ANTIMAGIC_RECALL_OUT, ! s.GetArgVal());
			break;
		case RC_RECT:
			if ( m_iLinkedSectors )
				return false;
			{
				CRectMap rect;
				rect.Read(s.GetArgStr());
				return AddRegionRect( rect );
			}
		case RC_SAFE:
			TogRegionFlags( REGION_FLAG_SAFE, s.GetArgVal() != 0);
			break;
		case RC_UNDERGROUND:
			TogRegionFlags( REGION_FLAG_UNDERGROUND, s.GetArgVal() != 0);
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}


void CRegion::r_WriteBody( CScript & s, lpctstr pszPrefix )
{
	ADDTOCALLSTACK("CRegion::r_WriteBody");
	TemporaryString tsTemp;
	if ( GetRegionFlags())
	{
		snprintf(tsTemp.buffer(), tsTemp.capacity(), "%sFLAGS", pszPrefix);
		s.WriteKeyHex(tsTemp.buffer(), GetRegionFlags());
	}

	if (!m_Events.empty())
	{
		CSString sVal;
		m_Events.WriteResourceRefList( sVal );
		snprintf(tsTemp.buffer(), tsTemp.capacity(), "%sEVENTS", pszPrefix);
		s.WriteKeyStr(tsTemp.buffer(), sVal);
	}

	// Write New variables
	m_BaseDefs.r_WritePrefix(s);

	// Write out any extra TAGS here.
	snprintf(tsTemp.buffer(), tsTemp.capacity(), "%sTAG", pszPrefix);
	m_TagDefs.r_WritePrefix(s, tsTemp.buffer());
}


void CRegion::r_WriteModified( CScript &s )
{
	ADDTOCALLSTACK("CRegion::r_WriteModified");
	if ( m_dwModifiedFlags & REGMOD_NAME )
		s.WriteKeyStr("NAME", GetName() );

	if ( m_dwModifiedFlags & REGMOD_GROUP )
		s.WriteKeyStr("GROUP", m_sGroup.GetBuffer() );

	if ( m_dwModifiedFlags & REGMOD_FLAGS )
	{
		s.WriteKeyHex("FLAGS", GetRegionFlags() );
	}

	if ( m_dwModifiedFlags & REGMOD_EVENTS )
	{
		CSString sVal;
		m_Events.WriteResourceRefList( sVal );
		s.WriteKeyStr( "EVENTS", sVal.GetBuffer() );
	}
}


void CRegion::r_WriteBase( CScript &s )
{
	ADDTOCALLSTACK("CRegion::r_WriteBase");
    lpctstr ptcName = GetName();
	if ( ptcName && ptcName[0] )
		s.WriteKeyStr("NAME", ptcName);

	if ( ! m_sGroup.IsEmpty() )
		s.WriteKeyStr("GROUP", m_sGroup.GetBuffer());

	CRegion::r_WriteBody( s, "" );

	if ( m_pt.IsValidPoint())
		s.WriteKeyStr("P", m_pt.WriteUsed());
	else if ( m_pt.m_map )
		s.WriteKeyVal("MAP", m_pt.m_map);

	size_t iQty = GetRegionRectCount();
	for ( size_t i = 0; i < iQty; ++i )
	{
		s.WriteKeyStr("RECT", GetRegionRect(i).Write() );
	}
}

void CRegion::r_Write( CScript &s )
{
	ADDTOCALLSTACK_DEBUG("CRegion::r_Write");
	s.WriteSection( "ROOMDEF %s", GetResourceName() );
	r_WriteBase( s );
}


bool CRegion::IsGuarded() const
{
	ADDTOCALLSTACK("CRegion::IsGuarded");
	// Safe areas do not need guards.
	return( IsFlag( REGION_FLAG_GUARDED ) && ! IsFlag( REGION_FLAG_SAFE ));
}

bool CRegion::CheckAntiMagic( SPELL_TYPE spell ) const
{
	ADDTOCALLSTACK("CRegion::CheckAntiMagic");
	// return: true = blocked.
	if ( ! IsFlag( REGION_FLAG_SHIP |
		REGION_ANTIMAGIC_ALL |
		REGION_ANTIMAGIC_RECALL_IN |
		REGION_ANTIMAGIC_RECALL_OUT |
		REGION_ANTIMAGIC_GATE |
		REGION_ANTIMAGIC_TELEPORT |
		REGION_ANTIMAGIC_DAMAGE ))	// no effects on magic anyhow.
		return false;

	if ( IsFlag( REGION_ANTIMAGIC_ALL ))
		return true;

	if ( IsFlag( REGION_ANTIMAGIC_RECALL_IN | REGION_FLAG_SHIP ))
	{
		if ( spell == SPELL_Mark || spell == SPELL_Gate_Travel )
			return true;
	}
	if ( IsFlag( REGION_ANTIMAGIC_RECALL_OUT ))
	{
		if ( spell == SPELL_Recall || spell == SPELL_Gate_Travel || spell == SPELL_Mark )
			return true;
	}
	if ( IsFlag( REGION_ANTIMAGIC_GATE ))
	{
		if ( spell == SPELL_Gate_Travel )
			return true;
	}
	if ( IsFlag( REGION_ANTIMAGIC_TELEPORT ))
	{
		if ( spell == SPELL_Teleport )
			return true;
	}
	if ( IsFlag( REGION_ANTIMAGIC_DAMAGE ))
	{
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
		ASSERT(pSpellDef);
		if ( pSpellDef->IsSpellType( SPELLFLAG_HARM ))
			return true;
	}
	return false;
}

enum RV_TYPE
{
	RV_ALLCLIENTS,
	RV_TAGLIST,
	RV_QTY
};

lpctstr const CRegion::sm_szVerbKeys[RV_QTY+1] =
{
	"ALLCLIENTS",
	"TAGLIST",
	nullptr
};

//	actualy part of CSector, here we need SEV_QTY to know that the command is part of the sector
enum SEV_TYPE
{
	#define ADD(a,b) SEV_##a,
	#include "../tables/CSector_functions.tbl"
	#undef ADD
	SEV_QTY
};

bool CRegion::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CRegion::r_Verb");
	EXC_TRY("Verb");
	lpctstr ptcKey = s.GetKey();

	if ( !strnicmp(ptcKey, "CLEARTAGS", 9) )
	{
		ptcKey = s.GetArgStr();
		SKIP_SEPARATORS(ptcKey);
		m_TagDefs.ClearKeys(ptcKey);
		return true;
	}

	int index = FindTableSorted( ptcKey, sm_szVerbKeys, ARRAY_COUNT( sm_szVerbKeys )-1 );
	switch (index)
	{
		case RV_ALLCLIENTS:
		{
			s.ParseKey(s.GetKey(), s.GetArgStr());
			ClientIterator it;
			for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
			{
				CChar * pChar = pClient->GetChar();
				if ( !pChar || ( pChar->GetRegion() != this ))
					continue;

				CScript script(s.GetArgRaw());
				script.CopyParseState(s);
				pChar->r_Verb(script, pSrc);
			}
			return true;
		}
		case RV_TAGLIST:
		{
			m_TagDefs.DumpKeys( pSrc, "TAG." );
			return true;
		}

		default:
			break;
	}

	if ( index < 0 )
	{
		index = FindTableSorted(s.GetKey(), CSector::sm_szVerbKeys, SEV_QTY);
		if ( index >= 0 )
			return SendSectorsVerb(s.GetKey(), s.GetArgRaw(), pSrc);
	}

	return CScriptObj::r_Verb(s, pSrc);
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CRegion::SendSectorsVerb( lpctstr pszVerb, lpctstr pszArgs, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CRegion::SendSectorsVerb");
	// Send a command to all the CSectors in this region.

	bool fRet = false;
	for ( int i=0; ; ++i )
	{
		CSector * pSector = GetSector(i);
		if ( pSector == nullptr )
			break;

		// Does the rect overlap ?
		if ( IsOverlapped( pSector->GetRect() ) )
		{
			CScript script( pszVerb, pszArgs );
			fRet |= pSector->r_Verb( script, pSrc );
		}
	}
	return fRet;
}

lpctstr const CRegion::sm_szTrigName[RTRIG_QTY+1] =	// static
{
	"@AAAUNUSED",
	"@CLIPERIODIC",
	"@ENTER",
	"@EXIT",
	"@REGPERIODIC",
	"@STEP",
	nullptr
};

TRIGRET_TYPE CRegion::OnRegionTrigger( CTextConsole * pSrc, RTRIG_TYPE iAction )
{
	ADDTOCALLSTACK("CRegion::OnRegionTrigger");
	// RETURN: true = halt processing (don't allow in this region)

	TRIGRET_TYPE iRet;

	for ( size_t i = 0; i < m_Events.size(); ++i )
	{
		CResourceLink * pLink = m_Events[i].GetRef();
		if ( !pLink || ( pLink->GetResType() != RES_REGIONTYPE ) || !pLink->HasTrigger(iAction) )
			continue;

		CResourceLock s;
		if ( pLink->ResourceLock(s) )
		{
			iRet = CScriptObj::OnTriggerScript(s, sm_szTrigName[iAction], pSrc);
			if ( iRet == TRIGRET_RET_TRUE )
				return iRet;
		}
	}

	//	EVENTSREGION triggers (constant events of regions set from sphere.ini)
	for ( size_t i = 0; i < g_Cfg.m_pEventsRegionLink.size(); ++i )
	{
		CResourceLink * pLink = g_Cfg.m_pEventsRegionLink[i].GetRef();
		if ( !pLink || ( pLink->GetResType() != RES_REGIONTYPE ) || !pLink->HasTrigger(iAction) )
			continue;

		CResourceLock s;
		if ( !pLink->ResourceLock(s) )
			continue;

		iRet = CScriptObj::OnTriggerScript(s, sm_szTrigName[iAction], pSrc);
		if ( iRet != TRIGRET_RET_FALSE && iRet != TRIGRET_RET_DEFAULT )
			return iRet;
	}

	return TRIGRET_RET_DEFAULT;
}

//*************************************************************************
// -CRegionWorld

CRegionWorld::CRegionWorld( CResourceID rid, lpctstr pszName ) :
	CRegion( rid, pszName )
{
}

CRegionWorld::~CRegionWorld()
{
}

enum RWC_TYPE
{
	RWC_REGION,
	RWC_RESOURCES,
	RWC_QTY
};

lpctstr const CRegionWorld::sm_szLoadKeys[RWC_QTY+1] =	// static
{
	"REGION",
	"RESOURCES",
	nullptr
};

bool CRegionWorld::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CRegionWorld::r_GetRef");
	return( CRegion::r_GetRef( ptcKey, pRef ));
}

bool CRegionWorld::r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UnreferencedParameter(fNoCallChildren);
	ADDTOCALLSTACK("CRegionWorld::r_WriteVal");
	EXC_TRY("WriteVal");
	//bool	fZero	= false;
	switch ( FindTableHeadSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
	{
		case RWC_RESOURCES:
			m_Events.WriteResourceRefList( sVal );
			break;
		case RWC_REGION:
			{
				// Check that the syntax is correct.
				if ( ptcKey[6] && ptcKey[6] != '.' )
					return false;

				CRegionWorld * pRegionTemp = dynamic_cast <CRegionWorld*>(m_pt.GetRegion( REGION_TYPE_AREA ));

				if ( !ptcKey[6] )
				{
					// We're just checking if the reference is valid.
					sVal.FormatVal( pRegionTemp? 1:0 );
					return true;
				}

				// ELSE - We're trying to retrieve a property from the region.
				ptcKey += 7;
				if ( pRegionTemp && m_pt.GetRegion( REGION_TYPE_MULTI ) )
					return pRegionTemp->r_WriteVal( ptcKey, sVal, pSrc );

				return( this->r_WriteVal( ptcKey, sVal, pSrc ));
			}
		default:
			return (fNoCallParent ? false : CRegion::r_WriteVal( ptcKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", ptcKey, static_cast<lpctstr>(sVal), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CRegionWorld::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CRegionWorld::r_LoadVal");
	EXC_TRY("LoadVal");

	// Load the values for the region from script.
	switch ( FindTableHeadSorted( s.GetKey(), sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 ))
	{
		case RWC_RESOURCES:
			SetModified( REGMOD_EVENTS );
			return( m_Events.r_LoadVal( s, RES_REGIONTYPE ));
		default:
			break;
	}
	return(CRegion::r_LoadVal(s));
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}



void CRegionWorld::r_WriteModified( CScript &s )
{
	ADDTOCALLSTACK("CRegionWorld::r_WriteModified");
	CRegion::r_WriteModified( s );

	if ( m_dwModifiedFlags & REGMOD_TAGS )
	{
		m_TagDefs.r_WritePrefix(s, "TAG", "GUARDOWNER");
	}
}


void CRegionWorld::r_WriteBody( CScript &s, lpctstr pszPrefix )
{
	ADDTOCALLSTACK("CRegionWorld::r_WriteBody");
	CRegion::r_WriteBody( s, pszPrefix );
}


void CRegionWorld::r_Write( CScript &s )
{
	ADDTOCALLSTACK_DEBUG("CRegionWorld::r_Write");
	s.WriteSection( "AREADEF %s", GetResourceName());
	r_WriteBase( s );

}

/*enum RWV_TYPE
{
	RWV_TAGLIST,
	RWV_QTY
};*/

/*lpctstr const CRegionWorld::sm_szVerbKeys[] =
{
	"TAGLIST",
	nullptr
};*/

bool CRegionWorld::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CRegionWorld::r_Verb");

	EXC_TRY("Verb");
	return CRegion::r_Verb( s, pSrc );
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}


const CRandGroupDef * CRegionWorld::FindNaturalResource(int type) const
{
	ADDTOCALLSTACK("CRegionWorld::FindNaturalResource");
	// Find the natural resources assinged to this region.
	// ARGS: type = IT_TYPE

	for ( size_t i = 0; i < m_Events.size(); ++i )
	{
		CResourceLink * pLink = m_Events[i].GetRef();
		if ( !pLink || ( pLink->GetResType() != RES_REGIONTYPE ))
			continue;

		if ( pLink->GetResPage() == type )
			return (dynamic_cast <const CRandGroupDef *>(pLink));
	}
	return nullptr;
}
