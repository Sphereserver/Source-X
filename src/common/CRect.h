/**
* @file CRect.h
*/

#pragma once
#ifndef _INC_CRECT_H
#define _INC_CRECT_H

#include "CString.h"
#include "graymul.h"


class CRegionBase;
class CRegionLinks;
class CSector;

struct CPointBase	// Non initialized 3d point.
{
public:
	static LPCTSTR const sm_szLoadKeys[];
	static const int sm_Moves[DIR_QTY+1][2];
	static LPCTSTR sm_szDirs[DIR_QTY+1];
public:
	short m_x;	// equipped items dont need x,y
	short m_y;
	signed char m_z;	// This might be layer if equipped ? or equipped on corpse. Not used if in other container.
	byte m_map;			// another map? (only if top level.)

public:
	bool operator == ( const CPointBase & pt ) const;
	bool operator != ( const CPointBase & pt ) const;
	const CPointBase operator += ( const CPointBase & pt );
	const CPointBase operator -= ( const CPointBase & pt );

	void InitPoint();
	void ZeroPoint();
	int GetDistZ( const CPointBase & pt ) const;
	int GetDistZAdj( const CPointBase & pt ) const;
	int GetDistBase( const CPointBase & pt ) const; // Distance between points
	int GetDist( const CPointBase & pt ) const; // Distance between points
	int GetDistSightBase( const CPointBase & pt ) const; // Distance between points based on UO sight
	int GetDistSight( const CPointBase & pt ) const; // Distance between points based on UO sight
	int GetDist3D( const CPointBase & pt ) const; // 3D Distance between points

	bool IsValidZ() const;
	bool IsValidXY() const;
	bool IsValidPoint() const;
	bool IsCharValid() const;

	void ValidatePoint();

	bool IsSame2D( const CPointBase & pt ) const;

	void Set( const CPointBase & pt );
	void Set( word x, word y, signed char z = 0, uchar map = 0 );
	size_t Read( TCHAR * pVal );

	TCHAR * WriteUsed( TCHAR * pszBuffer ) const;
	LPCTSTR WriteUsed() const;

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
	CRegionBase * GetRegion( dword dwType ) const;
	size_t GetRegions( dword dwType, CRegionLinks & rlinks ) const;

	long GetPointSortIndex() const;

	bool r_WriteVal( LPCTSTR pszKey, CGString & sVal ) const;
	bool r_LoadVal( LPCTSTR pszKey, LPCTSTR pszArgs );
};

struct CPointMap : public CPointBase
{
	// A point in the world (or in a container) (initialized)
	CPointMap();
	CPointMap( word x, word y, signed char z = 0, uchar map = 0 );
	CPointMap & operator = ( const CPointBase & pt );
	CPointMap( const CPointBase & pt );
	CPointMap( TCHAR * pVal );
};

struct CPointSort : public CPointMap
{
	CPointSort();
	CPointSort( word x, word y, signed char z = 0, uchar map = 0 );
	CPointSort( const CPointBase & pt );
	virtual ~CPointSort(); // just to make this dynamic
};

#endif	// _INC_CRECT_H

