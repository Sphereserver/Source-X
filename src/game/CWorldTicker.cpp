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

    _vWorldTicks.reserve(50);
    _vWorldObjsAddRequests.reserve(50);
    _vWorldObjsEraseRequests.reserve(50);

    _vCharTicks.reserve(50);
    _vPeriodicCharsAddRequests.reserve(50);
    _vPeriodicCharsEraseRequests.reserve(50);

    _vObjStatusUpdates.reserve(25);
    _vObjStatusUpdatesAddRequests.reserve(25);
    _vObjStatusUpdatesEraseRequests.reserve(25);
}


// CTimedObject TIMERs

auto CWorldTicker::IsTimeoutRegistered(const CTimedObject* pTimedObject) -> std::optional<std::pair<int64, CTimedObject*>>
{
    // Use this only for debugging!

    ASSERT(pTimedObject);
    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) constexpr noexcept {
        return pTimedObject == rhs.second;
    };

    const auto itEntryInAddList = std::find_if(
        _vWorldObjsAddRequests.begin(),
        _vWorldObjsAddRequests.end(),
        fnFindEntryByObj);
    if (_vWorldObjsAddRequests.end() != itEntryInAddList)
        return *itEntryInAddList;

    const auto itEntryInTickList = std::find_if(
        _vWorldTicks.begin(),
        _vWorldTicks.end(),
        fnFindEntryByObj);
    if (_vWorldTicks.end() != itEntryInTickList)
    {
        const auto itEntryInEraseList = std::find(
            _vWorldObjsEraseRequests.begin(),
            _vWorldObjsEraseRequests.end(),
            pTimedObject);

        if (itEntryInEraseList == _vWorldObjsEraseRequests.end())
            return *itEntryInTickList;
    }

    return std::nullopt;
}

bool CWorldTicker::_InsertTimedObject(const int64 iTimeout, CTimedObject* pTimedObject)
{
    ASSERT(pTimedObject);
    ASSERT(iTimeout != 0);

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    ASSERT(sl::ContainerIsSorted(_vWorldTicks));
    ASSERT(!sl::SortedContainerHasDuplicates(_vWorldTicks));
#endif

    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) constexpr noexcept {
        return pTimedObject == rhs.second;
    };
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vWorldTicks.MT_CMUTEX);
#endif

    if (pTimedObject->_fIsInWorldTickAddList)
    {
        const auto itEntryInAddList = std::find_if(
            _vWorldObjsAddRequests.begin(),
            _vWorldObjsAddRequests.end(),
            fnFindEntryByObj);
        ASSERT(_vWorldObjsAddRequests.end() != itEntryInAddList);

        // What's happening: I am requesting to add an element, but this was already done in this tick (there's already a request in the buffer).
        // To solve this (it might happen and it's legit), we just update the Timeout for the object we want to add.

        itEntryInAddList->first = iTimeout;

        ASSERT(pTimedObject->_fIsInWorldTickList == false);
        ASSERT(pTimedObject->_fIsInWorldTickAddList == true);

        return true; // Legit.
    }

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    // Debug redundant checks.

    const auto itEntryInTickList = std::find_if(
        _vWorldTicks.begin(),
        _vWorldTicks.end(),
        fnFindEntryByObj);
    if (_vWorldTicks.end() != itEntryInTickList)
    {
        // What's happening: I am requesting to add an element, but we already have it in the main ticking list (we are adding another one without deleting the old one?).

        const auto itEntryInEraseList = std::find(
            _vWorldObjsEraseRequests.begin(),
            _vWorldObjsEraseRequests.end(),
            pTimedObject);
        if (_vWorldObjsEraseRequests.end() == itEntryInEraseList)
        {
            // We don't have an erase request. We have to remove the existing one before adding a new one.

            ASSERT(false);
            return false;   // Not legit.
        }
    }
    else
    {
        // Legit, it isn't a duplicate.
        // Now ensure it isn't in the erase list. It's even a more redundant check, because if it's not in the main ticking list, there shouldn't be an erase request.
        const auto itEntryInEraseList = std::find(
            _vWorldObjsEraseRequests.begin(),
            _vWorldObjsEraseRequests.end(),
            pTimedObject);
        ASSERT(_vWorldObjsEraseRequests.end() == itEntryInEraseList);
    }
