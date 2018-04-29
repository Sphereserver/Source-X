/**
* @file CRect.h
* @brief Coordinates storage and operations.
*/

#ifndef _INC_CRECT_H
#define _INC_CRECT_H

#include "../game/uo_files/uofiles_enums.h"
#include "../game/uo_files/CUOMapList.h"
#include "../common/CLog.h"
#include "CPointBase.h"

class CRegion;
class CRegionLinks;
class CSector;


struct CRect			// Basic rectangle. (May not be on the map)
{						// Similar to _WIN32 RECT
public:
	int m_left;		// West	 x=0
	int m_top;		// North y=0
	int m_right;	// East	( NON INCLUSIVE !)
	int m_bottom;	// South ( NON INCLUSIVE !)
	int m_map;
public:
	int GetWidth() const { return( m_right - m_left ); }
	int GetHeight() const { return( m_bottom - m_top ); }

	bool IsRectEmpty() const
	{
		return( m_left >= m_right || m_top >= m_bottom );
	}
	void SetRectEmpty()
	{
		m_left = m_top = 0;	// 0x7ffe
		m_right = m_bottom = 0;
		m_map = 0;
	}

	void OffsetRect( int x, int y )
	{
		m_left += x;
		m_top += y;
		m_right += x;
		m_bottom += y;
	}
	void UnionPoint( int x, int y )
	{
		// Inflate this rect to include this point.
		// NON inclusive rect!
		if ( x	< m_left	) m_left = x;
		if ( y	< m_top		) m_top = y;
		if ( x	>= m_right	) m_right = x+1;
		if ( y	>= m_bottom	) m_bottom = y+1;
	}

	bool IsInsideX( int x ) const
	{	// non-inclusive
		return( x >= m_left && x < m_right );
	}
	bool IsInsideY( int y ) const
	{	// non-inclusive
		return( y >= m_top && y < m_bottom );
	}
	bool IsInside( int x, int y, int map ) const
	{
		// NON inclusive rect! Is the point in the rectangle ?
		return( IsInsideX(x) &&	IsInsideY(y) && ( m_map == map ));
	}
	bool IsInside2d( const CPointBase & pt ) const
	{
		// NON inclusive rect! Is the point in the rectangle ?
		return( IsInside( pt.m_x, pt.m_y, pt.m_map ) );
	}

	void UnionRect( const CRect & rect )
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
	bool IsInside( const CRect & rect ) const
	{
		// Is &rect inside me ?
		// ASSUME: Normalized rect
		if ( rect.m_map != m_map )
			return false;
		if ( rect.m_left	< m_left	)
			return false;
		if ( rect.m_top		< m_top		)
			return false;
		if ( rect.m_right	> m_right	)
			return false;
		if ( rect.m_bottom	> m_bottom	)
			return false;
		return true;
	}
	bool IsOverlapped( const CRect & rect ) const
	{
		// are the 2 rects overlapped at all ?
		// NON inclusive rect.
		// ASSUME: Normalized rect
		//		if ( rect.m_map != m_map ) return false;
		if ( rect.m_left	>= m_right	)
			return false;
		if ( rect.m_top		>= m_bottom	)
			return false;
		if ( rect.m_right	<= m_left	)
			return false;
		if ( rect.m_bottom	<= m_top	)
			return false;
		return true;
	}
	bool IsEqual( const CRect & rect ) const
	{
		return m_left == rect.m_left &&
			m_top == rect.m_top &&
			m_right == rect.m_right &&
			m_bottom == rect.m_bottom &&
			m_map == rect.m_map;
	}
	virtual void NormalizeRect()
	{
		if ( m_bottom < m_top )
		{
			int wtmp = m_bottom;
			m_bottom = m_top;
			m_top = wtmp;
		}
		if ( m_right < m_left )
		{
			int wtmp = m_right;
			m_right = m_left;
			m_left = wtmp;
		}
		if (( m_map < 0 ) || ( m_map >= 256 )) m_map = 0;
		if ( !g_MapList.m_maps[m_map] ) m_map = 0;
	}

	void SetRect( int left, int top, int right, int bottom, int map )
	{
		m_left = left;
		m_top = top;
		m_right = right;
		m_bottom = bottom;
		m_map = map;
		NormalizeRect();
	}

	virtual void NormalizeRectMax( int cx, int cy )
	{
		if ( m_left < 0 )
			m_left = 0;
		if ( m_top < 0 )
			m_top = 0;
		if ( m_right > cx )
			m_right = cx;
		if ( m_bottom > cy )
			m_bottom = cy;
	}

	size_t Read( lpctstr pVal );
	tchar * Write( tchar * pBuffer ) const
	{
		sprintf(pBuffer, "%d,%d,%d,%d,%d", m_left, m_top, m_right, m_bottom, m_map);
		return( pBuffer );
	}
	lpctstr Write() const;

	CPointBase GetCenter() const
	{
		CPointBase pt;
		pt.m_x = (short)(( m_left + m_right ) / 2);
		pt.m_y = (short)((m_top + m_bottom) / 2);
		pt.m_z = 0;
		pt.m_map = (uchar)(m_map);
		return( pt );
	}

	CPointBase GetRectCorner( DIR_TYPE dir ) const;
	CSector * GetSector( int i ) const;	// ge all the sectors that make up this rect.
};

struct CRectMap : public CRect
{
public:

	bool IsValid() const
	{
		int iSizeX = GetWidth();
		if ( iSizeX < 0 || iSizeX > g_MapList.GetX(m_map) )
			return false;
		int iSizeY = GetHeight();
		if ( iSizeY < 0 || iSizeY > g_MapList.GetY(m_map) )
			return false;
		return true;
	}

	virtual void NormalizeRect();
	virtual void NormalizeRectMax();
};


#endif	// _INC_CRECT_H
