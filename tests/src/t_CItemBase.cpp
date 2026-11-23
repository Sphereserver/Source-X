#include <doctest/doctest.h>
#include "../../src/game/items/CItemBase.h"
#include "../../src/game/uo_files/uofiles_enums_itemid.h"

// The item IDs: the tiles, then the multis (ITEMID_MULTI + the multi index, up to MULTI_QTY multis), then the IDs given to
//  named ITEMDEFs and MULTIDEFs.

namespace
{
    constexpr ITEMID_TYPE Id(int iId) noexcept
    {
        return static_cast<ITEMID_TYPE>(iId);
    }
}

TEST_CASE("CItemBase multi IDs")
{
    CHECK(!CItemBase::IsID_Multi(ITEMID_NOTHING));
    CHECK(!CItemBase::IsID_Multi(Id(ITEMID_MULTI - 1)));     // The last tile.
    CHECK(CItemBase::IsID_Multi(ITEMID_MULTI));
    CHECK(CItemBase::IsID_Multi(Id(ITEMID_MULTI + 0x1FFF)));
    // Beyond the old limit of 0x2000 multis: newer client data has them.
    CHECK(CItemBase::IsID_Multi(Id(ITEMID_MULTI + 0x2000)));
    CHECK(CItemBase::IsID_Multi(ITEMID_MULTI_MAX));
    // The first ID of the named ITEMDEFs and MULTIDEFs: not a multi by itself.
    CHECK(!CItemBase::IsID_Multi(Id(ITEMID_MULTI_MAX + 1)));
    CHECK(!CItemBase::IsID_Multi(ITEMID_SCRIPT_NAMED));
}

TEST_CASE("CItemBase display IDs (artwork: a tile or a multi)")
{
    CHECK(!CItemBase::IsValidDispID(ITEMID_NOTHING));
    CHECK(CItemBase::IsValidDispID(Id(1)));
    CHECK(CItemBase::IsValidDispID(Id(ITEMID_MULTI - 1)));
    CHECK(CItemBase::IsValidDispID(ITEMID_MULTI));
    CHECK(CItemBase::IsValidDispID(Id(ITEMID_MULTI + 0x2000)));
    CHECK(CItemBase::IsValidDispID(ITEMID_MULTI_MAX));
    // A named ITEMDEF or MULTIDEF has no artwork of its own: it takes it from its ID= line.
    CHECK(!CItemBase::IsValidDispID(Id(ITEMID_MULTI_MAX + 1)));
    CHECK(!CItemBase::IsValidDispID(ITEMID_SCRIPT_NAMED));
}
