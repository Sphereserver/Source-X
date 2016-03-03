#ifndef _INC_GRAYSVR_H_
#define _INC_GRAYSVR_H_
#pragma once

//	Enable advanced exceptions catching. Consumes some more resources, but is very useful
//	for debug on a running environment. Also it makes sphere more stable since exceptions
//	are local
#ifndef _DEBUG
	#ifndef EXCEPTIONS_DEBUG
	#define EXCEPTIONS_DEBUG
	#endif
#endif

#if defined(_WIN32) && !defined(_MTNETWORK)
	// _MTNETWORK enabled via makefile for other systems
	#define _MTNETWORK
#endif

//#define DEBUGWALKSTUFF 1
//#ifdef _DEBUG
#ifdef DEBUGWALKSTUFF
	#define WARNWALK(_x_)		g_pLog->EventWarn _x_;
#else
	#define WARNWALK(_x_)		if ( g_Cfg.m_wDebugFlags & DEBUGF_WALK ) { g_pLog->EventWarn _x_; }
#endif


enum RESDISPLAY_VERSION
{
	RDS_T2A = 1,
	RDS_LBR,
	RDS_AOS,
	RDS_SE,
	RDS_ML,
	RDS_KR,
	RDS_SA,
	RDS_HS,
	RDS_TOL,
	RDS_QTY
};
#include "CWorld.h"

///////////////////////////////////////////////

//	Triggers list
enum E_TRIGGERS
{
	#define ADD(a) TRIGGER_##a,
	#include "../tables/triggers.tbl"
	TRIGGER_QTY,
};

extern bool IsTrigUsed(E_TRIGGERS id);
extern bool IsTrigUsed(const char *name);
extern void TriglistInit();
extern void TriglistClear();
extern void TriglistAdd(E_TRIGGERS id);
extern void TriglistAdd(const char *name);
extern void Triglist(long &total, long &used);
extern void TriglistPrint();


// Text mashers.

extern DIR_TYPE GetDirStr( LPCTSTR pszDir );
extern LPCTSTR GetTimeMinDesc( int dwMinutes );
extern size_t FindStrWord( LPCTSTR pTextSearch, LPCTSTR pszKeyWord );

#include "CLog.h"

//////////////////

class CDialogResponseArgs : public CScriptTriggerArgs
{
	// The scriptable return from a gump dialog.
	// "ARG" = dialog args script block. ex. ARGTXT(id), ARGCHK(i)
public:
	static const char *m_sClassName;
	struct TResponseString
	{
	public:
		const WORD m_ID;
		CGString const m_sText;

		TResponseString(WORD id, LPCTSTR pszText) : m_ID(id), m_sText(pszText)
		{
		}

	private:
		TResponseString(const TResponseString& copy);
		TResponseString& operator=(const TResponseString& other);
	};

	CGTypedArray<DWORD,DWORD>		m_CheckArray;
	CGObArray<TResponseString *>	m_TextArray;
public:
	void AddText( WORD id, LPCTSTR pszText );
	LPCTSTR GetName() const;
	bool r_WriteVal( LPCTSTR pszKey, CGString &sVal, CTextConsole * pSrc );

public:
	CDialogResponseArgs() { };

private:
	CDialogResponseArgs(const CDialogResponseArgs& copy);
	CDialogResponseArgs& operator=(const CDialogResponseArgs& other);
};

struct CMenuItem 	// describe a menu item.
{
public:
	WORD m_id;			// ITEMID_TYPE in base set.
	WORD m_color;
	CGString m_sText;
public:
	bool ParseLine( TCHAR * pszArgs, CScriptObj * pObjBase, CTextConsole * pSrc );
};

enum CLIMODE_TYPE	// What mode is the client to server connection in ? (waiting for input ?)
{
	// Setup events ------------------------------------------------------------------
	CLIMODE_SETUP_CONNECTING,
	CLIMODE_SETUP_SERVERS,			// client has received the servers list
	CLIMODE_SETUP_RELAY,			// client has been relayed to the game server. wait for new login
	CLIMODE_SETUP_CHARLIST,			// client has the char list and may (select char, delete char, create new char)

	// Capture the user input for this mode  -----------------------------------------
	CLIMODE_NORMAL,					// No targeting going on, we are just walking around, etc

	// Asyc events enum here  --------------------------------------------------------
	CLIMODE_DRAG,					// I'm dragging something (not quite a targeting but similar)
	CLIMODE_DYE,					// the dye dialog is up and I'm targeting something to dye
	CLIMODE_INPVAL,					// special text input dialog (for setting item attrib)