#endif

    _vWorldObjsAddRequests.emplace_back(iTimeout, pTimedObject);

    ASSERT(pTimedObject->_fIsInWorldTickList == false);
    pTimedObject->_fIsInWorldTickAddList = true;

    return true;
}

bool CWorldTicker::_RemoveTimedObject(CTimedObject* pTimedObject)
{
    ASSERT(pTimedObject);

#ifdef DEBUG_LIST_OPS
    ASSERT(sl::ContainerIsSorted(_vWorldTicks));
    ASSERT(!sl::SortedContainerHasDuplicates(_vWorldTicks));
#endif

    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) constexpr noexcept {
        return pTimedObject == rhs.second;
    };

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vWorldTicks.MT_CMUTEX);
#endif

    if (pTimedObject->_fIsInWorldTickAddList)
    {
        // What's happening: it has the flag, so it must be in the add list. Ensure it, and remove it from there.
        //  We are reasonably sure that there isn't another entry in the main ticking list, because we would have gotten an error
        //  trying to append a request to the add list.
        const auto itEntryInAddList = std::find_if(
            _vWorldObjsAddRequests.begin(),
            _vWorldObjsAddRequests.end(),
            fnFindEntryByObj);
        ASSERT(itEntryInAddList != _vWorldObjsAddRequests.end());

        _vWorldObjsAddRequests.erase(itEntryInAddList);

        ASSERT(pTimedObject->_fIsInWorldTickList == false);
        pTimedObject->_fIsInWorldTickAddList = false;

        return true; // Legit.
    }

    // We don't allow duplicate delete requests.
    // This shouldn't happen because we set here _fIsInWorldTickList to false and we shouldn't ahve triggered another delete request.
    if (!pTimedObject->_fIsInWorldTickList)
    {
        // It isn't in the ticking list, or we already have received a remove request.
        ASSERT(false);  // Not legit.
    }

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    // Redundant check.
    const auto itEntryInRemoveList = std::find(
        _vWorldObjsEraseRequests.begin(),
        _vWorldObjsEraseRequests.end(),
        pTimedObject);
    if (_vWorldObjsEraseRequests.end() != itEntryInRemoveList)
    {

        const auto itEntryInAddList = std::find_if(
            _vWorldObjsAddRequests.begin(),
            _vWorldObjsAddRequests.end(),
            fnFindEntryByObj);
        ASSERT(itEntryInAddList == _vWorldObjsAddRequests.end());

        const auto itEntryInTickList = std::find_if(
            _vWorldTicks.begin(),
            _vWorldTicks.end(),
            fnFindEntryByObj);
        ASSERT(itEntryInTickList != _vWorldTicks.end());

        ASSERT(false);
        return false; // Already requested the removal.
    }
#endif

    // At this point, we should be fairly sure that the object is in the tick list.
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    // Redundant check.

    const auto itEntryInTickList = std::find_if(
        _vWorldTicks.begin(),
        _vWorldTicks.end(),
        fnFindEntryByObj);
    if (itEntryInTickList == _vWorldTicks.end())
    {
        // Not found -> not legit.
        // It has to be acknowledged that the object might have a timeout while being in a non-tickable state (like at server startup), so it isn't in the list,
        //  nevertheless we shouldn't have blindly tried to remove it from the list.
        ASSERT(false);
        return false;
    }
#endif

    _vWorldObjsEraseRequests.emplace_back(pTimedObject);

    pTimedObject->_fIsInWorldTickList = false;
    ASSERT(pTimedObject->_fIsInWorldTickAddList == false);

    return true;
}

bool CWorldTicker::AddTimedObject(const int64 iTimeout, CTimedObject* pTimedObject, bool fForce)
{
    //if (iTimeout < CWorldGameTime::GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible; don't uncomment.
    //    return;

    EXC_TRY("AddTimedObject");
    ASSERT(pTimedObject);
    const ProfileTask timersTask(PROFILE_TIMERS);

    EXC_SET_BLOCK("Already ticking?");
    if (pTimedObject->_fIsInWorldTickList)
    {
        // Do not check with IsTimeoutTickingActive(), because if it's in the add list, i can just overwrite the entry timer,
        //  before actually adding it to the main ticking list. Remove the entry only if it's already in the main ticking list.

        // Adding an object that might already be in the list?
        // Or, am i setting a new timeout without deleting the previous one?
        // I can have iTickOld != 0 but not be in the list in some cases, like when duping an obj with an active timer.
        EXC_SET_BLOCK("Remove");

        const bool fRemoved = _RemoveTimedObject(pTimedObject);
        ASSERT(fRemoved);
        UnreferencedParameter(fRemoved);
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
        /*
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
        */
    }

    if (fCanTick)
    {
        const bool fRet = _InsertTimedObject(iTimeout, pTimedObject);
        ASSERT(fRet);
        fCanTick = fRet;
    }
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
    else
        g_Log.EventDebug("[WorldTicker][%p] INFO: Cannot tick and no force, so i'm not adding CTimedObj to the ticking list.\n", (void*)pTimedObject);
#endif

    return fCanTick;

    EXC_CATCH;
    return false;
}

