/**
* @file CWorld.h
*
*/

#pragma once
#ifndef _INC_CWORLD_H
#define _INC_CWORLD_H

#include "../common/common.h"
#include "../common/sphere_library/CSArray.h"
#include "../common/CScript.h"
#include "../common/CScriptObj.h"
#include "../common/CUID.h"
#include "items/CItemBase.h"
#include "CServerTime.h"
#include "CSector.h"

class CObjBase;
class CItemStone;
class CItemTypeDef;
class CObjBaseTemplate;


enum IMPFLAGS_TYPE	// IMPORT and EXPORT flags.
{
	IMPFLAGS_NOTHING = 0,
	IMPFLAGS_ITEMS = 0x01,		// 0x01 = items,
	IMPFLAGS_CHARS = 0x02,		// 0x02 = characters
	IMPFLAGS_BOTH  = 0x03,		// 0x03 = both
	IMPFLAGS_PLAYERS = 0x04,
	IMPFLAGS_RELATIVE = 0x10,	// 0x10 = distance relative. dx, dy to you
	IMPFLAGS_ACCOUNT = 0x20		// 0x20 = recover just this account/char	(and all it is carrying)
};

#define FREE_UIDS_SIZE	500000	//	the list of free empty slots in the uids list should contain these elements


class CWorldThread
{
	// A regional thread of execution. hold all the objects in my region.
	// as well as those just created here. (but may not be here anymore)

protected:
	CSObjArray<CObjBase*> m_UIDs;	// all the UID's in the World. CChar and CItem.
	int m_iUIDIndexLast;		// remeber the last index allocated so we have more even usage.

	dword	*m_FreeUIDs;		//	list of free uids available
	dword	m_FreeOffset;		//	offset of the first free element

public:
	static const char *m_sClassName;
	//int m_iThreadNum;	// Thread number to id what range of UID's i own.
	//CRectMap m_ThreadRect;	// the World area that this thread owns.

	//int m_iUIDIndexBase;		// The start of the uid range that i will allocate from.
	//int m_iUIDIndexMaxQty;	// The max qty of UIDs i can allocate.

	CSObjList m_ObjNew;			// Obj created but not yet placed in the world.
	CSObjList m_ObjDelete;		// Objects to be deleted.

	// Background save. Does this belong here ?
	CScript m_FileData;			// Save or Load file.
	CScript m_FileWorld;		// Save or Load file.
	CScript m_FilePlayers;		// Save of the players chars.
	CScript m_FileMultis;		// Save of the custom multis.
	bool	m_fSaveParity;		// has the sector been saved relative to the char entering it ?

public:
	// Backgound Save
	bool IsSaving() const;

	// UID Managenent
	dword GetUIDCount() const;
#define UID_PLACE_HOLDER (reinterpret_cast<CObjBase*>(INTPTR_MAX))
	CObjBase * FindUID(dword dwIndex) const;
	void FreeUID(dword dwIndex);
	dword AllocUID( dword dwIndex, CObjBase * pObj );

	int FixObjTry( CObjBase * pObj, dword dwUID = 0 );
	int FixObj( CObjBase * pObj, dword dwUID = 0 );

	void SaveThreadClose();
	void GarbageCollection_UIDs();
	void GarbageCollection_New();

	void CloseAllUIDs();

public:
	CWorldThread();
	virtual ~CWorldThread();

private:
	CWorldThread(const CWorldThread& copy);
	CWorldThread& operator=(const CWorldThread& other);
};

class CWorldClock
{
#ifndef _WIN32
	#ifdef CLOCKS_PER_SEC
		#undef CLOCKS_PER_SEC
	#endif	// CLOCKS_PER_SEC
	#define CLOCKS_PER_SEC 1000	// must be converted from some internal clock.
#endif

private:
	CServerTime m_timeClock;		// the current relative tick time (in TICK_PER_SEC)
	int64 m_Clock_SysPrev;		// System time of the last OnTick() (in CLOCKS_PER_SEC)
public:
	static const char *m_sClassName;
	CWorldClock()
	{
		Init();
	}

private:
	CWorldClock(const CWorldClock& copy);
	CWorldClock& operator=(const CWorldClock& other);

public:
	void Init();
	void InitTime( int64 lTimeBase );
	bool Advance();
	CServerTime GetCurrentTime() const	// TICK_PER_SEC
	{
		return m_timeClock;
	}
	static int64 GetSystemClock();		// CLOCKS_PER_SEC
};

class CTimedFunctionHandler
{
	public:
		struct TimedFunction
		{
			int		elapsed;
			char	funcname[1024];
			CUID 	uid;
		};

	private:
		std::vector<TimedFunction *> m_timedFunctions[TICK_PER_SEC];
		int m_curTick;
		int m_processedFunctionsPerTick;
		std::vector<TimedFunction *> m_tfRecycled;
		std::vector<TimedFunction *> m_tfQueuedToBeAdded;
		bool m_isBeingProcessed;

