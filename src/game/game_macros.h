/**
* @file game_macros.h
* @brief Macros commonly used in the "game" folder (and by send/receive.cpp).
*/

#ifndef _INC_GAME_MACROS_H
#define _INC_GAME_MACROS_H


/*  Parametric macros  */

// Macro for fast NoCrypt Client version check
#define IsAosFlagEnabled( value )	( g_Cfg.m_iFeatureAOS & (value) )
#define IsResClient( value )		( GetAccount()->GetResDisp() >= (value) )


/*  Numeric macros  */
// In this case macros are more handy than enums

//--Feature
#define FEATURE_T2A_UPDATE 			0x01
#define FEATURE_T2A_CHAT 			0x02
#define FEATURE_LBR_UPDATE			0x01
#define FEATURE_LBR_SOUND			0x02

#define FEATURE_AOS_UPDATE_A		0x01	// AOS Monsters, Map, Skills
#define FEATURE_AOS_UPDATE_B		0x02	// Tooltip, Fightbook, Necro/paladin on creation, Single/Six char selection screen
#define FEATURE_AOS_POPUP			0x04	// PopUp Menus
#define FEATURE_AOS_DAMAGE			0x08

#define FEATURE_SE_UPDATE			0x01	// 0x00008 in 0xA9
#define FEATURE_SE_NINJASAM			0x02	// 0x00040 in feature

#define FEATURE_ML_UPDATE			0x01 	// 0x00100 on charlist and 0x0080 for feature to activate

#define FEATURE_KR_UPDATE			0x01	// 0x00200 in 0xA9 (KR crapness)
#define FEATURE_KR_CLIENTTYPE		0x02	// 0x00400 in 0xA9 (enables 0xE1 packet)

#define FEATURE_SA_UPDATE			0x01	// 0x10000 feature (unlock gargoyle character, housing items)
#define FEATURE_SA_MOVEMENT			0x02	// 0x04000 on charlist (new movement packets)

#define FEATURE_TOL_UPDATE			0x01	// 0x400000 feature
#define FEATURE_TOL_VIRTUALGOLD		0x02	// Not related to login flags

#define FEATURE_EXTRA_CRYSTAL		0x01	// 0x000200 feature (unlock ML crystal items on house design)
#define FEATURE_EXTRA_GOTHIC		0x02	// 0x040000 feature (unlock SA gothic items on house design)
#define FEATURE_EXTRA_RUSTIC		0x04	// 0x080000 feature (unlock SA rustic items on house design)
#define FEATURE_EXTRA_JUNGLE		0x08	// 0x100000 feature (unlock TOL jungle items on house design)
#define FEATURE_EXTRA_SHADOWGUARD	0x10	// 0x200000 feature (unlock TOL shadowguard items on house design)
#define FEATURE_EXTRA_ROLEPLAYFACES	0x20	// 0x002000 feature (unlock extra roleplay face styles on character creation) - enhanced clients only


//--Damage
#define DAMAGE_GOD			0x0001	// Nothing can block this.
#define DAMAGE_HIT_BLUNT	0x0002	// Physical hit of some sort.
#define DAMAGE_MAGIC		0x0004	// Magic blast of some sort. (we can be immune to magic to some extent)
#define DAMAGE_POISON		0x0008	// Or biological of some sort ? (HARM spell)
#define DAMAGE_FIRE			0x0010	// Fire damage of course.  (Some creatures are immune to fire)
#define DAMAGE_ENERGY		0x0020	// lightning.
#define DAMAGE_GENERAL		0x0080	// All over damage. As apposed to hitting just one point.
#define DAMAGE_ACIDIC		0x0100	// damages armor
#define DAMAGE_COLD			0x0200	// cold or water based damage
#define DAMAGE_HIT_SLASH	0x0400	// sword
#define DAMAGE_HIT_PIERCE	0x0800	// spear.
#define DAMAGE_NODISTURB	0x2000	// victim won't be disturbed
#define DAMAGE_NOREVEAL		0x4000	// Attacker is not revealed for this
#define DAMAGE_NOUNPARALYZE	0x8000  // victim won't be unparalyzed
#define DAMAGE_FIXED		0x10000	// already fixed damage, don't do calcs ... only create blood, anim, sounds... and update memories and attacker
#define DAMAGE_BREATH		0x20000	// Damage flag for breath NPC action.
#define DAMAGE_THROWN		0x40000	// Damage flag for the throw NPCs action (not the throwing skill!).
#define DAMAGE_REACTIVE     0x80000 // Damage reflected by Reactive Armor spell.

