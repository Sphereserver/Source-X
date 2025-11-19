#include "../game/uo_files/CUOMapList.h"
#include "../game/CSectorList.h"
#include "sphere_library/sfastmath.h"
#include "CLog.h"
#include "CRect.h"


//*************************************************************************
// -CRect

CRect::CRect() noexcept :
	m_left(0), m_top(0), m_right(0), m_bottom(0), m_map(0)
{
}

CRect::CRect(int left, int top, int right, int bottom, int map) noexcept :
	m_left(left), m_top(top), m_right(right), m_bottom(bottom), m_map(map)
{
}

void CRect::SetRectEmpty() noexcept
{
	/*
    m_left = m_top = 0;	// 0x7ffe
    m_right = m_bottom = 0;
    m_map = 0;
	*/
	m_left = m_top = m_right = m_bottom = m_map = 0;
}

void CRect::OffsetRect( int x, int y )
{
    m_left += x;
    m_top += y;
    m_right += x;
    m_bottom += y;
}

void CRect::UnionPoint( int x, int y )
{
    // Inflate this rect to include this point.
    // NON inclusive rect!
    if ( x	< m_left	) m_left = x;
    if ( y	< m_top		) m_top = y;
    if ( x	>= m_right	) m_right = x+1;
    if ( y	>= m_bottom	) m_bottom = y+1;
}

void CRect::UnionRect( const CRect & rect )
{
    // Inflate this rect to include both rectangles.
    // ASSUME: Normalized rect
    if ( rect.IsRectEmpty())
        return;
    if ( IsRectEmpty())
    {
        *this = rect;
        return;
    }
    if ( rect.m_left	< m_left	) m_left = rect.m_left;
    if ( rect.m_top		< m_top		) m_top = rect.m_top;
    if ( rect.m_right	> m_right	) m_right = rect.m_right;
    if ( rect.m_bottom	> m_bottom	) m_bottom = rect.m_bottom;
    if ( m_map != rect.m_map )
    {
        DEBUG_ERR(("Uniting regions from different maps!\n"));
    }
}

bool CRect::IsInside( int x, int y, int map ) const noexcept
{
    // NON inclusive rect! Is the point in the rectangle ?
    return( IsInsideX(x) &&	IsInsideY(y) && ( m_map == map ));
}

bool CRect::IsInside(const CRect & rect) const noexcept
{
    // Is &rect inside me ?
    // ASSUME: Normalized rect
    return (rect.m_map  == m_map)  &&
           (rect.m_left >= m_left) &&
           (rect.m_top  >= m_top)  &&
           (rect.m_right <= m_right)&&
           (rect.m_bottom<= m_bottom);
}

bool CRect::IsOverlapped(const CRect & rect) const noexcept
{
    // are the 2 rects overlapped at all ?
    // NON inclusive rect.
    // ASSUME: Normalized rect
    return (rect.m_left  < m_right)  &&
           (rect.m_top   < m_bottom) &&
           (rect.m_right > m_left)   &&
           (rect.m_bottom> m_top);
}

bool CRect::IsEqual( const CRect & rect ) const noexcept
{
    return  m_left   == rect.m_left     &&
            m_top    == rect.m_top      &&
            m_right  == rect.m_right    &&
            m_bottom == rect.m_bottom   &&
            m_map    == rect.m_map;
}

void CRect::NormalizeRect() noexcept
{
    sl::fmath::sSortPair(m_top, m_bottom);
    sl::fmath::sSortPair(m_left, m_right);
    m_map = g_MapList.IsMapSupported(m_map) ? m_map : 0;

/*
    if ( m_bottom < m_top )
    {
        const int wtmp = m_bottom;
        m_bottom = m_top;
        m_top = wtmp;
    }
    if ( m_right < m_left )
    {
        const int wtmp = m_right;
        m_right = m_left;
        m_left = wtmp;
    }
    if ( !g_MapList.IsMapSupported(m_map) )
        m_map = 0;
*/
}

void CRect::SetRect( int left, int top, int right, int bottom, int map ) noexcept
{
    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
    m_map = map;
    NormalizeRect();
}

void CRect::NormalizeRectMax( int cx, int cy ) noexcept
{
    m_left      = sl::fmath::sMax(0, m_left);
    m_top       = sl::fmath::sMax(0, m_top);
    m_right     = sl::fmath::sMin(cx, m_right);
    m_bottom    = sl::fmath::sMin(cy, m_bottom);

    /*
    if ( m_left < 0 )
        m_left = 0;
    if ( m_top < 0 )
        m_top = 0;
    if ( m_right > cx )
        m_right = cx;
    if ( m_bottom > cy )
        m_bottom = cy;
    */
}

