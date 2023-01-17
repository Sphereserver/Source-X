/**
* @file item_types.h
*
*/

#ifndef _INC_ITEMTYPES_H
#define _INC_ITEMTYPES_H


enum IT_TYPE		// double click type action.
{
	// NOTE: Never change this list as it will mess up the RES_ITEMDEF or RES_WORLDITEM already stored.
	// Just add stuff to end.
    IT_INVALID = -1,

	IT_NORMAL = 0,
	IT_CONTAINER,		    // any unlocked container or corpse. CContainer based
	IT_CONTAINER_LOCKED,	// 2 =
	IT_DOOR,				// 3 = door can be opened
	IT_DOOR_LOCKED,			// 4 = a locked door.
	IT_KEY,					// 5 =
	IT_LIGHT_LIT,			// 6 = Local Light giving object. (needs no fuel, never times out)
	IT_LIGHT_OUT,			// 7 = Can be lit.
	IT_FOOD,				// 8 = edible food. (poisoned food ?)
	IT_FOOD_RAW,			// 9 = Must cook raw meat unless your an animal.
	IT_ARMOR,				// 10 = some type of armor. (no real action)
	IT_WEAPON_MACE_SMITH,	// 11 = Can be used for smithing
	IT_WEAPON_MACE_SHARP,	// 12 = war axe can be used to cut/chop trees.
	IT_WEAPON_SWORD,		// 13 =
	IT_WEAPON_FENCE,		// 14 = can't be used to chop trees. (make kindling)
	IT_WEAPON_BOW,			// 15 = bow or xbow
	IT_WAND,			    // 16 = A magic storage item
	IT_TELEPAD,				// 17 = walk on teleport
	IT_SWITCH,				// 18 = this is a switch which effects some other object in the world.
	IT_BOOK,				// 19 = read this book. (static or dynamic text)
	IT_RUNE,				// 20 = can be marked and renamed as a recall rune.
	IT_BOOZE,				// 21 = booze	(drunk effect)
	IT_POTION,				// 22 = Some liquid in a container. Possibly Some magic effect.
	IT_FIRE,				// 23 = It will burn you.
	IT_CLOCK,				// 24 = or a wristwatch
	IT_TRAP,				// 25 = walk on trap.
	IT_TRAP_ACTIVE,			// 26 = some animation
	IT_MUSICAL,				// 27 = a musical instrument.
	IT_SPELL,				// 28 = magic spell effect.
	IT_GEM,					// 29 = no use yet
	IT_WATER,				// 30 = This is water (fishable) (Not a glass of water)
	IT_CLOTHING,			// 31 = All cloth based wearable stuff,
	IT_SCROLL,				// 32 = magic scroll
	IT_CARPENTRY,			// 33 = tool of some sort.
	IT_SPAWN_CHAR,			// 34 = spawn object. should be invis also.
	IT_GAME_PIECE,			// 35 = can't be removed from game.
	IT_PORTCULIS,			// 36 = Z delta moving gate. (open)
	IT_FIGURINE,			// 37 = magic figure that turns into a creature when activated.
	IT_SHRINE,				// 38 = can res you
	IT_MOONGATE,			// 39 = linked to other moon gates (hard coded locations)
	IT_UNUSED_40,			// 40 =This was typedef t_chair (Any sort of a chair item. we can sit on.)
	IT_FORGE,				// 41 = used to smelt ore, blacksmithy etc.
	IT_ORE,					// 42 = smelt to ingots.
	IT_LOG,					// 43 = make into furniture etc. lumber,logs,
	IT_TREE,				// 44 = can be chopped.
	IT_ROCK,				// 45 = can be mined for ore.
	IT_CARPENTRY_CHOP,		// 46 = tool of some sort.
	IT_MULTI,				// 47 = multi part object like house or ship.
	IT_REAGENT,				// 48
	IT_SHIP,				// 49 = this is a SHIP MULTI
	IT_SHIP_PLANK,			// 50
	IT_SHIP_SIDE,			// 51 = Should extend to make a plank
	IT_SHIP_SIDE_LOCKED,	// 52
	IT_SHIP_TILLER,			// 53 = Tiller man on the ship.
	IT_EQ_TRADE_WINDOW,		// 54 = container for the trade window.
	IT_FISH,				// 55 = fish can be cut up.
	IT_SIGN_GUMP,			// 56 = Things like grave stones and sign plaques
	IT_STONE_GUILD,			// 57 = Guild stones
	IT_ANIM_ACTIVE,			// 58 = active anium that will recycle when done.
	IT_SAND,				// 59 = sand
	IT_CLOTH,				// 60 = bolt or folded cloth
	IT_HAIR,				// 61
	IT_BEARD,				// 62 = just for grouping purposes.
	IT_INGOT,				// 63 = Ingot of some type made from IT_ORE.
	IT_COIN,				// 64 = coin of some sort. gold or otherwise.
	IT_CROPS,				// 65 = a plant that will regrow. picked type.
	IT_DRINK,				// 66 = some sort of drink (non booze, potion or water) (ex. cider)
	IT_ANVIL,				// 67 = for repair.
	IT_PORT_LOCKED,			// 68 = this portcullis must be triggered.
	IT_SPAWN_ITEM,			// 69 = spawn other items.
	IT_TELESCOPE,			// 70 = big telescope pic.
	IT_BED,					// 71 = bed. facing either way
	IT_GOLD,				// 72 = Gold Coin
	IT_MAP,					// 73 = Map object with pins.
	IT_EQ_MEMORY_OBJ,		// 74 = A Char has a memory link to some object. (I am fighting with someone. This records the fight.)
	IT_WEAPON_MACE_STAFF,	// 75 = staff type of mace. or just other type of mace.
	IT_EQ_HORSE,			// 76 = equipped horse object represents a riding horse to the client.
	IT_COMM_CRYSTAL,		// 77 = communication crystal.
	IT_GAME_BOARD,			// 78 = this is a container of pieces.
	IT_TRASH_CAN,			// 79 = delete any object dropped on it.
	IT_CANNON_MUZZLE,		// 80 = cannon muzzle. NOT the other cannon parts.
	IT_CANNON,				// 81 = the rest of the cannon.
	IT_CANNON_BALL,
	IT_ARMOR_LEATHER,		// 83 = Non metallic armor.
	IT_SEED,				// 84 = seed from fruit
	IT_JUNK,				// 85 = ring of reagents.
	IT_CRYSTAL_BALL,		// 86
	IT_SWAMP,				// 87 = swamp
	IT_MESSAGE,				// 88 = user written message item. (for bboard ussually)
	IT_REAGENT_RAW,			// 89 = Freshly grown reagents...not processed yet. NOT USED!
	IT_EQ_CLIENT_LINGER,	// 90 = Change player to NPC for a while.
	IT_SNOW,				// 91 = snow
	IT_ITEM_STONE,			// 92 = Double click for items
	IT_UNUSED_93,			// 93 = <unused> (was IT_METRONOME ticks once every n secs)
	IT_EXPLOSION,			// 94 = async explosion.
	IT_EQ_NPC_SCRIPT,		// [OFF] 95 = Script npc actions in the form of a book.
	IT_WEB,					// 96 = walk on this and transform into some other object. (stick to it)
	IT_GRASS,				// 97 = can be eaten by grazing animals
	IT_AROCK,				// 98 = a rock or boulder. can be thrown by giants.
	IT_TRACKER,				// 99 = points to a linked object.
	IT_SOUND,				// 100 = this is a sound source.
	IT_STONE_TOWN,			// 101 = Town stones. everyone free to join.
	IT_WEAPON_MACE_CROOK,	// 102
	IT_WEAPON_MACE_PICK,	// 103
	IT_LEATHER,				// 104 = Leather or skins of some sort.(not wearable)
	IT_SHIP_OTHER,			// 105 = some other part of a ship.
	IT_BBOARD,				// 106 = a container and bboard object.
	IT_SPELLBOOK,			// 107 = spellbook (with spells)
	IT_CORPSE,				// 108 = special type of item.
	IT_TRACK_ITEM,			// 109 - track a id or type of item.
	IT_TRACK_CHAR,			// 110 = track a char or range of char id's
	IT_WEAPON_ARROW,		// 111
	IT_WEAPON_BOLT,			// 112
	IT_EQ_VENDOR_BOX,		// 113 = an equipped vendor .
	IT_EQ_BANK_BOX,			// 114 = an equipped bank box
	IT_DEED,				// 115
	IT_LOOM,				// 116
	IT_BEE_HIVE,			// 117
	IT_ARCHERY_BUTTE,		// 118
	IT_EQ_MURDER_COUNT,		// 119 = my murder count flag.
	IT_EQ_STUCK,			// 120
	IT_TRAP_INACTIVE,		// 121 = a safe trap.
	IT_UNUSED_122,			// 122 = <unused> (was IT_STONE_ROOM for mapped house regions)
	IT_BANDAGE,				// 123 = can be used for healing.
	IT_CAMPFIRE,			// 124 = this is a fire but a small one.
	IT_MAP_BLANK,			// 125 = blank map.
	IT_SPY_GLASS,			// 126
	IT_SEXTANT,				// 127
	IT_SCROLL_BLANK,		// 128
	IT_FRUIT,				// 129
	IT_WATER_WASH,			// 130 = water that will not contain fish. (for washing or drinking)
	IT_WEAPON_AXE,			// 131 = not the same as a sword. but uses swordsmanship skill
	IT_WEAPON_XBOW,			// 132
	IT_SPELLICON,			// 133
	IT_DOOR_OPEN,			// 134 = You can walk through doors that are open.
	IT_MEAT_RAW,			// 135 = raw (uncooked meat) or part of a corpse.
	IT_GARBAGE,				// 136 = this is junk.
	IT_KEYRING,				// 137
	IT_TABLE,				// 138 = a table top
	IT_FLOOR,				// 139
	IT_ROOF,				// 140
	IT_FEATHER,				// 141 = a birds feather
	IT_WOOL,				// 142 = Wool cut frm a sheep.
	IT_FUR,					// 143
	IT_BLOOD,				// 144 = blood of some creature
	IT_FOLIAGE,				// 145 = does not go invis when reaped. but will if eaten
	IT_GRAIN,				// 146
	IT_SCISSORS,			// 147
	IT_THREAD,				// 148
	IT_YARN,				// 149
	IT_SPINWHEEL,			// 150
	IT_BANDAGE_BLOOD,		// 151 = must be washed in water to get bandage back.
	IT_FISH_POLE,			// 152
	IT_SHAFT,				// 153 = used to make arrows and xbolts
	IT_LOCKPICK,			// 154
	IT_KINDLING,			// 155 = lit to make campfire
	IT_TRAIN_DUMMY,			// 156
	IT_TRAIN_PICKPOCKET,	// 157
	IT_BEDROLL,				// 158
	IT_UNUSED_159,			// 159
	IT_HIDE,				// 160 = hides are cured to make leather.
	IT_CLOTH_BOLT,			// 161 = must be cut up to make cloth squares.
	IT_BOARD,				// 162 = logs are plained into decent lumber
	IT_PITCHER,				// 163
	IT_PITCHER_EMPTY,		// 164
	IT_DYE_VAT,				// 165
	IT_DYE,					// 166
	IT_POTION_EMPTY,		// 167 = empty bottle.
	IT_MORTAR,				// 168
	IT_HAIR_DYE,			// 169
	IT_SEWING_KIT,			// 170
	IT_TINKER_TOOLS,		// 171
	IT_WALL,				// 172 = wall of a structure.
	IT_WINDOW,				// 173 = window for a structure.
	IT_COTTON,				// 174 = Cotton from the plant
	IT_BONE,				// 175
	IT_EQ_SCRIPT,			// 176
	IT_SHIP_HOLD,			// 177 = ships hold.
	IT_SHIP_HOLD_LOCK,		// 178
	IT_LAVA,				// 179
	IT_SHIELD,				// 180 = equippable armor.
	IT_JEWELRY,				// 181 = equippable.
	IT_DIRT,				// 182 = a patch of dirt where i can plant something
	IT_SCRIPT,				// 183 = Scriptable item (but not equippable)
	IT_SPELLBOOK_NECRO,		// 184 = AOS Necromancy spellbook (should have MOREZ=100 by default)
	IT_SPELLBOOK_PALA,		// 185 = AOS Paladin spellbook (should have MOREZ=200 by default)
	IT_SPELLBOOK_EXTRA,		// 186 = some spellbook for script purposes
	IT_SPELLBOOK_BUSHIDO,	// 187 = SE Bushido spellbook (should have MOREZ=400 by default)
	IT_SPELLBOOK_NINJITSU,	// 188 = SE Ninjitsu spellbook (should have MOREZ=500 by default)
	IT_SPELLBOOK_ARCANIST,	// 189 = ML Spellweaver spellbook (should have MOREZ=600 by default)
	IT_MULTI_CUSTOM,		// 190 = Customisable multi
	IT_SPELLBOOK_MYSTIC,	// 191 = SA Mysticism spellbook (should have MOREX=677 by default)
	IT_HOVEROVER,			// 192 = Hover-over item (CAN_C_HOVER can hover over blocking items)
	IT_SPELLBOOK_MASTERY,	// 193 = SA/TOL Masteries spellbook (should have MOREZ=700 by default)
	IT_WEAPON_THROWING,		// 194 = Throwing Weapon
	IT_CARTOGRAPHY,			// 195 = cartography tool
	IT_COOKING,				// 196 = cooking tool
	IT_PILOT,				// 197 = ship's pilot (PacketWheelMove)
	IT_ROPE,				// 198 = t_rope (working like t_ship_plank but without id changes)
    IT_WEAPON_WHIP,			// 199
    
    // New SphereX hardcoded types starting from 300
    IT_SPAWN_CHAMPION = 300,// 300 = t_spawn_champion
    IT_MULTI_ADDON,         // 301 = t_multi_addon
	IT_ARMOR_CHAIN,			// 302 = t_armor_chain
	IT_ARMOR_RING,			// 303 = t_armor_ring
	IT_TALISMAN,			// 304 = t_talisman
	IT_ARMOR_BONE,			// 305 = t_armor_bone

	IT_QTY,
	IT_TRIGGER = 1000	// custom triggers starts from here
};


#endif // _INC_ITEMTYPES_H