	// Some sort of general gump dialog ----------------------------------------------
	CLIMODE_DIALOG,					// from RES_DIALOG

	// Hard-coded (internal) dialogs
	CLIMODE_DIALOG_VIRTUE = 0x1CD,

	// Making a selection from a menu  -----------------------------------------------
	CLIMODE_MENU,					// from RES_MENU

	// Hard-coded (internal) menus
	CLIMODE_MENU_SKILL,				// result of some skill (tracking, tinkering, blacksmith, etc)
	CLIMODE_MENU_SKILL_TRACK_SETUP,
	CLIMODE_MENU_SKILL_TRACK,
	CLIMODE_MENU_GM_PAGES,			// open gm pages list
	CLIMODE_MENU_EDIT,				// edit the contents of a container

	// Prompting for text input ------------------------------------------------------
	CLIMODE_PROMPT_NAME_RUNE,
	CLIMODE_PROMPT_NAME_KEY,		// naming a key
	CLIMODE_PROMPT_NAME_SIGN,		// naming a house sign
	CLIMODE_PROMPT_NAME_SHIP,
	CLIMODE_PROMPT_GM_PAGE_TEXT,	// allowed to enter text for GM page
	CLIMODE_PROMPT_VENDOR_PRICE,	// what would you like the price to be?
	CLIMODE_PROMPT_TARG_VERB,		// send message to another player
	CLIMODE_PROMPT_SCRIPT_VERB,		// script verb
	CLIMODE_PROMPT_STONE_NAME,		// prompt for text

	// Targeting mouse cursor  -------------------------------------------------------
	CLIMODE_MOUSE_TYPE,				// greater than this = mouse type targeting

	// GM targeting command stuff
	CLIMODE_TARG_OBJ_SET,			// set some attribute of the item I will show
	CLIMODE_TARG_OBJ_INFO,			// what item do I want props for?
	CLIMODE_TARG_OBJ_FUNC,

	CLIMODE_TARG_UNEXTRACT,			// break out multi items
	CLIMODE_TARG_ADDCHAR,			// "ADDNPC" command
	CLIMODE_TARG_ADDITEM,			// "ADDITEM" command
	CLIMODE_TARG_LINK,				// "LINK" command
	CLIMODE_TARG_TILE,				// "TILE" command

	// Normal user stuff  (mouse targeting)
	CLIMODE_TARG_SKILL,				// targeting a skill or spell
	CLIMODE_TARG_SKILL_MAGERY,
	CLIMODE_TARG_SKILL_HERD_DEST,
	CLIMODE_TARG_SKILL_POISON,
	CLIMODE_TARG_SKILL_PROVOKE,

	CLIMODE_TARG_USE_ITEM,			// target for using the selected item
	CLIMODE_TARG_PET_CMD,			// targeted pet command
	CLIMODE_TARG_PET_STABLE,		// pick a creature to stable
	CLIMODE_TARG_REPAIR,			// attempt to repair an item
	CLIMODE_TARG_STONE_RECRUIT,		// recruit members for a stone (mouse select)
	CLIMODE_TARG_STONE_RECRUITFULL,	// recruit/make a member and set abbrev show
	CLIMODE_TARG_PARTY_ADD,

	CLIMODE_TARG_QTY
};

//////////////////////////////////////////////////////////////////////////
// Buff Icons

