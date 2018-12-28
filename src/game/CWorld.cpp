#include "../common/CDataBase.h"
#include "../common/CException.h"
#include "../common/CUIDExtra.h"
#include "../common/sphereversion.h"
#include "../network/network.h"
#include "../network/send.h"
#include "../sphere/ProfileTask.h"
#include "../common/CLog.h"
#include "chars/CChar.h"
#include "clients/CClient.h"
#include "clients/CGMPage.h"
#include "chars/CChar.h"
#include "items/CItem.h"
#include "items/CItemShip.h"
#include "CObjBase.h"
#include "CServer.h"
#include "CServerTime.h"
#include "CScriptProfiler.h"

#ifndef _WIN32
    #include <sys/statvfs.h>
#endif
#include <sys/stat.h>
#include <algorithm>    // for std::vector.erase(std::remove())
#include <ctime>
#include <chrono>


static const SOUND_TYPE sm_Sounds_Ghost[] =
{
	SOUND_GHOST_1,
	SOUND_GHOST_2,
	SOUND_GHOST_3,
	SOUND_GHOST_4,
	SOUND_GHOST_5
};

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
			pStr = "Ridden NPC without a mount item";
			break;

		case 0x1105:
			pStr = "Ridden NPC with a mislinked mount item";
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

	if ( (pObj->m_iCreatedResScriptIdx != (size_t)-1) && (pObj->m_iCreatedResScriptLine != -1) )
	{
		// Object was created via NEWITEM or NEWNPC in scripts, tell me where
		CResourceScript* pResFile = g_Cfg.GetResourceFile((size_t)(pObj->m_iCreatedResScriptIdx));
		if (pResFile == nullptr)
			return;
		DEBUG_ERR(("GC:\t Object was created in '%s', line %d.\n", (lpctstr)pResFile->GetFilePath(), pObj->m_iCreatedResScriptLine));
	}
}



//////////////////////////////////////////////////////////////////
// -CWorldSearch

CWorldSearch::CWorldSearch( const CPointMap & pt, int iDist ) :
	m_pt(pt), m_iDist(iDist)
{
	// define a search of the world.
	m_fAllShow = false;
	m_fSearchSquare = false;
	m_pObj = m_pObjNext = nullptr;
	m_fInertToggle = false;

	m_pSectorBase = m_pSector = pt.GetSector();

	m_rectSector.SetRect(
		pt.m_x - iDist,
		pt.m_y - iDist,
		pt.m_x + iDist + 1,
		pt.m_y + iDist + 1,
		pt.m_map);

	// Get upper left of search rect.
	m_iSectorCur = 0;
}

bool CWorldSearch::GetNextSector()
{
	ADDTOCALLSTACK("CWorldSearch::GetNextSector");
	// Move search into nearby CSector(s) if necessary

	if ( ! m_iDist )
		return false;

	for (;;)
	{
		m_pSector = m_rectSector.GetSector(m_iSectorCur++);
		if ( m_pSector == nullptr )
			return false;	// done searching.
		if ( m_pSectorBase == m_pSector )
			continue;	// same as base.
		m_pObj = nullptr;	// start at head of next Sector.
		return true;
	}
}

