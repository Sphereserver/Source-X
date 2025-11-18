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


    struct SectIndexingHints
    {
        int iBaseSectorIndex;       // Sector index at origin, top-left x,y coords of the rect
        int iRectWidth;             // x1 - x: how much x units (columns) are inside the rect
        int iRectHeight;            // y1 - y: how much y units (rows) are inside the rect
        int iRectSectorCount;       // How much sectors are inside this rect
        int iRectMapSectorCols;  // Number of sectors columns (X) per each row in the map (so, map max X)
    };


    void SetRectEmpty() noexcept;

    CRect() noexcept;
    CRect(int left, int top, int right, int bottom, int map) noexcept;
    constexpr CRect(const CRect&) noexcept = default;
    constexpr CRect(CRect&&) noexcept = default;
    virtual ~CRect() noexcept = default;

    CRect& operator = (const CRect&) = default;
    const CRect& operator += (const CRect& rect);

    constexpr int GetWidth() const noexcept
    {
        return( m_right - m_left );
    }
    constexpr int GetHeight() const noexcept
    {
        return( m_bottom - m_top );
    }

    constexpr bool IsRectEmpty() const noexcept
    {
        return( m_left >= m_right || m_top >= m_bottom );
    }

	void OffsetRect( int x, int y );
	void UnionPoint( int x, int y );

    constexpr bool IsInsideX( int x ) const noexcept
	{	// non-inclusive
		return( x >= m_left && x < m_right );
	}
    constexpr bool IsInsideY( int y ) const noexcept
	{	// non-inclusive
		return( y >= m_top && y < m_bottom );
	}
    inline bool IsInside2d( const CPointBase & pt ) const noexcept
	{
		// NON inclusive rect! Is the point in the rectangle ?
		return( IsInside( pt.m_x, pt.m_y, pt.m_map ) );
	}

    bool IsInside( int x, int y, int map ) const noexcept;
    bool IsInside( const CRect & rect ) const noexcept;
    void UnionRect( const CRect & rect );
	bool IsOverlapped( const CRect & rect ) const noexcept;
	bool IsEqual( const CRect & rect ) const noexcept;

	virtual void NormalizeRect() noexcept;
    void NormalizeRectMax( int cx, int cy ) noexcept;

    CPointBase GetCenter() const noexcept;
    CPointBase GetRectCorner( DIR_TYPE dir ) const;

    // get all the sectors that make up this rect.
    CSector * GetSectorAtIndex( int i ) const noexcept;

    // get all the sectors that make up this rect (using cached precomputed data, that's faster if the use case allows its usage).
    CSector * GetSectorAtIndexWithHints(int i, SectIndexingHints hints) const noexcept;
    SectIndexingHints PrecomputeSectorIndexingHints() const noexcept;

	void SetRect( int left, int top, int right, int bottom, int map ) noexcept;

	size_t Read( lpctstr pVal );
	tchar * Write( tchar * ptcBuffer, uint uiBufferLen ) const;
	lpctstr Write() const;
};

struct CRectMap : public CRect
{
    CRectMap() noexcept = default;
    CRectMap(int left, int top, int right, int bottom, int map) noexcept;
    constexpr CRectMap(const CRectMap&) noexcept = default;
    constexpr CRectMap(CRectMap&&) noexcept = default;

	// special copy constructors, valid because CRectMap hasn't additional members compared to CRect, it only has more methods
    constexpr CRectMap(const CRect& rect) noexcept : CRectMap(static_cast<const CRectMap&>(rect)) {}
    constexpr CRectMap(CRect&& rect) noexcept : CRectMap(static_cast<CRectMap&&>(rect)) {}

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
