#include "../common/CDataBase.h"
#include "../common/CException.h"
#include "../common/sphereversion.h"
#include "../network/CClientIterator.h"
#include "../network/CNetworkManager.h"
#include "../sphere/ProfileTask.h"
#include "../common/CLog.h"
#include "chars/CChar.h"
#include "clients/CClient.h"
#include "clients/CGMPage.h"
#include "CServer.h"
#include "CScriptProfiler.h"
#include "CSector.h"
#include "CWorldComm.h"
#include "CWorldMap.h"
#include "CWorldTickingList.h"
#include "CWorld.h"

#ifndef _WIN32
    #include <sys/statvfs.h>
#endif
#include <sys/stat.h>


lpctstr GetReasonForGarbageCode(int iCode = -1)
{
	lpctstr pStr;
	switch ( iCode )
	{
		case -1:
		default:
			pStr = "Unknown";
			break;

		case 0x1102:
			pStr = "Disconnected char not acting as such";
			break;

		case 0x1103:
		case 0x1203:
			pStr = "Ridden NPC not acting as such";
			break;

		case 0x1104:
		case 0x1204:
			pStr = "Ridden NPC without a valid mount item";
			break;

		case 0x1106:
			pStr = "Disconnected NPC neither dead nor ridden";
			break;

		case 0x1107:
		case 0x1205:
			pStr = "In game char that is neither a player nor an NPC";
			break;

		case 0x1108:
			pStr = "Char not on a valid position";
			break;

		case 0x2102:
			pStr = "Item without ITEMDEF";
			break;

		case 0x2103:
			pStr = "Item ITEMDEF with ID = 0";
			break;

		case 0x2104:
			pStr = "Disconnected item";
			break;

		case 0x2105:
			pStr = "Item not on a valid position";
			break;

		case 0x2106:
			pStr = "Item flagged as being in a container but it isn't";
			break;

		case 0x2202:
			pStr = "Item flagged as equipped but it isn't";
			break;

		case 0x2205:
			pStr = "Mislinked item";
			break;

		case 0x2206:
			pStr = "Gm Robe / Deathshroud not on a char";
			break;

		case 0x2207:
			pStr = "Deathshroud not on a dead char";
			break;

		case 0x2208:
			pStr = "Gm Robe on a char without privilege";
			break;

		case 0x2220:
			pStr = "Trade window memory not equipped in the correct layer or equipped on disconnected char";
			break;

		case 0x2221:
			pStr = "Client linger memory not equipped in the correct layer";
			break;

		case 0x2226:
			pStr = "Mount memory not equipped in the correct layer";
			break;

		case 0x2227:
		case 0x2228:
			pStr = "Hair/Beard item not equipped / not in a corpse / not in a vendor box";
			break;

		case 0x2229:
			pStr = "Game piece not in a game board";
			break;

		case 0x2230:
			pStr = "Item equipped in the trade window layer but it isn't a trade window";
			break;

		case 0x2231:
			pStr = "Item equipped in the memory layer but it isn't a memory";
			break;

		case 0x2233:
			pStr = "Item equipped in the mount memory layer but it isn't a mount memory";
			break;

		case 0x2234:
			pStr = "Item equipped in the client linger layer but it isn't a client linger memory";
			break;

		case 0x2235:
			pStr = "Item equipped in the murder memory layer but it isn't a murder memory";
			break;

		case 0x2236:
			pStr = "Item flagged as decay but without timer set";
			break;

		case 0x3101:
			pStr = "Object is totally lost, no parent exists";
			break;

		case 0x3102:
			pStr = "Object was deleted or UID is incorrect";
			break;

		case 0x3201:
			pStr = "Object not correctly loaded by server (UID conflict)";
			break;

		case 0x3202:
			pStr = "Object not placed in the world";
			break;

		case 0x2222:
		case 0x4222:
			pStr = "Memory not equipped / not in the memory layer / without color";
			break;

		case 0x4223:
			pStr = "Memory not on a char";
			break;

		case 0x4224:
			pStr = "Stone/Guild memory mislinked";
			break;

		case 0x4225:
			pStr = "Stone/Guild memory linked to the wrong stone";
			break;
        case 0x4226:
            pStr = "Old Spawn memory item conversion";
            break;

		case 0xFFFF:
			pStr = "Bad memory allocation";
			break;
	}

	return pStr;
}

void ReportGarbageCollection(CObjBase * pObj, int iResultCode)
{
	ASSERT(pObj != nullptr);

	DEBUG_ERR(("GC: Deleted UID=0%" PRIx32 ", Defname='%s', Name='%s'. Invalid code=0x%x (%s).\n",
		(dword)pObj->GetUID(), pObj->Base_GetDef()->GetResourceName(), pObj->GetName(), iResultCode, GetReasonForGarbageCode(iResultCode)));

	if ( (pObj->_iCreatedResScriptIdx != -1) && (pObj->_iCreatedResScriptLine != -1) )
	{
		// Object was created via NEWITEM or NEWNPC in scripts, tell me where
		CResourceScript* pResFile = g_Cfg.GetResourceFile((size_t)(pObj->_iCreatedResScriptIdx));
		if (pResFile == nullptr)
			return;
		DEBUG_ERR(("GC:\t Object was created in '%s', line %d.\n", (lpctstr)pResFile->GetFilePath(), pObj->_iCreatedResScriptLine));
	}
}



//////////////////////////////////////////////////////////////////
// -CWorldThread

CWorldThread::CWorldThread()
{
	m_fSaveParity = false;		// has the sector been saved relative to the char entering it ?

	_ppUIDObjArray = nullptr;
	_uiUIDObjArraySize = 0;
	_dwUIDIndexLast = 0;

	_pdwFreeUIDs = nullptr;
	_dwFreeUIDOffset = 0;
}

CWorldThread::~CWorldThread()
{
	CloseAllUIDs();
}

void CWorldThread::InitUIDs()
{
	_uiUIDObjArraySize = 8 * 1024;
	_ppUIDObjArray = (CObjBase**)calloc(_uiUIDObjArraySize, sizeof(CObjBase*));
	_dwUIDIndexLast = 1;

	_dwFreeUIDOffset = FREE_UIDS_SIZE;
	_pdwFreeUIDs = (dword*)calloc(_dwFreeUIDOffset, sizeof(dword));
}

void CWorldThread::CloseAllUIDs()
{
	ADDTOCALLSTACK("CWorldThread::CloseAllUIDs");
	m_ObjDelete.ClearContainer();	// empty our list of unplaced objects (and delete the objects in the list)
	m_ObjSpecialDelete.ClearContainer();
	m_ObjNew.ClearContainer();		// empty our list of objects to delete (and delete the objects in the list)

	if (_ppUIDObjArray != nullptr)
	{
		free(_ppUIDObjArray);
		_ppUIDObjArray = nullptr;
		_uiUIDObjArraySize = 0;
	}

	if ( _pdwFreeUIDs != nullptr )
	{
		free(_pdwFreeUIDs);
		_pdwFreeUIDs = nullptr;
	}

	_dwFreeUIDOffset = 0;
	_dwUIDIndexLast = 0;
}

bool CWorldThread::IsSaving() const
{
	return (m_FileWorld.IsFileOpen() && m_FileWorld.IsWriteMode());
}

