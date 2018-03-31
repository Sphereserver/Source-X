#include "../game/items/CItem.h"
#include "../game/CSector.h"
#include "../game/CServer.h"
#include "../game/CWorld.h"
#include "CUIDExtra.h"
#include "CRect.h"


//*************************************************************************
// -CRect

size_t CRect::Read( lpctstr pszVal )
{
	ADDTOCALLSTACK("CRect::Read");
	// parse reading the rectangle
	tchar *pszTemp = Str_GetTemp();
	strcpy( pszTemp, pszVal );
	tchar * ppVal[5];
	size_t i = Str_ParseCmds( pszTemp, ppVal, CountOf( ppVal ), " ,\t");
	switch (i)
	{
		case 5:
			m_map = ATOI(ppVal[4]);
			if (( m_map < 0 ) || ( m_map >= 256 ) || !g_MapList.m_maps[m_map] )
			{
				g_Log.EventError("Unsupported map #%d specified. Auto-fixing that to 0.\n", m_map);
				m_map = 0;
			}
			m_bottom = ATOI(ppVal[3]);
			m_right = ATOI(ppVal[2]);
			m_top =	ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 4:
			m_map = 0;
			m_bottom = ATOI(ppVal[3]);
			m_right = ATOI(ppVal[2]);
			m_top =	ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 3:
			m_map = 0;
			m_bottom = 0;
			m_right = ATOI(ppVal[2]);
			m_top =	ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 2:
			m_map = 0;
			m_bottom = 0;
			m_right = 0;
			m_top =	ATOI(ppVal[1]);
			m_left = ATOI(ppVal[0]);
			break;
		case 1:
			m_map = 0;
			m_bottom = 0;
			m_right = 0;
			m_top = 0;
			m_left = ATOI(ppVal[0]);
			break;
	}
	NormalizeRect();
	return i;
}

lpctstr CRect::Write() const
{
	ADDTOCALLSTACK("CRect::Write");
	return Write( Str_GetTemp() );
}

CPointBase CRect::GetRectCorner( DIR_TYPE dir ) const
{
	ADDTOCALLSTACK("CRect::GetRectCorner");
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
	return( pt );
}

CSector * CRect::GetSector( int i ) const	// ge all the sectors that make up this rect.
{
	ADDTOCALLSTACK("CRect::GetSector");
	// get all the CSector(s) that overlap this rect.
	// RETURN: NULL = no more

	// Align new rect.
	CRectMap rect;
	rect.m_left = m_left &~ (g_MapList.GetSectorSize(m_map)-1);
	rect.m_right = ( m_right | (g_MapList.GetSectorSize(m_map)-1)) + 1;
	rect.m_top = m_top &~ (g_MapList.GetSectorSize(m_map)-1);
	rect.m_bottom = ( m_bottom | (g_MapList.GetSectorSize(m_map)-1)) + 1;
	rect.m_map = m_map;
	rect.NormalizeRectMax();

	int width = (rect.GetWidth()) / g_MapList.GetSectorSize(m_map);
	ASSERT(width <= g_MapList.GetSectorCols(m_map));
	int height = (rect.GetHeight()) / g_MapList.GetSectorSize(m_map);
	ASSERT(height <= g_MapList.GetSectorRows(m_map));

	int iBase = (( rect.m_top / g_MapList.GetSectorSize(m_map)) * g_MapList.GetSectorCols(m_map)) + ( rect.m_left / g_MapList.GetSectorSize(m_map) );

	if ( i >= ( height * width ))
	{
		if ( ! i )
			return g_World.GetSector(m_map, iBase);
		return NULL;
	}

	int indexoffset = (( i / width ) * g_MapList.GetSectorCols(m_map)) + ( i % width );

	return g_World.GetSector(m_map, iBase+indexoffset);
}


//CPointMap::CPointMap()
//{
//}

CPointMap::CPointMap( short x, short y, char z, uchar map )
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_map = map;
}

CPointMap & CPointMap::operator= ( const CPointBase & pt )
{
	Set( pt );
	return ( * this );
}

CPointMap::CPointMap( const CPointBase & pt )
{
	Set( pt );
}

CPointMap::CPointMap( tchar * pVal )
{
	Read( pVal );
}

CPointSort::CPointSort()
{
	InitPoint();
}

CPointSort::CPointSort( word x, word y, char z, uchar map )
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_map = map;
}

CPointSort::CPointSort( const CPointBase & pt )
{
	Set( pt );
}

CPointSort::~CPointSort()	// just to make this dynamic
{
}

//*************************************************************************
// -CRectMap

void CRectMap::NormalizeRect()
{
	ADDTOCALLSTACK("CRectMap::NormalizeRect");
	CRect::NormalizeRect();
	NormalizeRectMax();
}

void CRectMap::NormalizeRectMax()
{
	ADDTOCALLSTACK("CRectMap::NormalizeRectMax");
	CRect::NormalizeRectMax( g_MapList.GetX(m_map), g_MapList.GetY(m_map));
}
