#include "../common/sphere_library/scontainer_ops.h"
#include "../common/CException.h"
#include "../sphere/threads.h"
#include "../sphere/ProfileTask.h"
#include "chars/CChar.h"
#include "items/CItem.h"
#include "CSector.h"
#include "CWorldClock.h"
#include "CWorldGameTime.h"
#include "CWorldTicker.h"

#if defined _DEBUG || defined _NIGHTLY
#   define DEBUG_CTIMEDOBJ_TIMED_TICKING
#   define DEBUG_CCHAR_PERIODIC_TICKING
#   define DEBUG_STATUSUPDATES
#   define DEBUG_LIST_OPS

//#   define DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
//#   define DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
//#   define DEBUG_STATUSUPDATES_VERBOSE
//#   define DEBUG_LIST_OPS_VERBOSE

//#   define BENCHMARK_LISTS // TODO
#endif

CWorldTicker::CWorldTicker(CWorldClock *pClock)
{
    ASSERT(pClock);
    _pWorldClock = pClock;

    _iLastTickDone = 0;

    _vecGenericObjsToTick.reserve(50);
    _vecWorldObjsEraseRequests.reserve(50);
    _vecPeriodicCharsEraseRequests.reserve(25);
}


// CTimedObject TIMERs

auto CWorldTicker::HasTimedObject(const CTimedObject* pTimedObject) -> std::pair<int64, CTimedObject*>
{
    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) noexcept {
        return pTimedObject == rhs.second;
    };

    const auto itEntryInAddList = std::find_if(
        _vecWorldObjsAddRequests.begin(),
        _vecWorldObjsAddRequests.end(),
        fnFindEntryByObj);
    if (_vecWorldObjsAddRequests.end() != itEntryInAddList)
        return *itEntryInAddList;

    const auto itEntryInTickList = std::find_if(
        _mWorldTickList.begin(),
        _mWorldTickList.end(),
        fnFindEntryByObj);
    if (_mWorldTickList.end() != itEntryInTickList)
    {
        const auto itEntryInEraseList = std::find(
            _vecWorldObjsEraseRequests.begin(),
            _vecWorldObjsEraseRequests.end(),
            pTimedObject);

        if (itEntryInEraseList == _vecWorldObjsEraseRequests.end())
            return *itEntryInTickList;
    }

    return {0, nullptr};
}

void CWorldTicker::_InsertTimedObject(const int64 iTimeout, CTimedObject* pTimedObject)
{
    ASSERT(pTimedObject);
    ASSERT(iTimeout != 0);

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    ASSERT(sl::ContainerIsSorted(_mWorldTickList));
    ASSERT(!sl::SortedContainerHasDuplicates(_mWorldTickList));
#endif

    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) noexcept {
        return pTimedObject == rhs.second;
    };
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_mWorldTickList.MT_CMUTEX);
#endif

    if (pTimedObject->_fIsAddingInWorldTickList)
    {
        const auto itEntryInAddList = std::find_if(
            _vecWorldObjsAddRequests.begin(),
            _vecWorldObjsAddRequests.end(),
            fnFindEntryByObj);
        ASSERT(_vecWorldObjsAddRequests.end() != itEntryInAddList);

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
        g_Log.EventDebug("[WorldTicker][%p] WARN: CTimedObj insertion into ticking list already requested with %s timer. OVERWRITING.\n",
            (void*)pTimedObject,
            ((itEntryInAddList->first == iTimeout) ? "same" : "different"));
#endif

        itEntryInAddList->first = iTimeout;
        return; // Already requested the addition.
    }

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    const auto itEntryInTickList = std::find_if(
        _mWorldTickList.begin(),
        _mWorldTickList.end(),
        fnFindEntryByObj);
    if (_mWorldTickList.end() != itEntryInTickList)
    {
        /*
        if (itEntryInTickList->first == iTimeout)
        {
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
            g_Log.EventDebug("[WorldTicker][%p] INFO: Requested insertion of a CTimedObj in the main ticking list with the same timeout, skipping.\n", (void*)pTimedObject);
#endif
            return;
        }
        */

#   ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO: Requested insertion of a CTimedObj already in the main ticking list.\n", (void*)pTimedObject);
#   endif

        const auto itEntryInEraseList = std::find(
            _vecWorldObjsEraseRequests.begin(),
            _vecWorldObjsEraseRequests.end(),
            pTimedObject);
        if (_vecWorldObjsEraseRequests.end() == itEntryInEraseList)
        {
#   ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
            g_Log.EventDebug("[WorldTicker][%p] WARN: And i didn't requested to remove that!\n", (void*)pTimedObject);
#   endif

            ASSERT(false);
            return;
        }

#   ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO: But that's fine, because i already have requested to remove that!\n", (void*)pTimedObject);
#   endif
    }
    else
    {
        const auto itEntryInEraseList = std::find(
            _vecWorldObjsEraseRequests.begin(),
            _vecWorldObjsEraseRequests.end(),
            pTimedObject);
        ASSERT(_vecWorldObjsEraseRequests.end() == itEntryInEraseList);
    }
#endif

    _vecWorldObjsAddRequests.emplace_back(iTimeout, pTimedObject);
    pTimedObject->_fIsAddingInWorldTickList = true;

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
    g_Log.EventDebug("[WorldTicker][%p] INFO: Done adding CTimedObj in the ticking list add buffer with timeout %" PRId64 ".\n", (void*)pTimedObject, iTimeout);
#endif
}

