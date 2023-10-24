
#include "../../game/chars/CChar.h"
#include "../../game/items/CItem.h"
#include "../../game/triggers.h"
#include "../../game/game_enums.h"
#include "../../game/game_macros.h"
#include "../sphere_library/CSFileList.h"
#include "../CException.h"
#include "../CLog.h"
#include "CResourceBase.h"
#include "CResourceHash.h"
#include "CResourceScript.h"
#include "CResourceSortedArrays.h"


//***************************************************
// CResourceBase

lpctstr const CResourceBase::sm_szResourceBlocks[RES_QTY] =	// static
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
// Resource Files

CResourceScript * CResourceBase::FindResourceFile( lpctstr pszPath )
{
	ADDTOCALLSTACK("CResourceBase::FindResourceFile");
	// Just match the titles ( not the whole path)

	lpctstr pszTitle = CScript::GetFilesTitle( pszPath );

	for ( size_t i = 0; ; ++i )
	{
		CResourceScript * pResFile = GetResourceFile(i);
		if ( pResFile == nullptr )
			break;
		lpctstr pszTitle2 = pResFile->GetFileTitle();
		if ( ! strcmpi( pszTitle2, pszTitle ))
			return pResFile;
	}
	return nullptr;
}

CResourceScript * CResourceBase::AddResourceFile( lpctstr pszName )
{
	ADDTOCALLSTACK("CResourceBase::AddResourceFile");
	ASSERT(pszName != nullptr);
	// Is this really just a dir name ?

	if (strlen(pszName) >= _MAX_PATH)
		throw CSError(LOGL_ERROR, 0, "Filename too long!");

	tchar szName[_MAX_PATH];
	strcpy(szName, pszName);

	tchar szTitle[_MAX_PATH];
	lpctstr ptcTitle = CScript::GetFilesTitle(szName);
	PERSISTANT_ASSERT(strlen(ptcTitle) < sizeof(szTitle));
	Str_CopyLimitNull(szTitle, ptcTitle, sizeof(szTitle));

	if ( szTitle[0] == '\0' )
	{
		AddResourceDir( pszName );
		return nullptr;
	}

	lpctstr pszExt = CScript::GetFilesExt( szTitle );
	if ( pszExt == nullptr )
	{
		// No file extension provided, so append .scp to the filename
		Str_ConcatLimitNull( szName,  SPHERE_SCRIPT, sizeof(szName) );
		Str_ConcatLimitNull( szTitle, SPHERE_SCRIPT, sizeof(szTitle) );
	}

	if ( ! strnicmp( szTitle, SPHERE_FILE "tables", strlen(SPHERE_FILE "tables")))
	{
		// Don't dupe this.
		return nullptr;
	}

	// Try to prevent dupes
	CResourceScript * pNewRes = FindResourceFile(szTitle);
	if ( pNewRes )
		return pNewRes;

	// Find correct path
    pNewRes = new CResourceScript();
    if (! OpenResourceFind(static_cast<CScript&>(*pNewRes), szName))
    {
        delete pNewRes;
        return nullptr;
    }

    m_ResourceFiles.emplace_back(pNewRes);
    pNewRes->m_iResourceFileIndex = int(m_ResourceFiles.size() -1);
    return pNewRes;
}

void CResourceBase::AddResourceDir( lpctstr pszDirName )
{
	ADDTOCALLSTACK("CResourceBase::AddResourceDir");
	if ( pszDirName[0] == '\0' )
		return;

	CSString sFilePath = CSFile::GetMergedFileName( pszDirName, "*" SPHERE_SCRIPT );

	CSFileList filelist;
	int iRet = filelist.ReadDir( sFilePath, false );
	if ( iRet < 0 )
	{
		// also check script file path
		sFilePath = CSFile::GetMergedFileName(m_sSCPBaseDir, sFilePath.GetBuffer());

		iRet = filelist.ReadDir( sFilePath, true );
		if ( iRet < 0 )
		{
			DEBUG_ERR(( "DirList=%d for '%s'\n", iRet, pszDirName ));
			return;
		}
	}

	if ( iRet <= 0 )	// no files here.
		return;

	CSStringListRec * psFile = filelist.GetHead(), *psFileNext = nullptr;
	for ( ; psFile; psFile = psFileNext )
	{
        psFileNext = psFile->GetNext();
		sFilePath = CSFile::GetMergedFileName( pszDirName, *psFile );
		AddResourceFile( sFilePath );
	}
}

