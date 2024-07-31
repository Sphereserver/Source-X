/**
* @file CCFaction.cpp
*
*/

#include "CCFaction.h"
#include "../items/CItem.h"


CFactionDef::CFactionDef() : _iFaction(FACTION_NONE)
{
}

NPC_FACTION CFactionDef::GetFactionID() const
{
    return _iFaction;
}

void CFactionDef::SetFactionID(NPC_FACTION faction)
{
    _iFaction = faction;
}

//---

bool CCFaction::IsOppositeGroup(const CCFaction *target) const
{
    ADDTOCALLSTACK("CCFaction::IsOppositeGroup");
    if (IsGroupElemental() && target->IsGroupAbyss())
        return true;
    else if (IsGroupAbyss() && (target->IsGroupElemental() || target->IsGroupFey()))
        return true;
    else if (IsGroupFey() && target->IsGroupAbyss())
        return true;
    else if (IsGroupReptilian() && target->IsGroupArachnid())
        return true;
    else if (IsGroupArachnid() && target->IsGroupReptilian())
        return true;
    else if (IsGroupHumanoid() && target->IsGroupUndead())
        return true;
    else if (IsGroupUndead() && target->IsGroupHumanoid())
        return true;
    return false;
}

bool CCFaction::IsOppositeSuperSlayer(const CCFaction *target) const
{
    ADDTOCALLSTACK("CCFaction::IsOppositeSuperSlayer");
    const NPC_FACTION targFaction = target->GetFactionID();
    if ((IsGroupFey()) && (targFaction & FACTION_FEY))
        return true;
    else if ((IsGroupElemental()) && (targFaction & FACTION_ELEMENTAL))
        return true;
    else if ((IsGroupAbyss()) && (targFaction & FACTION_DEMON))
        return true;
    else if ((IsGroupHumanoid()) && (targFaction & FACTION_REPOND))
        return true;
    else if ((IsGroupUndead()) && (targFaction & FACTION_UNDEAD))
        return true;
    else if ((IsGroupArachnid()) && (targFaction & FACTION_ARACHNID))
        return true;
    else if ((IsGroupReptilian()) && (targFaction & FACTION_REPTILE))
        return true;
    return false;
}

bool CCFaction::IsOppositeLesserSlayer(const CCFaction *target) const
{
    ADDTOCALLSTACK("CCFaction::IsOppositeLesserSlayer");
    // Start Elemental Lesser Slayers
    const NPC_FACTION myFaction = GetFactionID();
    const NPC_FACTION targFaction = target->GetFactionID();
    if ((myFaction & FACTION_AIR_ELEMENTAL) && (targFaction & FACTION_AIR_ELEMENTAL))
        return true;
    else if ((myFaction & FACTION_BLOOD_ELEMENTAL) && (targFaction & FACTION_BLOOD_ELEMENTAL))
        return true;
    else if ((myFaction & FACTION_EARTH_ELEMENTAL) && (targFaction & FACTION_EARTH_ELEMENTAL))
        return true;
    else if ((myFaction & FACTION_FIRE_ELEMENTAL) && (targFaction & FACTION_FIRE_ELEMENTAL))
        return true;
    else if ((myFaction & FACTION_POISON_ELEMENTAL) && (targFaction & FACTION_POISON_ELEMENTAL))
        return true;
    else if ((myFaction & FACTION_SNOW_ELEMENTAL) && (targFaction & FACTION_SNOW_ELEMENTAL))
        return true;
    else if ((myFaction & FACTION_WATER_ELEMENTAL) && (targFaction & FACTION_WATER_ELEMENTAL))
        return true;

    // Abyss Lesser Slayers
    else if ((myFaction & FACTION_GARGOYLE) && (targFaction & FACTION_GARGOYLE))
        return true;

    // Humanoid Lesser Slayers
    else if ((myFaction & FACTION_GOBLIN) && (targFaction & FACTION_GOBLIN))
        return true;
    else if ((myFaction & FACTION_VERMIN) && (targFaction & FACTION_VERMIN))
        return true;
    else if ((myFaction & FACTION_OGRE) && (targFaction & FACTION_OGRE))
        return true;
    else if ((myFaction & FACTION_ORC) && (targFaction & FACTION_ORC))
        return true;
    else if ((myFaction & FACTION_TROLL) && (targFaction & FACTION_TROLL))
        return true;

    // Undead Lesser Slayers
    else if ((myFaction & FACTION_MAGE) && (targFaction & FACTION_MAGE))
        return true;

    // Arachnid Lesser Slayers
    else if ((myFaction & FACTION_SCORPION) && (targFaction & FACTION_SCORPION))
        return true;
    else if ((myFaction & FACTION_SPIDER) && (targFaction & FACTION_SPIDER))
        return true;
    else if ((myFaction & FACTION_TERATHAN) && (targFaction & FACTION_TERATHAN))
        return true;

    // Reptilian Lesser Slayers
    else if ((myFaction & FACTION_DRAGON) && (targFaction & FACTION_DRAGON))
        return true;
    else if ((myFaction & FACTION_OPHIDIAN) && (targFaction & FACTION_OPHIDIAN))
        return true;
    else if ((myFaction & FACTION_SNAKE) && (targFaction & FACTION_SNAKE))
        return true;
    else if ((myFaction & FACTION_LIZARDMAN) && (targFaction & FACTION_LIZARDMAN))
        return true;

    // Old ML's Lesser Slayers
    else if ((myFaction & FACTION_BAT) && (targFaction & FACTION_BAT))
        return true;
    else if ((myFaction & FACTION_BEAR) && (targFaction & FACTION_BEAR))
        return true;
    else if ((myFaction & FACTION_BEETLE) && (targFaction & FACTION_BEETLE))
        return true;
    else if ((myFaction & FACTION_BIRD) && (targFaction & FACTION_BIRD))
        return true;

    // Standalone Lesser Slayers
    else if ((myFaction & FACTION_BOVINE) && (targFaction & FACTION_BOVINE))
        return true;
    else if ((myFaction & FACTION_FLAME) && (targFaction & FACTION_FLAME))
        return true;
    else if ((myFaction & FACTION_ICE) && (targFaction & FACTION_ICE))
        return true;
    else if ((myFaction & FACTION_WOLF) && (targFaction & FACTION_WOLF))
        return true;

    return false;
}