void CWorldTicker::_RemoveTimedObject(CTimedObject* pTimedObject)
{
#ifdef DEBUG_LIST_OPS
    ASSERT(sl::ContainerIsSorted(_mWorldTickList));
    ASSERT(!sl::SortedContainerHasDuplicates(_mWorldTickList));
#endif

    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) noexcept {
        return pTimedObject == rhs.second;
    };
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_mWorldTickList.MT_CMUTEX);
#endif

    if (pTimedObject->_fIsAddingInWorldTickList)
    {
        const auto itEntryInAddList = std::find_if(
            _vecWorldObjsAddRequests.begin(),
            _vecWorldObjsAddRequests.end(),
            fnFindEntryByObj);
        ASSERT(itEntryInAddList != _vecWorldObjsAddRequests.end());

        _vecWorldObjsAddRequests.erase(itEntryInAddList);
        pTimedObject->_fIsAddingInWorldTickList = false;

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
#   ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO: Removing CTimedObj from the ticking list add buffer.\n", (void*)pTimedObject);
#   endif

        const auto itEntryInTickList = std::find_if(
            _mWorldTickList.begin(),
            _mWorldTickList.end(),
            fnFindEntryByObj);
        const auto itEntryInRemoveList = std::find(
            _vecWorldObjsEraseRequests.begin(),
            _vecWorldObjsEraseRequests.end(),
            pTimedObject);

        if (itEntryInRemoveList == _vecWorldObjsEraseRequests.end()) {
            ASSERT(itEntryInTickList == _mWorldTickList.end());
        }
        else if (itEntryInRemoveList != _vecWorldObjsEraseRequests.end()) {
            ASSERT(itEntryInTickList != _mWorldTickList.end());
        }
#endif

        return;
    }

    const auto itEntryInRemoveList = std::find(
        _vecWorldObjsEraseRequests.begin(),
        _vecWorldObjsEraseRequests.end(),
        pTimedObject);
    if (_vecWorldObjsEraseRequests.end() != itEntryInRemoveList)
    {
        // I have already requested to remove this from the main ticking list.
        // It can happen and it's legit. Example: we call this method via CObjBase::Delete and ~CObjBase.

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO: CTimedObj removal from the main ticking list already requested.\n", (void*)pTimedObject);
#endif
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
        const auto itEntryInAddList = std::find_if(
            _vecWorldObjsAddRequests.begin(),
            _vecWorldObjsAddRequests.end(),
            fnFindEntryByObj);
        ASSERT(itEntryInAddList == _vecWorldObjsAddRequests.end());

        const auto itEntryInTickList = std::find_if(
            _mWorldTickList.begin(),
            _mWorldTickList.end(),
            fnFindEntryByObj);
        ASSERT(itEntryInTickList != _mWorldTickList.end());
#endif

        return; // Already requested the removal.
    }

    const auto itEntryInTickList = std::find_if(
        _mWorldTickList.begin(),
        _mWorldTickList.end(),
        fnFindEntryByObj);
    if (itEntryInTickList == _mWorldTickList.end())
    {
        // Not found. The object might have a timeout while being in a non-tickable state (like at server startup), so it isn't in the list.
/*
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
        g_Log.EventDebug("[WorldTicker][%p] INFO: Requested erasure of TimedObject in mWorldTickList, but it wasn't found there.\n", (void*)pTimedObject);
#endif
*/
        return;
    }

    _vecWorldObjsEraseRequests.emplace_back(pTimedObject);

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
    g_Log.EventDebug("[WorldTicker][%p] INFO: Done adding CTimedObj to the ticking list remove buffer.\n", (void*)pTimedObject);
#endif
}

void CWorldTicker::AddTimedObject(const int64 iTimeout, CTimedObject* pTimedObject, bool fForce)
{
    //if (iTimeout < CWorldGameTime::GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible; don't uncomment.
    //    return;

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
    g_Log.EventDebug("[WorldTicker][%p] INFO: Trying to add CTimedObj to the ticking list.\n", (void*)pTimedObject);
#endif

    EXC_TRY("AddTimedObject");
    const ProfileTask timersTask(PROFILE_TIMERS);

    EXC_SET_BLOCK("Already ticking?");
    if (pTimedObject->IsTicking())
    {
        // Adding an object that might already be in the list?
        // Or, am i setting a new timeout without deleting the previous one?
        // I can have iTickOld != 0 but not be in the list in some cases, like when duping an obj with an active timer.
        EXC_SET_BLOCK("Remove");
        _RemoveTimedObject(pTimedObject);
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
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
    else
        g_Log.EventDebug("[WorldTicker][%p] INFO: Cannot tick and no force, so i'm not adding CTimedObj to the ticking list.\n", (void*)pTimedObject);
#endif

    EXC_CATCH;
}

void CWorldTicker::DelTimedObject(CTimedObject* pTimedObject)
{
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
    g_Log.EventDebug("[WorldTicker][%p] INFO: Trying to remove CTimedObj from the ticking list.\n", (void*)pTimedObject);
#endif

    EXC_TRY("DelTimedObject");
    const ProfileTask timersTask(PROFILE_TIMERS);

    /*
    EXC_SET_BLOCK("Not ticking?");

    const int64 iTickOld = pTimedObject->_GetTimeoutRaw();
    if (iTickOld == 0)
    {
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
#   ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO: Requested deletion of CTimedObj, but Timeout is 0, so it shouldn't be in the list, or just queued to be removed.\n", (void*)pTimedObject);
#   endif

        const auto itEntryInRemoveList = std::find(
            _vecWorldObjsEraseRequests.begin(),
            _vecWorldObjsEraseRequests.end(),
            pTimedObject);
        if (itEntryInRemoveList != _vecWorldObjsEraseRequests.end())
        {
#   ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
            g_Log.EventDebug("[WorldTicker][%p] INFO:   though, found it already in the removal list, so it's fine..\n", (void*)pTimedObject);
#   endif
            return;
        }

        const auto itTickList = std::find_if(
            _mWorldTickList.begin(),
            _mWorldTickList.end(),
            [pTimedObject](const TickingTimedObjEntry& entry) {
                return entry.second == pTimedObject;
            });
        if (itTickList != _mWorldTickList.end()) {
#   ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
            g_Log.EventDebug("[WorldTicker][%p] WARN:   But i have found it in the list! With Timeout %" PRId64 ".\n", (void*)pTimedObject, itTickList->first);
#   endif
            ASSERT(false);
        }
#   ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
        else
            g_Log.EventDebug("[WorldTicker][%p] INFO:   (rightfully) i haven't found it in the list.\n", (void*)pTimedObject);
#   endif
#endif
        return;
    }
*/

    EXC_SET_BLOCK("Remove");
    _RemoveTimedObject(pTimedObject);

    EXC_CATCH;
}


// CChar Periodic Ticks (it's a different thing than TIMER!)

void CWorldTicker::_InsertCharTicking(const int64 iTickNext, CChar* pChar)
{
    ASSERT(iTickNext != 0);
    ASSERT(pChar);

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_mCharTickList.MT_CMUTEX);
#endif

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
    const auto fnFindEntryByChar = [pChar](TickingPeriodicCharEntry const& rhs) noexcept {
        return pChar == rhs.second;
    };

    const auto itEntryInAddList = std::find_if(
        _vecPeriodicCharsAddRequests.begin(),
        _vecPeriodicCharsAddRequests.end(),
        fnFindEntryByChar);
    if (_vecPeriodicCharsAddRequests.end() != itEntryInAddList)
    {
        g_Log.EventDebug("[WorldTicker][%p] WARN: Periodic char insertion into ticking list already requested "
            "(requesting tick %" PRId64 ", previous requested tick %" PRId64 ").\n",
            (void*)pChar, iTickNext, itEntryInAddList->first);

        ASSERT(false);
        return; // Already requested the addition.
    }

    const auto itEntryInEraseList = std::find(
        _vecPeriodicCharsEraseRequests.begin(),
        _vecPeriodicCharsEraseRequests.end(),
        pChar);
    if (_vecPeriodicCharsEraseRequests.end() != itEntryInEraseList)
    {
        g_Log.EventDebug("[WorldTicker][%p] WARN: Stopped insertion attempt of a CChar which removal from periodic ticking list has been requested!\n", (void*)pChar);
        ASSERT(false);
        return; // Already requested the removal.
    }

    const auto itEntryInTickList = std::find_if(
        _mCharTickList.begin(),
        _mCharTickList.end(),
        fnFindEntryByChar);
    ASSERT(_mCharTickList.end() == itEntryInTickList);
#endif

    _vecPeriodicCharsAddRequests.emplace_back(iTickNext, pChar);
    pChar->_iTimePeriodicTick = iTickNext;

#ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
    g_Log.EventDebug("[WorldTicker][%p] INFO: Done adding the CChar to the periodic ticking list add buffer.\n", (void*)pChar);
#endif
}