void CResourceBase::LoadResourcesOpen( CScript * pScript )
{
	ADDTOCALLSTACK("CResourceBase::LoadResourcesOpen");
	// Load an already open resource file.

	ASSERT(pScript);
    ASSERT( pScript->HasCache() );

	int iSections = 0;
	while ( pScript->FindNextSection() )
	{
		LoadResourceSection( pScript );
		++iSections;
	}

	if ( ! iSections )
		DEBUG_WARN(( "No resource sections in '%s'\n", pScript->GetFilePath()));
}

bool CResourceBase::LoadResources( CResourceScript * pScript )
{
	ADDTOCALLSTACK("CResourceBase::LoadResources");
	// Open the file then load it.
	if ( pScript == nullptr )
		return false;

	if ( ! pScript->Open())
	{
		g_Log.Event(LOGL_CRIT|LOGM_INIT, "[RESOURCES] '%s' not found...\n", pScript->GetFilePath());
		return false;
	}

	g_Log.Event(LOGM_INIT, "Loading %s\n", pScript->GetFilePath());

	LoadResourcesOpen( pScript );
	pScript->Close();
	pScript->CloseForce();
	return true;
}

CResourceScript * CResourceBase::LoadResourcesAdd( lpctstr pszNewFileName )
{
	ADDTOCALLSTACK("CResourceBase::LoadResourcesAdd");
	// Make sure this is added to my list of resource files
	// And load it now.

	CResourceScript * pScript = AddResourceFile( pszNewFileName );
	if ( ! LoadResources(pScript) )
		return nullptr;
	return pScript;
}

bool CResourceBase::OpenResourceFind( CScript &s, lpctstr pszFilename, bool fCritical )
{
	ADDTOCALLSTACK("CResourceBase::OpenResourceFind");
	// Open a single resource script file.
	// Look in the specified path.

	if ( pszFilename == nullptr )
		pszFilename = s.GetFilePath();

	// search the local dir or full path first.
	if ( s.Open(pszFilename, OF_READ | OF_NONCRIT ))
		return true;
	if ( !fCritical )
		return false;

	// next, check the script file path
	CSString sPathName = CSFile::GetMergedFileName( m_sSCPBaseDir, pszFilename );
	if ( s.Open(sPathName, OF_READ | OF_NONCRIT ) )
		return true;

	// finally, strip the directory and re-check script file path
	lpctstr pszTitle = CSFile::GetFilesTitle(pszFilename);
	sPathName = CSFile::GetMergedFileName( m_sSCPBaseDir, pszTitle );
	return s.Open( sPathName, OF_READ );
}

bool CResourceBase::LoadResourceSection( CScript * pScript )
{
	ADDTOCALLSTACK("CResourceBase::LoadResourceSection");
	UnreferencedParameter(pScript);
	// Just stub this out for others for now.
	return false;
}

//*********************************************************
// Resource Section Definitions

lpctstr CResourceBase::ResourceGetName( const CResourceID& rid ) const
{
	ADDTOCALLSTACK("CResourceBase::ResourceGetName");
	// Get a portable name for the resource id type.

	const CResourceDef * pResourceDef = ResourceGetDef( rid );
	if ( pResourceDef )
		return pResourceDef->GetResourceName();

	tchar * pszTmp = Str_GetTemp();
	ASSERT(pszTmp);
	if ( !rid.IsValidUID() )
		sprintf( pszTmp, "%d", (int)rid.GetPrivateUID() );
	else
		sprintf( pszTmp, "0%" PRIx32, rid.GetResIndex() );
	return pszTmp;
}

lpctstr CResourceBase::GetName() const
{
	return "CFG";
}

CResourceScript * CResourceBase::GetResourceFile( size_t i )
{
	if ( ! m_ResourceFiles.IsValidIndex(i) )
		return nullptr;	// All resource files we need to get blocks from later.
	return m_ResourceFiles[i];
}