enum CHF_TYPE : int
{
    CHF_FACTION,
    CHF_SLAYER,
    CHF_FACTIONGROUP,
    CHF_SLAYERGROUP,
    CHF_QTY
};

lpctstr const CCFaction::sm_szLoadKeys[CHF_QTY + 1] =
{
    "FACTION",
    "SLAYER",
    "FACTIONGROUP",
    "SLAYERGROUP",
    nullptr
};

CCFaction::CCFaction() : CFactionDef(), CComponent(COMP_FACTION)
{
    //ADDTOCALLSTACK_DEBUG("CCFaction::CCFaction(FACTION_TYPE)");
}

CCFaction::CCFaction(CCFaction *copy) : CFactionDef(), CComponent(COMP_FACTION)
{
    //ADDTOCALLSTACK_DEBUG("CCFaction::CCFaction(CCFaction*)");
    Copy(copy);
}


bool CCFaction::CanSubscribe(const CItem* pItem) // static
{
    return pItem->IsTypeEquippable();
}

void CCFaction::Delete(bool fForced)
{
    UnreferencedParameter(fForced);
}

bool CCFaction::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CCFaction::r_LoadVal");
    CHF_TYPE iKeyNum = (CHF_TYPE)FindTableSorted(s.GetKey(), sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys) - 1);
    switch (iKeyNum)
    {
        case CHF_FACTION:
        case CHF_SLAYER:
        {
            SetFactionID(static_cast<NPC_FACTION>(s.GetArgLLVal()));
            return true;
        }
    }
    return false;
}

bool CCFaction::r_Load(CScript & s)
{
    ADDTOCALLSTACK("CCFaction::r_Load");
    UnreferencedParameter(s);
    return true;
}

CCRET_TYPE CCFaction::OnTickComponent()
{
    return CCRET_CONTINUE;
}

bool CCFaction::r_WriteVal(lpctstr ptcKey, CSString & s, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CCFaction::CCFaction");
    CHF_TYPE iKeyNum = (CHF_TYPE)FindTableSorted(ptcKey, sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys) - 1);
    UnreferencedParameter(pSrc);
    switch (iKeyNum)
    {
        case CHF_SLAYER:
        case CHF_FACTION:
        {
            s.FormatLLHex(GetFactionID());
            return true;
        }
        case CHF_SLAYERGROUP:
        case CHF_FACTIONGROUP:
        {
            s.FormatHex(GetGroupID());
            return true;
        }
        default:
            break;
    }
    return false;
}

void CCFaction::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCFaction::r_Write");
    if (GetFactionID() != FACTION_NONE){
        s.WriteKeyHex("FACTION", GetFactionID()); // Same value stored with different names for CChars and CItems.
    }
}

bool CCFaction::r_GetRef(lpctstr & ptcKey, CScriptObj *& pRef)
{
    UnreferencedParameter(ptcKey);
    UnreferencedParameter(pRef);
    return false;
}

bool CCFaction::r_Verb(CScript & s, CTextConsole * pSrc)
{
    UnreferencedParameter(s);
    UnreferencedParameter(pSrc);
    return false;
}