	public:
		static const char *m_sClassName;
		CTimedFunctionHandler();

	private:
		CTimedFunctionHandler(const CTimedFunctionHandler& copy);
		CTimedFunctionHandler& operator=(const CTimedFunctionHandler& other);

	public:
		void OnTick();
		void r_Write( CScript & s );
		
		int Load( const char *pszName, bool fQuoted, const char *pszVal );
		void Add( CUID uid, int numSeconds, lpctstr funcname );
		void Erase( CUID uid );
		void Stop( CUID uid, lpctstr funcname );
		TRIGRET_TYPE Loop(lpctstr funcname, int LoopsMade, CScriptLineContext StartContext, CScriptLineContext EndContext, CScript &s, CTextConsole * pSrc, CScriptTriggerArgs * pArgs, CSString * pResult);
		int IsTimer( CUID uid, lpctstr funcname );
};

extern class CWorld : public CScriptObj, public CWorldThread
{
	// the world. Stuff saved in *World.SCP
private:
	// Clock stuff. how long have we been running ? all i care about.
	CWorldClock m_Clock;		// the current relative tick time  (in TICK_PER_SEC)

	// Special purpose timers.
	CServerTime	m_nextTickTime;			// next time to do sector stuff.
	CServerTime	m_timeSave;				// when to auto save ?
	bool		m_bSaveNotificationSent;// has notification been sent?
	CServerTime	m_timeRespawn;			// when to res dead NPC's ?
	CServerTime	m_timeCallUserFunc;		// when to call next user func
	uint		m_Sector_Pulse;			// Slow some stuff down that doesn't need constant processing.
	ullong		m_ticksWithoutMySQL;	// MySQL should be running constantly if MySQLTicks is true, keep here record of how much ticks since Sphere is not connected.

	int m_iSaveStage;	// Current stage of the background save.
	llong	m_savetimer; // Time it takes to save

public:
	static const char *m_sClassName;
	// World data.
	CSector **m_Sectors;
	uint m_SectorsQty;

public:
	int m_iSaveCountID;			// Current archival backup id. Whole World must have this same stage id
	int m_iLoadVersion;			// Previous load version. (only used during load of course)
	int m_iPrevBuild;			// Previous __GITREVISION__
	CServerTime m_timeStartup;	// When did the system restore load/save ?

	CUID m_uidLastNewItem;	// for script access.
	CUID m_uidLastNewChar;	// for script access.
	CUID m_uidObj;			// for script access - auxiliary obj
	CUID m_uidNew;			// for script access - auxiliary obj

	CSObjList m_GMPages;		// Current outstanding GM pages. (CGMPage)

	
	CSPtrTypeArray<CItemStone*> m_Stones;	// links to leige/town stones. (not saved array)
	CSObjList m_Parties;	// links to all active parties. CPartyDef

	static lpctstr const sm_szLoadKeys[];
	CSPtrTypeArray <CItemTypeDef *> m_TileTypes;

	// TimedFunction Container/Wrapper
	CTimedFunctionHandler m_TimedFunctions;
	CSPtrTypeArray<CObjBase*> m_ObjStatusUpdates; // objects that need OnTickStatusUpdate called

private:
	bool LoadFile( lpctstr pszName, bool fError = true );
	bool LoadWorld();

	bool SaveTry(bool fForceImmediate); // Save world state
	bool SaveStage();
	static void GetBackupName( CSString & sArchive, lpctstr pszBaseDir, tchar chType, int savecount );
	bool SaveForce(); // Save world state

public:
	CWorld();
	~CWorld();

private:
	CWorld(const CWorld& copy);
	CWorld& operator=(const CWorld& other);

public:
	void Init();

	CSector *GetSector( int map, int i );	// gets sector # from one map

	// Time

	CServerTime GetCurrentTime() const
	{
		return m_Clock.GetCurrentTime();  // Time in TICK_PER_SEC
	}
	int64 GetTimeDiff( CServerTime time ) const
	{
		// How long till this event
		// negative = in the past.
		return time.GetTimeDiff( GetCurrentTime() ); // Time in TICK_PER_SEC
	}

#define TRAMMEL_SYNODIC_PERIOD 105 // in game world minutes
#define FELUCCA_SYNODIC_PERIOD 840 // in game world minutes
#define TRAMMEL_FULL_BRIGHTNESS 2 // light units LIGHT_BRIGHT
#define FELUCCA_FULL_BRIGHTNESS 6 // light units LIGHT_BRIGHT
	uint GetMoonPhase( bool bMoonIndex = false ) const;
	CServerTime GetNextNewMoon( bool bMoonIndex ) const;

	dword GetGameWorldTime( CServerTime basetime ) const;
	dword GetGameWorldTime() const	// return game world minutes
	{
		return( GetGameWorldTime( GetCurrentTime()));
	}

	// CSector World Map stuff.
	void GetHeightPoint2( const CPointMap & pt, CServerMapBlockState & block, bool fHouseCheck = false );
	char GetHeightPoint2(const CPointBase & pt, dword & wBlockFlags, bool fHouseCheck = false); // Height of player who walked to X/Y/OLDZ

	void GetHeightPoint( const CPointMap & pt, CServerMapBlockState & block, bool fHouseCheck = false );
	char GetHeightPoint( const CPointBase & pt, dword & wBlockFlags, bool fHouseCheck = false );

	void GetFixPoint( const CPointMap & pt, CServerMapBlockState & block);

	CItemTypeDef *	GetTerrainItemTypeDef( dword dwIndex );
	IT_TYPE			GetTerrainItemType( dword dwIndex );

	const CServerMapBlock * GetMapBlock( const CPointMap & pt )
	{
		return( pt.GetSector()->GetMapBlock(pt));
	}
	const CUOMapMeter * GetMapMeter( const CPointMap & pt ) const // Height of MAP0.MUL at given coordinates
	{
		const CServerMapBlock * pMapBlock = pt.GetSector()->GetMapBlock(pt);
		if ( !pMapBlock )
			return NULL;
		return( pMapBlock->GetTerrain( UO_BLOCK_OFFSET(pt.m_x), UO_BLOCK_OFFSET(pt.m_y)));
	}

	CPointMap FindItemTypeNearby( const CPointMap & pt, IT_TYPE iType, int iDistance = 0, bool bCheckMulti = false, bool bLimitZ = false );
	bool IsItemTypeNear( const CPointMap & pt, IT_TYPE iType, int iDistance, bool bCheckMulti );

	CPointMap FindTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance = 0 );
	bool IsTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance = 0 );
	CItem * CheckNaturalResource( const CPointMap & pt, IT_TYPE Type, bool fTest = true, CChar * pCharSrc = NULL );

	static bool OpenScriptBackup( CScript & s, lpctstr pszBaseDir, lpctstr pszBaseName, int savecount );

	void r_Write( CScript & s );
	bool r_WriteVal( lpctstr pszKey, CSString &sVal, CTextConsole * pSrc );
	bool r_LoadVal( CScript & s ) ;
	bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );

	void OnTick();
	void OnTickMySQL();

	void GarbageCollection();
	void Restock();
	void RespawnDeadNPCs();

	void Speak( const CObjBaseTemplate * pSrc, lpctstr pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_BOLD );
	void SpeakUNICODE( const CObjBaseTemplate * pSrc, const nchar * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );

	void Broadcast( lpctstr pMsg );
	void __cdecl Broadcastf( lpctstr pMsg, ...) __printfargs(2,3);

	bool Export( lpctstr pszFilename, const CChar* pSrc, word iModeFlags = IMPFLAGS_ITEMS, int iDist = INT16_MAX, int dx = 0, int dy = 0 );
	bool Import( lpctstr pszFilename, const CChar* pSrc, word iModeFlags = IMPFLAGS_ITEMS, int iDist = INT16_MAX, tchar *pszAgs1 = NULL, tchar *pszAgs2 = NULL );
	bool Save( bool fForceImmediate ); // Save world state
	void SaveStatics();
	bool LoadAll();
	bool DumpAreas( CTextConsole * pSrc, lpctstr pszFilename );
	void Close();

	lpctstr GetName() const { return( "World" ); }

} g_World;

