#include <doctest/doctest.h>
#include "../../src/game/CWorld.h"
#include "../../src/game/CWorldGameTime.h"
#include "../../src/game/CWorldTickingList.h"
#include "../../src/game/CWorldTimedFunctions.h"
#include "../../src/game/CServer.h"
#include "../../src/game/CServerConfig.h"
#include "../../src/game/chars/CChar.h"
#include "../../src/game/chars/CCharBase.h"
#include "../../src/game/uo_files/uofiles_enums_creid.h"
#include <algorithm>
#include <functional>
#include <vector>

// What the tick of an object does to another object expiring in the same tick, waiting for its turn in the same pass.
// These cases run the world ticker (g_World), without loading a world.

struct CWorldTestAccess
{
    // The time of the world only moves forward between the cases: the timing wheel keeps its current time.
    static void AdvanceTime(int64 iMsecs)
    {
        static int64 s_iTime = 1'000'000'000;
        s_iTime += iMsecs;
        g_World._GameClock.InitTime(s_iTime);
    }

    static void Tick()
    {
        g_World._Ticker.Tick();
    }

    // Objects whose deletion was scheduled (CWorldThread::ScheduleObjDeletion), destroyed by the next garbage collection.
    static size_t CountScheduledDeletions()
    {
        return g_World.m_ObjDelete.GetContentCount();
    }
    static void DestroyScheduledDeletions()
    {
        g_World.m_ObjDelete.ClearContainer(false);
    }

    // What CWorld::GarbageCollection does (a world save too, with SAVEGARBAGECOLLECT), without changing the server mode.
    static void CollectGarbage()
    {
        g_World.GarbageCollection_UIDs();
    }

    // Called in the turn of each char in the periodic ticks, before its tick.
    static void SetCharTurnHook(void (*pfnHook)(CChar*))
    {
        g_World._Ticker._pfnUnitTestCharTurn = pfnHook;
    }
};

namespace
{
    // A plain timer, as the timed functions (PROFILE_TIMEDFUNCTIONS: the ticker never deletes them).
    struct TestTimer : public CTimedObject
    {
        std::function<void()> fnOnTick;
        int* piTicks = nullptr;     // Outside the object, to count the ticks of a destroyed object too.

        explicit TestTimer(int* piTickCounter) :
            CTimedObject(PROFILE_TIMEDFUNCTIONS), piTicks(piTickCounter)
        {
            _SetAwakeFlagRaw();
        }

    protected:
        virtual bool _OnTick() override
        {
            ++(*piTicks);
            if (fnOnTick)
            {
                const auto fn = fnOnTick;   // A copy: the function may destroy this object.
                fn();
            }
            return true;
        }

    public:
        virtual bool _IsDeleted() const override
        {
            return false;
        }
        virtual bool IsDeleted() const override
        {
            return false;
        }
    };

    bool IsRegistered(const TestTimer& timer)
    {
        return CWorldTickingList::IsTimeoutRegistered(&timer).has_value();
    }

    // Without scripts, the char needs a definition.
    CREID_TYPE GetTestCharDef()
    {
        const CResourceID rid(RES_CHARDEF, CREID_MAN);
        if (g_Cfg.m_ResHash.FindKey(rid) == sl::scont_bad_index())
            g_Cfg.m_ResHash.AddSortKey(rid, new CCharBase(CREID_MAN));
        return CREID_MAN;
    }

    // A char not placed in the world (disconnected: no client, not an NPC), alive.
    struct TestChar : public CChar
    {
        TestChar() : CTimedObject(PROFILE_CHARS), CChar(GetTestCharDef())    // CTimedObject is a virtual base.
        {
            Stat_SetBase(STAT_STR, 50);
            Stat_SetVal(STAT_STR, 50);
        }

        // Registered for the periodic ticks. GoAwake keeps a disconnected char asleep: it's woken up as in an awake sector.
        void WakeUp()
        {
            GoAwake();
            _SetAwakeFlagRaw();
        }

