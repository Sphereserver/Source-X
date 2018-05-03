/**
* @file CFaction.cpp
*
*/

#include "CFaction.h"
#include "CObjBase.h"

bool CFaction::IsOppositeGroup(CFaction *target)
{
    ADDTOCALLSTACK("CFaction::IsOppositeGroup");
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

bool CFaction::IsOppositeSuperSlayer(CFaction *target)
{
    ADDTOCALLSTACK("CFaction::IsOppositeSuperSlayer");
    if ((IsGroupFey()) && (target->GetFactionID() & FACTION_FEY))
        return true;
    else if ((IsGroupElemental()) && (target->GetFactionID() & FACTION_ELEMENTAL))
        return true;
    else if ((IsGroupAbyss()) && (target->GetFactionID() & FACTION_DEMON))
        return true;
    else if ((IsGroupHumanoid()) && (target->GetFactionID() & FACTION_REPOND))
        return true;
    else if ((IsGroupUndead()) && (target->GetFactionID() & FACTION_UNDEAD))
        return true;
    else if ((IsGroupArachnid()) && (target->GetFactionID() & FACTION_ARACHNID))
        return true;
    else if ((IsGroupReptilian()) && (target->GetFactionID() & FACTION_REPTILE))
        return true;
    return false;
}

bool CFaction::IsOppositeLesserSlayer(CFaction *target)
{
    ADDTOCALLSTACK("CFaction::IsOppositeLesserSlayer");
    // Start Elemental Lesser Slayers
    if ((GetFactionID() & FACTION_BLOODELEMENTAL) && (target->GetFactionID() & FACTION_BLOODELEMENTAL))
        return true;
    else if ((GetFactionID() & FACTION_EARTHELEMENTAL) && (target->GetFactionID() & FACTION_EARTHELEMENTAL))
        return true;
    else if ((GetFactionID() & FACTION_FIREELEMENTAL) && (target->GetFactionID() & FACTION_FIREELEMENTAL))
        return true;
    else if ((GetFactionID() & FACTION_POISONELEMENTAL) && (target->GetFactionID() & FACTION_POISONELEMENTAL))
        return true;
    else if ((GetFactionID() & FACTION_SNOWELEMENTAL) && (target->GetFactionID() & FACTION_SNOWELEMENTAL))
        return true;
    else if ((GetFactionID() & FACTION_WATERELEMENTAL) && (target->GetFactionID() & FACTION_WATERELEMENTAL))
        return true;

    // Abyss Lesser Slayers
    else if ((GetFactionID() & FACTION_GARGOYLE) && (target->GetFactionID() & FACTION_GARGOYLE))
        return true;

    // Humanoid Lesser Slayers
    else if ((GetFactionID() & FACTION_GOBLIN) && (target->GetFactionID() & FACTION_GOBLIN))
        return true;
    else if ((GetFactionID() & FACTION_VERMIN) && (target->GetFactionID() & FACTION_VERMIN))
        return true;
    else if ((GetFactionID() & FACTION_OGRE) && (target->GetFactionID() & FACTION_OGRE))
        return true;
    else if ((GetFactionID() & FACTION_ORC) && (target->GetFactionID() & FACTION_ORC))
        return true;
    else if ((GetFactionID() & FACTION_TROLL) && (target->GetFactionID() & FACTION_TROLL))
        return true;

    // Undead Lesser Slayers
    else if ((GetFactionID() & FACTION_MAGE) && (target->GetFactionID() & FACTION_MAGE))
        return true;

    // Arachnid Lesser Slayers
    else if ((GetFactionID() & FACTION_SCORPION) && (target->GetFactionID() & FACTION_SCORPION))
        return true;
    else if ((GetFactionID() & FACTION_SPIDER) && (target->GetFactionID() & FACTION_SPIDER))
        return true;
    else if ((GetFactionID() & FACTION_TERATHAN) && (target->GetFactionID() & FACTION_TERATHAN))
        return true;

    // Reptilian Lesser Slayers
    else if ((GetFactionID() & FACTION_DRAGON) && (target->GetFactionID() & FACTION_DRAGON))
        return true;
    else if ((GetFactionID() & FACTION_OPHIDIAN) && (target->GetFactionID() & FACTION_OPHIDIAN))
        return true;
    else if ((GetFactionID() & FACTION_SNAKE) && (target->GetFactionID() & FACTION_SNAKE))
        return true; 
    else if ((GetFactionID() & FACTION_LIZARDMAN) && (target->GetFactionID() & FACTION_LIZARDMAN))
        return true;

    // Old ML's Lesser Slayers
    else if ((GetFactionID() & FACTION_BAT) && (target->GetFactionID() & FACTION_BAT))
        return true;
    else if ((GetFactionID() & FACTION_BEAR) && (target->GetFactionID() & FACTION_BEAR))
        return true;
    else if ((GetFactionID() & FACTION_BEETLE) && (target->GetFactionID() & FACTION_BEETLE))
        return true;
    else if ((GetFactionID() & FACTION_BIRD) && (target->GetFactionID() & FACTION_BIRD))
        return true;

    // Standalone Lesser Slayers
    else if ((GetFactionID() & FACTION_BOVINE) && (target->GetFactionID() & FACTION_BOVINE))
        return true;
    else if ((GetFactionID() & FACTION_FLAME) && (target->GetFactionID() & FACTION_FLAME))
        return true;
    else if ((GetFactionID() & FACTION_ICE) && (target->GetFactionID() & FACTION_ICE))
        return true;
    else if ((GetFactionID() & FACTION_WOLF) && (target->GetFactionID() & FACTION_WOLF))
        return true;

    return false;
}

enum CHF_TYPE
{
    CHF_FACTION,
    CHF_SLAYER,
    CHF_FACTIONGROUP,
    CHF_SLAYERGROUP,
    CHF_QTY
};

lpctstr const CFaction::sm_szLoadKeys[CHF_QTY + 1] =
{
    "FACTION",
    "SLAYER",
    "FACTIONGROUP",
    "SLAYERGROUP",
    NULL
};

CFaction::CFaction( const CObjBase* pLink) : CFactionDef(), CComponent(COMP_FACTION, pLink)
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::CFaction(FACTION_TYPE)");
    _iFaction = FACTION_NONE;
}

