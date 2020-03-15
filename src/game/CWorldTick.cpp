#include "../common/CException.h"
#include "../sphere/threads.h"
#include "../sphere/ProfileTask.h"
#include "chars/CChar.h"
#include "items/CItem.h"
#include "items/CItemShip.h"
#include "CSector.h"
#include "CWorldClock.h"
#include "CWorldTick.h"


CWorldTick::CWorldTick(CWorldClock *pClock)
{
    ASSERT(pClock);
    _pWorldClock = pClock;

    _iLastTickDone = 0;
}


void CWorldTick::_InsertTimedObject(int64 iTimeout, CTimedObject* pTimedObject)
{
    _mWorldTickList.THREAD_CMUTEX.lock();
    TimedObjectsContainer& timedObjCont = _mWorldTickList[iTimeout];
    _mWorldTickList.THREAD_CMUTEX.unlock();

    timedObjCont.THREAD_CMUTEX.lock();
    timedObjCont.emplace_back(pTimedObject);
    timedObjCont.THREAD_CMUTEX.unlock();
}

void CWorldTick::_RemoveTimedObject(const int64 iOldTimeout, const CTimedObject* pTimedObject)
{
    _mWorldTickList.THREAD_CMUTEX.lock();
    auto itList = _mWorldTickList.find(iOldTimeout);
    if (itList == _mWorldTickList.end())
    {
        // This shouldn't happen, since this function is called only after we found pChar in the lookup list and retrieved its iOldTimeout.
        // If this happens: there was an exception in the ticking code, or we are calling this method when we shouldn't.
        _mWorldTickList.THREAD_CMUTEX.unlock();
        return;
    }
    TimedObjectsContainer& cont = itList->second;  // direct access to the container.
    _mWorldTickList.THREAD_CMUTEX.unlock();

    std::vector<CTimedObject*> tmpCont;   // new container.
    cont.THREAD_CMUTEX.lock();
    if (cont.size() > 1) // if the old container only has 1 entry we don't need to create a new one.
    {
        for (CTimedObject* pObj : cont)    // Loop until the old container is empty
        {
            if (pObj == pTimedObject)   // if pTimedObject is this entry skip it to remove it from the container.
            {
                continue;
            }
            tmpCont.emplace_back(pObj);  // otherwise add it to the new container.
        }
    }
    cont.clear();

    /*
    * All references to the given CTimedObject have been taken out from the container
    * and the new one have been populated ? so let's add the new container to the main
    * container, if it has any entry, or clear the top container recursively.
    */
    if (!tmpCont.empty())
    {
        cont.swap(tmpCont);
    }
    cont.THREAD_CMUTEX.unlock();
}

void CWorldTick::AddTimedObject(int64 iTimeout, CTimedObject* pTimedObject)
{
    ADDTOCALLSTACK("CWorld::AddTimedObject");
    //if (iTimeout < g_World.GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible
    //    return;

    EXC_TRY("AddTimedObject");
    EXC_SET_BLOCK("Lookup");

    const ProfileTask timersTask(PROFILE_TIMERS);
    std::unique_lock<std::shared_mutex> lookupLock(_mWorldTickLookup.THREAD_CMUTEX);

    auto itLookup = _mWorldTickLookup.find(pTimedObject);
    if (itLookup != _mWorldTickLookup.end())
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        // It shouldn't happen, but if it does, this fixes it.
        EXC_SET_BLOCK("LookupReplace");
        _RemoveTimedObject(itLookup->second, itLookup->first);
        itLookup->second = iTimeout;
    }
    else
    {
        EXC_SET_BLOCK("LookupInsert");
        _mWorldTickLookup.emplace(pTimedObject, iTimeout);
    }

    EXC_SET_BLOCK("InsertTimedObject");
    _InsertTimedObject(iTimeout, pTimedObject);

    EXC_CATCH;
}

void CWorldTick::DelTimedObject(CTimedObject* pTimedObject)
{
    ADDTOCALLSTACK("CWorld::DelTimedObject");
    EXC_TRY("AddTimedObject");
    EXC_SET_BLOCK("Lookup");

    const ProfileTask timersTask(PROFILE_TIMERS);
    std::unique_lock<std::shared_mutex> lookupLock(_mWorldTickLookup.THREAD_CMUTEX);

    const auto lookupIt = _mWorldTickLookup.find(pTimedObject);
    if (lookupIt == _mWorldTickLookup.end())
        return;

    EXC_SET_BLOCK("LookupRemove");
    const int64 iOldTimeout = lookupIt->second;
    _mWorldTickLookup.erase(lookupIt);

    EXC_SET_BLOCK("RemoveTimedObject");
    _RemoveTimedObject(iOldTimeout, pTimedObject);

    EXC_CATCH;
}