        // As while ~CChar runs.
        void SetDestructing(bool fDestructing)
        {
            if (fDestructing)
                _uiInternalStateFlags |= SF_DESTRUCTING;
            else
                _uiInternalStateFlags &= (uchar)~SF_DESTRUCTING;
        }
    };

    // The server is in a startup mode in the tests: the objects created don't get a UID (IsDeleted is true, Delete refuses
    //  them, the garbage collection doesn't see them). In this scope they get one, as in a running server.
    struct RunModeScope
    {
        const ServMode prevMode = g_Serv.GetServerMode();
        RunModeScope()  { g_Serv.SetServerMode(ServMode::Run); }
        ~RunModeScope() { g_Serv.SetServerMode(prevMode); }
    };

    bool IsPeriodicRegistered(const CChar* pChar)
    {
        return CWorldTickingList::IsCharPeriodicTickRegistered(pChar).has_value();
    }

    // The chars that had their turn in the periodic ticks, and what to do in a turn, before the tick of that char.
    std::vector<const CChar*> s_vCharTurns;
    std::function<void(CChar*)> s_fnOnCharTurn;

    struct CharTurnHookScope
    {
        CharTurnHookScope()
        {
            s_vCharTurns.clear();
            CWorldTestAccess::SetCharTurnHook(
                [](CChar* pChar)
                {
                    s_vCharTurns.emplace_back(pChar);
                    if (s_fnOnCharTurn)
                        s_fnOnCharTurn(pChar);
                });
        }
        ~CharTurnHookScope()
        {
            CWorldTestAccess::SetCharTurnHook(nullptr);
            s_fnOnCharTurn = nullptr;
        }
    };
}

TEST_CASE("CWorldTicker a TIMER cleared by an earlier tick of the same pass")
{
    int iTicksA = 0, iTicksB = 0;
    TestTimer a(&iTicksA), b(&iTicksB);
    CWorldTestAccess::AdvanceTime(1000);
    a.SetTimeout(10);
    b.SetTimeout(20);
    a.fnOnTick = [&b]() { b.SetTimeout(-1); };

    CWorldTestAccess::AdvanceTime(1000);
    CWorldTestAccess::Tick();
    CHECK(iTicksA == 1);
    CHECK(iTicksB == 0);
    CHECK(!b.IsTimerSet());
    CHECK(!IsRegistered(b));
}

