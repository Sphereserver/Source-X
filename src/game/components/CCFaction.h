/**
* @file CCFaction.h
*
*/

#ifndef _INC_CCFACTION_H
#define _INC_CCFACTION_H

#include "../CComponent.h"

class CItem;


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

/*
    Groups of NPC_FACTION
*/
enum NPC_GROUP
{
    NPCGROUP_NONE       = 0,
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
    Faction IDs
*/
enum NPC_FACTION : llong
{
    FACTION_NONE                = 0,
    // Fey Group (opposed to Abyss Group)
    FACTION_FEY                 = 0x1,          // SuperSlayer

    // Elemental Group (opposed to Abyss Group)
    FACTION_ELEMENTAL           = 0x2,          //  SuperSlayer
    FACTION_AIR_ELEMENTAL       = 0x4,
    FACTION_BLOOD_ELEMENTAL     = 0x8,
    FACTION_EARTH_ELEMENTAL     = 0x10,
    FACTION_FIRE_ELEMENTAL      = 0x20,
    FACTION_POISON_ELEMENTAL    = 0x40,
    FACTION_SNOW_ELEMENTAL      = 0x90,
    FACTION_WATER_ELEMENTAL     = 0x100,
    FACTION_ELEMENTAL_QTY,

    // Abyss Group (opposed to Elemental and Fey Groups)
    FACTION_DEMON               = 0x200,        // SuperSlayer
    FACTION_GARGOYLE            = 0x400,
    FACTION_ABYSS_QTY,

    // Humanoid Group (opposed to Undead Group)
    FACTION_REPOND              = 0x800,        // SuperSlayer
    FACTION_GOBLIN              = 0x1000,
    FACTION_VERMIN              = 0x2000,
    FACTION_OGRE                = 0x4000,
    FACTION_ORC                 = 0x8000,
    FACTION_TROLL               = 0x10000,
    FACTION_HUMANOID_QTY,

    // Undead Group (opposed to Humanoid Group)
    FACTION_UNDEAD              = 0x20000,      // SuperSlayer
    FACTION_MAGE                = 0x40000,
    FACTION_UNDEAD_QTY,

    // Arachnid Group (opposed to Reptilian Group)
    FACTION_ARACHNID            = 0x80000,      // SuperSlayer
    FACTION_SCORPION            = 0x100000,
    FACTION_SPIDER              = 0x200000,
    FACTION_TERATHAN            = 0x400000,
    FACTION_ARACHNID_QTY,

    // Reptile Group (opposed to Arachnid)
    FACTION_REPTILE             = 0x800000,     // SuperSlayer
    FACTION_DRAGON              = 0x1000000,
    FACTION_OPHIDIAN            = 0x2000000,
    FACTION_SNAKE               = 0x4000000,
    FACTION_LIZARDMAN           = 0x8000000,
    FACTION_REPTILIAN_QTY,

    // Old Mondain’s Legacy Slayers
    FACTION_BAT                 = 0x10000000,
    FACTION_BEAR                = 0x20000000,
    FACTION_BEETLE              = 0x40000000,
    FACTION_BIRD                = 0x80000000,

    // Standalone Slayers
    FACTION_BOVINE              = 0x100000000,
    FACTION_FLAME               = 0x200000000,
    FACTION_ICE                 = 0x400000000,
    FACTION_WOLF                = 0x800000000,
    FACTION_QTY
};

class CChar;
/*
    This class is used to store FACTION for NPCs and SLAYER for items
    Initially involved in the Slayer system it handles Super Slayer,
    Lesser Slayer and Opposites to increase damage dealt/received
*/

class CFactionDef
{
protected:
    NPC_FACTION _iFaction;
public:
    CFactionDef();
    NPC_FACTION GetFactionID() const;
    void SetFactionID(NPC_FACTION faction);
};

class CCFaction : public CFactionDef, public CComponent
{
    static lpctstr const sm_szLoadKeys[];

public:
    CCFaction();
    CCFaction(CCFaction *copy);
	virtual ~CCFaction() override = default;
    static bool CanSubscribe(const CItem* pItem);
    virtual void Delete(bool fForced = false) override;
    virtual bool r_LoadVal(CScript & s) override;
    virtual bool r_WriteVal(lpctstr ptcKey, CSString & s, CTextConsole * pSrc = nullptr) override;
    virtual void r_Write(CScript & s) override;
    virtual bool r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef) override;
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) override;
    virtual void Copy(const CComponent *target) override;
    bool r_Load(CScript & s);  // Load a character from Script
    virtual CCRET_TYPE OnTickComponent() override;
    /*
        Sets the Group to the specified type.
    */
    void SetFactionID(NPC_FACTION group);
    /*
        Return true if I'm in the 'Elemental' group.
    */
    bool IsGroupElemental() const;
    /*
    Return true if I'm in the 'Fey' group.
    */
    bool IsGroupFey() const;
    /*
    Return true if I'm in the 'Abyss' group.
    */
    bool IsGroupAbyss() const;
    /*
    Return true if I'm in the 'Humaoind' group.
    */
    bool IsGroupHumanoid() const;
    /*
    Return true if I'm in the 'Undead' group.
    */
    bool IsGroupUndead() const;
    /*
    Return true if I'm in the 'Arachnid' group.
    */
    bool IsGroupArachnid() const;
    /*
    Return true if I'm in the 'Reptilian' group.
    */
    bool IsGroupReptilian() const;

    /*
        Checks for an specific Group and returns it I fit in anyone or FACTION_QTY if not.
    */
    NPC_GROUP GetGroupID() const;

    /*
    Checks for an specific Faction and returns it I fit in anyone or FACTION_QTY if not.
    */
    NPC_FACTION GetFactionID() const;

    /*
        Checks my group and the target's one and return true if we are enemies.
    */
    bool IsOppositeGroup(const CCFaction *target) const;

    /*
    Returns true if I'm a Super Slayer and the target has also the same type
    */
    bool IsOppositeSuperSlayer(const CCFaction *target) const;
    /*
    Returns true if I'm a Lesser Slayer and the target has also the same type
    */
    bool IsOppositeLesserSlayer(const CCFaction *target) const;
    /*
        Returns true if I'm a Super Slayer
    */
    bool IsSuperSlayer() const;

    /*
        Returns true if I'm a Lesser Slayer
    */
    bool IsLesserSlayer() const;
    /*
        Returns the Slayers's damage bonus (if any).
        NOTE: We can't direct check for target's Super Slayer or Lesser Slayer and compare
        with ours because we may both have more than one Group (even if we shouldn't,
        Sphere's 'versatility' forces us to do so).
    */
    int GetSlayerDamageBonus(const CCFaction *target) const;

    /*
        Wielding a slayer type against its opposite will cause the attacker to take more damage
        returns the penalty damage.
    */
    int GetSlayerDamagePenalty(const CCFaction *target) const;

};

#endif // _INC_CCFACTION_H