typedef dword DAMAGE_TYPE;		// describe a type of damage.


//--Spellflags
#define SPELLFLAG_DIR_ANIM			    0x0000001	// Evoke type cast or directed. (animation)
#define SPELLFLAG_TARG_ITEM			    0x0000002	// Need to target an object
#define SPELLFLAG_TARG_CHAR			    0x0000004	// Needs to target a living thing
#define SPELLFLAG_TARG_OBJ			    (SPELLFLAG_TARG_ITEM|SPELLFLAG_TARG_CHAR)
#define SPELLFLAG_TARG_XYZ			    0x0000008	// Can just target a location.
#define SPELLFLAG_HARM				    0x0000010	// The spell is in some way harmfull.
#define SPELLFLAG_FX_BOLT			    0x0000020	// Effect is a bolt to the target.
#define SPELLFLAG_FX_TARG			    0x0000040	// Effect is at the target.
#define SPELLFLAG_FIELD				    0x0000080	// create a field of stuff. (fire,poison,wall)
#define SPELLFLAG_SUMMON			    0x0000100	// summon a creature.
#define SPELLFLAG_GOOD				    0x0000200	// The spell is a good spell. u intend to help to receiver.
#define SPELLFLAG_RESIST			    0x0000400	// Allowed to resist this.
#define SPELLFLAG_TARG_NOSELF		    0x0000800
#define SPELLFLAG_FREEZEONCAST		    0x0001000	// freezes the char on cast for this spell.
#define SPELLFLAG_FIELD_RANDOMDECAY     0x0002000 // Make the field items have random timers.
#define SPELLFLAG_NO_ELEMENTALENGINE    0x0004000
#define SPELLFLAG_DISABLED			    0x0008000
#define SPELLFLAG_SCRIPTED			    0x0010000
#define	SPELLFLAG_PLAYERONLY		    0x0020000	// casted by players only
#define	SPELLFLAG_NOUNPARALYZE		    0x0040000	// casted by players only
#define SPELLFLAG_NO_CASTANIM		    0x0080000	// play no anim while casting (also override SPELLFLAG_DIR_ANIM)
#define SPELLFLAG_TARG_NO_PLAYER	    0x0100000	// if a char may be targetted, it may not be a player
#define SPELLFLAG_TARG_NO_NPC		    0x0200000	// if a char may be targetted, it may not be an NPC
#define SPELLFLAG_NOPRECAST			    0x0400000	// disable precasting for this spell
#define SPELLFLAG_NOFREEZEONCAST	    0x0800000	// disable freeze on cast for this spell
#define SPELLFLAG_AREA				    0x1000000	// area effect (uses local.arearadius)
#define SPELLFLAG_POLY				    0x2000000
#define SPELLFLAG_TARG_DEAD			    0x4000000	// Allowed to targ dead chars
#define SPELLFLAG_DAMAGE			    0x8000000	// Damage intended
#define SPELLFLAG_BLESS				    0x10000000	// Benefitial spells like Bless,Agility,etc.
#define SPELLFLAG_CURSE				    0x20000000	// Curses just like Weaken,Purge Magic,Curse,etc.
#define SPELLFLAG_HEAL                  0x40000000	// Healing spell
#define SPELLFLAG_TICK				    0x80000000	// A ticking spell like Poison.

#endif // _INC_GAME_MACROS_H
