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
#include <unordered_set>

/*
#if defined _DEBUG || defined _NIGHTLYBUILD
#   define DEBUG_CTIMEDOBJ_TIMED_TICKING
#   define DEBUG_CCHAR_PERIODIC_TICKING
#   define DEBUG_STATUSUPDATES
//#   define DEBUG_LIST_OPS
#endif
//#define TIMEDOBJS_COUNTER
//#define CHAR_PERIODIC_COUNTER
//#define STATUS_UPDATES_COUNTER
*/


template <typename TPair, typename T>
static void UnsortedVecDifference(
    std::vector<TPair> & vecMain,
    std::vector<TPair> & vecElemBuffer,
    std::vector<T*>    & vecToRemoveUnsorted)
{
    ASSERT(vecElemBuffer.empty());

    // Reserve space in vecElemBuffer to avoid reallocations
    vecElemBuffer.reserve(vecMain.size() - vecToRemoveUnsorted.size());

    /*
    // IMPLEMENTATION 1: linear, no bulk insertions
    // Iterate through vecMain, adding elements to vecElemBuffer only if they aren't in vecToRemoveUnsorted
    for (const auto& elem : vecMain) {
        if (std::find(vecToRemoveUnsorted.cbegin(), vecToRemoveUnsorted.cend(), elem.second) == vecToRemoveUnsorted.end()) {
            vecElemBuffer.push_back(elem);
        }
    }
    */

    if (vecToRemoveUnsorted.size() < 50 /*arbitrary number, this has to be benchmarked*/)
    {
        // IMPLEMENTATION 2: linear, with bulk insertions
        // We can do it because iterating through every vecToRemoveUnsorted element isn't so slow.

        // Use an iterator to store the position for bulk insertion
        auto itCopyFromThis = vecMain.cbegin();

        // Iterate through vecMain, copying elements that are not in vecToRemoveUnsorted
        for (auto itMain = vecMain.cbegin(); itMain != vecMain.cend(); ++itMain)
        {
            // Perform a linear search for the current element's pointer in vecToRemoveUnsorted
            auto itTemp = std::find(vecToRemoveUnsorted.cbegin(), vecToRemoveUnsorted.cend(), itMain->second);
            if (itTemp != vecToRemoveUnsorted.cend())
            {
                // If the element is found in vecToRemoveUnsorted, copy elements before it
                vecElemBuffer.insert(vecElemBuffer.cend(), itCopyFromThis, itMain); // Copy up to but not including itMain

                // Move itCopyFromThis forward to the next element
                itCopyFromThis = itMain + 1; // Move to the next element after itMain
            }
        }

        // Copy any remaining elements in vecMain after the last found element
        vecElemBuffer.insert(vecElemBuffer.cend(), itCopyFromThis, vecMain.cend());
    }
    else if (vecToRemoveUnsorted.size() < 5'000 /*arbitrary number, this has to be benchmarked*/)
    {
        // IMPLEMENTATION 3: sort vecToRemoveUnsorted and perform binary search
        std::sort(vecToRemoveUnsorted.begin(), vecToRemoveUnsorted.end());
        for (const auto& elem : vecMain)
        {
            if (!std::binary_search(vecToRemoveUnsorted.cbegin(), vecToRemoveUnsorted.cend(), elem.second))
                vecElemBuffer.push_back(elem);
        }
    }
    else
    {
        // IMPLEMENTATION 4: create an hash table for faster search
        static std::unordered_set<T*> removeSet;
        removeSet.clear();
        removeSet.reserve(vecToRemoveUnsorted.size());
        //removeSet.max_load_factor(0.75f);

        removeSet.insert(vecToRemoveUnsorted.cbegin(), vecToRemoveUnsorted.cend());
        std::copy_if(vecMain.cbegin(), vecMain.cend(), std::back_inserter(vecElemBuffer),
            [](const TPair& pair) constexpr {
                return removeSet.find(pair.second) == removeSet.end();
            });
    }

#ifdef DEBUG_LIST_OPS
    ASSERT(vecElemBuffer.size() == vecMain.size() - vecToRemoveUnsorted.size());
#endif

    vecMain.swap(vecElemBuffer);
}


template <typename TPair, typename T>
static void SortedVecRemoveAddQueued(
    std::vector<TPair> &vecMain, std::vector<TPair> &vecElemBuffer,
    std::vector<T>     &vecToRemoveUnsorted, std::vector<TPair> const& vecToAdd)
{
#ifdef DEBUG_LIST_OPS
    ASSERT(sl::ContainerIsSorted(vecMain));
    ASSERT(!sl::SortedContainerHasDuplicates(vecMain));

    ASSERT(sl::ContainerIsSorted(vecToAdd));
    ASSERT(!sl::SortedContainerHasDuplicates(vecToAdd));
#endif

    if (!vecToRemoveUnsorted.empty())
    {
        if (vecMain.empty()) {
            ASSERT(false);  // Shouldn't ever happen.
        }

        vecElemBuffer.clear();
        vecElemBuffer.reserve(vecMain.size() - vecToRemoveUnsorted.size());

        // Unsorted custom algorithm.
        // We can't use a classical sorted algorithm because vecMain is sorted by pair.first (int64 timeout)
        //  but vecToRemoveUnsorted is sorted by the pointer value!
        UnsortedVecDifference(vecMain, vecElemBuffer, vecToRemoveUnsorted);
        vecElemBuffer.clear();

#ifdef DEBUG_LIST_OPS
        for (auto& elem : vecToRemoveUnsorted)
        {
            auto it = std::find_if(vecMain.begin(), vecMain.end(), [elem](auto &rhs) constexpr noexcept {return elem == rhs.second;});
            UnreferencedParameter(it);
            ASSERT (it == vecMain.end());
        }

        ASSERT(sl::ContainerIsSorted(vecMain));
        ASSERT(!sl::SortedContainerHasDuplicates(vecMain));
#endif
    }

    if (!vecToAdd.empty())
    {
        vecElemBuffer.clear();
        vecElemBuffer.reserve(vecMain.size() + vecToAdd.size());

        // MergeSort
        std::merge(
            vecMain.cbegin(), vecMain.cend(),
            vecToAdd.cbegin(), vecToAdd.cend(),
            std::back_inserter(vecElemBuffer)
            );

#ifdef DEBUG_LIST_OPS
        ASSERT(vecElemBuffer.size() == vecMain.size() + vecToAdd.size());
#endif

        vecMain.swap(vecElemBuffer);
        vecElemBuffer.clear();

#ifdef DEBUG_LIST_OPS
        ASSERT(sl::ContainerIsSorted(vecMain));
        ASSERT(!sl::SortedContainerHasDuplicates(vecMain));
#endif

    }
}


// ----

CWorldTicker::CWorldTicker(CWorldClock *pClock) :
    _pWorldClock(nullptr),
    _iCurTickStartTime(0), _iLastTickDone(0)
{
    ASSERT(pClock);
    _pWorldClock = pClock;

    _vTimedObjsTimeouts.reserve(50);
    _vTimedObjsTimeoutsAddReq.reserve(50);
    _vTimedObjsTimeoutsEraseReq.reserve(50);
    _vTimedObjsTimeoutsBuffer.reserve(50);

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
        _vTimedObjsTimeoutsAddReq.cbegin(),
        _vTimedObjsTimeoutsAddReq.cend(),
        fnFindEntryByObj);
    if (_vTimedObjsTimeoutsAddReq.end() != itEntryInAddList)
        return *itEntryInAddList;

    const auto itEntryInTickList = std::find_if(
        _vTimedObjsTimeouts.cbegin(),
        _vTimedObjsTimeouts.cend(),
        fnFindEntryByObj);
    if (_vTimedObjsTimeouts.cend() != itEntryInTickList)
    {
        const auto itEntryInEraseList = std::find(
            _vTimedObjsTimeoutsEraseReq.cbegin(),
            _vTimedObjsTimeoutsEraseReq.cend(),
            pTimedObject);

        if (itEntryInEraseList == _vTimedObjsTimeoutsEraseReq.cend())
            return *itEntryInTickList;
    }

    return std::nullopt;
}

