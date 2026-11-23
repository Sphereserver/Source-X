#include "../common/sphere_library/scontainer_ops.h"
//#include "../common/CException.h" // included in the precompiled header
#include "../sphere/threads.h"
#include "../sphere/ProfileTask.h"
#include "chars/CChar.h"
#include "items/CItem.h"
#include "CSector.h"
#include "CServerConfig.h"
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



// ----

CWorldTicker::CWorldTicker(CWorldClock *pClock) :
    _pWorldClock(nullptr),
    _iCurTickStartTime(0), _iLastTickDone(0)
{
    // The objects store the handles as plain integers, to avoid including the wheel everywhere.
    static_assert(std::is_same_v<decltype(CTimedObject::_hWorldTickWheel), CTickWheelCore::Handle>);
    static_assert(std::is_same_v<decltype(CChar::_hPeriodicTickWheel), CTickWheelCore::Handle>);

    ASSERT(pClock);
    _pWorldClock = pClock;

    _vTimedObjsTimeoutsBuffer.reserve(50);
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
#if MT_ENGINES
    std::shared_lock<std::shared_mutex> lock(_wheelTimedObjs.MT_CMUTEX);
#endif
    if (pTimedObject->_hWorldTickWheel == CTickWheelCore::kNone)
        return std::nullopt;

    return std::make_pair(_wheelTimedObjs.GetTime(pTimedObject->_hWorldTickWheel, pTimedObject), const_cast<CTimedObject*>(pTimedObject));
}

bool CWorldTicker::_InsertTimedObject(const int64 iTimeout, CTimedObject* pTimedObject)
{
    ASSERT(pTimedObject);
    ASSERT(iTimeout != 0);

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_wheelTimedObjs.MT_CMUTEX);
#endif

    if (pTimedObject->_hWorldTickWheel != CTickWheelCore::kNone)
    {
        // Already registered: move it to the new time.
        if (_wheelTimedObjs.Reschedule(pTimedObject->_hWorldTickWheel, pTimedObject, iTimeout))
            return true;
        // Not expected: the handle isn't of this object anymore. Registered again, else it would never tick.
        DEBUG_ERR(("CWorldTicker: stale timer handle of a timed object, registered again.\n"));
    }

    pTimedObject->_hWorldTickWheel = _wheelTimedObjs.Insert(pTimedObject, iTimeout);
    return true;
}

bool CWorldTicker::_EraseTimedObject(CTimedObject* pTimedObject)
{
    ASSERT(pTimedObject);

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_wheelTimedObjs.MT_CMUTEX);
#endif

    if (pTimedObject->_hWorldTickWheel == CTickWheelCore::kNone)
    {
        // It isn't in the ticking list.
        ASSERT(false);  // Not legit.
        return false;
    }

    const bool fRet = _wheelTimedObjs.Erase(pTimedObject->_hWorldTickWheel, pTimedObject);
    pTimedObject->_hWorldTickWheel = CTickWheelCore::kNone;
    return fRet;
}

bool CWorldTicker::AddTimedObject(const int64 iTimeout, CTimedObject* pTimedObject, bool fForce)
{
    //if (iTimeout < CWorldGameTime::GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible; don't uncomment.
    //    return;

    UnreferencedParameter(fForce);

    EXC_TRY("AddTimedObject");
    ASSERT(pTimedObject);
    const ProfileTask timersTask(PROFILE_TIMERS);

    // If it's already registered (a new timeout set without deleting the previous one), it's rescheduled.
    EXC_SET_BLOCK("Insert");
    const bool fRet = _InsertTimedObject(iTimeout, pTimedObject);
    ASSERT(fRet);

    return fRet;

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
#if MT_ENGINES
    std::shared_lock<std::shared_mutex> lock(_wheelPeriodicChars.MT_CMUTEX);
#endif
    if (!pChar->IsPeriodicTickPending())
        return std::nullopt;

    return std::make_pair(_wheelPeriodicChars.GetTime(pChar->_hPeriodicTickWheel, pChar), const_cast<CChar*>(pChar));
}

bool CWorldTicker::_InsertCharTicking(const int64 iTickNext, CChar* pChar)
{
    // The caller sets pChar->_iTimePeriodicTick.
    ASSERT(iTickNext != 0);
    ASSERT(pChar);

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_wheelPeriodicChars.MT_CMUTEX);
#endif

    if (pChar->_hPeriodicTickWheel != CTickWheelCore::kNone)
    {
        // Already registered: move it to the new time.
        if (_wheelPeriodicChars.Reschedule(pChar->_hPeriodicTickWheel, pChar, iTickNext))
            return true;
        // Not expected: the handle isn't of this char anymore. Registered again, else it would never tick.
        DEBUG_ERR(("CWorldTicker: stale periodic tick handle of a char, registered again.\n"));
    }

    pChar->_hPeriodicTickWheel = _wheelPeriodicChars.Insert(pChar, iTickNext);
    return true;
}

