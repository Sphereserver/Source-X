
//#include "../CException.h" // included in the precompiled header
#include "../CExpression.h" // included in the precompiled header
#include "../CLog.h"
#include "CResourceHolder.h"
#include "CResourceHash.h"
//#include "CResourceScript.h"


//***************************************************
// CResourceHolder

lpctstr const CResourceHolder::sm_szResourceBlocks[RES_QTY] =	// static
{
	"AAAUNUSED",	// unused / unknown.
    "ACCOUNT",		// Define an account instance.
	"ADVANCE",		// Define the advance rates for stats.
	"AREA",			// Complex region. (w/ extra tags)
	"BLOCKIP",		// (SL) A list of IP's to block.
	"BOOK",			// A book or a page from a book.
	"CHAMPION",		// A Champion definition.
	"CHARDEF",		// Define a char type.
	"COMMENT",		// A commented out block type.
	"DEFNAME",		// (SL) Just add a bunch of new defs and equivs str/values.
	"DIALOG",		// A scriptable gump dialog", text or handler block.
	"EVENTS",		// (SL) Preload these Event files.
	"FAME",
	"FUNCTION",		// Define a new command verb script that applies to a char.
	"GMPAGE",		// A GM page. (SAVED in World)
	"ITEMDEF",		// Define an item type
	"KARMA",
	"KRDIALOGLIST",	// mapping of dialog<->kr ids
	"MENU",			// General scriptable menus.
	"MOONGATES",	// (SL) Define where the moongates are.
	"NAMES",		// A block of possible names for a NPC type. (read as needed)
	"NEWBIE",		// Triggers to execute on Player creation (based on skills selected)
	"NOTOTITLES",	// (SI) Define the noto titles used.
	"OBSCENE",		// (SL) A list of obscene words.
	"PLEVEL",		// Define the list of commands that a PLEVEL can access. (or not access)
	"REGIONRESOURCE",// Define natural resources (like ore types) to be found in a region.
	"REGIONTYPE",	// Triggers etc. that can be assinged to a "AREA
	"RESDEFNAME",	// (SL) Working like RES_DEFNAME, define aliases (e.g. a second DEFNAME) for existing resources.
	"RESOURCELIST",
	"RESOURCES",	// (SL) list of all the resource files we should index !
	"ROOM",			// Non-complex region. (no extra tags)
	"RUNES",		// (SI) Define list of the magic runes.
	"SCROLL",		// SCROLL_GUEST=message scroll sent to player at guest login. SCROLL_MOTD", SCROLL_NEWBIE
	"SECTOR",		// Make changes to a sector. (SAVED in World)
	"SERVERS",		// List a number of servers in 3 line format.
	"SKILL",		// Define attributes for a skill (how fast it raises etc)
	"SKILLCLASS",	// Define class specifics for a char with this skill class.
	"SKILLMENU",	// A menu that is attached to a skill. special arguments over other menus.
	"SPAWN",		// Define a list of NPC's and how often they may spawn.
	"SPEECH",		// (SL) Preload these speech files.
	"SPELL",		// Define a magic spell. (0-64 are reserved)
	"SPHERE",		// Main Server INI block
	"SPHERECRYPT",	// Encryption keys
	"STARTS",		// (SI) List of starting locations for newbies.
	"STAT",			// Stats elements like KARMA,STR,DEX,FOOD,FAME,CRIMINAL etc. Used for resource and desire scripts.
	"TELEPORTERS",	// (SL) Where are the teleporteres in the world ?
	"TEMPLATE",		// Define a list of items. (for filling loot etc)
	"TIMERF",
	"TIP",			// Tips that can come up at startup.
	"TYPEDEF",		// Define a trigger block for a "WORLDITEM m_type.
	"TYPEDEFS",
	"WC",			// =WORLDCHAR
	"WEBPAGE",		// Define a web page template.
	"WI",			// =WORLDITEM
	"WORLDCHAR",	// Define instance of char in the world. (SAVED in World)
	"WORLDITEM",	// Define instance of item in the world. (SAVED in World)
	"WORLDLISTS",	// Define instance of list in the world. (SAVED in World)
	"WORLDSCRIPT",	// Define instance of resource in the world. (SAVED in World)
	"WORLDVARS",	// block of global variables
	"WS"			// =WORLDSCRIPT
};




//*********************************************************
// Resource Section Definitions

lpctstr CResourceHolder::GetName() const
{
    return "CFG";
}

CResourceHolder::CResourceHolder()
{
    // Avoid unnecessary continuous growing re-allocations at startup, just start with some preallocated space.
    for (auto& arr : m_ResHash.m_Array)
    {
        arr.reserve(0x100);
    }
}