dword CWorldThread::GetUIDCount() const
{
	ASSERT(_uiUIDObjArraySize <= UINT32_MAX);
	return (dword)_uiUIDObjArraySize;
}

CObjBase *CWorldThread::FindUID(dword dwIndex) const
{
	if ( !dwIndex || dwIndex >= GetUIDCount() )
		return nullptr;
	if ( _ppUIDObjArray[ dwIndex ] == UID_PLACE_HOLDER )	// unusable for now. (background save is going on)
		return nullptr;
	return _ppUIDObjArray[dwIndex];
}

void CWorldThread::FreeUID(dword dwIndex)
{
	// Can't free up the UID til after the save !
	_ppUIDObjArray[dwIndex] = IsSaving() ? UID_PLACE_HOLDER : nullptr;
}

dword CWorldThread::AllocUID( dword dwIndex, CObjBase * pObj )
{
	ADDTOCALLSTACK("CWorldThread::AllocUID");
	dword dwCountTotal = GetUIDCount();

	if ( !dwIndex )					// auto-select tbe suitable hole
	{
		if ( !dwCountTotal )		// if the uids array is empty - increase it.
		{
			dwIndex = 1;
			goto setcount;
		}

		if ( _dwFreeUIDOffset < FREE_UIDS_SIZE && _pdwFreeUIDs != nullptr )
		{
			//	We do have a free uid's array. Use it if possible to determine the first free element
			for ( ; _dwFreeUIDOffset < FREE_UIDS_SIZE && _pdwFreeUIDs[_dwFreeUIDOffset] != 0; _dwFreeUIDOffset++ )
			{
				//	yes, that's a free slot
				if ( !_ppUIDObjArray[_pdwFreeUIDs[_dwFreeUIDOffset]] )
				{
					dwIndex = _pdwFreeUIDs[_dwFreeUIDOffset++];
					goto successalloc;
				}
			}
		}
		_dwFreeUIDOffset = FREE_UIDS_SIZE;	// mark array invalid, since it does not contain any empty slots
										// use default allocation for a while, till the next garbage collection
		dword dwCount = dwCountTotal - 1;
		dwIndex = _dwUIDIndexLast;
		while ( _ppUIDObjArray[dwIndex] != nullptr )
		{
			if ( ! -- dwIndex )
			{
				dwIndex = dwCountTotal - 1;
			}
			if ( ! -- dwCount )
			{
				dwIndex = dwCountTotal;
				goto setcount;
			}
		}
	}
	else if ( dwIndex >= dwCountTotal )
	{
setcount:
		// We have run out of free UID's !!! Grow the array
		const size_t uiOldArraySize = _uiUIDObjArraySize;
		_uiUIDObjArraySize = ((dwIndex + 0x1000) & ~0xFFF);

		CObjBase** pNewBlock = (CObjBase**)realloc(_ppUIDObjArray, _uiUIDObjArraySize * sizeof(CObjBase*));
		if (pNewBlock == nullptr)
		{
			throw CSError(LOGL_FATAL, 0, "Not enough memory to store new UIDs!.\n");
		}

		// zero initialize the expanded part of the memory, leave untouched the original one
		memset(pNewBlock + uiOldArraySize, 0, (_uiUIDObjArraySize - uiOldArraySize) * sizeof(CObjBase*));

		_ppUIDObjArray = (CObjBase**)pNewBlock;
	}

successalloc:
	_dwUIDIndexLast = dwIndex; // start from here next time so we have even distribution of allocation.
	CObjBase *pObjPrv = _ppUIDObjArray[dwIndex];
	if ( pObjPrv )
	{
		//NOTE: We cannot use Delete() in here because the UID will
		//	still be assigned til the async cleanup time. Delete() will not work here!
		DEBUG_ERR(("UID conflict delete 0%x, '%s'\n", dwIndex, pObjPrv->GetName()));
		delete pObjPrv;
	}
	_ppUIDObjArray[dwIndex] = pObj;
	return dwIndex;
}

void CWorldThread::SaveThreadClose()
{
	ADDTOCALLSTACK("CWorldThread::SaveThreadClose");
	for ( size_t i = 1; i < GetUIDCount(); ++i )
	{
		if ( _ppUIDObjArray[i] == UID_PLACE_HOLDER )
			_ppUIDObjArray[i] = nullptr;
	}

	m_FileData.Close();
	m_FileWorld.Close();
	m_FilePlayers.Close();
	m_FileMultis.Close();
}

int CWorldThread::FixObjTry( CObjBase * pObj, dword dwUID )
{
	ADDTOCALLSTACK_INTENSIVE("CWorldThread::FixObjTry");
	// RETURN: 0 = success.
	if ( !pObj )
		return 0x7102;

	if ( dwUID != 0 )
	{
		if (( pObj->GetUID().GetPrivateUID() & UID_O_INDEX_MASK ) != dwUID )
		{
			// Miss linked in the UID table !!! BAD
			// Hopefully it was just not linked at all. else How the hell should i clean this up ???
			DEBUG_ERR(("UID 0%x, '%s', Mislinked\n", dwUID, pObj->GetName()));
			return 0x7101;
		}
	}

	return pObj->FixWeirdness();
}

