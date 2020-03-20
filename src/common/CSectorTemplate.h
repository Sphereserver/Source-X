/**
* @file CSectorTemplate.h
*
*/

#ifndef _INC_CSECTORTEMPLATE_H
#define _INC_CSECTORTEMPLATE_H

#include "../game/CServer.h"
#include "../game/CRegion.h"


class CItem;
class CSector;

class CCharsDisconnectList : public CSObjList
{
public:
	static const char *m_sClassName;
public:
	CCharsDisconnectList() = default;
    void AddCharDisconnected( CChar * pChar );
private:
	CCharsDisconnectList(const CCharsDisconnectList& copy);
	CCharsDisconnectList& operator=(const CCharsDisconnectList& other);
};

class CCharsActiveList : public CSObjList
{
private:
    int m_iClients; // How many clients in this sector now?
public:
	static const char *m_sClassName;
	int64 m_timeLastClient;	// age the sector based on last client here.

protected:
	void OnRemoveObj( CSObjListRec* pObjRec );	// Override this = called when removed from list.

public:
    int GetClientsNumber() const;
	void ClientIncrease();
	void ClientDecrease();
	void AddCharActive( CChar * pChar );

public:
	CCharsActiveList();

private:
	CCharsActiveList(const CCharsActiveList& copy);
	CCharsActiveList& operator=(const CCharsActiveList& other);
};

class CItemsList : public CSObjList
{
	// Top level list of items.
public:
	static bool sm_fNotAMove;	// hack flag to prevent items from bouncing around too much.

protected:
	void OnRemoveObj( CSObjListRec* pObRec );	// Override this = called when removed from list.

public:
	static const char *m_sClassName;
	void AddItemToSector( CItem * pItem );

public:
	CItemsList() = default;

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
	int m_map;      // sector map
    int	m_index;    // sector index
    int _x, _y;     // x and y (row and column) of the sector in the map

public:
	static const char  *m_sClassName;
	CObjPointSortArray	m_Teleports;		//	CTeleport array
	CRegionLinks		m_RegionLinks;		//	CRegion(s) in this CSector
	dword			    m_dwFlags;

	CCharsActiveList		m_Chars_Active;		// CChar(s) activte in this CSector.
	CCharsDisconnectList	m_Chars_Disconnect;	// dead NPCs, etc
	CItemsList m_Items_Timer;				// CItem(s) in this CSector that need timers.
	CItemsList m_Items_Inert;				// CItem(s) in this CSector. (no timer required)

private:
    CSector* _ppAdjacentSectors[DIR_QTY];

public:
    /*
    * @brief Asign it's adjacent's sectors
    */
    void SetAdjacentSectors();
    CSector *GetAdjacentSector(DIR_TYPE dir) const;

	CSectorBase();
	virtual ~CSectorBase() = default;

private:
	CSectorBase(const CSectorBase& copy);
	CSectorBase& operator=(const CSectorBase& other);

public:
	void Init(int index, int map, int x, int y);

	// Location map units.
	int GetIndex() const { return m_index; }
	int GetMap() const { return m_map; }
	CPointMap GetBasePoint() const;
	CRectMap GetRect() const;
	bool IsInDungeon() const;

	// CRegion
	CRegion * GetRegion( const CPointBase & pt, dword dwType ) const;
	size_t GetRegions( const CPointBase & pt, dword dwType, CRegionLinks *pRLinks ) const;

	bool UnLinkRegion( CRegion * pRegionOld );
	bool LinkRegion( CRegion * pRegionNew );

	// CTeleport(s) in the region.
	CTeleport * GetTeleport( const CPointMap & pt ) const;
	bool AddTeleport( CTeleport * pTeleport );

	bool IsFlagSet( dword dwFlag ) const;

#define SECF_NoSleep	0x00000001
#define SECF_InstaSleep	0x00000002
};

#endif // _INC_CSECTORTEMPLATE_H
