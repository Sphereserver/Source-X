/**
* @file CCharSlayer.h
*
*/

#ifndef _INC_CCHARSLAYER_H
#define _INC_CCHARSLAYER_H

#include "../common/datatypes.h"
#include "../common/CScript.h"
#include "../common/CTextConsole.h"

/*
    The Original Slayers fall into 6 groups. Abyss, Arachnid, Elemental, Humanoid, Reptilian and Undead.
    Each group has a Super Slayer that will increase damage/success on all monsters within that group.
    Each of those groups have an opposing group. (i.e. Humanoid vs. Undead)
    – The opposing group will do double damage to anyone wielding the Slayer.
    – Mondain’s Legacy Slayers, usually found on Talismans, do not have an opposing group.
    A successful use of a Slayer Weapon, Spellbook or Instrument will yield a little white flash on the monster.
    Super Slayer Weapons will do double damage to the monsters they are meant for.
    “Single Slayers” or Slayers that fall into a Super Slayer subcategory will do triple damage
    Slayer Weapons can both be found as monster loot and be crafted by using a runic crafting tool.
    Slayer Spellbooks will cause single target, direct damage spells to do double damage to the monsters they are meant for.
    Player written Slayer Spellbooks can only be crafted by scribes with high magery skill using a scribe pen.
    Slayer Instruments will increase your success chance by 20% when used on the creatures it is meant for.
    Slayer Instruments will decrease your success chance by 20% when used on the creatures from its opposing group.
    Slayer Instruments can only be found as monster loot.
    Slayer Talismans are found on Mondain’s Legacy monsters or as Quest Rewards.
*/

#define DAMAGE_SLAYER_LESSER    3   // Lesser Slayer does x3 damage.
#define DAMAGE_SLAYER_SUPER     2   // Super Slayer does x2 damage.
#define DAMAGE_SLAYER_OPPOSITE  2   // Opposite Slayer does x2 damage.


enum FACTION_TYPE
{
    FT_ITEM,
    FT_CHAR
};
enum NPC_GROUP
{
    NPCGROUP_FEY        = 0x1,
    NPCGROUP_ELEMENTAL  = 0x2,
    NPCGROUP_ABYSS      = 0x4,
    NPCGROUP_HUMANOID   = 0x8,
    NPCGROUP_UNDEAD     = 0x10,
    NPCGROUP_ARACHNID   = 0x20,
    NPCGROUP_REPTILIAN  = 0x40,
    NPCGROUP_QTY
};
/*
    NPC's Types, factions, etc.
    Each NPC should have the 'group' and the inner type, eg: NPCGROUP_ELEMENTAL|NPCGROUP_POISONELEMENTAL or NPCGROUP_ABYSS|NPCGROUP_DEMON
*/
enum NPC_FACTION : ullong
{
    // Fey Group (opposed to Abyss Group)
    FACTION_FEY                    = 0x1,  // SuperSlayer

    // Elemental Group(opposed to Abyss Group)
    FACTION_ELEMENTAL              = 0x2,  //  SuperSlayer
    FACTION_BLOODELEMENTAL         = 0x4,
    FACTION_EARTHELEMENTAL         = 0x8,
    FACTION_FIREELEMENTAL          = 0x10,
    FACTION_POISONELEMENTAL        = 0x20,
    FACTION_SNOWELEMENTAL          = 0x40,
    FACTION_WATERELEMENTAL         = 0x80,
    FACTION_ELEMENTAL_QTY,

    // Abyss Group(opposed to Elemental and Fey Groups)
    FACTION_DEMON                  = 0x100,    // SuperSlayer
    FACTION_GARGOYLE               = 0x200,
    FACTION_ABYSS_QTY,

    // Humanoid Group (opposed to Undead Group)
    FACTION_REPOND                 = 0x400,   // SuperSlayer
    FACTION_GOBLIN                 = 0x800,
    FACTION_VERMIN                 = 0x1000,
    FACTION_OGRE                   = 0x2000,
    FACTION_ORC                    = 0x4000,
    FACTION_TROLL                  = 0x8000,
    FACTION_HUMANOID_QTY,

