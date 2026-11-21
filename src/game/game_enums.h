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
	CLIMODE_SETUP_CONNECTING = 0,
    CLIMODE_SETUP_SERVERS = 0x1,          // client has received the servers list
    CLIMODE_SETUP_RELAY = 0x2,            // client has been relayed to the game server. wait for new login
    CLIMODE_SETUP_CHARLIST = 0x3,         // client has the char list and may (select char, delete char, create new char)

    // Capture the user input for this mode  -----------------------------------------
	CLIMODE_NORMAL = 0x4,                 // No targeting going on, we are just walking around, etc

    // Asyc events enum here  --------------------------------------------------------
	CLIMODE_DRAG = 0x5,                   // I'm dragging something (not quite a targeting but similar)
    CLIMODE_DYE = 0x6,                    // the dye dialog is up and I'm targeting something to dye
    CLIMODE_INPVAL = 0x7,                 // special text input dialog (for setting item attrib)

    // Some sort of general gump dialog ----------------------------------------------
	CLIMODE_DIALOG = 0x8,                 // from RES_DIALOG

    // Hard-coded (internal) dialogs
	CLIMODE_DIALOG_VIRTUE           = 0x1CD,
	CLIMODE_DIALOG_FACESELECTION    = 0x2B0,    // enhanced clients only

    // Making a selection from a menu  -----------------------------------------------
	CLIMODE_MENU = 0x2b1,                   // from RES_MENU

    // Hard-coded (internal) menus
	CLIMODE_MENU_SKILL = 0x2b2,             // result of some skill (tracking, tinkering, blacksmith, etc)
    CLIMODE_MENU_SKILL_TRACK_SETUP = 0x2b3,
    CLIMODE_MENU_SKILL_TRACK = 0x2b4,
    CLIMODE_MENU_EDIT = 0x2b5,              // edit the contents of a container

    // Prompting for text input ------------------------------------------------------
    CLIMODE_PROMPT_NAME_KEY = 0x2b6,        // naming a key
    CLIMODE_PROMPT_NAME_PET = 0x2b7,        // naming a pet
    CLIMODE_PROMPT_NAME_RUNE = 0x2b8,
    CLIMODE_PROMPT_NAME_SHIP = 0x2b9,
    CLIMODE_PROMPT_NAME_SIGN = 0x2ba,       // naming a house sign

    CLIMODE_PROMPT_GM_PAGE_TEXT = 0x2bb,    // allowed to enter text for GM page
    CLIMODE_PROMPT_VENDOR_PRICE = 0x2bc,    // what would you like the price to be?
    CLIMODE_PROMPT_TARG_VERB = 0x2bd,       // send message to another player
    CLIMODE_PROMPT_SCRIPT_VERB = 0x2be,     // script verb
    CLIMODE_PROMPT_STONE_NAME = 0x2bf,      // prompt for text

    // Targeting mouse cursor  -------------------------------------------------------
	CLIMODE_MOUSE_TYPE = 0x2c0,             // greater than this = mouse type targeting

    // GM targeting command stuff
	CLIMODE_TARG_OBJ_SET = 0x2c1,           // set some attribute of the item I will show
    CLIMODE_TARG_OBJ_INFO = 0x2c2,          // what item do I want props for?
    CLIMODE_TARG_OBJ_FUNC = 0x2c3,

    CLIMODE_TARG_UNEXTRACT = 0x2c4,         // break out multi items
    CLIMODE_TARG_ADDCHAR = 0x2c5,           // "ADDNPC" command
    CLIMODE_TARG_ADDITEM = 0x2c6,           // "ADDITEM" command
    CLIMODE_TARG_LINK = 0x2c7,              // "LINK" command
    CLIMODE_TARG_TILE = 0x2c8,              // "TILE" command

    // Normal user stuff  (mouse targeting)
	CLIMODE_TARG_SKILL = 0x2c9,             // targeting a skill or spell
    CLIMODE_TARG_SKILL_MAGERY = 0x2ca,
    CLIMODE_TARG_SKILL_HERD_DEST = 0x2cb,
    CLIMODE_TARG_SKILL_POISON = 0x2cc,
    CLIMODE_TARG_SKILL_PROVOKE = 0x2cd,

    CLIMODE_TARG_USE_ITEM = 0x2ce,          // target for using the selected item
    CLIMODE_TARG_PET_CMD = 0x2cf,           // targeted pet command
    CLIMODE_TARG_PET_STABLE = 0x2d0,        // pick a creature to stable
    CLIMODE_TARG_REPAIR = 0x2d1,            // attempt to repair an item
    CLIMODE_TARG_STONE_RECRUIT = 0x2d2,     // recruit members for a stone (mouse select)
    CLIMODE_TARG_STONE_RECRUITFULL = 0x2d3, // recruit/make a member and set abbrev show
    CLIMODE_TARG_PARTY_ADD = 0x2d4,
    CLIMODE_TARG_GLOBALCHAT_ADD = 0x2d5,

    CLIMODE_TARG_QTY = 0x2d6
};