enum BUFF_ICONS
{
	BI_START,
	BI_NODISMOUNT = 0x3E9,
	BI_NOREARM = 0x3EA,
	BI_NIGHTSIGHT = 0x3ED,
	BI_DEATHSTRIKE,
	BI_EVILOMEN,
	BI_HEALINGTHROTTLE,
	BI_STAMINATHROTTLE,
	BI_DIVINEFURY,
	BI_ENEMYOFONE,
	BI_HIDDEN,
	BI_ACTIVEMEDITATION,
	BI_BLOODOATHCASTER,
	BI_BLOODOATHCURSE,
	BI_CORPSESKIN,
	BI_MINDROT,
	BI_PAINSPIKE,
	BI_STRANGLE,
	BI_GIFTOFRENEWAL,
	BI_ATTUNEWEAPON,
	BI_THUNDERSTORM,
	BI_ESSENCEOFWIND,
	BI_ETHEREALVOYAGE,
	BI_GIFTOFLIFE,
	BI_ARCANEEMPOWERMENT,
	BI_MORTALSTRIKE,
	BI_REACTIVEARMOR,
	BI_PROTECTION,
	BI_ARCHPROTECTION,
	BI_MAGICREFLECTION,
	BI_INCOGNITO,
	BI_DISGUISED,
	BI_ANIMALFORM,
	BI_POLYMORPH,
	BI_INVISIBILITY,
	BI_PARALYZE,
	BI_POISON,
	BI_BLEED,
	BI_CLUMSY,
	BI_FEEBLEMIND,
	BI_WEAKEN,
	BI_CURSE,
	BI_MASSCURSE,
	BI_AGILITY,
	BI_CUNNING,
	BI_STRENGTH,
	BI_BLESS,
	BI_SLEEP,
	BI_STONEFORM,
	BI_SPELLPLAGUE,
	BI_GARGOYLEBERSERK,
	BI_GARGOYLEFLY = 0x41E,
	// All icons below were added only on client patch 7.0.29.0
	BI_INSPIRE,
	BI_INVIGORATE,
	BI_RESILIENCE,
	BI_PERSEVERANCE,
	BI_TRIBULATIONDEBUFF,
	BI_DESPAIR,
	// BI_ARCANEEMPOWERMENT //already added 1026
	BI_FISHBUFF = 1062,
	BI_HITLOWERATTACK,
	BI_HITLOWERDEFENSE,
	BI_HITDUALWIELD,
	BI_BLOCK,
	BI_DEFENSEMASTERY,
	BI_DESPAIRDEBUFF,
	BI_HEALINGEFFECT,
	BI_SPELLFOCUSING,
	BI_SPELLFOCUSINGDEBUFF,
	BI_RAGEFOCUSINGDEBUFF,
	BI_RAGEFOCUSING,
	BI_WARDING,
	BI_TRIBULATION,
	BI_FORCEARROW,
	BI_DISARM,
	BI_SURGE,
	BI_FEIT,
	BI_TALONSTRIKE,
	BI_OHYSICATTACK,	//STRACTICS SAY PHYCHICATTACK?
	BI_CONSECRATE,
	BI_GRAPESOFWRATH,
	BI_ENEMYOFONEDEBUFF,
	BI_HORRIFICBEAST,
	BI_LICHFORM,
	BI_VAMPIRICEMBRACE,
	BI_CURSEWEAPON,
	BI_REAPERFORM,
	BI_INMOLATINGWEAPON,
	BI_ENCHANT,
	BI_HONORABLEEXECUTION,
	BI_CONFIDENCE,
	BI_EVASION,
	BI_COUNTERATTACK,
	BI_LIGHTNINGSTRIKE,
	BI_MOMENTUMSTRIKE,
	BI_ORANGEPETALS,
	BI_ROTPETALS,
	BI_POISONIMMUNITY,
	BI_VETERINARY,
	BI_PERFECTION,
	BI_HONORED,
	BI_MANAPHASE,
	BI_FANDANCERFANFIRE,
	BI_RAGE,
	BI_WEBBING,
	BI_MEDUSASTONE,
	BI_DRAGONSLASHERFEAR,
	BI_AURAOFNAUSEA,
	BI_HOWLOFCACOPHONY,
	BI_GAZEDESPAIR,
	BI_HIRYUPHYSICALRESISTANCE,
	BI_RUNEBEETLECORRUPTION,
	BI_BLOODWORMANEMIA,
	BI_ROTWORMBLOODDISEASE,
	BI_SKILLUSEDELAY,
	BI_FACTIONSTATLOSS,
	BI_HEATOFBATTLE,
	BI_CRIMINALSTATUS,
	BI_ARMORPIERCE,
	BI_SPLINTERINGEFFECT,
	BI_SWINGSPEEDSWING,
	BI_WRAITHFORM,
	//BI_HONORABLEEXECUTION	// already added on 1092
	BI_CITYTRADEDEAL = 1126,
	BI_QTY = 1126
};
// ---------------------------------------------------------------------------------------------