bool CWorldTicker::DelTimedObject(CTimedObject* pTimedObject)
{
    EXC_TRY("DelTimedObject");
    const ProfileTask timersTask(PROFILE_TIMERS);

    EXC_SET_BLOCK("Remove");
    const bool fRet = _RemoveTimedObject(pTimedObject);
    ASSERT(fRet);

    return fRet;

    EXC_CATCH;
    return false;
}


// CChar Periodic Ticks (it's a different thing than TIMER!)

auto CWorldTicker::IsCharPeriodicTickRegistered(const CChar* pChar) -> std::optional<std::pair<int64, CChar*>>
{
    // Use this only for debugging!

    ASSERT(pChar);
    const auto fnFindEntryByObj = [pChar](TickingPeriodicCharEntry const& rhs) constexpr noexcept {
        return pChar == rhs.second;
    };

    const auto itEntryInAddList = std::find_if(
        _vPeriodicCharsAddRequests.begin(),
        _vPeriodicCharsAddRequests.end(),
        fnFindEntryByObj);
    if (_vPeriodicCharsAddRequests.end() != itEntryInAddList)
        return *itEntryInAddList;

    const auto itEntryInTickList = std::find_if(
        _vCharTicks.begin(),
        _vCharTicks.end(),
        fnFindEntryByObj);
    if (_vCharTicks.end() != itEntryInTickList)
    {
        const auto itEntryInEraseList = std::find(
            _vPeriodicCharsEraseRequests.begin(),
            _vPeriodicCharsEraseRequests.end(),
            pChar);

        if (itEntryInEraseList == _vPeriodicCharsEraseRequests.end())
            return *itEntryInTickList;
    }

    return std::nullopt;
}

bool CWorldTicker::_InsertCharTicking(const int64 iTickNext, CChar* pChar)
{
    ASSERT(iTickNext != 0);
    ASSERT(pChar);

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vCharTicks.MT_CMUTEX);
#endif

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
    const auto fnFindEntryByChar = [pChar](TickingPeriodicCharEntry const& rhs) constexpr noexcept {
        return pChar == rhs.second;
    };

    const auto itEntryInAddList = std::find_if(
        _vPeriodicCharsAddRequests.begin(),
        _vPeriodicCharsAddRequests.end(),
        fnFindEntryByChar);
    if (_vPeriodicCharsAddRequests.end() != itEntryInAddList)
    {
        // We already had requested the insertion of this element.
        // We could just update periodic tick time, but by design we will add new periodic ticks only once per periodic tick.
        // If it happens more than once, we are making a mistake somewhere.

        ASSERT(false);
        return false; // Not legit.
    }

    const auto itEntryInEraseList = std::find(
        _vPeriodicCharsEraseRequests.begin(),
        _vPeriodicCharsEraseRequests.end(),
        pChar);
    if (_vPeriodicCharsEraseRequests.end() != itEntryInEraseList)
    {
        // Adding another one after we requested to remove it?
        // We could manage this, but we do not have this scenario, it shouldn't happen and we don't need to.
        // If it happens, we are making a mistake somewhere.

        ASSERT(false);
        return false; // Not legit.
    }

    // Do not add duplicates.
    const auto itEntryInTickList = std::find_if(
        _vCharTicks.begin(),
        _vCharTicks.end(),
        fnFindEntryByChar);
    ASSERT(_vCharTicks.end() == itEntryInTickList);
#endif

    _vPeriodicCharsAddRequests.emplace_back(iTickNext, pChar);

    return true;
}

