/**
* @file CSphereMap.h
*/

#pragma once
#ifndef _INC_CSPHEREMAP_H
#define _INC_CSPHEREMAP_H

#include "../game/uo_files/CUOMapBlock.h"
#include "../game/uo_files/CUOStaticItemRec.h"
#include "../game/uo_files/CUOMultiItemRec.h"
#include "../game/uo_files/uofiles_macros.h"
#include "../game/uo_files/uofiles_types.h"
#include "../game/CServTime.h"
#include "./sphere_library/CSArray.h"
#include "CRect.h"

class CSphereCachedMulItem
{
private:
	CServTime m_timeRef;		// When in world.GetTime() was this last referenced.
public:
	static const char *m_sClassName;
	CSphereCachedMulItem();
	virtual ~CSphereCachedMulItem();

private:
	CSphereCachedMulItem(const CSphereCachedMulItem& copy);
	CSphereCachedMulItem& operator=(const CSphereCachedMulItem& other);

public:
	void InitCacheTime();
	bool IsTimeValid() const;
	void HitCacheTime();
	int64 GetCacheAge() const;
};

class CSphereStaticsBlock
{
private:
	size_t m_iStatics;
	CUOStaticItemRec * m_pStatics;	// dyn alloc array block.
public:
	void LoadStatics(dword dwBlockIndex, int map);
	void LoadStatics(size_t iCount, CUOStaticItemRec * pStatics);
public:
	static const char *m_sClassName;
	CSphereStaticsBlock();
	~CSphereStaticsBlock();

private:
	CSphereStaticsBlock(const CSphereStaticsBlock& copy);
	CSphereStaticsBlock& operator=(const CSphereStaticsBlock& other);

public:
	size_t GetStaticQty() const;
	const CUOStaticItemRec * GetStatic( size_t i ) const;
	bool IsStaticPoint( size_t i, int xo, int yo ) const;
};

struct CSphereMapBlocker
{
	dword m_dwBlockFlags;	// How does this item block ? CAN_I_PLATFORM
	dword m_dwTile;			// TERRAIN_QTY + id.
	char m_z;		// Top of a solid object. or bottom of non blocking one.
};

struct CSphereMapBlockState
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
	const char m_z;	// the z we start at. (stay at if we are flying)
	const int m_iHeight;		// The height we need to stand here.
	const char m_zClimb; // We can climb at this height
	const height_t m_zHeight; //our height
	
	height_t m_zClimbHeight;	// return item climb height here
	
	CSphereMapBlocker m_Top;
	CSphereMapBlocker m_Bottom;	// What i would be standing on.
	CSphereMapBlocker m_Lowest;	// the lowest item we have found.	

public:
	CSphereMapBlockState( dword dwBlockFlags, char m_z, int iHeight = PLAYER_HEIGHT, height_t zHeight = PLAYER_HEIGHT );
	CSphereMapBlockState( dword dwBlockFlags, char m_z, int iHeight, char zClimb, height_t zHeight = PLAYER_HEIGHT );

private:
	CSphereMapBlockState(const CSphereMapBlockState& copy);
	CSphereMapBlockState& operator=(const CSphereMapBlockState& other);

public:
	bool IsUsableZ( char zBottom, height_t zHeightEstimate ) const;
	bool CheckTile( dword dwItemBlockFlags, char zBottom, height_t zheight, dword wID );
	bool CheckTile_Item( dword dwItemBlockFlags, char zBottom, height_t zheight, dword wID );
	inline void SetTop( dword &dwItemBlockFlags, char &z, dword &dwID );
	bool CheckTile_Terrain( dword dwItemBlockFlags, char z, dword dwID );
	static lpctstr GetTileName( dword dwID );
};

struct CMapDiffBlock
{
	// A patched map block
	CUOStaticItemRec * m_pStaticsBlock;		// Patched statics
	int m_iStaticsCount;					// Patched statics count
	CUOMapBlock * m_pTerrainBlock;			// Patched terrain
	dword m_BlockId;						// Block represented
	int m_map;	// Map this block is from

	CMapDiffBlock(dword dwBlockId, int map);

	~CMapDiffBlock();

private:
	CMapDiffBlock(const CMapDiffBlock& copy);
	CMapDiffBlock& operator=(const CMapDiffBlock& other);
};

class CMapDiffBlockArray : public CSObjSortArray< CMapDiffBlock*, dword >
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

class CSphereMapBlock :	// Cache this from the MUL files. 8x8 block of the world.
	public CPointSort	// The upper left corner. (ignore z) sort by this
{
protected:
	int		m_map;

private:
	static size_t sm_iCount;	// count number of loaded blocks.

	CUOMapBlock m_Terrain;

public:
	static const char *m_sClassName;
	CSphereStaticsBlock m_Statics;
	CSphereCachedMulItem m_CacheTime;	// keep track of the use time of this item. (client does not care about this)
private:
	void Load(int bx, int by);	// NOTE: This will "throw" on failure !
	void LoadDiffs(dword dwBlockIndex, int map);

public:
	explicit CSphereMapBlock( const CPointMap & pt );

	CSphereMapBlock(int bx, int by, int map);

	virtual ~CSphereMapBlock();

private:
	CSphereMapBlock(const CSphereMapBlock& copy);
	CSphereMapBlock& operator=(const CSphereMapBlock& other);

public:
	int GetOffsetX( int x ) const;
	int GetOffsetY( int y ) const;

	const CUOMapMeter * GetTerrain( int xo, int yo ) const;
	const CUOMapBlock * GetTerrainBlock() const;
};

class CSphereMulti : public CSphereCachedMulItem
{
	// Load all the relivant info for the
private:
	MULTI_TYPE m_id;
protected:
	CUOMultiItemRec_HS * m_pItems;
	size_t m_iItemQty;
private:
	void Init();
	void Release();
public:
	static const char *m_sClassName;
	CSphereMulti();
	explicit CSphereMulti( MULTI_TYPE id );
	virtual ~CSphereMulti();

private:
	CSphereMulti(const CSphereMulti& copy);
	CSphereMulti& operator=(const CSphereMulti& other);

public:
	size_t Load( MULTI_TYPE id );

	MULTI_TYPE GetMultiID() const;
	size_t GetItemCount() const;
	const CUOMultiItemRec_HS * GetItem( size_t i ) const;
};

#endif // _INC_CSphereMap_H