bool CWorldTicker::_RemoveCharTicking(CChar* pChar)
{
    // I'm reasonably sure that the element i'm trying to remove is present in this container.
    ASSERT(pChar);

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_mCharTickList.MT_CMUTEX);
#endif

    const auto fnFindEntryByChar = [pChar](TickingPeriodicCharEntry const& rhs) noexcept {
        return pChar == rhs.second;
    };

    const auto itEntryInTickList = std::find_if(
        _mCharTickList.begin(),
        _mCharTickList.end(),
        fnFindEntryByChar);

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
    const auto itEntryInRemoveList = std::find(
        _vecPeriodicCharsEraseRequests.begin(),
        _vecPeriodicCharsEraseRequests.end(),
        pChar);
    if (_vecPeriodicCharsEraseRequests.end() != itEntryInRemoveList)
    {
#   ifdef DEBUG_CCHAR_PERIODIC_TICKING
        g_Log.EventDebug("[WorldTicker][%p] INFO: TickingPeriodicChar erasure from ticking list already requested.\n", (void*)pChar);
#   endif

        ASSERT(false);
        return false; // Already requested the removal.
    }
#endif

    const auto itEntryInAddList = std::find_if(
        _vecPeriodicCharsAddRequests.begin(),
        _vecPeriodicCharsAddRequests.end(),
        fnFindEntryByChar);
    if (_vecPeriodicCharsAddRequests.end() != itEntryInAddList)
    {
#ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO: Erasing TickingPeriodicChar from periodic char ticking list add buffer.\n", (void*)pChar);
#endif

        _vecPeriodicCharsAddRequests.erase(itEntryInAddList);
        pChar->_iTimePeriodicTick = 0;

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
        if (itEntryInRemoveList == _vecPeriodicCharsEraseRequests.end()) {
            ASSERT(itEntryInTickList == _mCharTickList.end());
        }
        else {
            ASSERT(itEntryInTickList != _mCharTickList.end());
        }
#endif
        return true;
    }

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
    // Check if it's in the ticking list.
    if (itEntryInTickList == _mCharTickList.end())
    {
        g_Log.EventDebug("[WorldTicker][%p] WARN: Requested TickingPeriodicChar removal from ticking list, but not found.\n", (void*)pChar);

        ASSERT(false);
        return false;
    }

    if (_vecPeriodicCharsEraseRequests.end() != itEntryInRemoveList)
    {
        // I have already requested to remove this from the main ticking list.
        g_Log.EventDebug("[WorldTicker][%p] WARN: TickingPeriodicChar removal from the main ticking list already requested.\n", (void*)pChar);

        ASSERT(itEntryInAddList == _vecPeriodicCharsAddRequests.end());
        return false; // Already requested the removal.
    }
#endif

    if (itEntryInTickList == _mCharTickList.end())
    {
        // Not found. The object might have a timeout while being in a non-tickable state, so it isn't in the list.
#ifdef DEBUG_CCHAR_PERIODIC_TICKING
        g_Log.EventDebug("[WorldTicker][%p] INFO: Requested erasure of TimedObject in mWorldTickList, but it wasn't found there.\n", (void*)pChar);
#endif
        return false;
    }

    _vecPeriodicCharsEraseRequests.emplace_back(pChar);
    pChar->_iTimePeriodicTick = 0;

#ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
    g_Log.EventDebug("[WorldTicker][%p] INFO: Done adding the CChar to the periodic ticking list remove buffer.\n", (void*)pChar);
#endif
    return true;
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

#ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
    g_Log.EventDebug("[WorldTicker][%p] INFO: Trying to add CChar to the periodic ticking list. Tickold: %" PRId64 ". Ticknext: %" PRId64 ".\n",
        (void*)pChar, iTickOld, iTickNext);
#endif

    if (iTickNext == iTickOld)
    {
#ifdef DEBUG_CCHAR_PERIODIC_TICKING
        g_Log.EventDebug("[WorldTicker][%p] INFO: Do not add (again) char periodic tick, tickold == ticknext.\n", (void*)pChar);
#endif
        return;
    }

    //if (iTickNext < CWorldGameTime::GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible
    //    return;

    if (iTickOld != 0)
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        EXC_SET_BLOCK("Remove");
        _RemoveCharTicking(pChar);
    }

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

#ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
    g_Log.EventDebug("[WorldTicker][%p] INFO: Trying to remove CChar from the periodic ticking list. Tickold: %" PRId64 ".\n",
        (void*)pChar, iTickOld);
#endif

    if (iTickOld == 0)
    {
#ifdef DEBUG_CCHAR_PERIODIC_TICKING
#   ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO: Requested deletion of Periodic char, but Timeout is 0. It shouldn't be in the list, or just queued to be removed.\n", (void*)pChar);
#   endif
        auto fnFindEntryByChar = [pChar](const TickingPeriodicCharEntry& entry) noexcept {
                return entry.second == pChar;
        };

        const auto itEntryInEraseList = std::find(
            _vecPeriodicCharsEraseRequests.begin(),
            _vecPeriodicCharsEraseRequests.end(),
            pChar);
        if (itEntryInEraseList != _vecPeriodicCharsEraseRequests.end())
        {
#   ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
            g_Log.EventDebug("[WorldTicker][%p] INFO:   though, found it already in the removal list, so it's fine..\n", (void*)pChar);
#   endif

            return;
        }

        const auto itEntryInTickList = std::find_if(
            _mCharTickList.begin(),
            _mCharTickList.end(),
            fnFindEntryByChar);
        if (itEntryInTickList != _mCharTickList.end())
        {
#   ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
            g_Log.EventDebug("[WorldTicker][%p] WARN:   But i have found it in the list! With Timeout %" PRId64 ".\n", (void*)pChar, itEntryInTickList->first);
#   endif

            ASSERT(false);
            return;
        }

#   ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO:   (rightfully) i haven't found it in any list.\n", (void*)pChar);
#   endif
#endif
        return;
    }

    EXC_SET_BLOCK("Remove");
    _RemoveCharTicking(pChar);

    EXC_CATCH;
}

void CWorldTicker::AddObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
#ifdef DEBUG_STATUSUPDATES_VERBOSE
    g_Log.EventDebug("[%p] INFO: Trying to add CObjBase to the status update list.\n", (void*)pObj);
#endif
    EXC_TRY("AddObjStatusUpdate");
    const ProfileTask timersTask(PROFILE_TIMERS);

    UnreferencedParameter(fNeedsLock);
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(_ObjStatusUpdates.MT_CMUTEX);
#endif

        // Here i don't need to use an "add" buffer, like with CTimedObj, because this container isn't meant
        //  to be ordered and i can just push back stuff.
        const auto itDuplicate = std::find(
            _ObjStatusUpdates.begin(),
            _ObjStatusUpdates.end(),
            pObj
            );
        if (_ObjStatusUpdates.end() != itDuplicate)
        {
#ifdef DEBUG_STATUSUPDATES_VERBOSE
            g_Log.EventDebug("[%p] WARN: Trying to add status update for duplicate CObjBase. Blocked.\n", (void*)pObj);
#endif
            return;
        }

        _ObjStatusUpdates.emplace_back(pObj);

#ifdef DEBUG_STATUSUPDATES_VERBOSE
        g_Log.EventDebug("[%p] INFO: Done adding CObjBase to the status update list.\n", (void*)pObj);
#endif
    }

    EXC_CATCH;
}

void CWorldTicker::DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
#ifdef DEBUG_STATUSUPDATES_VERBOSE
    g_Log.EventDebug("[%p] INFO: Trying to remove CObjBase from the status update list.\n", (void*)pObj);
#endif
    EXC_TRY("DelObjStatusUpdate");
    const ProfileTask timersTask(PROFILE_TIMERS);

    UnreferencedParameter(fNeedsLock);
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(_ObjStatusUpdates.MT_CMUTEX);
#endif
        //_ObjStatusUpdates.erase(pObj);

        const auto itMissing = std::find(
            _ObjStatusUpdates.begin(),
            _ObjStatusUpdates.end(),
            pObj
            );
        if (_ObjStatusUpdates.end() == itMissing)
        {
#ifdef DEBUG_STATUSUPDATES_VERBOSE
            g_Log.EventDebug("[%p] INFO: Requested erasure of CObjBase from the status update list, but it wasn't found there.\n", (void*)pObj);
#endif
            return;
        }

        const auto itRepeated = std::find(
            _vecObjStatusUpdateEraseRequested.begin(),
            _vecObjStatusUpdateEraseRequested.end(),
            pObj
            );
        if (_vecObjStatusUpdateEraseRequested.end() != itRepeated)
        {
#ifdef DEBUG_STATUSUPDATES_VERBOSE
            // It can happen and it's legit. Example: we call this method via CObjBase::Delete and ~CObjBase.
            g_Log.EventDebug("INFO [%p]: CObjBase erasure from the status update list already requested.\n", (void*)pObj);
#endif
            return;
        }

        _vecObjStatusUpdateEraseRequested.emplace_back(pObj);
#ifdef DEBUG_STATUSUPDATES_VERBOSE
        g_Log.EventDebug("[%p] INFO: Done adding CObjBase to the status update remove buffer.\n", (void*)pObj);
#endif
    }

    EXC_CATCH;
}