bool CWorldTicker::_RemoveCharTicking(CChar* pChar)
{
    // I'm reasonably sure that the element i'm trying to remove is present in this container.
    ASSERT(pChar);

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vCharTicks.MT_CMUTEX);
#endif

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
    const auto fnFindEntryByChar = [pChar](TickingPeriodicCharEntry const& rhs) constexpr noexcept {
        return pChar == rhs.second;
    };

    const auto itEntryInRemoveList = std::find(
        _vPeriodicCharsEraseRequests.begin(),
        _vPeriodicCharsEraseRequests.end(),
        pChar);
    if (_vPeriodicCharsEraseRequests.end() != itEntryInRemoveList)
    {
        // Do not ask to remove it more than once per tick.
        // We have ways to check if we already did that via IsPeriodicTickPending.

        ASSERT(false);
        return false; // Not legit.
    }
#endif

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
    const auto itEntryInAddList = std::find_if(
        _vPeriodicCharsAddRequests.begin(),
        _vPeriodicCharsAddRequests.end(),
        fnFindEntryByChar);
    if (_vPeriodicCharsAddRequests.end() != itEntryInAddList)
    {
        // We are trying to remove something that was requested to be added.
        // While we could support it, we don't need to do this and it would be superfluous.
        // If it happens, we are making a mistake somewhere.

        ASSERT(false);
        return false;

        // In any case, for now we keep the code to remove it.
/*
        _vPeriodicCharsAddRequests.erase(itEntryInAddList);

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
        const auto itEntryInTickList = std::find_if(
            _vCharTicks.begin(),
            _vCharTicks.end(),
            fnFindEntryByChar);

        if (itEntryInRemoveList == _vPeriodicCharsEraseRequests.end()) {
            ASSERT(itEntryInTickList == _vCharTicks.end());
        }
        else {
            ASSERT(itEntryInTickList != _vCharTicks.end());
        }
#endif
        return true
*/;
    }
#endif

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
    // Ensure it's in the ticking list.
    const auto itEntryInTickList = std::find_if(
        _vCharTicks.begin(),
        _vCharTicks.end(),
        fnFindEntryByChar);
    if (itEntryInTickList == _vCharTicks.end())
    {
        // Requested TickingPeriodicChar removal from ticking list, but not found.
        // Shouldn't happen, so it's illegal.

        ASSERT(false);
        return false;   // Not legit.
    }

    if (_vPeriodicCharsEraseRequests.end() != itEntryInRemoveList)
    {
        // We have already requested to remove this from the main ticking list.
        ASSERT(itEntryInAddList == _vPeriodicCharsAddRequests.end());

        ASSERT(false);
        return false; // Not legit.
    }
#endif

    _vPeriodicCharsEraseRequests.emplace_back(pChar);

    return true;
}

bool CWorldTicker::AddCharTicking(CChar* pChar, bool fNeedsLock)
{
    // Add CChar to the periodic ticking list.
    EXC_TRY("AddCharTicking");

    const ProfileTask timersTask(PROFILE_TIMERS);

    int64 iTickNext, iTickOld;
    bool fTickPending;
    UnreferencedParameter(fNeedsLock);
#if MT_ENGINES
    if (fNeedsLock)
    {
        std::unique_lock<std::shared_mutex> lock(pChar->MT_CMUTEX);
        iTickNext = pChar->_iTimeNextRegen;
        iTickOld = pChar->_iTimePeriodicTick;
        fTickPending = pChar->IsPeriodicTickPending();
    }
    else
#endif
    {
        iTickNext = pChar->_iTimeNextRegen;
        iTickOld = pChar->_iTimePeriodicTick;
        fTickPending = pChar->IsPeriodicTickPending();
    }

    ASSERT(iTickNext != 0);
    if (iTickNext == iTickOld)
    {
        // Are we adding a new entry, but it has the current periodic ticking time instead of a new one?

        ASSERT(false);
        return false;
    }

    // We do that to get them tick as sooner as possible, so don't uncomment this.
    //if (iTickNext < CWorldGameTime::GetCurrentTime().GetTimeRaw())
    //    return;

    if (fTickPending)
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        EXC_SET_BLOCK("Remove");
        const bool fRemoved = _RemoveCharTicking(pChar);
        ASSERT(fRemoved);
        UnreferencedParameter(fRemoved);
    }

    EXC_SET_BLOCK("Insert");
    const bool fRet = _InsertCharTicking(iTickNext, pChar);
    ASSERT(fRet);
    UnreferencedParameter(fRet);

