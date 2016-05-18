/**
* @file uofiles_enums.h
*
*/

#pragma once
#ifndef _INC_UOFILES_ENUMS_H
#define _INC_UOFILES_ENUMS_H


// 20 colors of 10 hues and 5 brightnesses, which gives us 1000 colors.
//  plus black, and default.
// Skin color is similar, except there are 7 basic colors of 8 hues.
// For hair there are 7 colors of 7 hues.

enum HUE_CODE
{
    HUE_DEFAULT			= 0x0000,

    HUE_BLUE_LOW		= 0x0002,	// lowest dyeable color.
    HUE_BLUE_NAVY		= 0x0003,
    HUE_RED_DARK		= 0x0020,
    HUE_RED				= 0x0022,
    HUE_ORANGE			= 0x002b,
    HUE_YELLOW			= 0x0035,
    HUE_GREEN_LIGHT		= 0x0040,

    HUE_BLUE_DARK		= 0x0061,
    HUE_BLUE			= 0x0062,
    HUE_BLUE_LIGHT		= 0x0063,

    HUE_GRAY_DARK		= 0x0386,	// gray range.
    HUE_GRAY			= 0x0387,
    HUE_GRAY_LIGHT		= 0x0388,

    HUE_TEXT_DEF		= 0x03b2,	// NPC speech default: light gray color.
	HUE_SAY_DEF			= 0x0034,	// Player speech default: yellow

    HUE_DYE_HIGH		= 0x03e9,	// highest dyeable color = 1001

    HUE_SKIN_LOW		= 0x03EA,
    HUE_SKIN_HIGH		= 0x0422,

    // Strange mixed colors.
    HUE_HAIR_LOW		= 0x044e,	// valorite
    HUE_WHITE			= 0x0481,	// white....yup! a REAL white...
    HUE_STONE			= 0x0482,	// kinda like rock when you do it to a monster.....mabey for the Stone Harpy?
    HUE_HAIR_HIGH		= 0x04ad,

    HUE_GARGSKIN_LOW	= 0x06DB,	// lowest gargoyle skin color
    HUE_GARGSKIN_HIGH	= 0x06F3,	// highest

    HUE_MASK_LO			= 0x07FF,	// mask for items. (not really a valid thing to do i know)

    HUE_QTY				= 3000,		// 0x0bb8 Number of valid colors in hue table.
    HUE_MASK_HI			= 0x0FFF,

    HUE_TRANSLUCENT		= 0x4000,	// almost invis. may crash if not equipped ?
    HUE_UNDERWEAR		= 0x8000	// Only can be used on humans.
};


enum SOUND_CODE
{
    SOUND_NONE			= 0,
    SOUND_ANVIL			= 0x02a,

    SOUND_TURN_PAGE		= 0x055,	// open spellbook
    SOUND_SPELL_FIZZLE	= 0x05C,
    SOUND_OPEN_METAL	= 0x0ec,	// open bank box.

    SOUND_HOOF_MARBLE_1	= 0x121,	// marble echoing sound. (horse)
    SOUND_HOOF_MARBLE_2	= 0x122,
    SOUND_HOOF_BRIDGE_1 = 0x123,
    SOUND_HOOF_BRIDGE_2 = 0x124,
    SOUND_HOOF_DIRT_1	= 0x125,
    SOUND_HOOF_DIRT_2	= 0x126,
    SOUND_HOOF_QUIET_1	= 0x129,	// quiet
    SOUND_HOOF_QUIET_2	= 0x12a,	// quiet, horse run on grass.

    SOUND_FEET_STONE_1	= 0x12b,	// on stone (default)
    SOUND_FEET_STONE_2	= 0x12c,
    SOUND_FEET_GRASS_1	= 0x12d,
    SOUND_FEET_GRASS_2	= 0x12e,
    SOUND_FEET_PAVE_1	= 0x12f,	// on slate sound
    SOUND_FEET_PAVE_2	= 0x130,

    SOUND_HIT_10		= 0x013e,

    SOUND_GHOST_1		= 382,
    SOUND_GHOST_2,
    SOUND_GHOST_3,
    SOUND_GHOST_4,
    SOUND_GHOST_5,

    SOUND_SWORD_1		= 0x023b,
    SOUND_SWORD_7		= 0x023c,

    SOUND_SNIP			= 0x248,

    SOUND_QTY			= 0x300
};


enum ANIM_TYPE	// not all creatures animate the same for some reason.
{
    ANIM_WALK_UNARM		= 0x00,	// Walk (unarmed)

