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
    friend class CWorldTickingList;

    // Generic Timers. Calls to OnTick.
    // Ordered by timeout: each tick only looks at the head of the container, up to the first entry not yet expired.
    //  Each object stores the iterator of its own entry (CTimedObject::_itWorldTickEntry), so adding, removing or
    //  rescheduling it doesn't need to search or rebuild the container.
    using TickingTimedObjEntry = std::pair<int64, CTimedObject*>;
    struct WorldTickList : public TimedObjTickMap
    {
        MT_CMUTEX_DEF;
    };
    WorldTickList _mTimedObjsTimeouts;
    std::vector<CTimedObject*> _vTimedObjsTimeoutsBuffer;

    // Calls to OnTickPeriodic. CChar regens and periodic checks.
    //  Same as above, the char stores the iterator of its entry (CChar::_itPeriodicTickEntry).
    using TickingPeriodicCharEntry = std::pair<int64, CChar*>;
    struct CharTickList : public CharPeriodicTickMap
    {
        MT_CMUTEX_DEF;
    };
    CharTickList _mPeriodicCharsTicks;
    std::vector<CChar*> _vPeriodicCharsTicksBuffer;

    // Calls to OnTickStatusUpdate. Periodically send updated infos to the clients.
    struct StatusUpdatesList : public std::vector<CObjBase*>
    {
        MT_CMUTEX_DEF;
    };
    StatusUpdatesList _vObjStatusUpdates;
    std::vector<CObjBase*> _vObjStatusUpdatesEraseRequests;
    std::vector<CObjBase*> _vObjStatusUpdatesAddRequests;
    std::vector<CObjBase*> _vObjStatusUpdatesTickBuffer;

    //----

    friend class CWorld;
    friend class CWorldTimedFunctions;
    CTimedFunctionHandler _TimedFunctions; // CTimedFunction Container/Wrapper

    //----

    CWorldClock* _pWorldClock;
    int64        _iCurTickStartTime;
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
    void ProcessServerTickActions();
    void ProcessObjStatusUpdates();
    void ProcessTimedObjects();
    void ProcessCharPeriodicTicks();

    bool _InsertTimedObject(int64 iTimeout, CTimedObject* pTimedObject);
    [[nodiscard]]
    bool _EraseTimedObject(CTimedObject* pTimedObject);

    bool _InsertCharTicking(int64 iTickNext, CChar* pChar);
    [[nodiscard]]
    bool _EraseCharTicking(CChar* pChar);
};

#endif // _INC_CWORLDTICKER_H