void CWorldTick::_InsertCharTicking(int64 iTickNext, CChar* pChar)
{
    std::shared_lock<std::shared_mutex> shared_lock(_mWorldTickList.THREAD_CMUTEX);
    TimedCharsContainer& timedObjCont = _mCharTickList[iTickNext];

    std::shared_lock<std::shared_mutex> shared_lock_cont(timedObjCont.THREAD_CMUTEX);
    timedObjCont.emplace_back(pChar);
}

void CWorldTick::_RemoveCharTicking(const int64 iOldTimeout, const CChar* pChar)
{
    _mCharTickList.THREAD_CMUTEX.lock();
    auto itList = _mCharTickList.find(iOldTimeout);
    if (itList == _mCharTickList.end())
    {
        // This shouldn't happen, since this function is called only after we found pChar in the lookup list and retrieved its iOldTimeout.
        // If this happens: there was an exception in the ticking code, or we are calling this method when we shouldn't.
        _mCharTickList.THREAD_CMUTEX.unlock();
        return;
    }
    TimedCharsContainer& cont = itList->second;  // direct access to the container.
    _mCharTickList.THREAD_CMUTEX.unlock();

    std::vector<CChar*> tmpCont;   // new container.
    cont.THREAD_CMUTEX.lock();
    if (cont.size() > 1) // if the old container only has 1 entry we don't need to create a new one.
    {
        for (CChar* pCharLoop : cont)    // Loop until the old container is empty
        {
            if (pCharLoop == pChar)   // if pTimedObject is this entry skip it to remove it from the container.
            {
                continue;
            }
            tmpCont.emplace_back(pCharLoop);  // otherwise add it to the new container.
        }
    }
    cont.clear();

    /*
    * All references to the given CChar have been taken out from the container
    * and the new one have been populated ? so let's add the new container to the main
    * container, if it has any entry, or clear the top container recursively.
    */
    if (!tmpCont.empty())
    {
        cont.swap(tmpCont);
    }
    cont.THREAD_CMUTEX.unlock();
}

void CWorldTick::AddCharTicking(CChar* pChar, bool fIgnoreSleep, bool fOverwrite)
{
    ADDTOCALLSTACK("CWorld::AddCharTicking");
    EXC_TRY("AddCharTicking");

    if (!fIgnoreSleep && pChar->GetTopSector()->IsSleeping())
    {
        return; // Do not allow ticks on sleeping sectors;
    }

    EXC_SET_BLOCK("Lookup");
    const ProfileTask timersTask(PROFILE_TIMERS);
    std::unique_lock<std::shared_mutex> lookupLock(_mCharTickLookup.THREAD_CMUTEX);

    const int64 iTickNext = pChar->_timeNextRegen;
    //if (iTickNext < g_World.GetCurrentTime().GetTimeRaw())    // We do that to get them tick as sooner as possible
    //    return;

    bool fDoNotInsert = false;
    const auto itLookup = _mCharTickLookup.find(pChar);
    if (itLookup != _mCharTickLookup.end())
    {
        // Adding an object already on the list? Am i setting a new timeout without deleting the previous one?
        // It shouldn't happen, but if it does, this fixes it.
        EXC_SET_BLOCK("LookupReplace");
        if (fOverwrite)
        {
            _RemoveCharTicking(itLookup->second, itLookup->first);
            itLookup->second = iTickNext;
        }
        else
        {
            fDoNotInsert = true;
        }
    }
    else
    {
        EXC_SET_BLOCK("LookupInsert");
        _mCharTickLookup.emplace(pChar, iTickNext);
    }

    if (!fDoNotInsert)
    {
        EXC_SET_BLOCK("InsertCharTicking");
        _InsertCharTicking(iTickNext, pChar);
    }

    EXC_CATCH;
}

void CWorldTick::DelCharTicking(CChar* pChar)
{
    ADDTOCALLSTACK("CWorld::DelCharTicking");
    EXC_TRY("DelCharTicking");
    EXC_SET_BLOCK("Lookup");

    const ProfileTask timersTask(PROFILE_TIMERS);
    std::unique_lock<std::shared_mutex> lookupLock(_mCharTickLookup.THREAD_CMUTEX);

    auto lookupIt = _mCharTickLookup.find(pChar);
    if (lookupIt == _mCharTickLookup.end())
        return;

    EXC_SET_BLOCK("RemoveCharTicking");
    const int64 iOldTimeout = lookupIt->second;
    _RemoveCharTicking(iOldTimeout, pChar);

    EXC_SET_BLOCK("LookupRemove");
    _mCharTickLookup.erase(lookupIt);

    EXC_CATCH;
}