    // human anim. (supported by all humans)
    ANIM_WALK_ARM			= 0x01,	// Walk (armed) (but not war mode)
    ANIM_RUN_UNARM			= 0x02,	// Run (unarmed)
    ANIM_RUN_ARMED			= 0x03,	// Run (armed)
    ANIM_STAND				= 0x04,	// armed or unarmed.
    ANIM_FIDGET1			= 0x05,	// Look around
    ANIM_FIDGET_YAWN		= 0x06,	// Fidget, Yawn
    ANIM_STAND_WAR_1H		= 0x07,	// Stand for 1 hand attack.
    ANIM_STAND_WAR_2H		= 0x08,	// Stand for 2 hand attack.
    ANIM_ATTACK_WEAPON		= 0x09,	// 1H generic melee swing, any weapon.
    ANIM_ATTACK_1H_SLASH	= 0x09,
    ANIM_ATTACK_1H_PIERCE	= 0x0a,
    ANIM_ATTACK_1H_BASH		= 0x0b,
    ANIM_ATTACK_2H_BASH		= 0x0c,
    ANIM_ATTACK_2H_SLASH	= 0x0d,
    ANIM_ATTACK_2H_PIERCE	= 0x0e,
    ANIM_WALK_WAR			= 0x0f,	// Walk (warmode)
    ANIM_CAST_DIR			= 0x10,	// Directional spellcast
    ANIM_CAST_AREA			= 0x11,	// Area-effect spellcast
    ANIM_ATTACK_BOW			= 0x12,	// Bow attack / Mounted bow attack
    ANIM_ATTACK_XBOW		= 0x13,	// Crossbow attack
    ANIM_GET_HIT			= 0x14,	// Take a hit
    ANIM_DIE_BACK			= 0x15,	// (Die onto back)
    ANIM_DIE_FORWARD		= 0x16,	// (Die onto face)
    ANIM_BLOCK				= 0x1e,	// Dodge, Shield Block
    ANIM_ATTACK_WRESTLE		= 0x1f,	// Punch - attack while walking ?
    ANIM_BOW				= 0x20, // =32
    ANIM_SALUTE				= 0x21,
    ANIM_EAT				= 0x22,

    // don't use these directly these are just for translation.
    // Human on horseback
    ANIM_HORSE_RIDE_SLOW	= 0x17,
    ANIM_HORSE_RIDE_FAST	= 0x18,
    ANIM_HORSE_STAND		= 0x19,
    ANIM_HORSE_ATTACK		= 0x1a,
    ANIM_HORSE_ATTACK_BOW	= 0x1b,
    ANIM_HORSE_ATTACK_XBOW	= 0x1c,
    ANIM_HORSE_SLAP			= 0x1d,

    ANIM_QTY_MAN			= 35,	// 0x23

    ANIM_MAN_SIT			= 35,

    // monster anim	- (not all anims are supported for each creature)
    ANIM_MON_WALK 			= 0x00,
    ANIM_MON_STAND			= 0x01,
    ANIM_MON_DIE1			= 0x02,	// back
    ANIM_MON_DIE2			= 0x03, // fore or side.
    ANIM_MON_ATTACK1		= 0x04,	// ALL creatures have at least this attack.
    ANIM_MON_ATTACK2		= 0x05,	// swimming monsteers don't have this.
    ANIM_MON_ATTACK3		= 0x06,
    ANIM_MON_AttackBow		= 0x07, // air/fire elem = flail arms.
    ANIM_MON_AttackXBow		= 0x08,	// Misc Roll over,
    ANIM_MON_AttackThrow,
    ANIM_MON_GETHIT 		= 0x0a,
    ANIM_MON_PILLAGE		= 0x0b,	// 11 = Misc, Stomp, slap ground, lich conjure.
    ANIM_MON_Stomp			= 0x0c,	// Misc Cast, breath fire, elem creation.
    ANIM_MON_Cast2			= 0x0d,	// 13 = Trolls don't have this.
    ANIM_MON_Cast3,
    ANIM_MON_BlockRight		= 0x0f,
    ANIM_MON_BlockLeft		= 0x10,
    ANIM_MON_FIDGET1		= 0x11,	// 17=Idle
    ANIM_MON_FIDGET2		= 0x12,	// 18
    ANIM_MON_FLY			= 0x13,
    ANIM_MON_LAND			= 0x14,	// TakeOff
    ANIM_MON_DIE_FLIGHT		= 0x15,	// GetHitInAir

    ANIM_QTY_MON			= 22,

    // animals. (Most All animals have all anims)
    ANIM_ANI_WALK			= 0x00,
    ANIM_ANI_RUN			= 0x01,
    ANIM_ANI_STAND			= 0x02,
    ANIM_ANI_EAT			= 0x03,
    ANIM_ANI_ALERT,			// not all have this.
    ANIM_ANI_ATTACK1		= 0x05,
    ANIM_ANI_ATTACK2		= 0x06,
    ANIM_ANI_GETHIT 		= 0x07,
    ANIM_ANI_DIE1 			= 0x08,
    ANIM_ANI_FIDGET1		= 0x09,	// Idle
    ANIM_ANI_FIDGET2		= 0x0a,
    ANIM_ANI_SLEEP			= 0x0b,	// lie down (not all have this)
    ANIM_ANI_DIE2			= 0x0c,