    // Undead Group (opposed to Humanoid Group)
    FACTION_UNDEAD                 = 0x10000,  // SuperSlayer
    FACTION_MAGE                   = 0x20000,
    FACTION_UNDEAD_QTY,

    // Arachnid Group (opposed to Reptilian Group)
    FACTION_ARACHNID               = 0x40000,  // SuperSlayer
    FACTION_SCORPION               = 0x80000,
    FACTION_SPIDER                 = 0x100000,
    FACTION_TERATHAN               = 0x200000,
    FACTION_ARACHNID_QTY,

    // Reptile Group, opposite to Arachnid
    FACTION_REPTILE                = 0x400000,    // SuperSlayer
    FACTION_DRAGON                 = 0x800000,
    FACTION_OPHIDIAN               = 0x1000000,
    FACTION_SNAKE                  = 0x2000000,
    FACTION_LIZARDMAN              = 0x4000000,
    FACTION_REPTILIAN_QTY,

    // Old Mondain’s Legacy Slayers
    FACTION_BAT                    = 0x8000000,
    FACTION_BEAR                   = 0x10000000,
    FACTION_BEETLE                 = 0x20000000,
    FACTION_BIRD                   = 0x40000000,

    // Standalone Slayers
    FACTION_BOVINE                 = 0x80000000,
    FACTION_FLAME                  = 0x100000000,
    FACTION_ICE                    = 0x200000000,
    FACTION_WOLF                   = 0x400000000,
    FACTION_QTY                    = 0x800000000
};

class CObjBase;

class CFaction
{
    static lpctstr const sm_szLoadKeys[];
    NPC_FACTION _iFaction;
    FACTION_TYPE _iType;

public:
    CFaction(FACTION_TYPE type);
    CFaction(CFaction *copy);
    void Copy(CFaction *copy);
    virtual bool r_LoadVal(CScript & s);
    virtual bool r_Load(CScript & s);  // Load a character from Script
    virtual bool r_WriteVal(lpctstr pszKey, CSString & s, CTextConsole * pSrc = NULL);
    virtual void r_Write(CScript & s);

    /*
        returns the type (FACTION or SLAYER).
    */
    FACTION_TYPE GetType();
    /*
        Sets the Group to the specified type.
    */
    void SetFactionID(NPC_FACTION group);
    /*
        Return true if I'm in the 'Elemental' group.
    */
    bool IsGroupElemental();
    /*
    Return true if I'm in the 'Fey' group.
    */
    bool IsGroupFey();
    /*
    Return true if I'm in the 'Abyss' group.
    */
    bool IsGroupAbyss();
    /*
    Return true if I'm in the 'Humaoind' group.
    */
    bool IsGroupHumanoid();
    /*
    Return true if I'm in the 'Undead' group.
    */
    bool IsGroupUndead();
    /*
    Return true if I'm in the 'Arachnid' group.
    */
    bool IsGroupArachnid();
    /*
    Return true if I'm in the 'Reptilian' group.
    */
    bool IsGroupReptilian();

    /*
        Checks for an specific Group and returns it I fit in anyone or FACTION_QTY if not.
    */
    NPC_GROUP GetGroupID();

    /*
    Checks for an specific Faction and returns it I fit in anyone or FACTION_QTY if not.
    */
    NPC_FACTION GetFactionID();

    /*
        Checks my group and the target's one and return true if we are enemies.
    */
    bool IsOppositeGroup(CFaction *target);

    /*
    Returns true if I'm a Super Slayer and the target has also the same type
    */
    bool IsOppositeSuperSlayer(CFaction *target);
    /*
    Returns true if I'm a Lesser Slayer and the target has also the same type
    */
    bool IsOppositeLesserSlayer(CFaction *target);
    /*
        Returns true if I'm a Super Slayer
    */
    bool IsSuperSlayer();

    /*
        Returns true if I'm a Lesser Slayer
    */
    bool IsLesserSlayer();
    /*
        Returns the Slayers's damage bonus (if any).
        NOTE: We can't direct check for target's Super Slayer or Lesser Slayer and compare
        with ours because we may both have more than one Group (even if we shouldn't,
        Sphere's 'versatility' forces us to do so).
    */
    int GetSlayerDamageBonus(CFaction *target);
};

#endif // _INC_CCHARSLAYER_H