void CWorldTick::Tick()
{
    EXC_TRY("CWorld::OnTick");
    EXC_SET_BLOCK("Once per tick stuff");
    // Do this once per tick.
    // Update status flags from objects, update current tick.
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
            std::shared_lock<std::shared_mutex> lock_su(m_ObjStatusUpdates.THREAD_CMUTEX);
            if (!m_ObjStatusUpdates.empty())
            {
                EXC_TRYSUB("Tick::StatusUpdates");

                // loop backwards to avoid possible infinite loop if a status update is triggered
                // as part of the status update (e.g. property changed under tooltip trigger)
                for (CObjBase* pObj : m_ObjStatusUpdates)
                {
                    if (pObj != nullptr)
                        pObj->OnTickStatusUpdate();
                }
                m_ObjStatusUpdates.clear();

                EXC_CATCHSUB("StatusUpdates");
            }
        }

        // TimerF
        EXC_TRYSUB("Tick::TimerF");
        m_TimedFunctions.OnTick();
        EXC_CATCHSUB("CTimedFunction");
    }

    /* World ticking (timers) */
    // Items, Chars ... Everything relying on CTimedObject (excepting CObjBase, which inheritance is only virtual)
    int64 iCurTime = CServerTime::GetCurrentTime().GetTimeRaw();    // Current timestamp, a few msecs will advance in the current tick ... avoid them until the following tick(s).

    EXC_SET_BLOCK("WorldObjects selection");
    {
        const ProfileTask timersTask(PROFILE_TIMERS);
        std::vector<CTimedObject*> vecTimedObjs;
        {
            // Need here a new, inner scope to get rid of EXC_TRYSUB variables and for the unique_lock
            EXC_TRYSUB("Tick::WorldObj");
            std::unique_lock<std::shared_mutex> lock(_mWorldTickList.THREAD_CMUTEX);
            std::unique_lock<std::shared_mutex> lockLookup(_mWorldTickLookup.THREAD_CMUTEX);
            std::map<int64, TimedObjectsContainer>::iterator it = _mWorldTickList.begin();
            const std::map<int64, TimedObjectsContainer>::iterator itEnd = _mWorldTickList.end();
            int64 iTime;
            while ((it != itEnd) && (iCurTime > (iTime = it->first)))
            {
                {
                    // Need the inner scope for the lock
                    const TimedObjectsContainer& cont = it->second;
                    std::shared_lock<std::shared_mutex> lockCont(cont.THREAD_CMUTEX);

                    for (CTimedObject* pTimedObj : cont)
                    {
                        if (_mWorldTickLookup.erase(pTimedObj) != 0)    // Double check: ensure this object exists also in the lookup cont
                        {
                            vecTimedObjs.emplace_back(pTimedObj);
                        }
                    }
                    // Unlock cont's mutex before erasing the element at iterator
                }
                it = _mWorldTickList.erase(it);
            }
            EXC_CATCHSUB("Reading from _mWorldTickList");
        }

        EXC_SET_BLOCK("WorldObjects loop");
        lpctstr ptcSubDesc = TSTRING_NULL;
        for (CTimedObject* pObj : vecTimedObjs)    // Loop through all msecs stored, unless we passed the timestamp.
        {
            EXC_TRYSUB("Tick::WorldObj");
            EXC_SETSUB_BLOCK("Elapsed");
            ptcSubDesc = "Generic";
            const PROFILE_TYPE profile = pObj->GetProfileType();
            const ProfileTask  profileTask(profile);

            /*
            * Doing a SetTimeout() in the object's tick will force CWorld to search for that object's
            * current timeout to remove it from any list, prevent that to happen here since it should
            * not belong to any other tick than the current one.
            */
            if (pObj->IsSleeping()) // Ignore what is sleeping.
            {
                continue;
            }
            pObj->ClearTimeout();
            bool fRemove = true;    // Default to true, so if any error occurs it gets deleted for safety.
            switch (profile)
            {
            case PROFILE_ITEMS:
            {
                ptcSubDesc = "Item (Generic)";
                CItem* pItem = dynamic_cast<CItem*>(pObj);
                ptcSubDesc = "Item (Casted)";
                ASSERT(pItem);
                if (pItem->IsItemEquipped())
                {
                    ptcSubDesc = "ItemEquipped (CObjBaseTemplate)";
                    CObjBaseTemplate* pObjTop = pItem->GetTopLevelObj();
                    ptcSubDesc = "ItemEquipped (CObjBaseTemplateCasted)";
                    ASSERT(pObjTop);
                    CChar* pChar = dynamic_cast<CChar*>(pObjTop);
                    ASSERT(pChar);
                    ptcSubDesc = "ItemEquipped (CCharCasted)";
                    fRemove = !pChar->OnTickEquip(pItem);
                    break;
                }
                else
                {
                    ptcSubDesc = "Item";
                    fRemove = (pObj->OnTick() == false);
                    break;
                }
            }
            break;

            case PROFILE_CHARS:
            {
                ptcSubDesc = "Char";
                fRemove = !pObj->OnTick();
                ptcSubDesc = "Char (PostTick)";
                CChar* pChar = dynamic_cast<CChar*>(pObj);
                ptcSubDesc = "Char (Casted)";
                ASSERT(pChar);
                if (pChar->m_pNPC && !pObj->IsTimerSet())
                {
                    pObj->SetTimeoutS(3);   //3 seconds timeout to keep NPCs 'alive'
                }
            }
            break;

            case PROFILE_SECTORS:
            {
                ptcSubDesc = "Sector";
                fRemove = false;    // sectors should NEVER be deleted.
                pObj->OnTick();
            }
            break;

            case PROFILE_MULTIS:
            {
                ptcSubDesc = "Multi";
                CItemMulti* pMulti = dynamic_cast<CItemMulti*>(pObj);
                ptcSubDesc = "Multi (Casted)";
                ASSERT(pMulti);
                fRemove = !pMulti->OnTick();
            }
            break;

            case PROFILE_SHIPS:
            {
                ptcSubDesc = "Ship";
                CItem* pItem = dynamic_cast<CItem*>(pObj);
                ptcSubDesc = "Ship (CItem Casted)";
                ASSERT(pItem); UNREFERENCED_PARAMETER(pItem);
                ASSERT(dynamic_cast<CItemShip*>(pItem));
                ptcSubDesc = "Ship (CItemShip Casted)";
                fRemove = !pObj->OnTick();
            }
            break;

            default:
            {
                ptcSubDesc = "Default";
                fRemove = !pObj->OnTick(); // do tick.
            }
            break;
            }
            if (fRemove)
            {
                EXC_SETSUB_BLOCK("Delete");
                CObjBase* pObjBase = dynamic_cast<CObjBase*>(pObj);
                ASSERT(pObjBase);
                pObjBase->Delete();
            }
            EXC_CATCHSUB(ptcSubDesc);
        }
    }

    /* Periodic, automatic ticking for every char */

    EXC_SET_BLOCK("PeriodicChars selection");
    {
        const ProfileTask taskChars(PROFILE_CHARS);
        std::vector<CChar*> vecPeriodicChars;
        {
            // Need here a new, inner scope to get rid of EXC_TRYSUB variables, and for the unique_lock
            EXC_TRYSUB("Tick::PeriodicChar");
            std::unique_lock<std::shared_mutex> lock(_mCharTickList.THREAD_CMUTEX);
            std::unique_lock<std::shared_mutex> lockLookup(_mCharTickLookup.THREAD_CMUTEX);
            std::map<int64, TimedCharsContainer>::iterator charIt = _mCharTickList.begin();
            const std::map<int64, TimedCharsContainer>::iterator charItEnd = _mCharTickList.end();
            int64 iTime;
            while ((charIt != charItEnd) && (iCurTime > (iTime = charIt->first)))
            {
                {
                    // Need the inner scope for the lock
                    const TimedCharsContainer& cont = charIt->second;
                    std::shared_lock<std::shared_mutex> lockCont(cont.THREAD_CMUTEX);

                    for (CChar* pChar : cont)
                    {
                        if (_mCharTickLookup.erase(pChar) != 0) // Double check: ensure this object exists also in the lookup cont
                        {
                            vecPeriodicChars.emplace_back(pChar);
                        }
                    }
                    // Unlock cont's mutex before erasing the element at iterator
                }
                charIt = _mCharTickList.erase(charIt);
            }
            EXC_CATCHSUB("Reading from _mCharTickList");
        }

        EXC_SET_BLOCK("PeriodicChars loop");
        EXC_TRYSUB("Tick::PeriodicChar::Elapsed");
        for (CChar* pChar : vecPeriodicChars)    // Loop through all msecs stored, unless we passed the timestamp.
        {
            if (pChar->OnTickPeriodic())
            {
                AddCharTicking(pChar);
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