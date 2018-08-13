// Common for client and server.

#include "../common/CUIDExtra.h"
#include "../common/CResourceLock.h"
#include "../common/CScript.h"
#include "../sphere/threads.h"
#include "chars/CChar.h"
#include "clients/CClient.h"

//************************************************************************
// -CTeleport

CTeleport::CTeleport( tchar * pszArgs )
{
	// RES_TELEPORT
	// Assume valid iArgs >= 5

	tchar * ppCmds[4];
	size_t iArgs = Str_ParseCmds( pszArgs, ppCmds, CountOf( ppCmds ), "=" );
	if ( iArgs < 2 )
	{
		DEBUG_ERR(( "Bad CTeleport Def\n" ));
		return;
	}
	Read( ppCmds[0] );
	m_ptDst.Read( ppCmds[1] );
	if ( ppCmds[3] )
		bNpc = (ATOI(ppCmds[3]) != 0);
	else
		bNpc = false;
}

bool CTeleport::RealizeTeleport()
{
	ADDTOCALLSTACK("CTeleport::RealizeTeleport");
	if ( ! IsCharValid() || ! m_ptDst.IsCharValid())
	{
		DEBUG_ERR(( "CTeleport bad coords %s\n", WriteUsed() ));
		return false;
	}
	CSector *pSector = GetSector();
	if ( pSector )
		return pSector->AddTeleport(this);
	else
		return false;
}


//*************************************************************************
// -CRegion

CRegion::CRegion( CResourceID rid, lpctstr pszName ) :
	CResourceDef( rid )
{
	m_dwFlags	= 0;
	m_iModified	= 0;
	m_iLinkedSectors = 0;
	if ( pszName )
		SetName( pszName );
    _pMultiLink = nullptr;
}

CRegion::~CRegion()
{
	// RemoveSelf();
	UnRealizeRegion();
}

void CRegion::SetModified( int iModFlag )
{
	ADDTOCALLSTACK("CRegion::SetModified");
	if ( !m_iLinkedSectors ) return;
	m_iModified		= m_iModified | iModFlag;
}

