/**
* @file CServerMap.h
*/

#ifndef _INC_CSERVERMAP_H
#define _INC_CSERVERMAP_H

#include "../game/uo_files/CUOMapBlock.h"
#include "../game/uo_files/CUOStaticItemRec.h"
#include "../game/uo_files/CUOMultiItemRec.h"
#include "../game/uo_files/uofiles_macros.h"
#include "../game/uo_files/uofiles_types.h"
#include "../game/CServerTime.h"
#include "sphere_library/CSObjSortArray.h"
#include "CRect.h"

class CCachedMulItem
{
private:
	CServerTime m_timeRef;		// When in world.GetTime() was this last referenced.
public:
	static const char *m_sClassName;
	CCachedMulItem();
	virtual ~CCachedMulItem();

private:
	CCachedMulItem(const CCachedMulItem& copy);
	CCachedMulItem& operator=(const CCachedMulItem& other);

public:
	void InitCacheTime();
	bool IsTimeValid() const;
	void HitCacheTime();
	int64 GetCacheAge() const;
};

class CServerStaticsBlock
{
private:
	size_t m_iStatics;
	CUOStaticItemRec * m_pStatics;	// dyn alloc array block.
public:
	void LoadStatics(dword dwBlockIndex, int map);
	void LoadStatics(size_t iCount, CUOStaticItemRec * pStatics);
public:
	static const char *m_sClassName;
	CServerStaticsBlock();
	~CServerStaticsBlock();

private:
	CServerStaticsBlock(const CServerStaticsBlock& copy);
	CServerStaticsBlock& operator=(const CServerStaticsBlock& other);

public:
	inline size_t GetStaticQty() const { 
		return m_iStatics;
	}
	const CUOStaticItemRec * GetStatic( size_t i ) const;
	bool IsStaticPoint( size_t i, int xo, int yo ) const;
};

struct CServerMapBlocker
{
	dword m_dwBlockFlags;	// How does this item block ? CAN_I_PLATFORM
	dword m_dwTile;			// TERRAIN_QTY + id.
	char m_z;				// Top of a solid object. or bottom of non blocking one.
};

struct CServerMapBlockState
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
	
	CServerMapBlocker m_Top;
	CServerMapBlocker m_Bottom;	// What i would be standing on.
	CServerMapBlocker m_Lowest;	// the lowest item we have found.	

public:
	CServerMapBlockState( dword dwBlockFlags, char m_z, int iHeight = PLAYER_HEIGHT, height_t zHeight = PLAYER_HEIGHT );
	CServerMapBlockState( dword dwBlockFlags, char m_z, int iHeight, char zClimb, height_t zHeight = PLAYER_HEIGHT );

private:
	CServerMapBlockState(const CServerMapBlockState& copy);
	CServerMapBlockState& operator=(const CServerMapBlockState& other);

public:
	bool IsUsableZ( char zBottom, height_t zHeightEstimate ) const;
	bool CheckTile( dword dwItemBlockFlags, char zBottom, height_t zheight, dword wID );
	bool CheckTile_Item( dword dwItemBlockFlags, char zBottom, height_t zheight, dword wID );
	inline void SetTop( dword &dwItemBlockFlags, char &z, dword &dwID );
	bool CheckTile_Terrain( dword dwItemBlockFlags, char z, dword dwID );
	static lpctstr GetTileName( dword dwID );
};

struct CServerMapDiffBlock
{
	// A patched map block
	CUOStaticItemRec * m_pStaticsBlock;		// Patched statics
	int m_iStaticsCount;					// Patched statics count
	CUOMapBlock * m_pTerrainBlock;			// Patched terrain
	dword m_BlockId;						// Block represented
	int m_map;	// Map this block is from

	CServerMapDiffBlock(dword dwBlockId, int map);

	~CServerMapDiffBlock();

private:
	CServerMapDiffBlock(const CServerMapDiffBlock& copy);
	CServerMapDiffBlock& operator=(const CServerMapDiffBlock& other);
};

class CServerMapDiffBlockArray : public CSObjSortArray< CServerMapDiffBlock*, dword >
{
public:
	int CompareKey( dword id, CServerMapDiffBlock* pBase, bool fNoSpaces ) const;

public:
	CServerMapDiffBlockArray() { };
private:
	CServerMapDiffBlockArray(const CServerMapDiffBlockArray& copy);
	CServerMapDiffBlockArray& operator=(const CServerMapDiffBlockArray& other);
};

class CServerMapDiffCollection
{
	// This class will be used to access mapdiff data
private:
	bool m_bLoaded;

	CServerMapDiffBlockArray m_pMapDiffBlocks[256];
	CServerMapDiffBlock * GetNewBlock( dword dwBlockId, int map );
	void LoadMapDiffs();

public:
	CServerMapDiffCollection();
	~CServerMapDiffCollection();

private:
	CServerMapDiffCollection(const CServerMapDiffCollection& copy);
	CServerMapDiffCollection& operator=(const CServerMapDiffCollection& other);

public:
	void Init();
	CServerMapDiffBlock * GetAtBlock( int bx, int by, int map );
	CServerMapDiffBlock * GetAtBlock( dword dwBlockId, int map );
};

class CServerMapBlock :	// Cache this from the MUL files. 8x8 block of the world.
	public CPointSort	// The upper left corner. (ignore z) sort by this
{
protected:
	int	m_map;

private:
	static size_t sm_iCount;	// count number of loaded blocks.

	CUOMapBlock m_Terrain;

public:
	static const char *m_sClassName;
	CServerStaticsBlock m_Statics;
	CCachedMulItem m_CacheTime;	// keep track of the use time of this item. (client does not care about this)
private:
	void Load(int bx, int by);	// NOTE: This will "throw" on failure !
	void LoadDiffs(dword dwBlockIndex, int map);

public:
	explicit CServerMapBlock( const CPointMap & pt );

	CServerMapBlock(int bx, int by, int map);

	virtual ~CServerMapBlock()
	{ 
		--sm_iCount;
	}

private:
	CServerMapBlock(const CServerMapBlock& copy);
	CServerMapBlock& operator=(const CServerMapBlock& other);

public:
	int GetOffsetX(int x) const
	{
		return (x - m_x);
	}
	int GetOffsetY( int y ) const
	{
		return (y - m_y);
	}

	const CUOMapMeter * GetTerrain( int xo, int yo ) const
	{
		ASSERT(xo >= 0 && xo < UO_BLOCK_SIZE);
		ASSERT(yo >= 0 && yo < UO_BLOCK_SIZE);
		return &(m_Terrain.m_Meter[yo*UO_BLOCK_SIZE + xo]);
	}
	const CUOMapBlock * GetTerrainBlock() const
	{
		return &m_Terrain;
	}
};

class CSphereMulti : public CCachedMulItem
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

#endif // _INC_CSERVERMAP_H
