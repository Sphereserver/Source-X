/**
* @file CWorldTicker.h
*/

#ifndef _INC_CWORLDTICKER_H
#define _INC_CWORLDTICKER_H

#include "CTimedFunctionHandler.h"
#include "CTimedObject.h"


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
    struct StatusUpdatesList : public std::vector<CObjBase*>
    {
        MT_CMUTEX_DEF;
    };

    WorldTickList _mWorldTickList;
    CharTickList _mCharTickList;

    friend class CWorldTickingList;
    StatusUpdatesList _ObjStatusUpdates;   // objects that need OnTickStatusUpdate called
    std::vector<CObjBase*> _vecObjStatusUpdateEraseRequests;

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

    bool AddTimedObject(int64 iTimeout, CTimedObject* pTimedObject, bool fForce);
    bool DelTimedObject(CTimedObject* pTimedObject);
    auto IsTimeoutRegistered(const CTimedObject* pTimedObject) -> std::optional<TickingTimedObjEntry>;

    bool AddCharTicking(CChar* pChar, bool fNeedsLock);
    bool DelCharTicking(CChar* pChar, bool fNeedsLock);
    auto IsCharPeriodicTickRegistered(const CChar* pChar) -> std::optional<std::pair<int64, CChar*>>;

    bool AddObjStatusUpdate(CObjBase* pObj, bool fNeedsLock);
    bool DelObjStatusUpdate(CObjBase* pObj, bool fNeedsLock);
    bool IsStatusUpdateTickRegistered(const CObjBase* pObj);

private:
    bool _InsertTimedObject(int64 iTimeout, CTimedObject* pTimedObject);
    [[nodiscard]]
    bool _RemoveTimedObject(CTimedObject* pTimedObject);

    bool _InsertCharTicking(int64 iTickNext, CChar* pChar);
    [[nodiscard]]
    bool _RemoveCharTicking(CChar* pChar);
};

#endif // _INC_CWORLDTICKER_H