//////////////////////////////////////////////////////////////////////////
// Buff Icons

enum BUFF_ICONS
{
    BI_START = 0,
    BI_NOREMOUNT = 0x3e9,
    BI_NOREARM = 0x3ea,
    BI_NIGHTSIGHT = 0x3ed,
    BI_DEATHSTRIKE = 0x3ee,
    BI_EVILOMEN = 0x3ef,
    BI_HEALINGTHROTTLE = 0x3f0,
    BI_STAMINATHROTTLE = 0x3f1,
    BI_DIVINEFURY = 0x3f2,
    BI_ENEMYOFONE = 0x3f3,
    BI_HIDDEN = 0x3f4,
    BI_ACTIVEMEDITATION = 0x3f5,
    BI_BLOODOATHCASTER = 0x3f6,
    BI_BLOODOATHCURSE = 0x3f7,
    BI_CORPSESKIN = 0x3f8,
    BI_MINDROT = 0x3f9,
    BI_PAINSPIKE = 0x3fa,
    BI_STRANGLE = 0x3fb,
    BI_GIFTOFRENEWAL = 0x3fc,
    BI_ATTUNEWEAPON = 0x3fd,
    BI_THUNDERSTORM = 0x3fe,
    BI_ESSENCEOFWIND = 0x3ff,
    BI_ETHEREALVOYAGE = 0x400,
    BI_GIFTOFLIFE = 0x401,
    BI_ARCANEEMPOWERMENT = 0x402,
    BI_MORTALSTRIKE = 0x403,
    BI_REACTIVEARMOR = 0x404,
    BI_PROTECTION = 0x405,
    BI_ARCHPROTECTION = 0x406,
    BI_MAGICREFLECTION = 0x407,
    BI_INCOGNITO = 0x408,
    BI_DISGUISED = 0x409,
    BI_ANIMALFORM = 0x40a,
    BI_POLYMORPH = 0x40b,
    BI_INVISIBILITY = 0x40c,
    BI_PARALYZE = 0x40d,
    BI_POISON = 0x40e,
    BI_BLEED = 0x40f,
    BI_CLUMSY = 0x410,
    BI_FEEBLEMIND = 0x411,
    BI_WEAKEN = 0x412,
    BI_CURSE = 0x413,
    BI_MASSCURSE = 0x414,
    BI_AGILITY = 0x415,
    BI_CUNNING = 0x416,
    BI_STRENGTH = 0x417,
    BI_BLESS = 0x418,
    BI_SLEEP = 0x419,
    BI_STONEFORM = 0x41a,
    BI_SPELLPLAGUE = 0x41b,
    BI_GARGOYLEBERSERK = 0x41c,
    BI_GARGOYLEFLY = 0x41e,
    BI_INSPIRE = 0x41f,
    BI_INVIGORATE = 0x420,
    BI_RESILIENCE = 0x421,
    BI_PERSEVERANCE = 0x422,
    BI_TRIBULATIONDEBUFF = 0x423,
    BI_DESPAIR = 0x424,
    BI_FISHPIE = 0x426,
    BI_HITLOWERATTACK = 0x427,
    BI_HITLOWERDEFENSE = 0x428,
    BI_HITDUALWIELD = 0x429,
    BI_BLOCK = 0x42a,
    BI_DEFENSEMASTERY = 0x42b,
    BI_DESPAIRDEBUFF = 0x42c,
    BI_HEALINGEFFECT = 0x42d,
    BI_SPELLFOCUSING = 0x42e,
    BI_SPELLFOCUSINGDEBUFF = 0x42f,
    BI_RAGEFOCUSINGDEBUFF = 0x430,
    BI_RAGEFOCUSING = 0x431,
    BI_WARDING = 0x432,
    BI_TRIBULATION = 0x433,
    BI_FORCEARROW = 0x434,
    BI_DISARM = 0x435,
    BI_SURGE = 0x436,
    BI_FEINT = 0x437,
    BI_TALONSTRIKE = 0x438,
    BI_PHYSICATTACK = 0x439,
    BI_CONSECRATE = 0x43a,
    BI_GRAPESOFWRATH = 0x43b,
    BI_ENEMYOFONEDEBUFF = 0x43c,
    BI_HORRIFICBEAST = 0x43d,
    BI_LICHFORM = 0x43e,
    BI_VAMPIRICEMBRACE = 0x43f,
    BI_CURSEWEAPON = 0x440,
    BI_REAPERFORM = 0x441,
    BI_INMOLATINGWEAPON = 0x442,
    BI_ENCHANT = 0x443,
    BI_HONORABLEEXECUTION = 0x444,
    BI_CONFIDENCE = 0x445,
    BI_EVASION = 0x446,
    BI_COUNTERATTACK = 0x447,
    BI_LIGHTNINGSTRIKE = 0x448,
    BI_MOMENTUMSTRIKE = 0x449,
    BI_ORANGEPETALS = 0x44a,
    BI_ROSEOFTRINSIC = 0x44b,
    BI_POISONIMMUNITY = 0x44c,
    BI_VETERINARY = 0x44d,
    BI_PERFECTION = 0x44e,
    BI_HONORED = 0x44f,
    BI_MANAPHASE = 0x450,
    BI_FANDANCERFANFIRE = 0x451,
    BI_RAGE = 0x452,
    BI_WEBBING = 0x453,
    BI_MEDUSASTONE = 0x454,
    BI_DRAGONSLASHERFEAR = 0x455,
    BI_AURAOFNAUSEA = 0x456,
    BI_HOWLOFCACOPHONY = 0x457,
    BI_GAZEDESPAIR = 0x458,
    BI_HIRYUPHYSICALRESISTANCE = 0x459,
    BI_RUNEBEETLECORRUPTION = 0x45a,
    BI_BLOODWORMANEMIA = 0x45b,
    BI_ROTWORMBLOODDISEASE = 0x45c,
    BI_SKILLUSEDELAY = 0x45d,
    BI_FACTIONSTATLOSS = 0x45e,
    BI_HEATOFBATTLE = 0x45f,
    BI_CRIMINALSTATUS = 0x460,
    BI_ARMORPIERCE = 0x461,
    BI_SPLINTERINGEFFECT = 0x462,
    BI_SWINGSPEEDDEBUFF = 0x463,
    BI_WRAITHFORM = 0x464,
    BI_CITYTRADEDEAL = 0x466,
    BI_HUMILITYDEBUFF = 0x467,
    BI_SPIRITUALITY = 0x468,
    BI_HUMILITY = 0x469,
    BI_RAMPAGE = 0x46a,
    BI_STAGGERDEBUFF = 0x46b,
    BI_TOUGHNESS = 0x46c,
    BI_THRUST = 0x46d,
    BI_PIERCEDEBUFF = 0x46e,
    BI_PLAYINGTHEODDS = 0x46f,
    BI_FOCUSEDEYE = 0x470,
    BI_ONSLAUGHTDEBUFF = 0x471,
    BI_ELEMENTALFURY = 0x472,
    BI_ELEMENTALFURYDEBUFF = 0x473,
    BI_CALLEDSHOT = 0x474,
    BI_KNOCKOUT = 0x475,
    BI_WARRIORSGIFTS = 0x476,		// previously known as Saving Throw
    BI_CONDUIT = 0x477,
    BI_ETHEREALBURST = 0x478,
    BI_MYSTICWEAPON = 0x479,
    BI_MANASHIELD = 0x47a,
    BI_ANTICIPATEHIT = 0x47b,
    BI_WARCRY = 0x47c,
    BI_SHADOW = 0x47d,
    BI_WHITETIGERFORM = 0x47e,
    BI_BODYGUARD = 0x47f,
    BI_HEIGHTENEDSENSES = 0x480,
    BI_TOLERANCE = 0x481,
    BI_DEATHRAY = 0x482,
    BI_DEATHRAYDEBUFF = 0x483,
    BI_INTUITION = 0x484,
    BI_ENCHANTEDSUMMONING = 0x485,
    BI_SHIELDBASH = 0x486,
    BI_WHISPERING = 0x487,
    BI_COMBATTRAINING = 0x488,
    BI_INJECTEDSTRIKEDEBUFF = 0x489,
    BI_INJECTEDSTRIKE = 0x48a,
    BI_UNKNOWNTOMATO = 0x48b,
    BI_PLAYINGTHEODDSDEBUFF = 0x48c,
    BI_DRAGONTURTLEDEBUFF = 0x48d,
    BI_BOARDING = 0x48e,
    BI_POTENCY = 0x48f,
    BI_THRUSTDEBUFF = 0x490,
    BI_FISTSOFFURY = 0x491,
    BI_BARRABHEMOLYMPHCONCENTRATE = 0x492,
    BI_JUKARIBURNPOULTICE = 0x493,
    BI_KURAKAMBUSHERSESSENCE = 0x494,
    BI_BARAKODRAFTOFMIGHT = 0x495,
    BI_URALITRANCETONIC = 0x496,
    BI_SAKKHRAPROPHYLAXIS = 0x497,
    BI_SPARKSDEBUFF = 0x498,
    BI_SWARMDEBUFF = 0x499,
    BI_BONEBREAKERDEBUFF = 0x49a,
    BI_SPARKS = 0x49b,
    BI_SWARM = 0x49c,
    BI_BONEBREAKER = 0x49d,
	BI_STATUETRANSFORMATION = 0x49e,
	BI_CORASMANARAGE = 0x49f,
	BI_VIRTUESHIELD = 0x4a0,
	BI_FEINTDEBUFF = 0x4a1,
	BI_CADDELLITEINFUSED = 0x4a2,
	BI_POTIONOFGLORIOUSFORTUNE = 0x4a3,
	BI_MYSTICALPOLYMORPHTOTEM = 0x4a4,
	BI_DISCORDANCEDEBUFF = 0x4a5,
	BI_CHALICEOFPILFERINGPROTECTION = 0x4a6,
	BI_BATTLELUST = 0x4a7,
    BI_QTY = 0x4a8
};

#endif // _INC_ENUMS_H
