#include "CWorldSearch.h"
#include "CSector.h"
#include "../sphere/threads.h"
#include "../game/chars/CChar.h"
#include "../game/items/CItem.h"


CWorldSearch::CWorldSearch(const CPointMap& pt, int iDist) noexcept :
	_pt(pt), _iDist(iDist), _fAllShow(false), _fSearchSquare(false),
	_eSearchType(ws_search_e::None), _fInertToggle(false),
	_ppCurContObjs(nullptr), _pObj(nullptr),
	_idxObj(0), _idxObjMax(0),
	_iSectorCur(0)  // Get upper left of search rect.
{
	// define a search of the world.
	_pSectorBase = _pSector = pt.GetSector();
	_rectSector.SetRect(
		pt.m_x - iDist,
		pt.m_y - iDist,
		pt.m_x + iDist + 1,
		pt.m_y + iDist + 1,
		pt.m_map);
}

CWorldSearch::~CWorldSearch() noexcept
{
	if (nullptr != _ppCurContObjs)
		delete[] _ppCurContObjs;
}

void CWorldSearch::SetAllShow(bool fView)
{
	//ADDTOCALLSTACK_INTENSIVE("CWorldSearch::SetAllShow");
	_fAllShow = fView;
}

void CWorldSearch::SetSearchSquare(bool fSquareSearch)
{
	//ADDTOCALLSTACK_INTENSIVE("CWorldSearch::SetSearchSquare");
	_fSearchSquare = fSquareSearch;
}

void CWorldSearch::RestartSearch()
{
	//ADDTOCALLSTACK_INTENSIVE("CWorldSearch::RestartSearch");
	_eSearchType = ws_search_e::None;
	_pObj = nullptr;
	_idxObj = _idxObjMax = 0;
}

bool CWorldSearch::GetNextSector()
{
	ADDTOCALLSTACK("CWorldSearch::GetNextSector");
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
		_idxObj = _idxObjMax = 0;

		return true;
	}
}

CItem* CWorldSearch::GetItem()
{
	// This method is called very frequently, ADDTOCALLSTACK unneededly sucks cpu
	//ADDTOCALLSTACK_INTENSIVE("CWorldSearch::GetItem");

	constexpr size_t kuiContainerScaleFactor = 2;
	while (true)
	{
		if (_pObj == nullptr)
		{
			ASSERT(_eSearchType == ws_search_e::None);
			_eSearchType = ws_search_e::Items;

			const size_t sector_obj_num = _pSector->m_Items.size();
			if (0 != sector_obj_num)
			{
				if (_ppCurContObjs != nullptr)
				{
					if (_idxObjMax < sector_obj_num * kuiContainerScaleFactor)
					{
						delete[] _ppCurContObjs;
						_ppCurContObjs = new CSObjContRec * [sector_obj_num * kuiContainerScaleFactor];
					}
				}
				else
				{
					_ppCurContObjs = new CSObjContRec * [sector_obj_num * kuiContainerScaleFactor];
				}

				memcpy(_ppCurContObjs, _pSector->m_Items.data(), sector_obj_num * sizeof(CSObjContRec*)); // I need this to be as fast as possible
			}

			_idxObjMax = sector_obj_num;
			_idxObj = 0;
		}
		else
		{
			++_idxObj;
		}

		ASSERT(_eSearchType == ws_search_e::Items);
		_pObj = (_idxObj >= _idxObjMax) ? nullptr : static_cast <CObjBase*> (_ppCurContObjs[_idxObj]);
		if (_pObj == nullptr)
		{
			if (GetNextSector())
				continue;

			return nullptr;
		}

		const CPointMap& ptObj = _pObj->GetTopPoint();
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
	//ADDTOCALLSTACK_INTENSIVE("CWorldSearch::GetChar");

	constexpr size_t kuiContainerScaleFactor = 2;
	while (true)
	{
		if (_pObj == nullptr)
		{
			ASSERT(_eSearchType == ws_search_e::None);
			_eSearchType = ws_search_e::Chars;
			_fInertToggle = false;

			const size_t sector_obj_num = _pSector->m_Chars_Active.size();
			if (0 != sector_obj_num)
			{
				if (_ppCurContObjs != nullptr)
				{
					if (_idxObjMax < sector_obj_num * kuiContainerScaleFactor)
					{
						delete[] _ppCurContObjs;
						_ppCurContObjs = new CSObjContRec * [sector_obj_num * kuiContainerScaleFactor];
					}
				}
				else
				{
					_ppCurContObjs = new CSObjContRec * [sector_obj_num * kuiContainerScaleFactor];
				}
				memcpy(_ppCurContObjs, _pSector->m_Chars_Active.data(), sector_obj_num * sizeof(CSObjContRec*)); // I need this to be as fast as possible
			}

			_idxObjMax = sector_obj_num;
			_idxObj = 0;
		}
		else
		{
			++_idxObj;
		}

		ASSERT(_eSearchType == ws_search_e::Chars);
		_pObj = (_idxObj >= _idxObjMax) ? nullptr : static_cast <CObjBase*> (_ppCurContObjs[_idxObj]);
		if (_pObj == nullptr)
		{
			if (!_fInertToggle && _fAllShow)
			{
				_fInertToggle = true;

				const size_t sector_obj_num = _pSector->m_Chars_Disconnect.size();
				if (0 != sector_obj_num)
				{
					if (_ppCurContObjs != nullptr)
					{
						if (_idxObjMax < sector_obj_num * kuiContainerScaleFactor)
						{
							delete[] _ppCurContObjs;
							_ppCurContObjs = new CSObjContRec * [sector_obj_num * kuiContainerScaleFactor];
						}
					}
					else
					{
						_ppCurContObjs = new CSObjContRec * [sector_obj_num * 2];
					}
					memcpy(_ppCurContObjs, _pSector->m_Chars_Disconnect.data(), sector_obj_num * sizeof(CSObjContRec*)); // I need this to be as fast as possible
				}

				_idxObjMax = sector_obj_num;
				_idxObj = 0;

				_pObj = (_idxObj >= _idxObjMax) ? nullptr : static_cast <CObjBase*> (_ppCurContObjs[_idxObj]);
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
