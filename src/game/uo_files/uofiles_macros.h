/**
* @file uofiles_macros.h
*
*/

#ifndef _INC_UOFILES_MACROS_H
#define _INC_UOFILES_MACROS_H

#include "uofiles_enums.h"


// Door ID Attribute flags
#define DOOR_OPENED			0x00000001  // set is open
#define DOOR_RIGHTLEFT		0x00000002
#define DOOR_INOUT			0x00000004
#define DOOR_NORTHSOUTH		0x00000008

// Sight distance definitions
#define UO_MAP_VIEW_SIGHT			14	// Max sight distance of creatures is 14.
#define UO_MAP_VIEW_SIZE_DEFAULT	18  // Visibility for normal items (on old clients it's always 18;
										//  since 2d client 7.0.55.27 it's dynamic 18-24 based on client screen resolution;
										//  enhanced client supports values up to 24)
#define UO_MAP_VIEW_SIZE_MAX		24
#define UO_MAP_VIEW_RADAR			31  // Visibility for castles, keeps and boats

// Map definitions
#define UO_BLOCK_SIZE		8               // Base width/height size of a block.
#define UO_BLOCK_ALIGN(i)	((i) & ~7 )
#define UO_BLOCK_OFFSET(i)	((i) &  7 )     // i%UO_BLOCK_SIZE
#define UO_SIZE_Z			127
#define UO_SIZE_MIN_Z		-127

#define UO_SIZE_X_REAL		0x1400  // 640*UO_BLOCK_SIZE = 5120 = The actual world is only this big

#define UOTILE_BLOCK_QTY	32      // Each tiledata block has 32 items/terrain tiles.

// This should depend on height of players char.
#define PLAYER_HEIGHT		16  // We need x units of room to walk under something. (human) ??? this should vary based on creature type.

// Tiledata flags
#define UFLAG1_FLOOR		0x00000001  // 0= floor (Walkable at base position)
#define UFLAG1_EQUIP		0x00000002  // 1= equipable. m_layer is LAYER_TYPE
#define UFLAG1_NONBLOCKING	0x00000004  // 2= Signs and railings that do not block.
#define UFLAG1_LIQUID		0x00000008  // 3= blood,Wave,Dirt,webs,stain, (translucent items)
#define UFLAG1_WALL			0x00000010  // 4= wall type = wall/door/fireplace
#define UFLAG1_DAMAGE		0x00000020  // 5= damaging. (Sharp, hot or poisonous)
#define UFLAG1_BLOCK		0x00000040  // 6= blocked for normal human. (big and heavy)
#define UFLAG1_WATER		0x00000080  // 7= water/wet (blood/water)
#define UFLAG2_ZERO1		0x00000100  // 8= NOT USED (i checked)
#define UFLAG2_PLATFORM		0x00000200  // 9= platform/flat (can stand on, bed, floor, )
#define UFLAG2_CLIMBABLE	0x00000400  // a= climbable (stairs). m_height /= 2(For Stairs+Ladders)
#define UFLAG2_STACKABLE	0x00000800  // b= pileable/stackable (m_dwUnk7 = stack size ?)
#define UFLAG2_WINDOW		0x00001000  // c= window/arch/door can walk thru it
#define UFLAG2_WALL2		0x00002000  // d= another type of wall. (not sure why there are 2)
#define UFLAG2_A			0x00004000  // e= article a
#define UFLAG2_AN			0x00008000  // f= article an
#define UFLAG3_DESCRIPTION  0x00010000  // 10= descriptional tile. (Regions, professions, ...)
#define UFLAG3_TRANSPARENT	0x00020000  // 11= Transparent (Is used for leaves and sails)
#define UFLAG3_CLOTH		0x00040000  // 12= Probably dyeable ? effects the way the client colors the item. color gray scale stuff else must be converted to grayscale
#define UFLAG3_ZERO8		0x00080000  // 13= 0 NOT USED (i checked)
#define UFLAG3_MAP			0x00100000  // 14= map
#define UFLAG3_CONTAINER	0x00200000  // 15= container.
#define UFLAG3_EQUIP2		0x00400000  // 16= equipable (not sure why there are 2 of these)
#define UFLAG3_LIGHT		0x00800000  // 17= light source
#define UFLAG4_ANIM			0x01000000  // 18= animation with next several object frames.
#define UFLAG4_HOVEROVER	0x02000000  // 19= item can be hovered over (SA tiledata) (older tiledata has this applied to archway, easel, fountain - unknown purpose)
#define UFLAG4_WALL3		0x04000000  // 1a= tend to be types of walls ? I have no idea.
#define UFLAG4_BODYITEM		0x08000000  // 1b= Whole body item (ex.British", "Blackthorne", "GM Robe" and "Death shroud")
#define UFLAG4_ROOF			0x10000000  // 1c=
#define UFLAG4_DOOR			0x20000000  // 1d= door
#define UFLAG4_STAIRS		0x40000000  // 1e=
#define UFLAG4_WALKABLE		0x80000000  // 1f= We can walk here.


// Light levels
#define LIGHT_BRIGHT	0
#define LIGHT_DARK		30
#define LIGHT_BLACK		32

#define TRAMMEL_FULL_BRIGHTNESS		2		// light units LIGHT_BRIGHT
#define FELUCCA_FULL_BRIGHTNESS		6		// light units LIGHT_BRIGHT


#endif //_INC_UOFILES_MACROS_H
