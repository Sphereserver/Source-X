/**
* @file CResourceID.h
*
*/

#ifndef _INC_CRESOURCEID_H
#define _INC_CRESOURCEID_H

#include "CUID.h"

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
    // WARNING: when adding new resource types, make sure that the last bit (31) doesn't overlap with UID_F_RESOURCE!
#define RES_GET_TYPE(dw)	( ( dw >> RES_TYPE_SHIFT ) & RES_TYPE_MASK )
#define RES_GET_INDEX(dw)	( dw & (dword)RES_INDEX_MASK )

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


#endif // _INC_CRESOURCEDEF_H