    ANIM_QTY_ANI	= 13,

    ANIM_QTY		= 0x32
};


enum ANIM_TYPE_NEW	// not all creatures animate the same for some reason. http://img546.imageshack.us/img546/5439/uonewanimstable2.png
{
    NANIM_ATTACK		= 0x00,		// 8 SUB ANIMATIONS, VARIATION 0-*
    NANIM_BLOCK			= 0x01,		// VARIATION 0-1
    NANIM_BLOCK2		= 0x02,		// MONSTERS, VARIATION 0-1
    NANIM_DEATH			= 0x03,		// VARIATION 0-1
    NANIM_GETHIT		= 0x04,		// VARIATION 0-*
    NANIM_IDLE			= 0x05,
    NANIM_EAT			= 0x06,
    NANIM_EMOTE			= 0x07,		// 2 SUB ANIMATIONS
    NANIM_ANGER			= 0x08,
    NANIM_TAKEOFF		= 0x09,
    NANIM_LANDING		= 0x0a,
    NANIM_SPELL			= 0x0b,		// 2 SUB ANIMATIONS
    NANIM_UNKNOWN1		= 0x0c,		// According to RUOSI now this is StartCombat
    NANIM_UNKNOWN2		= 0x0d,		// and this one EndCombat (Maybe only for EC?)
    NANIM_PILLAGE		= 0x0e,		// Human/Animal (eat), Monster (pillage)
    NANIM_RISE			= 0x0f,		// Used on character creation (Only EC)

    NANIM_QTY = 16,

    NANIM_ATTACK_WRESTLING	= 0x00,
    NANIM_ATTACK_BOW		= 0x01,
    NANIM_ATTACK_CROSSBOW	= 0x02,
    NANIM_ATTACK_1H_BASH	= 0x03,
    NANIM_ATTACK_1H_SLASH	= 0x04,
    NANIM_ATTACK_1H_PIERCE	= 0x05,
    NANIM_ATTACK_2H_BASH	= 0x06,
    NANIM_ATTACK_2H_SLASH	= 0x07,
    NANIM_ATTACK_2H_PIERCE	= 0x08,
    NANIM_ATTACK_THROWING	= 0x09,

    NANIM_ATTACK_QTY = 10,

    NANIM_EMOTE_BOW		= 0x00,
    NANIM_EMOTE_SALUTE	= 0x01,

    NANIM_EMOTE_QTY = 2,

    NANIM_SPELL_NORMAL	= 0x00,
    NANIM_SPELL_SUMMON	= 0x01,

    NANIM_SPELL_QTY = 2,

};


enum CRESND_TYPE	// Creature sound offset types.
{	// Some creatures have no random sounds. others (humans,birds) have many sounds.
    CRESND_RAND1 = 0,	// just random noise. or "yes"
    CRESND_RAND2,		// "no" response
    CRESND_HIT,
    CRESND_GETHIT,
    CRESND_DIE
};


enum FONT_TYPE
{
    FONT_BOLD,		// 0 - Bold Text = Large plain filled block letters.
    FONT_SHAD,		// 1 - Text with shadow = small gray
    FONT_BOLD_SHAD,	// 2 - Bold+Shadow = Large Gray block letters.
    FONT_NORMAL,	// 3 - Normal (default) = Filled block letters.
    FONT_GOTH,		// 4 - Gothic = Very large blue letters.
    FONT_ITAL,		// 5 - Italic Script
    FONT_SM_DARK,	// 6 - Small Dark Letters = small Blue
    FONT_COLOR,		// 7 - Colorful Font (Buggy?) = small Gray (hazy)
    FONT_RUNE,		// 8 - Rune font (Only use capital letters with this!)
    FONT_SM_LITE,	// 9 - Small Light Letters = small roman gray font.
    FONT_QTY
};


enum AFFIX_TYPE
{
    AFFIX_APPEND  = 0x0,	// 0 - Append affix to end of message
    AFFIX_PREPEND = 0x1,	// 1 - Prepend affix to front of message
    AFFIX_SYSTEM  = 0x2		// 2 - Message is displayed as a system message
};


enum DIR_TYPE	// Walking directions. m_dir
{
    DIR_INVALID = -1,

    DIR_N = 0,
    DIR_NE,
    DIR_E,
    DIR_SE,
    DIR_S,
    DIR_SW,
    DIR_W,
    DIR_NW,
    DIR_QTY,		// Also means "Center"

    DIR_ANIM_QTY = 5	// Seems we only need 5 pics for an anim, assume ALL bi-symetrical creatures
};


enum SKILL_TYPE	// List of skill numbers (things that can be done at a given time)
{
    SKILL_NONE = -1,

