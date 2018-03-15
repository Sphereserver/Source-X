/**
* @file CResourceBase.h
*
*/

#ifndef _INC_CRESOURCEBASE_H
#define _INC_CRESOURCEBASE_H

#include "sphere_library/CSArray.h"
#include "sphere_library/CSTime.h"
#include "../game/CServerTime.h"
#include "../game/game_enums.h"
#include "../game/game_macros.h"
#include "common.h"
#include "CUID.h"
#include "CScript.h"
#include "CScriptObj.h"

class CVarDefContNum;


enum RES_TYPE	// all the script resource blocks we know how to deal with !
{
	// NOTE: SPHERE.INI, SPHERETABLE.SCP are read at start.
	// All other files are indexed from the SCPFILES directories.
	// (SI) = Single instance types.
	// (SL) = single line multiple definitions.
	// Alphabetical order.
	RES_UNKNOWN = 0,		// Not to be used.
	RES_ACCOUNT,		// Define an account instance.
	RES_ADVANCE,		// Define the advance rates for stats.
	RES_AREA,			// Complex region. (w/extra tags)
	RES_BLOCKIP,		// (SL) A list of IP's to block.
	RES_BOOK,			// A book or a page from a book.
	RES_CHARDEF,		// Define a char type. (overlap with RES_SPAWN)
	RES_COMMENT,		// A commented out block type.
	RES_DEFNAME,		// (SL) Just add a bunch of new defs and equivs str/values.
	RES_DIALOG,			// A scriptable gump dialog, text or handler block.
	RES_EVENTS,			// An Event handler block with the trigger type in it. ON=@Death etc.
	RES_FAME,
	RES_FUNCTION,		// Define a new command verb script that applies to a char.
	RES_GMPAGE,			// A GM page. (SAVED in World)
	RES_ITEMDEF,		// Define an item type. (overlap with RES_TEMPLATE)
	RES_KARMA,
	RES_KRDIALOGLIST,	// Mapping of dialog<->kr ids
	RES_MENU,			// General scriptable menus.
	RES_MOONGATES,		// (SL) Define where the moongates are.
	RES_NAMES,			// A block of possible names for a NPC type. (read as needed)
	RES_NEWBIE,			// Triggers to execute on Player creation (based on skills selected)
	RES_NOTOTITLES,		// (SI) Define the noto titles used.
	RES_OBSCENE,		// (SL) A list of obscene words.
	RES_PLEVEL,			// Define the list of commands that a PLEVEL can access. (or not access)
	RES_REGIONRESOURCE,	// Define an Ore type.
	RES_REGIONTYPE,		// Triggers etc. that can be assinged to a RES_AREA
	RES_RESOURCELIST,	// List of Sections to create lists from in DEFLIST
	RES_RESOURCES,		// (SL) list of all the resource files we should index !
	RES_ROOM,			// Non-complex region. (no extra tags)
	RES_RUNES,			// (SI) Define list of the magic runes.
	RES_SCROLL,			// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD, SCROLL_NEWBIE
	RES_SECTOR,			// Make changes to a sector. (SAVED in World)
	RES_SERVERS,		// List a number of servers in 3 line format. (Phase this out)
	RES_SKILL,			// Define attributes for a skill (how fast it raises etc)
	RES_SKILLCLASS,		// Define specifics for a char with this skill class. (ex. skill caps)
	RES_SKILLMENU,		// A menu that is attached to a skill. special arguments over other menus.
	RES_SPAWN,			// Define a list of NPC's and how often they may spawn.
	RES_SPEECH,			// A speech block with ON=*blah* in it.
	RES_SPELL,			// Define a magic spell. (0-64 are reserved)
	RES_SPHERE,			// Main Server INI block
	RES_SPHERECRYPT,	// Encryption keys
	RES_STARTS,			// (SI) List of starting locations for newbies.
	RES_STAT,			// Stats elements like KARMA,STR,DEX,FOOD,FAME,CRIMINAL etc. Used for resource and desire scripts.
	RES_TELEPORTERS,	// (SL) Where are the teleporters in the world ? dungeon transports etc.
	RES_TEMPLATE,		// Define lists of items. (for filling loot etc)
	RES_TIMERF,
	RES_TIP,			// Tips (similar to RES_SCROLL) that can come up at startup.
	RES_TYPEDEF,		// Define a trigger block for a RES_WORLDITEM m_type.
	RES_TYPEDEFS,
	RES_WC,				// =WORLDCHAR
	RES_WEBPAGE,		// Define a web page template.
	RES_WI,				// =WORLDITEM
	RES_WORLDCHAR,		// Define instance of char in the world. (SAVED in World)
	RES_WORLDITEM,		// Define instance of item in the world. (SAVED in World)
	RES_WORLDLISTS,
	RES_WORLDSCRIPT,	// Something to load into a script.
	RES_WORLDVARS,
	RES_WS,				// =WORLDSCRIPT
	RES_QTY				// Don't care
};

