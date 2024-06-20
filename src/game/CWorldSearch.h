#ifndef _INC_CWORLDSEARCH_H
#define _INC_CWORLDSEARCH_H

#include "../common/sphere_library/CSReferenceCount.h"
#include "../common/CRect.h"

/*
#ifdef _MSC_VER
#   pragma warning(push)
#   pragma warning(disable : 4244)
#   pragma warning(disable : 4267)
#   pragma warning(disable : 4324)
#endif

#include <cdrc/rc_ptr.h>

#ifdef _MSC_VER
#   pragma warning(pop)
#endif
*/

class CSObjContRec;
class CObjBase;
class CSector;
class CChar;
class CItem;


// Search for dynamic objects on a specified zone
class CWorldSearch
{
    enum class ws_search_e
    {
        None = 0,
        Items,
        Chars
    };

    CPointMap _pt;		// Base point of our search.
    int _iDist;			// How far from the point are we interested in
    bool _fAllShow;				// Include Even inert items.
    bool _fSearchSquare;		// Search in a square (uo-sight distance) rather than a circle (standard distance).

    ws_search_e _eSearchType;
    bool _fInertToggle;			// We are now doing the inert chars.

    CSObjContRec** _ppCurContObjs;	// Array with a copy of the pointers to the objects inside the sector we are in searching right now.
    CObjBase* _pObj;			// The current object of interest.
    size_t	_idxObj, _idxObjMax;

    int		 _iSectorCur;		// What is the current Sector index in m_rectSector
    CSector* _pSectorBase;		// Don't search the center sector 2 times.
    CSector* _pSector;			// current Sector
    CRectMap _rectSector;		// A rectangle containing our sectors we can search.

public:
    static const char* m_sClassName;
    CWorldSearch(const CWorldSearch& copy) = delete;
    CWorldSearch& operator=(const CWorldSearch& other) = delete;

    CWorldSearch() noexcept;
    CWorldSearch(size_t uiPreallocateSize);
    ~CWorldSearch() noexcept;

public:
    //static cdrc::rc_ptr<CWorldSearch> GetInstance(const CPointMap& pt, int iDist = 0);
    void Reset(const CPointMap& pt, int iDist = 0);
    void SetAllShow(bool fView);
    void SetSearchSquare(bool fSquareSearch);
    void RestartSearch();
    CChar* GetChar();
    CItem* GetItem();

private:
    bool GetNextSector();
};

struct CWorldSearchHolder
{
    static CSReferenceCounted<CWorldSearch> GetInstance(const CPointMap& pt, int iDist = 0);
};

#endif  // _INC_CWORLDSEARCH_H
