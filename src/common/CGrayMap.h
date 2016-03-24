/**
* @file CGrayMap.h
*/

#pragma once
#ifndef _INC_CGRAYMAP_H
#define _INC_CGRAYMAP_H

#include "../game/CServTime.h"
#include "CArray.h"
#include "graymul.h"
#include "CRect.h"


class CGrayCachedMulItem
{
private:
	CServTime m_timeRef;		// When in world.GetTime() was this last referenced.
public:
	static const char *m_sClassName;
	CGrayCachedMulItem();
	virtual ~CGrayCachedMulItem();

private:
	CGrayCachedMulItem(const CGrayCachedMulItem& copy);
	CGrayCachedMulItem& operator=(const CGrayCachedMulItem& other);

public:
	void InitCacheTime();
	bool IsTimeValid() const;
	void HitCacheTime();
	INT64 GetCacheAge() const;
};

class CGrayStaticsBlock
{
private:
	size_t m_iStatics;
	CUOStaticItemRec * m_pStatics;	// dyn alloc array block.
public:
	void LoadStatics(dword dwBlockIndex, int map);
	void LoadStatics(size_t iCount, CUOStaticItemRec * pStatics);
public:
	static const char *m_sClassName;
	CGrayStaticsBlock();
	~CGrayStaticsBlock();

private:
	CGrayStaticsBlock(const CGrayStaticsBlock& copy);
	CGrayStaticsBlock& operator=(const CGrayStaticsBlock& other);

public:
	size_t GetStaticQty() const;
	const CUOStaticItemRec * GetStatic( size_t i ) const;
	bool IsStaticPoint( size_t i, int xo, int yo ) const;
};

struct CGrayMapBlocker
{
	dword m_dwBlockFlags;	// How does this item block ? CAN_I_PLATFORM
	dword m_dwTile;			// TERRAIN_QTY + id.
	signed char m_z;		// Top of a solid object. or bottom of non blocking one.
};

struct CGrayMapBlockState
{
	// Go through the list of stuff at this location to decide what is  blocking us and what is not.
	//  dwBlockFlags = what we can pass thru. doors, water, walls ?
	//		CAN_C_GHOST	= Moves thru doors etc.	- CAN_I_DOOR = UFLAG4_DOOR						
	//		CAN_C_SWIM = walk thru water - CAN_I_WATER = UFLAG1_WATER
	//		CAN_C_WALK = it is possible that i can step up. - CAN_I_PLATFORM = UFLAG2_PLATFORM
	//		CAN_C_PASSWALLS	= walk through all blcking items - CAN_I_BLOCK = UFLAG1_BLOCK
	//		CAN_C_FLY  = gravity does not effect me. -  CAN_I_CLIMB = UFLAG2_CLIMBABLE
	//		CAN_C_FIRE_IMMUNE = i can walk into lava etc. - CAN_I_FIRE = UFLAG1_DAMAGE
	//		CAN_C_HOVER = i can follow hover routes. - CAN_I_HOVER = UFLAG4_HOVEROVER

	const dword m_dwBlockFlags;	// The block flags we can overcome.	
	const signed char m_z;	// the z we start at. (stay at if we are flying)
	const int m_iHeight;		// The height we need to stand here.
	const signed char m_zClimb; // We can climb at this height
	const height_t m_zHeight; //our height
	
	height_t m_zClimbHeight;	// return item climb height here
	
	CGrayMapBlocker m_Top;
	CGrayMapBlocker m_Bottom;	// What i would be standing on.
	CGrayMapBlocker m_Lowest;	// the lowest item we have found.	

public:
	CGrayMapBlockState( dword dwBlockFlags, signed char m_z, int iHeight = PLAYER_HEIGHT, height_t zHeight = PLAYER_HEIGHT );
	CGrayMapBlockState( dword dwBlockFlags, signed char m_z, int iHeight, signed char zClimb, height_t zHeight = PLAYER_HEIGHT );

private:
	CGrayMapBlockState(const CGrayMapBlockState& copy);
	CGrayMapBlockState& operator=(const CGrayMapBlockState& other);

public:
	bool IsUsableZ( signed char zBottom, height_t zHeightEstimate ) const;
	bool CheckTile( dword dwItemBlockFlags, signed char zBottom, height_t zheight, dword wID );
	bool CheckTile_Item( dword dwItemBlockFlags, signed char zBottom, height_t zheight, dword wID );
	inline void SetTop( dword &dwItemBlockFlags, signed char &z, dword &dwID );
	bool CheckTile_Terrain( dword dwItemBlockFlags, signed char z, dword dwID );
	static LPCTSTR GetTileName( dword dwID );
};