#if MT_ENGINES
    if (fNeedsLock)
    {
        std::unique_lock<std::shared_mutex> lock(pChar->MT_CMUTEX);
        pChar->_iTimePeriodicTick = iTickNext;
    }
    else
#endif
    {
        pChar->_iTimePeriodicTick = iTickNext;
    }

    return fRet;

    EXC_CATCH;
    return false;
}

bool CWorldTicker::DelCharTicking(CChar* pChar, bool fNeedsLock)
{
    EXC_TRY("DelCharTicking");
    const ProfileTask timersTask(PROFILE_TIMERS);

    UnreferencedParameter(fNeedsLock);

// DEBUG the IsPeriodicTickPending method, double check to ensure it works properly.
/*
    bool fIsTickPending;
#if MT_ENGINES
    if (fNeedsLock)
    {
        std::unique_lock<std::shared_mutex> lock(pChar->MT_CMUTEX);
        fIsTickPending = !pChar->IsPeriodicTickPending();
    }
    else
#endif
    {
        fIsTickPending = !pChar->IsPeriodicTickPending();
    }

    if (fIsTickPending)
    {
#ifdef DEBUG_CCHAR_PERIODIC_TICKING
#   ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO: Requested deletion of Periodic char, but Timeout is 0. It shouldn't be in the list, or just queued to be removed.\n", (void*)pChar);
#   endif
        auto fnFindEntryByChar = [pChar](const TickingPeriodicCharEntry& entry) noexcept {
                return entry.second == pChar;
        };

        const auto itEntryInEraseList = std::find(
            _vPeriodicCharsEraseRequests.begin(),
            _vPeriodicCharsEraseRequests.end(),
            pChar);
        if (itEntryInEraseList != _vPeriodicCharsEraseRequests.end())
        {
#   ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
            g_Log.EventDebug("[WorldTicker][%p] INFO:   though, found it already in the removal list, so it's fine..\n", (void*)pChar);
#   endif

            ASSERT(false);
            return false;
        }

        const auto itEntryInTickList = std::find_if(
            _vCharTicks.begin(),
            _vCharTicks.end(),
            fnFindEntryByChar);
        if (itEntryInTickList != _vCharTicks.end())
        {
#   ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
            g_Log.EventDebug("[WorldTicker][%p] WARN:   But i have found it in the list! With Timeout %" PRId64 ".\n", (void*)pChar, itEntryInTickList->first);
#   endif

            ASSERT(false);
            return false;
        }

#   ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
        g_Log.EventDebug("[WorldTicker][%p] INFO:   (rightfully) i haven't found it in any list.\n", (void*)pChar);
#   endif
#endif

        ASSERT(false);
        return false;
    }
*/

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
#if MT_ENGINES
    if (fNeedsLock)
    {
        std::unique_lock<std::shared_mutex> lock(pChar->MT_CMUTEX);
        ASSERT(pChar->IsPeriodicTickPending());
    }
    else
#endif
    {
        ASSERT(pChar->IsPeriodicTickPending());
    }
#endif

    EXC_SET_BLOCK("Remove");
    const bool fRet = _RemoveCharTicking(pChar);
    if (fRet)
    {
#if MT_ENGINES
        if (fNeedsLock)
        {
            std::unique_lock<std::shared_mutex> lock(pChar->MT_CMUTEX);
            pChar->_iTimePeriodicTick = 0;
        }
        else
#endif
        {
            pChar->_iTimePeriodicTick = 0;
        }
    }

    return fRet;

    EXC_CATCH;
    return false;
}


// CObjBase Status Updates.

bool CWorldTicker::IsStatusUpdateTickRegistered(const CObjBase *pObj)
{
    // Use this only for debugging!

    /*
    const auto itEntryInAddList = std::find(
        _vecObjStatusUpdatesAddRequests.begin(),
        _vecObjStatusUpdatesAddRequests.end(),
        pObj);
    if (_vecObjStatusUpdatesAddRequests.end() != itEntryInAddList)
        return *itEntryInAddList;
    */

    const auto itEntryInTickList = std::find(
        _vObjStatusUpdates.begin(),
        _vObjStatusUpdates.end(),
        pObj);
    if (_vObjStatusUpdates.end() != itEntryInTickList)
    {
        const auto itEntryInEraseList = std::find(
            _vObjStatusUpdatesEraseRequests.begin(),
            _vObjStatusUpdatesEraseRequests.end(),
            pObj);

        if (itEntryInEraseList == _vObjStatusUpdatesEraseRequests.end())
            return true;
    }

    return false;
}