#define RES_DIALOG_TEXT				1	// sub page for the section.
#define RES_DIALOG_BUTTON			2
#define RES_NEWBIE_MALE_DEFAULT		(10000+1)	// just an unused number for the range.
#define RES_NEWBIE_FEMALE_DEFAULT	(10000+2)
#define RES_NEWBIE_PROF_ADVANCED	(10000+3)
#define RES_NEWBIE_PROF_WARRIOR		(10000+4)
#define RES_NEWBIE_PROF_MAGE		(10000+5)
#define RES_NEWBIE_PROF_BLACKSMITH	(10000+6)
#define RES_NEWBIE_PROF_NECROMANCER	(10000+7)
#define RES_NEWBIE_PROF_PALADIN		(10000+8)
#define RES_NEWBIE_PROF_SAMURAI		(10000+9)
#define RES_NEWBIE_PROF_NINJA		(10000+10)

struct CResourceIDBase : public CUIDBase
{
// What is a Resource? Look at the comment made to the RES_TYPE enum.
// RES_TYPE: Resource Type (look at the RES_TYPE enum entries).
// RES_PAGE: Resource Page (used for dialog or book pages, but also to store an additional parameter
//		when using other Resource Types, like REGIONTYPE).
// RES_INDEX: Resource Index
#define RES_TYPE_SHIFT	25		// leave 6 bits = 64 for RES_TYPE;
#define RES_TYPE_MASK	63		//  63 = 0x3F = 6 bits.
#define RES_PAGE_SHIFT	17		// leave 8 bits = 255 pages of space;
#define RES_PAGE_MASK	255		//  255 = 0xFF = 8 bits.
#define RES_INDEX_SHIFT	0		// leave 18 bits = 262144 entries;
#define RES_INDEX_MASK	0x3FFFF	//  0x3FFFF = 18 bits.
// Size: 6 + 8 + 18 = 32 --> it's a 32 bits number.
#define RES_GET_TYPE(dw)	( ( dw >> RES_TYPE_SHIFT ) & RES_TYPE_MASK )
#define RES_GET_INDEX(dw)	( dw & RES_INDEX_MASK )

public:
	RES_TYPE GetResType() const
	{
		dword dwVal = RES_GET_TYPE(m_dwInternalVal);
		return static_cast<RES_TYPE>(dwVal);
	}
	int GetResIndex() const
	{
		return RES_GET_INDEX(m_dwInternalVal);
	}
	int GetResPage() const
	{
		dword dwVal = m_dwInternalVal >> RES_PAGE_SHIFT;
		dwVal &= RES_PAGE_MASK;
		return dwVal;
	}
	bool operator == (const CResourceIDBase & rid) const
	{
		return (rid.m_dwInternalVal == m_dwInternalVal);
	}
};