    SKILL_ALCHEMY,
    SKILL_ANATOMY,
    SKILL_ANIMALLORE,
    SKILL_ITEMID,
    SKILL_ARMSLORE,
    SKILL_PARRYING,
    SKILL_BEGGING,
    SKILL_BLACKSMITHING,
    SKILL_BOWCRAFT,
    SKILL_PEACEMAKING,
    SKILL_CAMPING,
    SKILL_CARPENTRY,
    SKILL_CARTOGRAPHY,
    SKILL_COOKING,
    SKILL_DETECTINGHIDDEN,
    SKILL_ENTICEMENT,
    SKILL_EVALINT,
    SKILL_HEALING,
    SKILL_FISHING,
    SKILL_FORENSICS,
    SKILL_HERDING,
    SKILL_HIDING,
    SKILL_PROVOCATION,
    SKILL_INSCRIPTION,
    SKILL_LOCKPICKING,
    SKILL_MAGERY,
    SKILL_MAGICRESISTANCE,
    SKILL_TACTICS,
    SKILL_SNOOPING,
    SKILL_MUSICIANSHIP,
    SKILL_POISONING,
    SKILL_ARCHERY,
    SKILL_SPIRITSPEAK,
    SKILL_STEALING,
    SKILL_TAILORING,
    SKILL_TAMING,
    SKILL_TASTEID,
    SKILL_TINKERING,
    SKILL_TRACKING,
    SKILL_VETERINARY,
    SKILL_SWORDSMANSHIP,
    SKILL_MACEFIGHTING,
    SKILL_FENCING,
    SKILL_WRESTLING,
    SKILL_LUMBERJACKING,
    SKILL_MINING,
    SKILL_MEDITATION,
    SKILL_STEALTH,
    SKILL_REMOVETRAP,
    //AOS
    SKILL_NECROMANCY,
    SKILL_FOCUS,
    SKILL_CHIVALRY,
    //SE
    SKILL_BUSHIDO,
    SKILL_NINJITSU,
    //ML
    SKILL_SPELLWEAVING,
    //SA
    SKILL_MYSTICISM,
    SKILL_IMBUING,
    SKILL_THROWING,

    /**
     * Skill level limit. Should not used directly, most cases are covered by g_Cfg.m_iMaxSkill instead
    */
    SKILL_QTY = 99,

    // Actions a npc will perform. (no need to track skill level for these)
    NPCACT_FOLLOW_TARG = 100,	// 100 = following a char.
    NPCACT_STAY,				// 101
    NPCACT_GOTO,				// 102 = Go to a location x,y. Pet command
    NPCACT_WANDER,				// 103 = Wander aimlessly.
    NPCACT_LOOKING,				// 104 = just look around intently.
    NPCACT_FLEE,				// 105 = Run away from target. m_Act_Targ
    NPCACT_TALK,				// 106 = Talking to my target. m_Act_Targ
    NPCACT_TALK_FOLLOW,			// 107 = m_Act_Targ / m_Fight_Targ.
    NPCACT_GUARD_TARG,			// 108 = Guard a targetted object. m_Act_Targ
    NPCACT_GO_HOME,				// 109 =
    NPCACT_BREATH,				// 110 = Using breath weapon. on m_Fight_Targ.
    NPCACT_RIDDEN,				// 111 = Being ridden or shrunk as figurine.
    NPCACT_THROWING,			// 112 = Throwing a stone at m_Fight_Targ.
    NPCACT_TRAINING,			// 113 = using a training dummy etc.
    NPCACT_NAPPING,				// 114 = just snoozong a little bit, but not sleeping.
    NPCACT_FOOD,				// 115 = Searching for food
    NPCACT_RUNTO,				// 116 = Run to a location x,y.
    NPCACT_QTY
};


enum LAYER_TYPE		// defined by UO. Only one item can be in a slot.
{
    LAYER_NONE = 0,	// spells that are layed on the CChar ?
    LAYER_HAND1,	// 1 = spellbook or weapon.
    LAYER_HAND2,	// 2 = other hand or 2 handed thing. = shield
    LAYER_SHOES,	// 3
    LAYER_PANTS,	// 4 = bone legs + pants.
    LAYER_SHIRT,
    LAYER_HELM,		// 6
    LAYER_GLOVES,	// 7
    LAYER_RING,
    LAYER_TALISMAN,	// 9 = talisman item (it was _LIGHT)
    LAYER_COLLAR,	// 10 = gorget or necklace.
    LAYER_HAIR,		// 11 = 0x0b =
    LAYER_HALF_APRON,// 12 = 0x0c =
    LAYER_CHEST,	// 13 = 0x0d = armor chest
    LAYER_WRIST,	// 14 = 0x0e = watch
    LAYER_NEWLIGHT,	// 15 = Unused (use it for: a ITEMID_LIGHT_SRC equip item can be put here.)
    LAYER_BEARD,	// 16 = try to have only men have this.
    LAYER_TUNIC,	// 17 = jester suit or full apron.
    LAYER_EARS,		// 18 = earrings
    LAYER_ARMS,		// 19 = armor
    LAYER_CAPE,		// 20 = cape
    LAYER_PACK,		// 21 = 0x15 = only used by ITEMID_BACKPACK
    LAYER_ROBE,		// 22 = robe over all.
    LAYER_SKIRT,	// 23 = skirt or kilt.
    LAYER_LEGS,		// 24= 0x18 = plate legs.

