/**
* @file CWorld.h
*
*/

#ifndef _INC_CWORLD_H
#define _INC_CWORLD_H

#include "../common/sphere_library/CSObjCont.h"
#include "../common/sphere_library/CSObjList.h"
#include "../common/sphere_library/sptr_containers.h"
#include "../common/CScript.h"
#include "../common/CUID.h"
#include "CSectorList.h"
#include "CWorldCache.h"
#include "CWorldClock.h"
#include "CWorldTicker.h"


class CGMPage;
class CPartyDef;
class CObjBaseTemplate;
class CObjBase;
class CItemStone;
class CItemMulti;
class CResourceDef;
class CSector;


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
	CObjBase**	_ppUIDObjArray;		// Array containing all the UID's in the World. CChar and CItem.
	size_t		_uiUIDObjArraySize;
	dword		_dwUIDIndexLast;	// remeber the last index allocated so we have more even usage.

	dword*		_pdwFreeUIDs;		// list of free uids available
	dword		_dwFreeUIDOffset;		// offset of the first free element

protected:
	static const char *m_sClassName;
	//int m_iThreadNum;	// Thread number to id what range of UID's i own.
	//CRectMap m_ThreadRect;	// the World area that this thread owns.

	//int m_iUIDIndexBase;		// The start of the uid range that i will allocate from.
	//int m_iUIDIndexMaxQty;	// The max qty of UIDs i can allocate.

	CSObjCont m_ObjNew;			// Obj created but not yet placed in the world.
	CSObjCont m_ObjDelete;		// Objects to be deleted.
	CSObjList m_ObjSpecialDelete;

public:
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
    #define UID_PLACE_HOLDER (reinterpret_cast<CObjBase*>(INTPTR_MAX))
	dword GetUIDCount() const;
	CObjBase * FindUID(dword dwIndex) const;
	void FreeUID(dword dwIndex);
	dword AllocUID( dword dwIndex, CObjBase * pObj );

	void AddIdleObj(CSObjContRec* obj);
	void ScheduleObjDeletion(CSObjContRec* obj);
	void ScheduleSpecialObjDeletion(CSObjListRec* obj);
	NODISCARD bool IsObjIdle(const CSObjContRec* obj) const noexcept;
	NODISCARD bool IsScheduledObjDeletion(const CSObjContRec* obj) const noexcept;
	NODISCARD bool IsScheduledSpecialObjDeletion(const CSObjListRec* obj) const noexcept;


    NODISCARD inline const CSObjCont* GetObjectsNew() noexcept {
        return &m_ObjNew;
    }
    /*
    NODISCARD inline const CSObjCont* GetObjectsDeleting() noexcept {
        return &m_ObjDelete;
    }
    NODISCARD inline const CSObjList* GetObjectsSpecialDeleting() noexcept {
        return &m_ObjSpecialDelete;
    }
    */

	int FixObjTry( CObjBase * pObj, dword dwUID = 0 );
	int FixObj( CObjBase * pObj, dword dwUID = 0 );

	void SaveThreadClose();
	void GarbageCollection_UIDs();
	void GarbageCollection_NewObjs();

	void InitUIDs();
	void CloseAllUIDs();

public:
	CWorldThread();
	virtual ~CWorldThread();

private:
	CWorldThread(const CWorldThread& copy);
	CWorldThread& operator=(const CWorldThread& other);
};


