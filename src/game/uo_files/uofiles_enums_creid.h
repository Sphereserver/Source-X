/**
* @file uofiles_enums_creid.h
*
*/

#pragma once
#ifndef _INC_UOFILES_ENUMS_CREID_H
#define _INC_UOFILES_ENUMS_CREID_H

enum CREID_TYPE	// enum the creature art work. (dont allow any others !) also know as "model number"
{
	CREID_INVALID = 0,

	CREID_OGRE = 0x0001,
	CREID_ETTIN = 0x0002,
	CREID_ZOMBIE = 0x0003,
	CREID_GARGOYLE = 0x0004,
	CREID_EAGLE = 0x0005,
	CREID_BIRD = 0x0006,
	CREID_ORC_LORD = 0x0007,
	CREID_CORPSER = 0x0008,
	CREID_DAEMON = 0x0009,
	CREID_DAEMON_SWORD = 0x000A,

	CREID_DRAGON_GREY = 0x000c,
	CREID_AIR_ELEM = 0x000d,
	CREID_EARTH_ELEM = 0x000e,
	CREID_FIRE_ELEM = 0x000f,
	CREID_WATER_ELEM = 0x0010,
	CREID_ORC = 0x0011,
	CREID_ETTIN_AXE = 0x0012,

	CREID_GIANT_SNAKE = 0x0015,
	CREID_GAZER = 0x0016,

	CREID_LICH = 0x0018,

	CREID_SPECTRE = 0x001a,

	CREID_GIANT_SPIDER = 0x001c,
	CREID_GORILLA = 0x001d,
	CREID_HARPY = 0x001e,
	CREID_HEADLESS = 0x001f,

	CREID_LIZMAN = 0x0021,
	CREID_LIZMAN_SPEAR = 0x0023,
	CREID_LIZMAN_MACE = 0x0024,

	CREID_MONGBAT = 0x0027,

	CREID_ORC_CLUB = 0x0029,
	CREID_RATMAN = 0x002a,

	CREID_RATMAN_CLUB = 0x002c,
	CREID_RATMAN_SWORD = 0x002d,

	CREID_REAPER = 0x002f,	// tree
	CREID_SCORP = 0x0030,	// giant scorp.

	CREID_SKELETON = 0x0032,
	CREID_SLIME = 0x0033,
	CREID_Snake = 0x0034,
	CREID_TROLL_SWORD = 0x0035,
	CREID_TROLL = 0x0036,
	CREID_TROLL_MACE = 0x0037,
	CREID_SKEL_AXE = 0x0038,
	CREID_SKEL_SW_SH = 0x0039,	// sword and sheild
	CREID_WISP = 0x003a,
	CREID_DRAGON_RED = 0x003b,
	CREID_DRAKE_GREY = 0x003c,
	CREID_DRAKE_RED = 0x003d,

	CREID_Tera_Warrior = 0x0046,	// T2A 0x46 = Terathen Warrior
	CREID_Tera_Drone = 0x0047,		// T2A 0x47 = Terathen Drone
	CREID_Tera_Matriarch = 0x0048,	// T2A 0x48 = Terathen Matriarch

	CREID_Titan = 0x004b,		// T2A 0x4b = Titan
	CREID_Cyclops = 0x004c,		// T2A 0x4c = Cyclops
	CREID_Giant_Toad = 0x0050,	// T2A 0x50 = Giant Toad
	CREID_Bull_Frog = 0x0051,	// T2A 0x51 = Bull Frog

	CREID_Ophid_Mage = 0x0055,		// T2A 0x55 = Ophidian Mage
	CREID_Ophid_Warrior = 0x0056,	// T2A 0x56 = Ophidian Warrior
	CREID_Ophid_Queen = 0x0057,		// T2A 0x57 = Ophidian Queen
	CREID_SEA_Creature = 0x005f,	// T2A 0x5f = (Unknown Sea Creature)

	CREID_SERPENTINE_DRAGON = 0x0067,	// LBR
	CREID_SKELETAL_DRAGON = 0x0068,		// LBR

	CREID_SEA_SERP = 0x0096,
	CREID_Dolphin = 0x0097,

	CREID_SHADE = 0x0099,	// LBR
	CREID_MUMMY = 0x009a,	// LBR


	// ---- Animals (Low detail critters) ----

