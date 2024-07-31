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
#include "sphere_library/CSObjSortArray.h"
#include "CRect.h"

class CCachedMulItem
{
protected:
	int64 m_timeRef;		// When in world.GetTime() was this last referenced.

public:
	static const char *m_sClassName;
	CCachedMulItem();
    // Do not make it virtual, since we never store this base class in a container which could store a polymorphic/child type.
    //  We will spare 8 bytes (for the virtual table pointer)
	~CCachedMulItem() = default;

	CCachedMulItem(const CCachedMulItem& copy) = delete;
	CCachedMulItem& operator=(const CCachedMulItem& other) = delete;

public:
	void InitCacheTime();
	bool IsTimeValid() const;
	void HitCacheTime();
	int64 GetCacheAge() const;
};

class CServerStaticsBlock
{
private:
	uint m_iStatics;
	CUOStaticItemRec * m_pStatics;	// dyn alloc array block.

public:
	void LoadStatics(dword dwBlockIndex, int map);
	void LoadStatics(uint uiCount, CUOStaticItemRec * pStatics);
public:
	static const char *m_sClassName;
	CServerStaticsBlock();
	~CServerStaticsBlock();

	CServerStaticsBlock(const CServerStaticsBlock& copy) = delete;
	CServerStaticsBlock& operator=(const CServerStaticsBlock& other) = delete;

public:
    // These methods are called so frequently but in so few pieces of code that's very important to inline them
	inline uint GetStaticQty() const { 
		return m_iStatics;
	}
    inline const CUOStaticItemRec * GetStatic( uint i ) const
    {
        ASSERT( i < m_iStatics );
        return( &m_pStatics[i] );
    }
    inline bool IsStaticPoint( uint i, int xo, int yo ) const
    {
        ASSERT( (xo >= 0) && (xo < UO_BLOCK_SIZE) );
        ASSERT( (yo >= 0) && (yo < UO_BLOCK_SIZE) );
        ASSERT( i < m_iStatics );
        return( (m_pStatics[i].m_x == xo) && (m_pStatics[i].m_y == yo) );
    }
};

struct CServerMapBlocker
{
	uint64 m_uiBlockFlags;	// How does this item block ? CAN_I_PLATFORM
	dword m_dwTile;			// TERRAIN_QTY + id.
	char m_z;				// Top (z + ((climbable item) ? height/2 : height)) of a solid object, or bottom (z) of non blocking one.
    height_t m_height;      // The actual height of the item (0 if terrain)
};

struct CServerMapBlockingState
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

	const uint64 m_uiBlockFlags;	// The block flags we (the specific char who requested this class instance) can overcome.	
	const char m_z;	            // the z we start at. (stay at if we are flying)
	const int m_iHeight;		// The height we need to stand here.
	const char m_zClimb;        // We can climb at this height
    const height_t m_zHeight;   // our height
	height_t m_zClimbHeight;	// return item climb height here
	
	CServerMapBlocker m_Top;
	CServerMapBlocker m_Bottom;	// What i would be standing on.
	CServerMapBlocker m_Lowest;	// the lowest item we have found.	

public:
	CServerMapBlockingState( uint64 uiBlockFlags, char m_z, int iHeight = PLAYER_HEIGHT, height_t zHeight = PLAYER_HEIGHT ) noexcept;
	CServerMapBlockingState( uint64 uiBlockFlags, char m_z, int iHeight, char zClimb, height_t zHeight = PLAYER_HEIGHT ) noexcept;

	CServerMapBlockingState(const CServerMapBlockingState& copy) = delete;
	CServerMapBlockingState& operator=(const CServerMapBlockingState& other) = delete;

public:
	bool CheckTile( uint64 uiItemBlockFlags, char zBottom, height_t zheight, dword wID ) noexcept;
	bool CheckTile_Item( uint64 uiItemBlockFlags, char zBottom, height_t zheight, dword wID ) noexcept;
	bool CheckTile_Terrain( uint64 uiItemBlockFlags, char z, dword dwID ) noexcept;
	static lpctstr GetTileName( dword dwID );
};

