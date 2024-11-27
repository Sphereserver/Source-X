/**
* @file CFactionDef.h
*
*/

#ifndef _INC_CFACTIONDEF_H
#define _INC_CFACTIONDEF_H

#include "../../../common/common.h"


class CChar;
class CItem;
class CItemBase;

/*
    The Original Slayers fall into 6 groups. Abyss, Arachnid, Elemental, Humanoid, Reptilian and Undead.
    Each group has a Super Slayer that will increase damage/success on all monsters within that group.
    Each of those groups have an opposing group. (i.e. Humanoid vs. Undead)
    – The opposing group will do double damage to anyone wielding the Slayer.
    – Mondain’s Legacy Slayers, usually found on Talismans, do not have an opposing group.
    A successful use of a Slayer Weapon, Spellbook or Instrument will yield a little white flash on the monster.
    Super Slayer Weapons will do double damage to the monsters they are meant for.
    “Single Slayers” or Slayers that fall into a Super Slayer subcategory will do triple damage.

    The following Super Slayers primarily are found on Talisman: Bat, Bear, Beetle, Bird, Bovine, Flame, Ice, Mage, Vermin, & Wolf.
    The following Super Slayers primarily are found on musical instruments, weapons, and spellbooks:
        Abyss, Arachnid, Elemental, Fey, Repond, Reptile, and Undead Slayer.

    Slayer Weapons can both be found as monster loot and be crafted by using a runic crafting tool.
    Slayer Spellbooks will cause single target, direct damage spells to do double damage to the monsters they are meant for.
    Player written Slayer Spellbooks can only be crafted by scribes with high magery skill using a scribe pen.
    Slayer Instruments will increase your success chance by 20% when used on the creatures it is meant for.
    Slayer Instruments will decrease your success chance by 20% when used on the creatures from its opposing group.
    Slayer Instruments can only be found as monster loot.
    Slayer Talismans are found on Mondain’s Legacy monsters or as Quest Rewards.
*/


/*
    This class is used to store FACTION for NPCs and SLAYER for items
    Initially involved in the Slayer system it handles Super Slayer,
    Lesser Slayer and Opposites to increase damage dealt/received.
*/
class CFactionDef
{
protected:
    uint32 _uiFactionData;

public:
    enum class Group : uint32
    {
        NONE       = 0,
        FEY        = 0x1,
        ELEMENTAL  = 0x2,
        ABYSS      = 0x4,
        HUMANOID   = 0x8,
        UNDEAD     = 0x10,
        ARACHNID   = 0x20,
        REPTILIAN  = 0x40,
        QTY
        // MAX = 0x800000
    };
    // Lowest two bytes are used to store the species as a plain number.
    static constexpr uint32 _kuiGroupMask = 0xFFFF'FF00;
    static constexpr uint32 _kuiGroupReservedBytes = 8;
    static constexpr uint32 _kuiGroupMaxVal = (_kuiGroupMask >> _kuiGroupReservedBytes);

    // TODO: move the values to scripts, we don't need to know here what's the number for a given species.
    //  Only interested in superslayers (which num is always 1 btw)
    static constexpr uint32 _kuiSuperSlayerSpeciesIndex = 1;
    enum class Species : uint32
    {
        NONE                = 0,
        // MAX = 255
        // Super Slayers always have species ID 1.

        // Fey Group (opposed to Abyss Group)
        FEY_SSLAYER         = _kuiSuperSlayerSpeciesIndex,          // SuperSlayer

        // Elemental Group (opposed to Abyss Group)
        ELEMENTAL_SSLAYER   = _kuiSuperSlayerSpeciesIndex,          //  SuperSlayer
        AIR_ELEMENTAL,
        BLOOD_ELEMENTAL,
        EARTH_ELEMENTAL,
        FIRE_ELEMENTAL,
        POISON_ELEMENTAL,
        SNOW_ELEMENTAL,
        WATER_ELEMENTAL,
        ELEMENTAL_QTY,

        // Abyss Group (opposed to Elemental and Fey Groups)
        DEMON_SSLAYER       = _kuiSuperSlayerSpeciesIndex,        // SuperSlayer
        GARGOYLE,
        ABYSS_QTY,

        // Humanoid Group (opposed to Undead Group)
        REPOND_SSLAYER      = _kuiSuperSlayerSpeciesIndex,        // SuperSlayer
        GOBLIN,
        VERMIN,
        OGRE,
        ORC,
        TROLL,
        HUMANOID_QTY,

        // Undead Group (opposed to Humanoid Group)
        UNDEAD_SSLAYER       = _kuiSuperSlayerSpeciesIndex,      // SuperSlayer
        MAGE,
        UNDEAD_QTY,

        // Arachnid Group (opposed to Reptilian Group)
        ARACHNID_SSLAYER     = _kuiSuperSlayerSpeciesIndex,      // SuperSlayer
        SCORPION,
        SPIDER,
        TERATHAN,
        ARACHNID_QTY,

        // Reptile Group (opposed to Arachnid)
        REPTILE_SSLAYER      = _kuiSuperSlayerSpeciesIndex,     // SuperSlayer
        DRAGON,
        OPHIDIAN,
        SNAKE,
        LIZARDMAN,
        REPTILIAN_QTY,

        // Old Mondain’s Legacy Slayers
        BAT_SSLAYER          = _kuiSuperSlayerSpeciesIndex,
        BEAR,
        BEETLE,
        BIRD,
        //ANIMAL_QTY, // This is not a real slayer group, but we might add ANIMAL_QTY for convenience

        // Standalone Slayers
        BOVINE_SSLAYER       = _kuiSuperSlayerSpeciesIndex,
        FLAME,
        ICE,
        WOLF
        //STANDALONE_QTY
    };
    // The upper 6 bytes are used to store the family/group as a bitmask.
    static constexpr uint32 _kuiSpeciesMask = 0x0000'00FF;
    //static constexpr uint32 _kuiSpeciesReservedBytes = 24;
    static constexpr uint32 _kuiSpeciesMaxVal = _kuiSpeciesMask; // 255


public:
    CFactionDef() noexcept;
    ~CFactionDef() noexcept = default;

    bool IsNone() const noexcept;
    Group GetGroup() const noexcept;
    bool SetGroup (Group faction) noexcept;
    Species GetSpecies() const noexcept;
    bool SetSpecies(Species species) noexcept;

        /*
        Checks my group and the target's one and return true if we are enemies.
    */
    bool IsOppositeGroup(const CFactionDef *target) const noexcept;

    /*
    Returns true if I'm a Super Slayer and the target has also the same type
    */
    bool IsSuperSlayerVersus(const CFactionDef *target) const noexcept;

    /*
    Returns true if I'm a Lesser Slayer and the target has also the same type
    */
    bool IsLesserSlayerVersus(const CFactionDef *target) const noexcept;

    /*
        Returns true if I have a Super Slayer property
    */
    bool HasSuperSlayer() const noexcept;

    /*
        Returns true if I have a Lesser Slayer property
    */

    bool HasLesserSlayer() const noexcept;
    /*
        Returns the Slayers's damage bonus (if any).
        NOTE: We can't direct check for target's Super Slayer or Lesser Slayer and compare
        with ours because we may both have more than one Group (even if we shouldn't,
        Sphere's 'versatility' forces us to do so).
    */
    int GetSlayerDamageBonusPercent(const CFactionDef *target) const noexcept;

    /*
        Wielding a slayer type against its opposite will cause the attacker to take more damage
        returns the penalty damage.
    */
    int GetSlayerDamagePenaltyPercent(const CFactionDef *target) const noexcept;
};


#endif // _INC_CFACTIONDEF_H
