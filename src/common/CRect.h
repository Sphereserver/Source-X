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

    void SetRectEmpty() noexcept;

	CRect() noexcept;
	CRect(int left, int top, int right, int bottom, int map) noexcept;
	CRect(const CRect&) noexcept = default;
	CRect(CRect&&) noexcept = default;
    virtual ~CRect() noexcept = default;

	CRect& operator = (const CRect&) = default;
	const CRect& operator += (const CRect& rect);

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
	bool IsInside( const CRect & rect ) const noexcept;
	bool IsOverlapped( const CRect & rect ) const noexcept;
	bool IsEqual( const CRect & rect ) const noexcept;

	virtual void NormalizeRect() noexcept;
    void NormalizeRectMax( int cx, int cy ) noexcept;

    CPointBase GetCenter() const;
    CPointBase GetRectCorner( DIR_TYPE dir ) const;
    CSector * GetSector( int i ) const noexcept;	// ge all the sectors that make up this rect.

	void SetRect( int left, int top, int right, int bottom, int map ) noexcept;

	size_t Read( lpctstr pVal );
	tchar * Write( tchar * ptcBuffer, uint uiBufferLen ) const;
	lpctstr Write() const;
};

struct CRectMap : public CRect
{
    CRectMap() noexcept = default;
	CRectMap(int left, int top, int right, int bottom, int map) noexcept;
	CRectMap(const CRectMap&) noexcept = default;
	CRectMap(CRectMap&&) noexcept = default;

	// special copy constructors, valid because CRectMap hasn't additional members compared to CRect, it only has more methods
	CRectMap(const CRect& rect) noexcept : CRectMap(static_cast<const CRectMap&>(rect)) {}
	CRectMap(CRect&& rect) noexcept : CRectMap(static_cast<CRectMap&&>(rect)) {}

	CRectMap& operator=(const CRectMap&) noexcept = default;
	CRectMap& operator=(const CRect& rect) noexcept
	{
		return CRectMap::operator=(static_cast<const CRectMap&>(rect));
	}

    virtual ~CRectMap() noexcept = default;

	bool IsValid() const noexcept;

	virtual void NormalizeRect() noexcept override;
	void NormalizeRectMax() noexcept;
};


#endif	// _INC_CRECT_H
