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

#if defined _DEBUG || defined _NIGHTLYBUILD
#   define DEBUG_CTIMEDOBJ_TIMED_TICKING
#   define DEBUG_CCHAR_PERIODIC_TICKING
#   define DEBUG_STATUSUPDATES
#   define DEBUG_LIST_OPS
#endif

CWorldTicker::CWorldTicker(CWorldClock *pClock) :
    _pWorldClock(nullptr),
    _iCurTickStartTime(0), _iLastTickDone(0)
{
    ASSERT(pClock);
    _pWorldClock = pClock;

    _vWorldObjsTicks.reserve(50);
    _vWorldObjsAddRequests.reserve(50);
    _vWorldObjsEraseRequests.reserve(50);
    _vWorldObjsTicksBuffer.reserve(50);

    _vPeriodicCharsTicks.reserve(50);
    _vPeriodicCharsAddRequests.reserve(50);
    _vPeriodicCharsEraseRequests.reserve(50);
    _vPeriodicCharsTicksBuffer.reserve(50);

    _vObjStatusUpdates.reserve(25);
    _vObjStatusUpdatesAddRequests.reserve(25);
    _vObjStatusUpdatesEraseRequests.reserve(25);
    _vObjStatusUpdatesTickBuffer.reserve(25);
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
        _vWorldObjsTicks.begin(),
        _vWorldObjsTicks.end(),
        fnFindEntryByObj);
    if (_vWorldObjsTicks.end() != itEntryInTickList)
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
    ASSERT(sl::ContainerIsSorted(_vWorldObjsTicks));
    ASSERT(!sl::SortedContainerHasDuplicates(_vWorldObjsTicks));
#endif

    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) constexpr noexcept {
        return pTimedObject == rhs.second;
    };
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vWorldObjsTicks.MT_CMUTEX);
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
        _vWorldObjsTicks.begin(),
        _vWorldObjsTicks.end(),
        fnFindEntryByObj);
    if (_vWorldObjsTicks.end() != itEntryInTickList)
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

bool CWorldTicker::_EraseTimedObject(CTimedObject* pTimedObject)
{
    ASSERT(pTimedObject);

#ifdef DEBUG_LIST_OPS
    ASSERT(sl::ContainerIsSorted(_vWorldObjsTicks));
    ASSERT(!sl::SortedContainerHasDuplicates(_vWorldObjsTicks));
#endif

    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) constexpr noexcept {
        return pTimedObject == rhs.second;
    };

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vWorldObjsTicks.MT_CMUTEX);
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
            _vWorldObjsTicks.begin(),
            _vWorldObjsTicks.end(),
            fnFindEntryByObj);
        ASSERT(itEntryInTickList != _vWorldObjsTicks.end());

        ASSERT(false);
        return false; // Already requested the removal.
    }
#endif

    // At this point, we should be fairly sure that the object is in the tick list.
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    // Redundant check.

    const auto itEntryInTickList = std::find_if(
        _vWorldObjsTicks.begin(),
        _vWorldObjsTicks.end(),
        fnFindEntryByObj);
    if (itEntryInTickList == _vWorldObjsTicks.end())
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

        const bool fRemoved = _EraseTimedObject(pTimedObject);
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

    return fCanTick;

    EXC_CATCH;
    return false;
}

bool CWorldTicker::DelTimedObject(CTimedObject* pTimedObject)
{
    EXC_TRY("DelTimedObject");
    const ProfileTask timersTask(PROFILE_TIMERS);

    EXC_SET_BLOCK("Remove");
    const bool fRet = _EraseTimedObject(pTimedObject);
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
        _vPeriodicCharsTicks.begin(),
        _vPeriodicCharsTicks.end(),
        fnFindEntryByObj);
    if (_vPeriodicCharsTicks.end() != itEntryInTickList)
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
    std::unique_lock<std::shared_mutex> lock(_vPeriodicCharsTicks.MT_CMUTEX);
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
        _vPeriodicCharsTicks.begin(),
        _vPeriodicCharsTicks.end(),
        fnFindEntryByChar);
    ASSERT(_vPeriodicCharsTicks.end() == itEntryInTickList);
#endif

    _vPeriodicCharsAddRequests.emplace_back(iTickNext, pChar);

    return true;
}

bool CWorldTicker::_EraseCharTicking(CChar* pChar)
{
    // I'm reasonably sure that the element i'm trying to remove is present in this container.
    ASSERT(pChar);

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vPeriodicCharsTicks.MT_CMUTEX);
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
            _vPeriodicCharsTicks.begin(),
            _vPeriodicCharsTicks.end(),
            fnFindEntryByChar);

        if (itEntryInRemoveList == _vPeriodicCharsEraseRequests.end()) {
            ASSERT(itEntryInTickList == _vPeriodicCharsTicks.end());
        }
        else {
            ASSERT(itEntryInTickList != _vPeriodicCharsTicks.end());
        }
