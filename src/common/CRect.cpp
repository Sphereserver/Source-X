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

CSector * CRect::GetSector( int i ) const noexcept	// ge all the sectors that make up this rect.
{
	//ADDTOCALLSTACK_DEBUG("CRect::GetSector");
	// get all the CSector(s) that overlap this rect.
	// RETURN: nullptr = no more

	// Align new rect.
    const CSectorList &pSectors = CSectorList::Get();
    const MapSectorsData& sd = pSectors.GetMapSectorData(m_map);
    const int iSectorSize = sd.iSectorSize;
    const int iSectorCols = sd.iSectorColumns;
    const uint uiSectorShift = sd.uiSectorDivShift;

    CRectMap rect;
    rect.m_left     =  m_left   & ~(iSectorSize-1);         // aligns the left boundary down to the nearest multiple of iSectorSize.
    rect.m_right    = (m_right  |  (iSectorSize-1)) + 1;    // rounds the right boundary up to cover the full last sector.
    rect.m_top      =  m_top    & ~(iSectorSize-1);
    rect.m_bottom   = (m_bottom |  (iSectorSize-1)) + 1;
    rect.m_map      = m_map;
	rect.NormalizeRectMax();

    //const int width = (rect.GetWidth()) / iSectorSize;
    //const int height = (rect.GetHeight()) / iSectorSize;
    const int width  = rect.GetWidth()  >> uiSectorShift;
    const int height = rect.GetHeight() >> uiSectorShift;

#ifdef _DEBUG
	ASSERT(width <= iSectorCols);
    const int iSectorRows = sd.iSectorRows;
	ASSERT(height <= iSectorRows);
#endif

    /*
	const int iBase = (( rect.m_top / iSectorSize) * iSectorCols) + ( rect.m_left / iSectorSize );
	if ( i >= ( height * width ))
        return i ? nullptr : pSectors->GetSector(m_map, iBase);

	const int indexoffset = (( i / width ) * iSectorCols) + ( i % width );
	return pSectors->GetSector(m_map, iBase + indexoffset);
    */

    const int baseRow = rect.m_top  >> uiSectorShift;
    const int baseCol = rect.m_left >> uiSectorShift;
    const int iBase = (baseRow * iSectorCols) + baseCol;

    if (i >= (height * width))
        return i ? nullptr : pSectors.GetSectorByIndex(m_map, iBase);

    const int indexoffset = ((i / width) * iSectorCols) + (i % width);
    return pSectors.GetSectorByIndex(m_map, iBase + indexoffset);
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

    /*
    const int iSizeX = GetWidth();
    if ( (iSizeX < 0) || (iSizeX > g_MapList.GetMapSizeX(m_map)) )
        return false;
    const int iSizeY = GetHeight();
    return !( (iSizeY < 0) || (iSizeY > g_MapList.GetMapSizeY(m_map)) );
    */

    //if ( (iSizeY < 0) || (iSizeY > g_MapList.GetMapSizeY(m_map)) )
    //    return false;
    //return true;
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