struct CMapDiffBlock
{
	// A patched map block
	CUOStaticItemRec * m_pStaticsBlock;		// Patched statics
	int m_iStaticsCount;					// Patched statics count
	CUOMapBlock * m_pTerrainBlock;			// Patched terrain
	dword m_BlockId;						// Block represented
	int m_map;								// Map this block is from

	CMapDiffBlock(dword dwBlockId, int map);

	~CMapDiffBlock();

private:
	CMapDiffBlock(const CMapDiffBlock& copy);
	CMapDiffBlock& operator=(const CMapDiffBlock& other);
};

class CMapDiffBlockArray : public CGObSortArray< CMapDiffBlock*, dword >
{
public:
	int CompareKey( dword id, CMapDiffBlock* pBase, bool fNoSpaces ) const;

public:
	CMapDiffBlockArray() { };
private:
	CMapDiffBlockArray(const CMapDiffBlockArray& copy);
	CMapDiffBlockArray& operator=(const CMapDiffBlockArray& other);
};

class CMapDiffCollection
{
	// This class will be used to access mapdiff data
private:
	bool m_bLoaded;

	CMapDiffBlockArray m_pMapDiffBlocks[256];
	CMapDiffBlock * GetNewBlock( dword dwBlockId, int map );
	void LoadMapDiffs();

public:
	CMapDiffCollection();
	~CMapDiffCollection();

private:
	CMapDiffCollection(const CMapDiffCollection& copy);
	CMapDiffCollection& operator=(const CMapDiffCollection& other);

public:
	void Init();
	CMapDiffBlock * GetAtBlock( int bx, int by, int map );
	CMapDiffBlock * GetAtBlock( dword dwBlockId, int map );
};

class CGrayMapBlock :	// Cache this from the MUL files. 8x8 block of the world.
	public CPointSort	// The upper left corner. (ignore z) sort by this
{
protected:
	int		m_map;

private:
	static size_t sm_iCount;	// count number of loaded blocks.

	CUOMapBlock m_Terrain;

public:
	static const char *m_sClassName;
	CGrayStaticsBlock m_Statics;
	CGrayCachedMulItem m_CacheTime;	// keep track of the use time of this item. (client does not care about this)
private:
	void Load(int bx, int by);	// NOTE: This will "throw" on failure !
	void LoadDiffs(dword dwBlockIndex, int map);

public:
	explicit CGrayMapBlock( const CPointMap & pt );

	CGrayMapBlock(int bx, int by, int map);

	virtual ~CGrayMapBlock();

private:
	CGrayMapBlock(const CGrayMapBlock& copy);
	CGrayMapBlock& operator=(const CGrayMapBlock& other);

public:
	int GetOffsetX( int x ) const;
	int GetOffsetY( int y ) const;

	const CUOMapMeter * GetTerrain( int xo, int yo ) const;
	const CUOMapBlock * GetTerrainBlock() const;
};

class CGrayMulti : public CGrayCachedMulItem
{
	// Load all the relivant info for the
private:
	MULTI_TYPE m_id;
protected:
	CUOMultiItemRec2 * m_pItems;
	size_t m_iItemQty;
private:
	void Init();
	void Release();
public:
	static const char *m_sClassName;
	CGrayMulti();
	explicit CGrayMulti( MULTI_TYPE id );
	virtual ~CGrayMulti();

private:
	CGrayMulti(const CGrayMulti& copy);
	CGrayMulti& operator=(const CGrayMulti& other);

public:
	size_t Load( MULTI_TYPE id );

	MULTI_TYPE GetMultiID() const;
	size_t GetItemCount() const;
	const CUOMultiItemRec2 * GetItem( size_t i ) const;
};

#endif // _INC_CGRAYMAP_H
