/**
* @file CSectorBase.h
*
*/

#ifndef _INC_CSECTORBASE_H
#define _INC_CSECTORBASE_H

#include "../common/sphere_library/CSObjCont.h"
#include "../common/sphere_library/CSObjSortArray.h"
#include "../common/CRect.h"
#include "CTeleport.h"

class CChar;
class CItem;
class CSector;
class CTeleport;

class CSectorObjCont
{
    // Marker class, consider it as a "tag".
};

class CCharsDisconnectList : public CSObjCont, public CSectorObjCont
{
public:
	CCharsDisconnectList();
    virtual ~CCharsDisconnectList();
	CCharsDisconnectList(const CCharsDisconnectList& copy) = delete;
	CCharsDisconnectList& operator=(const CCharsDisconnectList& other) = delete;

	void AddCharDisconnected(CChar* pChar);
};

class CCharsActiveList : public CSObjCont, public CSectorObjCont
{
private:
	int m_iClients;				// How many clients in this sector now?
	int64 m_iTimeLastClient;	// age the sector based on last client here.

protected:
	void OnRemoveObj(CSObjContRec* pObjRec);	// Override this = called when removed from list.

public:
	CCharsActiveList();
    ~CCharsActiveList();
	CCharsActiveList(const CCharsActiveList& copy) = delete;
	CCharsActiveList& operator=(const CCharsActiveList& other) = delete;

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
};

class CItemsList : public CSObjCont, public CSectorObjCont
{
public:
	CItemsList() = default;
	CItemsList(const CItemsList& copy) = delete;
	CItemsList& operator=(const CItemsList& other) = delete;

	void AddItemToSector( CItem * pItem );

protected:
	void OnRemoveObj(CSObjContRec* pObRec);	// Override this = called when removed from list.
};


class CSectorBase		// world sector
{
	template <typename T>
    class CObjPointSortArray : public CSObjSortArray< T*, int >
    {
		static_assert(std::is_base_of_v<CPointSort, T>, "Type has to inherit from CPointSort");

    public:
        static const char *m_sClassName;

		CObjPointSortArray() = default;
		virtual ~CObjPointSortArray() override = default;

		CObjPointSortArray(const CObjPointSortArray& copy) = delete;
		CObjPointSortArray& operator=(const CObjPointSortArray& other) = delete;

        // Find a point fast.
		virtual int CompareKey(int id, T* pBase, bool fNoSpaces) const override {
			UnreferencedParameter(fNoSpaces);
			return (id - pBase->GetPointSortIndex());
		}
    };


protected:
    int	m_index;            // Sector index in the 'sector grid'
    CPointBase m_BasePointSectUnits; // Sector coordinates in the 'sector grid'.
    CRectMap   m_MapRectWorldUnits;   // Map area inside this sector, in map coordinates.

private:
    // TODO: store indices instead of pointers, to make the class smaller?
	CSector* _ppAdjacentSectors[DIR_QTY];

public:
	static const char  *m_sClassName;
	CObjPointSortArray<CTeleport>	m_Teleports;		//	CTeleport array
	CRegionLinks		m_RegionLinks;		//	CRegion(s) in this CSector
	dword			    m_dwFlags;

	CCharsActiveList		m_Chars_Active;		// CChar(s) activte in this CSector.
	CCharsDisconnectList	m_Chars_Disconnect;	// dead NPCs, etc
    CItemsList m_Items;                         // CItem(s) in this CSector (not relevant if they have a timer set or not).

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

	CSectorBase(const CSectorBase& copy) = delete;
	CSectorBase& operator=(const CSectorBase& other) = delete;

public:
	virtual void Init(int index, uchar map, short x, short y);

	// Location map units.
    int GetIndex() const noexcept               { return m_index; }
    int GetMap() const noexcept                 { return m_BasePointSectUnits.m_map; }
    CPointBase GetBasePointMapUnits() const noexcept;
    constexpr const CPointBase& GetBasePointSectUnits() noexcept    { return m_BasePointSectUnits; }
    constexpr const CRectMap& GetRectWorldUnits() const noexcept    { return m_MapRectWorldUnits; }

	// CRegion
	CRegion * GetRegion( const CPointBase & pt, dword dwType ) const;
	size_t GetRegions( const CPointBase & pt, dword dwType, CRegionLinks *pRLinks ) const;

	bool UnLinkRegion( CRegion * pRegionOld );
	bool LinkRegion( CRegion * pRegionNew );

	// CTeleport(s) in the region.
	CTeleport * GetTeleport( const CPointMap & pt ) const;
	bool AddTeleport( CTeleport * pTeleport );

    constexpr bool IsFlagSet( dword dwFlag ) const noexcept {
        return (m_dwFlags & dwFlag);
    }

#define SECF_NoSleep	0x00000001
#define SECF_InstaSleep	0x00000002
};

#endif // _INC_CSECTORBASE_H
