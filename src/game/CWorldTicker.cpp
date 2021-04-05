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


CWorldTicker::CWorldTicker(CWorldClock *pClock)
{
    ASSERT(pClock);
    _pWorldClock = pClock;

    _iLastTickDone = 0;
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

    std::unique_lock<std::shared_mutex> lock(_mWorldTickList.THREAD_CMUTEX);
    TimedObjectsContainer& cont = _mWorldTickList[iTimeout];
    cont.emplace_back(pTimedObject);
}

void CWorldTicker::_RemoveTimedObject(const int64 iOldTimeout, CTimedObject* pTimedObject)
{
    ASSERT(iOldTimeout != 0);

    std::unique_lock<std::shared_mutex> lock(_mWorldTickList.THREAD_CMUTEX);
    auto itList = _mWorldTickList.find(iOldTimeout);
    if (itList == _mWorldTickList.end())
    {
        // The object might have a timeout while being in a non-tickable state, so it isn't in the list.
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
    TimedObjectsContainer& cont = itList->second;  // direct access to the container.

    cont.erase(std::remove(cont.begin(), cont.end(), pTimedObject), cont.end());
    if (cont.empty())
    {
        _mWorldTickList.erase(itList);
    }
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
    std::unique_lock<std::shared_mutex> lock(_mCharTickList.THREAD_CMUTEX);

    TimedCharsContainer& cont = _mCharTickList[iTickNext];
    cont.emplace_back(pChar);

    pChar->_iTimePeriodicTick = iTickNext;
}

void CWorldTicker::_RemoveCharTicking(const int64 iOldTimeout, CChar* pChar)
{
    std::unique_lock<std::shared_mutex> lock(_mCharTickList.THREAD_CMUTEX);
    auto itList = _mCharTickList.find(iOldTimeout);
    if (itList == _mCharTickList.end())
    {
        //ASSERT(0);  // This shouldn't happen
        return;
    }
    TimedCharsContainer& cont = itList->second;  // direct access to the container.

    cont.erase(std::remove(cont.begin(), cont.end(), pChar), cont.end());
    if (cont.empty())
    {
        _mCharTickList.erase(itList);
    }

    pChar->_iTimePeriodicTick = 0;
}

void CWorldTicker::AddCharTicking(CChar* pChar, bool fNeedsLock)
{
    EXC_TRY("AddCharTicking");

    const ProfileTask timersTask(PROFILE_TIMERS);

    int64 iTickNext, iTickOld;
    if (fNeedsLock)
    {
        std::unique_lock<std::shared_mutex> lock(pChar->THREAD_CMUTEX);
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
        std::unique_lock<std::shared_mutex> lock(pChar->THREAD_CMUTEX);
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

    UNREFERENCED_PARAMETER(fNeedsLock);
    {
        std::unique_lock<std::shared_mutex> lock(_ObjStatusUpdates.THREAD_CMUTEX);
        _ObjStatusUpdates.insert(pObj);
    }

    EXC_CATCH;
}

void CWorldTicker::DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
    EXC_TRY("DelObjStatusUpdate");

    UNREFERENCED_PARAMETER(fNeedsLock);
    {
        std::unique_lock<std::shared_mutex> lock(_ObjStatusUpdates.THREAD_CMUTEX);
        _ObjStatusUpdates.erase(pObj);
    }

    EXC_CATCH;
}

// Check timeouts and do ticks

void CWorldTicker::Tick()
{
    ADDTOCALLSTACK("CWorldTicker::Tick");
    EXC_TRY("CWorldTicker::Tick");

    std::vector<void*> vecObjs; // Reuse the same container to avoid unnecessary reallocations

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
        * TODO: implement a new class inheriting from CTimedObject to get rid of this code.
        */
        {
            EXC_TRYSUB("StatusUpdates");
            {
                EXC_SETSUB_BLOCK("Selection");
                std::unique_lock<std::shared_mutex> lock_su(_ObjStatusUpdates.THREAD_CMUTEX);
                if (!_ObjStatusUpdates.empty())
                {
                    // loop backwards? to avoid possible infinite loop if a status update is triggered
                    // as part of the status update (e.g. property changed under tooltip trigger)
                    for (CObjBase* pObj : _ObjStatusUpdates)
                    {
                        if (pObj != nullptr)
                            vecObjs.emplace_back(static_cast<void*>(pObj));
                    }
                    _ObjStatusUpdates.clear();
                }
            }

            EXC_SETSUB_BLOCK("Loop");
            for (void* pObjVoid : vecObjs)
            {
                CObjBase* pObj = static_cast<CObjBase*>(pObjVoid);
                pObj->OnTickStatusUpdate();
            }
            EXC_CATCHSUB("");

            vecObjs.clear();
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
            std::unique_lock<std::shared_mutex> lock(_mWorldTickList.THREAD_CMUTEX);

            WorldTickList::iterator itList      = _mWorldTickList.begin();
            WorldTickList::iterator itListEnd   = _mWorldTickList.end();

            int64 iTime;
            while ((itList != itListEnd) && (iCurTime > (iTime = itList->first)))
            {
                TimedObjectsContainer& cont = itList->second;

                TimedObjectsContainer::iterator itContEnd = cont.end();
                for (auto it = cont.begin(); it != itContEnd;)
                {
                    CTimedObject* pTimedObj = *it;
                    
                    // FIXME / TODO: For now, since we don't have multithreading fully working, locking an unneeded mutex causes only useless slowdowns.
                    //std::unique_lock<std::shared_mutex> lockTimeObj(pTimedObj->THREAD_CMUTEX);
                    
                    if (pTimedObj->_IsTimerSet() && pTimedObj->_CanTick())
                    {
                        if (pTimedObj->_GetTimeoutRaw() <= iCurTime)
                        {
                            vecObjs.emplace_back(static_cast<void*>(pTimedObj));
                            pTimedObj->_ClearTimeout();
                        }
                        /*
                        else
                        {
                            // This shouldn't happen... If it does, get rid of the entry on the list anyways,
                            //  it got desynchronized in some way and might be an invalid or even deleted and deallocated object!
                        }
                        */

                        it = cont.erase(it);
                        itContEnd = cont.end();
                    }
                    else
                    {
                        ++it;
                    }
                }

                if (cont.empty())
                {
                    itList      = _mWorldTickList.erase(itList);
                    itListEnd   = _mWorldTickList.end();
                }
                else
                {
                    ++itList;
                }
            }

            EXC_CATCHSUB("");
        }

        lpctstr ptcSubDesc;
        for (void* pObjVoid : vecObjs)    // Loop through all msecs stored, unless we passed the timestamp.
        {
            ptcSubDesc = "Generic";

            EXC_TRYSUB("Timed Object Tick");
            EXC_SETSUB_BLOCK("Elapsed");

            CTimedObject* pTimedObj = static_cast<CTimedObject*>(pObjVoid);

            // FIXME / TODO: For now, since we don't have multithreading fully working, locking an unneeded mutex causes only useless slowdowns.
            //std::unique_lock<std::shared_mutex> lockTimeObj(pTimedObj->THREAD_CMUTEX);

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

    vecObjs.clear();


    /* Periodic, automatic ticking for every char */

    const ProfileTask taskChars(PROFILE_CHARS);

    {
        // Need here a new, inner scope to get rid of EXC_TRYSUB variables, and for the unique_lock
        EXC_TRYSUB("Char Periodic Ticks Selection");
        std::unique_lock<std::shared_mutex> lock(_mCharTickList.THREAD_CMUTEX);

        CharTickList::iterator itList       = _mCharTickList.begin();
        CharTickList::iterator itListEnd    = _mCharTickList.end();

        int64 iTime;
        while ((itList != itListEnd) && (iCurTime > (iTime = itList->first)))
        {
            TimedCharsContainer& cont = itList->second;

            TimedCharsContainer::iterator itContEnd = cont.end();
            for (auto it = cont.begin(); it != itContEnd;)
            {
                CChar* pChar = *it;

                // FIXME / TODO: For now, since we don't have multithreading fully working, locking an unneeded mutex causes only useless slowdowns.
                //std::unique_lock<std::shared_mutex> lockTimeObj(pTimedObj->THREAD_CMUTEX);

                if ((pChar->_iTimePeriodicTick != 0) && pChar->_CanTick())
                {
                    if (pChar->_iTimePeriodicTick <= iCurTime)
                    {
                        vecObjs.emplace_back(static_cast<void*>(pChar));
                        pChar->_iTimePeriodicTick = 0;
                    }
                    /*
                    else
                    {
                        // This shouldn't happen... If it does, get rid of the entry on the list anyways,
                        //  it got desynchronized in some way and might be an invalid or even deleted and deallocated object!
                    }
                    */
                    it = cont.erase(it);
                    itContEnd = cont.end();
                }
                else
                {
                    ++it;
                }
            }

            if (cont.empty())
            {
                itList      = _mCharTickList.erase(itList);
                itListEnd   = _mCharTickList.end();
            }
            else
            {
                ++itList;
            }
        }

        EXC_CATCHSUB("");
    }

    {
        EXC_TRYSUB("Char Periodic Ticks Loop");
        for (void* pObjVoid : vecObjs)    // Loop through all msecs stored, unless we passed the timestamp.
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
    }

    EXC_CATCH;
}