void CRegion::UnRealizeRegion()
{
	ADDTOCALLSTACK("CRegion::UnRealizeRegion");
	// remove myself from the world.
	// used in the case of a ship where the region will move.

	for ( int i=0; ; i++ )
	{
		CSector * pSector = GetSector(i);
		if ( pSector == NULL )
			break;
		// Does the rect overlap ?
		if ( ! IsOverlapped( pSector->GetRect()))
			continue;
		if ( pSector->UnLinkRegion( this ))
			m_iLinkedSectors--;
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
	for ( int i = 0; i < g_MapList.GetSectorQty(m_pt.m_map); i++ )
	{
		CSector *pSector = g_World.GetSector(m_pt.m_map, i);

		if ( pSector && IsOverlapped(pSector->GetRect()) )
		{
			//	Yes, this sector overlapped, so add it to the sector list
			if ( !pSector->LinkRegion(this) )
			{
				g_Log.EventError("Linking sector #%d for map %d for region %s failed (fatal for this region).\n", i, m_pt.m_map, GetName());
				return false;
			}
			m_iLinkedSectors++;
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
	if ( pszName == NULL || pszName[0] == '%' )
	{
		m_sName = g_Serv.GetName();
	}
	else
	{
		m_sName = pszName;
	}
}

enum RC_TYPE
{
	RC_ANNOUNCE,
	RC_ARENA,
	RC_BUILDABLE,
	RC_CLIENTS,
	RC_COLDCHANCE,
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
	NULL
};

bool CRegion::r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CRegion::r_WriteVal");
	EXC_TRY("WriteVal");
	bool	fZero	= false;
	RC_TYPE index = (RC_TYPE) FindTableHeadSorted( pszKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );
	if ( index < 0 )
	{
		return( CScriptObj::r_WriteVal( pszKey, sVal, pSrc ));
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
				int i = 0;
				size_t iClients = 0;
				for ( ; ; i++ )
				{
					CSector	*pSector = GetSector(i);
					if ( pSector == NULL ) break;
					iClients += pSector->m_Chars_Active.HasClients();
				}
				sVal.FormatVal((int)(iClients));
				break;
			}
		case RC_EVENTS:
			m_Events.WriteResourceRefList( sVal );
			break;
		case RC_ISEVENT:
			if ( pszKey[7] != '.' )
				return false;
			pszKey += 8;
			sVal = m_Events.ContainsResourceName(RES_EVENTS, pszKey) ? "1" : "0";
			return true;
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
				pszKey += 4;
				if ( *pszKey == '\0' )
				{
					sVal.FormatVal( (int)(iQty));
					return true;
				}
				SKIP_SEPARATORS( pszKey );
				size_t iRect = Exp_GetVal( pszKey );
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
			sVal.FormatVal( (int)(m_TagDefs.GetCount()) );
			break;
		case RC_TAGAT:
			{
				pszKey += 5; // eat the 'TAGAT'
				if ( *pszKey == '.' ) // do we have an argument?
				{
					SKIP_SEPARATORS( pszKey );
					size_t iQty = (size_t)( Exp_GetVal( pszKey ) );
					if ( iQty >= m_TagDefs.GetCount() )
						return false; // trying to get non-existant tag

					CVarDefCont * pTagAt = m_TagDefs.GetAt( iQty );
					if ( !pTagAt )
						return false; // trying to get non-existant tag

					SKIP_SEPARATORS( pszKey );
					if ( ! *pszKey )
					{
						sVal.Format("%s=%s", static_cast<lpctstr>(pTagAt->GetKey()), static_cast<lpctstr>(pTagAt->GetValStr()));
						return true;
					}
					else if ( !strnicmp( pszKey, "KEY", 3 )) // key?
					{
						sVal = static_cast<lpctstr>(pTagAt->GetKey());
						return true;
					}
					else if ( !strnicmp( pszKey, "VAL", 3 )) // val?
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
			pszKey++;
		case RC_TAG:	// "TAG" = get/set a local tag.
			{
				if ( pszKey[3] != '.' )
					return false;
				pszKey += 4;
				sVal = m_TagDefs.GetKeyStr( pszKey, fZero );
				return true;
			}
		case RC_TYPEREGION:
			{
				const CItemBase * pBase = NULL;
				const CItem * pItem = GetResourceID().ItemFind();
				if (pItem != NULL)
					pBase = pItem->Item_GetDef();

				if (pBase != NULL)
					sVal = pBase->GetResourceName();
				else
					sVal = "";
			} break;
		case RC_UID:
			// Allow use of UID.x.KEY on the REGION object
			if ( pszKey[3] == '.' )
				return CScriptObj::r_WriteVal( pszKey, sVal, pSrc );

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
	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", pszKey, static_cast<lpctstr>(sVal), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CRegion::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CRegion::r_LoadVal");
	EXC_TRY("LoadVal");

	if ( s.IsKeyHead( "TAG.", 4 ))
	{
		SetModified( REGMOD_TAGS );
		bool fQuoted = false;
		m_TagDefs.SetStr( s.GetKey()+ 4, fQuoted, s.GetArgStr( &fQuoted ), false );
		return true;
	}
	if ( s.IsKeyHead( "TAG0.", 5 ))
	{
		SetModified( REGMOD_TAGS );
		bool fQuoted = false;
		m_TagDefs.SetStr( s.GetKey()+ 5, fQuoted, s.GetArgStr( &fQuoted ), false );
		return true;
	}

	RC_TYPE index = (RC_TYPE) FindTableSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 );
	if ( index < 0 )
		return false;

	switch ( index )
	{
		case RC_ANNOUNCE:
			TogRegionFlags( REGION_FLAG_ANNOUNCE, ( ! s.HasArgs()) || s.GetArgVal());
			break;
		case RC_ARENA:
			TogRegionFlags( REGION_FLAG_ARENA, s.GetArgVal() != 0);
			break;
		case RC_BUILDABLE:
			TogRegionFlags( REGION_FLAG_NOBUILDING, ! s.GetArgVal());
			break;
		case RC_COLDCHANCE:
			SendSectorsVerb( s.GetKey(), s.GetArgStr(), &g_Serv );
			break;
		case RC_EVENTS:
			SetModified( REGMOD_EVENTS );
			return( m_Events.r_LoadVal( s, RES_REGIONTYPE ));
		case RC_FLAGS:
			m_dwFlags = ( s.GetArgDWVal() &~REGION_FLAG_SHIP ) | ( m_dwFlags & REGION_FLAG_SHIP );
			SetModified( REGMOD_FLAGS );
			break;
		case RC_GATE:
			TogRegionFlags( REGION_ANTIMAGIC_GATE, ! s.GetArgVal());
			break;
		case RC_GROUP:
			m_sGroup	= s.GetArgStr();
			SetModified( REGMOD_GROUP );
			break;
		case RC_GUARDED:
			TogRegionFlags( REGION_FLAG_GUARDED, ( ! s.HasArgs()) || s.GetArgVal());
			break;
		case RC_MAGIC:
			TogRegionFlags( REGION_ANTIMAGIC_ALL, ! s.GetArgVal());
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
				return( AddRegionRect( rect ));
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
	TemporaryString tsZ;
	tchar* z = static_cast<tchar *>(tsZ);
	if ( GetRegionFlags())
	{
		sprintf(z, "%sFLAGS", pszPrefix);
		s.WriteKeyHex(z, GetRegionFlags());
	}

	if (m_Events.size() > 0 )
	{
		CSString sVal;
		m_Events.WriteResourceRefList( sVal );
		sprintf(z, "%sEVENTS", pszPrefix);
		s.WriteKey(z, sVal);
	}

	// Write New variables
	m_BaseDefs.r_WritePrefix(s);

	// Write out any extra TAGS here.
	sprintf(z, "%sTAG", pszPrefix);
	m_TagDefs.r_WritePrefix(s, z);
}


void CRegion::r_WriteModified( CScript &s )
{
	ADDTOCALLSTACK("CRegion::r_WriteModified");
	if ( m_iModified & REGMOD_NAME )
		s.WriteKey("NAME", GetName() );

	if ( m_iModified & REGMOD_GROUP )
		s.WriteKey("GROUP", m_sGroup );

	if ( m_iModified & REGMOD_FLAGS )
	{
		s.WriteKeyHex("FLAGS", GetRegionFlags() );
	}

	if ( m_iModified & REGMOD_EVENTS )
	{
		CSString sVal;
		m_Events.WriteResourceRefList( sVal );
		s.WriteKey( "EVENTS", sVal );
	}
}



void CRegion::r_WriteBase( CScript &s )
{
	ADDTOCALLSTACK("CRegion::r_WriteBase");
	if ( GetName() && GetName()[0] )
		s.WriteKey("NAME", GetName());

	if ( ! m_sGroup.IsEmpty() )
		s.WriteKey("GROUP", static_cast<lpctstr>(m_sGroup));

	CRegion::r_WriteBody( s, "" );

	if ( m_pt.IsValidPoint())
		s.WriteKey("P", m_pt.WriteUsed());
	else if ( m_pt.m_map )
		s.WriteKeyVal("MAP", m_pt.m_map);

	size_t iQty = GetRegionRectCount();
	for ( size_t i = 0; i < iQty; i++ )
	{
		s.WriteKey("RECT", GetRegionRect(i).Write() );
	}
}

void CRegion::r_Write( CScript &s )
{
	ADDTOCALLSTACK_INTENSIVE("CRegion::r_Write");
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
	NULL
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
	lpctstr pszKey = s.GetKey();

	if ( !strnicmp(pszKey, "CLEARTAGS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPARATORS(pszKey);
		m_TagDefs.ClearKeys(pszKey);
		return true;
	}

	int index = FindTableSorted( pszKey, sm_szVerbKeys, CountOf( sm_szVerbKeys )-1 );
	switch (index)
	{
		case RV_ALLCLIENTS:
		{
			s.ParseKey(s.GetKey(), s.GetArgStr());
			ClientIterator it;
			for (CClient* pClient = it.next(); pClient != NULL; pClient = it.next())
			{
				CChar * pChar = pClient->GetChar();
				if ( !pChar || ( pChar->GetRegion() != this ))
					continue;
				
				CScript script(s.GetArgRaw());
				script.m_iResourceFileIndex = s.m_iResourceFileIndex;	// Index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
				script.m_iLineNum = s.m_iLineNum;						// Line in the script file where Key/Arg were read
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
	for ( int i=0; ; i++ )
	{
		CSector * pSector = GetSector(i);
		if ( pSector == NULL )
			break;

		// Does the rect overlap ?
		if ( IsOverlapped( pSector->GetRect() ) )
		{
			CScript script( pszVerb, pszArgs );
			fRet |= pSector->r_Verb( script, pSrc );
		}
	}
	return fRet ;
}

lpctstr const CRegion::sm_szTrigName[RTRIG_QTY+1] =	// static
{
	"@AAAUNUSED",
	"@CLIPERIODIC",
	"@ENTER",
	"@EXIT",
	"@REGPERIODIC",
	"@STEP",
	NULL,
};

TRIGRET_TYPE CRegion::OnRegionTrigger( CTextConsole * pSrc, RTRIG_TYPE iAction )
{
	ADDTOCALLSTACK("CRegion::OnRegionTrigger");
	// RETURN: true = halt prodcessing (don't allow in this region

	TRIGRET_TYPE iRet;

	for ( size_t i = 0; i < m_Events.size(); ++i )
	{
		CResourceLink * pLink = m_Events[i];
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
		CResourceLink * pLink = g_Cfg.m_pEventsRegionLink[i];
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
	RWC_DEFNAME,
	RWC_REGION,
	RWC_RESOURCES,
	RWC_QTY
};

lpctstr const CRegionWorld::sm_szLoadKeys[RWC_QTY+1] =	// static
{
	"DEFNAME",
	"REGION",
	"RESOURCES",
	NULL
};

bool CRegionWorld::r_GetRef( lpctstr & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CRegionWorld::r_GetRef");
	return( CRegion::r_GetRef( pszKey, pRef ));
}

bool CRegionWorld::r_WriteVal( lpctstr pszKey, CSString & sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CRegionWorld::r_WriteVal");
	EXC_TRY("WriteVal");
	//bool	fZero	= false;
	switch ( FindTableHeadSorted( pszKey, sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		case RWC_DEFNAME: // "DEFNAME" = for the speech system.
			sVal = GetResourceName();
			break;
		case RWC_RESOURCES:
			m_Events.WriteResourceRefList( sVal );
			break;
		case RWC_REGION:
			{
				// Check that the syntax is correct.
				if ( pszKey[6] && pszKey[6] != '.' )
					return false;

				CRegionWorld * pRegionTemp = dynamic_cast <CRegionWorld*>(m_pt.GetRegion( REGION_TYPE_AREA ));

				if ( !pszKey[6] )
				{
					// We're just checking if the reference is valid.
					sVal.FormatVal( pRegionTemp? 1:0 );
					return true;
				}

				// ELSE - We're trying to retrieve a property from the region.
				pszKey += 7;
				if ( pRegionTemp && m_pt.GetRegion( REGION_TYPE_MULTI ) )
					return pRegionTemp->r_WriteVal( pszKey, sVal, pSrc );

				return( this->r_WriteVal( pszKey, sVal, pSrc ));
			}
		default:
			return( CRegion::r_WriteVal( pszKey, sVal, pSrc ));
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' ret '%s' [%p]\n", pszKey, static_cast<lpctstr>(sVal), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}

bool CRegionWorld::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CRegionWorld::r_LoadVal");
	EXC_TRY("LoadVal");

	// Load the values for the region from script.
	switch ( FindTableHeadSorted( s.GetKey(), sm_szLoadKeys, CountOf( sm_szLoadKeys )-1 ))
	{
		case RWC_DEFNAME: // "DEFNAME" = for the speech system.
			return SetResourceName( s.GetArgStr());
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

	if ( m_iModified & REGMOD_TAGS )
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
	ADDTOCALLSTACK_INTENSIVE("CRegionWorld::r_Write");
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
	NULL
};*/

bool CRegionWorld::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CRegionWorld::r_Verb");
	EXC_TRY("Verb");
/*	lpctstr pszKey = s.GetKey();

	if ( !strnicmp(pszKey, "CLEARTAGS", 9) )
	{
		pszKey = s.GetArgStr();
		SKIP_SEPARATORS(pszKey);
		m_TagDefs.ClearKeys(pszKey);
		return true;
	}

	int index = FindTableSorted( pszKey, sm_szVerbKeys, CountOf( sm_szVerbKeys )-1 );
	switch(index)
	{
		case RWV_TAGLIST:
			m_TagDefs.DumpKeys( pSrc, "TAG." );
			return true;
	}*/

	return( CRegion::r_Verb( s, pSrc ));
	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("command '%s' args '%s' [%p]\n", s.GetKey(), s.GetArgRaw(), static_cast<void *>(pSrc));
	EXC_DEBUG_END;
	return false;
}


const CSRandGroupDef * CRegionWorld::FindNaturalResource(int type) const
{
	ADDTOCALLSTACK("CRegionWorld::FindNaturalResource");
	// Find the natural resources assinged to this region.
	// ARGS: type = IT_TYPE

	for ( size_t i = 0; i < m_Events.size(); i++ )
	{
		CResourceLink * pLink = m_Events[i];
		if ( !pLink || ( pLink->GetResType() != RES_REGIONTYPE ))
			continue;

		if ( pLink->GetResPage() == type )
			return (dynamic_cast <const CSRandGroupDef *>(pLink));
	}
	return NULL;
}
