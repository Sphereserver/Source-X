/**
* @file CCFaction.cpp
*
*/

#include "CCFaction.h"
#include "../items/CItem.h"


CFactionDef::CFactionDef() noexcept :
    _uiFactionData(0)
{
}

bool CFactionDef::IsNone() const noexcept
{
    return (!_uiFactionData || (GetGroup() == Group::NONE) || (GetSpecies() == Species::NONE));
}

CFactionDef::Group CFactionDef::GetGroup() const noexcept
{
    const auto masked_shifted = (_uiFactionData & _kuiGroupMask) >> _kuiGroupReservedBytes;
    return num_alias_cast<CFactionDef::Group>(masked_shifted);
}

bool CFactionDef::SetGroup(CFactionDef::Group group) noexcept
{
    const auto group_numeric = num_alias_cast<uint32>(group);
    if (group_numeric > _kuiGroupMaxVal)
        return false;

    const auto masked_shifted = (group_numeric & _kuiGroupMaxVal) << _kuiGroupReservedBytes;
    _uiFactionData = (_uiFactionData & ~_kuiSpeciesMask) | num_alias_cast<uint32>(masked_shifted);
    return true;
}

CFactionDef::Species CFactionDef::GetSpecies() const noexcept
{
    return num_alias_cast<CFactionDef::Species>(_uiFactionData & _kuiSpeciesMask);
}

bool CFactionDef::SetSpecies(CFactionDef::Species species) noexcept
{
    const auto species_numeric = num_alias_cast<uint32>(species);
    if (species_numeric > _kuiSpeciesMaxVal)
        return false;

    const auto masked = species_numeric & _kuiSpeciesMask;
    _uiFactionData = (_uiFactionData & ~_kuiSpeciesMask) | masked;
    return true;
}

//---

bool CCFaction::IsOppositeGroup(const CCFaction *target) const
{
    ADDTOCALLSTACK("CCFaction::IsOppositeGroup");
    const auto myGroup      = GetGroup();
    const auto targGroup    = target->GetGroup();

    if ((myGroup == Group::ELEMENTAL) && (targGroup == Group::ABYSS))
        return true;
    else if ((myGroup == Group::ABYSS) && ((targGroup == Group::FEY) || (targGroup == Group::ELEMENTAL)))
        return true;
    else if ((myGroup == Group::FEY) && (targGroup == Group::ABYSS))
        return true;
    else if ((myGroup == Group::REPTILIAN) && (targGroup == Group::ARACHNID))
        return true;
    else if ((myGroup == Group::ARACHNID) && (targGroup == Group::REPTILIAN))
        return true;
    else if ((myGroup == Group::HUMANOID) && (targGroup == Group::UNDEAD))
        return true;
    else if ((myGroup == Group::UNDEAD) && (targGroup == Group::HUMANOID))
        return true;
    return false;
}

bool CCFaction::IsOppositeSuperSlayer(const CCFaction *target) const
{
    ADDTOCALLSTACK("CCFaction::IsOppositeSuperSlayer");
    const auto myGroup      = GetGroup();
    const auto targGroup    = target->GetGroup();
    const auto targSpecies  = target->GetSpecies();
    if ((myGroup == Group::FEY) && (targGroup == Group::FEY) && (targSpecies == Species::FEY_SSLAYER))
        return true;
    else if ((myGroup == Group::ELEMENTAL) && (targGroup == Group::ELEMENTAL) && (targSpecies == Species::ELEMENTAL_SSLAYER))
        return true;
    else if ((myGroup == Group::ABYSS) && (targGroup == Group::ELEMENTAL) && (targSpecies == Species::DEMON_SSLAYER))
        return true;
    else if ((myGroup == Group::HUMANOID) && (targGroup == Group::HUMANOID) && (targSpecies == Species::REPOND_SSLAYER))
        return true;
    else if ((myGroup == Group::UNDEAD) && (targGroup == Group::UNDEAD) && (targSpecies == Species::UNDEAD_SSLAYER))
        return true;
    else if ((myGroup == Group::ARACHNID) && (targGroup == Group::ARACHNID) && (targSpecies == Species::ARACHNID_SSLAYER))
        return true;
    else if ((myGroup == Group::REPTILIAN) && (targGroup == Group::REPTILIAN) && (targSpecies == Species::REPTILE_SSLAYER))
        return true;
    return false;
}

bool CCFaction::IsOppositeLesserSlayer(const CCFaction *target) const
{
    ADDTOCALLSTACK("CCFaction::IsOppositeLesserSlayer");

    // Do i belong to the same group?
    if (!(
        num_alias_cast<uint32>(GetGroup()) &
        num_alias_cast<uint32>(target->GetGroup()))
    )
        return false;

    // Same species?
    return (GetSpecies() == target->GetSpecies());
}

enum CHF_TYPE : int
{
    CHF_FACTION_GROUP,
    CHF_FACTION_SPECIES,
    CHF_SLAYER_GROUP,
    CHF_SLAYER_SPECIES,
    CHF_QTY
};

lpctstr const CCFaction::sm_szLoadKeys[CHF_QTY + 1] =
{
    "FACTION_GROUP",
    "FACTION_SPECIES",
    "SLAYER_GROUP",
    "SLAYER_SPECIES",
    nullptr
};

CCFaction::CCFaction() noexcept :
    CFactionDef(), CComponent(COMP_FACTION)
{
}

CCFaction::CCFaction(CCFaction *copy)  noexcept :
    CFactionDef(), CComponent(COMP_FACTION)
{
    Copy(copy);
}


bool CCFaction::CanSubscribe(const CItem* pItem) noexcept // static
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
        case CHF_FACTION_GROUP:
        case CHF_SLAYER_GROUP:
        {
            SetGroup(num_alias_cast<Group>(s.GetArgU32Val()));
            return true;
        }

        case CHF_FACTION_SPECIES:
        case CHF_SLAYER_SPECIES:
        {
            SetSpecies(num_alias_cast<Species>(s.GetArgU32Val()));
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
        case CHF_FACTION_GROUP:
        case CHF_SLAYER_GROUP:
        {
            s.FormatHex(num_alias_cast<uint32>(GetGroup()));
            return true;
        }
        case CHF_FACTION_SPECIES:
        case CHF_SLAYER_SPECIES:
        {
            s.FormatHex(num_alias_cast<uint32>(GetSpecies()));
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
    if (_uiFactionData != 0)
    {
        s.WriteKeyHex("FACTION_GROUP", num_alias_cast<uint32>(GetGroup()));
        s.WriteKeyHex("FACTION_SPECIES", num_alias_cast<uint32>(GetSpecies()));
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
    const CCFaction *pTarget = dynamic_cast<const CCFaction*>(target);
    ASSERT(pTarget);
    _uiFactionData = pTarget->_uiFactionData;
}

bool CCFaction::IsSuperSlayer() const
{
    ADDTOCALLSTACK_DEBUG("CCFaction::IsSuperSlayer");
    return (GetGroup() != Group::NONE && (_kuiSuperSlayerSpeciesIndex == num_alias_cast<uint32>(GetSpecies())));
}

bool CCFaction::IsLesserSlayer() const
{
    ADDTOCALLSTACK_DEBUG("CCFaction::IsLesserSlayer");
    return ((GetGroup() != Group::NONE) && (GetSpecies() != Species::NONE) && !IsSuperSlayer());
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


