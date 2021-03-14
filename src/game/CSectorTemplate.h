/**
* @file CSectorTemplate.h
*
*/

#ifndef _INC_CSECTORTEMPLATE_H
#define _INC_CSECTORTEMPLATE_H

#include "../common/sphere_library/CSObjCont.h"
#include "../common/sphere_library/CSObjSortArray.h"
#include "../common/CRect.h"


class CItem;
class CSector;
class CTeleport;

struct CCharsDisconnectList : public CSObjCont
{
	CCharsDisconnectList() = default;
    void AddCharDisconnected( CChar * pChar );

private:
	CCharsDisconnectList(const CCharsDisconnectList& copy);
	CCharsDisconnectList& operator=(const CCharsDisconnectList& other);
};

struct CCharsActiveList : public CSObjCont
{
private:
	int m_iClients;				// How many clients in this sector now?
	int64 m_iTimeLastClient;	// age the sector based on last client here.
    
protected:
	void OnRemoveObj(CSObjContRec* pObjRec);	// Override this = called when removed from list.

public:
	CCharsActiveList();
	void AddCharActive(CChar* pChar);
	int GetClientsNumber() const noexcept {
		return m_iClients;
	}
	int64 GetTimeLastClient() const noexcept {
		return m_iTimeLastClient;
	}
	void SetTimeLastClient(int64 iTime) noexcept {
		m_iTimeLastClient = iTime;
	}

private:
	CCharsActiveList(const CCharsActiveList& copy);
	CCharsActiveList& operator=(const CCharsActiveList& other);
};

struct CItemsList : public CSObjCont
{
	static bool sm_fNotAMove;	// hack flag to prevent items from bouncing around too much.

public:
	CItemsList() = default;
	void AddItemToSector( CItem * pItem );

protected:
	void OnRemoveObj(CSObjContRec* pObRec);	// Override this = called when removed from list.

private:
	CItemsList(const CItemsList& copy);
	CItemsList& operator=(const CItemsList& other);
};


class CSectorBase		// world sector
{
    class CObjPointSortArray : public CSObjSortArray< CPointSort*, int >
    {
    public:
        static const char *m_sClassName;
        // Find a point fast.
        int CompareKey( int id, CPointSort* pBase, bool fNoSpaces ) const;
    public:
        CObjPointSortArray() = default;
    private:
        CObjPointSortArray(const CObjPointSortArray& copy);
        CObjPointSortArray& operator=(const CObjPointSortArray& other);
    };

protected:
	uchar m_map;    // sector map
    short _x, _y;   // x and y (row and column) of the sector in the map
	int	m_index;    // sector index

public:
	static const char  *m_sClassName;
	CObjPointSortArray	m_Teleports;		//	CTeleport array
	CRegionLinks		m_RegionLinks;		//	CRegion(s) in this CSector
	dword			    m_dwFlags;

	CCharsActiveList		m_Chars_Active;		// CChar(s) activte in this CSector.
	CCharsDisconnectList	m_Chars_Disconnect;	// dead NPCs, etc
	CItemsList m_Items;				// CItem(s) in this CSector (not relevant if they have a timer set or not).

private:
    CSector* _ppAdjacentSectors[DIR_QTY];

public:
    /*
    * @brief Assign its adjacent sectors
    */
    void SetAdjacentSectors();

protected:
    CSector *_GetAdjacentSector(DIR_TYPE dir) const;

public:
	CSectorBase();
	virtual ~CSectorBase() = default;

private:
	CSectorBase(const CSectorBase& copy);
	CSectorBase& operator=(const CSectorBase& other);

public:
	virtual void Init(int index, uchar map, short x, short y);

	// Location map units.
	int GetIndex() const noexcept { return m_index; }
	int GetMap() const noexcept { return m_map; }
	CPointMap GetBasePoint() const;
	CRectMap GetRect() const noexcept;
	bool IsInDungeon() const;

	// CRegion
	CRegion * GetRegion( const CPointBase & pt, dword dwType ) const;
	size_t GetRegions( const CPointBase & pt, dword dwType, CRegionLinks *pRLinks ) const;

	bool UnLinkRegion( CRegion * pRegionOld );
	bool LinkRegion( CRegion * pRegionNew );

	// CTeleport(s) in the region.
	CTeleport * GetTeleport( const CPointMap & pt ) const;
	bool AddTeleport( CTeleport * pTeleport );

	bool IsFlagSet( dword dwFlag ) const noexcept;

#define SECF_NoSleep	0x00000001
#define SECF_InstaSleep	0x00000002
};

#endif // _INC_CSECTORTEMPLATE_H
