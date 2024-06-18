#ifndef _INC_CWORLDSEARCH_H
#define _INC_CWORLDSEARCH_H

#include "../common/CRect.h"

class CSObjContRec;
class CObjBase;
class CSector;
class CChar;
class CItem;


class CWorldSearch	// Search for dynamic objects on a specified zone
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

    CSObjContRec** _ppCurContObjs;	// Sector-attached object container in which we are searching right now.
    CObjBase* _pObj;			// The current object of interest.
    size_t				_idxObj, _idxObjMax;

    int		 _iSectorCur;		// What is the current Sector index in m_rectSector
    CSector* _pSectorBase;		// Don't search the center sector 2 times.
    CSector* _pSector;			// current Sector
    CRectMap _rectSector;		// A rectangle containing our sectors we can search.

public:
    static const char* m_sClassName;
    CWorldSearch(const CWorldSearch& copy) = delete;
    CWorldSearch& operator=(const CWorldSearch& other) = delete;

    explicit CWorldSearch(const CPointMap& pt, int iDist = 0) noexcept;
    ~CWorldSearch() noexcept;

    void SetAllShow(bool fView);
    void SetSearchSquare(bool fSquareSearch);
    void RestartSearch();
    CChar* GetChar();
    CItem* GetItem();

private:
    bool GetNextSector();
};


#endif  // _INC_CWORLDSEARCH_H