struct CServerMapDiffBlock
{
	// A patched map block
	CUOMapBlock * m_pTerrainBlock;			// Patched terrain
    CUOStaticItemRec * m_pStaticsBlock;		// Patched statics
    int m_iStaticsCount;					// Patched statics count
	dword m_BlockId;						// Block represented
	int m_map;	// Map this block is from

	CServerMapDiffBlock(dword dwBlockId, int map);
	~CServerMapDiffBlock();

	CServerMapDiffBlock(const CServerMapDiffBlock& copy) = delete;
	CServerMapDiffBlock& operator=(const CServerMapDiffBlock& other) = delete;
};

struct CServerMapDiffBlockArray : public CSObjSortArray< CServerMapDiffBlock*, dword >
{
    CServerMapDiffBlockArray() = default;
	int CompareKey( dword id, CServerMapDiffBlock* pBase, bool fNoSpaces ) const;

	CServerMapDiffBlockArray(const CServerMapDiffBlockArray& copy) = delete;
	CServerMapDiffBlockArray& operator=(const CServerMapDiffBlockArray& other) = delete;
};

class CServerMapDiffCollection
{
	// This class will be used to access mapdiff data
private:
	bool m_fLoaded;

	CServerMapDiffBlockArray m_pMapDiffBlocks[MAP_SUPPORTED_QTY];
	CServerMapDiffBlock * GetNewBlock( dword dwBlockId, int map );
	void LoadMapDiffs();

public:
	CServerMapDiffCollection();
	~CServerMapDiffCollection();

	CServerMapDiffCollection(const CServerMapDiffCollection& copy) = delete;
	CServerMapDiffCollection& operator=(const CServerMapDiffCollection& other) = delete;

public:
	void Init();
	CServerMapDiffBlock * GetAtBlock( int bx, int by, int map );
	CServerMapDiffBlock * GetAtBlock( dword dwBlockId, int map );
};

class CServerMapBlock :	// Cache this from the MUL files. 8x8 block of the world.
	public CPointSort	// The upper left corner. (ignore z) sort by this
{
private:
	static size_t sm_iCount;	// count number of loaded blocks.

	CUOMapBlock m_Terrain;

public:
	static const char *m_sClassName;
	CServerStaticsBlock m_Statics;
	CCachedMulItem m_CacheTime;	// keep track of the use time of this item. (client does not care about this)

private:
	void Load(int bx, int by);	// NOTE: This will "throw" on failure !
	//void LoadDiffs(dword dwBlockIndex, int map);

public:
	CServerMapBlock(int bx, int by, int map);
	virtual ~CServerMapBlock();

	CServerMapBlock(const CServerMapBlock& copy) = delete;
	CServerMapBlock& operator=(const CServerMapBlock& other) = delete;

public:
	inline int GetOffsetX(int x) const
	{
		return (x - m_x);
	}
	inline int GetOffsetY( int y ) const
	{
		return (y - m_y);
	}

	inline const CUOMapBlock * GetTerrainBlock() const
	{
		return &m_Terrain;
	}
	const CUOMapMeter* GetTerrain(int xo, int yo) const
	{
		ASSERT(xo >= 0 && xo < UO_BLOCK_SIZE);
		ASSERT(yo >= 0 && yo < UO_BLOCK_SIZE);
		return &(m_Terrain.m_Meter[yo * UO_BLOCK_SIZE + xo]);
	}
};

class CUOMulti : private CCachedMulItem
{
	// Load all the relivant info for the

protected:
	CUOMultiItemRec_HS * m_pItems;
	uint m_iItemQty;

private:
    MULTI_TYPE m_id;

public:
	static const char *m_sClassName;

	CUOMulti();
	explicit CUOMulti( MULTI_TYPE id );
	~CUOMulti();

    CUOMulti(const CUOMulti& copy) = delete;
	CUOMulti& operator=(const CUOMulti& other) = delete;

    void HitCacheTime() noexcept;

private:
	void Init();
	void Release();

public:
	size_t Load( MULTI_TYPE id );

    inline MULTI_TYPE GetMultiID() const {
        return m_id;
    }
    inline uint GetItemCount() const {
        return m_iItemQty;
    }
	const CUOMultiItemRec_HS * GetItem( size_t i ) const;
};

#endif // _INC_CSERVERMAP_H