bool CWorldTicker::_InsertTimedObject(const int64 iTimeout, CTimedObject* pTimedObject)
{
    ASSERT(pTimedObject);
    ASSERT(iTimeout != 0);

#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    ASSERT(sl::ContainerIsSorted(_vTimedObjsTimeouts));
    ASSERT(!sl::SortedContainerHasDuplicates(_vTimedObjsTimeouts));
#endif

    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) constexpr noexcept {
        return pTimedObject == rhs.second;
    };
#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vTimedObjsTimeouts.MT_CMUTEX);
#endif

    if (pTimedObject->_fIsInWorldTickAddList)
    {
        const auto itEntryInAddList = std::find_if(
            _vTimedObjsTimeoutsAddReq.begin(),
            _vTimedObjsTimeoutsAddReq.end(),
            fnFindEntryByObj);
        ASSERT(_vTimedObjsTimeoutsAddReq.end() != itEntryInAddList);

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
        _vTimedObjsTimeouts.cbegin(),
        _vTimedObjsTimeouts.cend(),
        fnFindEntryByObj);
    if (_vTimedObjsTimeouts.cend() != itEntryInTickList)
    {
        // What's happening: I am requesting to add an element, but we already have it in the main ticking list (we are adding another one without deleting the old one?).

        const auto itEntryInEraseList = std::find(
            _vTimedObjsTimeoutsEraseReq.cbegin(),
            _vTimedObjsTimeoutsEraseReq.cend(),
            pTimedObject);
        if (_vTimedObjsTimeoutsEraseReq.cend() == itEntryInEraseList)
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
            _vTimedObjsTimeoutsEraseReq.begin(),
            _vTimedObjsTimeoutsEraseReq.end(),
            pTimedObject);
        ASSERT(_vTimedObjsTimeoutsEraseReq.end() == itEntryInEraseList);
    }
