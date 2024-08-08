#include "../common/CException.h"
#include "../sphere/threads.h"
#include "../sphere/ProfileTask.h"
#include "chars/CChar.h"
#include "items/CItem.h"
#include "items/CItemShip.h"
#include "CSector.h"
#include "CWorldClock.h"
#include "CWorldGameTime.h"
#include "CWorldTicker.h"
#include <sstream>


CWorldTicker::CWorldTicker(CWorldClock *pClock)
{
    ASSERT(pClock);
    _pWorldClock = pClock;

    _iLastTickDone = 0;

    _vecObjs.reserve(50);
    _vecWorldObjsToEraseFromList.reserve(50);
    _vecPeriodicCharsToEraseFromList.reserve(25);
}


// CTimedObject TIMERs

void CWorldTicker::_InsertTimedObject(const int64 iTimeout, CTimedObject* pTimedObject)
{
    ASSERT(iTimeout != 0);

/*
#ifdef _DEBUG
    for (auto& elemList : _mWorldTickList)
    {
        for (auto& elem : elemList.second)
        {
            ASSERT(elem != pTimedObject);
        }
    }
#endif
*/

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_mWorldTickList.MT_CMUTEX);
#endif
    _mWorldTickList.emplace(iTimeout, pTimedObject);
}

void CWorldTicker::_RemoveTimedObject(const int64 iOldTimeout, CTimedObject* pTimedObject)
{
    ASSERT(iOldTimeout != 0);

    //g_Log.EventDebug("Trying to erase TimedObject 0x%p with old timeout %ld.\n", pTimedObject, iOldTimeout);
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_mWorldTickList.MT_CMUTEX);
#endif
    const auto itMap = _mWorldTickList.equal_range(iOldTimeout);
    decltype(_mWorldTickList)::const_iterator itFound = itMap.second;  // first element greater than the key we look for
    for (auto it = itMap.first; it != itMap.second; ++it)
    {
        // I have a pair of iterators for a range of the elements (all the elements with the same key)
        if (it->second == pTimedObject)
        {
            if (itFound != itMap.second)
            {
                g_Log.EventDebug("The same TimedObject is inserted multiple times in mWorldTickList. This shouldn't happen. Removing only the first one.\n");
            }
            itFound = it;
#if !defined(_DEBUG)
            break;
#endif
        }
    }
    if (itFound == itMap.second)
    {
        // Not found. The object might have a timeout while being in a non-tickable state, so it isn't in the list.
/*
#ifdef _DEBUG
        g_Log.EventDebug("Requested erasure of TimedObject in mWorldTickList, but it wasn't found.\n");
#endif
*/

/*
#ifdef _DEBUG
        for (auto& elemList : _mWorldTickList)
        {
            for (auto& elem : elemList.second)
            {
                ASSERT(elem != pTimedObject);
            }
        }
#endif
*/
        return;
    }
    _mWorldTickList.erase(itFound);
}

void CWorldTicker::AddTimedObject(const int64 iTimeout, CTimedObject* pTimedObject, bool fForce)
{
    //if (iTimeout < CWorldGameTime::GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible
    //    return;

    EXC_TRY("AddTimedObject");
    const ProfileTask timersTask(PROFILE_TIMERS);

    EXC_SET_BLOCK("Already ticking?");
    const int64 iTickOld = pTimedObject->_GetTimeoutRaw();
    if (iTickOld != 0)
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        EXC_SET_BLOCK("Remove");
        _RemoveTimedObject(iTickOld, pTimedObject);
    }

    EXC_SET_BLOCK("Insert");
    bool fCanTick;
    if (fForce)
    {
        fCanTick = true;
    }
    else
    {
        fCanTick = pTimedObject->_CanTick();
        if (!fCanTick)
        {
            if (auto pObjBase = dynamic_cast<const CObjBase*>(pTimedObject))
            {
                // Not yet placed in the world? We could have set the TIMER before setting its P or CONT, we can't know at this point...
                // In this case, add it to the list and check if it can tick in the tick loop. We have maybe useless object in the ticking list and this hampers
                //  performance, but it would be a pain to fix every script by setting the TIMER only after the item is placed in the world...
                fCanTick = !pObjBase->GetTopLevelObj()->GetTopPoint().IsValidPoint();
            }
        }
    }

    if (fCanTick)
    {
        _InsertTimedObject(iTimeout, pTimedObject);
    }

    EXC_CATCH;
}

