#include "CWorld.h"
#include "CWorldTicker.h"
#include "CWorldTickingList.h"

void CWorldTickingList::AddObjSingle(int64 iTimeout, CTimedObject* pObj, bool fForce) // static
{
    //The lock on pObj should already be acquired by CTimedObject::SetTimeout
    g_World._Ticker.AddTimedObject(iTimeout, pObj, fForce);
}

void CWorldTickingList::DelObjSingle(CTimedObject* pObj) // static
{
    //The lock on pObj should already be acquired by CTimedObject::SetTimeout
    g_World._Ticker.DelTimedObject(pObj);
}

void CWorldTickingList::AddCharPeriodic(CChar* pChar, bool fNeedsLock) // static
{
    g_World._Ticker.AddCharTicking(pChar, fNeedsLock);
}

void CWorldTickingList::DelCharPeriodic(CChar* pChar, bool fNeedsLock) // static
{
    g_World._Ticker.DelCharTicking(pChar, fNeedsLock);
}

void CWorldTickingList::AddObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
    g_World._Ticker.AddObjStatusUpdate(pObj, fNeedsLock);
}

void CWorldTickingList::DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock) // static
{
    g_World._Ticker.DelObjStatusUpdate(pObj, fNeedsLock);
}


void CWorldTickingList::ClearTickingLists() // static
{
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(g_World._Ticker._mWorldTickList.MT_CMUTEX);
#endif
        g_World._Ticker._mWorldTickList.clear();
    }
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(g_World._Ticker._mCharTickList.MT_CMUTEX);
#endif
        g_World._Ticker._mCharTickList.clear();
    }
    {
#if MT_ENGINES
        std::unique_lock<std::shared_mutex> lock(g_World._Ticker._ObjStatusUpdates.MT_CMUTEX);
#endif
        g_World._Ticker._ObjStatusUpdates.clear();
    }
}
