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

enum RESDISPLAY_VERSION : char
{
	RDS_PRET2A,
	RDS_T2A,
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


//////////////////////////////////////////////////////////////////////////
// Combat

enum BODYPART_TYPE
{
    ARMOR_HEAD = 0,
    ARMOR_NECK,
    ARMOR_BACK,
    ARMOR_CHEST,    // or thorax
    ARMOR_ARMS,
    ARMOR_HANDS,
    ARMOR_LEGS,
    ARMOR_FEET,
	ARMOR_SHIELD,
    ARMOR_QTY,      // All the parts that armor will cover.

    BODYPART_LEGS2, // Alternate set of legs (spider)
    BODYPART_TAIL,  // Dragon, Snake, Alligator, etc. (tail attack?)
    BODYPART_WINGS, // Dragon, Mongbat, Gargoyle
    BODYPART_CLAWS, // can't wear any gloves here!
    BODYPART_HOOVES,// No shoes
    BODYPART_HORNS, // Bull, Daemon

    BODYPART_STALKS,    // Gazer or Corpser
    BODYPART_BRANCHES,  // Reaper.
    BODYPART_TRUNK,     // Reaper.
    BODYPART_PSEUDOPOD, // Slime
    BODYPART_ABDOMEN,   // Spider or insect. asusme throax and chest are the same.

    BODYPART_QTY
};

//////////////////////////////////////////////////////////////////////////
// Climodes

enum CLIMODE_TYPE	// What mode is the client to server connection in ? (waiting for input ?)
{
    // Setup events ------------------------------------------------------------------
	CLIMODE_SETUP_CONNECTING,
    CLIMODE_SETUP_SERVERS,          // client has received the servers list
    CLIMODE_SETUP_RELAY,            // client has been relayed to the game server. wait for new login
    CLIMODE_SETUP_CHARLIST,         // client has the char list and may (select char, delete char, create new char)

    // Capture the user input for this mode  -----------------------------------------
	CLIMODE_NORMAL,                 // No targeting going on, we are just walking around, etc

    // Asyc events enum here  --------------------------------------------------------
	CLIMODE_DRAG,                   // I'm dragging something (not quite a targeting but similar)
    CLIMODE_DYE,                    // the dye dialog is up and I'm targeting something to dye
    CLIMODE_INPVAL,                 // special text input dialog (for setting item attrib)

    // Some sort of general gump dialog ----------------------------------------------
	CLIMODE_DIALOG,                 // from RES_DIALOG

    // Hard-coded (internal) dialogs
	CLIMODE_DIALOG_VIRTUE           = 0x1CD,
	CLIMODE_DIALOG_FACESELECTION    = 0x2B0,    // enhanced clients only

    // Making a selection from a menu  -----------------------------------------------
	CLIMODE_MENU,                   // from RES_MENU

    // Hard-coded (internal) menus
	CLIMODE_MENU_SKILL,             // result of some skill (tracking, tinkering, blacksmith, etc)
    CLIMODE_MENU_SKILL_TRACK_SETUP,
    CLIMODE_MENU_SKILL_TRACK,
    CLIMODE_MENU_EDIT,              // edit the contents of a container

    // Prompting for text input ------------------------------------------------------
    CLIMODE_PROMPT_NAME_KEY,        // naming a key
    CLIMODE_PROMPT_NAME_PET,        // naming a pet
    CLIMODE_PROMPT_NAME_RUNE,
    CLIMODE_PROMPT_NAME_SHIP,
    CLIMODE_PROMPT_NAME_SIGN,       // naming a house sign

    CLIMODE_PROMPT_GM_PAGE_TEXT,    // allowed to enter text for GM page
    CLIMODE_PROMPT_VENDOR_PRICE,    // what would you like the price to be?
    CLIMODE_PROMPT_TARG_VERB,       // send message to another player
    CLIMODE_PROMPT_SCRIPT_VERB,     // script verb
    CLIMODE_PROMPT_STONE_NAME,      // prompt for text

    // Targeting mouse cursor  -------------------------------------------------------
	CLIMODE_MOUSE_TYPE,             // greater than this = mouse type targeting

    // GM targeting command stuff
	CLIMODE_TARG_OBJ_SET,           // set some attribute of the item I will show
    CLIMODE_TARG_OBJ_INFO,          // what item do I want props for?
    CLIMODE_TARG_OBJ_FUNC,

    CLIMODE_TARG_UNEXTRACT,         // break out multi items
    CLIMODE_TARG_ADDCHAR,           // "ADDNPC" command
    CLIMODE_TARG_ADDITEM,           // "ADDITEM" command
    CLIMODE_TARG_LINK,              // "LINK" command
    CLIMODE_TARG_TILE,              // "TILE" command