int CWorldThread::FixObj( CObjBase * pObj, dword dwUID )
{
	ADDTOCALLSTACK("CWorldThread::FixObj");
	// Attempt to fix problems with this item.
	// Ignore any children it may have for now.
	// RETURN: 0 = success.
	//

	int iResultCode;

	try
	{
		iResultCode = FixObjTry(pObj, dwUID);
	}
	catch ( const CSError& e ) // catch all
	{
		g_Log.CatchEvent( &e, "FixObj" );
		iResultCode = 0xFFFF;	// bad mem ?
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent(nullptr, "FixObj" );
		iResultCode = 0xFFFF;	// bad mem ?
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	if ( iResultCode == 0 )
		return 0;

	try
	{
		dwUID = (dword)pObj->GetUID();

		// is it a real error ?
		if ( pObj->IsItem())
		{
			CItem * pItem = dynamic_cast <CItem*>(pObj);
			if ( pItem != nullptr && pItem->IsType(IT_EQ_MEMORY_OBJ) )
			{
				pObj->Delete();
				return iResultCode;
			}
		}

		ReportGarbageCollection(pObj, iResultCode);

		if ( iResultCode == 0x1203 || iResultCode == 0x1103 )
		{
			CChar * pChar = dynamic_cast <CChar*>(pObj);
			if ( pChar )
				pChar->Skill_Start( NPCACT_RIDDEN );
		}
		else
			pObj->Delete();
	}
	catch ( const CSError& e )	// catch all
	{
		g_Log.CatchEvent( &e, "UID=0%x, Asserted cleanup", dwUID );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( nullptr, "UID=0%x, Asserted cleanup", dwUID );
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	return iResultCode;
}

void CWorldThread::GarbageCollection_NewObjs()
{
	ADDTOCALLSTACK("CWorldThread::GarbageCollection_NewObjs");
	EXC_TRY("GarbageCollection_NewObjs");
	// Clean up new objects that are never placed.
	size_t iObjCount = m_ObjNew.GetContentCount();
	if (iObjCount > 0 )
	{
		g_Log.Event(LOGL_ERROR, "GC: %" PRIuSIZE_T " unplaced objects!\n", iObjCount);

		for (size_t i = 0; i < iObjCount; ++i)
		{
			CObjBase * pObj = dynamic_cast<CObjBase*>(m_ObjNew.GetContentIndex(i));
			if (pObj == nullptr)
				continue;

			ReportGarbageCollection(pObj, 0x3202);
		}
		m_ObjNew.ClearContainer();	// empty our list of unplaced objects (and delete the objects in the list)
	}
	m_ObjDelete.ClearContainer();	// empty our list of objects to delete (and delete the objects in the list)
	m_ObjSpecialDelete.ClearContainer();


	// Clean up GM pages not linked to an valid char/account
	CGMPage* pGMPageNext = nullptr;
	for (CGMPage* pGMPage = static_cast<CGMPage*>(g_World.m_GMPages.GetContainerHead()); pGMPage != nullptr; pGMPage = pGMPageNext)
	{
		pGMPageNext = pGMPage->GetNext();
		
		if (!pGMPage->m_uidChar.CharFind())
		{
			DEBUG_ERR(("GC: Deleted GM Page linked to invalid char (UID=0%x)\n", (dword)(pGMPage->m_uidChar)));
			delete pGMPage;
		}
		else if (!g_Accounts.Account_Find(pGMPage->m_sAccount))
		{
			DEBUG_ERR(("GC: Deleted GM Page linked to invalid account '%s'\n", pGMPage->GetName()));
			delete pGMPage;
		}
	}
	EXC_CATCH;
}

void CWorldThread::GarbageCollection_UIDs()
{
	ADDTOCALLSTACK("CWorldThread::GarbageCollection_UIDs");
	// Go through the m_ppUIDs looking for Objects without links to reality.
	// This can take a while.

	GarbageCollection_NewObjs();

	dword iCount = 0;
	for (dword i = 1; i < GetUIDCount(); ++i )
	{
		try
		{
			CObjBase * pObj = _ppUIDObjArray[i];
			if ( !pObj || pObj == UID_PLACE_HOLDER )
				continue;

			// Look for anomalies and fix them (that might mean delete it.)
			int iResultCode = FixObj(pObj, i);
			if ( iResultCode )
			{
				// FixObj directly calls Delete method
				//if (pObj->IsBeingDeleted() || pObj->IsDeleted())
				//{
					// Do an immediate delete here instead of Delete()
					delete pObj;
					FreeUID(i);	// Get rid of junk uid if all fails..
				//}
				/*
				else
				{
					pObj->Delete();
				}
				*/
				continue;
			}

			if ((iCount & 0x1FF ) == 0)
				g_Serv.PrintPercent(iCount, GetUIDCount());
			++iCount;
		}
		catch ( const CSError& e )
		{
			g_Log.CatchEvent(&e, "GarbageCollection_UIDs");
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch (...)
		{
			g_Log.CatchEvent(nullptr, "GarbageCollection_UIDs");
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
	}

	GarbageCollection_NewObjs();

	if ( iCount != CObjBase::sm_iCount )	// All objects must be accounted for.
		g_Log.Event(LOGL_ERROR|LOGM_NOCONTEXT, "Garbage Collection: done. Object memory leak %" PRIu32 "!=%" PRIu32 ".\n", iCount, CObjBase::sm_iCount);
	else
		g_Log.Event(LOGL_EVENT|LOGM_NOCONTEXT, "Garbage Collection: done. %" PRIu32 " Objects accounted for.\n", iCount);

	if ( _pdwFreeUIDs != nullptr )	// new UID engine - search for empty holes and store it in a huge array
	{							// the size of the array should be enough even for huge shards
								// to survive till next garbage collection
		memset(_pdwFreeUIDs, 0, FREE_UIDS_SIZE * sizeof(dword));
		_dwFreeUIDOffset = 0;

		for ( dword d = 1; d < GetUIDCount(); ++d )
		{
			CObjBase *pObj = _ppUIDObjArray[d];

			if ( !pObj )
			{
				if ( _dwFreeUIDOffset >= ( FREE_UIDS_SIZE - 1 ))
					break;

				_pdwFreeUIDs[_dwFreeUIDOffset++] = d;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
// -CWorld

CWorld::CWorld() :
	_Ticker(&_GameClock)
{
	_iTimeLastWorldSave = 0;
	m_ticksWithoutMySQL = 0;
	m_iSaveCountID = 0;
	_iSaveStage = 0;
	_iSaveTimer = 0;
	m_iPrevBuild = 0;
	m_iLoadVersion = 0;
	_fSaveNotificationSent = false;
	_iTimeLastDeadRespawn = 0;
	_iTimeStartup = 0;
	_iTimeLastCallUserFunc = 0;
}

void CWorld::Init()
{
	EXC_TRY("Init");

	// Initialize map planes
	g_MapList.Init();

	// Initialize sectors
	_Sectors.Init();

	EXC_CATCH;
}

CWorld::~CWorld()
{
	EXC_TRY("Destructor");

	Close();

	EXC_CATCH;
}

///////////////////////////////////////////////
// Loading and Saving.

void CWorld::GetBackupName( CSString & sArchive, lpctstr pszBaseDir, tchar chType, int iSaveCount ) // static
{
	ADDTOCALLSTACK("CWorld::GetBackupName");
	int iCount = iSaveCount;
	int iGroup = 0;
	for ( ; iGroup < g_Cfg.m_iSaveBackupLevels; ++iGroup )
	{
		if ( iCount & 0x7 )
			break;
		iCount >>= 3;
	}
	sArchive.Format( "%s" SPHERE_FILE "b%d%d%c%s",
		pszBaseDir,
		iGroup, iCount&0x07,
		chType,
		SPHERE_SCRIPT );
}

bool CWorld::OpenScriptBackup( CScript & s, lpctstr pszBaseDir, lpctstr pszBaseName, int iSaveCount ) // static
{
	ADDTOCALLSTACK("CWorld::OpenScriptBackup");
	ASSERT(pszBaseName);

	CSString sArchive;
	GetBackupName( sArchive, pszBaseDir, pszBaseName[0], iSaveCount );

	// remove possible previous archive of same name
	::remove( sArchive );

	// rename previous save to archive name.
	CSString sSaveName;
	sSaveName.Format( "%s" SPHERE_FILE "%s%s", pszBaseDir, pszBaseName, SPHERE_SCRIPT );
    if ( iSaveCount > 0 )
    {
        if ( ::rename(sSaveName, sArchive) )
        {
            g_Log.Event(LOGM_SAVE|LOGL_WARN, "Rename %s to '%s' FAILED code %d?\n", static_cast<lpctstr>(sSaveName), static_cast<lpctstr>(sArchive), CSFile::GetLastError() );
        }
    }
	if ( ! s.Open( sSaveName, OF_WRITE|OF_TEXT|OF_DEFAULTMODE ))
	{
		g_Log.Event(LOGM_SAVE|LOGL_CRIT, "Save '%s' FAILED\n", static_cast<lpctstr>(sSaveName));
		return false;
	}

	return true;
}

void CWorld::SyncGameTime() noexcept
{
	// Restore the GameWorld Clock internal System Clock
	_GameClock._iSysClock_Prev = CWorldClock::GetSystemClock();
}

bool CWorld::SaveStage() // Save world state in stages.
{
	ADDTOCALLSTACK("CWorld::SaveStage");
	// Do the next stage of the save.
	// RETURN: true = continue; false = done.

	const int iSectorsQty = _Sectors.GetSectorAbsoluteQty();

	EXC_TRY("SaveStage");
	bool fRc = true;

	if ( _iSaveStage == -1 )
	{
		if ( !g_Cfg.m_fSaveGarbageCollect )
			GarbageCollection_NewObjs();
	}
	else if ( _iSaveStage < iSectorsQty)
	{
		// NPC Chars in the world secors and the stuff they are carrying.
		// Sector lighting info.
		if(IsSetEF(EF_Dynamic_Backsave))
		{
			size_t szComplexity = 0;

			CSector *s = _Sectors.GetSectorAbsolute(_iSaveStage);
			if (s)
			{
				s->r_Write();
				szComplexity += ( s->GetCharComplexity() + s->GetInactiveChars())*100 + s->GetItemComplexity();
			}

			int dynStage = _iSaveStage + 1;
			if (szComplexity <= g_Cfg.m_iSaveStepMaxComplexity)
			{
				uint szSectorCnt = 1;
				while(dynStage < iSectorsQty && szSectorCnt <= g_Cfg.m_iSaveSectorsPerTick)
				{
					s = _Sectors.GetSectorAbsolute(dynStage);
					if ( s )
					{
						szComplexity += ( s->GetCharComplexity() + s->GetInactiveChars())*100 + s->GetItemComplexity();

						if(szComplexity <= g_Cfg.m_iSaveStepMaxComplexity)
						{
							s->r_Write();
							_iSaveStage = dynStage;
							++szSectorCnt;
						}
						else
						{
							break;
						}
					}
					++dynStage;
				}
			}
		}
		else
		{
			CSector* s = _Sectors.GetSectorAbsolute(_iSaveStage);
			if (s)
			{
				s->r_Write();
			}
		}
	}
	else if (_iSaveStage == iSectorsQty)
	{
		m_FileData.WriteSection( "TIMERF" );
		_Ticker._TimedFunctions.r_Write(m_FileData);

		m_FileData.WriteSection("GLOBALS");
		g_Exp.m_VarGlobals.r_WritePrefix(m_FileData, nullptr);

		g_Exp.m_ListGlobals.r_WriteSave(m_FileData);

		const size_t iQty = g_Cfg.m_RegionDefs.size();
		for ( size_t i = 0; i < iQty; ++i )
		{
			CRegion *pRegion = dynamic_cast <CRegion*> (g_Cfg.m_RegionDefs[i]);
			if ( !pRegion || !pRegion->HasResourceName() || !pRegion->m_iModified )
				continue;

			m_FileData.WriteSection("WORLDSCRIPT %s", pRegion->GetResourceName());
			pRegion->r_WriteModified(m_FileData);
		}

		// GM_Pages.
		for (CGMPage* pGMPage = static_cast<CGMPage*>(m_GMPages.GetContainerHead()); pGMPage != nullptr; pGMPage = pGMPage->GetNext())
		{
			pGMPage->r_Write(m_FileData);
		}
	}
	else if ( _iSaveStage == iSectorsQty +1 )
	{
		//	Empty save stage
	}
	else if ( _iSaveStage == iSectorsQty +2 )
	{
		// Now make a backup of the account file.
		fRc = g_Accounts.Account_SaveAll();
	}
	else if ( _iSaveStage == iSectorsQty +3 )
	{
		// EOF marker to show we reached the end.
		m_FileData.WriteSection("EOF");
		m_FileWorld.WriteSection("EOF");
		m_FilePlayers.WriteSection("EOF");
		m_FileMultis.WriteSection("EOF");

		++m_iSaveCountID;	// Save only counts if we get to the end winout trapping.
		_iTimeLastWorldSave = _GameClock.GetCurrentTime().GetTimeRaw() + g_Cfg.m_iSavePeriod;	// next save time.

		g_Log.Event(LOGM_SAVE, "World data saved   (%s).\n", m_FileWorld.GetFilePath());
		g_Log.Event(LOGM_SAVE, "Player data saved  (%s).\n", m_FilePlayers.GetFilePath());
		g_Log.Event(LOGM_SAVE, "Multi data saved   (%s).\n", m_FileMultis.GetFilePath());
		g_Log.Event(LOGM_SAVE, "Context data saved (%s).\n", m_FileData.GetFilePath());

		llong	llTicksEnd;
		llong	llTicksStart = _iSaveTimer;
		TIME_PROFILE_END;

		tchar * time = Str_GetTemp();
		sprintf(time, "%lld.%04lld", TIME_PROFILE_GET_HI, TIME_PROFILE_GET_LO);

		g_Log.Event(LOGM_SAVE, "World save completed, took %s seconds.\n", time);

		CScriptTriggerArgs Args;
		Args.Init(time);
		g_Serv.r_Call("f_onserver_save_finished", &g_Serv, &Args);

		// Now clean up all the held over UIDs
		SaveThreadClose();

		// Mark the end of the save (background or not).
		_iSaveStage = INT32_MAX;

		SyncGameTime();

		return false;
	}

	if ( g_Cfg.m_iSaveBackgroundTime )
	{
		ASSERT(iSectorsQty > 0);
		int64 iNextTime = g_Cfg.m_iSaveBackgroundTime / iSectorsQty;
		if ( iNextTime > MSECS_PER_SEC * 30 * 60 )
			iNextTime = MSECS_PER_SEC  * 30 * 60;	// max out at 30 minutes or so.
		_iTimeLastWorldSave = _GameClock.GetCurrentTime().GetTimeRaw() + iNextTime;
	}
	++_iSaveStage;
	return fRc;

	EXC_CATCH;

	SyncGameTime();

	EXC_DEBUG_START;
	g_Log.EventDebug("stage '%d' qty '%d' time '%" PRId64 "'\n", _iSaveStage, iSectorsQty, _iTimeLastWorldSave);
	EXC_DEBUG_END;

	++_iSaveStage;	// to avoid loops, we need to skip the current operation in world save
	return false;
}

bool CWorld::SaveForce() // Save world state
{
	ADDTOCALLSTACK("CWorld::SaveForce");
	CWorldComm::Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_SERVER_WORLDSAVE ) );
	if (g_NetworkManager.isOutputThreaded() == false)
		g_NetworkManager.flushAllClients();

	g_Serv.SetServerMode(SERVMODE_Saving);	// Forced save freezes the system.
	bool fSave = true;
	bool fSuccess = true;

	static lpctstr constexpr save_msgs[] =
	{
		"garbage collection",
		"sectors",
		"global variables, regions, gmpages",
		"servers",
		"accounts",
		""
	};
	const char *pCurBlock = save_msgs[0];

	const int iSectorsQty = _Sectors.GetSectorAbsoluteQty();
	while ( fSave )
	{
		try
		{
			if (( _iSaveStage >= 0 ) && ( _iSaveStage < iSectorsQty))
				pCurBlock = save_msgs[1];
			else if ( _iSaveStage == iSectorsQty)
				pCurBlock = save_msgs[2];
			else if ( _iSaveStage == iSectorsQty +1 )
				pCurBlock = save_msgs[3];
			else if ( _iSaveStage == iSectorsQty +2 )
				pCurBlock = save_msgs[4];
			else
				pCurBlock = save_msgs[5];

			fSave = SaveStage();
			if ( !(_iSaveStage & 0x1FF) )
			{
				g_Serv.PrintPercent( _iSaveStage, (ssize_t)iSectorsQty + 3 );
			}
			if ( !fSave && (pCurBlock != save_msgs[5]) )
				goto failedstage;
		}
		catch ( const CSError& e )
		{
			g_Log.CatchEvent(&e, "Save FAILED for stage %u (%s).", _iSaveStage, pCurBlock);
			fSuccess = false;
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch (...)
		{
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
			goto failedstage;
		}
		continue;
failedstage:
		g_Log.CatchEvent( nullptr, "Save FAILED for stage %u (%s).", _iSaveStage, pCurBlock);
		fSuccess = false;
	}

	g_Serv.SetServerMode(SERVMODE_Run);			// Game is up and running
	return fSuccess;
}

bool CWorld::SaveTry( bool fForceImmediate ) // Save world state
{
	ADDTOCALLSTACK("CWorld::SaveTry");
	EXC_TRY("SaveTry");

	if ( m_FileWorld.IsFileOpen() )
	{
		// Save is already active !
		ASSERT( IsSaving() );
		if ( fForceImmediate )	// finish it now !
			return SaveForce();
		else if ( g_Cfg.m_iSaveBackgroundTime )
			return SaveStage();
		return false;
	}

	// Start a new save.

	// Do the write async from here in the future.
	if ( g_Cfg.m_fSaveGarbageCollect )
		GarbageCollection();

	// Keep track of the world save length
	llong llTicksStart;
	TIME_PROFILE_START;
	_iSaveTimer = llTicksStart;

	// Determine the save name based on the time.
	// exponentially degrade the saves over time.
	if ( ! OpenScriptBackup( m_FileData, g_Cfg.m_sWorldBaseDir, "data", m_iSaveCountID ))
		return false;

	if ( ! OpenScriptBackup( m_FileWorld, g_Cfg.m_sWorldBaseDir, "world", m_iSaveCountID ))
		return false;

	if ( ! OpenScriptBackup( m_FilePlayers, g_Cfg.m_sWorldBaseDir, "chars", m_iSaveCountID ))
		return false;

	if ( ! OpenScriptBackup( m_FileMultis, g_Cfg.m_sWorldBaseDir, "multis", m_iSaveCountID ))
		return false;

	// Flip the parity of the save.... TODO: explain this a little better...
	m_fSaveParity = ! m_fSaveParity;

	// Init
	_iSaveStage = -1;
	_fSaveNotificationSent = false;
	_iTimeLastWorldSave = 0;

	// Write the file headers.
	r_Write(m_FileData);
	r_Write(m_FileWorld);
	r_Write(m_FilePlayers);
	r_Write(m_FileMultis);

	if ( fForceImmediate || ! g_Cfg.m_iSaveBackgroundTime )	// Save now !
		return SaveForce();

	return true;
	EXC_CATCH;

	SyncGameTime();

	EXC_DEBUG_START;
	g_Log.EventDebug("Immediate '%d'\n", fForceImmediate? 1 : 0);
	EXC_DEBUG_END;
	return false;
}

bool CWorld::CheckAvailableSpaceForSave(bool fStatics)
{
    //-- Do i have enough disk space to write the save file?

    // Get the available disk space
    ullong uiFreeSpace;     // in bytes
    lpctstr ptcSaveDir = g_Cfg.m_sWorldBaseDir.GetBuffer();
#ifndef _WIN32
    struct statvfs stvfs;
    if (statvfs(ptcSaveDir, &stvfs) != 0)
    {
        throw CSError(LOGL_CRIT, 0, "Can't statvfs the \"save\" folder! Save aborted!");
    }
    uiFreeSpace = stvfs.f_bsize * stvfs.f_bavail;
#else
    ULARGE_INTEGER liFreeBytesAvailable, liTotalNumberOfBytes, liTotalNumberOfFreeBytes;
    if (! ::GetDiskFreeSpaceEx(ptcSaveDir,  // directory name
        &liFreeBytesAvailable,              // bytes available to caller
        &liTotalNumberOfBytes,              // bytes on disk
        &liTotalNumberOfFreeBytes))         // free bytes on disk
    {
        throw CSError(LOGL_CRIT, 0, "GetDiskFreeSpaceEx failed for the \"save\" folder! Save aborted!");
    }
    uiFreeSpace = (ullong)liTotalNumberOfFreeBytes.QuadPart;
#endif
    uiFreeSpace /= 1024;    // from bytes to kilobytes (or to be more precise, kibibytes)

    // Calculate the previous save file size
    bool fSizeErr = false;
    ullong uiPreviousSaveSize = 0;
    auto CalcPrevSavesSize = [=, &fSizeErr, &uiPreviousSaveSize](lpctstr ptcSaveName) -> void
    {
        struct stat st;
        CSString strSaveFile = g_Cfg.m_sWorldBaseDir + SPHERE_FILE + ptcSaveName + SPHERE_SCRIPT;
		if (!stat(strSaveFile.GetBuffer(), &st))
		{
			const ullong uiCurSavefileSize = (ullong)st.st_size;
			if (uiCurSavefileSize == 0)
				fSizeErr = true;
			else
				uiPreviousSaveSize += uiCurSavefileSize;
		}
		else
			fSizeErr = true;        
    };

    if (fStatics)
    {
        CalcPrevSavesSize("statics");
    }
    else
    {
        CalcPrevSavesSize("data");
        CalcPrevSavesSize("world");
        CalcPrevSavesSize("chars");
        CalcPrevSavesSize("multis");
    }

    uiPreviousSaveSize /= 1024;
    uiPreviousSaveSize += (uiPreviousSaveSize*20)/100;  // Just to be sure increase the space requirement by 20%
    ullong uiServMem = (ullong)g_Serv.StatGet(SERV_STAT_MEM);   // In KB
    if (fSizeErr)
    {
        // In case we have corrupted or unexistant previous save files, check for this arbitrary amount of free space.
        uiPreviousSaveSize = maximum(uiPreviousSaveSize, (uiServMem/8));
    }
    else
    {
        // Work around suspiciously low previous save files sizes
        uiPreviousSaveSize = maximum(uiPreviousSaveSize, (uiServMem/20));
    }

    if (uiFreeSpace < uiPreviousSaveSize)
    {
        g_Log.Event(LOGL_CRIT, "-----------------------------");
        g_Log.Event(LOGL_CRIT, "Save ABORTED! Low disk space!");
        g_Log.Event(LOGL_CRIT, "-----------------------------");
		CWorldComm::Broadcast("Save ABORTED! Warn the administrator!");
        return false;
    }
    return true;
}

bool CWorld::Save( bool fForceImmediate ) // Save world state
{
	ADDTOCALLSTACK("CWorld::Save");

	bool fSaved = false;
	try
	{
		if (!CheckAvailableSpaceForSave(false))
			return false;

		//-- Ok we can start the save process, in which we eventually remove the previous saves and create the other.

		CScriptTriggerArgs Args(fForceImmediate, _iSaveStage);
		enum TRIGRET_TYPE tr;

		if ( g_Serv.r_Call("f_onserver_save", &g_Serv, &Args, nullptr, &tr) )
			if ( tr == TRIGRET_RET_TRUE )
				return false;
		//Flushing before the server should fix #2306
		//The scripts fills the clients buffer and the server flush
		//the data during the save.
		//Should we flush only non threaded output or force it
		//to flush on any conditions?

		if (g_NetworkManager.isOutputThreaded() == false)
        {
#ifdef _DEBUG
			g_Log.EventDebug("Flushing %" PRIuSIZE_T " client(s) output data...\n", g_Serv.StatGet(SERV_STAT_CLIENTS));
#endif
			g_NetworkManager.flushAllClients();
#ifdef _DEBUG
			g_Log.EventDebug("Done flushing clients output data.\n");
#endif
		}

		fForceImmediate = (Args.m_iN1 != 0);
		fSaved = SaveTry(fForceImmediate);
	}
	catch ( const CSError& e )
	{
		g_Log.CatchEvent( &e, "Save FAILED." );
		CWorldComm::Broadcast("Save FAILED. " SPHERE_TITLE " is UNSTABLE!");
		m_FileData.Close();		// close if not already closed.
		m_FileWorld.Close();	// close if not already closed.
		m_FilePlayers.Close();	// close if not already closed.
		m_FileMultis.Close();	// close if not already closed.
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( nullptr, "Save FAILED" );
		CWorldComm::Broadcast("Save FAILED. " SPHERE_TITLE " is UNSTABLE!");
		m_FileData.Close();		// close if not already closed.
		m_FileWorld.Close();	// close if not already closed.
		m_FilePlayers.Close();	// close if not already closed.
		m_FileMultis.Close();	// close if not already closed.
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	CScriptTriggerArgs Args(fForceImmediate, _iSaveStage);
	g_Serv.r_Call((fSaved ? "f_onserver_save_ok" : "f_onserver_save_fail"), &g_Serv, &Args);
	return fSaved;
}

void CWorld::SaveStatics()
{
	ADDTOCALLSTACK("CWorld::SaveStatics");

    if (!CheckAvailableSpaceForSave(true))
        return;

	try
	{
		if ( !g_Cfg.m_fSaveGarbageCollect )
			GarbageCollection_NewObjs();

		CScript m_FileStatics;
		if ( !OpenScriptBackup(m_FileStatics, g_Cfg.m_sWorldBaseDir, "statics", m_iSaveCountID) )
			return;
		r_Write(m_FileStatics);

		CWorldComm::Broadcast( g_Cfg.GetDefaultMsg(DEFMSG_SERVER_WORLDSTATICSAVE) );

		if (g_NetworkManager.isOutputThreaded() == false)
			g_NetworkManager.flushAllClients();

		//	loop through all sectors and save static items
		for ( int m = 0; m < MAP_SUPPORTED_QTY; ++m )
		{
			if ( !g_MapList.IsMapSupported(m) )
                continue;

			for (int s = 0, qty = _Sectors.GetSectorQty(m); s < qty; ++s)
			{
				CSector* pSector = _Sectors.GetSector(m, s);
				if ( !pSector )
                    continue;

				for (CSObjContRec* pObjRec : pSector->m_Items)
				{
					CItem* pItem = static_cast<CItem*>(pObjRec);
                    if (pItem->IsTypeMulti())
						continue;
					if ( !pItem->IsAttr(ATTR_STATIC) )
						continue;

					// No try/catch, this method has its own security measures
					pItem->r_WriteSafe(m_FileStatics);
				}
			}
		}

		m_FileStatics.WriteSection( "EOF" );
		m_FileStatics.Close();
		g_Log.Event(LOGM_SAVE, "Statics data saved (%s).\n", static_cast<lpctstr>(m_FileStatics.GetFilePath()));
	}
	catch (const CSError& e)
	{
		g_Log.CatchEvent(&e, "Statics Save FAILED.");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)
	{
		g_Log.CatchEvent(nullptr, "Statics Save FAILED.");
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	SyncGameTime();
}

/////////////////////////////////////////////////////////////////////

bool CWorld::LoadFile( lpctstr pszLoadName, bool fError ) // Load world from script
{
    ADDTOCALLSTACK("CWorld::LoadFile");
    EXC_TRY("LoadFile");
    g_Log.Event(LOGM_INIT, "Loading %s...\n", pszLoadName);
    CScript s;
	if ( ! s.Open( pszLoadName, OF_READ|OF_TEXT|OF_DEFAULTMODE ) )  // don't cache this script
	{
		if ( fError )
			g_Log.Event(LOGM_INIT|LOGL_ERROR, "Can't Load %s\n", pszLoadName);
		else
			g_Log.Event(LOGM_INIT|LOGL_WARN, "Can't Load %s\n", pszLoadName);
		return false;
	}

	// Find the size of the file.
	const int iLoadSize = s.GetLength();
    int iLoadStage = 0;

	CScriptFileContext ScriptContext( &s );

	// Read the header stuff first.
	CScriptObj::r_Load( s );

	while ( s.FindNextSection() )
	{
		if (! ( ++iLoadStage & 0x1FF ))	// don't update too often
			g_Serv.PrintPercent( s.GetPosition(), iLoadSize );

		try
		{
			g_Cfg.LoadResourceSection(&s);
		}
		catch ( const CSError& e )
		{
			g_Log.CatchEvent(&e, "Load Exception line %d " SPHERE_TITLE " is UNSTABLE!\n", s.GetContext().m_iLineNum);
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch (...)
		{
			g_Log.CatchEvent(nullptr, "Load Exception line %d " SPHERE_TITLE " is UNSTABLE!\n", s.GetContext().m_iLineNum);
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
	}

	if ( s.IsSectionType( "EOF" ) )
	{
		// The only valid way to end.
		s.Close();
		return true;
	}

    s.Close();
	g_Log.Event( LOGM_INIT|LOGL_CRIT, "No [EOF] marker. '%s' is corrupt!\n", s.GetFilePath());
    EXC_CATCH;
    return false;
}


bool CWorld::LoadWorld() // Load world from script
{
    ADDTOCALLSTACK("CWorld::LoadWorld");
	EXC_TRY("LoadWorld");
	// Auto change to the most recent previous backup !
	// Try to load a backup file instead ?
	// NOTE: WE MUST Sync these files ! CHAR and WORLD !!!

	CSString sStaticsName;
	sStaticsName.Format("%s" SPHERE_FILE "statics" SPHERE_SCRIPT, static_cast<lpctstr>(g_Cfg.m_sWorldBaseDir));

	CSString sWorldName;
	sWorldName.Format("%s" SPHERE_FILE "world" SPHERE_SCRIPT, static_cast<lpctstr>(g_Cfg.m_sWorldBaseDir));

	CSString sMultisName;
	sMultisName.Format("%s" SPHERE_FILE "multis" SPHERE_SCRIPT, static_cast<lpctstr>(g_Cfg.m_sWorldBaseDir));

	CSString sCharsName;
	sCharsName.Format("%s" SPHERE_FILE "chars" SPHERE_SCRIPT, static_cast<lpctstr>(g_Cfg.m_sWorldBaseDir));

	CSString sDataName;
	sDataName.Format("%s" SPHERE_FILE "data" SPHERE_SCRIPT,	static_cast<lpctstr>(g_Cfg.m_sWorldBaseDir));

	int iPrevSaveCount = m_iSaveCountID;
	for (;;)
	{
		InitUIDs();

		LoadFile(sDataName, false);
		LoadFile(sStaticsName, false);
		if ( LoadFile(sWorldName, false) && LoadFile(sCharsName, false) && LoadFile(sMultisName, false))
		{
		    return true;
		}

		// If we could not open the file at all then it was a bust!
		if ( m_iSaveCountID == iPrevSaveCount )
            break;

		// Reset everything that has been loaded
		CWorldTickingList::ClearTickingLists();

		m_Stones.clear();
		m_Parties.ClearContainer();
		m_GMPages.ClearContainer();

		_Sectors.Close();
		CloseAllUIDs();
		_GameClock.Init();


		// Get the name of the previous backups.
		CSString sArchive;
		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'w', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sWorldName ))	// ! same file ? break endless loop.
			break;
		sWorldName = sArchive;

		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'c', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sCharsName ))	// ! same file ? break endless loop.
			break;
		sCharsName = sArchive;

		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'm', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sMultisName ))
			break;
		sMultisName = sArchive;

		GetBackupName( sArchive, g_Cfg.m_sWorldBaseDir, 'd', m_iSaveCountID );
		if ( ! sArchive.CompareNoCase( sDataName ))	// ! same file ? break endless loop.
			break;
		sDataName = sArchive;
	}

	g_Log.Event(LOGL_WARN | LOGM_INIT, "No previous backup available ?\n");
    if ( !Save(true) )
        g_Log.Event(LOGL_FATAL | LOGM_INIT, "No save found unable to create new one.\n");
    else
        return true;
    EXC_CATCH;
    return false;
}


bool CWorld::LoadAll() // Load world from script
{
	// start count. (will grow as needed)
	_GameClock.Init();		// will be loaded from the world file.

	// Load all the accounts.
	if ( !g_Accounts.Account_LoadAll(false) )
		return false;

	// Try to load the world and chars files .
	if ( !LoadWorld() )
		return false;

	_iTimeStartup = _GameClock.GetCurrentTime().GetTimeRaw();
	_iTimeLastWorldSave = _GameClock.GetCurrentTime().GetTimeRaw() + g_Cfg.m_iSavePeriod;	// next save time.

	// Set all the sector light levels now that we know the time.
	// This should not look like part of the load. (CTRIG_EnvironChange triggers should run)
	
	for (int m = 0; m < MAP_SUPPORTED_QTY; ++m)
	{
		if (!g_MapList.IsMapSupported(m))
			continue;

		for (int s = 0, qty = _Sectors.GetSectorQty(m); s < qty; ++s)
		{
			EXC_TRYSUB("Load");

			CSector* pSector = _Sectors.GetSector(m, s);
			ASSERT(pSector);

            if (!pSector->IsLightOverriden())
                pSector->SetLight(-1);

            // Is this area too complex ?
            pSector->CheckItemComplexity();
            pSector->CheckCharComplexity();

			EXC_CATCHSUB("Sector light levels");
		}
	}

	EXC_TRYSUB("Load");
	GarbageCollection();
	EXC_CATCHSUB("Garbage collect");

	// Set the current version now.
	r_SetVal("VERSION", SPHERE_VER_ID_STR);	// Set m_iLoadVersion

	return true;
}

/////////////////////////////////////////////////////////////////

void CWorld::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CWorld::r_Write");
	// Write out the safe header.
	s.WriteKeyStr("TITLE", SPHERE_TITLE " World Script");
	s.WriteKeyStr("VERSION", SPHERE_VER_ID_STR);
	#ifdef __GITREVISION__
		s.WriteKeyVal("PREVBUILD", __GITREVISION__);
	#endif
	s.WriteKeyVal( "TIMEHIRES", _GameClock.GetCurrentTime().GetTimeRaw() );
	s.WriteKeyVal( "SAVECOUNT", m_iSaveCountID );
	s.Flush();	// Force this out to the file now.
}

bool CWorld::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CWorld::r_GetRef");
	if ( ! strnicmp( ptcKey, "LASTNEW", 7 ))
	{
		if ( ! strnicmp( ptcKey+7, "ITEM", 4 ) )
		{
			ptcKey += 11;
			SKIP_SEPARATORS(ptcKey);
			pRef = m_uidLastNewItem.ItemFind();
			return true;
		}
		else if ( ! strnicmp( ptcKey+7, "CHAR", 4 ) )
		{
			ptcKey += 11;
			SKIP_SEPARATORS(ptcKey);
			pRef = m_uidLastNewChar.CharFind();
			return true;
		}
	}
	return false;
}

enum WC_TYPE
{
    WC_CURTICK,
	WC_PREVBUILD,
	WC_SAVECOUNT,
	WC_TIME,
    WC_TIMEHIRES,
	WC_TITLE,
	WC_VERSION,
	WC_QTY
};

lpctstr const CWorld::sm_szLoadKeys[WC_QTY+1] =	// static
{
    "CURTICK",
	"PREVBUILD",
	"SAVECOUNT",
	"TIME",
    "TIMEHIRES",
	"TITLE",
	"VERSION",
	nullptr
};

bool CWorld::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
    UNREFERENCED_PARAMETER(fNoCallParent);
    UNREFERENCED_PARAMETER(fNoCallChildren);
	ADDTOCALLSTACK("CWorld::r_WriteVal");
	EXC_TRY("WriteVal");

	switch ( FindTableSorted( ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 ))
	{
        case WC_CURTICK:
            sVal.Format64Val(_GameClock.GetCurrentTick());
            break;
		case WC_PREVBUILD:
			sVal.FormatVal(m_iPrevBuild);
			break;
		case WC_SAVECOUNT:  // "SAVECOUNT"
			sVal.FormatVal( m_iSaveCountID );
			break;
		case WC_TIME:	    // "TIME"
			sVal.FormatLLVal(_GameClock.GetCurrentTime().GetTimeRaw() / MSECS_PER_TENTH);  // in tenths of second, for backwards compatibility
			break;
        case WC_TIMEHIRES:	// "TIMEHIRES"
            sVal.FormatLLVal(_GameClock.GetCurrentTime().GetTimeRaw() );      // in milliseconds
            break;
		case WC_TITLE:      // "TITLE",
			sVal = (SPHERE_TITLE " World Script");
			break;
		case WC_VERSION:    // "VERSION"
			sVal = SPHERE_VER_ID_STR;
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

bool CWorld::r_LoadVal( CScript &s )
{
	ADDTOCALLSTACK("CWorld::r_LoadVal");
	EXC_TRY("LoadVal");

	lpctstr	ptcKey = s.GetKey();
	switch ( FindTableSorted( ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 ))
	{
		case WC_PREVBUILD:
			m_iPrevBuild = s.GetArgVal();
			break;
		case WC_SAVECOUNT: // "SAVECOUNT"
			m_iSaveCountID = s.GetArgVal();
			break;
		case WC_TIME:	// "TIME"
            if (!g_Serv.IsLoading())
            {
                g_Log.EventError("Can't set TIME while server is running.\n");
                return false;
            }
			_GameClock.InitTime( s.GetArgLLVal() * MSECS_PER_SEC);
			break;
        case WC_TIMEHIRES:	// "TIMEHIRES"
            if (!g_Serv.IsLoading())
            {
                g_Log.EventError("Can't set TIMEHIRES while server is running.\n");
                return false;
            }
            _GameClock.InitTime(s.GetArgLLVal());
            break;
		case WC_VERSION: // "VERSION"
			m_iLoadVersion = s.GetArgVal();
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

void CWorld::RespawnDeadNPCs()
{
	ADDTOCALLSTACK("CWorld::RespawnDeadNPCs");
	// Respawn dead story NPC's
	g_Serv.SetServerMode(SERVMODE_RestockAll);
	for ( int m = 0; m < MAP_SUPPORTED_QTY; ++m )
	{
		if ( !g_MapList.IsMapSupported(m) )
            continue;

		for (int s = 0, qty = _Sectors.GetSectorQty(m); s < qty; ++s)
		{
			EXC_TRY("OnSector");

			CSector* pSector = _Sectors.GetSector(m, s);
			ASSERT(pSector);
			pSector->RespawnDeadNPCs();

			EXC_CATCH;
		}
	}
	g_Serv.SetServerMode(SERVMODE_Run);
}

void CWorld::Restock()
{
	ADDTOCALLSTACK("CWorld::Restock");
	// Recalc all the base items as well.
	g_Log.Event(LOGL_EVENT, "World Restock: started.\n");
	g_Serv.SetServerMode(SERVMODE_RestockAll);

	for ( size_t i = 0; i < CountOf(g_Cfg.m_ResHash.m_Array); ++i )
	{
		for ( size_t j = 0; j < g_Cfg.m_ResHash.m_Array[i].size(); ++j )
		{
			CResourceDef * pResDef = g_Cfg.m_ResHash.m_Array[i][j];
			if ( pResDef == nullptr || ( pResDef->GetResType() != RES_ITEMDEF ))
				continue;

			CItemBase * pBase = dynamic_cast<CItemBase *>(pResDef);
			if ( pBase != nullptr )
				pBase->Restock();
		}
	}

	for ( int m = 0; m < MAP_SUPPORTED_QTY; ++m )
	{
		if ( !g_MapList.IsMapSupported(m) )
			continue;

		for ( int s = 0, qty = _Sectors.GetSectorQty(m); s < qty; ++s )
		{
			EXC_TRY("OnSector");

			CSector	*pSector = _Sectors.GetSector(m, s);
			ASSERT(pSector);
			pSector->Restock();

			EXC_CATCH;
		}
	}

	g_Serv.SetServerMode(SERVMODE_Run);
	g_Log.Event(LOGL_EVENT, "World Restock: done.\n");
}

void CWorld::Close()
{
	ADDTOCALLSTACK("CWorld::Close");
	if ( IsSaving() )
		Save(true);

	m_Stones.clear();

    {
        std::unique_lock<std::shared_mutex> lock_su(_Ticker._ObjStatusUpdates.THREAD_CMUTEX);
		_Ticker._ObjStatusUpdates.clear();
    }

	m_Parties.ClearContainer();
	m_GMPages.ClearContainer();

    // Disconnect the players, so that we have none of them in a sector
    ClientIterator it;
    for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
    {
        //pClient->GetNetState()->clear();
        pClient->GetNetState()->markReadClosed();
        pClient->CharDisconnect();
    }

	_Sectors.Close();

	memset(g_MapList.m_maps, 0, sizeof(g_MapList.m_maps));
	if ( g_MapList.m_pMapDiffCollection != nullptr )
	{
		delete g_MapList.m_pMapDiffCollection;
		g_MapList.m_pMapDiffCollection = nullptr;
	}

	CloseAllUIDs();

	_GameClock.Init();	// no more sense of time.
}

void CWorld::GarbageCollection()
{
	ADDTOCALLSTACK("CWorld::GarbageCollection");
	g_Log.Flush();
	g_Serv.SetServerMode(SERVMODE_GarbageCollection);
	g_Log.Event(LOGL_EVENT|LOGM_NOCONTEXT, "Garbage Collection: started.\n");
	GarbageCollection_UIDs();
	g_Serv.SetServerMode(SERVMODE_Run);
	g_Log.Flush();
}

void CWorld::_OnTick()
{
	ADDTOCALLSTACK("CWorld::_OnTick");
	// 256 real secs = 1 server hour. 19 light levels. check every 10 minutes or so.

	// Do not tick while loading (startup, resync, exiting...) or when double ticking in the same msec?.
	if (g_Serv.IsLoading() || !_GameClock.Advance())
		return;

	EXC_TRY("CWorld Tick");

	EXC_SET_BLOCK("World Tick");
	_Ticker.Tick();

	EXC_SET_BLOCK("Delete objects");
	m_ObjDelete.ClearContainer();	// clean up our delete list (this DOES delete the objects, thanks to the virtual destructors).
	m_ObjSpecialDelete.ClearContainer();

	int64 iCurTime = _GameClock.GetCurrentTime().GetTimeRaw();

	EXC_SET_BLOCK("Worldsave checks");
	// Save state checks
	// Notifications
	if ((_fSaveNotificationSent == false) && ((_iTimeLastWorldSave - (10 * MSECS_PER_SEC)) <= iCurTime))
	{
		CWorldComm::Broadcast(g_Cfg.GetDefaultMsg(DEFMSG_SERVER_WORLDSAVE_NOTIFY));
		_fSaveNotificationSent = true;
	}

	// Save
	if (_iTimeLastWorldSave <= iCurTime)
	{
		// Auto save world
		_iTimeLastWorldSave = iCurTime + g_Cfg.m_iSavePeriod;
		g_Log.Flush();
		Save(false);
	}

	// Update map cache
	if (_Cache._iTimeLastMapBlockCacheCheck < iCurTime)
	{
		EXC_SET_BLOCK("Check map cache");
		// delete the static CServerMapBlock items that have not been used recently.
		_Cache.CheckMapBlockCache(iCurTime, g_Cfg._iMapCacheTime);
	}

	// Global (ini) stuff.
	// Respawn Dead NPCs
	if (_iTimeLastDeadRespawn <= iCurTime)
	{
		EXC_SET_BLOCK("Respawn dead NPCs");
		// Time to regen all the dead NPC's in the world.
		_iTimeLastDeadRespawn = iCurTime + (20 * 60 * MSECS_PER_SEC);
		RespawnDeadNPCs();
	}

	// f_onserver_timer function.
	if (_iTimeLastCallUserFunc < iCurTime)
	{
		if (g_Cfg._iTimerCall)
		{
			EXC_SET_BLOCK("f_onserver_timer");
			_iTimeLastCallUserFunc = iCurTime + g_Cfg._iTimerCall;
			CScriptTriggerArgs args(g_Cfg._iTimerCall / (60 * MSECS_PER_SEC));
			g_Serv.r_Call("f_onserver_timer", &g_Serv, &args);
		}
	}

	EXC_CATCH;
}