#endif

    _vTimedObjsTimeoutsAddReq.emplace_back(iTimeout, pTimedObject);

    ASSERT(pTimedObject->_fIsInWorldTickList == false);
    pTimedObject->_fIsInWorldTickAddList = true;

    return true;
}

bool CWorldTicker::_EraseTimedObject(CTimedObject* pTimedObject)
{
    ASSERT(pTimedObject);

#ifdef DEBUG_LIST_OPS
    ASSERT(sl::ContainerIsSorted(_vTimedObjsTimeouts));
    ASSERT(!sl::SortedContainerHasDuplicates(_vTimedObjsTimeouts));
#endif

    const auto fnFindEntryByObj = [pTimedObject](TickingTimedObjEntry const& rhs) constexpr noexcept {
        return pTimedObject == rhs.second;
    };

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_vTimedObjsTimeouts.MT_CMUTEX);
#endif

    if (pTimedObject->_fIsInWorldTickAddList)
    {
        // What's happening: it has the flag, so it must be in the add list. Ensure it, and remove it from there.
        //  We are reasonably sure that there isn't another entry in the main ticking list, because we would have gotten an error
        //  trying to append a request to the add list.
        const auto itEntryInAddList = std::find_if(
            _vTimedObjsTimeoutsAddReq.begin(),
            _vTimedObjsTimeoutsAddReq.end(),
            fnFindEntryByObj);
        ASSERT(itEntryInAddList != _vTimedObjsTimeoutsAddReq.end());

        _vTimedObjsTimeoutsAddReq.erase(itEntryInAddList);

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
        _vTimedObjsTimeoutsEraseReq.cbegin(),
        _vTimedObjsTimeoutsEraseReq.cend(),
        pTimedObject);
    if (_vTimedObjsTimeoutsEraseReq.cend() != itEntryInRemoveList)
    {

        const auto itEntryInAddList = std::find_if(
            _vTimedObjsTimeoutsAddReq.cbegin(),
            _vTimedObjsTimeoutsAddReq.cend(),
            fnFindEntryByObj);
        ASSERT(itEntryInAddList == _vTimedObjsTimeoutsAddReq.cend());

        const auto itEntryInTickList = std::find_if(
            _vTimedObjsTimeouts.cbegin(),
            _vTimedObjsTimeouts.cend(),
            fnFindEntryByObj);
        ASSERT(itEntryInTickList != _vTimedObjsTimeouts.cend());

        ASSERT(false);
        return false; // Already requested the removal.
    }