TEST_CASE("CWorldTicker a TIMER changed by an earlier tick of the same pass replaces the expired one")
{
    int iTicksA = 0, iTicksB = 0;
    TestTimer a(&iTicksA), b(&iTicksB);
    CWorldTestAccess::AdvanceTime(1000);
    a.SetTimeout(10);
    b.SetTimeout(20);
    a.fnOnTick = [&b]() { b.SetTimeout(60'000); };

    CWorldTestAccess::AdvanceTime(1000);
    CWorldTestAccess::Tick();
    CHECK(iTicksA == 1);
    CHECK(iTicksB == 0);
    CHECK(b.IsTimerSet());
    CHECK(IsRegistered(b));

    // The new TIMER fires when it expires, once.
    CWorldTestAccess::AdvanceTime(30'000);
    CWorldTestAccess::Tick();
    CHECK(iTicksB == 0);
    CWorldTestAccess::AdvanceTime(31'000);
    CWorldTestAccess::Tick();
    CHECK(iTicksB == 1);
    CHECK(!IsRegistered(b));
}

TEST_CASE("CWorldTicker put to sleep by an earlier tick of the same pass")
{
    int iTicksA = 0, iTicksB = 0;
    TestTimer a(&iTicksA), b(&iTicksB);
    CWorldTestAccess::AdvanceTime(1000);
    a.SetTimeout(10);
    b.SetTimeout(20);
    a.fnOnTick = [&b]() { b.GoSleep(); };

    CWorldTestAccess::AdvanceTime(1000);
    CWorldTestAccess::Tick();
    CHECK(iTicksA == 1);
    CHECK(iTicksB == 0);

    // Woken up with its timer still set: it's registered again and fires in the next pass.
    b.GoAwake();
    CHECK(IsRegistered(b));
    CWorldTestAccess::AdvanceTime(1);
    CWorldTestAccess::Tick();
    CHECK(iTicksB == 1);
}

TEST_CASE("CWorldTicker woken up by an earlier tick of the same pass")
{
    int iTicksA = 0, iTicksB = 0;
    TestTimer a(&iTicksA), b(&iTicksB);
    CWorldTestAccess::AdvanceTime(1000);
    a.SetTimeout(10);
    b.SetTimeout(20);
    b.GoSleep();    // A sleeping object stays registered (as a sector): its expired entry is collected, then skipped.
    a.fnOnTick = [&b]() { b.GoAwake(); };

    CWorldTestAccess::AdvanceTime(1000);
    CWorldTestAccess::Tick();
    CHECK(iTicksA == 1);
    CHECK(iTicksB == 0);
    CHECK(IsRegistered(b));

    CWorldTestAccess::AdvanceTime(1);
    CWorldTestAccess::Tick();
    CHECK(iTicksB == 1);
}

TEST_CASE("CWorldTicker destroyed by an earlier tick of the same pass")
{
    // As a world save or a garbage collection run by a script destroys the objects deleted until then.
    int iTicksA = 0, iTicksB = 0, iTicksC = 0;
    TestTimer a(&iTicksA), c(&iTicksC);
    auto* pB = new TestTimer(&iTicksB);
    CWorldTestAccess::AdvanceTime(1000);
    a.SetTimeout(10);
    pB->SetTimeout(20);
    c.SetTimeout(30);
    a.fnOnTick = [&pB]() { delete pB; pB = nullptr; };

    CWorldTestAccess::AdvanceTime(1000);
    CWorldTestAccess::Tick();
    CHECK(pB == nullptr);
    CHECK(iTicksA == 1);
    CHECK(iTicksB == 0);
    CHECK(iTicksC == 1);    // The objects after it still tick.
}

TEST_CASE("CWorldTicker destroyed in its own tick")
{
    // As a timed function, which deletes itself in its tick.
    int iTicksA = 0, iTicksB = 0;
    auto* pA = new TestTimer(&iTicksA);
    TestTimer b(&iTicksB);
    CWorldTestAccess::AdvanceTime(1000);
    pA->SetTimeout(10);
    b.SetTimeout(20);
    pA->fnOnTick = [&pA]() { delete pA; pA = nullptr; };

    CWorldTestAccess::AdvanceTime(1000);
    CWorldTestAccess::Tick();
    CHECK(pA == nullptr);
    CHECK(iTicksA == 1);
    CHECK(iTicksB == 1);
}

TEST_CASE("CWorldTicker a TIMER set again in its own tick fires once per pass")
{
    // As TIMER=0 in @Timer: the object is due again at once, but not in the pass that is ticking it.
    int iTicks = 0;
    TestTimer a(&iTicks);
    CWorldTestAccess::AdvanceTime(1000);
    a.SetTimeout(10);
    a.fnOnTick = [&a]() { a.SetTimeout(0); };

    CWorldTestAccess::AdvanceTime(1000);
    CWorldTestAccess::Tick();
    CHECK(iTicks == 1);
    CHECK(IsRegistered(a));

    // Another pass at the same time: the new timer expires at the current time, not before it.
    CWorldTestAccess::Tick();
    CHECK(iTicks == 1);

    for (int i = 2; i <= 4; ++i)
    {
        CWorldTestAccess::AdvanceTime(1);
        CWorldTestAccess::Tick();
        CHECK(iTicks == i);
    }
}

TEST_CASE("CWorldTicker a TIMERF stopped by an earlier tick of the same pass")
{
    // No object has this UID: the function would do nothing, but running deletes it.
    const CUID uid(0x00ABCDEFu);
    int iTicksA = 0;
    TestTimer a(&iTicksA);
    CWorldTestAccess::AdvanceTime(1000);
    a.SetTimeout(10);
    CWorldTimedFunctions::Add(uid, 20, "f_unit_test");
    SUBCASE("TIMERF STOP")
    {
        a.fnOnTick = [uid]() { CWorldTimedFunctions::Stop(uid, "f_unit_test"); };
    }
    SUBCASE("TIMERF CLEAR, or the deletion of its object")
    {
        a.fnOnTick = [uid]() { CWorldTimedFunctions::ClearUID(uid); };
    }
    const size_t uiScheduled = CWorldTestAccess::CountScheduledDeletions();

    CWorldTestAccess::AdvanceTime(1000);
    CWorldTestAccess::Tick();
    CHECK(iTicksA == 1);
    // Still waiting to be deleted: it didn't run.
    CHECK(CWorldTestAccess::CountScheduledDeletions() == uiScheduled + 1);
    CWorldTestAccess::DestroyScheduledDeletions();
}

TEST_CASE("CWorldTicker a char destroyed by the periodic tick of a char before it")
{
    // As a world save or a garbage collection run by a script destroys the objects deleted until then.
    CWorldTestAccess::AdvanceTime(1000);
    TestChar* pA = new TestChar;
    TestChar* pB = new TestChar;
    TestChar* pC = new TestChar;
    pA->WakeUp();       // Registered in this order, with the same time: they tick in this order.
    pB->WakeUp();
    pC->WakeUp();
    REQUIRE(IsPeriodicRegistered(pB));

    const CharTurnHookScope hook;
    TestChar* pNew = nullptr;
    s_fnOnCharTurn = [&pA, &pB, &pNew](CChar* pChar)
    {
        if (pChar == pA)
        {
            delete pB;
            pB = nullptr;
            // A char created now, and awake, likely takes the memory of B: if B was still in the batch, it would tick.
            //  Its own time doesn't expire in this pass.
            pNew = new TestChar;
            pNew->WakeUp();
        }
    };
    CWorldTestAccess::AdvanceTime(2000);
    CWorldTestAccess::Tick();
    CHECK(pB == nullptr);
    CHECK(s_vCharTurns == std::vector<const CChar*>{pA, pC});
    CHECK(IsPeriodicRegistered(pA));
    CHECK(IsPeriodicRegistered(pC));
    CHECK(IsPeriodicRegistered(pNew));

    delete pA;
    delete pC;
    delete pNew;
}

TEST_CASE("CWorldTicker a char put to sleep by the periodic tick of a char before it")
{
    CWorldTestAccess::AdvanceTime(1000);
    TestChar* pA = new TestChar;
    TestChar* pB = new TestChar;
    pA->WakeUp();
    pB->WakeUp();

    {
        const CharTurnHookScope hook;
        s_fnOnCharTurn = [pA, pB](CChar* pChar)
        {
            if (pChar == pA)
                pB->GoSleep();
        };
        CWorldTestAccess::AdvanceTime(2000);
        CWorldTestAccess::Tick();
        CHECK(s_vCharTurns == std::vector<const CChar*>{pA});
        CHECK(!IsPeriodicRegistered(pB));
    }

    // Registered again when woken up.
    pB->WakeUp();
    CHECK(IsPeriodicRegistered(pB));
    {
        const CharTurnHookScope hook;
        CWorldTestAccess::AdvanceTime(2000);
        CWorldTestAccess::Tick();
        CHECK(s_vCharTurns.size() == 2);
        CHECK(std::count(s_vCharTurns.begin(), s_vCharTurns.end(), pB) == 1);
    }

    delete pA;
    delete pB;
}

TEST_CASE("CWorldTicker a char destroyed by a TIMER, before its periodic tick in the same server tick")
{
    // The timers tick before the periodic ticks of the chars: a world save run by a @Timer destroys the deleted chars,
    //  and some of them may be due for their periodic tick in the same server tick.
    CWorldTestAccess::AdvanceTime(1000);
    TestChar* pA = new TestChar;
    TestChar* pB = new TestChar;
    pA->WakeUp();
    pB->WakeUp();
    REQUIRE(IsPeriodicRegistered(pA));

    int iTicks = 0;
    TestTimer timer(&iTicks);
    timer.SetTimeout(10);
    TestChar* pNew = nullptr;
    timer.fnOnTick = [&pA, &pNew]()
    {
        delete pA;
        pA = nullptr;
        // A char created now, and awake, likely takes the memory of A: if A was still registered, it would tick.
        //  Its own time doesn't expire in this pass.
        pNew = new TestChar;
        pNew->WakeUp();
    };

    const CharTurnHookScope hook;
    CWorldTestAccess::AdvanceTime(2000);
    CWorldTestAccess::Tick();
    CHECK(iTicks == 1);
    CHECK(pA == nullptr);
    CHECK(s_vCharTurns == std::vector<const CChar*>{pB});
    CHECK(IsPeriodicRegistered(pB));
    CHECK(IsPeriodicRegistered(pNew));

    delete pB;
    delete pNew;
}

TEST_CASE("CWorldTicker a char whose destructor runs isn't registered for the periodic ticks")
{
    // ~CChar removes the char from the periodic ticks, then runs triggers: @CombatDelete has the dying char as SRC, a script
    //  setting its STR to 0 would register it again (Stat_SetVal), and the wheel would keep a dangling pointer.
    TestChar* pA = new TestChar;
    REQUIRE(!IsPeriodicRegistered(pA));

    pA->SetDestructing(true);
    pA->Stat_SetVal(STAT_STR, 0);
    CHECK(!IsPeriodicRegistered(pA));

    // Otherwise it's registered, to tick and die.
    pA->SetDestructing(false);
    pA->Stat_SetVal(STAT_STR, 0);
    CHECK(IsPeriodicRegistered(pA));

    delete pA;
}

TEST_CASE("CWorldTicker a garbage collection run by a tick doesn't destroy the objects deleted or not yet placed")
{
    // As REMOVE then SERV.GARBAGE (or a world save, with SAVEGARBAGECOLLECT) in a @Timer: the code after the script still uses
    //  the object ticking, deleted by its own script. And the objects created but not yet placed, as a char being spawned.
    const RunModeScope runMode;
    CWorldTestAccess::AdvanceTime(1000);
    TestChar* pDeleted = new TestChar;
    TestChar* pUnplaced = new TestChar;     // The test chars are never placed in the world.
    REQUIRE(pDeleted->GetUID().IsValidUID());
    REQUIRE(g_World.FindUID(pUnplaced->GetUID().GetObjUID()) == pUnplaced);
    REQUIRE(g_World.IsObjIdle(pUnplaced));
    const dword uiObjs = CObjBase::sm_iCount;

    int iTicks = 0;
    TestTimer timer(&iTicks);
    timer.SetTimeout(10);
    bool fScheduled = false, fIdle = false;
    dword uiObjsInTick = 0;
    timer.fnOnTick = [&]()
    {
        pDeleted->Delete();
        CWorldTestAccess::CollectGarbage();
        fScheduled = g_World.IsScheduledObjDeletion(pDeleted);
        fIdle = g_World.IsObjIdle(pUnplaced);
        uiObjsInTick = CObjBase::sm_iCount;
    };

    CWorldTestAccess::AdvanceTime(1000);
    CWorldTestAccess::Tick();
    CHECK(iTicks == 1);
    CHECK(uiObjsInTick == uiObjs);      // Nothing destroyed.
    CHECK(fScheduled);
    CHECK(fIdle);

    // Destroyed after the tick, as by CWorld::_OnTick.
    CWorldTestAccess::DestroyScheduledDeletions();
    CHECK(CObjBase::sm_iCount == uiObjs - 1);

    delete pUnplaced;
}