bool CWorldTicker::_EraseCharTicking(CChar* pChar)
{
    // The caller checks that it's registered (IsPeriodicTickPending) and clears pChar->_iTimePeriodicTick.
    ASSERT(pChar);

#if MT_ENGINES
    std::unique_lock<std::shared_mutex> lock(_wheelPeriodicChars.MT_CMUTEX);
#endif

    const bool fRet = _wheelPeriodicChars.Erase(pChar->_hPeriodicTickWheel, pChar);
    pChar->_hPeriodicTickWheel = CTickWheelCore::kNone;
    return fRet;
}

bool CWorldTicker::AddCharTicking(CChar* pChar, bool fNeedsLock)
{
    // Add CChar to the periodic ticking list.
    EXC_TRY("AddCharTicking");

    if (pChar->_IsBeingDestructed())
    {
        // Its destructor already removed it, and the wheel would keep a dangling pointer: the destructor runs triggers
        //  after that (as @CombatDelete, with the dying char as SRC), and a script setting its STR to 0 would add it.
        return false;
    }

    const ProfileTask timersTask(PROFILE_TIMERS);

    int64 iTickNext, iTickOld;
    UnreferencedParameter(fNeedsLock);
#if MT_ENGINES
    if (fNeedsLock)
    {
        std::unique_lock<std::shared_mutex> lock(pChar->MT_CMUTEX);
        iTickNext = pChar->_iTimeNextRegen;
        iTickOld = pChar->_iTimePeriodicTick;
    }
    else
#endif
    {
        iTickNext = pChar->_iTimeNextRegen;
        iTickOld = pChar->_iTimePeriodicTick;
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

    // If it's already registered, it's rescheduled.
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

    bool fTickPending;
#if MT_ENGINES
    if (fNeedsLock)
    {
        std::unique_lock<std::shared_mutex> lock(pChar->MT_CMUTEX);
        fTickPending = pChar->IsPeriodicTickPending();
    }
    else
#endif
    {
        fTickPending = pChar->IsPeriodicTickPending();
    }

    if (!fTickPending)
    {
        // Not registered, nothing to remove.
        return true;
    }

    EXC_SET_BLOCK("Remove");
    // Not registered afterwards, even if the erase failed (not expected: the handle wasn't of this char anymore).
    //  Else the char would look registered, and never be registered again.
    const bool fRet = _EraseCharTicking(pChar);
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

    return fRet;

    EXC_CATCH;
    return false;
}

// Objects destroyed while waiting for their turn in the current pass.
// Rare: the tick of an object processed before saved the world or collected the garbage, destroying the objects deleted
//  until then. A linear search is fine. The entry is nulled, not removed, because the pass is iterating the buffer.

void CWorldTicker::ForgetCollectedTimedObject(const CTimedObject* pTimedObject) noexcept
{
    const auto it = std::find(_vTimedObjsTimeoutsBuffer.begin(), _vTimedObjsTimeoutsBuffer.end(), pTimedObject);
    if (it != _vTimedObjsTimeoutsBuffer.end())
        *it = nullptr;
}

void CWorldTicker::ForgetCollectedChar(const CChar* pChar) noexcept
{
    const auto it = std::find(_vPeriodicCharsTicksBuffer.begin(), _vPeriodicCharsTicksBuffer.end(), pChar);
    if (it != _vPeriodicCharsTicksBuffer.end())
        *it = nullptr;
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
        std::unique_lock<std::shared_mutex> lock(_wheelTimedObjs.MT_CMUTEX);
#endif
        _vTimedObjsTimeoutsBuffer.clear();
        EXC_TRYSUB("Selection");
        _wheelTimedObjs.CollectExpired(_iCurTickStartTime, _vTimedObjsTimeoutsBuffer);
        // Their entries were removed from the wheel: the handles aren't valid anymore.
        // The flag lets the destructor remove the object from the buffer, if something destroys it before its turn.
        for (CTimedObject* pTimedObj : _vTimedObjsTimeoutsBuffer)
        {
            pTimedObj->_hWorldTickWheel = CTickWheelCore::kNone;
            pTimedObj->_uiTickFlags |= CTimedObject::kTickFlagTimeoutCollected;
        }
        EXC_CATCHSUB("");

#ifdef TIMEDOBJS_COUNTER
        if (!_vTimedObjsTimeoutsBuffer.empty())
            g_Log.EventDebug("CTimedObj ticking: CTimedObject queued %" PRIuSIZE_T ", going to tick %" PRIuSIZE_T ".\n",
                _wheelTimedObjs.size(), _vTimedObjsTimeoutsBuffer.size());
#endif

#ifdef DEBUG_LIST_OPS
        // _vTimedObjsTimeoutsBuffer is a vector of CTimedObjs, it is still sorted by the int64 timeout, not by CTimedObj* value.
        ASSERT(!sl::UnortedContainerHasDuplicates(_vTimedObjsTimeoutsBuffer));
#endif

    } // destroy mutex
    // Done working with _wheelTimedObjs, we don't need the lock from now on.

    lpctstr ptcSubDesc;
    for (CTimedObject* pTimedObj : _vTimedObjsTimeoutsBuffer)    // Loop through all msecs stored, unless we passed the timestamp.
    {
        if (pTimedObj == nullptr)
            continue;   // Destroyed by the tick of an object processed before (ForgetCollectedTimedObject).
        pTimedObj->_uiTickFlags &= (uint8)~CTimedObject::kTickFlagTimeoutCollected;

        // Checked here and not while collecting, so that the wheel doesn't need to read the objects, and the state is the
        //  current one (the tick of a previous object may have changed it).
        // Registered again: the tick of an object processed before this one, in this same loop, set a new timer on this
        //  object (or woke it up). The new timer replaced the expired one, which doesn't fire, as if each timer fired at its
        //  own time; the new one fires when it expires.
        if (pTimedObj->_IsTimeoutTickingActive())
            continue;
        // Their entry was removed: a sleeping object is registered again by _GoAwake, if its timer is still set;
        //  an object being deleted doesn't need it anymore.
        if (!pTimedObj->_IsTimerSet() || pTimedObj->_IsSleeping())
            continue;
        if (auto pObjBase = dynamic_cast<const CObjBase*>(pTimedObj); pObjBase && pObjBase->_IsBeingDeleted())
            continue;

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
        std::unique_lock<std::shared_mutex> lock(_wheelPeriodicChars.MT_CMUTEX);
#endif
        _vPeriodicCharsTicksBuffer.clear();
        EXC_TRYSUB("Selection");
        _wheelPeriodicChars.CollectExpired(_iCurTickStartTime, _vPeriodicCharsTicksBuffer);
        // Their entries were removed from the wheel: the handles aren't valid anymore.
        // The flag lets the destructor remove the char from the buffer, if something destroys it before its turn.
        for (CChar* pChar : _vPeriodicCharsTicksBuffer)
        {
            pChar->_iTimePeriodicTick = 0;
            pChar->_hPeriodicTickWheel = CTickWheelCore::kNone;
            pChar->_uiTickFlags |= CTimedObject::kTickFlagPeriodicCollected;
        }
        EXC_CATCHSUB("");

        // Done working with _wheelPeriodicChars, we don't need the lock from now on.
    }

    {
        EXC_TRYSUB("Char Periodic Ticks Loop");
        for (CChar* pChar : _vPeriodicCharsTicksBuffer)    // Loop through all msecs stored, unless we passed the timestamp.
        {
            if (pChar == nullptr)
                continue;   // Destroyed by the tick of a char processed before (ForgetCollectedChar).
            pChar->_uiTickFlags &= (uint8)~CTimedObject::kTickFlagPeriodicCollected;

            // Their entry was removed: a sleeping char is registered again by _GoAwake, one being deleted doesn't need it.
            if (pChar->_IsBeingDeleted() || pChar->_IsSleeping())
            {
                continue;
            }

#ifdef UNIT_TESTING
            if (_pfnUnitTestCharTurn != nullptr)
                _pfnUnitTestCharTurn(pChar);
#endif
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

    // Reset even if an exception goes through (EXC_TRY doesn't catch anything in some builds).
    struct TickingScope
    {
        bool& fTicking;
        explicit TickingScope(bool& fFlag) noexcept : fTicking(fFlag) { fTicking = true; }
        ~TickingScope() { fTicking = false; }
    } const tickingScope(_fTicking);

    EXC_TRY("CWorldTicker::Tick");

    ProcessServerTickActions();

    _iCurTickStartTime = CWorldGameTime::GetCurrentTime().GetTimeRaw();    // Current timestamp, a few msecs will advance in the current tick ... avoid them until the following tick(s).

    // Items, Chars ... Everything relying on CTimedObject.
    ProcessTimedObjects();

    // Periodic, automatic ticking specific for CChar instances.
    ProcessCharPeriodicTicks();

    EXC_CATCH;
}