struct CResourceID : public CResourceIDBase
{
	CResourceID()
	{
		InitUID();
	}
	CResourceID(RES_TYPE restype)
	{
		// single instance type.
		m_dwInternalVal = UID_F_RESOURCE | (restype << RES_TYPE_SHIFT);
	}
	CResourceID(RES_TYPE restype, int index)
	{
		ASSERT(index < RES_INDEX_MASK);
		m_dwInternalVal = UID_F_RESOURCE | (restype << RES_TYPE_SHIFT) | index;
	}
	CResourceID(RES_TYPE restype, int index, int iPage)
	{
		ASSERT(index < RES_INDEX_MASK);
		ASSERT(iPage < RES_PAGE_MASK);
		m_dwInternalVal = UID_F_RESOURCE | (restype << RES_TYPE_SHIFT) | (iPage << RES_PAGE_SHIFT) | index;
	}
	CResourceIDBase & operator = (const CResourceIDBase & rid)
	{
		ASSERT(rid.IsValidUID());
		ASSERT(rid.IsResource());
		m_dwInternalVal = rid.GetPrivateUID();
		return *this;
	}
};


//*********************************************************

struct CResourceQty
{
private:
	CResourceID m_rid;		// A RES_SKILL, RES_ITEMDEF, or RES_TYPEDEF
	int64 m_iQty;			// How much of this ?
public:
	CResourceID GetResourceID() const
	{
		return m_rid;
	}
	void SetResourceID(CResourceID rid, int iQty)
	{
		m_rid = rid;
		m_iQty = iQty;
	}
	RES_TYPE GetResType() const
	{
		return m_rid.GetResType();
	}
	int GetResIndex() const
	{
		return m_rid.GetResIndex();
	}
	int64 GetResQty() const
	{
		return m_iQty;
	}
	void SetResQty(int64 wQty)
	{
		m_iQty = wQty;
	}
	inline bool Load( lptstr & arg )
	{
		return Load( const_cast<lpctstr&>(arg) );
	}
	bool Load( lpctstr & pszCmds );
	size_t WriteKey( tchar * pszArgs, bool fQtyOnly = false, bool fKeyOnly = false ) const;
	size_t WriteNameSingle( tchar * pszArgs, int iQty = 0 ) const;
public:
	CResourceQty() : m_iQty(0) { };
};

class CResourceQtyArray : public CSTypedArray<CResourceQty, CResourceQty&>
{
	// Define a list of index id's (not references) to resource objects. (Not owned by the list)
public:
	static const char *m_sClassName;
	CResourceQtyArray();
	explicit CResourceQtyArray(lpctstr pszCmds);
	bool operator == ( const CResourceQtyArray & array ) const;
	CResourceQtyArray& operator=(const CResourceQtyArray& other);

private:
	CResourceQtyArray(const CResourceQtyArray& copy);

public:
	size_t Load( lpctstr pszCmds );
	void WriteKeys( tchar * pszArgs, size_t index = 0, bool fQtyOnly = false, bool fKeyOnly = false ) const;
	void WriteNames( tchar * pszArgs, size_t index = 0 ) const;

	size_t FindResourceID( CResourceIDBase rid ) const;
	size_t FindResourceType( RES_TYPE type ) const;
	size_t FindResourceMatch( CObjBase * pObj ) const;
	bool IsResourceMatchAll( CChar * pChar ) const;

	inline bool ContainsResourceID( CResourceIDBase & rid ) const
	{
		return FindResourceID(rid) != BadIndex();
	}
	inline bool ContainsResourceMatch( CObjBase * pObj ) const
	{
		return FindResourceMatch(pObj) != BadIndex();
	}

	void setNoMergeOnLoad();

private:
	bool m_mergeOnLoad;
};

//*********************************************************

class CScriptFileContext
{
	// Track a temporary context into a script.
	// NOTE: This should ONLY be stack based !
private:
	bool m_fOpenScript;	// NULL context may be legit.
	const CScript * m_pPrvScriptContext;	// previous general context before this was opened.
private:
	void Init()
	{
		m_fOpenScript = false;
	}

public:
	static const char *m_sClassName;
	void OpenScript( const CScript * pScriptContext );
	void Close();
	CScriptFileContext() : m_pPrvScriptContext(NULL)
	{
		Init();
	}
	explicit CScriptFileContext(const CScript * pScriptContext)
	{
		Init();
		OpenScript(pScriptContext);
	}
	~CScriptFileContext()
	{
		Close();
	}

private:
	CScriptFileContext(const CScriptFileContext& copy);
	CScriptFileContext& operator=(const CScriptFileContext& other);
};

