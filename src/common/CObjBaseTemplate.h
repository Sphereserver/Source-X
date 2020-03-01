/**
* @file CObjBaseTemplate.h
* @brief Base functionalities for every CObjBase.
*/

#ifndef _INC_COBJBASETEMPLATE_H
#define _INC_COBJBASETEMPLATE_H

#include "sphere_library/CSObjListRec.h"
#include "sphere_library/CSString.h"
#include "CRect.h"
#include "CUID.h"


class CObjBaseTemplate : public CSObjListRec
{
	// A dynamic object of some sort.
private:
	CUID		m_UID;		// How the server will refer to this. 0 = static item
	CSString	m_sName;	// unique name for the individual object.
	CPointMap	m_pt;		// List is sorted by m_z_sort.
protected:
	void DupeCopy( const CObjBaseTemplate * pObj ); // it's not a virtual, pay attention to the argument of this method!

	void SetUID( dword dwIndex );
	void SetUnkZ( char z );

public:
	static const char *m_sClassName;
	CObjBaseTemplate() = default;
	virtual ~CObjBaseTemplate() = default;

private:
	CObjBaseTemplate(const CObjBaseTemplate& copy);
	CObjBaseTemplate& operator=(const CObjBaseTemplate& other);

public:
	CObjBaseTemplate * GetNext() const;
	CObjBaseTemplate * GetPrev() const;

    const CUID& GetUID() const {
		return m_UID; 
	}
	bool IsItem() const {
		return m_UID.IsItem();
	}
	bool IsChar() const {
		return m_UID.IsChar();
	}
	bool IsItemInContainer() const {
		return m_UID.IsItemInContainer();
	}
	bool IsItemEquipped() const {
		return m_UID.IsItemEquipped();
	}
	bool IsDisconnected() const {
		return m_UID.IsObjDisconnected();
	}
	bool IsTopLevel() const {
		return m_UID.IsObjTopLevel();
	}
	bool IsValidUID() const {
		return m_UID.IsValidUID();
	}
	bool IsDeleted() const;

	void SetUIDContainerFlags(dword dwFlags) {
		m_UID.SetObjContainerFlags( dwFlags );
	}
    void RemoveUIDFlags(dword dwFlags) {
        m_UID.RemoveObjFlags( dwFlags );
    }

	virtual int IsWeird() const;
	virtual const CObjBaseTemplate * GetTopLevelObj() const = 0;
	virtual CObjBaseTemplate* GetTopLevelObj() = 0;

	CSector * GetTopSector() const;
	// Location

    LAYER_TYPE GetEquipLayer() const {
        return (LAYER_TYPE)(m_pt.m_z);
    }
	void SetEquipLayer( LAYER_TYPE layer );

    inline byte GetContainedLayer() const {
        // used for corpse or Restock count as well in Vendor container.
        return m_pt.m_z;
    }
	void SetContainedLayer( byte layer );
    inline const CPointMap & GetContainedPoint() const {
        return m_pt;
    }
	void SetContainedPoint( const CPointMap & pt );

	void SetTopPoint( const CPointMap & pt );
    inline const CPointMap & GetTopPoint() const {
        return m_pt;
    }
	virtual void SetTopZ( char z );
	char GetTopZ() const;
	uchar GetTopMap() const;

	void SetUnkPoint( const CPointMap & pt );
    inline const CPointMap & GetUnkPoint() const {
        // don't care where this
        return m_pt;
    }
	char GetUnkZ() const;

	// Distance and direction
	int GetTopDist( const CPointMap & pt ) const;

	int GetTopDist( const CObjBaseTemplate * pObj ) const;

	int GetTopDistSight( const CPointMap & pt ) const;

	int GetTopDistSight( const CObjBaseTemplate * pObj ) const;

	int GetDist( const CObjBaseTemplate * pObj ) const;

	int GetTopDist3D( const CObjBaseTemplate * pObj ) const;

	DIR_TYPE GetTopDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const;

	DIR_TYPE GetDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const;

	virtual int GetVisualRange() const;

	// Names
	lpctstr GetIndividualName() const;
	bool IsIndividualName() const;
	virtual lpctstr GetName() const;
	virtual bool SetName( lpctstr pszName );
};

#endif // _INC_COBJBASETEMPLATE_H