#endif
        return true
*/;
    }
#endif

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
    // Ensure it's in the ticking list.
    const auto itEntryInTickList = std::find_if(
        _vPeriodicCharsTicks.begin(),
        _vPeriodicCharsTicks.end(),
        fnFindEntryByChar);
    if (itEntryInTickList == _vPeriodicCharsTicks.end())
    {
        // Requested TickingPeriodicChar removal from ticking list, but not found.
        // Shouldn't happen, so it's illegal.

        ASSERT(false);
        return false;   // Not legit.
    }

    const auto itEntryInRemoveList = std::find(
        _vPeriodicCharsEraseRequests.begin(),
        _vPeriodicCharsEraseRequests.end(),
        pChar);
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
        const bool fRemoved = _EraseCharTicking(pChar);
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
        auto fnFindEntryByChar = [pChar](const TickingPeriodicCharEntry& entry) noexcept {
                return entry.second == pChar;
        };

        const auto itEntryInEraseList = std::find(
            _vPeriodicCharsEraseRequests.begin(),
            _vPeriodicCharsEraseRequests.end(),
            pChar);
        if (itEntryInEraseList != _vPeriodicCharsEraseRequests.end())
        {
            ASSERT(false);
            return false;
        }

        const auto itEntryInTickList = std::find_if(
            _vPeriodicCharsTicks.begin(),
            _vPeriodicCharsTicks.end(),
            fnFindEntryByChar);
        if (itEntryInTickList != _vPeriodicCharsTicks.end())
        {
            ASSERT(false);
            return false;
        }
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
    const bool fRet = _EraseCharTicking(pChar);
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
    std::unique_lock<std::shared_mutex> lock(_vObjStatusUpdates.MT_CMUTEX);
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


void CWorldTicker::ProcessServerTickActions()
{
    EXC_TRY("CWorldTicker::ProcessServerTickActions");

    // Do this once per tick.
    //  Update status flags from objects, update current tick.
    if (_iLastTickDone > _pWorldClock->GetCurrentTick())
        return;

    ++_iLastTickDone;   // Update current tick.

    ProcessObjStatusUpdates();

    EXC_CATCH;
}

void CWorldTicker::ProcessObjStatusUpdates()
{
    EXC_TRY("CWorldTicker::ProcessObjStatusUpdates");

    /* process objects that need status updates
        * these objects will normally be in containers which don't have any period _OnTick method
        * called (whereas other items can receive the OnTickStatusUpdate() call via their normal
        * tick method).
        * note: ideally, a better solution to accomplish this should be found if possible
        * TODO: implement a new class inheriting from CTimedObject to get rid of this code?
        */
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock_su(_vObjStatusUpdates.MT_CMUTEX);
#endif
        EXC_SET_BLOCK("Remove requested");

        for (CObjBase* elem : _vObjStatusUpdatesAddRequests)
        {
            elem->_fIsInStatusUpdatesAddList = false;
            elem->_fIsInStatusUpdatesList = true;
        }

        sl::UnsortedVecRemoveElementsByValues(_vObjStatusUpdates, _vObjStatusUpdatesEraseRequests);
        _vObjStatusUpdates.insert(_vObjStatusUpdates.end(), _vObjStatusUpdatesAddRequests.begin(), _vObjStatusUpdatesAddRequests.end());
        _vObjStatusUpdatesAddRequests.clear();
        _vObjStatusUpdatesEraseRequests.clear();

        EXC_SET_BLOCK("Selection");
        _vObjStatusUpdatesTickBuffer.clear();
        if (_vObjStatusUpdates.empty())
            return;

        for (CObjBase* pObj : _vObjStatusUpdates)
        {
            if (!pObj)
                continue;

            pObj->_fIsInStatusUpdatesList = false;
            if (pObj->_IsBeingDeleted())
                continue;

            _vObjStatusUpdatesTickBuffer.emplace_back(pObj);
        }

    } // destroy mutex

    EXC_SET_BLOCK("Loop");
    for (CObjBase* pObj : _vObjStatusUpdatesTickBuffer)
    {
        pObj->OnTickStatusUpdate();
    }

    EXC_CATCH;

    _vObjStatusUpdates.clear();
    _vObjStatusUpdatesTickBuffer.clear();
}