extern class CWorld : public CScriptObj, public CWorldThread
{
	// the world. Stuff saved in *World.SCP
public:
	static const char* m_sClassName;

// Helper classes
private:
	// Clock stuff. how long have we been running ? all i care about.
	friend class CWorldGameTime;
	friend CServerTime;
	CWorldClock _GameClock;		// the current relative tick time (in milliseconds)

	// Ticking world objects
	friend class CWorldTickingList;
	friend class CWorldTimedFunctions;
	CWorldTicker _Ticker;

	// Map cache
	friend class CServer;
	friend class CWorldMap;
	CWorldCache _Cache;

	// Sector data
	friend class CSectorList;
	CSectorList _Sectors;

// World data.
private:
	// Special purpose timers.
	bool	_fSaveNotificationSent;		// has notification been sent?
	int64	_iTimeLastWorldSave;		// when to auto do the worldsave ?
	int64	_iTimeLastDeadRespawn;		// when to res dead NPC's ?
	int64	_iTimeLastCallUserFunc;		// when to call next user func
	ullong	m_ticksWithoutMySQL;		// MySQL should be running constantly if MySQLTicks is true, keep here record of how much ticks since Sphere is not connected.

	int		_iSaveStage;	// Current stage of the background save.
	llong	_iSaveTimer;	// Time it takes to save

public:
	int64 _iTimeStartup;		// When did the system restore load/save ?
	int m_iSaveCountID;			// Current archival backup id. Whole World must have this same stage id
	int m_iLoadVersion;			// Previous load version. (only used during load of course)
	int m_iPrevBuild;			// Previous __GITREVISION__

	CUID m_uidLastNewItem;	// for script access.
	CUID m_uidLastNewChar;	// for script access.
	CUID m_uidObj;			// for script access - auxiliary obj
	CUID m_uidNew;			// for script access - auxiliary obj

	sl::unique_ptr_vector<CGMPage>			m_GMPages;	// Owns current outstanding GM pages. (CGMPage)
	sl::unique_ptr_vector<CItemStone>		m_Stones;		// Owns guild/town stones. (not saved array)
	sl::unique_ptr_vector<CPartyDef>		m_Parties;	// Owns all active parties.
	sl::raw_ptr_view_vector<CItemMulti>		m_Multis;		// World multis?
	sl::smart_ptr_view_vector<CResourceDef> m_TileTypes;	// Links to CItemTypeDef items owned by g_Cfg.m_ResHash

public:
	CWorld();
	virtual ~CWorld();

private:
	bool LoadFile( lpctstr pszName, bool fError = true );
	bool LoadWorld();

	// WorldSave methods
	static void GetBackupName(CSString& sArchive, lpctstr ptcBaseDir, tchar tcType, int iSaveCount);

	bool SaveTry(bool fForceImmediate); // Save world state
	bool SaveStage();
	bool SaveForce(); // Save world state

	// Sync again the WorldClock internal timer with the Real World Time after a lengthy operation (WorldSave, Resync...)
	friend class CServer;
	void SyncGameTime() noexcept;

private:
	CWorld(const CWorld& copy);
	CWorld& operator=(const CWorld& other);

public:
	static lpctstr const sm_szLoadKeys[];

	void Init();

	void r_Write(CScript& s);
	virtual bool r_WriteVal(lpctstr ptcKey, CSString& sVal, CTextConsole* pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false) override;
	virtual bool r_LoadVal(CScript& s) override;
	virtual bool r_GetRef(lpctstr& ptcKey, CScriptObj*& pRef) override;


	// World stuff

	void _OnTick();

	void GarbageCollection();
	void Restock();
	void RespawnDeadNPCs();

	static bool OpenScriptBackup(CScript& s, lpctstr pszBaseDir, lpctstr pszBaseName, int savecount);
    bool CheckAvailableSpaceForSave(bool fStatics);
	bool Save( bool fForceImmediate ); // Save world state
	void SaveStatics();
	bool LoadAll();
	bool DumpAreas( CTextConsole * pSrc, lpctstr pszFilename );
	void Close();

	bool Export(lpctstr pszFilename, const CChar* pSrc, word iModeFlags = IMPFLAGS_ITEMS, int iDist = INT16_MAX, int dx = 0, int dy = 0);
	bool Import(lpctstr pszFilename, const CChar* pSrc, word iModeFlags = IMPFLAGS_ITEMS, int iDist = INT16_MAX, tchar* pszAgs1 = nullptr, tchar* pszAgs2 = nullptr);

	virtual lpctstr GetName() const override {
	    return "World";
	    }

} g_World;


#endif // _INC_CWORLD_H