CResourceID CResourceBase::ResourceGetID_Advance(RES_TYPE restype, lpctstr &ptcName, word wPage)
{
    ADDTOCALLSTACK("CResourceBase::ResourceGetID_Advance");
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

    dword dwEvalPrivateUID = Exp_GetDWVal(ptcName);    // May be some complex expression {}
    int iEvalResType  = RES_GET_TYPE(dwEvalPrivateUID);
    int iEvalResIndex = RES_GET_INDEX(dwEvalPrivateUID);

    // We are NOT creating.
    if ((restype != RES_UNKNOWN) && (iEvalResType == RES_UNKNOWN))
    {
        // Label it with the type we want.
        return CResourceID(restype, iEvalResIndex, wPage);
    }
    // CResourceID always needs to be a valid resource (there's an ASSERT in CResourceID copy constructor).
    return CResourceID((RES_TYPE)iEvalResType, iEvalResIndex, wPage);
}

CResourceID CResourceBase::ResourceGetID( RES_TYPE restype, lpctstr ptcName, word wPage )
{
	ADDTOCALLSTACK("CResourceBase::ResourceGetID");
	return ResourceGetID_Advance(restype, ptcName, wPage);
}

CResourceID CResourceBase::ResourceGetIDType( RES_TYPE restype, lpctstr pszName, word wPage )
{
	// Get a resource of just this index type.
	CResourceID rid = ResourceGetID( restype, pszName, wPage );
	if ( rid.GetResType() != restype )
	{
		rid.Init();
		return rid;
	}
	return rid;
}

int CResourceBase::ResourceGetIndexType( RES_TYPE restype, lpctstr pszName, word wPage )
{
	ADDTOCALLSTACK("CResourceBase::ResourceGetIndexType");
	// Get a resource of just this index type.
	const CResourceID rid = ResourceGetID( restype, pszName, wPage );
	if ( rid.GetResType() != restype )
		return -1;
	return rid.GetResIndex();
}

sl::smart_ptr_view<CResourceDef> CResourceBase::ResourceGetDefRef(const CResourceID& rid) const
{
	ADDTOCALLSTACK("CResourceBase::ResourceGetDefRef");
	if ( ! rid.IsValidResource() )
		return {};
	size_t index = m_ResHash.FindKey( rid );
	if ( index == sl::scont_bad_index() )
		return {};
	return m_ResHash.GetSmartPtrViewAt( rid, index );
}

sl::smart_ptr_view<CResourceDef> CResourceBase::ResourceGetDefRefByName( RES_TYPE restype, lpctstr pszName, word wPage )
{
    ADDTOCALLSTACK("CResourceBase::ResourceGetDefRefByName");
    // resolve a name to the actual resource def.
    CResourceID res = ResourceGetID(restype, pszName, wPage);
    res.m_wPage = wPage ? wPage : RES_PAGE_ANY;   // Create a CResourceID with page == RES_PAGE_ANY: search independently from the page
    return ResourceGetDefRef(res);
}

CResourceDef* CResourceBase::ResourceGetDef(const CResourceID& rid) const
{
	ADDTOCALLSTACK("CResourceBase::ResourceGetDef");
	return ResourceGetDefRef(rid).get();
}

CResourceDef* CResourceBase::ResourceGetDefByName(RES_TYPE restype, lpctstr pszName, word wPage)
{
	ADDTOCALLSTACK("CResourceBase::ResourceGetDefByName");
	return ResourceGetDefRefByName(restype, pszName, wPage).get();
}

//*******************************************************
// Open resource section.

bool CResourceBase::ResourceLock( CResourceLock & s, const CResourceID& rid )
{
	ADDTOCALLSTACK("CResourceBase::ResourceLock");
	// Lock a referenced resource object.
	if ( ! rid.IsValidUID() )
		return false;

    CResourceLink* pResourceLink = dynamic_cast <CResourceLink*>(ResourceGetDefRef(rid).get());
    if (pResourceLink)
        return pResourceLink->ResourceLock(s);
	
	return false;
}