template <typename T>
static void sortedVecRemoveElementsByIndices(std::vector<T>& vecMain, const std::vector<size_t>& vecIndicesToRemove)
{
    // Erase in chunks, call erase the least times possible.
    if (vecIndicesToRemove.empty())
        return;

    if (vecMain.empty())
        return;

    const size_t sz = vecMain.size();

#ifdef DEBUG_LIST_OPS
    ASSERT(sl::ContainerIsSorted(vecMain));
    ASSERT(sl::ContainerIsSorted(vecIndicesToRemove));
    ASSERT(!sl::SortedContainerHasDuplicates(vecMain));
    ASSERT(!sl::SortedContainerHasDuplicates(vecIndicesToRemove));
    //g_Log.EventDebug("Starting sortedVecRemoveElementsByIndices.\n");
#endif

    // Copy the original vector to check against later
    std::vector<T> originalVecMain = vecMain;

    // Start from the end of vecIndicesToRemove, working backwards with normal iterators
    auto itRemoveFirst = vecIndicesToRemove.end();  // Points one past the last element
    while (itRemoveFirst != vecIndicesToRemove.begin())
    {
        // Move the iterator back to point to a valid element.
        --itRemoveFirst;

        auto itRemoveLast = itRemoveFirst;  // Marks the start of a contiguous block
        // Find contiguous block by working backwards and finding consecutive indices
        // This loop identifies a contiguous block of indices to remove.
        // A block is contiguous if the current index *itRemoveFirst is exactly 1 greater than the previous index.
        while (itRemoveFirst != vecIndicesToRemove.begin() && *(itRemoveFirst - 1) + 1 == *itRemoveFirst)
        {
            --itRemoveFirst;
        }

        /*
#ifdef DEBUG_LIST_OPS
         g_Log.EventDebug("Removing contiguous indices: %" PRIuSIZE_T " to %" PRIuSIZE_T " (total sizes vecMain: %" PRIuSIZE_T ", vecIndices: %" PRIuSIZE_T ").\n",
            *itRemoveFirst, *itRemoveLast, vecMain.size(), vecIndicesToRemove.size());
#endif
        */

        // Once we find a contiguous block, we erase that block from vecMain.
        auto itRemoveLastPast = (*itRemoveLast == vecMain.size() - 1) ? vecMain.end() : (vecMain.begin() + *itRemoveLast + 1);
        vecMain.erase(vecMain.begin() + *itRemoveFirst, itRemoveLastPast);
    }

#ifdef DEBUG_LIST_OPS
    // Sanity Check: Verify that the removed elements are no longer present in vecMain
    for (auto index : vecIndicesToRemove) {
        UnreferencedParameter(index);
        ASSERT(index < originalVecMain.size());
        ASSERT(std::find(vecMain.begin(), vecMain.end(), originalVecMain[index]) == vecMain.end());
    }
#   ifdef DEBUG_LIST_OPS_VERBOSE
    g_Log.EventDebug("Sizes: new vec %" PRIuSIZE_T ", old vec %" PRIuSIZE_T ", remove vec %" PRIuSIZE_T ".\n",
        vecMain.size(), sz, vecIndicesToRemove.size());
#   endif
#endif

    UnreferencedParameter(sz);
    ASSERT(vecMain.size() == sz - vecIndicesToRemove.size());


    /*
    // Alternative implementation:
    // We cannot use a vector with the indices but we need a vector with a copy of the elements to remove.
    // std::remove_if in itself is more efficient than multiple erase calls, because the number of the element shifts is lesser.
    // Though, we need to consider the memory overhead of reading through an std::pair of two types, which is bigger than just an index.
    // Also, jumping across the vector in a non-contiguous way with the binary search can add additional memory overhead by itself, and
    //   this will be greater the bigger are the elements in the vector..
    // The bottom line is that we need to run some benchmarks between the two algorithms, and possibly also for two versions of this algorithm,
    //   one using binary search and another with linear search.
    // The latter might actually be faster for a small number of elements, since it's more predictable for the prefetcher.

    // Use std::remove_if to shift elements not marked for removal to the front
    auto it = std::remove_if(vecMain.begin(), vecMain.end(),
        [&](const T& element) {
            // Check if the current element is in the valuesToRemove vector using binary search
            return std::binary_search(valuesToRemove.begin(), valuesToRemove.end(), element);
        });

    // Erase the removed elements in one go
    vecMain.erase(it, vecMain.end());
    */
}

template <typename T>
static void unsortedVecRemoveElementsByValues(std::vector<T>& vecMain, const std::vector<T> & vecValuesToRemove)
{
    if (vecValuesToRemove.empty())
        return;

#ifdef DEBUG_LIST_OPS
    ASSERT(!sl::UnsortedContainerHasDuplicates(vecMain));
    ASSERT(!sl::UnsortedContainerHasDuplicates(vecValuesToRemove));
#endif

    // Sort valuesToRemove for binary search
    //std::sort(vecValuesToRemove.begin(), vecValuesToRemove.end());

    // Use std::remove_if to shift elements not marked for removal to the front
    auto it = std::remove_if(vecMain.begin(), vecMain.end(),
        [&](const T& element) {
            // Use binary search to check if the element should be removed
            //return std::binary_search(vecValuesToRemove.begin(), vecValuesToRemove.end(), element);
            return std::find(vecValuesToRemove.begin(), vecValuesToRemove.end(), element) != vecValuesToRemove.end();
        });

    // Erase the removed elements in one go
    vecMain.erase(it, vecMain.end());
}


/*
// To be tested and benchmarked.
template <typename T>
static void sortedVecRemoveElementsByValues(std::vector<T>& vecMain, const std::vector<T>& toRemove)
{
    if (toRemove.empty() || vecMain.empty())
        return;

    auto mainIt = vecMain.begin();
    auto removeIt = toRemove.begin();

    // Destination pointer for in-place shifting
    auto destIt = mainIt;

    while (mainIt != vecMain.end() && removeIt != toRemove.end()) {
        // Skip over elements in the main vector that are smaller than the current element to remove
        auto nextRangeEnd = std::lower_bound(mainIt, vecMain.end(), *removeIt);

        // Batch copy the range of elements not marked for removal
        std::move(mainIt, nextRangeEnd, destIt);
        destIt += std::distance(mainIt, nextRangeEnd);

        // Advance main iterator and remove iterator
        mainIt = nextRangeEnd;

        // Skip the elements that need to be removed
        if (mainIt != vecMain.end() && *mainIt == *removeIt) {
            ++mainIt;
            ++removeIt;
        }
    }

    // Copy the remaining elements if there are any left
    std::move(mainIt, vecMain.end(), destIt);

    // Resize the vector to remove the now extraneous elements at the end
    vecMain.resize(destIt - vecMain.begin());
}
*/

