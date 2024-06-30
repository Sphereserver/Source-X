#include "../common/CLog.h"
#include "chars/CChar.h"
#include "items/CItem.h"
#include "CObjBase.h"
#include "CWorld.h"
#include "CWorldSearch.h"

struct CImportSer : public CSObjListRec
{
	// Temporary holding structure for new objects being impoted.

public:
	// Translate the import UID's into my UID's
	const dword m_dwSer;		// My Imported serial number
	CObjBase * m_pObj;	// new world object corresponding.
	dword m_dwContSer;	// My containers' serial number
	LAYER_TYPE m_layer;	// UOX does this diff than us. so store this here.

public:
	bool IsTopLevel() const noexcept
	{
		return( m_dwContSer == UID_UNUSED );
	}

	CImportSer( dword dwSer ) noexcept :
		m_dwSer( dwSer )
	{
		m_pObj = nullptr;
		m_dwContSer = UID_UNUSED;
		m_layer = LAYER_NONE;
	}
	~CImportSer() noexcept = default;

	CImportSer(const CImportSer& copy) = delete;
	CImportSer& operator=(const CImportSer& other) = delete;
};

struct CImportFile
{
	CObjBase * m_pCurObj;		// current entry.
	CImportSer * m_pCurSer;	// current entry.

	CSObjList m_ListSer;	// list of imported objects i'm working on.

	const word m_wModeFlags;	// IMPFLAGS_TYPE
	const CPointMap m_ptCenter;
	const int m_iDist;	// distance fom center.

	tchar * m_pszArg1;	// account
	tchar * m_pszArg2;	// name

public:
	CImportFile( word wModeFlags, CPointMap ptCenter, int iDist ) :
		m_wModeFlags(wModeFlags),
		m_ptCenter(ptCenter),
		m_iDist(iDist)
	{
		m_pCurSer = nullptr;
		m_pCurObj = nullptr;
		m_pszArg1 = nullptr;
		m_pszArg2 = nullptr;
	}

	CImportFile(const CImportFile& copy) = delete;
	CImportFile& operator=(const CImportFile& other) = delete;

public:
	void CheckLast();
	void ImportFix();
	bool ImportSCP( CScript & s, word wModeFlags );
	bool ImportWSC( CScript & s, word wModeFlags );
};

void CImportFile::CheckLast()
{
	ADDTOCALLSTACK("CImportFile::CheckLast");
	// Make sure the object we where last working on is linked to the world.

	if ( m_pCurSer != nullptr )
	{
		if ( m_pCurObj != nullptr && m_pCurSer->m_pObj == m_pCurObj )
		{
			// Do we even want it ?
			if ( m_iDist &&
				m_ptCenter.IsValidPoint() &&
				m_pCurSer->IsTopLevel() &&
				m_ptCenter.GetDist( m_pCurObj->GetTopPoint()) > m_iDist )
			{
				delete m_pCurSer;
			}
			else
			{
				m_pCurObj = nullptr;	// accept it.
			}
		}
		else
		{
			delete m_pCurSer;
		}
		m_pCurSer = nullptr;
	}
	if ( m_pCurObj != nullptr )
	{
		delete m_pCurObj;
		m_pCurObj = nullptr;
	}
}

