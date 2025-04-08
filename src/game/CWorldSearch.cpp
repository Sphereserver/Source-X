#include "../common/CException.h"
#include "../common/CLog.h"
#include "../game/chars/CChar.h"
#include "../game/items/CItem.h"
#include "CSector.h"
#include "CWorldSearch.h"
#include <array>

static constexpr size_t kuiContainerScaleFactor = 2;


class CWorldSearchHolderImpl
{
    static constexpr size_t kuiPreallocateSize = 20;
    static constexpr size_t kuiCachedInstances = 20;
    std::array<CSReferenceCountedOwned<CWorldSearch>, kuiCachedInstances> _instances;

public:
    ~CWorldSearchHolderImpl() noexcept = default;
    CWorldSearchHolderImpl() noexcept = default;

    CSReferenceCounted<CWorldSearch> GetOne(const CPointMap& pt, int iDist)
    {
        for (auto& inst : _instances)
        {
            //if (inst.use_count() == 1)
            if (inst._counted_references == 1)
            {
                // It's free, it's only referenced by me (i'm owning it).
                inst->Reset(pt, iDist);
                return inst.GetRef();
            }
        }

        // No free instances!
        throw CSError(LOGL_CRIT, 0, "Not enough instances of CWorldSearch!");
    }
};

//--------------

CSReferenceCounted<CWorldSearch> CWorldSearchHolder::GetInstance(const CPointMap& pt, int iDist)    // static
{
    static CWorldSearchHolderImpl holder;
    return holder.GetOne(pt, iDist);
}

CWorldSearch::CWorldSearch() noexcept :
    _iDist(0), _fAllShow(false), _fSearchSquare(false),
    _eSearchType(ws_search_e::None), _fInertToggle(false),
    _ppCurContObjs(nullptr), _pObj(nullptr),
    _uiCurObjIndex(0), _uiObjArrayCapacity(0), _uiObjArraySize(0),
    _iSectorCur(0),  // Get upper left of search rect.
    _pSectorBase(nullptr), _pSector(nullptr)
{
}

CWorldSearch::CWorldSearch(size_t uiPreallocateSize) :
    CWorldSearch()
{
    if (!uiPreallocateSize)
        return;

    _uiObjArrayCapacity = uiPreallocateSize * kuiContainerScaleFactor;
    _ppCurContObjs = new CSObjContRec* [_uiObjArrayCapacity];
}

CWorldSearch::~CWorldSearch() noexcept
{
    if (nullptr != _ppCurContObjs)
        delete[] _ppCurContObjs;
}

void CWorldSearch::Reset(const CPointMap& pt, int iDist)
{
    //ADDTOCALLSTACK("CWorldSearch::Reset");
    // define a search of the world.

    _fAllShow = false;
    _fSearchSquare = false;
    _eSearchType = ws_search_e::None;
    _fInertToggle = false;
    _pObj = nullptr;
    _uiCurObjIndex = 0;
    _uiObjArraySize = 0;
    //_uiObjArrayCapacity = 0;   // Don't! Recycle the allocated space for _ppCurContObjs.
    _iSectorCur = 0; // Get upper left of search rect.

    _pt = pt;
    _iDist = iDist;
    _pSectorBase = _pSector = pt.GetSector();
    _rectSector.SetRect(
        pt.m_x - iDist,
        pt.m_y - iDist,
        pt.m_x + iDist + 1,
        pt.m_y + iDist + 1,
        pt.m_map);
}

void CWorldSearch::SetAllShow(bool fView)
{
	//ADDTOCALLSTACK_DEBUG("CWorldSearch::SetAllShow");
	_fAllShow = fView;
}

void CWorldSearch::SetSearchSquare(bool fSquareSearch)
{
	//ADDTOCALLSTACK_DEBUG("CWorldSearch::SetSearchSquare");
	_fSearchSquare = fSquareSearch;
}

void CWorldSearch::RestartSearch()
{
	//ADDTOCALLSTACK_DEBUG("CWorldSearch::RestartSearch");
	_eSearchType = ws_search_e::None;
	_pObj = nullptr;
    _uiCurObjIndex = _uiObjArraySize = 0;
    _pSector = _pSectorBase;
    // We could memset 0 _ppCurContObjs, but it isn't actually necessary...
}

bool CWorldSearch::GetNextSector()
{
    ADDTOCALLSTACK_DEBUG("CWorldSearch::GetNextSector");
	// Move search into nearby CSector(s) if necessary

	if (!_iDist)
		return false;

	while (true)
	{
		_pSector = _rectSector.GetSector(_iSectorCur++);
		if (_pSector == nullptr)
			return false;	// done searching.
		if (_pSectorBase == _pSector)
			continue;	// same as base.

		_eSearchType = ws_search_e::None;
		_pObj = nullptr;	// start at head of next Sector.
        _uiCurObjIndex = _uiObjArraySize = 0;

		return true;
	}
}

