/**
* @file game_enums.h
* @brief Enums commonly used in the "game" folder. 
*/

#ifndef _INC_GAME_ENUMS_H
#define _INC_GAME_ENUMS_H

enum ePayGold
{
    PAYGOLD_TRAIN,
    PAYGOLD_BUY,
    PAYGOLD_HIRE
};

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


enum TOOLTIPMODE_TYPE
{
	TOOLTIPMODE_SENDFULL = 0x00,	// always send full tooltip packet
	TOOLTIPMODE_SENDVERSION = 0x01	// send version packet and wait for client to request full tooltip
};

enum RACIALFLAGS_TYPE
{
	RACIALF_HUMAN_STRONGBACK = 0x0001,		// Increase carrying capacity (+60 stones of weight)
	RACIALF_HUMAN_TOUGH = 0x0002,		// Regenerate hitpoints faster (+2 Hit Point Regeneration)
	RACIALF_HUMAN_WORKHORSE = 0x0004,		// Find more resources while gathering hides, ore and lumber
	RACIALF_HUMAN_JACKOFTRADES = 0x0008,		// Skill calculations always consider 20.0 minimum ability on untrained skills
	RACIALF_ELF_NIGHTSIGHT = 0x0010,		// Permanent night sight effect
	RACIALF_ELF_DIFFTRACK = 0x0020,		// Increase difficulty to be tracked while hidden/invisible
	RACIALF_ELF_WISDOM = 0x0040,		// Permanent max mana bonus (+20 Mana Increase)
	RACIALF_GARG_FLY = 0x0080,		// Enable gargoyle fly ability (FEATURE_AOS_UPDATE_B is required to enable gargoyle ability book)
	RACIALF_GARG_BERSERK = 0x0100,		// Increase ferocity in situations of danger (15% Damage Increase + 3% Spell Damage Increase for each 20hp lost)
	RACIALF_GARG_DEADLYAIM = 0x0200,		// Throwing calculations always consider 20.0 minimum ability when untrained
	RACIALF_GARG_MYSTICINSIGHT = 0x0400		// Mysticism calculations always consider 30.0 minimum ability when untrained
};



//////////////////////////////////////////////////////////////////////////
// Skills

enum SKTRIG_TYPE
{
	// All skills may be scripted.
	// XTRIG_UNKNOWN    = some named trigger not on this list.
	SKTRIG_ABORT = 1,  	// Some odd thing went wrong.
	SKTRIG_FAIL,        // we failed the skill check.
	SKTRIG_GAIN,        // called when there is a chance to gain skill
	SKTRIG_PRESTART,    // called before any hardcoded messages
	SKTRIG_SELECT,      // just selecting params for the skill
	SKTRIG_START,       // params for skill are done. (stroke)
	SKTRIG_STROKE,      // Not really a trigger! Just a stage.
	SKTRIG_SUCCESS,     // we passed the skill check
	SKTRIG_TARGETCANCEL,// called when a target cursor is cancelled
	SKTRIG_USEQUICK,    // called when a 'quick' usage of the skill is made
	SKTRIG_WAIT,        // called when a test is made to see if the character must wait before starting
	SKTRIG_QTY
};


enum SKF_TYPE
{
	SKF_SCRIPTED = 0x0001,		// fully scripted, no hardcoded behaviour
	SKF_FIGHT = 0x0002,			// considered a fight skill, maintains fight active
	SKF_MAGIC = 0x0004,			// considered a magic skill
	SKF_CRAFT = 0x0008,			// considered a crafting skill, compatible with MAKEITEM function
	SKF_IMMOBILE = 0x0010,		// fails skill if character moves
	SKF_SELECTABLE = 0x0020,	// allows skill to be selected from the skill menu
	SKF_NOMINDIST = 0x0040,		// you can mine, fish, chop, hack on the same point you are standing on
	SKF_NOANIM = 0x0080,		// prevents hardcoded animation from playing
	SKF_NOSFX = 0x0100,			// prevents hardcoded sound from playing
	SKF_RANGED = 0x0200,		// Considered a ranged skill (combine with SKF_FIGHT)
	SKF_GATHER = 0x0400,		// Considered a gathering skill, using SkillStrokes as SKF_CRAFT
	SKF_DISABLED = 0x0800		// Disabled skill, can't be used.
};


//////////////////////////////////////////////////////////////////////////
// Combat

