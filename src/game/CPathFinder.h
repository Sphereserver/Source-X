//
//	CPathFinder
//		pathfinding algorithm based on AStar (A*) algorithm
//		based on A* Pathfinder (Version 1.71a) by Patrick Lester, pwlester@policyalmanac.org
//

#ifndef _INC_PATHFINDER_H
#define _INC_PATHFINDER_H

#include <deque>
#include "../common/CPointBase.h"
#include "uo_files/uofiles_macros.h"


#define PATH_SIZE (UO_MAP_VIEW_SIGHT*2)	// limit NPC view by one screen (both sides)

class CChar;

class CPathFinderPoint : public CPointMap
{
public:
	CPathFinderPoint* m_Parent;
	bool m_Walkable;
	int FValue;
	int GValue;
	int HValue;

public:
	CPathFinderPoint();
	explicit CPathFinderPoint(const CPointMap& pt);

private:
	CPathFinderPoint(const CPathFinderPoint& copy);
	CPathFinderPoint& operator=(const CPathFinderPoint& other);

public:
    inline bool operator < (const CPathFinderPoint& pt) const noexcept
    {
        return (FValue < pt.FValue);
    }
};


class CPathFinder
{
public:
	static const char *m_sClassName;

	#define PATH_NONEXISTENT 0
	#define PATH_FOUND 1


	#define PATH_WALKABLE 1
	#define PATH_UNWALKABLE 0

	CPathFinder(CChar *pChar, CPointMap ptTarget);
	~CPathFinder() = default;

private:
	CPathFinder(const CPathFinder& copy);
	CPathFinder& operator=(const CPathFinder& other);

public:
    int FindPath();
    size_t LastPathSize() const noexcept
    {
        return m_LastPath.size();
    }
    void ClearLastPath() noexcept
    {
        m_LastPath.clear();
    }
    inline const CPointMap& ReadStep(size_t Step = 0) const
    {
        return m_LastPath[Step];
    }

protected:
	CPathFinderPoint m_Points[PATH_SIZE][PATH_SIZE];
	std::deque<CPathFinderPoint*> m_Opened;
	std::deque<CPathFinderPoint*> m_Closed;

	std::deque<CPointMap> m_LastPath;

	int m_RealX;
	int m_RealY;

	CChar *m_pChar;
	CPointMap m_Target;

protected:
    static uint Heuristic(const CPathFinderPoint* Pt1, const CPathFinderPoint* Pt2) noexcept;

	void Clear();
	void GetChildren(const CPathFinderPoint* Point, std::deque<CPathFinderPoint*>& ChildrenRefList );
	void FillMap();	// prepares map with walkable statuses
};



#endif // _INC_PATHFINDER_H
