#include "CWorldTickingList.h"
#include "chars/CChar.h"
#include "CWorld.h"
#include "CWorldTicker.h"



void CWorldTickingList::AddObjSingle(int64 iTimeout, CTimedObject* pObj, bool fForce) // static
{
    //The lock on pObj should already be acquired by CTimedObject::SetTimeout
    g_World._Ticker.AddTimedObject(iTimeout, pObj, fForce);
}

bool CWorldTickingList::DelObjSingle(CTimedObject* pObj) // static
{
    //The lock on pObj should already be acquired by CTimedObject::SetTimeout
    return g_World._Ticker.DelTimedObject(pObj);
}

auto CWorldTickingList::IsTimeoutRegistered(const CTimedObject* pTimedObject) -> std::optional<std::pair<int64, CTimedObject*>>
{
    return g_World._Ticker.IsTimeoutRegistered(pTimedObject);
}

void CWorldTickingList::AddCharPeriodic(CChar* pChar, bool fNeedsLock) // static
{
    g_World._Ticker.AddCharTicking(pChar, fNeedsLock);
}

bool CWorldTickingList::DelCharPeriodic(CChar* pChar, bool fNeedsLock) // static
{
    return g_World._Ticker.DelCharTicking(pChar, fNeedsLock);
}

auto CWorldTickingList::IsCharPeriodicTickRegistered(const CChar* pChar) -> std::optional<std::pair<int64, CChar*>>
{
    return g_World._Ticker.IsCharPeriodicTickRegistered(pChar);
}

void CWorldTickingList::AddObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
    g_World._Ticker.AddObjStatusUpdate(pObj, fNeedsLock);
}

bool CWorldTickingList::DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
    return g_World._Ticker.DelObjStatusUpdate(pObj, fNeedsLock);
}

bool CWorldTickingList::IsStatusUpdateTickRegistered(const CObjBase* pObj)
{
    return g_World._Ticker.IsStatusUpdateTickRegistered(pObj);
}


void CWorldTickingList::ClearTickingLists() // static
{
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(g_World._Ticker._mWorldTickList.MT_CMUTEX);
#endif
        for (auto& cont = g_World._Ticker._mWorldTickList; auto& elem : cont) {
            DEBUG_ASSERT(elem.second->_fIsAddingInWorldTickList == false);
            elem.second->_fIsInWorldTickList = false;
        }
        for (auto& cont = g_World._Ticker._vecWorldObjsAddRequests; auto& elem : cont) {
            DEBUG_ASSERT(elem.second->_fIsInWorldTickList == false);
            elem.second->_fIsAddingInWorldTickList = false;
        }
#ifdef _DEBUG
        for (auto& cont = g_World._Ticker._vecWorldObjsEraseRequests; auto& elem : cont) {
            DEBUG_ASSERT(elem->_fIsInWorldTickList == false);
            DEBUG_ASSERT(elem->_fIsAddingInWorldTickList == false);
        }
#endif
        g_World._Ticker._mWorldTickList.clear();
        g_World._Ticker._vecWorldObjsAddRequests.clear();
        g_World._Ticker._vecWorldObjsEraseRequests.clear();
    }
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(g_World._Ticker._mCharTickList.MT_CMUTEX);
#endif
        for (auto& cont = g_World._Ticker._mCharTickList ; auto& elem : cont) {
            elem.second->_iTimePeriodicTick = 0;
        }
        for (auto& cont = g_World._Ticker._vecPeriodicCharsAddRequests; auto& elem : cont) {
            elem.second->_iTimePeriodicTick = 0;
        }
#ifdef _DEBUG
        for (auto& cont = g_World._Ticker._vecPeriodicCharsEraseRequests; auto& elem : cont) {
            DEBUG_ASSERT(elem->_iTimePeriodicTick == 0);
        }
#endif

        g_World._Ticker._mCharTickList.clear();
        g_World._Ticker._vecPeriodicCharsAddRequests.clear();
        g_World._Ticker._vecPeriodicCharsEraseRequests.clear();
    }
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(g_World._Ticker._ObjStatusUpdates.MT_CMUTEX);
#endif

        for (auto& cont = g_World._Ticker._ObjStatusUpdates; auto& elem : cont) {
            elem->_fIsInStatusUpdatesList = false;
        }
        for (auto& cont = g_World._Ticker._vecObjStatusUpdateEraseRequests; auto& elem : cont) {
            elem->_fIsInStatusUpdatesEraseList = false;
        }
        g_World._Ticker._ObjStatusUpdates.clear();
        g_World._Ticker._vecObjStatusUpdateEraseRequests.clear();
    }
}
