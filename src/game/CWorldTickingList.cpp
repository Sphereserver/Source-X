#include "CWorld.h"
#include "CWorldTicker.h"
#include "CWorldTickingList.h"

void CWorldTickingList::AddObjSingle(int64 iTimeout, CTimedObject* pObj) // static
{
    //The lock on pObj should already be acquired by CTimedObject::SetTimeout
    g_World._Ticker.AddTimedObject(iTimeout, pObj);
}

void CWorldTickingList::DelObjSingle(CTimedObject* pObj) // static
{
    //The lock on pObj should already be acquired by CTimedObject::SetTimeout
    g_World._Ticker.DelTimedObject(pObj);
}

void CWorldTickingList::AddCharPeriodic(CChar* pChar, bool fIgnoreSleep) // static
{
    g_World._Ticker.AddCharTicking(pChar, fIgnoreSleep);
}

void CWorldTickingList::DelCharPeriodic(CChar* pChar) // static
{
    g_World._Ticker.DelCharTicking(pChar);
}

void CWorldTickingList::AddObjStatusUpdate(CObjBase* pObj) // static
{
    std::unique_lock<std::shared_mutex> lock(g_World._Ticker._ObjStatusUpdates.THREAD_CMUTEX);
    g_World._Ticker._ObjStatusUpdates.insert(pObj);
}

void CWorldTickingList::DelObjStatusUpdate(CObjBase* pObj) // static
{
    std::unique_lock<std::shared_mutex> lock(g_World._Ticker._ObjStatusUpdates.THREAD_CMUTEX);
    g_World._Ticker._ObjStatusUpdates.erase(pObj);
}


void CWorldTickingList::ClearTickingLists() // static
{
    {
        std::unique_lock<std::shared_mutex> lock(g_World._Ticker._mWorldTickList.THREAD_CMUTEX);
        g_World._Ticker._mWorldTickList.clear();
    }
    {
        std::unique_lock<std::shared_mutex> lock(g_World._Ticker._mCharTickList.THREAD_CMUTEX);
        g_World._Ticker._mCharTickList.clear();
    }
    {
        std::unique_lock<std::shared_mutex> lock(g_World._Ticker._ObjStatusUpdates.THREAD_CMUTEX);
        g_World._Ticker._ObjStatusUpdates.clear();
    }
}