/*
template <typename TPair, typename T>
static void sortedVecDifference(
    const std::vector<TPair>& vecMain, const std::vector<T*>& vecToRemove, std::vector<TPair>& vecElemBuffer
    )
{
    auto itMain = vecMain.begin();    // Iterator for vecMain
    auto start = itMain;              // Start marker for non-matching ranges in vecMain

    for (auto& elem : vecToRemove) {
        g_Log.EventDebug("Should remove %p.\n", (void*)elem);
    }
    for (auto& elem : vecMain) {
        g_Log.EventDebug("VecMain %p.\n", (void*)elem.second);
    }

    // Iterate through each element in vecToRemove to locate and exclude its matches in vecMain
    for (const auto& removePtr : vecToRemove) {
        // Binary search to find the start of the block where vecMain.second == removePtr
        itMain = std::lower_bound(itMain, vecMain.end(), removePtr,
            [](const TPair& lhs, const T* rhs) noexcept {
                return lhs.second < rhs;  // Compare TPair.second with T*
            });

        // Insert all elements from `start` up to `itMain` (non-matching elements)
        vecElemBuffer.insert(vecElemBuffer.end(), start, itMain);

        // Skip over all contiguous elements in vecMain that match removePtr
        while (itMain != vecMain.end() && itMain->second == removePtr) {
            ++itMain;
        }

        // Update `start` to the new non-matching range after any matching elements are skipped
        start = itMain;
    }

    // Insert remaining elements from vecMain after the last matched element
    vecElemBuffer.insert(vecElemBuffer.end(), start, vecMain.end());

    for (auto& elem : vecMain) {
        g_Log.EventDebug("VecMain %p.\n", (void*)elem.second);
    }
}
*/

template <typename TPair, typename T>
static void unsortedVecDifference(
    const std::vector<TPair>& vecMain, const std::vector<T*>& vecToRemove, std::vector<TPair>& vecElemBuffer
    )
{
    /*
    // Iterate through vecMain, adding elements to vecElemBuffer only if they aren't in vecToRemove
    for (const auto& elem : vecMain) {
        if (std::find(vecToRemove.begin(), vecToRemove.end(), elem.second) == vecToRemove.end()) {
            vecElemBuffer.push_back(elem);
        }
    }
    */
    // vecMain is sorted by timeout (first elem of the pair), not by pointer (second elem)
    // vecToRemove is sorted by its contents (pointer)

    // Reserve space in vecElemBuffer to avoid reallocations
    vecElemBuffer.reserve(vecMain.size() - vecToRemove.size());

    // Use an iterator to store the position for bulk insertion
    auto itCopyFromThis = vecMain.begin();
    /*
    auto itFindBegin = vecToRemove.begin();

    // Iterate through vecMain, copying elements that are not in vecToRemove
    for (auto itMain = vecMain.begin(); itMain != vecMain.end(); ++itMain)
    {
        // Perform a linear search for the current element's pointer in vecToRemove
        auto itTemp = std::find(itFindBegin, vecToRemove.end(), itMain->second);
        if (itTemp != vecToRemove.end())
        {
            // If the element is found in vecToRemove, copy elements before it
            vecElemBuffer.insert(vecElemBuffer.end(), itCopyFromThis, itMain); // Copy up to but not including itMain

            // Move itCopyFromThis forward to the next element
            itCopyFromThis = itMain + 1;
            //itFindBegin = itTemp + 1;

            // Update itFindBegin to continue searching for the next instance in vecToRemove
            // We do not change itFindBegin here, since we want to keep searching for this pointer
            // in vecToRemove for subsequent elements in vecMain
        }
        else
        {
            // If itTemp is not found, we can still copy the current element
            // Check if itCopyFromThis is equal to itMain to avoid double-copying in case of duplicates
            if (itCopyFromThis != itMain)
            {
                // Move itCopyFromThis forward to the next element
                itCopyFromThis = itMain + 1;
            }
        }
    }*/

    // TODO: maybe optimize this algorithm.
    // Iterate through vecMain, copying elements that are not in vecToRemove
    for (auto itMain = vecMain.begin(); itMain != vecMain.end(); ++itMain) {
        // Perform a linear search for the current element's pointer in vecToRemove
        auto itTemp = std::find(vecToRemove.begin(), vecToRemove.end(), itMain->second);
        if (itTemp != vecToRemove.end()) {
            // If the element is found in vecToRemove, copy elements before it
            vecElemBuffer.insert(vecElemBuffer.end(), itCopyFromThis, itMain); // Copy up to but not including itMain

            // Move itCopyFromThis forward to the next element
            itCopyFromThis = itMain + 1; // Move to the next element after itMain
        }
    }

    // Copy any remaining elements in vecMain after the last found element
    vecElemBuffer.insert(vecElemBuffer.end(), itCopyFromThis, vecMain.end());

#ifdef DEBUG_LIST_OPS_VERBOSE
    g_Log.EventDebug("Sizes: new vec %" PRIuSIZE_T ", old vec %" PRIuSIZE_T ", remove vec %" PRIuSIZE_T ".\n",
        vecElemBuffer.size(), vecMain.size(), vecToRemove.size());

    for (auto& elem : vecToRemove) {
        g_Log.EventDebug("Should remove %p.\n", (void*)elem);
    }
    for (auto& elem : vecMain) {
        g_Log.EventDebug("VecMain %p.\n", (void*)elem.second);
    }
    for (auto& elem : vecElemBuffer) {
        g_Log.EventDebug("NewVec %p.\n", (void*)elem.second);
    }
    ASSERT(vecElemBuffer.size() == vecMain.size() - vecToRemove.size());
#endif
}