// Storage for Tooltip data while in trigger on an item
class CClientTooltip
{
public:
	static const char *m_sClassName;
	DWORD m_clilocid;
	TCHAR m_args[SCRIPT_MAX_LINE_LEN];

public:
	explicit CClientTooltip(DWORD clilocid, LPCTSTR args = NULL)
	{
		m_clilocid = clilocid;
		if ( args )
			strcpylen(m_args, args, SCRIPT_MAX_LINE_LEN);
		else
			m_args[0] = '\0';
	}

private:
	CClientTooltip(const CClientTooltip& copy);
	CClientTooltip& operator=(const CClientTooltip& other);

public:
	void __cdecl FormatArgs(LPCTSTR format, ...) __printfargs(2,3)
	{
		va_list vargs;
		va_start( vargs, format );

		if ( ! _vsnprintf( m_args, SCRIPT_MAX_LINE_LEN, format, vargs ) )
			strcpylen( m_args, format, SCRIPT_MAX_LINE_LEN );

		va_end( vargs );
	}
};


////////////////////////////////////////////////////////////////////////////////////

class Main : public AbstractSphereThread
{
public:
	Main();
	virtual ~Main() { };

private:
	Main(const Main& copy);
	Main& operator=(const Main& other);

public:
	// we increase the access level from protected to public in order to allow manual execution when
	// configuration disables using threads
	// TODO: in the future, such simulated functionality should lie in AbstractThread inself instead of hacks
	virtual void tick();

protected:
	virtual void onStart();
	virtual bool shouldExit();
};

//////////////////////////////////////////////////////////////

extern LPCTSTR g_szServerDescription;
extern int g_szServerBuild;
extern LPCTSTR const g_Stat_Name[STAT_QTY];
extern CGStringList g_AutoComplete;

extern int Sphere_InitServer( int argc, char *argv[] );
extern int Sphere_OnTick();
extern void Sphere_ExitServer();
extern int Sphere_MainEntryPoint( int argc, char *argv[] );

///////////////////////////////////////////////////////////////
// -CGrayUID

inline INT64 CObjBase::GetTimerDiff() const
{
	// How long till this will expire ?
	return( g_World.GetTimeDiff( m_timeout ));
}
inline CObjBase * CGrayUIDBase::ObjFind() const
{
	if ( IsResource())
		return( NULL );
	return( g_World.FindUID( m_dwInternalVal & UID_O_INDEX_MASK ));
}
inline CItem * CGrayUIDBase::ItemFind() const
{
	// IsItem() may be faster ?
	return( dynamic_cast <CItem *>( ObjFind()));
}
inline CChar * CGrayUIDBase::CharFind() const
{
	return( dynamic_cast <CChar *>( ObjFind()));
}

// ---------------------------------------------------------------------------------------------


struct TScriptProfiler
{
	unsigned char	initstate;
	DWORD		called;
	LONGLONG	total;
	struct TScriptProfilerFunction
	{
		TCHAR		name[128];	// name of the function
		DWORD		called;		// how many times called
		LONGLONG	total;		// total executions time
		LONGLONG	min;		// minimal executions time
		LONGLONG	max;		// maximal executions time
		LONGLONG	average;	// average executions time
		TScriptProfilerFunction *next;
	}		*FunctionsHead, *FunctionsTail;
	struct TScriptProfilerTrigger
	{
		TCHAR		name[128];	// name of the trigger
		DWORD		called;		// how many times called
		LONGLONG	total;		// total executions time
		LONGLONG	min;		// minimal executions time
		LONGLONG	max;		// maximal executions time
		LONGLONG	average;	// average executions time
		TScriptProfilerTrigger *next;
	}		*TriggersHead, *TriggersTail;
};
extern TScriptProfiler g_profiler;

//	Time measurement macros
extern LONGLONG llTimeProfileFrequency;

#ifdef _WIN32

#define	TIME_PROFILE_INIT	\
	LONGLONG llTicks(0), llTicksEnd
#define	TIME_PROFILE_START	\
	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicks)) llTicks = GetTickCount()
#define TIME_PROFILE_END	if ( !QueryPerformanceCounter((LARGE_INTEGER *)&llTicksEnd)) llTicksEnd = GetTickCount()

#else

#define	TIME_PROFILE_INIT	\
	LONGLONG llTicks(0), llTicksEnd
#define	TIME_PROFILE_START	\
	llTicks = GetTickCount()
#define TIME_PROFILE_END	llTicksEnd = GetTickCount();

#endif

#define TIME_PROFILE_GET_HI	((llTicksEnd - llTicks)/(llTimeProfileFrequency/1000))
#define	TIME_PROFILE_GET_LO	((((llTicksEnd - llTicks)*10000)/(llTimeProfileFrequency/1000))%10000)

#endif