void CImportFile::ImportFix()
{
	ADDTOCALLSTACK("CImportFile::ImportFix");
	// adjust all the containered items and eliminate duplicates.

	CheckLast();

	int iRemoved = 0;

	CImportSer * pSerNext;
	m_pCurSer = static_cast <CImportSer*> ( m_ListSer.GetContainerHead());
	for ( ; m_pCurSer != nullptr; m_pCurSer = pSerNext )
	{
		pSerNext = static_cast <CImportSer*> ( m_pCurSer->GetNext());
		if ( m_pCurSer->m_pObj == nullptr )		// NEver created correctly
		{
			delete m_pCurSer;
			continue;
		}

		// Make sure this item is not a dupe ?

		CItem * pItemTest;
		if ( m_pCurSer->IsTopLevel())	// top level only
		{
			if ( m_pCurSer->m_pObj->IsItem())
			{
				CItem * pItemCheck = dynamic_cast <CItem*>( m_pCurSer->m_pObj );
				ASSERT(pItemCheck);
				pItemCheck->SetAttr(ATTR_MOVE_NEVER);

				auto AreaItems = CWorldSearchHolder::GetInstance(m_pCurSer->m_pObj->GetTopPoint());
				for (;;)
				{
					CItem * pItem = AreaItems->GetItem();
					if ( pItem == nullptr )
						break;
					if ( ! pItem->IsSameType( m_pCurSer->m_pObj ))
						continue;
					pItem->SetName( m_pCurSer->m_pObj->GetName());
					if ( ! ( m_pCurSer->m_pObj->GetTopZ() == pItem->GetTopZ()))
						continue;

					goto item_delete;
				}
			}
			else
			{
				// dupe char ?
			}

			// Make sure the top level object is placed correctly.
			m_pCurSer->m_pObj->MoveTo( m_pCurSer->m_pObj->GetTopPoint());
			m_pCurSer->m_pObj->Update();
			if ( ! m_pCurSer->m_pObj->IsContainer())
				delete m_pCurSer;
			continue;
		}

		pItemTest = dynamic_cast <CItem*> (m_pCurSer->m_pObj);
		if ( pItemTest == nullptr )
		{
		item_delete:
			delete m_pCurSer->m_pObj;
			delete m_pCurSer;
			++ iRemoved;
			continue;
		}

		// Find it's container.
		CImportSer* pSerCont = static_cast <CImportSer*> ( m_ListSer.GetContainerHead());
		CObjBase * pObjCont = nullptr;
		for ( ; pSerCont != nullptr; pSerCont = static_cast <CImportSer*> ( pSerCont->GetNext()))
		{
			if ( pSerCont->m_pObj == nullptr )
				continue;
			if ( pSerCont->m_dwSer == m_pCurSer->m_dwContSer )
			{
				pObjCont = pSerCont->m_pObj;
				if ( ! pItemTest->LoadSetContainer( pObjCont->GetUID(), m_pCurSer->m_layer ))
				{
					goto item_delete;	// not in a cont ?
				}
				m_pCurSer->m_dwContSer = UID_UNUSED;	// found it.
				break;
			}
		}
		if ( ! m_pCurSer->IsTopLevel() || pObjCont == nullptr)
		{
			goto item_delete;
		}

		// Is it a dupe in the container or equipped ?
		CContainer* pObjContBase = dynamic_cast<CContainer*>(pObjCont);
		ASSERT(pObjCont);
		for (CSObjContRec* pObjRec : *pObjContBase)
		{
			CItem* pItem = static_cast<CItem*>(pObjRec);
			if ( pItemTest == pItem )
				continue;
			if ( pItemTest->IsItemEquipped())
			{
				if ( pItemTest->GetEquipLayer() != pItem->GetEquipLayer())
					continue;
			}
			else
			{
				if ( ! pItemTest->GetContainedPoint().IsSame2D( pItem->GetContainedPoint()))
					continue;
			}
			if ( ! pItemTest->IsSameType( pItem ))
				continue;
			goto item_delete;
		}

		// done with it if not a container.
		if ( ! pItemTest->IsContainer())
			delete m_pCurSer;
	}

	if ( iRemoved )
	{
		DEBUG_ERR(( "Import: removed %d bad items\n", iRemoved ));
	}
	m_ListSer.ClearContainer();	// done with the list now.
}

bool CImportFile::ImportSCP( CScript & s, word wModeFlags )
{
	ADDTOCALLSTACK("CImportFile::ImportSCP");
	// Import a SPHERE format SCP file.

	while ( s.FindNextSection())
	{
		CheckLast();
		if ( s.IsSectionType( "ACCOUNT" ))
		{
			g_Cfg.LoadResourceSection( &s );
			continue;
		}
		else if ( s.IsSectionType( "WORLDCHAR" ) || s.IsSectionType("WC"))
		{
			ImportFix();
			if ( wModeFlags & IMPFLAGS_CHARS )
			{
				m_pCurObj = CChar::CreateBasic(static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr())));
			}
		}
		else if ( s.IsSectionType( "WORLDITEM" ) || s.IsSectionType("WI"))
		{
			if ( wModeFlags & IMPFLAGS_ITEMS )
			{
				m_pCurObj = CItem::CreateTemplate((ITEMID_TYPE)(g_Cfg.ResourceGetIndexType(RES_ITEMDEF, s.GetArgStr())));
			}
		}
		else
		{
			continue;
		}

		if ( m_pCurObj == nullptr )
			continue;

		while ( s.ReadKeyParse())
		{
			if ( s.IsKey( "SERIAL"))
			{
				if ( m_pCurSer != nullptr )
					return false;
				m_pCurSer = new CImportSer( s.GetArgVal());
				m_pCurSer->m_pObj = m_pCurObj;
				m_ListSer.InsertContentHead( m_pCurSer );
				continue;
			}

			if ( m_pCurSer == nullptr )
				continue;

			if ( s.IsKey( "CONT" ))
			{
				m_pCurSer->m_dwContSer = s.GetArgVal();
			}
			else if ( s.IsKey( "LAYER" ))
			{
				m_pCurSer->m_layer = static_cast<LAYER_TYPE>(s.GetArgVal());
			}
			else
			{
				m_pCurObj->r_LoadVal( s );
			}
		}
	}

	return true;
}