    // These are not part of the paper doll (but get sent to the client)
    LAYER_HORSE,		// 25 = 0x19 = ride this object. (horse objects are strange?)
    LAYER_VENDOR_STOCK,	// 26 = 0x1a = the stuff the vendor will restock and sell to the players
    LAYER_VENDOR_EXTRA,	// 27 = 0x1b = the stuff the vendor will resell to players but is not restocked. (bought from players)
    LAYER_VENDOR_BUYS,	// 28 = 0x1c = the stuff the vendor can buy from players but does not stock.
    LAYER_BANKBOX,		// 29 = 0x1d = contents of my bank box.

    // Internally used layers - Don't bother sending these to client.
    LAYER_SPECIAL,		// 30 =	Can be multiple of these. memories
    LAYER_DRAGGING,

    // Spells that are effecting us go here.
    LAYER_SPELL_STATS,			// 32 = Stats effecting spell. These cancel each other out.
    LAYER_SPELL_Reactive,		// 33 =
    LAYER_SPELL_Night_Sight,
    LAYER_SPELL_Protection,		// 35
    LAYER_SPELL_Incognito,
    LAYER_SPELL_Magic_Reflect,
    LAYER_SPELL_Paralyze,		// or turned to stone.
    LAYER_SPELL_Invis,
    LAYER_SPELL_Polymorph,		// 40
    LAYER_SPELL_Summon,			// 41 = magical summoned creature.

    LAYER_FLAG_Poison,			// 42
    LAYER_FLAG_Criminal,		// criminal or murderer ? decay over time
    LAYER_FLAG_Potion,			// Some magic type effect done by a potion. (they cannot be dispelled)
    LAYER_FLAG_SpiritSpeak,		// 45
    LAYER_FLAG_Wool,			// regrowing wool.
    LAYER_FLAG_Drunk,			// Booze effect.
    LAYER_FLAG_ClientLinger,	// 48
    LAYER_FLAG_Hallucination,	// shrooms etc.
    LAYER_FLAG_PotionUsed,		// 50 = track the time till we can use a potion again.
    LAYER_FLAG_Stuck,			// In a trap or web.
    LAYER_FLAG_Murders,			// How many murders do we have ? decay over time
    LAYER_FLAG_Bandage,			// 53 = Bandages go here for healing

    LAYER_AUCTION,				// Auction layer

    //Necro
    LAYER_SPELL_Blood_Oath,
    LAYER_SPELL_Curse_Weapon,
    LAYER_SPELL_Corpse_Skin,
    LAYER_SPELL_Evil_Omen,
    LAYER_SPELL_Pain_Spike,
    LAYER_SPELL_Mind_Rot,
    LAYER_SPELL_Strangle,

    //Ninjitsu
    //LAYER_SPELL_Surprise_Attack,

    //Chivalry
    LAYER_SPELL_Consecrate_Weapon,
    LAYER_SPELL_Divine_Fury,
    LAYER_SPELL_Enemy_Of_One,

    //SpellWeaving
    LAYER_SPELL_Attunement,
    LAYER_SPELL_Gift_Of_Renewal,
    LAYER_SPELL_Immolating_Weapon,
    LAYER_SPELL_Thunderstorm,
    LAYER_SPELL_Arcane_Empowerment,
    LAYER_SPELL_Ethereal_Voyage,
    LAYER_SPELL_Gift_Of_Life,
    LAYER_SPELL_Dryad_Allure,
    LAYER_SPELL_Essence_Of_Wind,

    //Mysticism
    LAYER_SPELL_Sleep,
    LAYER_SPELL_Bombard,
    LAYER_SPELL_Spell_Plague,
    LAYER_SPELL_Nether_Cyclone,

    LAYER_QTY
};


enum SPELL_TYPE	// List of spell numbers in spell book.
{
    SPELL_NONE = 0,