size_t CRect::Read( lpctstr pszVal )
{
	ADDTOCALLSTACK("CRect::Read");
	// parse reading the rectangle
	tchar *ptcTemp = Str_GetTemp();
	Str_CopyLimitNull(ptcTemp, pszVal, Str_TempLength());
	tchar * ppVal[5];
	size_t i = Str_ParseCmds(ptcTemp, ppVal, ARRAY_COUNT( ppVal ), " ,\t");
	switch (i)
	{
		case 5:
			m_map = atoi(ppVal[4]);
			if (( m_map < 0 ) || ( m_map >= MAP_SUPPORTED_QTY) || !g_MapList.IsMapSupported(m_map) )
			{
				g_Log.EventError("Unsupported map #%d specified. Auto-fixing that to 0.\n", m_map);
				m_map = 0;
			}
			m_bottom = atoi(ppVal[3]);
			m_right = atoi(ppVal[2]);
			m_top =	atoi(ppVal[1]);
			m_left = atoi(ppVal[0]);
			break;
		case 4:
			m_map = 0;
			m_bottom = atoi(ppVal[3]);
			m_right = atoi(ppVal[2]);
			m_top =	atoi(ppVal[1]);
			m_left = atoi(ppVal[0]);
			break;
		case 3:
			m_map = 0;
			m_bottom = 0;
			m_right = atoi(ppVal[2]);
			m_top =	atoi(ppVal[1]);
			m_left = atoi(ppVal[0]);
			break;
		case 2:
			m_map = 0;
			m_bottom = 0;
			m_right = 0;
			m_top =	atoi(ppVal[1]);
			m_left = atoi(ppVal[0]);
			break;
		case 1:
			m_map = 0;
			m_bottom = 0;
			m_right = 0;
			m_top = 0;
			m_left = atoi(ppVal[0]);
			break;
	}
	NormalizeRect();
	return i;
}

tchar * CRect::Write( tchar * pBuffer, uint uiBufferLen) const
{
    snprintf(pBuffer, uiBufferLen, "%d,%d,%d,%d,%d", m_left, m_top, m_right, m_bottom, m_map);
    return pBuffer;
}

lpctstr CRect::Write() const
{
	ADDTOCALLSTACK("CRect::Write");
	return Write( Str_GetTemp(), (uint)Str_TempLength() );
}

CPointBase CRect::GetCenter() const noexcept
{
    return CPointBase(
        (short)((m_left + m_right) / 2),
        (short)((m_top + m_bottom) / 2),
        0,
        (uchar)(m_map)
        );
}

CPointBase CRect::GetRectCorner( DIR_TYPE dir ) const
{
	ADDTOCALLSTACK("CRect::GetRectCorner");
	ASSERT(dir <= DIR_QTY);
	// Get the point if a directional corner of the CRectMap.

	CPointBase pt;
	pt.m_z = 0;	// NOTE: remember this is a nonsense value.
	pt.m_map = (uchar)m_map;
	switch ( dir )
	{
		case DIR_N:
			pt.m_x = (short)((m_left + m_right) / 2);
			pt.m_y = (short)m_top;
			break;
		case DIR_NE:
			pt.m_x = (short)m_right;
			pt.m_y = (short)m_top;
			break;
		case DIR_E:
			pt.m_x = (short)m_right;
			pt.m_y = (short)((m_top + m_bottom) / 2);
			break;
		case DIR_SE:
			pt.m_x = (short)m_right;
			pt.m_y = (short)m_bottom;
			break;
		case DIR_S:
			pt.m_x = (short)((m_left + m_right) / 2);
			pt.m_y = (short)m_bottom;
			break;
		case DIR_SW:
			pt.m_x = (short)m_left;
			pt.m_y = (short)m_bottom;
			break;
		case DIR_W:
			pt.m_x = (short)m_left;
			pt.m_y = (short)((m_top + m_bottom) / 2);
			break;
		case DIR_NW:
			pt.m_x = (short)m_left;
			pt.m_y = (short)m_top;
			break;
		case DIR_QTY:
			pt = GetCenter();
			break;
		default:
			break;
	}
	return pt;
}

// get the n-th sector that makes up this rect.
CSector * CRect::GetSectorAtIndex( int i ) const noexcept
{
    //ADDTOCALLSTACK_DEBUG("CRect::GetSectorAtIndexWithHints");
    // get all the CSector(s) that overlap this rect.
    // RETURN: nullptr = no more

    if (g_MapList.IsMapSupported(m_map) == false)
        return nullptr;

    SectIndexingHints_s hints = PrecomputeSectorIndexingHints();
    return GetSectorAtIndexWithHints(i, hints);
}

