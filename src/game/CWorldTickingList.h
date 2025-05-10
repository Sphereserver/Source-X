/**
* @file CWorldTickingList.h
*/

#ifndef _INC_CWORLDTICKINGLIST_H
#define _INC_CWORLDTICKINGLIST_H

#include "../common/common.h"

class CTimedObject;
class CObjBase;
class CChar;

class CWorldTickingList
{
public:
    static const char* m_sClassName;

    static void AddObjSingle(int64 iTimeout, CTimedObject* pObj, bool fForce);
    static bool DelObjSingle(CTimedObject* pObj);
    static auto IsTimeoutRegistered(const CTimedObject* pTimedObject) -> std::optional<std::pair<int64, CTimedObject*>>;

    static void AddCharPeriodic(CChar* pChar, bool fNeedsLock);
    static bool DelCharPeriodic(CChar* pChar, bool fNeedsLock);
    static auto IsCharPeriodicTickRegistered(const CChar* pChar) -> std::optional<std::pair<int64, CChar*>>;

    static void AddObjStatusUpdate(CObjBase* pObj, bool fNeedsLock);
    static bool DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock);
    static bool IsStatusUpdateTickRegistered(const CObjBase *pObj);

private:
    friend class CWorld;
    static void ClearTickingLists();
};

#endif // _INC_CWORLDTICKINGLIST_H