bool CImportFile::ImportWSC( CScript & s, word wModeFlags )
{
	ADDTOCALLSTACK("CImportFile::ImportWSC");
	// This file is a WSC or UOX world script file.

	IMPFLAGS_TYPE mode = IMPFLAGS_NOTHING;
	CSString sName;
	CItem * pItem = nullptr;
	CChar * pChar = nullptr;

	while ( s.ReadTextLine(true))
	{
		if ( s.IsKeyHead( "SECTION WORLDITEM", 17 ))
		{
			CheckLast();
			mode = IMPFLAGS_ITEMS;
			continue;
		}
		else if ( s.IsKeyHead( "SECTION CHARACTER", 17 ))
		{
			CheckLast();
			mode = ( wModeFlags & IMPFLAGS_CHARS ) ? IMPFLAGS_CHARS : IMPFLAGS_NOTHING;
			continue;
		}
		else if ( s.GetKey()[0] == '{' )
		{
			CheckLast();
			continue;
		}
		else if ( s.GetKey()[0] == '}' )
		{
			CheckLast();
			mode = IMPFLAGS_NOTHING;
			continue;
		}
		else if ( mode == IMPFLAGS_NOTHING )
			continue;
		else if ( s.GetKey()[0] == '\\' )
			continue;

		// Parse the line.
		tchar* pKey = const_cast<tchar*>(strchr(s.GetKey(), ' '));
		lpctstr pArg = nullptr;

		if (pKey != nullptr)
		{
			*pKey++ = '\0';
			GETNONWHITESPACE(pKey);
			pArg = pKey;
		}
		else
		{
			pArg = "";
		}

		if ( s.IsKey("SERIAL" ))
		{
			if ( m_pCurSer != nullptr )
				return false;

			dword dwSerial = atoi( pArg );
			if ( dwSerial == UID_UNUSED )
			{
				DEBUG_ERR(( "Import:Bad serial number\n" ));
				break;
			}
			m_pCurSer = new CImportSer( dwSerial );
			m_ListSer.InsertContentHead( m_pCurSer );
			continue;
		}
		if ( s.IsKey("NAME" ))
		{
			sName = ( pArg[0] == '#' ) ? "" : pArg;
			if ( mode == IMPFLAGS_ITEMS )
				continue;
		}
		if ( m_pCurSer == nullptr )
		{
			DEBUG_ERR(( "Import:No serial number\n" ));
			break;
		}

		if ( mode == IMPFLAGS_ITEMS )	// CItem.
		{
			if ( s.IsKey("ID" ))
			{
				if ( m_pCurObj != nullptr )
					return false;
				pItem = CItem::CreateTemplate((ITEMID_TYPE)atoi(pArg));
                if (!pItem)
                {
                    DEBUG_ERR(("Import: Bad Item '%s'\n", pArg));
                    break;
                }
				pItem->SetName( sName );
				m_pCurObj = pItem;
				m_pCurSer->m_pObj = pItem;
				continue;
			}

			if ( m_pCurObj == nullptr )
			{
				DEBUG_ERR(( "Import: Bad Item Key '%s'\n", s.GetKey()));
				break;
			}
			else if ( s.IsKey("CONT" ))
			{
				m_pCurSer->m_dwContSer = atoi(pArg);
			}
			else if ( s.IsKey("LAYER" ))
			{
				m_pCurSer->m_layer = static_cast<LAYER_TYPE>(atoi(pArg));
				continue;
			}
			else if (pItem == nullptr)
			{
				DEBUG_ERR(( "Import:Found '%s' before ID.\n", s.GetKey()));
				continue;
			}

			if ( s.IsKey("X" ))
			{
				CPointMap pt = pItem->GetUnkPoint();
				pt.m_x = (short)( atoi(pArg) );
				pItem->SetUnkPoint(pt);
				continue;
			}
			else if ( s.IsKey("Y" ))
			{
				CPointMap pt = pItem->GetUnkPoint();
				pt.m_y = (short)( atoi(pArg) );
				pItem->SetUnkPoint(pt);
				continue;
			}
			else if ( s.IsKey("Z" ))
			{
				CPointMap pt = pItem->GetUnkPoint();
				pt.m_z = (char)( atoi(pArg) );
				pItem->SetUnkPoint(pt);
				continue;
			}
			else if ( s.IsKey("COLOR" ))
			{
				pItem->SetHue( (HUE_TYPE)( atoi(pArg) ) );
				continue;
			}
			else if ( s.IsKey("AMOUNT" ))
			{
				pItem->SetAmount( (word)atoi(pArg) );
				continue;
			}
			else if ( s.IsKey("MOREX" ))
			{
				pItem->m_itNormal.m_morep.m_x = (short)(atoi(pArg));
				continue;
			}
			else if ( s.IsKey("MOREY" ))
			{
				pItem->m_itNormal.m_morep.m_y = (short)(atoi(pArg));
				continue;
			}
			else if ( s.IsKey("MOREZ" ))
			{
				pItem->m_itNormal.m_morep.m_z = (char)( atoi(pArg) );
				continue;
			}
			else if ( s.IsKey("MORE" ))
			{
				pItem->m_itNormal.m_more1 = Str_ToUI(pArg);
				continue;
			}
			else if ( s.IsKey("MORE2" ))
			{
				pItem->m_itNormal.m_more2 = Str_ToUI(pArg);
				continue;
			}
			else if ( s.IsKey("DYEABLE" ))
			{
				if ( atoi(pArg))
					pItem->m_CanMask |= CAN_I_DYE;
				continue;
			}
			else if ( s.IsKey("ATT" ))
			{
				// pItem->m_pDef->m_attackBase = atoi(pArg);
			}
			else if ( s.IsKey("TYPE" ))
			{
				// ??? translate the type field.
				//int i = atoi(pArg);
			}
		}

		if ( mode == IMPFLAGS_CHARS )
		{
			if ( s.IsKey("NAME" ))
			{
				if ( m_pCurObj != nullptr )
					return false;
				pChar = CChar::CreateBasic( CREID_MAN );
				pChar->SetName( sName );
				m_pCurObj = pChar;
				m_pCurSer->m_pObj = pChar;
				continue;
			}
			if ( m_pCurObj == nullptr )
			{
				DEBUG_ERR(( "Import:Bad Item Key '%s'\n", s.GetKey()));
				break;
			}
			else if (pChar == nullptr)
			{
				DEBUG_ERR(( "Import:Found '%s' before NAME.\n", s.GetKey()));
				continue;
			}

			if ( s.IsKey("X" ))
			{
				CPointMap pt = pChar->GetUnkPoint();
				pt.m_x = (short)(atoi(pArg));
				pChar->SetUnkPoint(pt);
				continue;
			}
			else if ( s.IsKey("Y" ))
			{
				CPointMap pt = pChar->GetUnkPoint();
				pt.m_y = (short)(atoi(pArg));
				pChar->SetUnkPoint(pt);
				continue;
			}
			else if ( s.IsKey("Z" ))
			{
				CPointMap pt = pChar->GetUnkPoint();
				pt.m_z = (char)(atoi(pArg));
				pChar->SetUnkPoint(pt);
				continue;
			}
			else if ( s.IsKey("BODY" ))
			{
				pChar->SetID((CREID_TYPE)(atoi(pArg)));
				continue;
			}
			else if ( s.IsKey("SKIN" ))
			{
				pChar->SetHue( (HUE_TYPE)( atoi(pArg) ));
				continue;
			}
			else if ( s.IsKey("DIR" ))
			{
				pChar->m_dirFace = (DIR_TYPE)(atoi(pArg));
				if ( (pChar->m_dirFace < 0) || (pChar->m_dirFace >= DIR_QTY) )
					pChar->m_dirFace = DIR_SE;
				continue;
			}
			else if ( s.IsKey("XBODY" ))
			{
				pChar->_iPrev_id = (CREID_TYPE)(atoi(pArg));
				continue;
			}
			else if ( s.IsKey("XSKIN" ))
			{
				pChar->_wPrev_Hue = (HUE_TYPE)( atoi(pArg) );
				continue;
			}
			else if ( s.IsKey("FONT" ))
			{
				pChar->m_fonttype = (FONT_TYPE)(atoi(pArg));
				continue;
			}
			else if ( s.IsKey("KARMA" ))
			{
				pChar->SetKarma((short)(atoi(pArg)));
				continue;
			}
			else if ( s.IsKey("FAME" ))
			{
				pChar->SetFame((ushort)(atoi(pArg)));
				continue;
			}
			else if ( s.IsKey("TITLE" ))
			{
				pChar->m_sTitle = pArg;
				continue;
			}
			else if ( s.IsKey("STRENGTH" ))
			{
				pChar->Stat_SetBase(STAT_STR, (ushort)(atoi(pArg)));
			}
			else if ( s.IsKey("DEXTERITY" ))
			{
				pChar->Stat_SetBase(STAT_DEX, (ushort)(atoi(pArg)));
			}
			else if ( s.IsKey("INTELLIGENCE" ))
			{
				pChar->Stat_SetBase(STAT_INT, (ushort)(atoi(pArg)));
			}
			else if ( s.IsKey("HITPOINTS" ))
			{
				pChar->Stat_SetVal(STAT_STR,(ushort)(atoi(pArg)));
			}
			else if ( s.IsKey("STAMINA" ))
			{
				pChar->Stat_SetVal(STAT_DEX,(ushort)(atoi(pArg)));
			}
			else if ( s.IsKey( "MANA" ))
			{
				pChar->Stat_SetVal(STAT_INT,(ushort)(atoi(pArg)));
			}
			else if ( s.IsKeyHead( "SKILL", 5 ))
			{
				SKILL_TYPE skill = (SKILL_TYPE)(atoi( &(s.GetKey()[5]) ));
				if ( pChar->IsSkillBase(skill) && g_Cfg.m_SkillIndexDefs.valid_index(skill) )
					pChar->Skill_SetBase( skill, (ushort)atoi(pArg));
			}
			else if ( s.IsKey("ACCOUNT" ))
			{
				// What if the account does not exist ?
				pChar->SetPlayerAccount( pArg );
			}
			else if ( s.IsKey("KILLS" ) && pChar->m_pPlayer )
			{
				pChar->m_pPlayer->m_wMurders = (word)(atoi(pArg));
			}
			else if ( s.IsKey("NPCAITYPE" ))
			{
				// Convert to proper NPC type.
				int i = atoi( pArg );
				switch ( i )
				{
				case 0x01:	pChar->SetNPCBrain( NPCBRAIN_HEALER ); break;
				case 0x02:	pChar->SetNPCBrain( NPCBRAIN_MONSTER ); break;
				case 0x04:
				case 0x40:	pChar->SetNPCBrain( NPCBRAIN_GUARD ); break;
				case 0x08:	pChar->SetNPCBrain( NPCBRAIN_BANKER ); break;
				default:	pChar->SetNPCBrain( pChar->GetNPCBrainAuto() ); break;
				}
			}
			continue;
		}
	}
	return true;
}

