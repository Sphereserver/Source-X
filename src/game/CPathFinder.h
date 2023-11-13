//
//	CPathFinder
//		pathfinding algorithm based on AStar (A*) algorithm
//		based on A* Pathfinder (Version 1.71a) by Patrick Lester, pwlester@policyalmanac.org
//

#ifndef _INC_PATHFINDER_H
#define _INC_PATHFINDER_H

#include <deque>
#include "../common/sphere_library/ssorted_vector.h"
#include "../common/CPointBase.h"
#include "uo_files/uofiles_macros.h"

//	number of steps to remember for pathfinding, default to 28 steps (one screen, both sides), will have 28*4 extra bytes per char
#define MAX_NPC_PATH_STORAGE_SIZE	(UO_MAP_VIEW_SIGHT*2)


class CChar;

class CPathFinderPoint : public CPointMap
{
public:
	CPathFinderPoint* _Parent;
	bool _Walkable;
	int _FValue;
	int _GValue;
	int _HValue;

public:
	CPathFinderPoint();

    // We shouldn't be using these
	explicit CPathFinderPoint(const CPointMap& pt) = delete;
    void operator = (const CPointMap& copy) = delete;

private:
	CPathFinderPoint(const CPathFinderPoint& copy);
	CPathFinderPoint& operator=(const CPathFinderPoint& other);

public:
    inline bool operator < (const CPathFinderPoint& pt) const noexcept
    {
        return (_FValue < pt._FValue);
    }
};


class CPathFinder
{
public:
	static const char *m_sClassName;

	CPathFinder(CChar *pChar, const CPointMap& ptTarget);
	~CPathFinder() = default;

private:
	CPathFinder(const CPathFinder& copy);
	CPathFinder& operator=(const CPathFinder& other);

public:
    bool FindPath();
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
	CPathFinderPoint m_Points[MAX_NPC_PATH_STORAGE_SIZE][MAX_NPC_PATH_STORAGE_SIZE];
	std::deque <CPathFinderPoint*>    m_Opened; // push/pop front/back
	sl::sorted_vector<CPathFinderPoint*> m_Closed; // i'm only searching (a lot) and pushing_back here

	std::deque<CPointMap> m_LastPath;

	short m_RealX;
	short m_RealY;

	CChar *m_pChar;
	CPointMap m_Target;

protected:
    static int Heuristic(const CPathFinderPoint* Pt1, const CPathFinderPoint* Pt2) noexcept;

	void Clear();
	void GetAdjacentCells(const CPathFinderPoint* Point, std::deque<CPathFinderPoint*>& ChildrenRefList );
	void FillMap();	// prepares map with walkable statuses
};



#endif // _INC_PATHFINDER_H
