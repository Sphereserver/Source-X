/**
* @file CPointBase.h
* @brief Classes for a point in a tridimensional space.
*/

#ifndef _INC_CPOINTBASE_H
#define _INC_CPOINTBASE_H

#include "../game/uo_files/uofiles_enums.h"
#include "../game/uo_files/CUOMapList.h"
#include <vector>

class CSString;
class CSector;
class CRegion;

using CRegionLinks = std::vector<CRegion*>;


DIR_TYPE GetDirTurn( DIR_TYPE dir, int offset );


struct CPointBase	// Non initialized 3d point.
{
public:
	static lpctstr const sm_szLoadKeys[];
	static const short sm_Moves[DIR_QTY+1][2];
	static lpctstr sm_szDirs[DIR_QTY+1];

public:
	// Do NOT change these datatypes: they seem to not have much sense, but are stored this way inside the mul files.
	short m_x;		// equipped items dont need x,y
	short m_y;
	char m_z;		// this might be layer if equipped ? or equipped on corpse. Not used if in other container.
	uchar m_map;		// another map? (only if top level.)

public:
	void InitPoint() noexcept;
	void ZeroPoint() noexcept;

	CPointBase() noexcept;
	CPointBase(short x, short y, char z = 0, uchar map = 0) noexcept;
	CPointBase(const CPointBase&) noexcept = default;
	CPointBase(CPointBase&&) noexcept = default;

	CPointBase& operator = (const CPointBase&) noexcept = default;
	bool operator == ( const CPointBase & pt ) const noexcept;
	bool operator != ( const CPointBase & pt ) const noexcept;
	const CPointBase& operator += ( const CPointBase & pt ) noexcept;
	const CPointBase& operator -= ( const CPointBase & pt ) noexcept;

	
	int GetDistZ( const CPointBase & pt ) const noexcept;
	int GetDistBase( const CPointBase & pt ) const noexcept;	    // Distance between points
	int GetDist( const CPointBase & pt ) const noexcept;			// Distance between points
	int GetDistSightBase( const CPointBase & pt ) const noexcept;	// Distance between points based on UO sight
	int GetDistSight( const CPointBase & pt ) const noexcept;		// Distance between points based on UO sight
	int GetDist3D( const CPointBase & pt ) const noexcept;			// 3D Distance between points

	bool IsValidZ() const noexcept;
	bool IsValidXY() const noexcept;
	bool IsCharValid() const noexcept;
	inline bool IsValidPoint() const noexcept
	{
		// Called a LOT of times, it's worth inlining it.
		return (IsValidXY() && IsValidZ());
	}

	void ValidatePoint() noexcept;

	bool IsSame2D( const CPointBase & pt ) const noexcept;

	void Set( const CPointBase & pt ) noexcept;
	void Set( short x, short y, char z = 0, uchar map = 0 ) noexcept;
	int Read( tchar * pVal );

	tchar * WriteUsed( tchar * ptcBuffer, uint uiBufferLen ) const;
	lpctstr WriteUsed() const;

	void Move( DIR_TYPE dir );
	void MoveN( DIR_TYPE dir, int amount );

	DIR_TYPE GetDir( const CPointBase & pt, DIR_TYPE DirDefault = DIR_QTY ) const; // Direction to point pt

	// Take a step directly toward the target.
	int StepLinePath( const CPointBase & ptSrc, int iSteps );

	CSector * GetSector() const;

#define REGION_TYPE_AREA  1
#define REGION_TYPE_ROOM  2
#define REGION_TYPE_HOUSE 4
#define REGION_TYPE_SHIP  8
#define REGION_TYPE_MULTI 12
	CRegion * GetRegion( dword dwType ) const;
	size_t GetRegions( dword dwType, CRegionLinks *pRLinks ) const;

	int GetPointSortIndex() const noexcept;

	bool r_WriteVal( lpctstr ptcKey, CSString & sVal ) const;
	bool r_LoadVal( lpctstr ptcKey, lpctstr pszArgs );
};

struct CPointMap : public CPointBase
{
	// A point in the world (or in a container) (initialized)
    CPointMap() noexcept = default;
	CPointMap(short x, short y, char z = 0, uchar map = 0) noexcept :
		CPointBase(x, y, z, map) { }

	CPointMap(const CPointMap& pt) noexcept :
		CPointBase(pt.m_x, pt.m_y, pt.m_z, pt.m_map) { }
	CPointMap(const CPointBase& pt) noexcept :
		CPointBase(pt.m_x, pt.m_y, pt.m_z, pt.m_map) { }

	CPointMap(CPointMap&&) noexcept = default;
	CPointMap(CPointBase&& pt) noexcept : CPointMap(static_cast<CPointMap&&>(pt)) { }

	CPointMap(tchar* pVal) {
		Read(pVal);
	}

	CPointMap& operator = (const CPointMap&) noexcept = default;
	CPointMap& operator = (const CPointBase& pt) noexcept {
		return CPointMap::operator=(static_cast<const CPointMap&>(pt));
	}
};

struct CPointSort : public CPointMap
{
    CPointSort() noexcept = default; // InitPoint() already called by CPointBase constructor
	CPointSort( short x, short y, char z = 0, uchar map = 0 ) noexcept;
	explicit CPointSort(const CPointBase& pt) noexcept;
    ~CPointSort() = default;
};


#endif // _INC_CPOINTBASE_H