class CScriptObjectContext
{
	// Track a temporary context of an object.
	// NOTE: This should ONLY be stack based !
private:
	bool m_fOpenObject;	// NULL context may be legit.
	const CScriptObj * m_pPrvObjectContext;	// previous general context before this was opened.
private:
	void Init()
	{
		m_fOpenObject = false;
	}
public:
	static const char *m_sClassName;
	void OpenObject( const CScriptObj * pObjectContext );
	void Close();
	CScriptObjectContext() : m_pPrvObjectContext(NULL)
	{
		Init();
	}
	explicit CScriptObjectContext(const CScriptObj * pObjectContext)
	{
		Init();
		OpenObject(pObjectContext);
	}
	~CScriptObjectContext()
	{
		Close();
	}

private:
	CScriptObjectContext(const CScriptObjectContext& copy);
	CScriptObjectContext& operator=(const CScriptObjectContext& other);
};

//*********************************************************

class CResourceScript : public CScript
{
	// A script file containing resource, speech, motives or events handlers.
	// NOTE: we should check periodically if this file has been altered externally ?

private:
	int		m_iOpenCount;		// How many CResourceLock(s) have this open ?
	CServerTime m_timeLastAccess;	// CWorld time of last access/Open.

	// Last time it was closed. What did the file params look like ?
	dword m_dwSize;			// Compare to see if this has changed.
	CSTime m_dateChange;	// real world time/date of last change.

private:
	void Init()
	{
		m_iOpenCount = 0;
		m_timeLastAccess.Init();
		m_dwSize = UINT32_MAX;			// Compare to see if this has changed.
	}
	bool CheckForChange();

public:
	static const char *m_sClassName;
	explicit CResourceScript(lpctstr pszFileName)
	{
		Init();
		SetFilePath(pszFileName);
	}
	CResourceScript()
	{
		Init();
	}

private:
	CResourceScript(const CResourceScript& copy);
	CResourceScript& operator=(const CResourceScript& other);

public:
	bool IsFirstCheck() const
	{
		return (m_dwSize == UINT32_MAX && !m_dateChange.IsTimeValid());
	}
	void ReSync();
	bool Open( lpctstr pszFilename = NULL, uint wFlags = OF_READ );
	virtual void Close();
	virtual void CloseForce();
};

class CResourceLock : public CScript
{
	// Open a copy of a script that is already open
	// NOTE: This should ONLY be stack based !
	// preserve the previous openers offset in the script.
private:
	CResourceScript * m_pLock;
	CScriptLineContext m_PrvLockContext;		// i must return the locked file back here.

	CScriptFileContext m_PrvScriptContext;		// where was i before (context wise) opening this. (for error tracking)
	CScriptObjectContext m_PrvObjectContext;	// object context (for error tracking)
private:
	void Init()
	{
		m_pLock = NULL;
		m_PrvLockContext.Init();	// means the script was NOT open when we started.
	}

protected:
	virtual bool OpenBase( void * pExtra );
	virtual void CloseBase();
	virtual bool ReadTextLine( bool fRemoveBlanks );

public:
	static const char *m_sClassName;
	CResourceLock()
	{
		Init();
	}
	~CResourceLock()
	{
		Close();
	}

private:
	CResourceLock(const CResourceLock& copy);
	CResourceLock& operator=(const CResourceLock& other);

public:
	int OpenLock( CResourceScript * pLock, CScriptLineContext context );
	void AttachObj( const CScriptObj * pObj );
};