void CWorldSearch::LoadSectorObjs(CSObjCont const& pSectorObjList)
{
    ADDTOCALLSTACK_DEBUG("CWorldSearch::LoadSectorObjs");
    const size_t sector_obj_num = pSectorObjList.size();
    if (0 != sector_obj_num)
    {
        bool fAllocate = false;
        if (_ppCurContObjs != nullptr)
        {
            if (_uiObjArrayCapacity < sector_obj_num)
            {
                delete[] _ppCurContObjs;
                fAllocate = true;
            }
        }
        else
        {
            fAllocate = true;
        }

        if (fAllocate)
        {
            _uiObjArrayCapacity = sector_obj_num * kuiContainerScaleFactor;
            _ppCurContObjs = new CSObjContRec * [_uiObjArrayCapacity];
        }

        memcpy(_ppCurContObjs, pSectorObjList.data(), sector_obj_num * sizeof(CSObjContRec*)); // I need this to be as fast as possible
    }

    _uiObjArraySize = sector_obj_num;
    _uiCurObjIndex = 0;
}

CItem* CWorldSearch::GetItem()
{
	// This method is called very frequently, ADDTOCALLSTACK unneededly sucks cpu
	//ADDTOCALLSTACK_DEBUG("CWorldSearch::GetItem");

	while (true)
	{
		if (_pObj == nullptr)
		{
			ASSERT(_eSearchType == ws_search_e::None);
			_eSearchType = ws_search_e::Items;
            
            LoadSectorObjs(_pSector->m_Items);
		}
		else
		{
            ++_uiCurObjIndex;
		}

		ASSERT(_eSearchType == ws_search_e::Items);
        _pObj = (_uiCurObjIndex >= _uiObjArraySize) ? nullptr : static_cast<CObjBase*>(_ppCurContObjs[_uiCurObjIndex]);
		if (_pObj == nullptr)
		{
			if (GetNextSector())
				continue;

			return nullptr;
		}

		const CPointMap& ptObj(_pObj->GetTopPoint());
		if (_fSearchSquare)
		{
			if (_fAllShow)
			{
				if (_pt.GetDistSightBase(ptObj) <= _iDist)
					return static_cast <CItem*> (_pObj);
			}
			else
			{
				if (_pt.GetDistSight(ptObj) <= _iDist)
					return static_cast <CItem*> (_pObj);
			}
		}
		else
		{
			if (_fAllShow)
			{
				if (_pt.GetDistBase(ptObj) <= _iDist)
					return static_cast <CItem*> (_pObj);
			}
			else
			{
				if (_pt.GetDist(ptObj) <= _iDist)
					return static_cast <CItem*> (_pObj);
			}
		}
	}
}

CChar* CWorldSearch::GetChar()
{
	// This method is called very frequently, ADDTOCALLSTACK unneededly sucks cpu
	//ADDTOCALLSTACK_DEBUG("CWorldSearch::GetChar");

	while (true)
	{
		if (_pObj == nullptr)
		{
			ASSERT(_eSearchType == ws_search_e::None);
			_eSearchType = ws_search_e::Chars;
			_fInertToggle = false;

            LoadSectorObjs(_pSector->m_Chars_Active);
		}
		else
		{
            ++_uiCurObjIndex;
		}

		ASSERT(_eSearchType == ws_search_e::Chars);
        _pObj = (_uiCurObjIndex >= _uiObjArraySize) ? nullptr : static_cast<CObjBase*>(_ppCurContObjs[_uiCurObjIndex]);
		if (_pObj == nullptr)
		{
			if (!_fInertToggle && _fAllShow)
			{
				_fInertToggle = true;

                LoadSectorObjs(_pSector->m_Chars_Disconnect);
                _pObj = (_uiCurObjIndex >= _uiObjArraySize) ? nullptr : static_cast<CObjBase*>(_ppCurContObjs[_uiCurObjIndex]);
				if (_pObj != nullptr)
					goto jumpover;
			}

			if (GetNextSector())
				continue;

			return nullptr;
		}

	jumpover:
		const CPointMap& ptObj = _pObj->GetTopPoint();

		if (_fSearchSquare)
		{
			if (_fAllShow)
			{
				if (_pt.GetDistSightBase(ptObj) <= _iDist)
					return static_cast <CChar*> (_pObj);
			}
			else
			{
				if (_pt.GetDistSight(ptObj) <= _iDist)
					return static_cast <CChar*> (_pObj);
			}
		}
		else
		{
			if (_fAllShow)
			{
				if (_pt.GetDistBase(ptObj) <= _iDist)
					return static_cast <CChar*> (_pObj);
			}
			else
			{
				if (_pt.GetDist(ptObj) <= _iDist)
					return static_cast <CChar*> (_pObj);
			}
		}
	}
}
