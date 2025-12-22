#include <doctest/doctest.h>
#include "../../src/common/CPointBase.h"
#include "../../src/game/uo_files/uofiles_macros.h"

// Reference implementations, inside anonymous namespace to make clear that the functions are local to this translation unit.
namespace { namespace ref_impl
{
    DIR_TYPE GetDir(const CPointBase &ptMe, const CPointBase &ptOther, DIR_TYPE DirDefault = DIR_QTY )
    {
        // Direction to point pt
        const int dx = (ptMe.m_x-ptOther.m_x);
        const int dy = (ptMe.m_y-ptOther.m_y);
        const int ax = abs(dx);
        const int ay = abs(dy);
        if ( ay > ax )
        {
            if ( ! ax )
                return(( dy > 0 ) ? DIR_N : DIR_S );

            const int slope = ay / ax;
            if ( slope > 2 )
                return(( dy > 0 ) ? DIR_N : DIR_S );
            if ( dx > 0 )	// westish
                return(( dy > 0 ) ? DIR_NW : DIR_SW );
            return(( dy > 0 ) ? DIR_NE : DIR_SE );
        }
        else
        {
            if ( ! ay )
            {
                if ( ! dx )
                    return( DirDefault );	// here ?
                return(( dx > 0 ) ? DIR_W : DIR_E );
            }
            const int slope = ax / ay;
            if ( slope > 2 )
                return(( dx > 0 ) ? DIR_W : DIR_E );
            if ( dy > 0 )
                return(( dx > 0 ) ? DIR_NW : DIR_NE );
            return(( dx > 0 ) ? DIR_SW : DIR_SE );
        }
    }

    bool IsValidPoint(CPointBase const& pt) noexcept
    {
        if (!((pt.m_z > -127 /* -UO_SIZE_Z */) && (pt.m_z < 127 /* UO_SIZE_Z */)))
            return false;
        if ( (pt.m_x < 0) || (pt.m_y < 0) )
            return false;
        if (   (pt.m_x >= 7168 /*g_MapList.GetMapSizeX(pt.m_map)*/)
            || (pt.m_y >= 4096 /*g_MapList.GetMapSizeY(pt.m_map)*/))
            return false;
        return true;
    }

    bool IsCharValid(CPointBase const& pt) noexcept
    {
         if ( (pt.m_z <= -UO_SIZE_Z) || (pt.m_z >= UO_SIZE_Z) )
         	return false;
          if ((pt.m_x <= 0) || (pt.m_y <= 0))
         	return false;
          if (   (pt.m_x >= 7168 /*g_MapList.GetMapSizeX(pt.m_map)*/)
              || (pt.m_y >= 4096 /*g_MapList.GetMapSizeY(pt.m_map)*/))
         	return false;
          return true;
    }
}}

TEST_CASE("CPointBase::GetDir")\
// clazy:exclude=non-pod-global-static
{
    const CPointBase pt(100, 100, 0, 0);
    CPointBase ptOther;
    //DIR_TYPE def = DIR_N;

    for (short x : {-20, -5, 0, 5, 20})
    {
        for (short y : {-20, -5, 0, 5, 20})
        {
            ptOther = pt;
            ptOther.m_x += x;
            ptOther.m_y += y;

            CHECK_EQ(pt.GetDir(ptOther), ref_impl::GetDir(pt, ptOther));
        }
    }
}


TEST_CASE("CPointBase::IsValidPoint")\
// clazy:exclude=non-pod-global-static
{
    CPointBase pt;

    SUBCASE("Valid")
    {
        pt.Set(0, 0, 0, 0);
        CHECK_EQ(pt.IsValidPoint(), ref_impl::IsValidPoint(pt));

        pt.Set(1, 2, 3, 4);
        CHECK_EQ(pt.IsValidPoint(), ref_impl::IsValidPoint(pt));

        pt.Set(0, 0, -126, 0);
        CHECK_EQ(pt.IsValidPoint(), ref_impl::IsValidPoint(pt));

        pt.Set(0, 0, 126, 0);
        CHECK_EQ(pt.IsValidPoint(), ref_impl::IsValidPoint(pt));

    }

    SUBCASE("Invalid")
    {
        // For now avoid negative values, the unsigned conversion trick will fail because sphere hasn't
        //  yet loaded map sizes and the default is -1.
        //pt.Set(-11, 2, 3, 4);
        //CHECK_EQ(pt.IsValidPoint(),  ref_impl::IsValidPoint(pt));

        pt.Set(0, 0, -128, 0); // uint8_t goes from [-128; 127]
        CHECK_EQ(pt.IsValidPoint(), ref_impl::IsValidPoint(pt));

        pt.Set(0, 0, -127, 0);
        CHECK_EQ(pt.IsValidPoint(), ref_impl::IsValidPoint(pt));

        pt.Set(0, 0, 127, 0);
        CHECK_EQ(pt.IsValidPoint(), ref_impl::IsValidPoint(pt));

        pt.Set(7168, 4096, 127, 0);
        CHECK_EQ(pt.IsValidPoint(), ref_impl::IsValidPoint(pt));
    }
}


TEST_CASE("CPointBase::IsCharValid")\
// clazy:exclude=non-pod-global-static
{
    CPointBase pt;

    SUBCASE("Valid")
    {
        pt.Set(1, 2, 3, 4);
        CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));

        pt.Set(1, 1, -126, 0);
        CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));

        pt.Set(1, 1, 126, 0);
        CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));
    }

    SUBCASE("Invalid")
    {
        pt.Set(0, 0, 0, 0);
        CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));

        //pt.Set(-11, 2, 3, 4);
        //CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));

        pt.Set(1, 1, -127, 0);
        CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));

        pt.Set(1, 1, 127, 0);
        CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));

        pt.Set(1, 1, -128, 0); // uint8_t goes from [-128; 127]
        CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));

        pt.Set(7168, 4096, 127, 0);
        CHECK_EQ(pt.IsValidPoint(),  ref_impl::IsValidPoint(pt));

        pt.Set(0, 1, 1, 0);
        CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));

        pt.Set(1, 0, 1, 0);
        CHECK_EQ(pt.IsCharValid(), ref_impl::IsCharValid(pt));
    }
}