CResourceID CResourceHolder::ResourceGetID_EatStr(RES_TYPE restype, lpctstr &ptcName, word wPage, bool fCanFail)
{
    ADDTOCALLSTACK("CResourceHolder::ResourceGetID_EatStr");
    // Find the Resource ID given this name.
    // We are NOT creating a new resource. just looking up an existing one
    // NOTE: Do not enforce the restype.
    //		Just fill it in if we are not sure what the type is.
    // NOTE:
    //  Some restype's have private name spaces. (ie. RES_AREA)
    // RETURN:
    //  pszName is now set to be after the expression.


    // Try to handle private name spaces.
    /*
    switch ( restype )
    {
    case RES_ACCOUNT:
    case RES_AREA:
    case RES_GMPAGE:
    case RES_ROOM:
    case RES_SECTOR:
    break;

    default:
    break;
    }
    */

    lpctstr ptcNameStart = ptcName;
    dword dwEvalPrivateUID = Exp_GetDWVal(ptcName);    // May be some complex expression {}
    int iEvalResType  = ResGetType(dwEvalPrivateUID);
    int iEvalResIndex = ResGetIndex(dwEvalPrivateUID);

    // We are NOT creating.
    if ((restype != RES_UNKNOWN) && (iEvalResType == RES_UNKNOWN))
    {
        // Label it with the type we want.
        ASSERT(restype > RES_UNKNOWN && restype <= RES_QTY);
        if (restype == RES_QTY)
        {
            // RES_QTY means i don't care, because i pass a defname and it already carries data about which kind of resource it is.
            // If it doesn't (?!), or simply i pass an ID instead of a defname (which i expected), i'll have unexpected results, so better throw an error.
            // If i pass a bare ID, Sphere cannot know if you meant a char, item, etc...
            if (!fCanFail)
                g_Log.EventError("Can't get the resource type from a bare ID ('%s')!\n", ptcNameStart);
            return CResourceID(RES_UNKNOWN /* 0 */, 0, 0u);
        }
        return CResourceID(restype, iEvalResIndex, wPage);
    }
    // CResourceID always needs to be a valid resource (there's an ASSERT in CResourceID copy constructor).
    return CResourceID((RES_TYPE)iEvalResType, iEvalResIndex, wPage);
}

CResourceID CResourceHolder::ResourceGetID( RES_TYPE restype, lpctstr ptcName, word wPage, bool fCanFail )
{
	ADDTOCALLSTACK("CResourceHolder::ResourceGetID");
	return ResourceGetID_EatStr(restype, ptcName, wPage, fCanFail);
}

CResourceID CResourceHolder::ResourceGetIDType( RES_TYPE restype, lpctstr pszName, word wPage )
{
	// Get a resource of just this index type.
    ASSERT(restype != RES_QTY);
	CResourceID rid = ResourceGetID( restype, pszName, wPage, true );
	if ( rid.GetResType() != restype )
	{
		rid.Init();
		return rid;
	}
	return rid;
}

int CResourceHolder::ResourceGetIndexType( RES_TYPE restype, lpctstr pszName, word wPage )
{
	ADDTOCALLSTACK("CResourceHolder::ResourceGetIndexType");
	// Get a resource of just this index type.
	const CResourceID rid = ResourceGetID( restype, pszName, wPage );
	if ( rid.GetResType() != restype )
		return -1;
	return rid.GetResIndex();
}

sl::smart_ptr_view<CResourceDef> CResourceHolder::ResourceGetDefRef(const CResourceID& rid) const
{
	ADDTOCALLSTACK("CResourceHolder::ResourceGetDefRef");
	if ( ! rid.IsValidResource() )
		return {};
	size_t index = m_ResHash.FindKey( rid );
	if ( index == sl::scont_bad_index() )
		return {};
	return m_ResHash.GetSmartPtrViewAt( rid, index );
}

sl::smart_ptr_view<CResourceDef> CResourceHolder::ResourceGetDefRefByName( RES_TYPE restype, lpctstr pszName, word wPage )
{
    ADDTOCALLSTACK("CResourceHolder::ResourceGetDefRefByName");
    // resolve a name to the actual resource def.
    CResourceID res = ResourceGetID(restype, pszName, wPage);
    res.m_wPage = wPage ? wPage : RES_PAGE_ANY;   // Create a CResourceID with page == RES_PAGE_ANY: search independently from the page
    return ResourceGetDefRef(res);
}

CResourceDef* CResourceHolder::ResourceGetDef(const CResourceID& rid) const
{
	ADDTOCALLSTACK("CResourceHolder::ResourceGetDef");
	return ResourceGetDefRef(rid).get();
}

CResourceDef* CResourceHolder::ResourceGetDefByName(RES_TYPE restype, lpctstr pszName, word wPage)
{
	ADDTOCALLSTACK("CResourceHolder::ResourceGetDefByName");
	return ResourceGetDefRefByName(restype, pszName, wPage).get();
}

//*******************************************************
// Open resource section.

bool CResourceHolder::ResourceLock( CResourceLock & s, const CResourceID& rid )
{
	ADDTOCALLSTACK("CResourceHolder::ResourceLock");
	// Lock a referenced resource object.
	if ( ! rid.IsValidUID() )
		return false;

    CResourceLink* pResourceLink = dynamic_cast <CResourceLink*>(ResourceGetDefRef(rid).get());
    if (pResourceLink)
        return pResourceLink->ResourceLock(s);

	return false;
}