    // Magery
    SPELL_Clumsy = 1,		// 1st circle
    SPELL_Create_Food,
    SPELL_Feeblemind,
    SPELL_Heal,
    SPELL_Magic_Arrow,
    SPELL_Night_Sight,
    SPELL_Reactive_Armor,
    SPELL_Weaken,
    SPELL_Agility,			// 2nd circle
    SPELL_Cunning,
    SPELL_Cure,
    SPELL_Harm,
    SPELL_Magic_Trap,
    SPELL_Magic_Untrap,
    SPELL_Protection,
    SPELL_Strength,
    SPELL_Bless,			// 3rd circle
    SPELL_Fireball,
    SPELL_Magic_Lock,
    SPELL_Poison,
    SPELL_Telekin,
    SPELL_Teleport,
    SPELL_Unlock,
    SPELL_Wall_of_Stone,
    SPELL_Arch_Cure,		// 4th circle
    SPELL_Arch_Prot,
    SPELL_Curse,
    SPELL_Fire_Field,
    SPELL_Great_Heal,
    SPELL_Lightning,
    SPELL_Mana_Drain,
    SPELL_Recall,
    SPELL_Blade_Spirit,		// 5th circle
    SPELL_Dispel_Field,
    SPELL_Incognito,
    SPELL_Magic_Reflect,
    SPELL_Mind_Blast,
    SPELL_Paralyze,
    SPELL_Poison_Field,
    SPELL_Summon,
    SPELL_Dispel,			// 6th circle
    SPELL_Energy_Bolt,
    SPELL_Explosion,
    SPELL_Invis,
    SPELL_Mark,
    SPELL_Mass_Curse,
    SPELL_Paralyze_Field,
    SPELL_Reveal,
    SPELL_Chain_Lightning,	// 7th circle
    SPELL_Energy_Field,
    SPELL_Flame_Strike,
    SPELL_Gate_Travel,
    SPELL_Mana_Vamp,
    SPELL_Mass_Dispel,
    SPELL_Meteor_Swarm,
    SPELL_Polymorph,
    SPELL_Earthquake,		// 8th circle
    SPELL_Vortex,
    SPELL_Resurrection,
    SPELL_Air_Elem,
    SPELL_Daemon,
    SPELL_Earth_Elem,
    SPELL_Fire_Elem,
    SPELL_Water_Elem,
    SPELL_MAGERY_QTY = SPELL_Water_Elem,

    // Necromancy (AOS)
    SPELL_Animate_Dead_AOS = 101,
    SPELL_Blood_Oath,
    SPELL_Corpse_Skin,
    SPELL_Curse_Weapon,
    SPELL_Evil_Omen,
    SPELL_Horrific_Beast,
    SPELL_Lich_Form,
    SPELL_Mind_Rot,
    SPELL_Pain_Spike,
    SPELL_Poison_Strike,
    SPELL_Strangle,
    SPELL_Summon_Familiar,
    SPELL_Vampiric_Embrace,
    SPELL_Vengeful_Spirit,
    SPELL_Wither,
    SPELL_Wraith_Form,
    SPELL_Exorcism,
    SPELL_NECROMANCY_QTY = SPELL_Exorcism,

    // Chivalry (AOS)
    SPELL_Cleanse_by_Fire = 201,
    SPELL_Close_Wounds,
    SPELL_Consecrate_Weapon,
    SPELL_Dispel_Evil,
    SPELL_Divine_Fury,
    SPELL_Enemy_of_One,
    SPELL_Holy_Light,
    SPELL_Noble_Sacrifice,
    SPELL_Remove_Curse,
    SPELL_Sacred_Journey,
    SPELL_CHIVALRY_QTY = SPELL_Sacred_Journey,

    // Bushido (SE)
    SPELL_Honorable_Execution = 401,
    SPELL_Confidence,
    SPELL_Evasion,
    SPELL_Counter_Attack,
    SPELL_Lightning_Strike,
    SPELL_Momentum_Strike,
    SPELL_BUSHIDO_QTY = SPELL_Momentum_Strike,

    // Ninjitsu (SE)
    SPELL_Focus_Attack = 501,
    SPELL_Death_Strike,
    SPELL_Animal_Form,
    SPELL_Ki_Attack,
    SPELL_Surprise_Attack,
    SPELL_Backstab,
    SPELL_Shadowjump,
    SPELL_Mirror_Image,
    SPELL_NINJITSU_QTY = SPELL_Mirror_Image,

    // Spellweaving (ML)
    SPELL_Arcane_Circle = 601,
    SPELL_Gift_of_Renewal,
    SPELL_Immolating_Weapon,
    SPELL_Attunement,
    SPELL_Thunderstorm,
    SPELL_Natures_Fury,
    SPELL_Summon_Fey,
    SPELL_Summon_Fiend,
    SPELL_Reaper_Form,
    SPELL_Wildfire,
    SPELL_Essence_of_Wind,
    SPELL_Dryad_Allure,
    SPELL_Ethereal_Voyage,
    SPELL_Word_of_Death,
    SPELL_Gift_of_Life,
    SPELL_Arcane_Empowerment,
    SPELL_SPELLWEAVING_QTY = SPELL_Arcane_Empowerment,

    // Mysticism (SA)
    SPELL_Nether_Bolt = 678,
    SPELL_Healing_Stone,
    SPELL_Enchant_Weapon,
    SPELL_Sleep,
    SPELL_Eagle_Strike,
    SPELL_Animated_Weapon,
    SPELL_Stone_Form,
    SPELL_Spell_Trigger,
    SPELL_Mass_Sleep,
    SPELL_Cleansing_Winds,
    SPELL_Bombard,
    SPELL_Spell_Plague,
    SPELL_Hail_Storm,
    SPELL_Nether_Cyclone,
    SPELL_Rising_Collossus,
    SPELL_MYSTICISM_QTY = SPELL_Rising_Collossus,

