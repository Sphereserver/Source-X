/**
* @file CResourceID.h
*
*/

#ifndef _INC_CRESOURCEID_H
#define _INC_CRESOURCEID_H

#include "../CUID.h"

enum RES_TYPE	// all the script resource sections we know how to deal with !
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
    RES_CHAMPION,		// A Champion definition.
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
    RES_RESDEFNAME,     // (SL) Working like RES_DEFNAME, define aliases (e.g. a second DEFNAME) for existing resources.
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


struct CResourceIDBase : public CUID    // It has not the "page" part/variable. Use it to store defnames or UIDs of world objects (items, chars...) or spawns and templates.
{
    // What is a Resource? Look at the comment made to the RES_TYPE enum.
    // RES_TYPE: Resource Type (look at the RES_TYPE enum entries).
    // RES_INDEX: Resource Index

    // m_dwInternalVal:
    // - Uppper 4 bits: RESERVED for flags UID_F_RESOURCE, UID_F_ITEM, UID_O_EQUIPPED, UID_O_CONTAINED.
    // - Usable size: 8 bits (res_type) + 20 bits (index) = 28 --> it's a 28 bits number.
#define RES_TYPE_SHIFT	20		// skip first 20 bits, use next 8 bits = 0xFFFF = 65535 possible unique RES_TYPEs;
#define RES_TYPE_MASK	0xFF	//  0xFF = 8 bits.
#define RES_INDEX_SHIFT	0		// use first 20 bits = 0xFFFFF = 1048575 possible unique indexes.
#define RES_INDEX_MASK	0xFFFFF	//  0xFFFFF = 20 bits.
    
#define RES_GET_TYPE(dw)	( ( (dw) >> RES_TYPE_SHIFT ) & RES_TYPE_MASK )
#define RES_GET_INDEX(dw)	( (dw) & (dword)RES_INDEX_MASK )

    void InitUID() = delete;
    void ClearUID() = delete;
    inline void Init() noexcept
    {
        m_dwInternalVal = UID_UNUSED;
    }
    inline void Clear() noexcept
    {
        m_dwInternalVal = UID_CLEAR;
    }

    CResourceIDBase() noexcept
    {
        Init();
    }
    explicit CResourceIDBase(RES_TYPE restype);
    explicit CResourceIDBase(RES_TYPE, const CResourceIDBase&) = delete;
    explicit CResourceIDBase(RES_TYPE restype, int iIndex);
    explicit CResourceIDBase(dword dwPrivateID);

    CResourceIDBase(const CResourceIDBase& rid);                // copy constructor
    CResourceIDBase& operator = (const CResourceIDBase& rid);   // assignment operator

    // operator CUID() const noexcept = delete;    // Such a call will never happen, this "conversion" is done via the constructor

    //---

    void FixRes();

    RES_TYPE GetResType() const noexcept
    {
        return (RES_TYPE)(RES_GET_TYPE(m_dwInternalVal));
    }
    int GetResIndex() const noexcept
    {
        return RES_GET_INDEX(m_dwInternalVal);
    }
    bool operator == (const CResourceIDBase & rid) const noexcept
    {
        return (rid.m_dwInternalVal == m_dwInternalVal);
    }

    void SetPrivateUID(dword dwVal) = delete;   // Don't do this, you'll end forgetting UID_F_RESOURCE or god knows what else...

    bool IsItem() const = delete;   // Try to warn and block the calls to this CUID method, in most cases it's incorrect and it will lead to bugs
    bool IsChar() const = delete;   // Try to warn and block the calls to this CUID method, in most cases it's incorrect and it will lead to bugs
    CObjBase*   ObjFind()  const = delete;   // Same as above
    CItem*      ItemFind() const = delete;
    CChar*      CharFind() const = delete;

    bool IsUIDItem() const; //  replacement for CUID::IsItem(), but don't be virtual, since we don't need that and the class size will increase due to the vtable
    CItem* ItemFindFromResource(bool fInvalidateBeingDeleted = false) const;   //  replacement for CUID::ItemFind()
};

struct CResourceID : public CResourceIDBase     // It has the "page" part. Use it to handle every other resource section.
{
    // RES_PAGE: Resource Page (used for dialog or book pages, but also to store an additional parameter
    //		when using other Resource Types, like REGIONTYPE).
    word m_wPage;
    #define RES_PAGE_MAX    UINT16_MAX - 1
    #define RES_PAGE_ANY    UINT16_MAX      // Pick a CResourceID independently from the page

    void Init()
    {
        m_dwInternalVal = UID_UNUSED;
        m_wPage = 0;
    }
    void Clear()
    {
        m_dwInternalVal = UID_CLEAR;
        m_wPage = 0;
    }

    CResourceID() : CResourceIDBase()
    {
        m_wPage = 0;
    }
    explicit CResourceID(RES_TYPE restype) : CResourceIDBase(restype)
    {
        m_wPage = 0;
    }
    explicit CResourceID(RES_TYPE, const CResourceIDBase&) = delete;
    explicit CResourceID(RES_TYPE restype, int iIndex) : CResourceIDBase(restype, iIndex)
    {
        m_wPage = 0;
    }
    explicit CResourceID(RES_TYPE, const CResourceIDBase&, word) = delete;
    explicit CResourceID(RES_TYPE restype, int iIndex, word wPage) : CResourceIDBase(restype, iIndex)
    {
        m_wPage = wPage;
    }
    explicit CResourceID(const CResourceIDBase&, word) = delete;
    explicit CResourceID(dword dwPrivateID, word wPage) : CResourceIDBase(dwPrivateID)
    {
        m_wPage = wPage;
    }

    CResourceID(const CResourceID & rid) : CResourceIDBase(rid)     // copy constructor
    {
        m_wPage = rid.m_wPage;
    }
    CResourceID(const CResourceIDBase & rid): CResourceIDBase(rid)
    {
        m_wPage = 0;
    }

    CResourceID& operator = (const CResourceID& rid);              // assignment operator
    CResourceID& operator = (const CResourceIDBase& rid);

    word GetResPage() const noexcept
    {
        return m_wPage;
    }
    bool operator == (const CResourceID & rid) const noexcept
    {
        return ((rid.m_wPage == m_wPage) && (rid.m_dwInternalVal == m_dwInternalVal));
    }
};


#endif // _INC_CRESOURCEID_H
