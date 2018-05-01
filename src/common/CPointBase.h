/**
* @file CPointBase.h
* @brief Classes for a point in a tridimensional space.
*/

#ifndef _INC_CPOINTBASE_H
#define _INC_CPOINTBASE_H

#include "../game/uo_files/uofiles_enums.h"
#include "../game/uo_files/CUOMapList.h"

class CSString;
class CSector;
class CRegion;
class CRegionLinks;


DIR_TYPE GetDirTurn( DIR_TYPE dir, int offset );


struct CPointBase	// Non initialized 3d point.
{
public:
	static lpctstr const sm_szLoadKeys[];
	static const int sm_Moves[DIR_QTY+1][2];
	static lpctstr sm_szDirs[DIR_QTY+1];
public:
	// Do NOT change these datatypes: they seem to not have much sense, but are stored this way inside the mul files.
	short m_x;		// equipped items dont need x,y
	short m_y;
	char m_z;		// this might be layer if equipped ? or equipped on corpse. Not used if in other container.
	uchar m_map;		// another map? (only if top level.)

public:
	void InitPoint();
	void ZeroPoint();

	CPointBase()
	{
		InitPoint();
	}

	bool operator == ( const CPointBase & pt ) const;
	bool operator != ( const CPointBase & pt ) const;
	const CPointBase operator += ( const CPointBase & pt );
	const CPointBase operator -= ( const CPointBase & pt );

	
	int GetDistZ( const CPointBase & pt ) const;
	int GetDistZAdj( const CPointBase & pt ) const;
	int GetDistBase( const CPointBase & pt ) const;			// Distance between points
	int GetDist( const CPointBase & pt ) const;				// Distance between points
	int GetDistSightBase( const CPointBase & pt ) const;	// Distance between points based on UO sight
	int GetDistSight( const CPointBase & pt ) const;		// Distance between points based on UO sight
	int GetDist3D( const CPointBase & pt ) const;			// 3D Distance between points

	bool IsValidZ() const;
	bool IsValidXY() const;
	bool IsValidPoint() const;
	bool IsCharValid() const;

	void ValidatePoint();

	bool IsSame2D( const CPointBase & pt ) const;

	void Set( const CPointBase & pt );
	void Set( word x, word y, char z = 0, uchar map = 0 );
	size_t Read( tchar * pVal );

	tchar * WriteUsed( tchar * pszBuffer ) const;
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
	size_t GetRegions( dword dwType, CRegionLinks & rlinks ) const;

	int GetPointSortIndex() const;

	bool r_WriteVal( lpctstr pszKey, CSString & sVal ) const;
	bool r_LoadVal( lpctstr pszKey, lpctstr pszArgs );
};

struct CPointMap : public CPointBase
{
	// A point in the world (or in a container) (initialized)
	CPointMap() {};
	CPointMap( short x, short y, char z = 0, uchar map = 0 );
    inline CPointMap & operator = (const CPointBase & pt)
    {
        Set( pt );
        return ( * this );
    }
    inline CPointMap(const CPointBase & pt)
    {
        Set( pt );
    }
    inline CPointMap(tchar * pVal)
    {
        Read( pVal );
    }
};

struct CPointSort : public CPointMap
{
    inline CPointSort()
    {
        InitPoint();
    }
	CPointSort( word x, word y, char z = 0, uchar map = 0 );
    inline CPointSort(const CPointBase & pt)
    {
        Set( pt );
    }
    virtual ~CPointSort() {}; // just to make this dynamic
};


#endif // _INC_CPOINTBASE_H
