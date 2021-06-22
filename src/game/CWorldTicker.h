/**
* @file CWorldTicker.h
*/

#ifndef _INC_CWORLDTICKER_H
#define _INC_CWORLDTICKER_H

#include "../parallel_hashmap/phmap.h"
#include "CTimedFunctionHandler.h"
#include "CTimedObject.h"
#include <map>


class CObjBase;
class CChar;
class CWorldClock;

class CWorldTicker
{
public:
    static const char* m_sClassName;
    CWorldTicker(CWorldClock* pClock);
    ~CWorldTicker() = default;

private:
    using TimedObjectsContainer = std::vector<CTimedObject*>;
    struct WorldTickList : public std::map<int64, TimedObjectsContainer>
    {
        THREAD_CMUTEX_DEF;
    };

    using TimedCharsContainer = std::vector<CChar*>;
    struct CharTickList : public std::map<int64, TimedCharsContainer>
    {
        THREAD_CMUTEX_DEF;
    };

    struct StatusUpdatesList : public phmap::parallel_flat_hash_set<CObjBase*>
    {
        THREAD_CMUTEX_DEF;
    };

    WorldTickList _mWorldTickList;
    CharTickList _mCharTickList;

    friend class CWorldTickingList;
    StatusUpdatesList _ObjStatusUpdates;   // objects that need OnTickStatusUpdate called

    friend class CWorld;
    friend class CWorldTimedFunctions;
    CTimedFunctionHandler _TimedFunctions; // CTimedFunction Container/Wrapper

    CWorldClock* _pWorldClock;
    int64        _iLastTickDone;  

public:
    void Tick();

    void AddTimedObject(int64 iTimeout, CTimedObject* pTimedObject, bool fForce);
    void DelTimedObject(CTimedObject* pTimedObject);
    void AddCharTicking(CChar* pChar, bool fNeedsLock);
    void DelCharTicking(CChar* pChar, bool fNeedsLock);
    void AddObjStatusUpdate(CObjBase* pObj, bool fNeedsLock);
    void DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock);

private:
    void _InsertTimedObject(const int64 iTimeout, CTimedObject* pTimedObject);
    void _RemoveTimedObject(const int64 iOldTimeout, CTimedObject* pTimedObject);
    void _InsertCharTicking(const int64 iTickNext, CChar* pChar);
    void _RemoveCharTicking(const int64 iOldTimeout, CChar* pChar);
};

#endif // _INC_CWORLDTICKER_H
