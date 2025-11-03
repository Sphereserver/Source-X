#include "../common/CLog.h"
#include "../sphere/threads.h"
#include "CRegionBase.h"

//*************************************************************************
// -CRegionBase

CRegionBase::CRegionBase()
{
	m_rectUnion.SetRectEmpty();
}

void CRegionBase::EmptyRegion()
{
    m_rectUnion.SetRectEmpty();
    m_Rects.clear();
}

size_t CRegionBase::GetRegionRectCount() const
{
	ADDTOCALLSTACK("CRegionBase::GetRegionRectCount");
	// How many rectangles in this region ?
	const size_t iQty = m_Rects.size();
	if ( iQty <= 0 )
	{
		if ( ! IsRegionEmpty())
			return 1;
	}
	return iQty;
}

CRectMap & CRegionBase::GetRegionRect(size_t i)
{
	ADDTOCALLSTACK("CRegionBase::GetRegionRect");
	// Get a particular rectangle.
	const size_t iQty = m_Rects.size();
	if ( iQty <= 0 )
		return m_rectUnion;
	return m_Rects[i];
}

const CRectMap & CRegionBase::GetRegionRect(size_t i) const
{
	ADDTOCALLSTACK("CRegionBase::GetRegionRect");
	const size_t iQty = m_Rects.size();
	if ( iQty <= 0 )
		return m_rectUnion;
	return m_Rects[i];
}

CPointMap CRegionBase::GetRegionCorner( DIR_TYPE dir ) const
{
	ADDTOCALLSTACK("CRegionBase::GetRegionCorner");
	// NOTE: DIR_QTY = center.
	return m_rectUnion.GetRectCorner(dir);
}

bool CRegionBase::IsInside2d( const CPointMap & pt ) const
{
	ADDTOCALLSTACK_DEBUG("CRegionBase::IsInside2d");
	if ( ! m_rectUnion.IsInside2d( pt ))
		return false;

	const size_t iQty = m_Rects.size();
	if ( iQty > 0 )
	{
		for ( size_t i = 0; i < iQty; ++i )
		{
			if ( m_Rects[i].IsInside2d( pt ))
				return true;
		}
		return false;
	}
	return true;
}

bool CRegionBase::AddRegionRect( const CRectMap & rect )
{
	ADDTOCALLSTACK("CRegionBase::AddRegionRect");
	if ( rect.IsRectEmpty() )
		return false;

	const size_t iQty = m_Rects.size();
	if ( iQty <= 0 && IsRegionEmpty())
	{
		m_rectUnion = rect;
	}
	else
	{
		if ( rect.m_map != m_rectUnion.m_map )
		{
			g_Log.Event(LOGL_ERROR, "Adding rect [%d,%d,%d,%d,%d] to region with different map (%d)\n",
				rect.m_left, rect.m_top, rect.m_right, rect.m_bottom, rect.m_map, m_rectUnion.m_map);
		}

		// Make sure it is not inside or equal to a previous rect !
		for ( size_t j = 0; j < iQty; ++j )
		{
			if ( rect.IsInside( m_Rects[j] ))
				return true;
		}

		if ( iQty <= 0 )
		{
			if ( rect.IsInside( m_rectUnion ))
				return true;
			m_Rects.emplace_back(m_rectUnion);
		}

		m_Rects.emplace_back(rect);
		m_rectUnion.UnionRect( rect );
	}
	return true;
}