bool CWorldTicker::AddObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
    EXC_TRY("AddObjStatusUpdate");
    ASSERT(pObj);
    const ProfileTask timersTask(PROFILE_TIMERS);

    UnreferencedParameter(fNeedsLock);
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vCharTicks.MT_CMUTEX);
#endif

    if (pObj->_fIsInStatusUpdatesAddList)
    {
        const auto itEntryInAddList = std::find(
            _vObjStatusUpdatesAddRequests.begin(),
            _vObjStatusUpdatesAddRequests.end(),
            pObj);
        ASSERT(_vObjStatusUpdatesAddRequests.end() != itEntryInAddList);

        // I am requesting to add an element, but this was already done in this tick (there's already a request in the buffer).
        // We don't want duplicates.

        return false; // Not legit.
    }

#ifdef DEBUG_STATUSUPDATES
    // Debug redundant checks.

    const auto itEntryInTickList = std::find(
        _vObjStatusUpdates.begin(),
        _vObjStatusUpdates.end(),
        pObj);
    if (_vObjStatusUpdates.end() != itEntryInTickList)
    {
        // What's happening: I am requesting to add an element, but we already have it in the status updates list.

        // If it's in the erase list:
        //  Are we trying to add an element after we requested to remove it?
        //  We could manage this, but we do not have this scenario, it shouldn't happen and we don't need to.
        //  If it happens, we are making a mistake somewhere.
        // If it's not:
        //  This would be illegal because we are trying to add a duplicate.

        ASSERT(false);
        return false;
    }
    else
    {
        const auto itEntryInEraseList = std::find(
            _vObjStatusUpdatesEraseRequests.begin(),
            _vObjStatusUpdatesEraseRequests.end(),
            pObj);
        ASSERT(_vObjStatusUpdatesEraseRequests.end() == itEntryInEraseList);
    }
#endif

    _vObjStatusUpdatesAddRequests.emplace_back(pObj);

    ASSERT(pObj->_fIsInStatusUpdatesList == false);
    pObj->_fIsInStatusUpdatesAddList = true;

    return true;
    EXC_CATCH;

    return false;
}

bool CWorldTicker::DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
    EXC_TRY("DelObjStatusUpdate");
    ASSERT(pObj);
    const ProfileTask timersTask(PROFILE_TIMERS);

    UnreferencedParameter(fNeedsLock);
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vObjStatusUpdates.MT_CMUTEX);
#endif

    if (pObj->_fIsInStatusUpdatesAddList)
    {
        // What's happening: it has the flag, so it must be in the add list. Ensure it, and remove it from there.
        //  We are reasonably sure that there isn't another entry in the main ticking list, because we would have gotten an error
        //  trying to append a request to the add list.
        const auto itEntryInAddList = std::find(
            _vObjStatusUpdatesAddRequests.begin(),
            _vObjStatusUpdatesAddRequests.end(),
            pObj);
        ASSERT(itEntryInAddList != _vObjStatusUpdatesAddRequests.end());

        _vObjStatusUpdatesAddRequests.erase(itEntryInAddList);

        ASSERT(pObj->_fIsInStatusUpdatesList == false);
        pObj->_fIsInStatusUpdatesAddList = false;

        return true;
    }

    // We don't allow duplicate delete requests.
    // This shouldn't happen because we set here _fIsInWorldTickList to false and we shouldn't ahve triggered another delete request.
    if (!pObj->_fIsInStatusUpdatesList)
    {
        // It isn't in the ticking list, or we already have received a remove request.
        ASSERT(false);  // Not legit.
    }

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    // Redundant check.
    const auto itEntryInRemoveList = std::find(
        _vObjStatusUpdatesEraseRequests.begin(),
        _vObjStatusUpdatesEraseRequests.end(),
        pObj);
    if (_vObjStatusUpdatesEraseRequests.end() != itEntryInRemoveList)
    {
        const auto itEntryInAddList = std::find(
            _vObjStatusUpdatesAddRequests.begin(),
            _vObjStatusUpdatesAddRequests.end(),
            pObj);
        ASSERT(itEntryInAddList == _vObjStatusUpdatesAddRequests.end());

        const auto itEntryInTickList = std::find(
            _vObjStatusUpdates.begin(),
            _vObjStatusUpdates.end(),
            pObj);
        ASSERT(itEntryInTickList != _vObjStatusUpdates.end());

        ASSERT(false);
        return false; // Already requested the removal.
    }