    // Bard (SA)
    SPELL_Inspire = 701,
    SPELL_Invigorate,
    SPELL_Resilience,
    SPELL_Perseverance,
    SPELL_Tribulation,
    SPELL_Despair,
    SPELL_BARD_QTY = SPELL_Despair,

    // Custom Sphere spells (used by some monsters)
    SPELL_Summon_Undead = 1000,
    SPELL_Animate_Dead,
    SPELL_Bone_Armor,
    SPELL_Light,
    SPELL_Fire_Bolt,
    SPELL_Hallucination,
    SPELL_CUSTOM_QTY = SPELL_Hallucination,

    // Custom extra special spells (can be used as potion effects as well). Commented value = old index.
    SPELL_Stone,			// 71 = Turn to stone (permanent).
    SPELL_Shrink,			// 72 = turn pet into icon.
    SPELL_Refresh,			// 73 = stamina
    SPELL_Restore,			// 74 = This potion increases both your hit points and your stamina.
    SPELL_Mana,				// 75 = restone mana
    SPELL_Sustenance,		// 76 = serves to fill you up. (Remember, healing rate depends on how well fed you are!)
    SPELL_Chameleon,		// 77 = makes your skin match the colors of whatever is behind you.
    SPELL_BeastForm,		// 78 = polymorphs you into an animal for a while.
    SPELL_Monster_Form,		// 79 = polymorphs you into a monster for a while.
    SPELL_Gender_Swap,		// 81 = permanently changes your gender.
    SPELL_Trance,			// 82 = temporarily increases your meditation skill.
    SPELL_Particle_Form,	// 83 = turns you into an immobile, but untargetable particle system for a while.
    SPELL_Shield,			// 84 = erects a temporary force field around you. Nobody approaching will be able to get within 1 tile of you, though you can move close to them if you wish.
    SPELL_Steelskin,		// 85 = turns your skin into steel, giving a boost to your AR.
    SPELL_Stoneskin,		// 86 = turns your skin into stone, giving a boost to your AR.
    SPELL_Regenerate,		// 87 = regen hitpoints at a fast rate.
    SPELL_Enchant,			// 88 = Enchant an item (weapon or armor)
    SPELL_Forget,			// 89 = only existed in sphere_spells.scp before
    SPELL_Ale,				// 90 = drunkeness ?
    SPELL_Wine,				// 91 = mild drunkeness ?
    SPELL_Liquor,			// 92 = extreme drunkeness ?
    SPELL_QTY = SPELL_Liquor
};


enum LIGHT_PATTERN	// What pattern (m_light_pattern) does the light source (CAN_LIGHT) take.
{
    LIGHT_LARGE = 1,
    // ... etc
    // Colored light is in here some place as well.
    LIGHT_QTY = 56	// This makes it go black.
};


enum GUMP_TYPE	// The gumps. (most of these are not useful to the server.)
{
    GUMP_NONE				= 0,
    GUMP_RESERVED			= 1,

    GUMP_CORPSE				= 0x09,
    GUMP_VENDOR_RECT		= 0x30,	// Big blue rectangle for vendor mask.
    GUMP_PACK				= 0x3c,	// Open backpack
    GUMP_BAG				= 0x3D,	// Leather Bag
    GUMP_BARREL				= 0x3E,	// Barrel
    GUMP_BASKET_SQ			= 0x3F,	// Square picknick Basket
    GUMP_BOX_WOOD			= 0x40,	// Small wood box with a lock
    GUMP_BASKET_RO			= 0x41,	// Round Basket
    GUMP_CHEST_GO_SI		= 0x42,	// Gold and Silver Chest.
    GUMP_BOX_WOOD_OR		= 0x43,	// Small wood box (ornate)(no lock)
    GUMP_CRATE				= 0x44,	// Wood Crate
    GUMP_DRAWER_DK			= 0x48,
    GUMP_CHEST_WO_GO		= 0x49,	// Wood with gold trim
    GUMP_CHEST_SI			= 0x4a,	// Silver chest
    GUMP_BOX_GO_LO			= 0x4b,	// Gold/Brass box with a lock
    GUMP_SHIP_HOLD			= 0x4c,
    GUMP_BOOK_SHELF			= 0x4d,
    GUMP_CABINET_DK			= 0x4e,
    GUMP_CABINET_LT			= 0x4f,
    GUMP_DRAWER_LT			= 0x51,
    GUMP_GIFT_BOX			= 0x102,
    GUMP_ARMOIRE_RED		= 0x105,
    GUMP_ARMOIRE_MAPLE		= 0x106,
    GUMP_ARMOIRE_CHERRY		= 0x107,
    GUMP_BASKET_TALL		= 0x108,
    GUMP_CHEST_WOOD_PLAIN	= 0x109,
    GUMP_CHEST_WOOD_GILDED	= 0x10A,
    GUMP_CHEST_WOOD_ORNATE	= 0x10B,
    GUMP_TALL_CABINET		= 0x10C,
    GUMP_CHEST_WOOD_FINISH	= 0x10D,
    GUMP_GIFT_BOX_CUBE		= 0x11B,
    GUMP_GIFT_BOX_CILINDER	= 0x11C,
    GUMP_GIFT_BOX_OCTOGON	= 0x11D,
    GUMP_GIFT_BOX_RECTANGLE	= 0x11E,
    GUMP_GIFT_BOX_ANGEL		= 0x11F,
    GUMP_HEART_SHAPED		= 0x120,