void CCFaction::Copy(const CComponent * target)
{
    ADDTOCALLSTACK("CCFaction::Copy");
    const CCFaction *pTarget = static_cast<const CCFaction*>(target);
    if (pTarget)
    {
        _iFaction = pTarget->GetFactionID();
    }
}

NPC_GROUP CCFaction::GetGroupID() const
{
    ADDTOCALLSTACK("CCFaction::GetGroupID");
    if (IsGroupElemental())
        return NPCGROUP_ELEMENTAL;
    else if (IsGroupAbyss())
        return NPCGROUP_ABYSS;
    else if (IsGroupFey())
        return NPCGROUP_FEY;
    else if (IsGroupHumanoid())
        return NPCGROUP_HUMANOID;
    else if (IsGroupUndead())
        return NPCGROUP_UNDEAD;
    else if (IsGroupArachnid())
        return NPCGROUP_ARACHNID;
    else if (IsGroupReptilian())
        return NPCGROUP_REPTILIAN;
    else
        return NPCGROUP_NONE;
}

NPC_FACTION CCFaction::GetFactionID() const
{
    ADDTOCALLSTACK_DEBUG("CCFaction::GetFactionID");
    return _iFaction;
}

void CCFaction::SetFactionID(NPC_FACTION faction)
{
    ADDTOCALLSTACK_DEBUG("CCFaction::SetFactionID");
    ASSERT(faction < FACTION_QTY);
    ASSERT(faction >= 0);
    _iFaction = faction;
}

bool CCFaction::IsGroupElemental() const
{
    //ADDTOCALLSTACK_DEBUG("CCFaction::IsGroupElemental");
    return ((_iFaction >= FACTION_ELEMENTAL) && (_iFaction < FACTION_ELEMENTAL_QTY));
}

bool CCFaction::IsGroupFey() const
{
    //ADDTOCALLSTACK_DEBUG("CCFaction::IsGroupFey");
    return (_iFaction == FACTION_FEY);
}

bool CCFaction::IsGroupAbyss() const
{
    //ADDTOCALLSTACK_DEBUG("CCFaction::IsGroupAbyss");
    return ((_iFaction >= FACTION_DEMON) && (_iFaction < FACTION_ABYSS_QTY));
}

bool CCFaction::IsGroupHumanoid() const
{
    //ADDTOCALLSTACK_DEBUG("CCFaction::IsGroupHumanoid");
    return ((_iFaction >= FACTION_REPOND) && (_iFaction < FACTION_HUMANOID_QTY));
}

bool CCFaction::IsGroupUndead() const
{
    //ADDTOCALLSTACK_DEBUG("CCFaction::IsGroupUndead");
    return ((_iFaction >= FACTION_UNDEAD) && (_iFaction < FACTION_UNDEAD_QTY));
}

bool CCFaction::IsGroupArachnid() const
{
    //ADDTOCALLSTACK_DEBUG("CCFaction::IsGroupArachnid");
    return ((_iFaction >= FACTION_ARACHNID) && (_iFaction < FACTION_ARACHNID_QTY));
}

bool CCFaction::IsGroupReptilian() const
{
    //ADDTOCALLSTACK_DEBUG("CCFaction::IsGroupReptilian");
    return ((_iFaction >= FACTION_REPTILE) && (_iFaction < FACTION_REPTILIAN_QTY));
}

bool CCFaction::IsSuperSlayer() const
{
    ADDTOCALLSTACK_DEBUG("CCFaction::IsSuperSlayer");
    switch (_iFaction)
    {
        case FACTION_FEY:
        case FACTION_ELEMENTAL:
        case FACTION_DEMON:
        case FACTION_REPOND:
        case FACTION_UNDEAD:
        case FACTION_ARACHNID:
        case FACTION_REPTILE:
            return true;
    }
    return false;
}

bool CCFaction::IsLesserSlayer() const
{
    ADDTOCALLSTACK_DEBUG("CCFaction::IsLesserSlayer");
    if ((_iFaction > FACTION_NONE) && (_iFaction < FACTION_QTY) && (!IsSuperSlayer()))
        return true;
    return false;
}

int CCFaction::GetSlayerDamageBonus(const CCFaction *target) const
{
    ADDTOCALLSTACK_DEBUG("CCFaction::GetSlayerDamageBonus");
    if (IsOppositeLesserSlayer(target))
        return DAMAGE_SLAYER_LESSER;
    else if (IsOppositeSuperSlayer(target))
        return DAMAGE_SLAYER_SUPER;
    return 1;
}

int CCFaction::GetSlayerDamagePenalty(const CCFaction * target) const
{
    ADDTOCALLSTACK_DEBUG("CCFaction::GetSlayerDamagePenalty");
    if (IsOppositeGroup(target))
        return DAMAGE_SLAYER_OPPOSITE;
    return 1;
}