bool CRegionBase::IsOverlapped( const CRectMap & rect ) const noexcept
{
	// ADDTOCALLSTACK("CRegionBase::IsOverlapped"); // It's called very frequently on server startup, so avoid this call
    // Does the region overlap this rectangle.

    /*
    const int left   = rect.m_left;
    const int right  = rect.m_right;
    const int top    = rect.m_top;
    const int bottom = rect.m_bottom;
    if (
        (right                  <= m_rectUnion.m_left)  &&  // Left edge of rect is to the left of this rect's right edge
        (m_rectUnion.m_right    <= left)                &&  // Right edge of rect is to the right of this rect's left edge
        (bottom                 <= m_rectUnion.m_top)   &&  // Top edge of rect is above this rect's bottom edge
        (m_rectUnion.m_bottom   <= top)                     // Bottom edge of rect is below this rect's top edge
        )
        return false;
    */

    if ( !m_rectUnion.IsOverlapped(rect) )
        return false;

    const uint iQty = (uint)m_Rects.size();
	if ( iQty <= 0 )
		return true;

    // Avoid the cost of the IsOverlapped function call and just slap the content here.
    // Also, reorder conditions to prioritize early exits for non-overlapping rectangles.
    const int left   = rect.m_left;
    const int right  = rect.m_right;
    const int top    = rect.m_top;
    const int bottom = rect.m_bottom;

    for ( uint i = 0; i < iQty; ++i )
	{
        CRect const& r = m_Rects[i];
        if (
            (right      > r.m_left) &&  // Left edge of rect is to the left of this rect's right edge
            (r.m_right  > left)     &&  // Right edge of rect is to the right of this rect's left edge
            (bottom     > r.m_top)  &&  // Top edge of rect is above this rect's bottom edge
            (r.m_bottom > top)          // Bottom edge of rect is below this rect's top edge
            )
            return true;
        //if ( rect.IsOverlapped(m_Rects[i]))
        //	return true;
	}
	return false;
}

bool CRegionBase::IsInside( const CRectMap & rect ) const
{
	ADDTOCALLSTACK("CRegionBase::IsInside");
	// NOTE: This is NOT 100% true !!

	if ( ! m_rectUnion.IsInside( rect ))
		return false;

	const size_t iQty = m_Rects.size();
	if ( iQty <= 0 )
		return true;

	for ( size_t i = 0; i < iQty; ++i )
	{
		if ( m_Rects[i].IsInside( rect ))
			return true;
	}

	return false;
}

bool CRegionBase::IsInside( const CRegionBase * pRegionTest ) const
{
	ADDTOCALLSTACK("CRegionBase::IsInside");
	// This is a rather hard test to make.
	// Is the pRegionTest completely inside this region ?

	if ( ! m_rectUnion.IsInside( pRegionTest->m_rectUnion ))
		return false;

	const size_t iQtyTest = pRegionTest->m_Rects.size();
	for ( size_t j = 0; j < iQtyTest; ++j )
	{
		if ( ! IsInside( pRegionTest->m_Rects[j] ))
			return false;
	}

	return true;
}

bool CRegionBase::IsOverlapped( const CRegionBase * pRegionTest ) const
{
	ADDTOCALLSTACK("CRegionBase::IsOverlapped");
	// Does the region overlap this rectangle.
	if ( ! m_rectUnion.IsOverlapped( pRegionTest->m_rectUnion ))
		return false;
	size_t iQty = m_Rects.size();
	size_t iQtyTest = pRegionTest->m_Rects.size();
	if ( iQty == 0 )
	{
		if ( iQtyTest == 0 )
			return true;
		return pRegionTest->IsOverlapped(m_rectUnion);
	}
	if ( iQtyTest == 0 )
	{
		return IsOverlapped(pRegionTest->m_rectUnion);
	}
	for ( size_t j = 0; j < iQty; ++j )
	{
		for ( size_t i = 0; i < iQtyTest; ++i )
		{
			if ( m_Rects[j].IsOverlapped( pRegionTest->m_Rects[i] ))
				return true;
		}
	}
	return false;
}

bool CRegionBase::IsEqualRegion( const CRegionBase * pRegionTest ) const
{
	ADDTOCALLSTACK("CRegionBase::IsEqualRegion");
	// Find dupe rectangles.
	if ( ! m_rectUnion.IsEqual( pRegionTest->m_rectUnion ))
		return false;

	const size_t iQty = m_Rects.size();
	const size_t iQtyTest = pRegionTest->m_Rects.size();
	if ( iQty != iQtyTest )
		return false;

	for ( size_t j = 0; j < iQty; ++j )
	{
		for ( size_t i = 0; ; ++i )
		{
			if ( i >= iQtyTest )
				return false;
			if ( m_Rects[j].IsEqual( pRegionTest->m_Rects[i] ))
				break;
		}
	}
	return true;
}
