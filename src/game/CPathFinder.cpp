
#include "../common/CException.h"
#include "../sphere/threads.h"
#include "chars/CChar.h"
#include "CPathFinder.h"
#include <algorithm>


uint CPathFinder::Heuristic(const CPathFinderPoint* Pt1, const CPathFinderPoint* Pt2) // static
{
	return 10*(abs(Pt1->m_x - Pt2->m_x) + abs(Pt1->m_y - Pt2->m_y));
}

void CPathFinder::GetChildren(const CPathFinderPoint* Point, std::deque<CPathFinderPoint*>& ChildrenRefList )
{
	short RealX = 0, RealY = 0;
	for (short x = -1; x != 2; ++x )
	{
		for (short y = -1; y != 2; ++y )
		{
			if ( x == 0 && y == 0 )
				continue;
			RealX = x + Point->m_x;
			RealY = y + Point->m_y;
			if ( RealX < 0 || RealY < 0 || RealX > 23 || RealY > 23 )
				continue;
			if ( m_Points[RealX][RealY].m_Walkable == false )
				continue;
			if ( x != 0 && y != 0 ) // Diagonal
			{
				if ( m_Points[RealX - x][RealY].m_Walkable == false || m_Points[RealX][RealY - y].m_Walkable == false )
					continue;
			}

			ChildrenRefList.emplace_back( &m_Points[RealX][RealY] );
		}
	}
}

CPathFinderPoint::CPathFinderPoint() : m_Parent(0), m_Walkable(false), FValue(0), GValue(0), HValue(0)
{
	//ADDTOCALLSTACK("CPathFinderPoint::CPathFinderPoint");
	m_x = 0;
	m_y = 0;
	m_z = 0;
	m_map = 0;
}

CPathFinderPoint::CPathFinderPoint(const CPointMap& pt) : m_Parent(0), m_Walkable(false), FValue(0), GValue(0), HValue(0)
{
	//ADDTOCALLSTACK("CPathFinderPoint::CPathFinderPoint");
	m_x = pt.m_x;
	m_y = pt.m_y;
	m_z = pt.m_z;
	m_map = pt.m_map;
}


CPathFinderPoint* CPathFinderPoint::GetParent() const
{
    ADDTOCALLSTACK("CPathFinderPoint::GetParent");
	return m_Parent;
}

void CPathFinderPoint::SetParent(CPathFinderPoint* pt)
{
	ADDTOCALLSTACK("CPathFinderPoint::SetParent");
	m_Parent = pt;
}

CPathFinder::CPathFinder(CChar *pChar, CPointMap ptTarget)
{
	ADDTOCALLSTACK("CPathFinder::CPathFinder");
	EXC_TRY("CPathFinder Constructor");

	m_pChar = pChar;
	m_Target = ptTarget;

	const CPointMap& pt = m_pChar->GetTopPoint();
	m_RealX = pt.m_x - (PATH_SIZE / 2);
	m_RealY = pt.m_y - (PATH_SIZE / 2);
	m_Target.m_x -= (short)(m_RealX);
	m_Target.m_y -= (short)(m_RealY);

	EXC_SET_BLOCK("FillMap");

	FillMap();

	EXC_CATCH;
}


