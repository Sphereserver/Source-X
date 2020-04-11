/**
* @file CRect.h
* @brief Coordinates storage and operations.
*/

#ifndef _INC_CRECT_H
#define _INC_CRECT_H

#include "../game/uo_files/uofiles_enums.h"
#include "CPointBase.h"

class CRegion;
class CSector;


struct CRect		// Basic rectangle, similar to _WIN32 RECT (May not be on the map)
{
	int m_left;		// West	 x=0
	int m_top;		// North y=0
	int m_right;	// East	( NON INCLUSIVE !)
	int m_bottom;	// South ( NON INCLUSIVE !)
	int m_map;

    void SetRectEmpty();

    CRect()
    {
        SetRectEmpty();
    }
    virtual ~CRect() = default;

    inline int GetWidth() const noexcept
    {
        return( m_right - m_left );
    }
    inline int GetHeight() const noexcept
    {
        return( m_bottom - m_top );
    }

    inline bool IsRectEmpty() const noexcept
    {
        return( m_left >= m_right || m_top >= m_bottom );
    }

	void OffsetRect( int x, int y );
	void UnionPoint( int x, int y );

    inline bool IsInsideX( int x ) const
	{	// non-inclusive
		return( x >= m_left && x < m_right );
	}
    inline bool IsInsideY( int y ) const
	{	// non-inclusive
		return( y >= m_top && y < m_bottom );
	}
    inline bool IsInside( int x, int y, int map ) const
	{
		// NON inclusive rect! Is the point in the rectangle ?
		return( IsInsideX(x) &&	IsInsideY(y) && ( m_map == map ));
	}
    inline bool IsInside2d( const CPointBase & pt ) const
	{
		// NON inclusive rect! Is the point in the rectangle ?
		return( IsInside( pt.m_x, pt.m_y, pt.m_map ) );
	}

	void UnionRect( const CRect & rect );
	bool IsInside( const CRect & rect ) const;
	bool IsOverlapped( const CRect & rect ) const;
	bool IsEqual( const CRect & rect ) const;
	virtual void NormalizeRect();
    virtual void NormalizeRectMax( int cx, int cy );
    CPointBase GetCenter() const;
    CPointBase GetRectCorner( DIR_TYPE dir ) const;
    CSector * GetSector( int i ) const;	// ge all the sectors that make up this rect.
    const CRect operator += (const CRect& rect);

	void SetRect( int left, int top, int right, int bottom, int map );

	size_t Read( lpctstr pVal );
	tchar * Write( tchar * pBuffer ) const;
	lpctstr Write() const;
};

struct CRectMap : public CRect
{
    CRectMap() = default;
    virtual ~CRectMap() = default;
    CRectMap(const CRect& rect) : CRect(rect) {} // special copy constructor, valid because CRectMap hasn't additional members compared to CRect, it only has more methods

	bool IsValid() const;

	virtual void NormalizeRect();
	virtual void NormalizeRectMax();
};


#endif	// _INC_CRECT_H