void CWorldTicker::DelTimedObject(CTimedObject* pTimedObject)
{
    EXC_TRY("DelTimedObject");
    const ProfileTask timersTask(PROFILE_TIMERS);

    EXC_SET_BLOCK("Not ticking?");
    const int64 iTickOld = pTimedObject->_GetTimeoutRaw();
    if (iTickOld == 0)
        return;

    EXC_SET_BLOCK("Remove");
    _RemoveTimedObject(iTickOld, pTimedObject);

    EXC_CATCH;
}


// CChar Periodic Ticks (it's a different thing than TIMER!)

void CWorldTicker::_InsertCharTicking(const int64 iTickNext, CChar* pChar)
{
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_mCharTickList.MT_CMUTEX);
#endif


    _mCharTickList.emplace(iTickNext, pChar);
    pChar->_iTimePeriodicTick = iTickNext;
}

void CWorldTicker::_RemoveCharTicking(const int64 iOldTimeout, CChar* pChar)
{
    // I'm reasonably sure that the element i'm trying to remove is present in this container.
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_mCharTickList.MT_CMUTEX);
#endif

    const auto itMap = _mCharTickList.equal_range(iOldTimeout);
    decltype(_mCharTickList)::const_iterator itFound = itMap.second;  // first element greater than the key we look for
    for (auto it = itMap.first; it != itMap.second; ++it)
    {
        // I have a pair of iterators for a range of the elements (all the elements with the same key)
        if (it->second == pChar)
        {
            if (itFound != itMap.second)
            {
                g_Log.EventDebug("The same CChar is inserted multiple times in mCharTickList. This shouldn't happen. Removing only the first one.\n");
            }
            itFound = it;
#if !_DEBUG
            break;
#endif
        }
    }
    if (itFound == itMap.second)
    {
        return;
    }
    _mCharTickList.erase(itFound);


    pChar->_iTimePeriodicTick = 0;
}

void CWorldTicker::AddCharTicking(CChar* pChar, bool fNeedsLock)
{
    EXC_TRY("AddCharTicking");

    const ProfileTask timersTask(PROFILE_TIMERS);

    int64 iTickNext, iTickOld;
    if (fNeedsLock)
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(pChar->MT_CMUTEX);
#endif
        iTickNext = pChar->_iTimeNextRegen;
        iTickOld = pChar->_iTimePeriodicTick;
    }
    else
    {
        iTickNext = pChar->_iTimeNextRegen;
        iTickOld = pChar->_iTimePeriodicTick;
    }

    if (iTickNext == iTickOld)
        return;

    //if (iTickNext < CWorldGameTime::GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible
    //    return;

    if (iTickOld != 0)
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        EXC_SET_BLOCK("Remove");
        _RemoveCharTicking(iTickOld, pChar);
    }

/*
#ifdef _DEBUG
    for (auto& elemList : _mCharTickList)
    {
        for (auto& elemChar : elemList.second)
        {
            ASSERT(elemChar != pChar);
        }
    }
#endif
*/

    EXC_SET_BLOCK("Insert");
    _InsertCharTicking(iTickNext, pChar);

    EXC_CATCH;
}

void CWorldTicker::DelCharTicking(CChar* pChar, bool fNeedsLock)
{
    EXC_TRY("DelCharTicking");
    const ProfileTask timersTask(PROFILE_TIMERS);

    int64 iTickOld;
    if (fNeedsLock)
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(pChar->MT_CMUTEX);
#endif
        iTickOld = pChar->_iTimePeriodicTick;
    }
    else
    {
        iTickOld = pChar->_iTimePeriodicTick;
    }
    if (iTickOld == 0)
        return;

    EXC_SET_BLOCK("Remove");
    _RemoveCharTicking(iTickOld, pChar);

    EXC_CATCH;
}