    // Normal user stuff  (mouse targeting)
	CLIMODE_TARG_SKILL,             // targeting a skill or spell
    CLIMODE_TARG_SKILL_MAGERY,
    CLIMODE_TARG_SKILL_HERD_DEST,
    CLIMODE_TARG_SKILL_POISON,
    CLIMODE_TARG_SKILL_PROVOKE,

    CLIMODE_TARG_USE_ITEM,          // target for using the selected item
    CLIMODE_TARG_PET_CMD,           // targeted pet command
    CLIMODE_TARG_PET_STABLE,        // pick a creature to stable
    CLIMODE_TARG_REPAIR,            // attempt to repair an item
    CLIMODE_TARG_STONE_RECRUIT,     // recruit members for a stone (mouse select)
    CLIMODE_TARG_STONE_RECRUITFULL, // recruit/make a member and set abbrev show
    CLIMODE_TARG_PARTY_ADD,
    CLIMODE_TARG_GLOBALCHAT_ADD,

    CLIMODE_TARG_QTY
};



//////////////////////////////////////////////////////////////////////////
// Buff Icons

enum BUFF_ICONS
{
    BI_START,
    BI_NOREMOUNT = 0x3E9,
    BI_NOREARM,
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
    BI_INSPIRE,
    BI_INVIGORATE,
    BI_RESILIENCE,
    BI_PERSEVERANCE,
    BI_TRIBULATIONDEBUFF,
    BI_DESPAIR,
    BI_FISHPIE = 0x426,
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
    BI_FEINT,
    BI_TALONSTRIKE,
    BI_PHYSICATTACK,
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
    BI_ROSEOFTRINSIC,
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
    BI_SWINGSPEEDDEBUFF,
    BI_WRAITHFORM,
    BI_CITYTRADEDEAL = 0x466,
    BI_HUMILITYDEBUFF,
    BI_SPIRITUALITY,
    BI_HUMILITY,
    BI_RAMPAGE,
    BI_STAGGERDEBUFF,
    BI_TOUGHNESS,
    BI_THRUST,
    BI_PIERCEDEBUFF,
    BI_PLAYINGTHEODDS,
    BI_FOCUSEDEYE,
    BI_ONSLAUGHTDEBUFF,
    BI_ELEMENTALFURY,
    BI_ELEMENTALFURYDEBUFF,
    BI_CALLEDSHOT,
    BI_KNOCKOUT,
    BI_WARRIORSGIFTS,		// previously known as Saving Throw
    BI_CONDUIT,
    BI_ETHEREALBURST,
    BI_MYSTICWEAPON,
    BI_MANASHIELD,
    BI_ANTICIPATEHIT,
    BI_WARCRY,
    BI_SHADOW,
    BI_WHITETIGERFORM,
    BI_BODYGUARD,
    BI_HEIGHTENEDSENSES,
    BI_TOLERANCE,
    BI_DEATHRAY,
    BI_DEATHRAYDEBUFF,
    BI_INTUITION,
    BI_ENCHANTEDSUMMONING,
    BI_SHIELDBASH,
    BI_WHISPERING,
    BI_COMBATTRAINING,
    BI_INJECTEDSTRIKEDEBUFF,
    BI_INJECTEDSTRIKE,
    BI_UNKNOWNTOMATO,
    BI_PLAYINGTHEODDSDEBUFF,
    BI_DRAGONTURTLEDEBUFF,
    BI_BOARDING,
    BI_POTENCY,
    BI_THRUSTDEBUFF,
    BI_FISTSOFFURY,
    BI_BARRABHEMOLYMPHCONCENTRATE,
    BI_JUKARIBURNPOULTICE,
    BI_KURAKAMBUSHERSESSENCE,
    BI_BARAKODRAFTOFMIGHT,
    BI_URALITRANCETONIC,
    BI_SAKKHRAPROPHYLAXIS,
    BI_SPARKSDEBUFF,
    BI_SWARMDEBUFF,
    BI_BONEBREAKERDEBUFF,
    BI_SPARKS,
    BI_SWARM,
    BI_BONEBREAKER,
	BI_STATUETRANSFORMATION,
	BI_CORASMANARAGE,
	BI_VIRTUESHIELD,
	BI_FEINTDEBUFF,
	BI_CADDELLITEINFUSED,
	BI_POTIONOFGLORIOUSFORTUNE,
	BI_MYSTICALPOLYMORPHTOTEM,
	BI_DISCORDANCEDEBUFF,
	BI_CHALICEOFPILFERINGPROTECTION,
	BI_BATTLELUST,
    BI_QTY
};

#endif // _INC_ENUMS_H
