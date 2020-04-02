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

    static void AddObjSingle(int64 iTimeout, CTimedObject* pObj);
    static void DelObjSingle(CTimedObject* pObj);

    static void AddCharPeriodic(CChar* pChar, bool fIgnoreSleep = false);
    static void DelCharPeriodic(CChar* pChar);

    static void AddObjStatusUpdate(CObjBase* pObj);
    static void DelObjStatusUpdate(CObjBase* pObj);

private:
    friend class CWorld;
    static void ClearTickingLists();
};

#endif // _INC_CWORLDTICKINGLIST_H