int CPathFinder::FindPath() //A* algorithm
{
	ADDTOCALLSTACK("CPathFinder::FindPath");
	ASSERT(m_pChar != nullptr);

    const CPointMap& ptTop = m_pChar->GetTopPoint();
	int X = ptTop.m_x - m_RealX;
	int Y = ptTop.m_y - m_RealY;

	if ( X < 0 || Y < 0 || X > 23 || Y > 23 )
	{
		//Too far away
		Clear();
		return PATH_NONEXISTENT;
	}

	CPathFinderPoint* Start = &m_Points[X][Y]; //Start point
	CPathFinderPoint* End =   &m_Points[m_Target.m_x][m_Target.m_y]; //End Point

	ASSERT(Start);
	ASSERT(End);

	Start->GValue = 0;
	Start->HValue = Heuristic(Start, End);
	Start->FValue = Start->HValue;

	m_Opened.emplace_back( Start );

	std::deque<CPathFinderPoint*> Children;
	std::deque<CPathFinderPoint*>::const_iterator InOpened, InClosed;

	while ( !m_Opened.empty() )
	{
		std::sort(m_Opened.begin(), m_Opened.end());
        CPathFinderPoint *Current = *m_Opened.begin();

		m_Opened.pop_front();
		m_Closed.emplace_back( Current );

		if ( Current == End )
		{
			CPathFinderPoint *PathRef = Current;
			while ( PathRef->GetParent() ) //Rebuild path + save
			{
				PathRef = PathRef->GetParent();
				m_LastPath.emplace_front(CPointMap((word)(PathRef->m_x + m_RealX), (word)(PathRef->m_y + m_RealY), 0, PathRef->m_map));
			}
			Clear();
			return PATH_FOUND;
		}
		else
		{
			Children.clear();
			GetChildren( Current, Children );

			while ( !Children.empty() )
			{
                CPathFinderPoint *Child = Children.front();
				Children.pop_front();

				InClosed = std::find( m_Closed.begin(), m_Closed.end(), Child );
				if ( InClosed != m_Closed.end() )
					continue;

                InOpened = std::find(m_Opened.begin(), m_Opened.end(), Child);
				if ( InOpened == m_Opened.end() )
				{
					Child->SetParent( Current );
					Child->GValue = Current->GValue;

					if ( Child->m_x == Current->m_x || Child->m_y == Current->m_y )
						Child->GValue += 10; //Not diagonal
					else
						Child->GValue += 14; //Diagonal

					Child->HValue = Heuristic( Child, End );
					Child->FValue = Child->GValue + Child->HValue;
					m_Opened.emplace_back( Child );
					//sort ( m_Opened.begin(), m_Opened.end() );
				}
				else
				{
					if ( Child->GValue < Current->GValue )
					{
						Child->SetParent( Current );
						if ( Child->m_x == Current->m_x || Child->m_y == Current->m_y )
							Child->GValue += 10;
						else
							Child->GValue += 14;
						Child->FValue = Child->GValue + Child->HValue;
						//sort ( m_Opened.begin(), m_Opened.end() );
					}
				}
			}
		}
	}


	Clear();
	return PATH_NONEXISTENT;
}

void CPathFinder::Clear()
{
	ADDTOCALLSTACK("CPathFinder::Clear");
	m_Target = CPointMap(0,0);
	m_pChar = 0;
	m_Opened.clear();
	m_Closed.clear();
	m_RealX = 0;
	m_RealY = 0;
}

void CPathFinder::FillMap()
{
	ADDTOCALLSTACK("CPathFinder::FillMap");
	CRegion	*pArea;
	CPointMap	pt, ptChar;

	EXC_TRY("FillMap");
	pt = ptChar = m_pChar->GetTopPoint();

	for ( int x = 0 ; x != PATH_SIZE; ++x )
	{
		for ( int y = 0; y != PATH_SIZE; ++y )
		{
			if (x == m_Target.m_x && y == m_Target.m_y)
			{
				// always assume that our target position is walkable
				m_Points[x][y].m_Walkable = PATH_WALKABLE;
			}
			else
			{
				pt.m_x = (short)(x + (short)(m_RealX));
				pt.m_y = (short)(y + m_RealY);
					pArea = m_pChar->CanMoveWalkTo(pt, true, true, DIR_QTY, true);

				m_Points[x][y].m_Walkable = pArea ? PATH_WALKABLE : PATH_UNWALKABLE;
			}

			m_Points[x][y].Set((word)(x), (word)(y), pt.m_z, pt.m_map);
		}
	}

	EXC_CATCH;
}

size_t CPathFinder::LastPathSize()
{
	ADDTOCALLSTACK("CPathFinder::LastPathSize");
	return m_LastPath.size();
}

void CPathFinder::ClearLastPath()
{
	ADDTOCALLSTACK("CPathFinder::ClearLastPath");
	m_LastPath.clear();
}