class CWorldSearch	// define a search of the world.
{
private:
	const CPointMap m_pt;		// Base point of our search.
	const int m_iDist;			// How far from the point are we interested in
	bool m_fAllShow;		// Include Even inert items.
	bool m_fSearchSquare;		// Search in a square (uo-sight distance) rather than a circle (standard distance).

	CObjBase * m_pObj;	// The current object of interest.
	CObjBase * m_pObjNext;	// In case the object get deleted.
	bool m_fInertToggle;		// We are now doing the inert items

	CSector * m_pSectorBase;	// Don't search the center sector 2 times.
	CSector * m_pSector;	// current Sector
	CRectMap m_rectSector;		// A rectangle containing our sectors we can search.
	int		m_iSectorCur;		// What is the current Sector index in m_rectSector
private:
	bool GetNextSector();
public:
	static const char *m_sClassName;
	explicit CWorldSearch( const CPointMap & pt, int iDist = 0 );
private:
	CWorldSearch(const CWorldSearch& copy);
	CWorldSearch& operator=(const CWorldSearch& other);

public:
	void SetAllShow( bool fView );
	void SetSearchSquare( bool fSquareSearch );
	void RestartSearch();		// Setting current obj to NULL will restart the search 
	CChar * GetChar();
	CItem * GetItem();
};

inline CServerTime CServerTime::GetCurrentTime()	// static
{
	return g_World.GetCurrentTime();
}


#endif // _INC_CWORLD_H