#endif

    // At this point, we should be fairly sure that the object is in the tick list.
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    const auto itEntryInTickList = std::find(
        _vObjStatusUpdates.begin(),
        _vObjStatusUpdates.end(),
        pObj);
    if (itEntryInTickList == _vObjStatusUpdates.end())
    {
        // Not found!?
        ASSERT(false);
        return false;
    }
#endif

    _vObjStatusUpdatesEraseRequests.emplace_back(pObj);

    pObj->_fIsInStatusUpdatesList = false;
    ASSERT(pObj->_fIsInStatusUpdatesAddList == false);

    return true;

    EXC_CATCH;
    return false;
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

    // Copy the original vector to check against later
    std::vector<T> originalVecMain = vecMain;
#endif

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

#ifdef DEBUG_LIST_OPS
    for (auto& elem : vecMain) {
        ASSERT(std::find(vecValuesToRemove.begin(), vecValuesToRemove.end(), elem) == vecValuesToRemove.end());
    }
#endif
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
#endif
    ASSERT(vecElemBuffer.size() == vecMain.size() - vecToRemove.size());
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

        // TODO: use a sorted algorithm?
        unsortedVecDifference(vecMain, vecToRemove, vecElemBuffer);

#ifdef DEBUG_LIST_OPS
        for (auto& elem : vecToRemove)
        {
            auto it = std::find_if(vecElemBuffer.begin(), vecElemBuffer.end(), [elem](auto &rhs) constexpr noexcept {return elem == rhs.second;});
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
        //vecToRemove.clear();
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
        //vecToAdd.clear();

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
                std::unique_lock<std::shared_mutex> lock_su(_vObjStatusUpdates.MT_CMUTEX);
#endif
                EXC_TRYSUB("StatusUpdates");
                EXC_SETSUB_BLOCK("Remove requested");

                for (CObjBase* elem : _vObjStatusUpdatesAddRequests)
                {
                    elem->_fIsInStatusUpdatesAddList = false;
                    elem->_fIsInStatusUpdatesList = true;
                }

                unsortedVecRemoveElementsByValues(_vObjStatusUpdates, _vObjStatusUpdatesEraseRequests);
                _vObjStatusUpdates.insert(_vObjStatusUpdates.end(), _vObjStatusUpdatesAddRequests.begin(), _vObjStatusUpdatesAddRequests.end());

                EXC_SETSUB_BLOCK("Selection");
                _vecGenericObjsToTick.clear();
                if (!_vObjStatusUpdates.empty())
                {                    
                    for (CObjBase* pObj : _vObjStatusUpdates)
                    {
                        if (pObj)
                        {
                            pObj->_fIsInStatusUpdatesList = false;
                            if (!pObj->_IsBeingDeleted())
                                _vecGenericObjsToTick.emplace_back(static_cast<void*>(pObj));
                        }
                    }
                }
                EXC_CATCHSUB("");
                //EXC_DEBUGSUB_START;
                //EXC_DEBUGSUB_END;
                _vObjStatusUpdates.clear();
                _vObjStatusUpdatesAddRequests.clear();
                _vObjStatusUpdatesEraseRequests.clear();
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
            std::unique_lock<std::shared_mutex> lock(_vWorldTicks.MT_CMUTEX);
#endif
            {
                // New requests done during the world loop.
                EXC_TRYSUB("Update main list");

                /*
                for (CTimedObject* elem : _vWorldObjsEraseRequests)
                {
                    elem->_fIsInWorldTickAddList = false;
                    elem->_fIsInWorldTickList = false;
                }
                */
                for (TickingTimedObjEntry& elem : _vWorldObjsAddRequests)
                {
                    ASSERT(elem.second->_fIsInWorldTickAddList == true);
                    elem.second->_fIsInWorldTickAddList = false;
                    elem.second->_fIsInWorldTickList = true;
                }

                sortedVecRemoveAddQueued(_vWorldTicks, _vWorldObjsEraseRequests, _vWorldObjsAddRequests, _vecWorldObjsElementBuffer);
                EXC_CATCHSUB("");
            }

            _vWorldObjsAddRequests.clear();
            _vWorldObjsEraseRequests.clear();
            _vecWorldObjsElementBuffer.clear();

            // Need here a new, inner scope to get rid of EXC_TRYSUB variables
            _vecIndexMiscBuffer.clear();
            if (!_vWorldTicks.empty())
            {
                {
                    EXC_TRYSUB("Selection");
                    WorldTickList::iterator itMap = _vWorldTicks.begin();
                    WorldTickList::iterator itMapEnd = _vWorldTicks.end();

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
                                    if (!pObjBase->_IsBeingDeleted())
                                    {
                                        // Object should tick.
                                        pTimedObj->_fIsInWorldTickList = false;
                                        _vecGenericObjsToTick.emplace_back(static_cast<void*>(pTimedObj));
                                        _vecIndexMiscBuffer.emplace_back(uiProgressive);
                                    }
                                }
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
                    sortedVecRemoveElementsByIndices(_vWorldTicks, _vecIndexMiscBuffer);

#ifdef DEBUG_LIST_OPS
                    // Ensure that the sortedVecRemoveElementsByIndices worked. No element of _vecGenericObjsToTick has to still be in _vWorldTicks.
                    for (void* obj : _vecGenericObjsToTick)
                    {
                        auto itit = std::find_if(_vWorldTicks.begin(), _vWorldTicks.end(),
                            [obj](TickingTimedObjEntry const& lhs) constexpr noexcept {
                                return static_cast<CTimedObject*>(obj) == lhs.second;
                            });
                        UnreferencedParameter(itit);
                        ASSERT(itit == _vWorldTicks.end());
                    }
#endif
                    EXC_CATCHSUB("");
                }


                // Done working with _vWorldTicks, we don't need the lock from now on.

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
        std::unique_lock<std::shared_mutex> lock(_vCharTicks.MT_CMUTEX);
#endif
        {
            // New requests done during the world loop.
            EXC_TRYSUB("Update main list");

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING_VERBOSE
            g_Log.EventDebug("[GLOBAL] STATUS: Updating CharTickList.\n");
#endif
            sortedVecRemoveAddQueued(_vCharTicks, _vPeriodicCharsEraseRequests, _vPeriodicCharsAddRequests, _vecPeriodicCharsElementBuffer);
            EXC_CATCHSUB("");
            ASSERT(sl::ContainerIsSorted(_vCharTicks));
            ASSERT(!sl::SortedContainerHasDuplicates(_vCharTicks));

#ifdef DEBUG_LIST_OPS
            // Ensure that the sortedVecRemoveAddQueued worked
            for (CChar* obj : _vPeriodicCharsEraseRequests)
            {
                auto itit = std::find_if(_vCharTicks.begin(), _vCharTicks.end(),
                    [obj](TickingPeriodicCharEntry const& lhs) constexpr noexcept {
                        return obj == lhs.second;
                    });
                UnreferencedParameter(itit);
                ASSERT(itit == _vCharTicks.end());
            }

            for (auto& obj : _vPeriodicCharsAddRequests)
            {
                auto itit = std::find_if(_vCharTicks.begin(), _vCharTicks.end(),
                    [obj](TickingPeriodicCharEntry const& lhs) constexpr noexcept {
                        return obj.second == lhs.second;
                    });
                UnreferencedParameter(itit);
                ASSERT(itit != _vCharTicks.end());
            }
#endif
            _vPeriodicCharsAddRequests.clear();
            _vPeriodicCharsEraseRequests.clear();
            _vecPeriodicCharsElementBuffer.clear();
        }

        if (!_vCharTicks.empty())
        {
            {
                EXC_TRYSUB("Selection");
#ifdef DEBUG_CCHAR_PERIODIC_TICKING_VERBOSE
                //g_Log.EventDebug("Start looping through char periodic ticks.\n");
#endif
                _vecIndexMiscBuffer.clear();
                CharTickList::iterator itMap       = _vCharTicks.begin();
                CharTickList::iterator itMapEnd    = _vCharTicks.end();

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
                    ASSERT(itMap->first != 0);
                    if (pChar->_CanTick() && !pChar->_IsBeingDeleted())
                    {
                        //if (pChar->_iTimePeriodicTick <= iCurTime)
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
                sortedVecRemoveElementsByIndices(_vCharTicks, _vecIndexMiscBuffer);
                EXC_CATCHSUB("DeleteFromList");

                _vecIndexMiscBuffer.clear();
            }

            // Done working with _vCharTicks, we don't need the lock from now on.
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
