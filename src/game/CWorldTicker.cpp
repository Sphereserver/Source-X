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
    std::unique_lock<std::shared_mutex> lock(_mWorldTickList.THREAD_CMUTEX);
    TimedObjectsContainer& cont = _mWorldTickList[iTimeout];
    cont.emplace_back(pTimedObject);

    // pTimedObject should already have its mutex locked by CTimedObject::SetTimeout
    pTimedObject->_iTimeout = iTimeout;
}

void CWorldTicker::_RemoveTimedObject(const int64 iOldTimeout, CTimedObject* pTimedObject)
{
    std::unique_lock<std::shared_mutex> lock(_mWorldTickList.THREAD_CMUTEX);
    auto itList = _mWorldTickList.find(iOldTimeout);
    if (itList == _mWorldTickList.end())
    {
        //ASSERT(0);  // This shouldn't happen
        return;
    }
    TimedObjectsContainer& cont = itList->second;  // direct access to the container.

    cont.erase(std::remove(cont.begin(), cont.end(), pTimedObject), cont.end());
    if (cont.empty())
    {
        _mWorldTickList.erase(itList);
    }

    // pTimedObject should already have its mutex locked by CTimedObject::SetTimeout
    pTimedObject->ClearTimeout();
}

void CWorldTicker::AddTimedObject(const int64 iTimeout, CTimedObject* pTimedObject)
{
    //if (iTimeout < CWorldGameTime::GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible
    //    return;

    EXC_TRY("AddTimedObject");
    const ProfileTask timersTask(PROFILE_TIMERS);

    /*
    if (!fIgnoreSleep)
    {
        const CSector* pSector = pChar->GetTopSector();
        if (pSector && pSector->IsSleeping())
            return; // Do not allow ticks on sleeping sectors;
    }
    */

    EXC_SET_BLOCK("Already ticking?");
    const int64 iTickOld = pTimedObject->_iTimeout;
    if (iTickOld != 0)
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        EXC_SET_BLOCK("Remove");
        _RemoveTimedObject(iTickOld, pTimedObject);
    }

    EXC_SET_BLOCK("Insert");
    _InsertTimedObject(iTimeout, pTimedObject);

    EXC_CATCH;
}

void CWorldTicker::DelTimedObject(CTimedObject* pTimedObject)
{
    EXC_TRY("DelTimedObject");
    const ProfileTask timersTask(PROFILE_TIMERS);

    EXC_SET_BLOCK("Not ticking?");
    const int64 iTickOld = pTimedObject->_iTimeout;
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

void CWorldTicker::AddCharTicking(CChar* pChar, bool fIgnoreSleep)
{
    EXC_TRY("AddCharTicking");

    const ProfileTask timersTask(PROFILE_TIMERS);
    // The mutex on the char should already be locked at this point, by 

    if (!fIgnoreSleep)
    {
        EXC_SET_BLOCK("Sector sleeping?");
        const CSector* pSector = pChar->GetTopSector();
        if (pSector && pSector->IsSleeping())
            return; // Do not allow ticks on sleeping sectors. This char will be added when the sector awakes.
    }


    const int64 iTickNext = pChar->_iTimeNextRegen;
    //if (iTickNext < CWorldGameTime::GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible
    //    return;

    const int64 iTickOld = pChar->_iTimePeriodicTick;
    if (iTickOld != 0)
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        EXC_SET_BLOCK("Remove");
        _RemoveCharTicking(iTickOld, pChar);
    }

    EXC_SET_BLOCK("Insert");
    _InsertCharTicking(iTickNext, pChar);

    EXC_CATCH;
}

void CWorldTicker::DelCharTicking(CChar* pChar)
{
    EXC_TRY("DelCharTicking");
    const ProfileTask timersTask(PROFILE_TIMERS);

    const int64 iOldTimeout = pChar->_iTimePeriodicTick;

    if (iOldTimeout == 0)
        return;

    EXC_SET_BLOCK("Remove");
    _RemoveCharTicking(iOldTimeout, pChar);

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
        * these objects will normally be in containers which don't have any period OnTick method
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
                    if (pTimedObj->IsTimerSet() && !pTimedObj->IsSleeping()) // Double check
                    {
                        if (pTimedObj->_iTimeout <= iTime)
                        {
                            vecObjs.emplace_back(static_cast<void*>(pTimedObj));
                        }

                        /*
                        * Doing a SetTimeout() in the object's tick will force CWorld to search for that object's
                        * current timeout to remove it from any list, prevent that to happen here since it should
                        * not belong to any other tick than the current one.
                        */
                        pTimedObj->ClearTimeout();

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

            CTimedObject* pObj = static_cast<CTimedObject*>(pObjVoid);
            const PROFILE_TYPE profile = pObj->GetProfileType();
            const ProfileTask  profileTask(profile);

            // Default to true, so if any error occurs it gets deleted for safety
            //  (valid only for classes having the Delete method, which, for everyone to know, does NOT destroy the object).
            bool fDelete = true;

            switch (profile)
            {
                case PROFILE_ITEMS:
                {
                    CItem* pItem = dynamic_cast<CItem*>(pObj);
                    ASSERT(pItem);
                    if (pItem->IsItemEquipped())
                    {
                        ptcSubDesc = "ItemEquipped";
                        CObjBaseTemplate* pObjTop = pItem->GetTopLevelObj();
                        ASSERT(pObjTop);
                        CChar* pChar = dynamic_cast<CChar*>(pObjTop);
                        ASSERT(pChar);
                        fDelete = !pChar->OnTickEquip(pItem);
                        break;
                    }
                    else
                    {
                        ptcSubDesc = "Item";
                        fDelete = (pItem->OnTick() == false);
                        break;
                    }
                }
                break;

                case PROFILE_CHARS:
                {
                    ptcSubDesc = "Char";
                    CChar* pChar = dynamic_cast<CChar*>(pObj);
                    ASSERT(pChar);
                    fDelete = !pChar->OnTick();
                    if (pChar->m_pNPC && !pObj->IsTimerSet())
                    {
                        pObj->SetTimeoutS(3);   //3 seconds timeout to keep NPCs 'alive'
                    }
                }
                break;

                case PROFILE_SECTORS:
                {
                    ptcSubDesc = "Sector";
                    fDelete = false;    // sectors should NEVER be deleted.
                    pObj->OnTick();
                }
                break;

                case PROFILE_MULTIS:
                {
                    ptcSubDesc = "Multi";
                    fDelete = !pObj->OnTick();
                }
                break;

                case PROFILE_SHIPS:
                {
                    ptcSubDesc = "ItemShip";
                    fDelete = !pObj->OnTick();
                }
                break;

                case PROFILE_TIMEDFUNCTIONS:
                {
                    ptcSubDesc = "TimedFunction";
                    fDelete = false;
                    pObj->OnTick();
                }
                break;

                default:
                {
                    ptcSubDesc = "Default";
                    fDelete = !pObj->OnTick();
                }
                break;
            }

            if (fDelete)
            {
                EXC_SETSUB_BLOCK("Delete");
                CObjBase* pObjBase = dynamic_cast<CObjBase*>(pObj);
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
                if ((pChar->_iTimePeriodicTick != 0) && !pChar->IsSleeping())
                {
                    if (pChar->_iTimePeriodicTick <= iTime)
                    {
                        vecObjs.emplace_back(static_cast<void*>(pChar));
                    }
                    pChar->_iTimePeriodicTick = 0;

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
                pChar->Delete();
            }
        }
        EXC_CATCHSUB("");
    }

    EXC_CATCH;
}