bool CWorld::Import( lpctstr pszFilename, const CChar * pSrc, word wModeFlags, int iDist, tchar * pszArg1, tchar * pszArg2 )
{
	ADDTOCALLSTACK("CWorld::Import");
	// wModeFlags = IMPFLAGS_TYPE
	//
	// iDistance = distance from the current point.

	// dx = change in x from file to world.
	// dy = change in y

	// NOTE: We need to set the IsLoading() for this ???

	// ??? What if i want to just import into the local area ?
	size_t iLen = strlen( pszFilename );
	if ( iLen <= 4 )
		return false;
	CScript s;
	if ( ! s.Open( pszFilename ))
		return false;

	CPointMap ptCenter;
	if ( pSrc )
	{
		ptCenter = pSrc->GetTopPoint();

		if ( wModeFlags & IMPFLAGS_RELATIVE )
		{
			// dx += ptCenter.m_x;
			// dy += ptCenter.m_y;
		}
	}

	CImportFile fImport( wModeFlags, ptCenter, iDist );

	fImport.m_pszArg1 = pszArg1;
	fImport.m_pszArg2 = pszArg2;

	if ( ! strcmpi( pszFilename + (iLen - 4), ".WSC" ))
	{
		if ( ! fImport.ImportWSC(s, wModeFlags ))
			return false;
	}
	else
	{
		// This is one of our files. ".SCP"
		if ( ! fImport.ImportSCP(s, wModeFlags ))
			return false;
	}

	// now fix the contained items.
	fImport.ImportFix();

	GarbageCollection();
	return true;
}