#endif

    // At this point, we should be fairly sure that the object is in the tick list.
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    // Redundant check.

    const auto itEntryInTickList = std::find_if(
        _vTimedObjsTimeouts.cbegin(),
        _vTimedObjsTimeouts.cend(),
        fnFindEntryByObj);
    if (itEntryInTickList == _vTimedObjsTimeouts.end())
    {
        // Not found -> not legit.
        // It has to be acknowledged that the object might have a timeout while being in a non-tickable state (like at server startup), so it isn't in the list,
        //  nevertheless we shouldn't have blindly tried to remove it from the list.
        ASSERT(false);
        return false;
    }
#endif

    _vTimedObjsTimeoutsEraseReq.emplace_back(pTimedObject);

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
        _vPeriodicCharsAddRequests.cbegin(),
        _vPeriodicCharsAddRequests.cend(),
        fnFindEntryByObj);
    if (_vPeriodicCharsAddRequests.cend() != itEntryInAddList)
        return *itEntryInAddList;

    const auto itEntryInTickList = std::find_if(
        _vPeriodicCharsTicks.cbegin(),
        _vPeriodicCharsTicks.cend(),
        fnFindEntryByObj);
    if (_vPeriodicCharsTicks.cend() != itEntryInTickList)
    {
        const auto itEntryInEraseList = std::find(
            _vPeriodicCharsEraseRequests.cbegin(),
            _vPeriodicCharsEraseRequests.cend(),
            pChar);

        if (itEntryInEraseList == _vPeriodicCharsEraseRequests.cend())
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
        _vPeriodicCharsAddRequests.cbegin(),
        _vPeriodicCharsAddRequests.cend(),
        fnFindEntryByChar);
    if (_vPeriodicCharsAddRequests.cend() != itEntryInAddList)
    {
        // We already had requested the insertion of this element.
        // We could just update periodic tick time, but by design we will add new periodic ticks only once per periodic tick.
        // If it happens more than once, we are making a mistake somewhere.

        ASSERT(false);
        return false; // Not legit.
    }

    const auto itEntryInEraseList = std::find(
        _vPeriodicCharsEraseRequests.cbegin(),
        _vPeriodicCharsEraseRequests.cend(),
        pChar);
    if (_vPeriodicCharsEraseRequests.cend() != itEntryInEraseList)
    {
        // Adding another one after we requested to remove it?
        // We could manage this, but we do not have this scenario, it shouldn't happen and we don't need to.
        // If it happens, we are making a mistake somewhere.

        ASSERT(false);
        return false; // Not legit.
    }

    // Do not add duplicates.
    const auto itEntryInTickList = std::find_if(
        _vPeriodicCharsTicks.cbegin(),
        _vPeriodicCharsTicks.cend(),
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

    const auto fnFindEntryByChar = [pChar](TickingPeriodicCharEntry const& rhs) constexpr noexcept {
        return pChar == rhs.second;
    };
    const auto itEntryInAddList = std::find_if(
        _vPeriodicCharsAddRequests.cbegin(),
        _vPeriodicCharsAddRequests.cend(),
        fnFindEntryByChar);
    if (_vPeriodicCharsAddRequests.cend() != itEntryInAddList)
    {
        // On the same tick the char did its periodic tick, re-added itself to the list,
        //  then something asked for its removal? Like calling Delete or destroying the char.

        _vPeriodicCharsAddRequests.erase(itEntryInAddList);

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
        const auto itEntryInTickList = std::find_if(
            _vPeriodicCharsTicks.cbegin(),
            _vPeriodicCharsTicks.cend(),
            fnFindEntryByChar);
        const auto itEntryInRemoveList = std::find(
            _vPeriodicCharsEraseRequests.cbegin(),
            _vPeriodicCharsEraseRequests.cend(),
            pChar);

        if (itEntryInRemoveList == _vPeriodicCharsEraseRequests.cend()) {
            ASSERT(itEntryInTickList == _vPeriodicCharsTicks.cend());
        }
        else {
            ASSERT(itEntryInTickList != _vPeriodicCharsTicks.cend());
        }
#endif
        return true;
    }

#ifdef DEBUG_CCHAR_PERIODIC_TICKING
    // Ensure it's in the ticking list.
    const auto itEntryInTickList = std::find_if(
        _vPeriodicCharsTicks.cbegin(),
        _vPeriodicCharsTicks.cend(),
        fnFindEntryByChar);
    if (itEntryInTickList == _vPeriodicCharsTicks.cend())
    {
        // Requested TickingPeriodicChar removal from ticking list, but not found.
        // Shouldn't happen, so it's illegal.

        ASSERT(false);
        return false;   // Not legit.
    }

    const auto itEntryInRemoveList = std::find(
        _vPeriodicCharsEraseRequests.cbegin(),
        _vPeriodicCharsEraseRequests.cend(),
        pChar);
    if (_vPeriodicCharsEraseRequests.cend() != itEntryInRemoveList)
    {
        // We have already requested to remove this from the main ticking list.
        ASSERT(itEntryInAddList == _vPeriodicCharsAddRequests.cend());

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
        _vecObjStatusUpdatesAddRequests.cbegin(),
        _vecObjStatusUpdatesAddRequests.cend(),
        pObj);
    if (_vecObjStatusUpdatesAddRequests.end() != itEntryInAddList)
        return *itEntryInAddList;
    */

    const auto itEntryInTickList = std::find(
        _vObjStatusUpdates.cbegin(),
        _vObjStatusUpdates.cend(),
        pObj);
    if (_vObjStatusUpdates.cend() != itEntryInTickList)
    {
        const auto itEntryInEraseList = std::find(
            _vObjStatusUpdatesEraseRequests.cbegin(),
            _vObjStatusUpdatesEraseRequests.cend(),
            pObj);

        if (itEntryInEraseList == _vObjStatusUpdatesEraseRequests.cend())
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
            _vObjStatusUpdatesAddRequests.cbegin(),
            _vObjStatusUpdatesAddRequests.cend(),
            pObj);
        ASSERT(_vObjStatusUpdatesAddRequests.cend() != itEntryInAddList);

        // I am requesting to add an element, but this was already done in this tick (there's already a request in the buffer).
        // We don't want duplicates.

        return false; // Not legit.
    }