enum BODYPART_TYPE
{
	ARMOR_HEAD = 0,
	ARMOR_NECK,
	ARMOR_BACK,
	ARMOR_CHEST,	// or thorax
	ARMOR_ARMS,
	ARMOR_HANDS,
	ARMOR_LEGS,
	ARMOR_FEET,
	ARMOR_QTY,		// All the parts that armor will cover.

	BODYPART_LEGS2,	// Alternate set of legs (spider)
	BODYPART_TAIL,	// Dragon, Snake, Alligator, etc. (tail attack?)
	BODYPART_WINGS,	// Dragon, Mongbat, Gargoyle
	BODYPART_CLAWS,	// can't wear any gloves here!
	BODYPART_HOOVES,	// No shoes
	BODYPART_HORNS,	// Bull, Daemon

	BODYPART_STALKS,		// Gazer or Corpser
	BODYPART_BRANCHES,	// Reaper.
	BODYPART_TRUNK,		// Reaper.
	BODYPART_PSEUDOPOD,	// Slime
	BODYPART_ABDOMEN,		// Spider or insect. asusme throax and chest are the same.

	BODYPART_QTY
};

enum COMBATFLAGS_TYPE
{
	COMBAT_NODIRCHANGE = 0x0000001,	// not rotate player when fighting
	COMBAT_FACECOMBAT = 0x0000002,	// allow faced combat only
	COMBAT_PREHIT = 0x0000004,	// allow prehit for close combat. first hit is instant (delay 0.1sec)
	COMBAT_ELEMENTAL_ENGINE = 0x0000008,	// use DAM*/RES* to split damage/resist into Physical/Fire/Cold/Poison/Energy (AOS) instead use old AR (pre-AOS)
	COMBAT_DCLICKSELF_UNMOUNTS = 0x0000020,	// unmount horse when dclicking self while in warmode
	COMBAT_ALLOWHITFROMSHIP = 0x0000040,	// allow attacking opponents from ships
	COMBAT_ARCHERYCANMOVE = 0x0000100,	// allow firing bow while moving
	COMBAT_STAYINRANGE = 0x0000200,	// must be in range at the end of the swing or the hit will miss
	COMBAT_STACKARMOR = 0x0001000,	// if a region is covered by more than one armor part, all AR will count
	COMBAT_NOPOISONHIT = 0x0002000		// Disables old (55i like) poisoning style: Poisoning > 30.0 && (RAND(100.0)> Poisoning) for monsters OR weapon.morez && (RAND(100) < weapon.morez ) for poisoned weapons.
};


//////////////////////////////////////////////////////////////////////////
//Magic

enum MAGICFLAGS_TYPE
{
	MAGICF_NODIRCHANGE = 0x0000001,	// not rotate player when casting/targeting
	MAGICF_PRECAST = 0x0000002,	// use precasting (cast spell before targeting)
	MAGICF_IGNOREAR = 0x0000004,	// magic ignore ar
	MAGICF_CANHARMSELF = 0x0000008,	// i can do damage on self
	MAGICF_STACKSTATS = 0x0000010,	// allow multiple stat spells at once
	MAGICF_FREEZEONCAST = 0x0000020,	// disallow movement whilst casting
	MAGICF_SUMMONWALKCHECK = 0x0000040,	// disallow summoning creatures to places they can't normally step
	MAGICF_NOFIELDSOVERWALLS = 0x0000080,	// prevent fields from being formed over blocking objects.
	MAGICF_NOANIM = 0x0000100,	// auto spellflag_no_anim on all spells
	MAGICF_OSIFORMULAS = 0x0000200,	// calculated damage and duration based on OSI formulas
	MAGICF_NOCASTFROZENHANDS = 0x0000400,	// can't cast spells if got paralyzed holding something on hands
	MAGICF_POLYMORPHSTATS = 0x0000800,	// Polymorph spells give out stats based on base chars (old behaviour backwards).
	MAGICF_OVERRIDEFIELDS = 0x0001000,	// Prevent cast multiple field spells on the same tile, making the new field tile remove the previous field
};

enum REVEALFLAGS_TYPE
{
	REVEALF_DETECTINGHIDDEN = 0x001,		///* Reveal Spell with Detecting Hidden Skill.
	REVEALF_LOOTINGSELF = 0x002,		///* Reveal when looting self bodies.
	REVEALF_LOOTINGOTHERS = 0x004,		///* Reveal when looting bodies of other Players or NPCs.
	REVEALF_SPEAK = 0x008,		///* Reveal when speaking.
	REVEALF_SPELLCAST = 0x010			///* Reveal when starting to cast a Spell.
};


//////////////////////////////////////////////////////////////////////////
// Climodes

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


#endif // _INC_ENUMS_H