template <typename TPair, typename T>
static void sortedVecRemoveAddQueued(
    std::vector<TPair> &vecMain, std::vector<T> &vecToRemove, std::vector<TPair> &vecToAdd, std::vector<TPair> &vecElemBuffer
    )
{
#ifdef DEBUG_LIST_OPS
    ASSERT(sl::ContainerIsSorted(vecMain));
    ASSERT(!sl::SortedContainerHasDuplicates(vecMain));
#endif

    //EXC_TRY("vecRemoveAddQueued");
    //EXC_SET_BLOCK("Sort intermediate lists");
    std::sort(vecToAdd.begin(), vecToAdd.end());
    std::sort(vecToRemove.begin(), vecToRemove.end());

#ifdef DEBUG_LIST_OPS
    //ASSERT(sl::ContainerIsSorted(vecMain));
    ASSERT(!sl::SortedContainerHasDuplicates(vecMain));
#endif

    //EXC_SET_BLOCK("Ordered remove");
    if (!vecToRemove.empty())
    {
        if (vecMain.empty()) {
            ASSERT(false);  // Shouldn't ever happen.
        }

        // TODO: test and benchmark if the approach of the above function (sortedVecRemoveElementsInPlace) might be faster.
        vecElemBuffer.clear();
        vecElemBuffer.reserve(vecMain.size() / 2);
        unsortedVecDifference(vecMain, vecToRemove, vecElemBuffer);

#ifdef DEBUG_LIST_OPS
        for (auto& elem : vecToRemove)
        {
            auto it = std::find_if(vecElemBuffer.begin(), vecElemBuffer.end(), [elem](auto &rhs) {return elem == rhs.second;});
            UnreferencedParameter(it);
            ASSERT (it == vecElemBuffer.end());
        }

        ASSERT(vecElemBuffer.size() == vecMain.size() - vecToRemove.size());
        ASSERT(sl::ContainerIsSorted(vecElemBuffer));
        ASSERT(!sl::SortedContainerHasDuplicates(vecElemBuffer));
#endif

        vecMain.swap(vecElemBuffer);

        //vecMain = std::move(vecElemBuffer);
        vecElemBuffer.clear();
        vecToRemove.clear();
    }

    //EXC_SET_BLOCK("Mergesort");
    if (!vecToAdd.empty())
    {
        vecElemBuffer.clear();
        vecElemBuffer.reserve(vecMain.size() + vecToAdd.size());
        std::merge(
            vecMain.begin(), vecMain.end(),
            vecToAdd.begin(), vecToAdd.end(),
            std::back_inserter(vecElemBuffer)
            );

#ifdef DEBUG_LIST_OPS
        ASSERT(vecElemBuffer.size() == vecMain.size() + vecToAdd.size());
        ASSERT(sl::ContainerIsSorted(vecElemBuffer));
        ASSERT(!sl::SortedContainerHasDuplicates(vecElemBuffer));
#endif

        vecMain.swap(vecElemBuffer);
        //vecMain = std::move(vecElemBuffer);
        vecElemBuffer.clear();
        vecToAdd.clear();

#ifdef DEBUG_LIST_OPS_VERBOSE
        g_Log.EventDebug("[GLOBAL] STATUS: Nonempty tick list add buffer processed.\n");
#endif
    }

    //EXC_CATCH:
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
            {
#if MT_ENGINES
                std::unique_lock<std::shared_mutex> lock_su(_ObjStatusUpdates.MT_CMUTEX);
#endif
                EXC_TRYSUB("StatusUpdates");
                EXC_SETSUB_BLOCK("Remove requested");
                unsortedVecRemoveElementsByValues(_ObjStatusUpdates, _vecObjStatusUpdateEraseRequested);

                EXC_SETSUB_BLOCK("Selection");
                if (!_ObjStatusUpdates.empty())
                {
                    for (CObjBase* pObj : _ObjStatusUpdates)
                    {
                        if (pObj && !pObj->_IsBeingDeleted())
                            _vecGenericObjsToTick.emplace_back(static_cast<void*>(pObj));
                    }
                }
                EXC_CATCHSUB("");
                //EXC_DEBUGSUB_START;
                //EXC_DEBUGSUB_END;
                _ObjStatusUpdates.clear();
                _vecObjStatusUpdateEraseRequested.clear();
            }

            EXC_TRYSUB("StatusUpdates");
            EXC_SETSUB_BLOCK("Loop");
            for (void* pObjVoid : _vecGenericObjsToTick)
            {
                CObjBase* pObj = static_cast<CObjBase*>(pObjVoid);
                pObj->OnTickStatusUpdate();
            }
            EXC_CATCHSUB("");

            _vecGenericObjsToTick.clear();
        }
    }


    /* World ticking (timers) */

    // Items, Chars ... Everything relying on CTimedObject (excepting CObjBase, which inheritance is only virtual)
    int64 iCurTime = CWorldGameTime::GetCurrentTime().GetTimeRaw();    // Current timestamp, a few msecs will advance in the current tick ... avoid them until the following tick(s).

    {
        // Need this new scope to give the right lifetime to ProfileTask.
        //g_Log.EventDebug("Start ctimedobj section.\n");
        EXC_SET_BLOCK("TimedObjects");
        const ProfileTask timersTask(PROFILE_TIMERS);
        {
            // Need here another scope to give the right lifetime to the unique_lock.
#if MT_ENGINES
            std::unique_lock<std::shared_mutex> lock(_mWorldTickList.MT_CMUTEX);
#endif
            {
                // New requests done during the world loop.
                EXC_TRYSUB("Update main list");
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
                g_Log.EventDebug("[GLOBAL] STATUS: Updating WorldTickList.\n");
#endif
                for (CTimedObject* elem : _vecWorldObjsEraseRequests)
                {
                    elem->_fIsAddingInWorldTickList = false;
                    elem->_fIsInWorldTickList = false;
                }
                for (TickingTimedObjEntry& elem : _vecWorldObjsAddRequests)
                {
                    elem.second->_fIsAddingInWorldTickList = false;
                    elem.second->_fIsInWorldTickList = true;
                }

                sortedVecRemoveAddQueued(_mWorldTickList, _vecWorldObjsEraseRequests, _vecWorldObjsAddRequests, _vecWorldObjsElementBuffer);
                EXC_CATCHSUB("");
            }


            // Need here a new, inner scope to get rid of EXC_TRYSUB variables
            if (!_mWorldTickList.empty())
            {
                {
                    EXC_TRYSUB("Selection");
                    _vecIndexMiscBuffer.clear();
                    WorldTickList::iterator itMap = _mWorldTickList.begin();
                    WorldTickList::iterator itMapEnd = _mWorldTickList.end();

                    size_t uiProgressive = 0;
                    int64 iTime;
                    while ((itMap != itMapEnd) && (iCurTime > (iTime = itMap->first)))
                    {
                        CTimedObject* pTimedObj = itMap->second;
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
                        g_Log.EventDebug("Checking if CTimedObject %p should tick.\n", reinterpret_cast<void*>(pTimedObj));
#endif
                        if (pTimedObj->_IsTimerSet() && pTimedObj->_CanTick())
                        {
                            if (pTimedObj->_GetTimeoutRaw() <= iCurTime)
                            {
                                if (auto pObjBase = dynamic_cast<const CObjBase*>(pTimedObj))
                                {
                                    if (pObjBase->_IsBeingDeleted())
                                        continue;
                                }

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
                                g_Log.EventDebug("Yes it should (%p).\n", reinterpret_cast<void*>(pTimedObj));
#endif
                                _vecGenericObjsToTick.emplace_back(static_cast<void*>(pTimedObj));
                                _vecIndexMiscBuffer.emplace_back(uiProgressive);
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

#ifdef DEBUG_LIST_OPS
                ASSERT(!sl::UnsortedContainerHasDuplicates(_vecGenericObjsToTick));
#endif
                {
                    EXC_TRYSUB("Delete from List");
                    sortedVecRemoveElementsByIndices(_mWorldTickList, _vecIndexMiscBuffer);

#ifdef DEBUG_LIST_OPS
                    // Ensure
                    for (void* obj : _vecGenericObjsToTick)
                    {
                        auto itit = std::find_if(_mWorldTickList.begin(), _mWorldTickList.end(),
                            [obj](TickingTimedObjEntry const& lhs) {
                                return static_cast<CTimedObject*>(obj) == lhs.second;
                            });
                        UnreferencedParameter(itit);
                        ASSERT(itit == _mWorldTickList.end());
                    }
#endif
                    EXC_CATCHSUB("");
                }


                // Done working with _mWorldTickList, we don't need the lock from now on.

                lpctstr ptcSubDesc;
                for (void* pObjVoid : _vecGenericObjsToTick)    // Loop through all msecs stored, unless we passed the timestamp.
                {
                    ptcSubDesc = "Generic";

                    EXC_TRYSUB("Tick");
                    EXC_SETSUB_BLOCK("Elapsed");

                    CTimedObject* pTimedObj = static_cast<CTimedObject*>(pObjVoid);
                    pTimedObj->_ClearTimeoutRaw();

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
/*
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
            else
                g_Log.EventDebug("No ctimedobj ticks for this loop.\n");
#endif
*/
        }
    }

    _vecGenericObjsToTick.clear();

    // ----

    /* Periodic, automatic ticking for every char */

    // No need another scope here to encapsulate this ProfileTask, because from now on, to the end of this method,
    //  everything we do is related to char-only stuff.
    //g_Log.EventDebug("Start periodic ticks section.\n");
    EXC_SET_BLOCK("Char Periodic Ticks");
    const ProfileTask taskChars(PROFILE_CHARS);
    {
        // Need here another scope to give the right lifetime to the unique_lock.
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(_mCharTickList.MT_CMUTEX);
#endif
        {
            // New requests done during the world loop.
            EXC_TRYSUB("Update main list");

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
            g_Log.EventDebug("[GLOBAL] STATUS: Updating CharTickList.\n");
#endif
            sortedVecRemoveAddQueued(_mCharTickList, _vecPeriodicCharsEraseRequests, _vecPeriodicCharsAddRequests, _vecPeriodicCharsElementBuffer);
            EXC_CATCHSUB("");
            ASSERT(sl::ContainerIsSorted(_mCharTickList));
            ASSERT(!sl::SortedContainerHasDuplicates(_mCharTickList));
        }

        if (!_mCharTickList.empty())
        {
            {
                EXC_TRYSUB("Selection");
#ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
                //g_Log.EventDebug("Start looping through char periodic ticks.\n");
#endif
                _vecIndexMiscBuffer.clear();
                CharTickList::iterator itMap       = _mCharTickList.begin();
                CharTickList::iterator itMapEnd    = _mCharTickList.end();

                size_t uiProgressive = 0;
                int64 iTime;
                while ((itMap != itMapEnd) && (iCurTime > (iTime = itMap->first)))
                {
                    CChar* pChar = itMap->second;
#ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
                    g_Log.EventDebug("Executing char periodic tick: %p. Registered time: %" PRId64 ". pChar->_iTimePeriodicTick: %" PRId64 "\n",
                        (void*)pChar, itMap->first, pChar->_iTimePeriodicTick);
                    ASSERT(itMap->first == pChar->_iTimePeriodicTick);
#endif
                    if ((pChar->_iTimePeriodicTick != 0) && pChar->_CanTick() && !pChar->_IsBeingDeleted())
                    {
                        if (pChar->_iTimePeriodicTick <= iCurTime)
                        {
                            _vecGenericObjsToTick.emplace_back(static_cast<void*>(pChar));
                            _vecIndexMiscBuffer.emplace_back(uiProgressive);
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

#ifdef DEBUG_LIST_OPS
                ASSERT(sl::ContainerIsSorted(_vecIndexMiscBuffer));
                ASSERT(!sl::UnsortedContainerHasDuplicates(_vecGenericObjsToTick));
                ASSERT(!sl::SortedContainerHasDuplicates(_vecIndexMiscBuffer));
#endif

#ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
                g_Log.EventDebug("Done looping through char periodic ticks. Need to tick n %" PRIuSIZE_T " objs.\n", _vecGenericObjsToTick.size());
#endif
            }

            {
                EXC_TRYSUB("Delete from List");
                // Erase in chunks, call erase the least times possible.
                sortedVecRemoveElementsByIndices(_mCharTickList, _vecIndexMiscBuffer);
                EXC_CATCHSUB("DeleteFromList");

                _vecIndexMiscBuffer.clear();
            }

            // Done working with _mCharTickList, we don't need the lock from now on.
        }
#ifdef DEBUG_CCHAR_PERIODIC_TICKING
        else
        {}; //g_Log.EventDebug("No char periodic ticks for this loop.\n");
#endif
    }

    {
        EXC_TRYSUB("Char Periodic Ticks Loop");
        for (void* pObjVoid : _vecGenericObjsToTick)    // Loop through all msecs stored, unless we passed the timestamp.
        {
            CChar* pChar = static_cast<CChar*>(pObjVoid);
            pChar->_iTimePeriodicTick = 0;
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

        _vecGenericObjsToTick.clear();
    }

    //g_Log.EventDebug("END periodic ticks section.\n");

    EXC_CATCH;
}