void CWorldTicker::AddObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
    EXC_TRY("AddObjStatusUpdate");

    UnreferencedParameter(fNeedsLock);
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(_ObjStatusUpdates.MT_CMUTEX);
#endif
        _ObjStatusUpdates.insert(pObj);
    }

    EXC_CATCH;
}

void CWorldTicker::DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
    EXC_TRY("DelObjStatusUpdate");

    UnreferencedParameter(fNeedsLock);
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(_ObjStatusUpdates.MT_CMUTEX);
#endif
        _ObjStatusUpdates.erase(pObj);
    }

    EXC_CATCH;
}

// Check timeouts and do ticks

void CWorldTicker::Tick()
{
    ADDTOCALLSTACK("CWorldTicker::Tick");
    EXC_TRY("CWorldTicker::Tick");

    EXC_SET_BLOCK("Once per tick stuff");
    // Do this once per tick.
    //  Update status flags from objects, update current tick.
    if (_iLastTickDone <= _pWorldClock->GetCurrentTick())
    {
        ++_iLastTickDone;   // Update current tick.

        /* process objects that need status updates
        * these objects will normally be in containers which don't have any period _OnTick method
        * called (whereas other items can receive the OnTickStatusUpdate() call via their normal
        * tick method).
        * note: ideally, a better solution to accomplish this should be found if possible
        * TODO: implement a new class inheriting from CTimedObject to get rid of this code?
        */
        {
            EXC_TRYSUB("StatusUpdates");
            {
                EXC_SETSUB_BLOCK("Selection");
#if MT_ENGINES
                std::unique_lock<std::shared_mutex> lock_su(_ObjStatusUpdates.MT_CMUTEX);
#endif
                if (!_ObjStatusUpdates.empty())
                {
                    for (CObjBase* pObj : _ObjStatusUpdates)
                    {
                        if (pObj && !pObj->_IsBeingDeleted())
                            _vecObjs.emplace_back(static_cast<void*>(pObj));
                    }
                    _ObjStatusUpdates.clear();
                }
            }

            EXC_SETSUB_BLOCK("Loop");
            for (void* pObjVoid : _vecObjs)
            {
                CObjBase* pObj = static_cast<CObjBase*>(pObjVoid);
                pObj->OnTickStatusUpdate();
            }
            EXC_CATCHSUB("");

            _vecObjs.clear();
        }
    }


    /* World ticking (timers) */

    // Items, Chars ... Everything relying on CTimedObject (excepting CObjBase, which inheritance is only virtual)
    int64 iCurTime = CWorldGameTime::GetCurrentTime().GetTimeRaw();    // Current timestamp, a few msecs will advance in the current tick ... avoid them until the following tick(s).

    EXC_SET_BLOCK("TimedObjects");
    {
        const ProfileTask timersTask(PROFILE_TIMERS);
        {
            // Need here a new, inner scope to get rid of EXC_TRYSUB variables and for the unique_lock
            EXC_TRYSUB("Timed Objects Selection");
#if MT_ENGINES
            std::unique_lock<std::shared_mutex> lock(_mWorldTickList.MT_CMUTEX);
#endif

            WorldTickList::iterator itMap = _mWorldTickList.begin();
            WorldTickList::iterator itMapEnd = _mWorldTickList.end();

            size_t uiProgressive = 0;
            int64 iTime;
            while ((itMap != itMapEnd) && (iCurTime > (iTime = itMap->first)))
            {
                CTimedObject* pTimedObj = itMap->second;
                if (pTimedObj->_IsTimerSet() && pTimedObj->_CanTick())
                {
                    if (pTimedObj->_GetTimeoutRaw() <= iCurTime)
                    {
                        if (auto pObjBase = dynamic_cast<const CObjBase*>(pTimedObj))
                        {
                            if (pObjBase->_IsBeingDeleted())
                                continue;
                        }

                        _vecObjs.emplace_back(static_cast<void*>(pTimedObj));
                        _vecWorldObjsToEraseFromList.emplace_back(uiProgressive);

                        pTimedObj->_ClearTimeout();
                    }
                    //else
                    //{
                    //    // This shouldn't happen... If it does, get rid of the entry on the list anyways,
                    //    //  it got desynchronized in some way and might be an invalid or even deleted and deallocated object!
                    //}
                }
                ++itMap;
                ++uiProgressive;
            }

            EXC_CATCHSUB("AddToSubLists");
        }

        {
            EXC_TRYSUB("Timed Objects Delete from List");

            // Erase in chunks, call erase the least times possible.
            if (!_vecWorldObjsToEraseFromList.empty())
            {
                /*
                g_Log.EventDebug("-- Start WORLDTICK. I need to remove %lu items:\n", _vecWorldObjsToEraseFromList.size());
                std::stringstream ss;
                for (size_t elem : _vecWorldObjsToEraseFromList)
                {
                    ss << elem << ' ';
                }
                g_Log.EventDebug("%s\n", ss.str().c_str());
                */

                if (_vecWorldObjsToEraseFromList.size() > 1)
                {
                    size_t uiCurMapElemIdx = 0;
                    size_t uiCurVecElemIdx = 0;
                    //size_t uiSubRangeStartIdx = 0;
                    WorldTickList::iterator itSubRangeStart = _mWorldTickList.begin();
                    WorldTickList::iterator itMap = _mWorldTickList.begin();
                    bool fContiguous = true;
                    bool fFirstMatch = false;
                    while ((itMap != _mWorldTickList.end()) &&
                        (uiCurMapElemIdx <= _vecWorldObjsToEraseFromList.back()) &&
                        (uiCurVecElemIdx < _vecWorldObjsToEraseFromList.size()))
                    {
                        if (!fFirstMatch)
                        {
                            if (uiCurMapElemIdx == _vecWorldObjsToEraseFromList[uiCurVecElemIdx])
                            {
                                //uiSubRangeStartIdx = uiCurMapElemIdx;
                                itSubRangeStart = itMap;
                                fFirstMatch = true;

                                ++uiCurVecElemIdx;
                            }

                            ++itMap;
                            ++uiCurMapElemIdx;
                            continue;
                        }

                        if (uiCurMapElemIdx == _vecWorldObjsToEraseFromList[uiCurVecElemIdx])
                        {
                            // Matches. I want to delete this.
                            if (uiCurMapElemIdx == _vecWorldObjsToEraseFromList[uiCurVecElemIdx - 1] + 1)
                            {
                                // I want to delete it and it's contiguous, go on
                                ASSERT(fContiguous);
                            }
                            else
                            {
                                // It isn't contiguous. Go below.
                                ASSERT(!fContiguous);
                                // This is the first one that matches after previous mismatches. We start this chunk from here.
                                //uiSubRangeStartIdx = uiCurMapElemIdx;
                                fContiguous = true;
                            }

                            ++itMap;
                            ++uiCurMapElemIdx;
                            ++uiCurVecElemIdx;
                            continue;
                        }

                        // Not contiguous to the next element to be erased (stored in the vector).
                        //  What to do?
                        if (uiCurMapElemIdx != _vecWorldObjsToEraseFromList[uiCurVecElemIdx])
                        {
                            // I don't want to erase this.
                            if (!fContiguous)
                            {
                                // This is an element after the first one successive to the previous contiguous block (2nd, 3rd...)
                                // Ignore it.
                                //g_Log.EventDebug("Skip this %lu\n", uiCurMapElemIdx);
                                ++itMap;
                            }
                            else
                            {
                                // This is the first element after the previous contiguous block
                                // I want to erase until the previous one
                                // erase doesn't delete the last element in the range
                                itMap = _mWorldTickList.erase(itSubRangeStart, itMap);
                                //g_Log.EventDebug("Skip this %lu, not to be deleted, and...\n", uiCurMapElemIdx);
                                //g_Log.EventDebug("Erasing %lu items starting from pos %lu\n", (uiCurMapElemIdx - uiSubRangeStartIdx), uiSubRangeStartIdx);

                                ++itMap;
                                itSubRangeStart = itMap;
                                //uiSubRangeStartIdx = uiCurMapElemIdx;   // Not really needed
                                fContiguous = false;
                            }
                            ++uiCurMapElemIdx;
                            continue;
                        }

                       ASSERT(false);   // Shouldn't really be here.
                    }
                    if (fFirstMatch && fContiguous)
                    {
                        /*itMap =*/ _mWorldTickList.erase(itSubRangeStart, itMap); // last range to erase
                        //g_Log.EventDebug("(End cycle) Erasing %lu items starting from pos %lu\n", (uiCurMapElemIdx - uiSubRangeStartIdx), uiSubRangeStartIdx);
                    }
                }
                else
                {
                    _mWorldTickList.erase(std::next(_mWorldTickList.begin(), _vecWorldObjsToEraseFromList.front()));
                    //g_Log.EventDebug("Erasing 1 item.\n");
                }
            }

            EXC_CATCHSUB("DeleteFromList");
            _vecWorldObjsToEraseFromList.clear();
        }

        lpctstr ptcSubDesc;
        for (void* pObjVoid : _vecObjs)    // Loop through all msecs stored, unless we passed the timestamp.
        {
            ptcSubDesc = "Generic";

            EXC_TRYSUB("Timed Object Tick");
            EXC_SETSUB_BLOCK("Elapsed");

            CTimedObject* pTimedObj = static_cast<CTimedObject*>(pObjVoid);

#if MT_ENGINES
            std::unique_lock<std::shared_mutex> lockTimeObj(pTimedObj->MT_CMUTEX);
#endif

            const PROFILE_TYPE profile = pTimedObj->_GetProfileType();
            const ProfileTask  profileTask(profile);

            // Default to true, so if any error occurs it gets deleted for safety
            //  (valid only for classes having the Delete method, which, for everyone to know, does NOT destroy the object).
            bool fDelete = true;

            switch (profile)
            {
                case PROFILE_ITEMS:
                {
                    CItem* pItem = dynamic_cast<CItem*>(pTimedObj);
                    ASSERT(pItem);
                    if (pItem->IsItemEquipped())
                    {
                        ptcSubDesc = "ItemEquipped";
                        CObjBaseTemplate* pObjTop = pItem->GetTopLevelObj();
                        ASSERT(pObjTop);

                        CChar* pChar = dynamic_cast<CChar*>(pObjTop);
                        if (pChar)
                        {
                            fDelete = !pChar->OnTickEquip(pItem);
                            break;
                        }

                        ptcSubDesc = "Item (fallback)";
                        g_Log.Event(LOGL_CRIT, "Item equipped, but not contained in a character? (UID: 0%" PRIx32 ")\n.", pItem->GetUID().GetObjUID());
                    }
                    else
                    {
                        ptcSubDesc = "Item";
                    }
                    fDelete = (pItem->_OnTick() == false);
                    break;
                }
                break;

                case PROFILE_CHARS:
                {
                    ptcSubDesc = "Char";
                    CChar* pChar = dynamic_cast<CChar*>(pTimedObj);
                    ASSERT(pChar);
                    fDelete = !pChar->_OnTick();
                    if (!fDelete && pChar->m_pNPC && !pTimedObj->_IsTimerSet())
                    {
                        pTimedObj->_SetTimeoutS(3);   //3 seconds timeout to keep NPCs 'alive'
                    }
                }
                break;

                case PROFILE_SECTORS:
                {
                    ptcSubDesc = "Sector";
                    fDelete = false;    // sectors should NEVER be deleted.
                    pTimedObj->_OnTick();
                }
                break;

                case PROFILE_MULTIS:
                {
                    ptcSubDesc = "Multi";
                    fDelete = !pTimedObj->_OnTick();
                }
                break;

                case PROFILE_SHIPS:
                {
                    ptcSubDesc = "ItemShip";
                    fDelete = !pTimedObj->_OnTick();
                }
                break;

                case PROFILE_TIMEDFUNCTIONS:
                {
                    ptcSubDesc = "TimedFunction";
                    fDelete = false;
                    pTimedObj->_OnTick();
                }
                break;

                default:
                {
                    ptcSubDesc = "Default";
                    fDelete = !pTimedObj->_OnTick();
                }
                break;
            }

            if (fDelete)
            {
                EXC_SETSUB_BLOCK("Delete");
                CObjBase* pObjBase = dynamic_cast<CObjBase*>(pTimedObj);
                ASSERT(pObjBase); // Only CObjBase-derived objects have the Delete method, and should be Delete-d.
                pObjBase->Delete();
            }

            EXC_CATCHSUB(ptcSubDesc);
        }
    }

    _vecObjs.clear();

    // ----

    /* Periodic, automatic ticking for every char */

    const ProfileTask taskChars(PROFILE_CHARS);

    {
        // Need here a new, inner scope to get rid of EXC_TRYSUB variables, and for the unique_lock
        EXC_TRYSUB("Char Periodic Ticks Selection");
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(_mCharTickList.MT_CMUTEX);
#endif

        CharTickList::iterator itMap       = _mCharTickList.begin();
        CharTickList::iterator itMapEnd    = _mCharTickList.end();

        size_t uiProgressive = 0;
        int64 iTime;
        while ((itMap != itMapEnd) && (iCurTime > (iTime = itMap->first)))
        {
            CChar* pChar = itMap->second;

            if ((pChar->_iTimePeriodicTick != 0) && pChar->_CanTick() && !pChar->_IsBeingDeleted())
            {
                if (pChar->_iTimePeriodicTick <= iCurTime)
                {
                    _vecObjs.emplace_back(static_cast<void*>(pChar));
                    _vecPeriodicCharsToEraseFromList.emplace_back(uiProgressive);

                    pChar->_iTimePeriodicTick = 0;
                }
                //else
                //{
                //    // This shouldn't happen... If it does, get rid of the entry on the list anyways,
                //    //  it got desynchronized in some way and might be an invalid or even deleted and deallocated object!
                //}

            }
            ++itMap;
            ++uiProgressive;
        }

        EXC_CATCHSUB("");
    }

    {
        EXC_TRYSUB("Periodic Chars Delete from List");
//#if MT_ENGINES
//        std::unique_lock<std::shared_mutex> lockTimeObj(pTimedObj->MT_CMUTEX);
//#endif
            // Erase in chunks, call erase the least times possible.
            if (!_vecPeriodicCharsToEraseFromList.empty())
            {
                /*
                g_Log.EventDebug("-- Start CHARPERIODICTICK. I need to remove %lu items:\n", _vecPeriodicCharsToEraseFromList.size());
                std::stringstream ss;
                for (size_t elem : _vecPeriodicCharsToEraseFromList)
                {
                    ss << elem << ' ';
                }
                g_Log.EventDebug("%s\n", ss.str().c_str());
                */

                if (_vecPeriodicCharsToEraseFromList.size() > 1)
                {
                    size_t uiCurMapElemIdx = 0;
                    size_t uiCurVecElemIdx = 0;
                   // size_t uiSubRangeStartIdx = 0;
                    CharTickList::iterator itSubRangeStart = _mCharTickList.begin();
                    CharTickList::iterator itMap = _mCharTickList.begin();
                    bool fContiguous = true;
                    bool fFirstMatch = false;
                    while ((itMap != _mCharTickList.end()) &&
                        (uiCurMapElemIdx <= _vecPeriodicCharsToEraseFromList.back()) &&
                        (uiCurVecElemIdx < _vecPeriodicCharsToEraseFromList.size()))
                    {
                        if (!fFirstMatch)
                        {
                            if (uiCurMapElemIdx == _vecPeriodicCharsToEraseFromList[uiCurVecElemIdx])
                            {
                                //uiSubRangeStartIdx = uiCurMapElemIdx;
                                itSubRangeStart = itMap;
                                fFirstMatch = true;

                                ++uiCurVecElemIdx;
                            }

                            ++itMap;
                            ++uiCurMapElemIdx;
                            continue;
                        }

                        if (uiCurMapElemIdx == _vecPeriodicCharsToEraseFromList[uiCurVecElemIdx])
                        {
                            // Matches. I want to delete this.
                            if (uiCurMapElemIdx == _vecPeriodicCharsToEraseFromList[uiCurVecElemIdx - 1] + 1)
                            {
                                // I want to delete it and it's contiguous, go on
                                ASSERT(fContiguous);
                            }
                            else
                            {
                                // It isn't contiguous. Go below.
                                ASSERT(!fContiguous);
                                // This is the first one that matches after previous mismatches. We start this chunk from here.
                                //uiSubRangeStartIdx = uiCurMapElemIdx;
                                fContiguous = true;
                            }

                            ++itMap;
                            ++uiCurMapElemIdx;
                            ++uiCurVecElemIdx;
                            continue;
                        }

                        // Not contiguous to the next element to be erased (stored in the vector).
                        //  What to do?
                        if (uiCurMapElemIdx != _vecPeriodicCharsToEraseFromList[uiCurVecElemIdx])
                        {
                            // I don't want to erase this.
                            if (!fContiguous)
                            {
                                // This is an element after the first one successive to the previous contiguous block (2nd, 3rd...)
                                // Ignore it.
                                //g_Log.EventDebug("Skip this %lu\n", uiCurMapElemIdx);
                                ++itMap;
                            }
                            else
                            {
                                // This is the first element after the previous contiguous block
                                // I want to erase until the previous one
                                // erase doesn't delete the last element in the range
                                //g_Log.EventDebug("Skip this %lu, not to be deleted, and...\n", uiCurMapElemIdx);
                                //g_Log.EventDebug("Erasing %lu items starting from pos %lu\n", (uiCurMapElemIdx - uiSubRangeStartIdx), uiSubRangeStartIdx);

                                itMap = _mCharTickList.erase(itSubRangeStart, itMap);
                                ++itMap;
                                itSubRangeStart = itMap;
                                //uiSubRangeStartIdx = uiCurMapElemIdx;   // Not really needed
                                fContiguous = false;
                            }
                            ++uiCurMapElemIdx;
                            continue;
                        }

                       ASSERT(false);   // Shouldn't really be here.
                    }
                    if (fFirstMatch && fContiguous)
                    {
                        /*itMap =*/ _mCharTickList.erase(itSubRangeStart, itMap); // last range to erase
                        //g_Log.EventDebug("(End cycle) Erasing %lu items starting from pos %lu\n", (uiCurMapElemIdx - uiSubRangeStartIdx), uiSubRangeStartIdx);
                    }
                }
                else
                {
                    _mCharTickList.erase(std::next(_mCharTickList.begin(), _vecPeriodicCharsToEraseFromList.front()));
                    //g_Log.EventDebug("Erasing 1 item.\n");
                }
            }

        EXC_CATCHSUB("DeleteFromList");
        _vecPeriodicCharsToEraseFromList.clear();
    }

    {
        EXC_TRYSUB("Char Periodic Ticks Loop");
        for (void* pObjVoid : _vecObjs)    // Loop through all msecs stored, unless we passed the timestamp.
        {
            CChar* pChar = static_cast<CChar*>(pObjVoid);
            if (pChar->OnTickPeriodic())
            {
                AddCharTicking(pChar, false);
            }
            else
            {
                pChar->Delete(true);
            }
        }
        EXC_CATCHSUB("");
        _vecObjs.clear();
    }

    EXC_CATCH;
}