// get the n-th sector that makes up this rect.
CSector * CRect::GetSectorAtIndexWithHints(int i, SectIndexingHints_s hints) const noexcept
{
    //if (g_MapList.IsMapSupported(m_map) == false)
    //    return nullptr;

    const CSectorList &pSectors = CSectorList::Get();

    if (i >= hints.iRectSectorCount)
        return i ? nullptr : pSectors.GetSectorByIndexUnchecked(m_map, hints.iBaseSectorIndex);

    // From now on we need only height and sectorcols. We'll use precomputed data when checking
    //  incremental sector indices for the same CRect.

    // Column-major
    //    col = i / height
    //    row = i % height
    //    indexoffset = col * sectorRows + row
    // Row-major
    const int row = i / hints.iRectWidth;
    const int col = i % hints.iRectWidth;
    const int iSectorIndexOffset = CSectorList::CalcSectorIndex(hints.iRectMapSectorCols, col, row);
    //const int iSectorIndexOffset = row * hints.iRectMapSectorCols + col;
    const int iSectorIndex = hints.iBaseSectorIndex + iSectorIndexOffset;
    return pSectors.GetSectorByIndexUnchecked(m_map, iSectorIndex);
}

CRect::SectIndexingHints_s
CRect::PrecomputeSectorIndexingHints() const noexcept
{
    const CSectorList &pSectors = CSectorList::Get();
    const MapSectorsData* pSecData = pSectors.GetMapSectorData(m_map);
    //ASSERT(pSecData);

    const int iSectorSize = pSecData->iSectorSize;
    const int iSectorCols = pSecData->iSectorColumns;
    const uint uiSectorShift = pSecData->uiSectorSizeDivShift;

    // Align new rect.
    // CRectMap(int left, int top, int right, int bottom, int map)
    CRectMap rect(
        (m_left   & ~(iSectorSize-1)),        // aligns the left boundary down to the nearest multiple of iSectorSize.
        (m_top    & ~(iSectorSize-1)),
        (m_right  |  (iSectorSize-1)) + 1,    // rounds the right boundary up to cover the full last sector.
        (m_bottom |  (iSectorSize-1)) + 1,
        m_map
        );
    //g_Log.EventDebug("Precomputing hints. Starting rect: %d,%d, %d,%d. Aligned rect: %d,%d, %d,%d.\n",
    //    m_left, m_top, m_right, m_bottom, rect.m_left, rect.m_top, rect.m_right, rect.m_bottom);
    rect.NormalizeRectMax();

    //const int width = (rect.GetWidth()) / iSectorSize;
    //const int height = (rect.GetHeight()) / iSectorSize;
    // Bit-shifting is faster than plain division. We can use it because sector size is guaranteed to be a multiple of 2.
    const int width  = rect.GetWidth()  >> uiSectorShift;
    const int height = rect.GetHeight() >> uiSectorShift;

#ifdef _DEBUG
    const int iSectorRows = pSecData->iSectorRows;
    ASSERT(width <= iSectorCols);
    ASSERT(height <= iSectorRows);
#endif

    const int baseRow = rect.m_top  >> uiSectorShift;
    const int baseCol = rect.m_left >> uiSectorShift;
    const int iBase = CSectorList::CalcSectorIndex(iSectorCols, baseCol, baseRow);

    return SectIndexingHints_s {
        .iBaseSectorIndex = iBase,
        .iRectWidth = width,
        .iRectHeight = height,
        .iRectSectorCount = width * height,
        .iRectMapSectorCols = iSectorCols
    };
}


const CRect& CRect::operator += (const CRect& rect)
{
    m_top += rect.m_top;
    m_bottom += rect.m_bottom;
    m_right += rect.m_right;
    m_left += rect.m_left;
    return *this;
}

//*************************************************************************
// -CRectMap

CRectMap::CRectMap(int left, int top, int right, int bottom, int map) noexcept :
	CRect(left, top, right, bottom, map)
{
}

bool CRectMap::IsValid() const noexcept
{
    const int iSizeX = GetWidth();
    const int iSizeY = GetHeight();
    return (iSizeX >= 0 && iSizeX <= g_MapList.GetMapSizeX(m_map)) &&
           (iSizeY >= 0 && iSizeY <= g_MapList.GetMapSizeY(m_map));
}

void CRectMap::NormalizeRect() noexcept
{
	//ADDTOCALLSTACK_DEBUG("CRectMap::NormalizeRect");
	CRect::NormalizeRect();
	NormalizeRectMax();
}

void CRectMap::NormalizeRectMax() noexcept
{
    //ADDTOCALLSTACK_DEBUG("CRectMap::NormalizeRectMax");
	CRect::NormalizeRectMax( g_MapList.GetMapSizeX(m_map), g_MapList.GetMapSizeY(m_map));
}
