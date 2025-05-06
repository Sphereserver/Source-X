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
    using TickingTimedObjEntry = std::pair<int64, CTimedObject*>;
    struct WorldTickList : public std::vector<TickingTimedObjEntry>
    {
        MT_CMUTEX_DEF;
    };
    WorldTickList _vTimedObjsTimeouts;
    std::vector<CTimedObject*> _vTimedObjsTimeoutsEraseReq;
    std::vector<TickingTimedObjEntry> _vTimedObjsTimeoutsAddReq;
    std::vector<TickingTimedObjEntry> _vTimedObjsTimeoutsElementBuffer;
    std::vector<CTimedObject*> _vTimedObjsTimeoutsBuffer;

    // Calls to OnTickPeriodic. CChar regens and periodic checks.
    using TickingPeriodicCharEntry = std::pair<int64, CChar*>;
    struct CharTickList : public std::vector<TickingPeriodicCharEntry>
    {
        MT_CMUTEX_DEF;
    };
    CharTickList _vPeriodicCharsTicks;
    std::vector<CChar*> _vPeriodicCharsEraseRequests;
    std::vector<TickingPeriodicCharEntry> _vPeriodicCharsAddRequests;
    std::vector<TickingPeriodicCharEntry> _vPeriodicCharsElementBuffer;
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

    // Reuse the same container (using void pointers statically casted) to avoid unnecessary reallocations.
    std::vector<size_t> _vIndexMiscBuffer;

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
