/**
* @file CWorldTicker.h
*/

#ifndef _INC_CWORLDTICKER_H
#define _INC_CWORLDTICKER_H

#include "CTimedFunctionHandler.h"
#include "CTimedObject.h"
#include <map>
//#include <unordered_set>

#ifdef ADDRESS_SANITIZER
    #define MYASAN_
#endif

#ifdef _WIN32
    #undef SRWLOCK_INIT
#endif
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wshift-count-overflow"
#endif

// TODO: TEMPORARY !!
#undef ADDRESS_SANITIZER
#include <parallel_hashmap/phmap.h>
#ifdef MYASAN_
    #define ADDRESS_SANITIZER
#endif

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif
#include <parallel_hashmap/btree.h>



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
    struct WorldTickList : public phmap::btree_multimap<int64, CTimedObject*>
    {
        MT_CMUTEX_DEF;
    };

    struct CharTickList : public phmap::btree_multimap<int64, CChar*>
    {
        MT_CMUTEX_DEF;
    };

    struct StatusUpdatesList : public phmap::parallel_flat_hash_set<CObjBase*>
    //struct StatusUpdatesList : public std::unordered_set<CObjBase*>
    {
        MT_CMUTEX_DEF;
    };

    WorldTickList _mWorldTickList;
    CharTickList _mCharTickList;

    friend class CWorldTickingList;
    StatusUpdatesList _ObjStatusUpdates;   // objects that need OnTickStatusUpdate called

    // Reuse the same container (using void pointers statically casted) to avoid unnecessary reallocations.
    std::vector<void*> _vecObjs;
    // "Index" in the multimap
    std::vector<size_t> _vecWorldObjsToEraseFromList;
    // "Index" in the multimap
    std::vector<size_t> _vecPeriodicCharsToEraseFromList;

    //----

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