bool CWorld::DumpAreas( CTextConsole * pSrc, lpctstr pszFilename )
{
	ADDTOCALLSTACK("CWorld::DumpAreas");
	if ( pSrc == nullptr )
		return false;

	if ( pszFilename == nullptr || *pszFilename == '\0' )
		pszFilename	= "map_all" SPHERE_SCRIPT;
	else if ( strlen( pszFilename ) <= 4 )
		return false;

	CScript s;
	if ( ! s.Open( pszFilename, OF_WRITE|OF_TEXT|OF_DEFAULTMODE ))
		return false;

	size_t iMax = g_Cfg.m_RegionDefs.size();
	for ( size_t i = 0; i < iMax; i++ )
	{
		CRegion * pRegion = g_Cfg.m_RegionDefs.at(i);
		if ( !pRegion  )
			continue;
		pRegion->r_Write( s );
	}

	s.WriteSection( "EOF" );
	return true;
}



bool CWorld::Export( lpctstr pszFilename, const CChar * pSrc, word wModeFlags, int iDist, int dx, int dy )
{
	ADDTOCALLSTACK("CWorld::Export");
	// wModeFlags = IMPFLAGS_TYPE
	// Just get the items in the local area to export.
	// dx = change in x from world to file.
	// dy = change in y

	if ( pSrc == nullptr )
		return false;

	size_t iLen = strlen( pszFilename );
	if ( iLen <= 4 )
		return false;

	CScript s;
	if ( ! s.Open( pszFilename, OF_WRITE|OF_TEXT|OF_DEFAULTMODE ))
		return false;

	if ( wModeFlags & IMPFLAGS_RELATIVE )
	{
		dx -= pSrc->GetTopPoint().m_x;
		dy -= pSrc->GetTopPoint().m_y;
	}

	if ( ! strcmpi( pszFilename + (iLen - 4), ".WSC" ))
	{
		// Export as UOX format. for world forge stuff.
		int index = 0;
		auto AreaItems = CWorldSearchHolder::GetInstance( pSrc->GetTopPoint(), iDist );
		AreaItems->SetSearchSquare(true);
		for (;;)
		{
			CItem * pItem = AreaItems->GetItem();
			if ( pItem == nullptr )
				break;
			pItem->WriteUOX( s, index++ );
		}
		return true;
	}

	// (???NPC) Chars and the stuff they are carrying.
	if ( wModeFlags & IMPFLAGS_CHARS )
	{
		auto AreaChars = CWorldSearchHolder::GetInstance( pSrc->GetTopPoint(), iDist );
		AreaChars->SetSearchSquare(true);
		AreaChars->SetAllShow( pSrc->IsPriv( PRIV_ALLSHOW ));	// show logged out chars?
		for (;;)
		{
			CChar * pChar = AreaChars->GetChar();
			if ( pChar == nullptr )
				break;
			pChar->r_WriteSafe( s );
		}
	}

	if ( wModeFlags & IMPFLAGS_ITEMS )
	{
		// Items on the ground.
		auto AreaItems = CWorldSearchHolder::GetInstance( pSrc->GetTopPoint(), iDist );
		AreaItems->SetSearchSquare(true);
		AreaItems->SetAllShow( pSrc->IsPriv( PRIV_ALLSHOW ));	// show logged out chars?
		for (;;)
		{
			CItem * pItem = AreaItems->GetItem();
			if ( pItem == nullptr )
				break;
			pItem->r_WriteSafe( s );
		}
	}

	return true;
}

