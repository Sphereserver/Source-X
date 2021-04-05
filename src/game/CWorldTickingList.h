/**
* @file CWorldTickingList.h
*/

#ifndef _INC_CWORLDTICKINGLIST_H
#define _INC_CWORLDTICKINGLIST_H

class CTimedObject;
class CObjBase;
class CChar;

class CWorldTickingList
{
public:
    static const char* m_sClassName;

    static void AddObjSingle(int64 iTimeout, CTimedObject* pObj, bool fForce);
    static void DelObjSingle(CTimedObject* pObj);

    static void AddCharPeriodic(CChar* pChar, bool fNeedsLock);
    static void DelCharPeriodic(CChar* pChar, bool fNeedsLock);

    static void AddObjStatusUpdate(CObjBase* pObj, bool fNeedsLock);
    static void DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock);

private:
    friend class CWorld;
    static void ClearTickingLists();
};

#endif // _INC_CWORLDTICKINGLIST_H