#ifdef DEBUG_STATUSUPDATES
    // Debug redundant checks.

    const auto itEntryInTickList = std::find(
        _vObjStatusUpdates.cbegin(),
        _vObjStatusUpdates.cend(),
        pObj);
    if (_vObjStatusUpdates.cend() != itEntryInTickList)
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
            _vObjStatusUpdatesEraseRequests.cbegin(),
            _vObjStatusUpdatesEraseRequests.cend(),
            pObj);
        ASSERT(_vObjStatusUpdatesEraseRequests.cend() == itEntryInEraseList);
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
            _vObjStatusUpdatesAddRequests.cbegin(),
            _vObjStatusUpdatesAddRequests.cend(),
            pObj);
        ASSERT(itEntryInAddList != _vObjStatusUpdatesAddRequests.cend());

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
        _vObjStatusUpdatesEraseRequests.cbegin(),
        _vObjStatusUpdatesEraseRequests.cend(),
        pObj);
    if (_vObjStatusUpdatesEraseRequests.cend() != itEntryInRemoveList)
    {
        const auto itEntryInAddList = std::find(
            _vObjStatusUpdatesAddRequests.cbegin(),
            _vObjStatusUpdatesAddRequests.cend(),
            pObj);
        ASSERT(itEntryInAddList == _vObjStatusUpdatesAddRequests.cend());

        const auto itEntryInTickList = std::find(
            _vObjStatusUpdates.cbegin(),
            _vObjStatusUpdates.cend(),
            pObj);
        ASSERT(itEntryInTickList != _vObjStatusUpdates.cend());

        ASSERT(false);
        return false; // Already requested the removal.
    }
#endif

    // At this point, we should be fairly sure that the object is in the tick list.
