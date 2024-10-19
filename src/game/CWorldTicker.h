/**
* @file CWorldTicker.h
*/

#ifndef _INC_CWORLDTICKER_H
#define _INC_CWORLDTICKER_H

#include "CTimedFunctionHandler.h"
#include "CTimedObject.h"

/*
 * #include <flat_containers/flat_set.hpp>
*/

/*
//--- Include phmap.h
#ifdef ADDRESS_SANITIZER
    #define MYASAN_
#endif

#ifdef _WIN32
//    #define MYSRWLOCK_
    #undef SRWLOCK_INIT
#endif
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wshift-count-overflow"
#endif

// TODO: undef is TEMPORARY !! There's a bug that needs to be solved
#undef ADDRESS_SANITIZER
#include <parallel_hashmap/phmap.h>

#ifdef MYASAN_
    #define ADDRESS_SANITIZER
#endif
//#ifdef MYSRWLOCK_
//#   define SRWLOCK_INIT
//#endif

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif
//--- End of phmap.h inclusion
*/


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
    // Generic Timers. Calls to OnTick.
    using TickingTimedObjEntry = std::pair<int64, CTimedObject*>;
    struct WorldTickList : public std::vector<TickingTimedObjEntry>
    {
        MT_CMUTEX_DEF;
    };

    // Calls to OnTickPeriodic. Regens and periodic checks.
    using TickingPeriodicCharEntry = std::pair<int64, CChar*>;
    struct CharTickList : public std::vector<TickingPeriodicCharEntry>
    {
        MT_CMUTEX_DEF;
    };

    // Calls to OnTickStatusUpdate. Periodically send updated infos to the clients.
    //struct StatusUpdatesList : public phmap::parallel_flat_hash_set<CObjBase*>
    //struct StatusUpdatesList : public fc::flat_set<CObjBase*>
    //struct StatusUpdatesList : public std::unordered_set<CObjBase*>
    struct StatusUpdatesList : public std::vector<CObjBase*>
    {
        MT_CMUTEX_DEF;
    };

    WorldTickList _mWorldTickList;
    CharTickList _mCharTickList;

    friend class CWorldTickingList;
    StatusUpdatesList _ObjStatusUpdates;   // objects that need OnTickStatusUpdate called
    std::vector<CObjBase*> _vecObjStatusUpdateEraseRequested;

    // Reuse the same container (using void pointers statically casted) to avoid unnecessary reallocations.
    std::vector<void*> _vecGenericObjsToTick;
    std::vector<size_t> _vecIndexMiscBuffer;

    std::vector<TickingTimedObjEntry> _vecWorldObjsAddRequests;
    std::vector<CTimedObject*> _vecWorldObjsEraseRequests;
    std::vector<TickingTimedObjEntry> _vecWorldObjsElementBuffer;

    std::vector<TickingPeriodicCharEntry> _vecPeriodicCharsAddRequests;
    std::vector<CChar*> _vecPeriodicCharsEraseRequests;
    std::vector<TickingPeriodicCharEntry> _vecPeriodicCharsElementBuffer;

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
    void _InsertTimedObject(int64 iTimeout, CTimedObject* pTimedObject);
    void _RemoveTimedObject(CTimedObject* pTimedObject);
    void _InsertCharTicking(int64 iTickNext, CChar* pChar);
    bool _RemoveCharTicking(CChar* pChar);
};

#endif // _INC_CWORLDTICKER_H