class CResourceDef : public CScriptObj
{
	// Define a generic resource block in the scripts.
	// Now the scripts can be modular. resources can be defined any place.
	// NOTE: This may be loaded fully into memory or just an Index to a file.
private:
	CResourceID m_rid;		// the true resource id. (must be unique for the RES_TYPE)
protected:
	const CVarDefContNum * m_pDefName;	// The name of the resource. (optional)
public:
	static const char *m_sClassName;
	CResourceDef(CResourceID rid, lpctstr pszDefName) : m_rid(rid), m_pDefName(NULL)
	{
		SetResourceName(pszDefName);
	}
	CResourceDef(CResourceID rid, const CVarDefContNum * pDefName = NULL) : m_rid(rid), m_pDefName(pDefName)
	{
	}
	virtual ~CResourceDef()
	{// need a virtual for the dynamic_cast to work.
		// ?? Attempt to remove m_pDefName ?
	}

private:
	CResourceDef(const CResourceDef& copy);
	CResourceDef& operator=(const CResourceDef& other);

public:
	CResourceID GetResourceID() const
	{
		return m_rid;
	}
	RES_TYPE GetResType() const
	{
		return m_rid.GetResType();
	}
	int GetResPage() const
	{
		return m_rid.GetResPage();
	}

	void CopyDef(const CResourceDef * pLink)
	{
		m_pDefName = pLink->m_pDefName;
	}

	// Get the name of the resource item. (Used for saving) may be number or name
	lpctstr GetResourceName() const;
	virtual lpctstr GetName() const	// default to same as the DEFNAME name.
	{
		return GetResourceName();
	}

	// Give it another DEFNAME= even if it already has one. it's ok to have multiple names.
	bool SetResourceName( lpctstr pszName );
	void SetResourceVar( const CVarDefContNum* pVarNum )
	{
		if (pVarNum != NULL && m_pDefName == NULL)
			m_pDefName = pVarNum;
	}

	// unlink all this data. (tho don't delete the def as the pointer might still be used !)
	virtual void UnLink()
	{
		// This does nothing in the CResourceDef case, Only in the CResourceLink case.
	}

	bool HasResourceName();
	bool MakeResourceName();
};

#define MAX_TRIGGERS_ARRAY	5

class CResourceLink : public CResourceDef
{
	// A single resource object that also has part of itself remain in resource file.
	// A pre-indexed link into a script file.
	// This is a CResourceDef not fully read into memory at index time.
	// We are able to lock it and read it as needed
private:
	CResourceScript * m_pScript;	// we already found the script.
	CScriptLineContext m_Context;

	dword m_lRefInstances;	// How many CResourceRef objects refer to this ?
public:
	static const char *m_sClassName;
	dword m_dwOnTriggers[MAX_TRIGGERS_ARRAY];

#define XTRIG_UNKNOWN 0	// bit 0 is reserved to say there are triggers here that do not conform.

public:
	void AddRefInstance()
	{
		m_lRefInstances++;
	}
	void DelRefInstance()
	{
#ifdef _DEBUG
		ASSERT(m_lRefInstances > 0);
#endif
		m_lRefInstances--;
	}
	dword GetRefInstances() const
	{
		return m_lRefInstances;
	}

	bool IsLinked() const;	// been loaded from the scripts ?
	CResourceScript * GetLinkFile() const;
	size_t GetLinkOffset() const;
	void SetLink( CResourceScript * pScript );
    void CopyTransfer( CResourceLink * pLink );
	void ScanSection( RES_TYPE restype );
	void ClearTriggers();
	void SetTrigger( int i );
	bool HasTrigger( int i ) const;
	bool ResourceLock( CResourceLock & s );

public:
	CResourceLink( CResourceID rid, const CVarDefContNum * pDef = NULL );
	virtual ~CResourceLink();

private:
	CResourceLink(const CResourceLink& copy);
	CResourceLink& operator=(const CResourceLink& other);
};

class CResourceNamed : public CResourceLink
{
	// Private name pool. (does not use DEFNAME) RES_FUNCTION
public:
	static const char *m_sClassName;
	const CSString m_sName;
public:
	CResourceNamed(CResourceID rid, lpctstr pszName) : CResourceLink(rid), m_sName(pszName)
	{
	}
	virtual ~CResourceNamed()
	{
	}

private:
	CResourceNamed(const CResourceNamed& copy);
	CResourceNamed& operator=(const CResourceNamed& other);

public:
	lpctstr GetName() const
	{
		return m_sName;
	}
};