#ifdef DEBUG_CTIMEDOBJ_TIMED_TICKING
    const auto itEntryInTickList = std::find(
        _vObjStatusUpdates.cbegin(),
        _vObjStatusUpdates.cend(),
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

#ifdef STATUS_UPDATES_COUNTER
        if (!_vObjStatusUpdates.empty() || !_vObjStatusUpdatesEraseRequests.empty() || !_vObjStatusUpdatesAddRequests.empty())
            g_Log.EventDebug("CTimedObj ticking: StatusUpdates queued and going to tick %" PRIuSIZE_T ", to erase %" PRIuSIZE_T ", to add %" PRIuSIZE_T ".\n",
                _vObjStatusUpdates.size(), _vObjStatusUpdatesEraseRequests.size(), _vObjStatusUpdatesAddRequests.size());
#endif
        sl::UnsortedVecRemoveElementsByValues(_vObjStatusUpdates, _vObjStatusUpdatesEraseRequests);
        _vObjStatusUpdates.insert(_vObjStatusUpdates.end(), _vObjStatusUpdatesAddRequests.cbegin(), _vObjStatusUpdatesAddRequests.cend());
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
        std::unique_lock<std::shared_mutex> lock(_vTimedObjsTimeouts.MT_CMUTEX);
#endif
        // New requests done during the world loop.
        {
            EXC_TRYSUB("Update main list");

#ifdef TIMEDOBJS_COUNTER
            if (/*!_vTimedObjsTimeouts.empty() ||*/ !_vTimedObjsTimeoutsEraseReq.empty() || !_vTimedObjsTimeoutsAddReq.empty())
                g_Log.EventDebug("CTimedObj ticking: CTimedObject queued %" PRIuSIZE_T ", to erase %" PRIuSIZE_T ", to add %" PRIuSIZE_T ".\n",
                    _vTimedObjsTimeouts.size(), _vTimedObjsTimeoutsEraseReq.size(), _vTimedObjsTimeoutsAddReq.size());
#endif

            for (TickingTimedObjEntry& elem : _vTimedObjsTimeoutsAddReq)
            {
                ASSERT(elem.second->_fIsInWorldTickAddList == true);
                elem.second->_fIsInWorldTickAddList = false;
                elem.second->_fIsInWorldTickList = true;
            }

            _vTimedObjsTimeoutsElementBuffer.clear();
            std::sort(_vTimedObjsTimeoutsEraseReq.begin(), _vTimedObjsTimeoutsEraseReq.end());
            std::sort(_vTimedObjsTimeoutsAddReq.begin(), _vTimedObjsTimeoutsAddReq.end());
            SortedVecRemoveAddQueued(_vTimedObjsTimeouts, _vTimedObjsTimeoutsElementBuffer, _vTimedObjsTimeoutsEraseReq, _vTimedObjsTimeoutsAddReq);
            EXC_CATCHSUB("");
        }

        _vTimedObjsTimeoutsAddReq.clear();
        _vTimedObjsTimeoutsEraseReq.clear();
        _vTimedObjsTimeoutsElementBuffer.clear();

        // Need here a new, inner scope to get rid of EXC_TRYSUB variables
        _vTimedObjsTimeoutsBuffer.clear();
        if (_vTimedObjsTimeouts.empty())
            return;

        {
            EXC_TRYSUB("Selection");

            _vIndexMiscBuffer.clear();
            size_t uiProgressive = 0;

            for (auto it = _vTimedObjsTimeouts.begin(), itEnd = _vTimedObjsTimeouts.end();
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
                _vTimedObjsTimeoutsBuffer.emplace_back(pTimedObj);
                _vIndexMiscBuffer.emplace_back(uiProgressive);

            }
            EXC_CATCHSUB("");
        }

#ifdef DEBUG_LIST_OPS
        // _vTimedObjsTimeoutsBuffer is a vector of CTimedObjs, it is still sorteed by the int64 timeout, not by CTimedObj* value.
        ASSERT(!sl::UnortedContainerHasDuplicates(_vTimedObjsTimeoutsBuffer));
        //ASSERT(sl::ContainerIsSorted(_vIndexMiscBuffer));
#endif

#ifdef TIMEDOBJS_COUNTER
        if (!_vIndexMiscBuffer.empty())
            g_Log.EventDebug("CTimedObj ticking: CTimedobject going to tick and be removed from the ticking list %" PRIuSIZE_T ".\n",
                _vIndexMiscBuffer.size());
#endif
        // Erase in chunks from _vTimedObjsTimeouts (which is sorted), do the least amount of operations possible.
        sl::SortedVecRemoveElementsByIndices(_vTimedObjsTimeouts, _vIndexMiscBuffer);
        _vIndexMiscBuffer.clear();

#ifdef DEBUG_LIST_OPS
        // Ensure that the SortedVecRemoveElementsByIndices worked. No element of _vecGenericObjsToTick has to still be in _vTimedObjsTimeouts.
        for (CTimedObject* obj : _vTimedObjsTimeoutsBuffer)
        {
            auto itit = std::find_if(_vTimedObjsTimeouts.cbegin(), _vTimedObjsTimeouts.cend(),
                [obj](TickingTimedObjEntry const& lhs) constexpr noexcept {
                    return obj == lhs.second;
                });
            UnreferencedParameter(itit);
            ASSERT(itit == _vTimedObjsTimeouts.cend());
        }
#endif

    } // destroy mutex
    // Done working with _vTimedObjsTimeouts, we don't need the lock from now on.

    lpctstr ptcSubDesc;
    for (CTimedObject* pTimedObj : _vTimedObjsTimeoutsBuffer)    // Loop through all msecs stored, unless we passed the timestamp.
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

    _vTimedObjsTimeoutsBuffer.clear();
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