CFaction::CFaction(CFaction *copy, const CObjBase* pLink) : CFactionDef(), CComponent(COMP_FACTION, pLink)
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::CFaction(CFaction*)");
    Copy(copy);
}

void CFaction::Delete(bool fForced)
{
    UNREFERENCED_PARAMETER(fForced);
}

bool CFaction::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CFaction::r_LoadVal");
    lpctstr	pszKey = s.GetKey();
    CHF_TYPE iKeyNum = (CHF_TYPE)FindTableHeadSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    switch (iKeyNum)
    {
        case CHF_FACTION:
        case CHF_SLAYER:
        {
            SetFactionID(static_cast<NPC_FACTION>(s.GetArgULLVal()));
            return true;
        }
        default:
            return false;
    }
    return false;
}

bool CFaction::r_Load(CScript & s)
{
    ADDTOCALLSTACK("CFaction::r_Load");
    UNREFERENCED_PARAMETER(s);
    return true;
}

bool CFaction::r_WriteVal(lpctstr pszKey, CSString & s, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CFaction::CFaction");
    CHF_TYPE iKeyNum = (CHF_TYPE)FindTableHeadSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    UNREFERENCED_PARAMETER(pSrc);
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

void CFaction::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CFaction::r_Write");
    if (GetFactionID() != FACTION_NONE){
        s.WriteKeyVal("FACTION", (int)GetFactionID()); // Same value stored with different names for CChars and CItems.
    }
}

bool CFaction::r_GetRef(lpctstr & pszKey, CScriptObj *& pRef)
{
    UNREFERENCED_PARAMETER(pszKey);
    UNREFERENCED_PARAMETER(pRef);
    return false;
}

bool CFaction::r_Verb(CScript & s, CTextConsole * pSrc)
{
    UNREFERENCED_PARAMETER(s);
    UNREFERENCED_PARAMETER(pSrc);
    return false;
}

void CFaction::Copy(CComponent * target)
{
    ADDTOCALLSTACK("CFaction::Copy");
    CFaction *pTarget = static_cast<CFaction*>(target);
    if (pTarget)
    {
        _iFaction = pTarget->GetFactionID();
    }
}

NPC_GROUP CFaction::GetGroupID()
{
    ADDTOCALLSTACK("CFaction::GetGroupID");
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

NPC_FACTION CFaction::GetFactionID()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::GetFactionID");
    return _iFaction;
}

void CFaction::SetFactionID(NPC_FACTION faction)
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::SetFactionID");
    ASSERT(faction < FACTION_QTY);
    _iFaction = faction;
}

bool CFaction::IsGroupElemental()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::IsGroupElemental");
    return ((_iFaction >= FACTION_ELEMENTAL) && (_iFaction < FACTION_ELEMENTAL_QTY));
}

bool CFaction::IsGroupFey()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::IsGroupFey");
    return (_iFaction == FACTION_FEY);
}

bool CFaction::IsGroupAbyss()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::IsGroupAbyss");
    return ((_iFaction >= FACTION_DEMON) && (_iFaction < FACTION_ABYSS_QTY));
}

bool CFaction::IsGroupHumanoid()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::IsGroupHumanoid");
    return ((_iFaction >= FACTION_REPOND) && (_iFaction < FACTION_HUMANOID_QTY));
}

bool CFaction::IsGroupUndead()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::IsGroupUndead");
    return ((_iFaction >= FACTION_UNDEAD) && (_iFaction < FACTION_UNDEAD_QTY));
}

bool CFaction::IsGroupArachnid()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::IsGroupArachnid");
    return ((_iFaction >= FACTION_ARACHNID) && (_iFaction < FACTION_ARACHNID_QTY));
}

bool CFaction::IsGroupReptilian()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::IsGroupReptilian");
    return ((_iFaction >= FACTION_REPTILE) && (_iFaction < FACTION_REPTILIAN_QTY));
}

bool CFaction::IsSuperSlayer()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::IsSuperSlayer");
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
        default:
            return false;
    }
    return false;
}

bool CFaction::IsLesserSlayer()
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::IsLesserSlayer");
    if ((_iFaction > FACTION_NONE) && (_iFaction < FACTION_QTY) && (!IsSuperSlayer()))
        return true;
    return false;
}

int CFaction::GetSlayerDamageBonus(CFaction *target)
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::GetSlayerDamageBonus");
    if (IsOppositeLesserSlayer(target))
        return DAMAGE_SLAYER_LESSER;
    else if (IsOppositeSuperSlayer(target))
        return DAMAGE_SLAYER_SUPER;
    return 1;
}

int CFaction::GetSlayerDamagePenalty(CFaction * target)
{
    ADDTOCALLSTACK_INTENSIVE("CFaction::GetSlayerDamagePenalty");
    if (IsOppositeGroup(target))
        return DAMAGE_SLAYER_OPPOSITE;
    return 1;
}

NPC_FACTION CFactionDef::GetFactionID()
{
    return _iFaction;
}

void CFactionDef::SetFactionID(NPC_FACTION faction)
{
    _iFaction = faction;
}
