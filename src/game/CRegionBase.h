/**
* @file CRegionBase.h
* @brief 
*/

#ifndef _INC_CREGIONBASE_H
#define _INC_CREGIONBASE_H

#include "../common/sphere_library/CSTypedArray.h"
#include "../common/CRect.h"


class CRegionBase
{
	// A bunch of rectangles forming an area.
public:
	static const char *m_sClassName;

	CRectMap m_rectUnion;	// The union rectangle.
	CSTypedArray<CRectMap> m_Rects;

	bool IsRegionEmpty() const
	{
		return m_rectUnion.IsRectEmpty();
	}
	void EmptyRegion()
	{
		m_rectUnion.SetRectEmpty();
        m_Rects.clear();
	}
	size_t GetRegionRectCount() const;
    CRectMap & GetRegionRect(size_t i);
	const CRectMap & GetRegionRect(size_t i) const;
	virtual bool AddRegionRect( const CRectMap & rect );

    CPointMap GetRegionCorner( DIR_TYPE dir = DIR_QTY ) const;
	bool IsInside2d( const CPointMap & pt ) const;

	bool IsOverlapped( const CRectMap & rect ) const noexcept;
	bool IsInside( const CRectMap & rect ) const;

	bool IsInside( const CRegionBase * pRegionIsSmaller ) const;
	bool IsOverlapped( const CRegionBase * pRegionTest ) const;
	bool IsEqualRegion( const CRegionBase * pRegionTest ) const;

	inline CSector * GetSector( int i ) const // get all the sectors that make up this rect.
	{
		return m_rectUnion.GetSector(i);
	}

public:
	CRegionBase();
	virtual ~CRegionBase() = default;

private:
	CRegionBase(const CRegionBase& copy);
	CRegionBase& operator=(const CRegionBase& other);
};


#endif // _INC_CREGIONBASE_H