//***********************************************************

class CResourceRef
{
private:
	CResourceLink* m_pLink;
public:
	static const char *m_sClassName;
	CResourceRef()
	{
		m_pLink = NULL;
	}
	CResourceRef(CResourceLink* pLink) : m_pLink(pLink)
	{
		ASSERT(pLink);
		pLink->AddRefInstance();
	}
	CResourceRef(const CResourceRef& copy)
	{
		m_pLink = copy.m_pLink;
		if (m_pLink != NULL)
			m_pLink->AddRefInstance();
	}
	~CResourceRef()
	{
		if (m_pLink != NULL)
			m_pLink->DelRefInstance();
	}
	CResourceRef& operator=(const CResourceRef& other)
	{
		if (this != &other)
			SetRef(other.m_pLink);
		return *this;
	}

public:
	CResourceLink* GetRef() const
	{
		return m_pLink;
	}
	void SetRef(CResourceLink* pLink)
	{
		if (m_pLink != NULL)
			m_pLink->DelRefInstance();

		m_pLink = pLink;

		if (pLink != NULL)
			pLink->AddRefInstance();
	}
	operator CResourceLink*() const
	{
		return GetRef();
	}
};

class CResourceRefArray : public CSPtrTypeArray<CResourceRef>
{
	// Define a list of pointer references to resource. (Not owned by the list)
	// An indexed list of CResourceLink s.
private:
	lpctstr GetResourceName( size_t iIndex ) const;
public:
	static const char *m_sClassName;
	CResourceRefArray()
	{
	}
private:
	CResourceRefArray(const CResourceRefArray& copy);
	CResourceRefArray& operator=(const CResourceRefArray& other);

public:
	size_t FindResourceType( RES_TYPE type ) const;
	size_t FindResourceID( CResourceIDBase rid ) const;
	size_t FindResourceName( RES_TYPE restype, lpctstr pszKey ) const;

	void WriteResourceRefList( CSString & sVal ) const;
	bool r_LoadVal( CScript & s, RES_TYPE restype );
	void r_Write( CScript & s, lpctstr pszKey ) const;

	inline bool ContainsResourceID( CResourceIDBase & rid ) const
	{
		return FindResourceID(rid) != BadIndex();
	}
	inline bool ContainsResourceName( RES_TYPE restype, lpctstr & pszKey ) const
	{
		return FindResourceName(restype, pszKey) != BadIndex();
	}
};

//*********************************************************

class CResourceHashArray : public CSObjSortArray< CResourceDef*, CResourceIDBase >
{
	// This list OWNS the CResourceDef and CResourceLink objects.
	// Sorted array of RESOURCE_ID
public:
	static const char *m_sClassName;
	CResourceHashArray() { }
private:
	CResourceHashArray(const CResourceHashArray& copy);
	CResourceHashArray& operator=(const CResourceHashArray& other);
public:
	int CompareKey( CResourceIDBase rid, CResourceDef * pBase, bool fNoSpaces ) const;
};

class CResourceHash
{
public:
	static const char *m_sClassName;
	CResourceHashArray m_Array[16];
public:
	CResourceHash()
	{
	}
private:
	CResourceHash(const CResourceHash& copy);
	CResourceHash& operator=(const CResourceHash& other);
private:
	int GetHashArray(CResourceIDBase rid) const
	{
		return (rid.GetResIndex() & 0x0F);
	}
public:
	inline size_t BadIndex() const
	{
		return m_Array[0].BadIndex();
	}
	size_t FindKey(CResourceIDBase rid) const
	{
		return m_Array[GetHashArray(rid)].FindKey(rid);
	}
	CResourceDef* GetAt(CResourceIDBase rid, size_t index) const
	{
		return m_Array[GetHashArray(rid)].GetAt(index);
	}
	size_t AddSortKey(CResourceIDBase rid, CResourceDef* pNew)
	{
		return m_Array[GetHashArray(rid)].AddSortKey(pNew, rid);
	}
	void SetAt(CResourceIDBase rid, size_t index, CResourceDef* pNew)
	{
		m_Array[GetHashArray(rid)].SetAt(index, pNew);
	}
};

