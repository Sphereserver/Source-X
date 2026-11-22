/**
* @file CWorldTicker.h
*/

#ifndef _INC_CWORLDTICKER_H
#define _INC_CWORLDTICKER_H

#include "CTimedFunctionHandler.h"
#include "CTickWheel.h"
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
    // Hierarchical timing wheel (see CTickWheel): each object stores the handle of its entry (CTimedObject::_hWorldTickWheel),
    //  so adding, removing or rescheduling it takes constant time. Each tick only visits the entries that expired.
    using TickingTimedObjEntry = std::pair<int64, CTimedObject*>;
    struct WorldTickWheel : public CTickWheel<CTimedObject>
    {
        MT_CMUTEX_DEF;
    };
    WorldTickWheel _wheelTimedObjs;
    std::vector<CTimedObject*> _vTimedObjsTimeoutsBuffer;

    // Calls to OnTickPeriodic. CChar regens and periodic checks.
    //  Same as above, the char stores the handle of its entry in CChar::_hPeriodicTickWheel.
    using TickingPeriodicCharEntry = std::pair<int64, CChar*>;
    struct CharTickWheel : public CTickWheel<CChar>
    {
        MT_CMUTEX_DEF;
    };
    CharTickWheel _wheelPeriodicChars;
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

#ifdef UNIT_TESTING
    // The unit tests make it do, in the turn of a char, what its periodic tick can do to the chars after it.
    friend struct CWorldTestAccess;
    void (*_pfnUnitTestCharTurn)(CChar* pChar) = nullptr;
#endif

public:
    void Tick();

    bool AddTimedObject(int64 iTimeout, CTimedObject* pTimedObject, bool fForce);
    bool DelTimedObject(CTimedObject* pTimedObject);
    auto IsTimeoutRegistered(const CTimedObject* pTimedObject) -> std::optional<TickingTimedObjEntry>;

    bool AddCharTicking(CChar* pChar, bool fNeedsLock);
    bool DelCharTicking(CChar* pChar, bool fNeedsLock);
    auto IsCharPeriodicTickRegistered(const CChar* pChar) -> std::optional<std::pair<int64, CChar*>>;

    // An object destroyed while waiting for its turn in the current pass (it has kTickFlagTimeoutCollected or
    //  kTickFlagPeriodicCollected): it's removed from there.
    void ForgetCollectedTimedObject(const CTimedObject* pTimedObject) noexcept;
    void ForgetCollectedChar(const CChar* pChar) noexcept;

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