void CWorldTicker::ProcessTimedObjects()
{
    EXC_TRY("CWorldTicker::ProcessTimedObjects");

    // Need this new scope to give the right lifetime to ProfileTask.
    const ProfileTask timersTask(PROFILE_TIMERS);
    {
        // Need here another scope to give the right lifetime to the unique_lock.
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(_vWorldObjsTicks.MT_CMUTEX);
#endif
        // New requests done during the world loop.
        {
            EXC_TRYSUB("Update main list");

            for (TickingTimedObjEntry& elem : _vWorldObjsAddRequests)
            {
                ASSERT(elem.second->_fIsInWorldTickAddList == true);
                elem.second->_fIsInWorldTickAddList = false;
                elem.second->_fIsInWorldTickList = true;
            }

            _vWorldObjsElementBuffer.clear();
            std::sort(_vWorldObjsEraseRequests.begin(), _vWorldObjsEraseRequests.end());
            std::sort(_vWorldObjsAddRequests.begin(), _vWorldObjsAddRequests.end());
            sl::SortedVecRemoveAddQueued(_vWorldObjsTicks, _vWorldObjsElementBuffer, _vWorldObjsEraseRequests, _vWorldObjsAddRequests);
            EXC_CATCHSUB("");
        }

        _vWorldObjsAddRequests.clear();
        _vWorldObjsEraseRequests.clear();
        _vWorldObjsElementBuffer.clear();

        // Need here a new, inner scope to get rid of EXC_TRYSUB variables
        _vWorldObjsTicksBuffer.clear();
        if (_vWorldObjsTicks.empty())
            return;

        {
            EXC_TRYSUB("Selection");

            _vIndexMiscBuffer.clear();
            size_t uiProgressive = 0;

            for (auto it = _vWorldObjsTicks.begin(), itEnd = _vWorldObjsTicks.end();
                (it != itEnd) && (_iCurTickStartTime > it->first);
                ++it, ++uiProgressive)
            {
                CTimedObject* pTimedObj = it->second;
                if (!pTimedObj->_IsTimerSet() || !pTimedObj->_CanTick())
                    continue;

                //if (pTimedObj->_GetTimeoutRaw() > _iCurTickStartTime)
                //    continue;

                if (auto pObjBase = dynamic_cast<const CObjBase*>(pTimedObj))
                {
                    if (pObjBase->_IsBeingDeleted())
                        continue;
                }

                // Object should tick.
                pTimedObj->_fIsInWorldTickList = false;
                _vWorldObjsTicksBuffer.emplace_back(pTimedObj);
                _vIndexMiscBuffer.emplace_back(uiProgressive);

            }
            EXC_CATCHSUB("");
        }

#ifdef DEBUG_LIST_OPS
        // _vWorldObjsTicksBuffer is a vector of CTimedObjs, it is still sorteed by the int64 timeout, not by CTimedObj* value.
        ASSERT(!sl::UnortedContainerHasDuplicates(_vWorldObjsTicksBuffer));
        //ASSERT(sl::ContainerIsSorted(_vIndexMiscBuffer));
#endif

        // Erase in chunks from _vWorldObjsTicks (which is sorted), do the least amount of operations possible.
        sl::SortedVecRemoveElementsByIndices(_vWorldObjsTicks, _vIndexMiscBuffer);
        _vIndexMiscBuffer.clear();

#ifdef DEBUG_LIST_OPS
        // Ensure that the SortedVecRemoveElementsByIndices worked. No element of _vecGenericObjsToTick has to still be in _vWorldObjsTicks.
        for (CTimedObject* obj : _vWorldObjsTicksBuffer)
        {
            auto itit = std::find_if(_vWorldObjsTicks.begin(), _vWorldObjsTicks.end(),
                [obj](TickingTimedObjEntry const& lhs) constexpr noexcept {
                    return obj == lhs.second;
                });
            UnreferencedParameter(itit);
            ASSERT(itit == _vWorldObjsTicks.end());
        }
#endif

    } // destroy mutex
    // Done working with _vWorldObjsTicks, we don't need the lock from now on.

    lpctstr ptcSubDesc;
    for (CTimedObject* pTimedObj : _vWorldObjsTicksBuffer)    // Loop through all msecs stored, unless we passed the timestamp.
    {
        ptcSubDesc = "Generic";

        EXC_TRYSUB("Tick");
        EXC_SETSUB_BLOCK("Elapsed");

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

    EXC_CATCH;

    _vWorldObjsTicksBuffer.clear();
}

void CWorldTicker::ProcessCharPeriodicTicks()
{
    EXC_TRY("CWorldTicker::ProcessCharPeriodicTicks");

    const ProfileTask taskChars(PROFILE_CHARS);
    {
        // Need here another scope to give the right lifetime to the unique_lock.
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(_vPeriodicCharsTicks.MT_CMUTEX);
#endif
        {
            // New requests done during the world loop.
            EXC_TRYSUB("Update main list");

            _vPeriodicCharsElementBuffer.clear();
            std::sort(_vPeriodicCharsEraseRequests.begin(), _vPeriodicCharsEraseRequests.end());
            std::sort(_vPeriodicCharsAddRequests.begin(), _vPeriodicCharsAddRequests.end());
            sl::SortedVecRemoveAddQueued(_vPeriodicCharsTicks, _vPeriodicCharsElementBuffer, _vPeriodicCharsEraseRequests, _vPeriodicCharsAddRequests);

            ASSERT(sl::ContainerIsSorted(_vPeriodicCharsTicks));
            ASSERT(!sl::SortedContainerHasDuplicates(_vPeriodicCharsTicks));

            EXC_CATCHSUB("");

#ifdef DEBUG_LIST_OPS
            // Ensure that the SortedVecRemoveAddQueued worked
            for (CChar* obj : _vPeriodicCharsEraseRequests)
            {
                auto itit = std::find_if(_vPeriodicCharsTicks.begin(), _vPeriodicCharsTicks.end(),
                    [obj](TickingPeriodicCharEntry const& lhs) constexpr noexcept {
                        return obj == lhs.second;
                    });
                UnreferencedParameter(itit);
                ASSERT(itit == _vPeriodicCharsTicks.end());
            }

            for (auto& obj : _vPeriodicCharsAddRequests)
            {
                auto itit = std::find_if(_vPeriodicCharsTicks.begin(), _vPeriodicCharsTicks.end(),
                    [obj](TickingPeriodicCharEntry const& lhs) constexpr noexcept {
                        return obj.second == lhs.second;
                    });
                UnreferencedParameter(itit);
                ASSERT(itit != _vPeriodicCharsTicks.end());
            }
#endif
            _vPeriodicCharsAddRequests.clear();
            _vPeriodicCharsEraseRequests.clear();
            _vPeriodicCharsElementBuffer.clear();
        }

        _vPeriodicCharsTicksBuffer.clear();
        if (_vPeriodicCharsTicks.empty())
            return;

        {
            EXC_TRYSUB("Selection");

            _vIndexMiscBuffer.clear();
            size_t uiProgressive = 0;

            for (auto it = _vPeriodicCharsTicks.begin(), itEnd = _vPeriodicCharsTicks.end();
                (it != itEnd) && (_iCurTickStartTime > it->first);
                ++it, ++uiProgressive)
            {
                ASSERT(it->first != 0);
                CChar* pChar = it->second;
                if (!pChar->_CanTick() || pChar->_IsBeingDeleted())
                    continue;

                _vPeriodicCharsTicksBuffer.emplace_back(pChar);
                _vIndexMiscBuffer.emplace_back(uiProgressive);
            }

#ifdef DEBUG_LIST_OPS
            // _vPeriodicCharsTicksBuffer is a vector of CTimedObjs, it is still sorteed by the int64 timeout, not by CTimedObj* value.
            ASSERT(!sl::UnortedContainerHasDuplicates(_vPeriodicCharsTicksBuffer));
            //ASSERT(sl::ContainerIsSorted(_vIndexMiscBuffer));
#endif
            EXC_CATCHSUB("");
        }

        {
            EXC_TRYSUB("Delete from List");
            // Erase in chunks from _vPeriodicCharsTicks (which is sorted), do the least amount of operations possible.
            sl::SortedVecRemoveElementsByIndices(_vPeriodicCharsTicks, _vIndexMiscBuffer);
            EXC_CATCHSUB("DeleteFromList");

            _vIndexMiscBuffer.clear();
        }

        // Done working with _vPeriodicCharsTicks, we don't need the lock from now on.
    }

    {
        EXC_TRYSUB("Char Periodic Ticks Loop");
        for (CChar* pChar : _vPeriodicCharsTicksBuffer)    // Loop through all msecs stored, unless we passed the timestamp.
        {
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
    }

    EXC_CATCH;

    _vPeriodicCharsTicksBuffer.clear();
}


// Check timeouts and do ticks
void CWorldTicker::Tick()
{
    ADDTOCALLSTACK("CWorldTicker::Tick");
    EXC_TRY("CWorldTicker::Tick");

    ProcessServerTickActions();

    _iCurTickStartTime = CWorldGameTime::GetCurrentTime().GetTimeRaw();    // Current timestamp, a few msecs will advance in the current tick ... avoid them until the following tick(s).

    // Items, Chars ... Everything relying on CTimedObject.
    ProcessTimedObjects();

    // Periodic, automatic ticking specific for CChar instances.
    ProcessCharPeriodicTicks();

    EXC_CATCH;
}