    GUMP_SECURE_TRADE		= 0x866,
    GUMP_VENDOR_1			= 0x870,
    GUMP_GAME_BOARD			= 0x91a,	// Chess or checker board.
    GUMP_GAME_BACKGAM		= 0x92e,	// backgammon board.
    GUMP_MAP_2_NORTH		= 0x139d,
    GUMP_QTY				= 0xfffe,
    GUMP_OPEN_SPELLBOOK		= 0xFFFF
};

enum
{
    // Terrain samples
    TERRAIN_HOLE	= 0x0002,	// "NODRAW" we can pas thru this.

    TERRAIN_WATER1	= 0x00a8,
    TERRAIN_WATER2	= 0x00a9,
    TERRAIN_WATER3	= 0x00aa,
    TERRAIN_WATER4	= 0x00ab,
    TERRAIN_WATER5	= 0x0136,
    TERRAIN_WATER6	= 0x0137,

    TERRAIN_NULL	= 0x0244,	// impassible interdungeon

    TERRAIN_QTY     = 0x4000	// Terrain tile qyt
};


/////////////////////////////////////////////////////////////////
// File blocks

enum VERFILE_TYPE		// skew list. (verdata.mul)
{
    VERFILE_ARTIDX		= 0x00,	// "artidx.mul" = Index to ART
    VERFILE_ART			= 0x01, // "art.mul" = Artwork such as ground, objects, etc.
    VERFILE_ANIMIDX		= 0x02,	// "anim.idx" = 2454ah animations.
    VERFILE_ANIM		= 0x03,	// "anim.mul" = Animations such as monsters, people, and armor.
    VERFILE_SOUNDIDX	= 0x04, // "soundidx.mul" = Index into SOUND
    VERFILE_SOUND		= 0x05, // "sound.mul" = Sampled sounds
    VERFILE_TEXIDX		= 0x06, // "texidx.mul" = Index into TEXMAPS
    VERFILE_TEXMAPS		= 0x07,	// "texmaps.mul" = Texture map data (the ground).
    VERFILE_GUMPIDX		= 0x08, // "gumpidx.mul" = Index to GUMPART
    VERFILE_GUMPART		= 0x09, // "gumpart.mul" = Gumps. Stationary controller bitmaps such as windows, buttons, paperdoll pieces, etc.
    VERFILE_MULTIIDX	= 0x0A,	// "multi.idx" = Index to MULTI
    VERFILE_MULTI		= 0x0B,	// "multi.mul" = Groups of art (houses, castles, etc)
    VERFILE_SKILLSIDX	= 0x0C, // "skills.idx" =
    VERFILE_SKILLS		= 0x0D, // "skills.mul" =
    VERFILE_VERDATA		= 0x0E, // ? "verdata.mul" = This version file.
    //	maps
    VERFILE_MAP			= 0x0F, // MAP*.mul(s)
    VERFILE_STAIDX		= 0x13, // STAIDX*.mul(s)
    VERFILE_STATICS		= 0x17,	// STATICS*.mul(s)
    // empty			= 0x10, // "map2.mul"
    // empty			= 0x11, // "map3.mul"
    // empty			= 0x12, // "map4.mul"
    // empty			= 0x14, // "staidx2.mul"
    // empty			= 0x15, // "staidx3.mul"
    // empty			= 0x16, // "staidx4.mul"
    // empty			= 0x18, // "statics2.mul"
    // empty			= 0x19, // "statics3.mul"
    // empty			= 0x1A, // "statics4.mul"
    // empty			= 0x1B space for new map
    // empty			= 0x1C space for new map
    // empty			= 0x1D space for new map

    VERFILE_TILEDATA	= 0x1E, // "tiledata.mul" = Data about tiles in ART. name and flags, etc
    VERFILE_ANIMDATA	= 0x1F, // "animdata.mul" = ? no idea, might be item animation ?.
    VERFILE_HUES		= 0x20, // ? "hues.mul"
    VERFILE_QTY					// NOTE: 021 is used for something ?!
};


enum VERFILE_FORMAT	// mul formats
{
    VERFORMAT_ORIGINAL = 0x01,	// original mul format
    VERFORMAT_HIGHSEAS = 0x02,	// high seas mul format
    VERFORMAT_QTY
};


#endif //_INC_UOFILES_ENUMS_H
