/**
* @file CObjBaseTemplate.h
* @brief Base functionalities for every CObjBase.
*/

#ifndef _INC_COBJBASETEMPLATE_H
#define _INC_COBJBASETEMPLATE_H

#include "../common/sphere_library/CSObjContRec.h"
#include "../common/sphere_library/CSString.h"
#include "../common/CPointBase.h"
#include "../common/CUID.h"


class CObjBaseTemplate : public CSObjContRec
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
	CObjBaseTemplate();
	virtual ~CObjBaseTemplate() = default;

	CObjBaseTemplate(const CObjBaseTemplate& copy) = delete;
	CObjBaseTemplate& operator=(const CObjBaseTemplate& other) = delete;

public:
    const CUID& GetUID() const noexcept {
		return m_UID;
	}
	bool IsItem() const noexcept {
		return m_UID.IsItem();
	}
	bool IsChar() const noexcept {
		return m_UID.IsChar();
	}
	bool IsItemInContainer() const noexcept {
		return m_UID.IsItemInContainer();
	}
	bool IsItemEquipped() const noexcept {
		return m_UID.IsItemEquipped();
	}
	bool IsDisconnected() const noexcept {
		return m_UID.IsObjDisconnected();
	}
	bool IsTopLevel() const noexcept {
		return m_UID.IsObjTopLevel();
	}
	bool IsValidUID() const noexcept {
		return m_UID.IsValidUID();
	}

	void SetUIDContainerFlags(dword dwFlags) noexcept {
		m_UID.SetObjContainerFlags( dwFlags );
	}
    void RemoveUIDFlags(dword dwFlags) noexcept {
        m_UID.RemoveObjFlags( dwFlags );
    }

	// Attributes
	virtual int IsWeird() const;

	// Parent objects
	virtual const CObjBaseTemplate* GetTopLevelObj() const {
		return this;
	}
	virtual CObjBaseTemplate* GetTopLevelObj() {
		return this;
	}


	// Location

    LAYER_TYPE GetEquipLayer() const noexcept {
        return (LAYER_TYPE)(m_pt.m_z);
    }
	void SetEquipLayer( LAYER_TYPE layer );

    inline byte GetContainedLayer() const noexcept {
        // used for corpse or Restock count as well in Vendor container.
        return m_pt.m_z;
    }
	void SetContainedLayer( byte layer ) noexcept;
    inline const CPointMap & GetContainedPoint() const noexcept {
        return m_pt;
    }
	void SetContainedPoint( const CPointMap & pt ) noexcept;

	// - *Top* methods: are virtual and may do additional checks.
	void SetTopPoint(const CPointMap& pt);
	virtual void SetTopZ(char z);
    inline const CPointMap & GetTopPoint() const noexcept {
        return m_pt;
    }
	char GetTopZ() const noexcept;
	uchar GetTopMap() const noexcept;
	CSector* GetTopSector() const noexcept;

	// - *Unk* methods: are not virtual and get/set raw values, without any check.
	void SetUnkPoint(const CPointMap& pt) noexcept {
		m_pt = pt;
	}
    inline const CPointMap & GetUnkPoint() const noexcept {
        // don't care where this
        return m_pt;
    }
	inline char GetUnkZ() const noexcept {
		return m_pt.m_z;
	}


	// Distance and direction

	int GetTopDist( const CPointMap & pt ) const;

	int GetTopDist( const CObjBaseTemplate * pObj ) const;

	int GetTopDistSight( const CPointMap & pt ) const;

	int GetTopDistSight( const CObjBaseTemplate * pObj ) const;

	int GetDist( const CObjBaseTemplate * pObj ) const;

	int GetTopDist3D( const CObjBaseTemplate * pObj ) const;

	DIR_TYPE GetTopDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const; // Can return DIR_QTY if are on the same tile!

	DIR_TYPE GetDir( const CObjBaseTemplate * pObj, DIR_TYPE DirDefault = DIR_QTY ) const; // Can return DIR_QTY if are on the same tile!

	virtual int GetVisualRange() const;

	// Names
	lpctstr GetIndividualName() const;
	bool IsIndividualName() const;
	virtual lpctstr GetName() const;
	virtual bool SetName( lpctstr pszName );
};

#endif // _INC_COBJBASETEMPLATE_H