CItem * CWorldSearch::GetItem()
{
	ADDTOCALLSTACK("CWorldSearch::GetItem");
	for (;;)
	{
		if ( m_pObj == nullptr )
		{
			m_fInertToggle = false;
			m_pObj = static_cast <CObjBase*> ( m_pSector->m_Items_Inert.GetHead());
		}
		else
		{
			m_pObj = m_pObjNext;
		}
		if ( m_pObj == nullptr )
		{
			if ( ! m_fInertToggle )
			{
				m_fInertToggle = true;
				m_pObj = static_cast <CObjBase*> ( m_pSector->m_Items_Timer.GetHead());
				if ( m_pObj != nullptr )
					goto jumpover;
			}
			if ( GetNextSector() )
				continue;
			return nullptr;
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( m_fSearchSquare )
		{
			if ( m_fAllShow )
			{
				if ( m_pt.GetDistSightBase( m_pObj->GetTopPoint() ) <= m_iDist )
					return static_cast <CItem *> ( m_pObj );
			}
			else
			{
				if ( m_pt.GetDistSight( m_pObj->GetTopPoint() ) <= m_iDist )
					return static_cast <CItem *> ( m_pObj );
			}
		}
		else
		{
			if ( m_fAllShow )
			{
				if ( m_pt.GetDistBase( m_pObj->GetTopPoint() ) <= m_iDist )
					return static_cast <CItem *> ( m_pObj );
			}
			else
			{
				if ( m_pt.GetDist( m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast <CItem *> ( m_pObj );
			}
		}
	}
}

void CWorldSearch::SetAllShow( bool fView )
{
	ADDTOCALLSTACK("CWorldSearch::SetAllShow");
	m_fAllShow = fView;
}

void CWorldSearch::SetSearchSquare( bool fSquareSearch )
{
	ADDTOCALLSTACK("CWorldSearch::SetSearchSquare");
	m_fSearchSquare = fSquareSearch;
}

void CWorldSearch::RestartSearch()
{
	ADDTOCALLSTACK("CWorldSearch::RestartSearch");
	m_pObj = nullptr;
}

CChar * CWorldSearch::GetChar()
{
	ADDTOCALLSTACK("CWorldSearch::GetChar");
	for (;;)
	{
		if ( m_pObj == nullptr )
		{
			m_fInertToggle = false;
			m_pObj = static_cast <CObjBase*> ( m_pSector->m_Chars_Active.GetHead());
		}
		else
			m_pObj = m_pObjNext;

		if ( m_pObj == nullptr )
		{
			if ( ! m_fInertToggle && m_fAllShow )
			{
				m_fInertToggle = true;
				m_pObj = static_cast <CObjBase*> ( m_pSector->m_Chars_Disconnect.GetHead());
				if ( m_pObj != nullptr )
					goto jumpover;
			}
			if ( GetNextSector() )
				continue;
			return nullptr;
		}

jumpover:
		m_pObjNext = m_pObj->GetNext();
		if ( m_fSearchSquare )
		{
			if ( m_fAllShow )
			{
				if ( m_pt.GetDistSightBase( m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast <CChar *> ( m_pObj );
			}
			else
			{
				if ( m_pt.GetDistSight( m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast <CChar *> ( m_pObj );
			}
		}
		else
		{
			if ( m_fAllShow )
			{
				if ( m_pt.GetDistBase( m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast <CChar *> ( m_pObj );
			}
			else
			{
				if ( m_pt.GetDist( m_pObj->GetTopPoint()) <= m_iDist )
					return static_cast <CChar *> ( m_pObj );
			}
		}
	}
}


//////////////////////////////////////////////////////////////////
// -CWorldThread

CWorldThread::CWorldThread()
{
	m_fSaveParity = false;		// has the sector been saved relative to the char entering it ?
	m_iUIDIndexLast = 1;

	m_FreeUIDs = (dword*)calloc(FREE_UIDS_SIZE, sizeof(dword));
	m_FreeOffset = FREE_UIDS_SIZE;
}

CWorldThread::~CWorldThread()
{
	CloseAllUIDs();
}

void CWorldThread::CloseAllUIDs()
{
	ADDTOCALLSTACK("CWorldThread::CloseAllUIDs");
	m_ObjDelete.Clear();	// empty our list of unplaced objects (and delete the objects in the list)
	m_ObjNew.Clear();		// empty our list of objects to delete (and delete the objects in the list)
	m_UIDs.clear();

	if ( m_FreeUIDs != nullptr )
	{
		free(m_FreeUIDs);
		m_FreeUIDs = nullptr;
	}
	m_FreeOffset = FREE_UIDS_SIZE;
}

bool CWorldThread::IsSaving() const
{
	return (m_FileWorld.IsFileOpen() && m_FileWorld.IsWriteMode());
}

dword CWorldThread::GetUIDCount() const
{
	return (dword) m_UIDs.size();
}

CObjBase *CWorldThread::FindUID(dword dwIndex) const
{
	if ( !dwIndex || dwIndex >= GetUIDCount() )
		return nullptr;
	if ( m_UIDs[ dwIndex ] == UID_PLACE_HOLDER )	// unusable for now. (background save is going on)
		return nullptr;
	return m_UIDs[dwIndex];
}

void CWorldThread::FreeUID(dword dwIndex)
{
	// Can't free up the UID til after the save !
	m_UIDs[dwIndex] = IsSaving() ? UID_PLACE_HOLDER : nullptr;
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

		if ( m_FreeOffset < FREE_UIDS_SIZE && m_FreeUIDs != nullptr )
		{
			//	We do have a free uid's array. Use it if possible to determine the first free element
			for ( ; m_FreeOffset < FREE_UIDS_SIZE && m_FreeUIDs[m_FreeOffset] != 0; m_FreeOffset++ )
			{
				//	yes, that's a free slot
				if ( !m_UIDs[m_FreeUIDs[m_FreeOffset]] )
				{
					dwIndex = m_FreeUIDs[m_FreeOffset++];
					goto successalloc;
				}
			}
		}
		m_FreeOffset = FREE_UIDS_SIZE;	// mark array invalid, since it does not contain any empty slots
										// use default allocation for a while, till the next garbage collection
		dword dwCount = dwCountTotal - 1;
		dwIndex = m_iUIDIndexLast;
		while ( m_UIDs[dwIndex] != nullptr )
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
		m_UIDs.resize((dwIndex + 0x1000) & ~0xFFF);
	}

successalloc:
	m_iUIDIndexLast = dwIndex; // start from here next time so we have even distribution of allocation.
	CObjBase *pObjPrv = m_UIDs[dwIndex];
	if ( pObjPrv )
	{
		//NOTE: We cannot use Delete() in here because the UID will
		//	still be assigned til the async cleanup time. Delete() will not work here!
		DEBUG_ERR(("UID conflict delete 0%x, '%s'\n", dwIndex, pObjPrv->GetName()));
		delete pObjPrv;
	}
	m_UIDs[dwIndex] = pObj;
	return dwIndex;
}

void CWorldThread::SaveThreadClose()
{
	ADDTOCALLSTACK("CWorldThread::SaveThreadClose");
	for ( size_t i = 1; i < GetUIDCount(); ++i )
	{
		if ( m_UIDs[i] == UID_PLACE_HOLDER )
			m_UIDs[i] = nullptr;
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
		if (( pObj->GetUID() & UID_O_INDEX_MASK ) != dwUID )
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

void CWorldThread::GarbageCollection_New()
{
	ADDTOCALLSTACK("CWorldThread::GarbageCollection_New");
	EXC_TRY("GarbageCollection_New");
	// Clean up new objects that are never placed.
	size_t iObjCount = m_ObjNew.GetCount();
	if (iObjCount > 0 )
	{
		g_Log.Event(LOGL_ERROR, "GC: %" PRIuSIZE_T " unplaced objects!\n", iObjCount);

		for (size_t i = 0; i < iObjCount; ++i)
		{
			CObjBase * pObj = dynamic_cast<CObjBase*>(m_ObjNew.GetAt(i));
			if (pObj == nullptr)
				continue;

			ReportGarbageCollection(pObj, 0x3202);
		}
		m_ObjNew.Clear();	// empty our list of unplaced objects (and delete the objects in the list)
	}
	m_ObjDelete.Clear();	// empty our list of objects to delete (and delete the objects in the list)

	// Make sure all GM pages have accounts.
	CGMPage *pPage = static_cast<CGMPage *>(g_World.m_GMPages.GetHead());
	while ( pPage != nullptr )
	{
		CGMPage * pPageNext = pPage->GetNext();
		if ( ! pPage->FindAccount()) // Open script file
		{
			DEBUG_ERR(("GC: GM Page has invalid account '%s'\n", pPage->GetName()));
			delete pPage;
		}
		pPage = pPageNext;
	}
	EXC_CATCH;
}

void CWorldThread::GarbageCollection_UIDs()
{
	ADDTOCALLSTACK("CWorldThread::GarbageCollection_UIDs");
	// Go through the m_ppUIDs looking for Objects without links to reality.
	// This can take a while.

	GarbageCollection_New();

	dword iCount = 0;
	for (dword i = 1; i < GetUIDCount(); ++i )
	{
		try
		{
			CObjBase * pObj = m_UIDs[i];
			if ( !pObj || pObj == UID_PLACE_HOLDER )
				continue;

			// Look for anomalies and fix them (that might mean delete it.)
			int iResultCode = FixObj(pObj, i);
			if ( iResultCode )
			{
				// Do an immediate delete here instead of Delete()
				delete pObj;
				FreeUID(i);	// Get rid of junk uid if all fails..
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

	GarbageCollection_New();

	if ( iCount != CObjBase::sm_iCount )	// All objects must be accounted for.
		g_Log.Event(LOGL_ERROR|LOGM_NOCONTEXT, "Garbage Collection: done. Object memory leak %" PRIu32 "!=%" PRIu32 ".\n", iCount, CObjBase::sm_iCount);
	else
		g_Log.Event(LOGL_EVENT|LOGM_NOCONTEXT, "Garbage Collection: done. %" PRIu32 " Objects accounted for.\n", iCount);

	if ( m_FreeUIDs != nullptr )	// new UID engine - search for empty holes and store it in a huge array
	{							// the size of the array should be enough even for huge shards
								// to survive till next garbage collection
		memset(m_FreeUIDs, 0, FREE_UIDS_SIZE * sizeof(dword));
		m_FreeOffset = 0;

		for ( dword d = 1; d < GetUIDCount(); ++d )
		{
			CObjBase *pObj = m_UIDs[d];

			if ( !pObj )
			{
				if ( m_FreeOffset >= ( FREE_UIDS_SIZE - 1 ))
					break;

				m_FreeUIDs[m_FreeOffset++] = d;
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
// -CWorldClock

int64 CWorldClock::GetSystemClock() // static
{
	ADDTOCALLSTACK("CWorldClock::GetSystemClock");
	// Return system wall-clock using high resolution value (milliseconds)
    auto timeMaxResolution = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(timeMaxResolution).count();
}

void CWorldClock::InitTime( int64 iTimeBase )
{
	ADDTOCALLSTACK("CWorldClock::InitTime");
	m_Clock_SysPrev = GetSystemClock();
	m_timeClock.InitTime(iTimeBase+MSECS_PER_TICK);
    _iCurTick = iTimeBase;
}

void CWorldClock::Init()
{
	ADDTOCALLSTACK("CWorldClock::Init");
	m_Clock_SysPrev = GetSystemClock();
	m_timeClock = 0;
    _iCurTick = 0;
}

bool CWorldClock::Advance()
{
	ADDTOCALLSTACK("CWorldClock::Advance");
	const int64 Clock_Sys = GetSystemClock();
	const int64 iTimeDiff = Clock_Sys - m_Clock_SysPrev;
	if ( iTimeDiff == 0 )
		return false;
	else if ( iTimeDiff < 0 )
	{
		// System clock has changed forward
		// Just wait until next cycle and it should be ok
		g_Log.Event(LOGL_WARN, "System clock has changed forward (daylight saving change, etc). This may cause strange behavior on some objects.\n");
		m_Clock_SysPrev = Clock_Sys;
		return false;
	}

	m_Clock_SysPrev = Clock_Sys;
	const CServerTime Clock_New = m_timeClock + iTimeDiff;

	// CServerTime is signed (it's now int64)!
	// NOTE: This will overflow after 292 millions or so years of run time, good luck!
	if ( Clock_New < m_timeClock )
	{
		g_Log.Event(LOGL_WARN, "System clock has changed backward (daylight saving change, etc). This may cause strange behavior on some objects.\n");
		m_timeClock = Clock_New;
		return false;
	}

	m_timeClock = Clock_New;
    // Maths here are done with MSECs precision, if proceed we advance a server's TICK.
    if (m_nextTickTime <= GetCurrentTime())
    {
        m_nextTickTime = GetCurrentTime() + MSECS_PER_TICK;	// Next hit time.
        ++_iCurTick;
    }
	return true;
}

//////////////////////////////////////////////////////////////////
// -CWorld

CWorld::CWorld()
{
	m_ticksWithoutMySQL = 0;
	m_savetimer = 0;
	m_iSaveCountID = 0;
	m_iSaveStage = 0;
	m_iPrevBuild = 0;
	m_iLoadVersion = 0;
	m_bSaveNotificationSent = false;
	m_timeSave = 0;
	m_timeRespawn = 0;
	m_timeStartup = 0;
	m_timeCallUserFunc = 0;
	m_Sectors = nullptr;
	m_SectorsQty = 0;
}

void CWorld::Init()
{
	EXC_TRY("Init");

	if ( m_Sectors )	//	disable changes on-a-fly
		return;

	g_MapList.Init();
	if ( g_MapList.m_pMapDiffCollection )
		g_MapList.m_pMapDiffCollection->Init();

	//	initialize all sectors
	int	sectors = 0;
	int m = 0;
	for ( m = 0; m < 256; ++m )
	{
		if ( !g_MapList.m_maps[m] )
			continue;
		sectors += g_MapList.GetSectorQty(m);
	}

	m_Sectors = new CSector*[sectors];
	TemporaryString tsZ;
	TemporaryString tsZ1;
	tchar* z = static_cast<tchar *>(tsZ);
	tchar* z1 = static_cast<tchar *>(tsZ1);

	for ( m = 0; m < 256; ++m )
	{
		if ( !g_MapList.m_maps[m] )
			continue;

		sprintf(z1, " map%d=%d", m, g_MapList.GetSectorQty(m));
		strcat(z, z1);
		for ( int s = 0; s < g_MapList.GetSectorQty(m); ++s )
		{
			CSector	*pSector = new CSector;
			ASSERT(pSector);
			pSector->Init(s, m);
			m_Sectors[m_SectorsQty++] = pSector;
		}
	}
    for (uint s = 0; s < m_SectorsQty; ++s)
    {
        CSector *pSector = m_Sectors[s];
        if (pSector)
        {
            pSector->SetAdjacentSectors();
        }
    }
    g_Log.Event(LOGM_INIT, "Allocated map sectors:%s\n", static_cast<lpctstr>(z));
	ASSERT(m_SectorsQty);

	EXC_CATCH;
}

CWorld::~CWorld()
{
	Close();
}

///////////////////////////////////////////////
// Loading and Saving.

void CWorld::GetBackupName( CSString & sArchive, lpctstr pszBaseDir, tchar chType, int iSaveCount ) // static
{
	ADDTOCALLSTACK("CWorld::GetBackupName");
	int iCount = iSaveCount;
	int iGroup = 0;
	for ( ; iGroup<g_Cfg.m_iSaveBackupLevels; ++iGroup )
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
	remove( sArchive );

	// rename previous save to archive name.
	CSString sSaveName;
	sSaveName.Format( "%s" SPHERE_FILE "%s%s", pszBaseDir, pszBaseName, SPHERE_SCRIPT );

	if ( rename( sSaveName, sArchive ))
	{
		// May not exist if this is the first time.
		g_Log.Event(LOGM_SAVE|LOGL_WARN, "Rename %s to '%s' FAILED code %d?\n", static_cast<lpctstr>(sSaveName), static_cast<lpctstr>(sArchive), CSFile::GetLastError() );
	}

	if ( ! s.Open( sSaveName, OF_WRITE|OF_TEXT|OF_DEFAULTMODE ))
	{
		g_Log.Event(LOGM_SAVE|LOGL_CRIT, "Save '%s' FAILED\n", static_cast<lpctstr>(sSaveName));
		return false;
	}

	return true;
}

bool CWorld::SaveStage() // Save world state in stages.
{
	ADDTOCALLSTACK("CWorld::SaveStage");
	// Do the next stage of the save.
	// RETURN: true = continue; false = done.
	EXC_TRY("SaveStage");
	bool bRc = true;

	if ( m_iSaveStage == -1 )
	{
		if ( !g_Cfg.m_fSaveGarbageCollect )
			GarbageCollection_New();
	}
	else if ( m_iSaveStage < (int)(m_SectorsQty) )
	{
		// NPC Chars in the world secors and the stuff they are carrying.
		// Sector lighting info.
		if(IsSetEF(EF_Dynamic_Backsave))
		{
			size_t szComplexity = 0;

			CSector *s = m_Sectors[m_iSaveStage];
			if( s )
			{
				s->r_Write();
				szComplexity += ( s->GetCharComplexity() + s->GetInactiveChars())*100 + s->GetItemComplexity();
			}
			uint dynStage = m_iSaveStage + 1;
			if( szComplexity <= g_Cfg.m_iSaveStepMaxComplexity )
			{
				uint szSectorCnt = 1;
				while(dynStage < m_SectorsQty && szSectorCnt <= g_Cfg.m_iSaveSectorsPerTick)
				{
					s = m_Sectors[dynStage];
					if ( s )
					{
						szComplexity += ( s->GetCharComplexity() + s->GetInactiveChars())*100 + s->GetItemComplexity();

						if(szComplexity <= g_Cfg.m_iSaveStepMaxComplexity)
						{
							s->r_Write();
							m_iSaveStage = dynStage;
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
			if ( m_Sectors[m_iSaveStage] )
				m_Sectors[m_iSaveStage]->r_Write();
		}
	}
	else if ( m_iSaveStage == (int)(m_SectorsQty) )
	{
		m_FileData.WriteSection( "TIMERF" );
		g_World.m_TimedFunctions.r_Write(m_FileData);

		m_FileData.WriteSection("GLOBALS");
		g_Exp.m_VarGlobals.r_WritePrefix(m_FileData, nullptr);

		g_Exp.m_ListGlobals.r_WriteSave(m_FileData);

		size_t iQty = g_Cfg.m_RegionDefs.size();
		for ( size_t i = 0; i < iQty; ++i )
		{
			CRegion *pRegion = dynamic_cast <CRegion*> (g_Cfg.m_RegionDefs.at(i));
			if ( !pRegion || !pRegion->HasResourceName() || !pRegion->m_iModified )
				continue;

			m_FileData.WriteSection("WORLDSCRIPT %s", pRegion->GetResourceName());
			pRegion->r_WriteModified(m_FileData);
		}

		// GM_Pages.
		CGMPage *pPage = static_cast <CGMPage*>(m_GMPages.GetHead());
		for ( ; pPage != nullptr; pPage = pPage->GetNext())
		{
			pPage->r_Write(m_FileData);
		}
	}
	else if ( m_iSaveStage == (int)(m_SectorsQty)+1 )
	{
		//	Empty save stage
	}
	else if ( m_iSaveStage == (int)(m_SectorsQty)+2 )
	{
		// Now make a backup of the account file.
		bRc = g_Accounts.Account_SaveAll();
	}
	else if ( m_iSaveStage == (int)(m_SectorsQty)+3 )
	{
		// EOF marker to show we reached the end.
		m_FileData.WriteSection("EOF");
		m_FileWorld.WriteSection("EOF");
		m_FilePlayers.WriteSection("EOF");
		m_FileMultis.WriteSection("EOF");

		m_iSaveCountID++;	// Save only counts if we get to the end winout trapping.
		m_timeSave = g_World.GetCurrentTime().GetTimeRaw() + g_Cfg.m_iSavePeriod;	// next save time.

		g_Log.Event(LOGM_SAVE, "World data saved   (%s).\n", m_FileWorld.GetFilePath());
		g_Log.Event(LOGM_SAVE, "Player data saved  (%s).\n", m_FilePlayers.GetFilePath());
		g_Log.Event(LOGM_SAVE, "Multi data saved   (%s).\n", m_FileMultis.GetFilePath());
		g_Log.Event(LOGM_SAVE, "Context data saved (%s).\n", m_FileData.GetFilePath());

		llong	llTicksEnd;
		llong	llTicksStart = m_savetimer;
		TIME_PROFILE_END;

		tchar * time = Str_GetTemp();
		sprintf(time, "%" PRId64 ".%04lld", (int64)(TIME_PROFILE_GET_HI / MSECS_PER_SEC), (int64)(TIME_PROFILE_GET_LO));

		g_Log.Event(LOGM_SAVE, "World save completed, took %s seconds.\n", time);

		CScriptTriggerArgs Args;
		Args.Init(time);
		g_Serv.r_Call("f_onserver_save_finished", &g_Serv, &Args);

		// Now clean up all the held over UIDs
		SaveThreadClose();
		m_iSaveStage = INT32_MAX;
		return false;
	}

	if ( g_Cfg.m_iSaveBackgroundTime )
	{
		int64 iNextTime = g_Cfg.m_iSaveBackgroundTime / m_SectorsQty;
		if ( iNextTime > MSECS_PER_SEC *30 * 60 )
			iNextTime = MSECS_PER_SEC * 30 * 60;	// max out at 30 minutes or so.
		m_timeSave = g_World.GetCurrentTime().GetTimeRaw() + iNextTime;
	}
	++m_iSaveStage;
	return bRc;

	EXC_CATCH;

	EXC_DEBUG_START;
	g_Log.EventDebug("stage '%d' qty '%d' time '%" PRId64 "'\n", m_iSaveStage, m_SectorsQty, m_timeSave);
	EXC_DEBUG_END;

	++m_iSaveStage;	// to avoid loops, we need to skip the current operation in world save
	return false;
}

bool CWorld::SaveForce() // Save world state
{
	ADDTOCALLSTACK("CWorld::SaveForce");
	Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_SERVER_WORLDSAVE ) );
#ifndef _MTNETWORK
	if (g_NetworkOut.isActive() == false)
		g_NetworkOut.flushAll();
#else
	if (g_NetworkManager.isOutputThreaded() == false)
		g_NetworkManager.flushAllClients();
#endif

	g_Serv.SetServerMode(SERVMODE_Saving);	// Forced save freezes the system.
	bool bSave = true;
	bool bSuccess = true;

	static lpctstr const msgs[] =
	{
		"garbage collection",
		"sectors",
		"global variables, regions, gmpages",
		"servers",
		"accounts",
		""
	};
	const char *pCurBlock = msgs[0];

	while ( bSave )
	{
		try
		{
			if (( m_iSaveStage >= 0 ) && ( m_iSaveStage < (int)(m_SectorsQty) ))
				pCurBlock = msgs[1];
			else if ( m_iSaveStage == (int)(m_SectorsQty) )
				pCurBlock = msgs[2];
			else if ( m_iSaveStage == (int)(m_SectorsQty)+1 )
				pCurBlock = msgs[3];
			else if ( m_iSaveStage == (int)(m_SectorsQty)+2 )
				pCurBlock = msgs[4];
			else
				pCurBlock = msgs[5];

			bSave = SaveStage();
			if (! ( m_iSaveStage & 0x1FF ))
			{
				g_Serv.PrintPercent( m_iSaveStage, m_SectorsQty+3 );
			}
			if ( !bSave && ( pCurBlock != msgs[5] ))
				goto failedstage;
		}
		catch ( const CSError& e )
		{
			g_Log.CatchEvent(&e, "Save FAILED for stage %u (%s).", m_iSaveStage, pCurBlock);
			bSuccess = false;
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch (...)
		{
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
			goto failedstage;
		}
		continue;
failedstage:
		g_Log.CatchEvent( nullptr, "Save FAILED for stage %u (%s).", m_iSaveStage, pCurBlock);
		bSuccess = false;
	}

	g_Serv.SetServerMode(SERVMODE_Run);			// Game is up and running
	return bSuccess;
}

bool CWorld::SaveTry( bool fForceImmediate ) // Save world state
{
	ADDTOCALLSTACK("CWorld::SaveTry");
	EXC_TRY("SaveTry");
	if ( m_FileWorld.IsFileOpen() )
	{
		// Save is already active !
		ASSERT( IsSaving() ) ;
		if ( fForceImmediate )	// finish it now !
			return SaveForce();
		else if ( g_Cfg.m_iSaveBackgroundTime )
			return SaveStage();
		return false;
	}

	// Do the write async from here in the future.
	if ( g_Cfg.m_fSaveGarbageCollect )
		GarbageCollection();

	llong llTicksStart;
	TIME_PROFILE_START;
	m_savetimer = llTicksStart;

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

	m_fSaveParity = ! m_fSaveParity; // Flip the parity of the save.
	m_iSaveStage = -1;
	m_bSaveNotificationSent = false;
	m_timeSave = 0;

	// Write the file headers.
	r_Write(m_FileData);
	r_Write(m_FileWorld);
	r_Write(m_FilePlayers);
	r_Write(m_FileMultis);

	if ( fForceImmediate || ! g_Cfg.m_iSaveBackgroundTime )	// Save now !
		return SaveForce();
	return true;
	EXC_CATCH;

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
    lpctstr ptcSaveDir = g_Cfg.m_sWorldBaseDir.GetPtr();
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
    auto CalcPrevSavesSize = [=,&fSizeErr, &uiPreviousSaveSize](lpctstr ptcSaveName) -> void
    {
        struct stat st;
        CSString strSaveFile = g_Cfg.m_sWorldBaseDir + SPHERE_FILE + ptcSaveName + SPHERE_SCRIPT;
        stat(strSaveFile.GetPtr(), &st);
        ullong uiCurSavefileSize = (ullong)st.st_size;
        if (uiCurSavefileSize == 0)
            fSizeErr = true;
        else
            uiPreviousSaveSize += uiCurSavefileSize;
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
        g_Log.Event(LOGL_CRIT, "Save ABORTED! Disk space low!");
        g_Log.Event(LOGL_CRIT, "-----------------------------");
        Broadcast("Save ABORTED! Warn the administrator!");
        return false;
    }
    return true;
}

bool CWorld::Save( bool fForceImmediate ) // Save world state
{
	ADDTOCALLSTACK("CWorld::Save");

    //if (!CheckAvailableSpaceForSave(false))
    //    return false;

    //-- Ok we can start the save process, in which we eventually remove the previous saves and create the other.

	bool bSaved = false;
	try
	{
		CScriptTriggerArgs Args(fForceImmediate, m_iSaveStage);
		enum TRIGRET_TYPE tr;

		if ( g_Serv.r_Call("f_onserver_save", &g_Serv, &Args, nullptr, &tr) )
			if ( tr == TRIGRET_RET_TRUE )
				return false;
		//Fushing before the server should fix #2306
		//The scripts fills the clients buffer and the server flush
		//the data during the save.
		//Should we flush only non threaded output or force it
		//to flush on any conditions?

#ifndef _MTNETWORK
		if (g_NetworkOut.isActive() == false)
#else
		if (g_NetworkManager.isOutputThreaded() == false)
#endif
        {
#ifdef _DEBUG
			g_Log.EventDebug("Flushing %" PRIuSIZE_T " client(s) output data...\n", g_Serv.StatGet(SERV_STAT_CLIENTS));
#endif
#ifndef _MTNETWORK
			g_NetworkOut.flushAll();
#else
			g_NetworkManager.flushAllClients();
#endif
#ifdef _DEBUG
			g_Log.EventDebug("Done flushing clients output data.\n");
#endif
		}

		fForceImmediate = (Args.m_iN1 != 0);
		bSaved = SaveTry(fForceImmediate);
	}
	catch ( const CSError& e )
	{
		g_Log.CatchEvent( &e, "Save FAILED." );
		Broadcast("Save FAILED. " SPHERE_TITLE " is UNSTABLE!");
		m_FileData.Close();		// close if not already closed.
		m_FileWorld.Close();	// close if not already closed.
		m_FilePlayers.Close();	// close if not already closed.
		m_FileMultis.Close();	// close if not already closed.
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent( nullptr, "Save FAILED" );
		Broadcast("Save FAILED. " SPHERE_TITLE " is UNSTABLE!");
		m_FileData.Close();		// close if not already closed.
		m_FileWorld.Close();	// close if not already closed.
		m_FilePlayers.Close();	// close if not already closed.
		m_FileMultis.Close();	// close if not already closed.
		CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
	}

	CScriptTriggerArgs Args(fForceImmediate, m_iSaveStage);
	g_Serv.r_Call((bSaved?"f_onserver_save_ok":"f_onserver_save_fail"), &g_Serv, &Args);
	return bSaved;
}

void CWorld::SaveStatics()
{
	ADDTOCALLSTACK("CWorld::SaveStatics");

    if (!CheckAvailableSpaceForSave(true))
        return;

	try
	{
		if ( !g_Cfg.m_fSaveGarbageCollect )
			GarbageCollection_New();

		CScript m_FileStatics;
		if ( !OpenScriptBackup(m_FileStatics, g_Cfg.m_sWorldBaseDir, "statics", m_iSaveCountID) )
			return;
		r_Write(m_FileStatics);

		Broadcast( g_Cfg.GetDefaultMsg(DEFMSG_SERVER_WORLDSTATICSAVE) );
#ifndef _MTNETWORK
		if (g_NetworkOut.isActive() == false)
			g_NetworkOut.flushAll();
#else
		if (g_NetworkManager.isOutputThreaded() == false)
			g_NetworkManager.flushAllClients();
#endif

		//	loop through all sectors and save static items
		for ( int m = 0; m < 256; ++m )
		{
			if ( !g_MapList.m_maps[m] )
                continue;

			for ( int d = 0; d < g_MapList.GetSectorQty(m); ++d )
			{
				CItem	*pNext, *pItem;
				CSector	*pSector = GetSector(m, d);

				if ( !pSector )
                    continue;

				pItem = static_cast <CItem*>(pSector->m_Items_Inert.GetHead());
				for ( ; pItem != nullptr; pItem = pNext )
				{
					pNext = pItem->GetNext();
                    if (pItem->IsTypeMulti())
						continue;
					if ( !pItem->IsAttr(ATTR_STATIC) )
						continue;

					pItem->r_WriteSafe(m_FileStatics);
				}

				pItem = static_cast <CItem*>(pSector->m_Items_Timer.GetHead());
				for ( ; pItem != nullptr; pItem = pNext )
				{
					pNext = pItem->GetNext();
					if ( pItem->IsTypeMulti() )
						continue;
					if ( !pItem->IsAttr(ATTR_STATIC) )
						continue;

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
}

/////////////////////////////////////////////////////////////////////

void CWorld::_InsertTimedObject(int64 iTimeout, CCTimedObject *pTimedObject)
{
    _mWorldTickList.THREAD_CMUTEX.lock();
    TimedObjectsContainer& timedObjCont = _mWorldTickList[iTimeout];
    _mWorldTickList.THREAD_CMUTEX.unlock();

    timedObjCont.THREAD_CMUTEX.lock();
    timedObjCont.emplace_back(pTimedObject);
    timedObjCont.THREAD_CMUTEX.unlock();
}

void CWorld::_RemoveTimedObject(const int64 iOldTimeout, const CCTimedObject* pTimedObject)
{
    _mWorldTickList.THREAD_CMUTEX.lock();
    auto itList =  _mWorldTickList.find(iOldTimeout);
    if (itList == _mWorldTickList.end())
    {
        // This shouldn't happen, since this function is called only after we found pChar in the lookup list and retrieved its iOldTimeout.
        // If this happens: there was an exception in the ticking code, or we are calling this method when we shouldn't.
        _mWorldTickList.THREAD_CMUTEX.unlock();
        return;
    }
    TimedObjectsContainer& cont = itList->second;  // direct access to the container.
    _mWorldTickList.THREAD_CMUTEX.unlock();

    std::vector<CCTimedObject*> tmpCont;   // new container.
    cont.THREAD_CMUTEX.lock();
    if (cont.size() > 1) // if the old container only has 1 entry we don't need to create a new one.
    {
        for (CCTimedObject* pObj : cont)    // Loop until the old container is empty
        {
            if (pObj == pTimedObject)   // if pTimedObject is this entry skip it to remove it from the container.
            {
                continue;
            }
            tmpCont.emplace_back(pObj);  // otherwise add it to the new container.
        }
    }
    cont.clear();

    /*
    * All references to the given CCTimedObject have been taken out from the container
    * and the new one have been populated ? so let's add the new container to the main
    * container, if it has any entry, or clear the top container recursively.
    */
    if (!tmpCont.empty())
    {
        cont.swap(tmpCont);
    }
    cont.THREAD_CMUTEX.unlock();
}

void CWorld::AddTimedObject(int64 iTimeout, CCTimedObject * pTimedObject)
{
    ADDTOCALLSTACK("CWorld::AddTimedObject");
    //if (iTimeout < g_World.GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible
    //    return;

    EXC_TRY("AddTimedObject");
    EXC_SET_BLOCK("Lookup");

    ProfileTask timersTask(PROFILE_TIMERS);
    std::unique_lock<std::shared_mutex> lookupLock(_mWorldTickLookup.THREAD_CMUTEX);

    auto itLookup = _mWorldTickLookup.find(pTimedObject);
    if (itLookup != _mWorldTickLookup.end())
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        // It shouldn't happen, but if it does, this fixes it.
        EXC_SET_BLOCK("LookupReplace");
        _RemoveTimedObject(itLookup->second, itLookup->first);
        itLookup->second = iTimeout;
    }
    else
    {
        EXC_SET_BLOCK("LookupInsert");
        _mWorldTickLookup.insert(std::make_pair(pTimedObject, iTimeout));
    }
    
    EXC_SET_BLOCK("InsertTimedObject");
    _InsertTimedObject(iTimeout, pTimedObject);

    EXC_CATCH;
}

void CWorld::DelTimedObject(CCTimedObject * pTimedObject)
{
    ADDTOCALLSTACK("CWorld::DelTimedObject");
    EXC_TRY("AddTimedObject");
    EXC_SET_BLOCK("Lookup");

    ProfileTask timersTask(PROFILE_TIMERS);
    std::unique_lock<std::shared_mutex> lookupLock(_mWorldTickLookup.THREAD_CMUTEX);

    auto lookupIt = _mWorldTickLookup.find(pTimedObject);
    if (lookupIt == _mWorldTickLookup.end())
        return;

    EXC_SET_BLOCK("LookupRemove");
    const int64 iOldTimeout = lookupIt->second;
    _mWorldTickLookup.erase(lookupIt);

    EXC_SET_BLOCK("RemoveTimedObject");
    _RemoveTimedObject(iOldTimeout, pTimedObject);

    EXC_CATCH;
}

void CWorld::_InsertCharTicking(int64 iTickNext, CChar* pChar)
{
    _mWorldTickList.THREAD_CMUTEX.lock();
    TimedCharsContainer& timedObjCont = _mCharTickList[iTickNext];
    _mWorldTickList.THREAD_CMUTEX.unlock();

    timedObjCont.THREAD_CMUTEX.lock();
    timedObjCont.emplace_back(pChar);
    timedObjCont.THREAD_CMUTEX.unlock();
}

void CWorld::_RemoveCharTicking(const int64 iOldTimeout, const CChar* pChar)
{
    _mCharTickList.THREAD_CMUTEX.lock();
    auto itList =  _mCharTickList.find(iOldTimeout);
    if (itList == _mCharTickList.end())
    {
        // This shouldn't happen, since this function is called only after we found pChar in the lookup list and retrieved its iOldTimeout.
        // If this happens: there was an exception in the ticking code, or we are calling this method when we shouldn't.
        _mCharTickList.THREAD_CMUTEX.unlock();
        return;
    }
    TimedCharsContainer& cont = itList->second;  // direct access to the container.
    _mCharTickList.THREAD_CMUTEX.unlock();

    std::vector<CChar*> tmpCont;   // new container.
    cont.THREAD_CMUTEX.lock();
    if (cont.size() > 1) // if the old container only has 1 entry we don't need to create a new one.
    {
        for (CChar* pCharLoop : cont)    // Loop until the old container is empty
        {
            if (pCharLoop == pChar)   // if pTimedObject is this entry skip it to remove it from the container.
            {
                continue;
            }
            tmpCont.emplace_back(pCharLoop);  // otherwise add it to the new container.
        }
    }
    cont.clear();

    /*
    * All references to the given CChar have been taken out from the container
    * and the new one have been populated ? so let's add the new container to the main
    * container, if it has any entry, or clear the top container recursively.
    */
    if (!tmpCont.empty())
    {
        cont.swap(tmpCont);
    }
    cont.THREAD_CMUTEX.unlock();
}

void CWorld::AddCharTicking(CChar * pChar, bool fIgnoreSleep, bool fOverwrite)
{
    ADDTOCALLSTACK("CWorld::AddCharTicking");
    EXC_TRY("AddCharTicking");

    if (!fIgnoreSleep && pChar->GetTopSector()->IsSleeping())
    {
        return; // Do not allow ticks on sleeping sectors;
    }

    EXC_SET_BLOCK("Lookup");
    ProfileTask timersTask(PROFILE_TIMERS);
    std::unique_lock<std::shared_mutex> lookupLock(_mCharTickLookup.THREAD_CMUTEX);
    
    const int64 iTickNext = pChar->_timeNextRegen;
    //if (iTickNext < g_World.GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible
    //    return;

    bool fDoNotInsert = false;
    auto itLookup = _mCharTickLookup.find(pChar);
    if (itLookup != _mCharTickLookup.end())
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        // It shouldn't happen, but if it does, this fixes it.
        EXC_SET_BLOCK("LookupReplace");
        if (fOverwrite)
        {
            _RemoveCharTicking(itLookup->second, itLookup->first);
            itLookup->second = iTickNext;
        }
        else
        {
            fDoNotInsert = true;
        }
    }
    else
    {
        EXC_SET_BLOCK("LookupInsert");
        _mCharTickLookup.insert(std::make_pair(pChar, iTickNext));
    }

    if (!fDoNotInsert)
    {
        EXC_SET_BLOCK("InsertCharTicking");
        _InsertCharTicking(iTickNext, pChar);
    }
    
    EXC_CATCH;
}

void CWorld::DelCharTicking(CChar * pChar)
{
    ADDTOCALLSTACK("CWorld::DelCharTicking");
    EXC_TRY("DelCharTicking");
    EXC_SET_BLOCK("Lookup");

    ProfileTask timersTask(PROFILE_TIMERS);
    std::unique_lock<std::shared_mutex> lookupLock(_mCharTickLookup.THREAD_CMUTEX);

    auto lookupIt = _mCharTickLookup.find(pChar);
    if (lookupIt == _mCharTickLookup.end())
        return;

    EXC_SET_BLOCK("RemoveCharTicking");
    const int64 iOldTimeout = lookupIt->second;
    _RemoveCharTicking(iOldTimeout, pChar);

    EXC_SET_BLOCK("LookupRemove");
    _mCharTickLookup.erase(lookupIt);

    EXC_CATCH;
}

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
	int iLoadSize = s.GetLength();
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
		LoadFile(sDataName, false);
		LoadFile(sStaticsName, false);
		if ( LoadFile(sWorldName) && LoadFile(sCharsName) && LoadFile(sMultisName, false))
		{
		    return true;
		}

		// If we could not open the file at all then it was a bust!
		if ( m_iSaveCountID == iPrevSaveCount )
            break;

		// Reset everything that has been loaded
		m_ObjStatusUpdates.clear();
        m_Stones.clear();
		m_TimedFunctions.Clear();
		m_Parties.Clear();
		m_GMPages.Clear();

		if ( m_Sectors )
		{
			for ( uint s = 0; s < m_SectorsQty; ++s )
			{
				// Remove everything from the sectors
				m_Sectors[s]->Close();
			}
		}

		CloseAllUIDs();
		m_Clock.Init();
		m_UIDs.resize(8 * 1024);

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

	g_Log.Event(LOGL_FATAL|LOGM_INIT, "No previous backup available ?\n");
	EXC_CATCH;
	return false;
}


bool CWorld::LoadAll() // Load world from script
{
	// start count. (will grow as needed)
	m_UIDs.resize(8 * 1024);
	m_Clock.Init();		// will be loaded from the world file.

	// Load all the accounts.
	if ( !g_Accounts.Account_LoadAll(false) )
		return false;

	// Try to load the world and chars files .
	if ( !LoadWorld() )
		return false;

	m_timeStartup = g_World.GetCurrentTime().GetTimeRaw();
	m_timeSave = g_World.GetCurrentTime().GetTimeRaw() + g_Cfg.m_iSavePeriod;	// next save time.

	// Set all the sector light levels now that we know the time.
	// This should not look like part of the load. (CTRIG_EnvironChange triggers should run)
	size_t iCount;
	for ( uint s = 0; s < m_SectorsQty; ++s )
	{
		EXC_TRYSUB("Load");
		CSector *pSector = m_Sectors[s];

		if ( pSector != nullptr )
		{
			if ( !pSector->IsLightOverriden() )
				pSector->SetLight(-1);

			// Is this area too complex ?
			iCount = pSector->GetItemComplexity();
			if ( iCount > g_Cfg.m_iMaxSectorComplexity )
				g_Log.Event(LOGL_WARN, "%" PRIuSIZE_T " items at %s. Sector too complex!\n", iCount, pSector->GetBasePoint().WriteUsed());

			iCount = pSector->GetCharComplexity();
			if ( iCount > g_Cfg.m_iMaxCharComplexity )
				g_Log.Event(LOGL_WARN, "%" PRIuSIZE_T " chars at %s. Sector too complex!\n", iCount, pSector->GetBasePoint().WriteUsed());
		}
		EXC_CATCHSUB("Sector light levels");
	}

	EXC_TRYSUB("Load");
	GarbageCollection();
	EXC_CATCHSUB("Garbage collect");

	// Set the current version now.
	r_SetVal("VERSION", SPHERE_VERSION);	// Set m_iLoadVersion

	return true;
}

/////////////////////////////////////////////////////////////////

void CWorld::r_Write( CScript & s )
{
	ADDTOCALLSTACK("CWorld::r_Write");
	// Write out the safe header.
	s.WriteKey("TITLE", SPHERE_TITLE " World Script");
	s.WriteKey("VERSION", SPHERE_VERSION);
	#ifdef __GITREVISION__
		s.WriteKeyVal("PREVBUILD", __GITREVISION__);
	#endif
	s.WriteKeyVal( "TIME", GetCurrentTick() );
	s.WriteKeyVal( "SAVECOUNT", m_iSaveCountID );
	s.Flush();	// Force this out to the file now.
}

bool CWorld::r_GetRef( lpctstr & pszKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CWorld::r_GetRef");
	if ( ! strnicmp( pszKey, "LASTNEW", 7 ))
	{
		if ( ! strnicmp( pszKey+7, "ITEM", 4 ) )
		{
			pszKey += 11;
			SKIP_SEPARATORS(pszKey);
			pRef = m_uidLastNewItem.ItemFind();
			return true;
		}
		else if ( ! strnicmp( pszKey+7, "CHAR", 4 ) )
		{
			pszKey += 11;
			SKIP_SEPARATORS(pszKey);
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

bool CWorld::r_WriteVal( lpctstr pszKey, CSString &sVal, CTextConsole * pSrc )
{
	ADDTOCALLSTACK("CWorld::r_WriteVal");
	EXC_TRY("WriteVal");

	if ( !strnicmp(pszKey, "GMPAGE", 6) )		//	GM pages
	{
		pszKey += 6;
		if (( *pszKey == 'S' ) || ( *pszKey == 's' ))	//	SERV.GMPAGES
		{
			pszKey++;
			if ( *pszKey != '\0' )
				return false;

			sVal.FormatSTVal(m_GMPages.GetCount());
		}
		else if ( *pszKey == '.' )						//	SERV.GMPAGE.*
		{
			SKIP_SEPARATORS(pszKey);
			size_t index = Exp_GetVal(pszKey);
			if ( index >= m_GMPages.GetCount() )
				return false;

			SKIP_SEPARATORS(pszKey);
			CGMPage* pPage = static_cast <CGMPage*> (m_GMPages.GetAt(index));
			if ( pPage == nullptr )
				return false;

			if ( !strnicmp(pszKey, "HANDLED", 7) )
			{
				CClient *pClient = pPage->FindGMHandler();
				if ( pClient != nullptr && pClient->GetChar() != nullptr )
					sVal.FormatHex(pClient->GetChar()->GetUID());
				else
					sVal.FormatVal(0);
				return true;
			}
			else
				return pPage->r_WriteVal(pszKey, sVal, pSrc);
		}
		else
			sVal.FormatVal(0);
		return true;
	}

	switch ( FindTableSorted( pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 ))
	{
        case WC_CURTICK:
            sVal.Format64Val(GetCurrentTick());
            break;
		case WC_PREVBUILD:
			sVal.FormatVal(m_iPrevBuild);
			break;
		case WC_SAVECOUNT:  // "SAVECOUNT"
			sVal.FormatVal( m_iSaveCountID );
			break;
		case WC_TIME:	    // "TIME"
			sVal.FormatLLVal(GetCurrentTime().GetTimeRaw() / MSECS_PER_TENTH);  // in tenths of second, for backwards compatibility
			break;
        case WC_TIMEHIRES:	// "TIMEHIRES"
            sVal.FormatLLVal( GetCurrentTime().GetTimeRaw() );      // in milliseconds
            break;
		case WC_TITLE:      // "TITLE",
			sVal = (SPHERE_TITLE " World Script");
			break;
		case WC_VERSION:    // "VERSION"
			sVal = SPHERE_VERSION;
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

	lpctstr	pszKey = s.GetKey();
	switch ( FindTableSorted( pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys)-1 ))
	{
		case WC_PREVBUILD:
			m_iPrevBuild = s.GetArgVal();
			break;
		case WC_SAVECOUNT: // "SAVECOUNT"
			m_iSaveCountID = s.GetArgVal();
			break;
		case WC_TIME:	// "TIME"
			if ( ! g_Serv.IsLoading() )
			{
				DEBUG_WARN(( "Setting TIME while running is BAD!\n" ));
			}
			m_Clock.InitTime( s.GetArgLLVal());
            _iLastTick = m_Clock.GetCurrentTick();
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
	for ( int m = 0; m < 256; ++m )
	{
		if ( !g_MapList.m_maps[m] ) continue;

		for ( int s = 0; s < g_MapList.GetSectorQty(m); ++s )
		{
			CSector	*pSector = GetSector(m, s);

			if ( pSector )
				pSector->RespawnDeadNPCs();
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

	for ( int m = 0; m < 256; ++m )
	{
		if ( !g_MapList.m_maps[m] )
			continue;

		for ( int s = 0; s < g_MapList.GetSectorQty(m); ++s )
		{
			CSector	*pSector = GetSector(m, s);
			if ( pSector != nullptr )
				pSector->Restock();
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
	m_ObjStatusUpdates.clear();
	m_Parties.Clear();
	m_GMPages.Clear();

    // Disconnect the players, so that we have none of them in a sector
    ClientIterator it;
    for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
    {
        //pClient->GetNetState()->clear();
        pClient->GetNetState()->markReadClosed();
        pClient->CharDisconnect();
    }

	if ( m_Sectors != nullptr )
	{
		//	free memory allocated by sectors
		for ( uint s = 0; s < m_SectorsQty; ++s )
		{
			// delete everything in sector
			m_Sectors[s]->Close();
		}
		// do this in two loops because destructors of items
		// may access server sectors
		for ( uint s = 0; s < m_SectorsQty; ++s )
		{
			// delete the sectors
			delete m_Sectors[s];
			m_Sectors[s] = nullptr;
		}

		delete[] m_Sectors;
		m_Sectors = nullptr;
		m_SectorsQty = 0;
	}

	memset(g_MapList.m_maps, 0, sizeof(g_MapList.m_maps));

	if ( g_MapList.m_pMapDiffCollection != nullptr )
	{
		delete g_MapList.m_pMapDiffCollection;
		g_MapList.m_pMapDiffCollection = nullptr;
	}

	CloseAllUIDs();

	m_Clock.Init();	// no more sense of time.
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

void CWorld::Speak( const CObjBaseTemplate * pSrc, lpctstr pszText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	ADDTOCALLSTACK("CWorld::Speak");
	if ( !pszText || !pszText[0] )
		return;

	bool fSpeakAsGhost = false;
	if ( pSrc )
	{
		if ( pSrc->IsChar() )
		{
			const CChar *pSrcChar = static_cast<const CChar *>(pSrc);
			ASSERT(pSrcChar);

			// Are they dead? Garble the text. unless we have SpiritSpeak
			fSpeakAsGhost = pSrcChar->IsSpeakAsGhost();
		}
	}
	else
		mode = TALKMODE_BROADCAST;

	//CSString sTextUID;
	//CSString sTextName;	// name labelled text.
	CSString sTextGhost;	// ghost speak.

	// For things
	bool fCanSee = false;
	CChar * pChar = nullptr;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next(), fCanSee = false, pChar = nullptr)
	{
		if ( ! pClient->CanHear( pSrc, mode ) )
			continue;

		tchar * myName = Str_GetTemp();

		lpctstr pszSpeak = pszText;
		pChar = pClient->GetChar();

		if ( pChar != nullptr )
		{
			fCanSee = pChar->CanSee(pSrc);

			if ( fSpeakAsGhost && !pChar->CanUnderstandGhost() )
			{
				if ( sTextGhost.IsEmpty() )
				{
					sTextGhost = pszText;
					for ( int i = 0; i < sTextGhost.GetLength(); i++ )
					{
						if ( sTextGhost[i] != ' ' &&  sTextGhost[i] != '\t' )
							sTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
					}
				}
				pszSpeak = sTextGhost;
				pClient->addSound( sm_Sounds_Ghost[ Calc_GetRandVal( CountOf( sm_Sounds_Ghost )) ], pSrc );
			}

			if ( !fCanSee && pSrc )
			{
				//if ( sTextName.IsEmpty() )
				//{
				//	sTextName.Format("<%s>", pSrc->GetName());
				//}
				//myName = sTextName;
				if ( !*myName )
					sprintf(myName, "<%s>", pSrc->GetName());
			}
		}

		if ( ! fCanSee && pSrc && pClient->IsPriv( PRIV_HEARALL|PRIV_DEBUG ))
		{
			//if ( sTextUID.IsEmpty() )
			//{
			//	sTextUID.Format("<%s [%x]>", pSrc->GetName(), (dword)pSrc->GetUID());
			//}
			//myName = sTextUID;
			if ( !*myName )
				sprintf(myName, "<%s [%x]>", pSrc->GetName(), (dword)pSrc->GetUID());
		}

		if (*myName)
			pClient->addBarkParse( pszSpeak, pSrc, wHue, mode, font, false, myName );
		else
			pClient->addBarkParse( pszSpeak, pSrc, wHue, mode, font );
	}
}

void CWorld::SpeakUNICODE( const CObjBaseTemplate * pSrc, const nchar * pwText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ADDTOCALLSTACK("CWorld::SpeakUNICODE");
	bool fSpeakAsGhost = false;
	const CChar * pSrcChar = nullptr;

	if ( pSrc )
	{
		if ( pSrc->IsChar() )
		{
			pSrcChar = static_cast<const CChar *>(pSrc);
			ASSERT(pSrcChar);

			// Are they dead? Garble the text. unless we have SpiritSpeak
			fSpeakAsGhost = pSrcChar->IsSpeakAsGhost();
		}
	}
	else
		mode = TALKMODE_BROADCAST;

	if (mode != TALKMODE_SPELL)
	{
		if (pSrcChar && pSrcChar->m_SpeechHueOverride)
		{
			// This hue overwriting part for ASCII text isn't done in CWorld::Speak but in CClient::AddBarkParse.
			// If a specific hue is not given, use SpeechHueOverride.
			wHue = pSrcChar->m_SpeechHueOverride;
		}
	}

	nchar wTextUID[MAX_TALK_BUFFER];	// uid labelled text.
	wTextUID[0] = '\0';
	nchar wTextName[MAX_TALK_BUFFER];	// name labelled text.
	wTextName[0] = '\0';
	nchar wTextGhost[MAX_TALK_BUFFER];	// ghost speak.
	wTextGhost[0] = '\0';

	// For things
	bool fCanSee = false;
	CChar * pChar = nullptr;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next(), fCanSee = false, pChar = nullptr)
	{
		if ( ! pClient->CanHear( pSrc, mode ) )
			continue;

		const nchar * pwSpeak = pwText;
		pChar = pClient->GetChar();

		if ( pChar != nullptr )
		{
			// Cansee?
			fCanSee = pChar->CanSee( pSrc );

			if ( fSpeakAsGhost && ! pChar->CanUnderstandGhost() )
			{
				if ( wTextGhost[0] == '\0' )	// Garble ghost.
				{
					size_t i;
					for ( i = 0; i < MAX_TALK_BUFFER - 1 && pwText[i]; ++i )
					{
						if ( pwText[i] != ' ' && pwText[i] != '\t' )
							wTextGhost[i] = Calc_GetRandVal(2) ? 'O' : 'o';
						else
							wTextGhost[i] = pwText[i];
					}
					wTextGhost[i] = '\0';
				}
				pwSpeak = wTextGhost;
				pClient->addSound( sm_Sounds_Ghost[ Calc_GetRandVal( CountOf( sm_Sounds_Ghost )) ], pSrc );
			}

			// Must label the text.
			if ( ! fCanSee && pSrc )
			{
				if ( wTextName[0] == '\0' )
				{
					CSString sTextName;
					sTextName.Format("<%s>", pSrc->GetName());
					int iLen = CvtSystemToNUNICODE( wTextName, CountOf(wTextName), sTextName, -1 );
					if ( wTextGhost[0] != '\0' )
					{
						for ( int i = 0; wTextGhost[i] != '\0' && iLen < MAX_TALK_BUFFER; i++, iLen++ )
							wTextName[iLen] = wTextGhost[i];
					}
					else
					{
						for ( int i = 0; pwText[i] != 0 && iLen < MAX_TALK_BUFFER - 1; i++, iLen++ )
							wTextName[iLen] = pwText[i];
					}
					wTextName[iLen] = '\0';
				}
				pwSpeak = wTextName;
			}
		}

		if ( ! fCanSee && pSrc && pClient->IsPriv( PRIV_HEARALL|PRIV_DEBUG ))
		{
			if ( wTextUID[0] == '\0' )
			{
				tchar * pszMsg = Str_GetTemp();
				sprintf(pszMsg, "<%s [%x]>", pSrc->GetName(), (dword)pSrc->GetUID());
				int iLen = CvtSystemToNUNICODE( wTextUID, CountOf(wTextUID), pszMsg, -1 );
				for ( int i = 0; pwText[i] && iLen < MAX_TALK_BUFFER - 1; i++, iLen++ )
					wTextUID[iLen] = pwText[i];
				wTextUID[iLen] = '\0';
			}
			pwSpeak = wTextUID;
		}

		pClient->addBarkUNICODE( pwSpeak, pSrc, wHue, mode, font, lang );
	}
}

void CWorld::Broadcast(lpctstr pMsg) // System broadcast in bold text
{
	ADDTOCALLSTACK("CWorld::Broadcast");
	Speak( nullptr, pMsg, HUE_TEXT_DEF, TALKMODE_BROADCAST, FONT_BOLD );
}

void __cdecl CWorld::Broadcastf(lpctstr pMsg, ...) // System broadcast in bold text
{
	ADDTOCALLSTACK("CWorld::Broadcastf");
	TemporaryString tsTemp;
	tchar* pszTemp = static_cast<tchar *>(tsTemp);
	va_list vargs;
	va_start(vargs, pMsg);
	vsnprintf(pszTemp, tsTemp.realLength(), pMsg, vargs);
	va_end(vargs);
	Broadcast(pszTemp);
}

//////////////////////////////////////////////////////////////////
// Game time.

int64 CWorld::GetGameWorldTime( int64 basetime ) const
{
	ADDTOCALLSTACK("CWorld::GetGameWorldTime");
	// Get the time of the day in GameWorld minutes.
    // basetime = ticks.
	// 8 real world seconds = 1 game minute.
	// 1 real minute = 7.5 game minutes
	// 3.2 hours = 1 game day.
    
	return( basetime / g_Cfg.m_iGameMinuteLength );
}

int64 CWorld::GetNextNewMoon( bool bMoonIndex ) const
{
	ADDTOCALLSTACK("CWorld::GetNextNewMoon");
	// "Predict" the next new moon for this moon
	// Get the period
	int64 iSynodic = bMoonIndex ? FELUCCA_SYNODIC_PERIOD : TRAMMEL_SYNODIC_PERIOD;

	// Add a "month" to the current game time
	int64 iNextMonth = GetGameWorldTime() + iSynodic;

	// Get the game time when this cycle will start
	int64 iNewStart = (int64)(iNextMonth - (double)(iNextMonth % iSynodic));
	return iNewStart * g_Cfg.m_iGameMinuteLength;
	
}

uint CWorld::GetMoonPhase (bool bMoonIndex) const
{
	ADDTOCALLSTACK("CWorld::GetMoonPhase ");
	// bMoonIndex is FALSE if we are looking for the phase of Trammel,
	// TRUE if we are looking for the phase of Felucca.

	// There are 8 distinct moon phases:  New, Waxing Crescent, First Quarter, Waxing Gibbous,
	// Full, Waning Gibbous, Third Quarter, and Waning Crescent

	// To calculate the phase, we use the following formula:
	//				CurrentTime % SynodicPeriod
	//	Phase = 	-----------------------------------------     * 8
	//			              SynodicPeriod
	//

	int64 dwCurrentTime = GetGameWorldTime();	// game world time in minutes

	if (!bMoonIndex)	// Trammel

		return IMulDiv( dwCurrentTime % TRAMMEL_SYNODIC_PERIOD, 8, TRAMMEL_SYNODIC_PERIOD );
	else	// Luna2

		return IMulDiv( dwCurrentTime % FELUCCA_SYNODIC_PERIOD, 8, FELUCCA_SYNODIC_PERIOD );
}

void CWorld::OnTick()
{
	ADDTOCALLSTACK("CWorld::OnTick");
	// 256 real secs = 1 server hour. 19 light levels. check every 10 minutes or so.

    // Do not tick while loading (startup, resync, exiting...) or when double ticking in the same msec?.
	if ( g_Serv.IsLoading() || !m_Clock.Advance() )
		return;

    EXC_TRY("CWorld::OnTick");
    EXC_SET_BLOCK("Once per tick stuff");
    // Do this once per tick.
    // Update status flags from objects, update current tick.
    if (_iLastTick <= GetCurrentTick())
    {
        ++_iLastTick;   // Update current tick.

        /* process objects that need status updates
        * these objects will normally be in containers which don't have any period OnTick method
        * called (whereas other items can receive the OnTickStatusUpdate() call via their normal
        * tick method).
        * note: ideally, a better solution to accomplish this should be found if possible
        * TODO: implement a new class inheriting from CCTimedObject to get rid of this code.
        */
        if (!m_ObjStatusUpdates.empty())
        {
            EXC_TRYSUB("Tick::StatusUpdates");

            // loop backwards to avoid possible infinite loop if a status update is triggered
            // as part of the status update (e.g. property changed under tooltip trigger)
            for (CObjBase * pObj : m_ObjStatusUpdates)
            {
                if (pObj != nullptr)
                    pObj->OnTickStatusUpdate();
            }
            m_ObjStatusUpdates.clear();

            EXC_CATCHSUB("StatusUpdates");
        }


        // TimerF
        EXC_TRYSUB("Tick::TimerF");
        m_TimedFunctions.OnTick();
        EXC_CATCHSUB("CTimedFunction");
    }

    /* World ticking (timers) */
    // Items, Chars ... Everything relying on CCTimedObject (excepting CObjBase, which inheritance is only virtual)
    const int64 iCurTime = CServerTime::GetCurrentTime().GetTimeRaw();    // Current timestamp, a few msecs will advance in the current tick ... avoid them until the following tick(s).

    EXC_SET_BLOCK("WorldObjects selection");
    ProfileTask timersTask(PROFILE_TIMERS);
    std::map<int64, std::vector<CCTimedObject*>> tmpMap;
    {
        // Need here a new, inner scope to get rid of EXC_TRYSUB variables and for the unique_lock
        EXC_TRYSUB("Tick::WorldObj");
        std::unique_lock<std::shared_mutex> lock(_mWorldTickList.THREAD_CMUTEX);
        std::unique_lock<std::shared_mutex> lockLookup(_mWorldTickLookup.THREAD_CMUTEX);
        std::map<int64, TimedObjectsContainer>::iterator it = _mWorldTickList.begin();
        const std::map<int64, TimedObjectsContainer>::iterator itEnd = _mWorldTickList.end();
        int64 iTime;
        while ( (it != itEnd) && (iCurTime > (iTime = it->first)))
        {
            TimedObjectsContainer& cont = it->second;
            std::shared_lock<std::shared_mutex> lockCont(cont.THREAD_CMUTEX);
            
            for (CCTimedObject* pTimedObj : cont)
            {
                if (_mWorldTickLookup.erase(pTimedObj) == 0)    // Double check: ensure this object exists also in the lookup cont
                {
                    it = _mWorldTickList.erase(it);
                    continue;
                }
            }
            tmpMap[iTime] = cont;
            it = _mWorldTickList.erase(it);
        }
        EXC_CATCHSUB("Reading from _mWorldTickList");
    }

    EXC_SET_BLOCK("WorldObjects loop");
    for (const auto &pairObj : tmpMap)    // Loop through all msecs stored, unless we passed the timestamp.
    {
        lpctstr ptcSubDesc = "\0";
        for (CCTimedObject* pObj : pairObj.second)
        {
            EXC_TRYSUB("Tick::WorldObj");
            EXC_SETSUB_BLOCK("Elapsed");
            ptcSubDesc = "Generic";
            const PROFILE_TYPE profile = pObj->GetProfileType();
            ProfileTask profileTask(profile);
            
            /*
            * Doing a SetTimeout() in the object's tick will force CWorld to search for that object's
            * current timeout to remove it from any list, prevent that to happen here since it should
            * not belong to any other tick than the current one.
            */
            if (pObj->IsSleeping()) // Ignore what is sleeping.
            {
                continue;
            }
            pObj->ClearTimeout();
            bool fRemove = true;    // Default to true, so if any error occurs it gets deleted for safety.
            switch (profile)
            {
                case PROFILE_ITEMS:
                {
                    ptcSubDesc = "Item (Generic)";
                    CItem *pItem = dynamic_cast<CItem*>(pObj);
                    ptcSubDesc = "Item (Casted)";
                    ASSERT(pItem);
                    if (pItem->IsItemEquipped())
                    {
                        ptcSubDesc = "ItemEquipped (CObjBaseTemplate)";
                        CObjBaseTemplate* pObjTop = pItem->GetTopLevelObj();
                        ptcSubDesc = "ItemEquipped (CObjBaseTemplateCasted)";
                        ASSERT(pObjTop);
                        CChar *pChar = dynamic_cast<CChar*>(pObjTop);
                        ASSERT(pChar);
                        ptcSubDesc = "ItemEquipped (CCharCasted)";
                        fRemove = !pChar->OnTickEquip(pItem);
                        break;
                    }
                    else
                    {
                        ptcSubDesc = "Item";
                        fRemove = (pObj->OnTick() == false);
                        break;
                    }
                }
                break;
                case PROFILE_CHARS:
                {
                    ptcSubDesc = "Char";
                    fRemove = !pObj->OnTick();
                    ptcSubDesc = "Char (PostTick)";
                    CChar* pChar = dynamic_cast<CChar*>(pObj);
                    ptcSubDesc = "Char (Casted)";
                    ASSERT(pChar);
                    if (pChar->m_pNPC && !pObj->IsTimerSet())
                    {
                        pObj->SetTimeoutS(3);   //3 seconds timeout to keep NPCs 'alive'
                    }
                }
                break;
                case PROFILE_SECTORS:
                {
                    ptcSubDesc = "Sector";
                    fRemove = false;    // sectors should NEVER be deleted.
                    pObj->OnTick();
                }
                break;
                case PROFILE_MULTIS:
                {
                    ptcSubDesc = "Multi";
                    CItemMulti *pMulti = dynamic_cast<CItemMulti*>(pObj);
                    ptcSubDesc = "Multi (Casted)";
                    ASSERT(pMulti);
                    fRemove = !pMulti->OnTick();
                }
                break;
                case PROFILE_SHIPS:
                {
                    ptcSubDesc = "Ship";
                    CItem* pItem = dynamic_cast<CItem*>(pObj);
                    ptcSubDesc = "Ship (CItem Casted)";
                    ASSERT(pItem); UNREFERENCED_PARAMETER(pItem);
                    ASSERT(dynamic_cast<CItemShip*>(pItem));
                    ptcSubDesc = "Ship (CItemShip Casted)";
                    fRemove = !pObj->OnTick();
                }
                break;
                default:
                {
                    ptcSubDesc = "Default";
                    fRemove = !pObj->OnTick(); // do tick.
                }
                break;
            }
            if (fRemove)
            {
                EXC_SETSUB_BLOCK("Delete");
                CObjBase* pObjBase = dynamic_cast<CObjBase*>(pObj);
                ASSERT(pObjBase);
                pObjBase->Delete();
            }
            EXC_CATCHSUB(ptcSubDesc);
        }
    }


    /* Periodic, automatic ticking for every char */

    EXC_SET_BLOCK("PeriodicChars selection");
    ProfileTask taskChars(PROFILE_CHARS);
    std::map<int64, std::vector<CChar*>> tmpCharMap;
    {
        // Need here a new, inner scope to get rid of EXC_TRYSUB variables, and for the unique_lock
        EXC_TRYSUB("Tick::PeriodicChar");
        std::unique_lock<std::shared_mutex> lock(_mCharTickList.THREAD_CMUTEX);
        std::unique_lock<std::shared_mutex> lockLookup(_mCharTickLookup.THREAD_CMUTEX);
        std::map<int64, TimedCharsContainer>::iterator charIt = _mCharTickList.begin();
        const std::map<int64, TimedCharsContainer>::iterator charItEnd = _mCharTickList.end();
        int64 iTime;
        while ((charIt != charItEnd) && (iCurTime > (iTime = charIt->first)))
        {
            TimedCharsContainer& cont = charIt->second;
            std::shared_lock<std::shared_mutex> lockCont(cont.THREAD_CMUTEX);

            for (CChar* pChar : cont)
            {
                if (_mCharTickLookup.erase(pChar) == 0) // Double check: ensure this object exists also in the lookup cont
                {
                    charIt = _mCharTickList.erase(charIt);
                    continue;
                }
            }
            tmpCharMap[iTime] = cont;
            charIt = _mCharTickList.erase(charIt);
            
        }
        EXC_CATCHSUB("Reading from _mCharTickList");
    }

    EXC_SET_BLOCK("PeriodicChars loop");
    for (const auto &pairChar : tmpCharMap)    // Loop through all msecs stored, unless we passed the timestamp.
    {
        EXC_TRYSUB("Tick::PeriodicChar::Elapsed");
        for (CChar* pChar : pairChar.second)
        {
            if (pChar->OnTickPeriodic())
            {
                AddCharTicking(pChar);
            }
            else
            {
                pChar->Delete();
            }
        }
        EXC_CATCHSUB("");
    }

    m_ObjDelete.Clear();	// clean up our delete list (this DOES delete the objects, thanks to the virtual destructors).


    // Save state checks
    // Notifications
	if ( (m_bSaveNotificationSent == false) && ((m_timeSave - (10 * MSECS_PER_SEC)) <= GetCurrentTime().GetTimeRaw()) )
	{
		Broadcast( g_Cfg.GetDefaultMsg( DEFMSG_SERVER_WORLDSAVE_NOTIFY ) );
		m_bSaveNotificationSent = true;
	}

    // Save
	if ( m_timeSave <= GetCurrentTime().GetTimeRaw())
	{
		// Auto save world
		m_timeSave = GetCurrentTime().GetTimeRaw() + g_Cfg.m_iSavePeriod;
		g_Log.Flush();
		Save( false );
	}

    // Global (ini) stuff.
    // Respawn Dead NPCs
	if ( m_timeRespawn <= GetCurrentTime().GetTimeRaw())
	{
		// Time to regen all the dead NPC's in the world.
		m_timeRespawn = GetCurrentTime().GetTimeRaw() + (20 * 60 * MSECS_PER_SEC);
		RespawnDeadNPCs();
	}

    // f_onserver_timer function.
	if ( m_timeCallUserFunc < GetCurrentTime().GetTimeRaw())
	{
		if ( g_Cfg._iTimerCall )
		{
			m_timeCallUserFunc = GetCurrentTime().GetTimeRaw() + g_Cfg._iTimerCall;
			CScriptTriggerArgs args(g_Cfg._iTimerCall/(60 * MSECS_PER_SEC));
			g_Serv.r_Call("f_onserver_timer", &g_Serv, &args);
		}
	}

    EXC_CATCH;
}

CSector *CWorld::GetSector(int map, int i) const	// gets sector # from one map
{
	ADDTOCALLSTACK_INTENSIVE("CWorld::GetSector");

	// if the map is not supported, return empty sector
	if (( map < 0 ) || ( map >= 256 ) || !g_MapList.m_maps[map] )
		return nullptr;

	if ( i >= g_MapList.GetSectorQty(map) )
	{
		g_Log.EventError("Unsupported sector #%d for map #%d specified.\n", i, map);
		return nullptr;
	}

	int base = 0;
	for ( int m = 0; m < 256; ++m )
	{
		if ( !g_MapList.m_maps[m] )
			continue;

		if ( m == map )
		{
			if ( g_MapList.GetSectorQty(map) < i )
				return nullptr;

			return m_Sectors[base + i];
		}

		base += g_MapList.GetSectorQty(m);
	}
	return nullptr;
}
