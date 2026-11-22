#include <doctest/doctest.h>
#include "../../src/game/CWorld.h"
#include "../../src/game/CWorldGameTime.h"
#include "../../src/game/CWorldTickingList.h"
#include <functional>

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