#ifdef CHAR_PERIODIC_COUNTER
            if (/*!_vPeriodicCharsTicks.empty() ||*/ !_vPeriodicCharsEraseRequests.empty() || !_vPeriodicCharsAddRequests.empty())
                g_Log.EventDebug("CTimedObj ticking: Periodic Chars queued %" PRIuSIZE_T ", to erase %" PRIuSIZE_T ", to add %" PRIuSIZE_T ".\n",
                    _vPeriodicCharsTicks.size(), _vPeriodicCharsEraseRequests.size(), _vPeriodicCharsAddRequests.size());
#endif

            _vPeriodicCharsElementBuffer.clear();
            std::sort(_vPeriodicCharsEraseRequests.begin(), _vPeriodicCharsEraseRequests.end());
            std::sort(_vPeriodicCharsAddRequests.begin(), _vPeriodicCharsAddRequests.end());
            SortedVecRemoveAddQueued(_vPeriodicCharsTicks, _vPeriodicCharsElementBuffer, _vPeriodicCharsEraseRequests, _vPeriodicCharsAddRequests);

            ASSERT(sl::ContainerIsSorted(_vPeriodicCharsTicks));
            ASSERT(!sl::SortedContainerHasDuplicates(_vPeriodicCharsTicks));

            EXC_CATCHSUB("");

#ifdef DEBUG_LIST_OPS
            // Ensure that the SortedVecRemoveAddQueued worked
            for (CChar* obj : _vPeriodicCharsEraseRequests)
            {
                auto itit = std::find_if(_vPeriodicCharsTicks.cbegin(), _vPeriodicCharsTicks.cend(),
                    [obj](TickingPeriodicCharEntry const& lhs) constexpr noexcept {
                        return obj == lhs.second;
                    });
                UnreferencedParameter(itit);
                ASSERT(itit == _vPeriodicCharsTicks.cend());
            }

            for (auto& obj : _vPeriodicCharsAddRequests)
            {
                auto itit = std::find_if(_vPeriodicCharsTicks.cbegin(), _vPeriodicCharsTicks.cend(),
                    [obj](TickingPeriodicCharEntry const& lhs) constexpr noexcept {
                        return obj.second == lhs.second;
                    });
                UnreferencedParameter(itit);
                ASSERT(itit != _vPeriodicCharsTicks.cend());
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

#ifdef CHAR_PERIODIC_COUNTER
            if (!_vIndexMiscBuffer.empty())
                g_Log.EventDebug("CTimedObj ticking: Periodic Chars going to tick and be removed from the ticking list %" PRIuSIZE_T ".\n",
                    _vIndexMiscBuffer.size());
#endif
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
                if (!pChar->IsPeriodicTickPending())
                {
                    // Need to check this, people could do funny stuff in @Death trigger and force-add the Char back to
                    //  the ticking list from OnTickPeriodic. It's the only exceptional case, because it normally is added back here.
                    AddCharTicking(pChar, false);
                }
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
