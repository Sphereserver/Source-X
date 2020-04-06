/**
* @file CWorldMap.h
*
*/

#ifndef _INC_CWORLDMAP_H
#define _INC_CWORLDMAP_H

#include "../common/resource/blocks/CItemTypeDef.h"
#include "../common/CServerMap.h"
#include "items/item_types.h"
#include "CSector.h"

class CObjBase;


class CWorldMap
{
public:
	static const char* m_sClassName;

	// Natural resources

	static CItem* CheckNaturalResource(const CPointMap& pt, IT_TYPE iType, bool fTest = true, CChar* pCharSrc = nullptr);


	// Sectors

	static CSector* GetSector(int map, int i);	// gets sector # from one map


	// Map blocks (for caching) and terrain

	static const CServerMapBlock* GetMapBlock(const CPointMap& pt);
	static const CUOMapMeter* GetMapMeter(const CPointMap& pt); // Height of MAP0.MUL at given coordinates

	static CItemTypeDef* GetTerrainItemTypeDef(dword dwIndex);
	static IT_TYPE		 GetTerrainItemType(dword dwIndex);


	// Height checks

	static void GetHeightPoint2( const CPointMap & pt, CServerMapBlockState & block, bool fHouseCheck = false );
	static char GetHeightPoint2(const CPointMap & pt, dword & dwBlockFlags, bool fHouseCheck = false); // Height of player who walked to X/Y/OLDZ

	static void GetHeightPoint( const CPointMap & pt, CServerMapBlockState & block, bool fHouseCheck = false );
	static char GetHeightPoint( const CPointMap & pt, dword & dwBlockFlags, bool fHouseCheck = false );

	static void GetFixPoint( const CPointMap & pt, CServerMapBlockState & block);


	// Object position-based search

	static CPointMap FindItemTypeNearby( const CPointMap & pt, IT_TYPE iType, int iDistance = 0, bool fCheckMulti = false, bool fLimitZ = false );
	static bool IsItemTypeNear( const CPointMap & pt, IT_TYPE iType, int iDistance, bool fCheckMulti );

	static CPointMap FindTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance = 0 );
	static bool IsTypeNear_Top( const CPointMap & pt, IT_TYPE iType, int iDistance = 0 );
};


class CWorldSearch	// define a search of the world.
{
	enum class ws_search_e
	{
		None = 0,
		Items,
		Chars
	};

	const CPointMap _pt;		// Base point of our search.
	const int _iDist;			// How far from the point are we interested in
	bool _fAllShow;				// Include Even inert items.
	bool _fSearchSquare;		// Search in a square (uo-sight distance) rather than a circle (standard distance).

	ws_search_e _eSearchType;
	bool _fInertToggle;			// We are now doing the inert chars.
	CSObjCont* _pCurCont;		// Sector-attached object container in which we are searching right now.
	CObjBase* _pObj;			// The current object of interest.
	size_t _idxObj;

	CSector * _pSectorBase;		// Don't search the center sector 2 times.
	CSector * _pSector;			// current Sector
	CRectMap _rectSector;		// A rectangle containing our sectors we can search.
	int		 _iSectorCur;		// What is the current Sector index in m_rectSector

public:
	static const char *m_sClassName;
	CWorldSearch(const CWorldSearch& copy) = delete;
	CWorldSearch& operator=(const CWorldSearch& other) = delete;

	explicit CWorldSearch( const CPointMap & pt, int iDist = 0 );

	void SetAllShow( bool fView );
	void SetSearchSquare( bool fSquareSearch );
	void RestartSearch();		// Setting current obj to nullptr will restart the search 
	CChar * GetChar();
	CItem * GetItem();

private:
	bool GetNextSector();
};


#endif // _INC_CWORLDMAP_H
