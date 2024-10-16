/**
* @file CFactionDef.cpp
*
*/

#include "CFactionDef.h"
#include "../../items/CItem.h"


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
    const uint32 masked_shifted = (_uiFactionData & _kuiGroupMask) >> _kuiGroupReservedBytes;
    return enum_alias_cast<CFactionDef::Group>(masked_shifted);
}

bool CFactionDef::SetGroup(CFactionDef::Group group) noexcept
{
    const uint32 group_numeric = enum_alias_cast<uint32>(group);
    if (group_numeric > _kuiGroupMaxVal)
        return false;

    const uint32 masked_shifted = (group_numeric & _kuiGroupMaxVal) << _kuiGroupReservedBytes;
    _uiFactionData = (_uiFactionData & ~_kuiGroupMask) | n_alias_cast<uint32>(masked_shifted);
    return true;
}

CFactionDef::Species CFactionDef::GetSpecies() const noexcept
{
    return enum_alias_cast<CFactionDef::Species>(_uiFactionData & _kuiSpeciesMask);
}

bool CFactionDef::SetSpecies(CFactionDef::Species species) noexcept
{
    const uint32 species_numeric = enum_alias_cast<uint32>(species);
    if (species_numeric > _kuiSpeciesMaxVal)
        return false;

    const uint32 masked = species_numeric & _kuiSpeciesMask;
    _uiFactionData = (_uiFactionData & ~_kuiSpeciesMask) | masked;
    return true;
}

//---

bool CFactionDef::IsOppositeGroup(const CFactionDef *target) const noexcept
{
    ADDTOCALLSTACK("CFactionDef::IsOppositeGroup");
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

bool CFactionDef::IsSuperSlayerVersus(const CFactionDef *target) const noexcept
{
    ADDTOCALLSTACK("CFactionDef::IsSuperSlayerVersus");
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

bool CFactionDef::IsLesserSlayerVersus(const CFactionDef *target) const noexcept
{
    ADDTOCALLSTACK("CFactionDef::IsLesserSlayerVersus");

    // Do i belong to the same group?
    if (!(
        enum_alias_cast<uint32>(GetGroup()) &
        enum_alias_cast<uint32>(target->GetGroup()))
    )
        return false;

    // Same species?
    return (GetSpecies() == target->GetSpecies());
}

bool CFactionDef::HasSuperSlayer() const noexcept
{
    ADDTOCALLSTACK_DEBUG("CFactionDef::HasSuperSlayer");
    return (GetGroup() != Group::NONE && (_kuiSuperSlayerSpeciesIndex == enum_alias_cast<uint32>(GetSpecies())));
}

bool CFactionDef::HasLesserSlayer() const noexcept
{
    ADDTOCALLSTACK_DEBUG("CFactionDef::HasLesserSlayer");
    return ((GetGroup() != Group::NONE) && (GetSpecies() != Species::NONE) && !HasSuperSlayer());
}

/*
TODO: enable customization of slayer bonus damage?
Until the Stygian Abyss expansion, all Slayers did double damage to applicable monsters. However, following that expansion, Lesser Slayers do triple damage, and Super Slayers still do double damage.
*/
#define DAMAGE_SLAYER_LESSER    200   // Lesser Slayer does x3 (100 + 200%) damage.
#define DAMAGE_SLAYER_SUPER     100   // Super Slayer does x2 (100 + 100%) damage.
#define DAMAGE_SLAYER_OPPOSITE  100   // Opposite Slayer does x2 (100 + 100%) damage.

int CFactionDef::GetSlayerDamageBonusPercent(const CFactionDef *target) const noexcept
{
    ADDTOCALLSTACK_DEBUG("CFactionDef::GetSlayerDamageBonusPercent");
    if (IsLesserSlayerVersus(target))
        return DAMAGE_SLAYER_LESSER;
    if (IsSuperSlayerVersus(target))
        return DAMAGE_SLAYER_SUPER;
    return 0;
}

int CFactionDef::GetSlayerDamagePenaltyPercent(const CFactionDef * target) const noexcept
{
    ADDTOCALLSTACK_DEBUG("CFactionDef::GetSlayerDamagePenaltyPercent");
    if (HasSuperSlayer() && IsOppositeGroup(target))
        return - DAMAGE_SLAYER_OPPOSITE;
    return 0;
}