	CREID_HORSE1 = 0x00C8,		// white = 200 decinal
	CREID_Cat = 0x00c9,
	CREID_Alligator = 0x00CA,
	CREID_Pig = 0x00CB,
	CREID_HORSE4 = 0x00CC,		// brown
	CREID_Rabbit = 0x00CD,
	CREID_LavaLizard = 0x00ce,	// T2A = Lava Lizard
	CREID_Sheep = 0x00CF,		// un-sheered.
	CREID_Chicken = 0x00D0,
	CREID_Goat = 0x00d1,
	CREID_Ostard_Desert = 0x00d2,	// T2A = Desert Ostard (ridable)
	CREID_BrownBear = 0x00D3,
	CREID_GrizzlyBear = 0x00D4,
	CREID_PolarBear = 0x00D5,
	CREID_Panther = 0x00d6,
	CREID_GiantRat = 0x00D7,
	CREID_Cow_BW = 0x00d8,
	CREID_Dog = 0x00D9,
	CREID_Ostard_Frenz = 0x00da,	// T2A = Frenzied Ostard (ridable)
	CREID_Ostard_Forest = 0x00db,	// T2A = Forest Ostard (ridable)
	CREID_Llama = 0x00dc,
	CREID_Walrus = 0x00dd,
	CREID_Sheep_Sheered = 0x00df,
	CREID_Wolf = 0x00e1,
	CREID_HORSE2 = 0x00E2,
	CREID_HORSE3 = 0x00E4,
	CREID_Cow2 = 0x00e7,
	CREID_Bull_Brown = 0x00e8,	// light brown
	CREID_Bull2 = 0x00e9,		// dark brown
	CREID_Hart = 0x00EA,		// Male deer.
	CREID_Deer = 0x00ED,
	CREID_Rat = 0x00ee,

	CREID_Boar = 0x0122,		// large pig
	CREID_HORSE_PACK = 0x0123,	// Pack horse with saddle bags
	CREID_LLAMA_PACK = 0x0124,	// Pack llama with saddle bags

	// all below here are humanish or clothing.
	CREID_MAN = 0x0190,		// 400 decimal
	CREID_WOMAN = 0x0191,
	CREID_GHOSTMAN = 0x0192,	// Ghost robe is not automatic !
	CREID_GHOSTWOMAN = 0x0193,
	CREID_EQUIP,

	CREID_VORTEX = 0x023d,	// T2A = vortex
	CREID_BLADES = 0x023e,	// blade spirits (in human range? not sure why)

	CREID_ELFMAN = 0x025D,	// 605 decimal
	CREID_ELFWOMAN = 0x025E,
	CREID_ELFGHOSTMAN = 0x025F,
	CREID_ELFGHOSTWOMAN = 0x0260,

	CREID_GARGMAN = 0x029A,	// 666 decimal
	CREID_GARGWOMAN = 0x029B,
	CREID_GARGGHOSTMAN = 0x02B6,	// 694
	CREID_GARGGHOSTWOMAN = 0x02B7,

	//	new monsters lies between this range
	CREID_IRON_GOLEM = 0x02f0,		// LBR
	//..
	CREID_GIANT_BEETLE = 0x0317,
	CREID_SWAMP_DRAGON1 = 0x031a,	// LBR
	CREID_REPTILE_LORD = 0x031d,	// LBR
	CREID_ANCIENT_WYRM = 0x031e,	// LBR
	CREID_SWAMP_DRAGON2 = 0x031f,	// LBR
	//..
	CREID_EQUIP_GM_ROBE = 0x03db,
	//..
	CREID_MULTICOLORED_HORDE_DAEMON = 0x03e7,	// LBR

	CREID_Revenant = 0x2ee,
	CREID_Horrific_Beast = 0x2ea,
	CREID_Stone_Form = 0x2c1,
	CREID_Vampire_Bat = 0x13d,
	CREID_Reaper_Form = 0xe6,

	CREID_QTY = 0x0800,	// Max number of base character types, based on art work.

	// re-use artwork to make other types on NPC's
	NPCID_SCRIPT = 0x801,

	NPCID_SCRIPT2 = 0x4000,	// Safe area for server specific NPC defintions.
	NPCID_Qty = 0x8000,	// Spawn types start here.

	SPAWNTYPE_START = 0x8001
};


#endif //_INC_UOFILES_ENUMS_CREID_H
