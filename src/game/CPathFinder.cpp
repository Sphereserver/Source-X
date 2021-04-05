
#include "../common/CException.h"
#include "../sphere/threads.h"
#include "chars/CChar.h"
#include "CPathFinder.h"
#include <algorithm>


int CPathFinder::Heuristic(const CPathFinderPoint* Pt1, const CPathFinderPoint* Pt2) noexcept // static
{
    // Hexagonal heuristic (thought for a hexagonal grid, but in our case, by using this, the movements are more natural and the rotation angles more wide)
	return 10*(abs(Pt1->m_x - Pt2->m_x) + abs(Pt1->m_y - Pt2->m_y));

    // Diagonal heuristic, thought for a square grid which allows movement in 8 directions from a cell (our case)
    //return std::max(abs(Pt1->m_x - Pt2->m_x), abs(Pt1->m_y - Pt2->m_y));
}

void CPathFinder::GetAdjacentCells(const CPathFinderPoint* Point, std::deque<CPathFinderPoint*>& AdjacentCellsRefList )
{
	for (short x = -1; x != 2; ++x )
	{
		for (short y = -1; y != 2; ++y )
		{
			if ( x == 0 && y == 0 )
				continue;
            const short RealX = x + Point->m_x;
            const short RealY = y + Point->m_y;
			if ( RealX < 0 || RealY < 0 || RealX >= (MAX_NPC_PATH_STORAGE_SIZE - 1) || (RealY >= MAX_NPC_PATH_STORAGE_SIZE - 1))
				continue;
			if ( m_Points[RealX][RealY]._Walkable == false )
				continue;
			if ( x != 0 && y != 0 ) // Diagonal
			{
				if ( m_Points[RealX - x][RealY]._Walkable == false || m_Points[RealX][RealY - y]._Walkable == false )
					continue;
			}

			AdjacentCellsRefList.emplace_back( &m_Points[RealX][RealY] );
		}
	}
}

CPathFinderPoint::CPathFinderPoint() :
	CPointMap(0, 0, 0, 0),
    _Parent(nullptr), _Walkable(false), _FValue(0), _GValue(0), _HValue(0)
{
}

/*
CPathFinderPoint::CPathFinderPoint(const CPointMap& pt) : 
    _Parent(nullptr), _Walkable(false), _FValue(0), _GValue(0), _HValue(0)
{
	//ADDTOCALLSTACK("CPathFinderPoint::CPathFinderPoint");
	m_x = pt.m_x;
	m_y = pt.m_y;
	m_z = pt.m_z;
	m_map = pt.m_map;
}

void CPathFinderPoint::operator = (const CPointMap& copy)
{
    //ADDTOCALLSTACK("CPathFinderPoint::operator=");
    _Parent = nullptr;
    _Walkable = false;
    _FValue = _GValue = _HValue = 0;
    CPointMap::operator=(copy);
}
*/

CPathFinder::CPathFinder(CChar *pChar, const CPointMap& ptTarget) :
	m_Points{ {} }
{
	ADDTOCALLSTACK("CPathFinder::CPathFinder");
	EXC_TRY("CPathFinder Constructor");

	m_pChar = pChar;
    m_Target = ptTarget;

    const CPointMap& pt = m_pChar->GetTopPoint();
    m_RealX = pt.m_x - (MAX_NPC_PATH_STORAGE_SIZE / 2);
    m_RealY = pt.m_y - (MAX_NPC_PATH_STORAGE_SIZE / 2);

	m_Target.m_x -= m_RealX;
	m_Target.m_y -= m_RealY;

	FillMap();
	EXC_CATCH;
}

inline static bool CPathFinderPointPtrCompLess(const CPathFinderPoint* Pt1, const CPathFinderPoint* Pt2) noexcept
{
    return *Pt1 < *Pt2;
}