//*************************************************

struct CSStringSortArray : public CSObjSortArray< tchar*, tchar* >
{
public:
	CSStringSortArray() { }
private:
	CSStringSortArray(const CSStringSortArray& copy);
	CSStringSortArray& operator=(const CSStringSortArray& other);
public:
	virtual void DestructElements( tchar** pElements, size_t nCount );
	// Sorted array of strings
	int CompareKey( tchar* pszID1, tchar* pszID2, bool fNoSpaces ) const;
	void AddSortString( lpctstr pszText );
};

class CObjNameSortArray : public CSObjSortArray< CScriptObj*, lpctstr >
{
public:
	static const char *m_sClassName;
	CObjNameSortArray();
private:
	CObjNameSortArray(const CObjNameSortArray& copy);
	CObjNameSortArray& operator=(const CObjNameSortArray& other);

public:
	// Array of CScriptObj. name sorted.
	int CompareKey( lpctstr pszID, CScriptObj* pObj, bool fNoSpaces ) const;
};

//***************************************************************8

class CResourceBase : public CScriptObj
{
protected:
	static lpctstr const sm_szResourceBlocks[RES_QTY];

	CSObjArray< CResourceScript* > m_ResourceFiles;	// All resource files we need to get blocks from later.

public:
	static const char *m_sClassName;
	CResourceHash m_ResHash;		// All script linked resources RES_QTY

	// INI File options.
	CSString m_sSCPBaseDir;		// if we want to get *.SCP files from elsewhere.

protected:
	CResourceScript * AddResourceFile( lpctstr pszName );
	void AddResourceDir( lpctstr pszDirName );

public:
	void LoadResourcesOpen( CScript * pScript );
	bool LoadResources( CResourceScript * pScript );
	static lpctstr GetResourceBlockName( RES_TYPE restype );
	lpctstr GetName() const;
	CResourceScript * GetResourceFile( size_t i );
	CResourceID ResourceGetID( RES_TYPE restype, lpctstr & pszName );
	CResourceID ResourceGetIDType( RES_TYPE restype, lpctstr pszName );
	int ResourceGetIndexType( RES_TYPE restype, lpctstr pszName );
	lpctstr ResourceGetName( CResourceIDBase rid ) const;
	CScriptObj * ResourceGetDefByName( RES_TYPE restype, lpctstr pszName )
	{
		// resolve a name to the actual resource def.
		return ResourceGetDef(ResourceGetID(restype, pszName));
	}
	bool ResourceLock( CResourceLock & s, CResourceIDBase rid );
	bool ResourceLock( CResourceLock & s, RES_TYPE restype, lpctstr pszName )
	{
		return ResourceLock(s, ResourceGetIDType(restype, pszName));
	}

	CResourceScript * FindResourceFile( lpctstr pszTitle );
	CResourceScript * LoadResourcesAdd( lpctstr pszNewName );

	virtual CResourceDef * ResourceGetDef( CResourceIDBase rid ) const;
	virtual bool OpenResourceFind( CScript &s, lpctstr pszFilename, bool bCritical = true );
	virtual bool LoadResourceSection( CScript * pScript ) = 0;

public:
	CResourceBase()
	{
	}
	virtual ~CResourceBase()
	{
	}

private:
	CResourceBase(const CResourceBase& copy);
	CResourceBase& operator=(const CResourceBase& other);
};

inline lpctstr CResourceBase::GetResourceBlockName( RES_TYPE restype )	// static
{
	if ( restype < 0 || restype >= RES_QTY )
		restype = RES_UNKNOWN;
	return( sm_szResourceBlocks[restype] );
}

#endif // _INC_CRESOURCEBASE_H