bool CPathFinder::FindPath() //A* algorithm
{
	ADDTOCALLSTACK("CPathFinder::FindPath");
	ASSERT(m_pChar != nullptr);

    const CPointMap& ptTop = m_pChar->GetTopPoint();
	const short X = ptTop.m_x - m_RealX;
	const short Y = ptTop.m_y - m_RealY;

	if ( X < 0 || Y < 0 || X >= MAX_NPC_PATH_STORAGE_SIZE || Y >= MAX_NPC_PATH_STORAGE_SIZE)
	{
		//Too far away
		Clear();
		return false; // path not existent
	}

	CPathFinderPoint* Start = &m_Points[X][Y]; //Start point
	CPathFinderPoint* End =   &m_Points[m_Target.m_x][m_Target.m_y]; //End Point

	ASSERT(Start);
	ASSERT(End);

	Start->_Walkable = false;
	Start->_GValue = 0;
    Start->_HValue = Heuristic(Start, End);
    Start->_FValue = Start->_HValue;

    std::deque<CPathFinderPoint*> AdjacentCells;
	m_Opened.emplace_back( Start );
	while ( !m_Opened.empty() )
	{
        // Sort open (explorable) points by FValue
        std::sort(m_Opened.begin(), m_Opened.end(), CPathFinderPointPtrCompLess);

        // Take the point with the lowest FValue
        CPathFinderPoint *Current = *m_Opened.begin();
		
		if ( Current == End )
		{
            // Arrived to destination: reconstruct path and save it
			while (Current->_Parent)
			{
                Current = Current->_Parent;
				m_LastPath.emplace_front(short(Current->m_x + m_RealX), short(Current->m_y + m_RealY), char(0), Current->m_map);
			}
			Clear();
			return true; // path found
		}

        m_Opened.pop_front();

        // Closed points are stored automatically in a sorted order, for faster search
        m_Closed.emplace(Current);
        //m_Closed.emplace_back(Current);

        AdjacentCells.clear();
        GetAdjacentCells(Current, AdjacentCells);

        while (!AdjacentCells.empty())
        {
            CPathFinderPoint* Cell = AdjacentCells.front();
            AdjacentCells.pop_front();

            // Linear search
            /*
            const auto InClosed = std::find(m_Closed.cbegin(), m_Closed.cend(), Cell);
            if (InClosed != m_Closed.cend())
                continue;
             */
            // Binary (pre-sorted) search in our CSSortedVector
            if (m_Closed.find(Cell) != SCONT_BADINDEX)
                continue;            

            ASSERT(Cell->_Walkable);
            // Linear search
            //const auto InOpened = std::find(m_Opened.cbegin(), m_Opened.cend(), Cell);
            //if (InOpened == m_Opened.cend())
            // Binary search (m_Opened were pre-sorted by our call to std::sort at the beginning of the cycle)
            if (!std::binary_search(m_Opened.cbegin(), m_Opened.cend(), Cell, CPathFinderPointPtrCompLess))
            {
                Cell->_Parent = Current;
                Cell->_GValue = Current->_GValue;

                if (Cell->m_x == Current->m_x || Cell->m_y == Current->m_y)
                    Cell->_GValue += 10; //Not diagonal
                else
                    Cell->_GValue += 14; //Diagonal

                Cell->_HValue = Heuristic(Cell, End);
                Cell->_FValue = Cell->_GValue + Cell->_HValue;
                m_Opened.emplace_back(Cell);
            }
            else
            {
                if (Cell->_GValue < Current->_GValue)
                {
                    Cell->_Parent = Current;
                    if (Cell->m_x == Current->m_x || Cell->m_y == Current->m_y)
                        Cell->_GValue += 10;
                    else
                        Cell->_GValue += 14;
                    Cell->_FValue = Cell->_GValue + Cell->_HValue;
                }
            }
		}
	}

	Clear();
	return false;
}

void CPathFinder::Clear()
{
	ADDTOCALLSTACK("CPathFinder::Clear");
	m_Target = CPointMap(0,0);
	m_pChar = nullptr;
	m_Opened.clear();
	m_Closed.clear();
	m_RealX = m_RealY = 0;
}

void CPathFinder::FillMap()
{
	ADDTOCALLSTACK("CPathFinder::FillMap");
    EXC_TRY("FillMap");

    CPointMap pt(m_pChar->GetTopPoint());
	for ( short x = 0 ; x != MAX_NPC_PATH_STORAGE_SIZE; ++x )
	{
		for ( short y = 0; y != MAX_NPC_PATH_STORAGE_SIZE; ++y )
		{
			if (x == m_Target.m_x && y == m_Target.m_y)
			{
				// always assume that our target position is walkable
				m_Points[x][y]._Walkable = true;
			}
			else
			{
				pt.m_x = (x + m_RealX);
				pt.m_y = (y + m_RealY);
				const CRegion *pArea = m_pChar->CanMoveWalkTo(pt, true, true, DIR_QTY, true);

				m_Points[x][y]._Walkable = pArea ? true : false;
			}

			m_Points[x][y].Set(x, y, pt.m_z, pt.m_map);
		}
	}

	EXC_CATCH;
}

