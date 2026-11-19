#include <doctest/doctest.h>
#include "../../src/game/CTickWheel.h"
#include <algorithm>
#include <climits>
#include <cstring>
#include <map>
#include <random>
#include <vector>

namespace
{
    struct TestObj;
    using RefMap = std::multimap<int64, TestObj*>;

    struct TestObj
    {
        int id = 0;
        bool fSleeping = false;
        TickWheelHook<TestObj> hook;
        bool fInRef = false;
        RefMap::iterator itRef;
    };

    using Wheel = CTickWheel<TestObj>;

    bool SelectAwake(TestObj* pObj)
    {
        return !pObj->fSleeping;
    }

    // Reference: an ordered container, as the sorted lists used before the wheel.
    //  Entries with key < now are processed in key order, then insertion order; rejected ones stay registered.
    struct RefScheduler
    {
        RefMap m;

        void Insert(TestObj& obj, int64 iTime)
        {
            REQUIRE(!obj.fInRef);
            obj.itRef = m.emplace(iTime, &obj);
            obj.fInRef = true;
        }
        void Erase(TestObj& obj)
        {
            REQUIRE(obj.fInRef);
            m.erase(obj.itRef);
            obj.fInRef = false;
        }
        void CollectExpired(int64 iNow, std::vector<TestObj*>& vOut)
        {
            for (auto it = m.begin(); (it != m.end()) && (it->first < iNow); )
            {
                if (SelectAwake(it->second))
                {
                    it->second->fInRef = false;
                    vOut.emplace_back(it->second);
                    it = m.erase(it);
                }
                else
                {
                    ++it;
                }
            }
        }
    };

    std::vector<int> Ids(const std::vector<TestObj*>& v)
    {
        std::vector<int> vIds;
        vIds.reserve(v.size());
        for (const TestObj* pObj : v)
            vIds.emplace_back(pObj->id);
        return vIds;
    }

    std::vector<int> Collect(Wheel& wheel, int64 iNow)
    {
        std::vector<TestObj*> v;
        wheel.CollectExpired(iNow, SelectAwake, v);
        return Ids(v);
    }
}

TEST_CASE("CTickWheel slot count")
{
    CHECK(Wheel::NormalizeSlotCount(0) == Wheel::kMinSlots);
    CHECK(Wheel::NormalizeSlotCount(-5) == Wheel::kMinSlots);
    CHECK(Wheel::NormalizeSlotCount(256) == 256);
    CHECK(Wheel::NormalizeSlotCount(257) == 512);
    CHECK(Wheel::NormalizeSlotCount(131072) == 131072);
    CHECK(Wheel::NormalizeSlotCount(100000) == 131072);
    CHECK(Wheel::NormalizeSlotCount(1ll << 40) == Wheel::kMaxSlots);

    Wheel wheel;
    CHECK(!wheel.IsInitialized());
    std::vector<TestObj*> v;
    wheel.CollectExpired(1000, SelectAwake, v);   // Not initialized: no-op.
    CHECK(v.empty());
    wheel.Init(1000);
    CHECK(wheel.IsInitialized());
    CHECK(wheel.GetSlotCount() == 1024);
}

TEST_CASE("CTickWheel expiration boundary and order")
{
    Wheel wheel;
    wheel.Init(256);
    const int64 iBase = 10'000'000'000;   // Far from 0: the first call does a full pass.
    CHECK(Collect(wheel, iBase).empty());

    TestObj a, b, c, d;
    a.id = 1; b.id = 2; c.id = 3; d.id = 4;
    wheel.Insert(c.hook, &c, iBase + 500);
    wheel.Insert(a.hook, &a, iBase + 500);
    wheel.Insert(b.hook, &b, iBase + 499);
    wheel.Insert(d.hook, &d, iBase + 510);   // Same 25 ms slot as the others.
    CHECK(wheel.size() == 4);

    CHECK(Collect(wheel, iBase + 499).empty());                    // Expires when iTime < now.
    CHECK(Collect(wheel, iBase + 500) == std::vector<int>{2});
    CHECK(Collect(wheel, iBase + 501) == std::vector<int>{3, 1});  // Same time: insertion order.
    CHECK(Collect(wheel, iBase + 510).empty());
    CHECK(Collect(wheel, iBase + 2000) == std::vector<int>{4});
    CHECK(wheel.size() == 0);
    CHECK(!Wheel::IsLinked(a.hook));
}

TEST_CASE("CTickWheel overflow entries fire on time")
{
    Wheel wheel;
    wheel.Init(256);    // Window of 6.4 seconds.
    int64 iNow = 1'000'000;
    CHECK(Collect(wheel, iNow).empty());

    std::vector<TestObj> vObjs(50);
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        vObjs[i].id = i;
        wheel.Insert(vObjs[i].hook, &vObjs[i], iNow + 7'000 + (int64)i * 1'337);  // From 7 s to ~72 s: overflow.
    }
    CHECK(wheel.overflow_size() == vObjs.size());

    std::vector<int64> vFiredAt(vObjs.size(), -1);
    for (int iStep = 0; (iStep < 20'000) && (wheel.size() > 0); ++iStep)
    {
        iNow += 7;
        std::vector<TestObj*> v;
        wheel.CollectExpired(iNow, SelectAwake, v);
        for (TestObj* pObj : v)
            vFiredAt[(size_t)pObj->id] = iNow;
    }
    CHECK(wheel.size() == 0);
    CHECK(wheel.overflow_size() == 0);
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        const int64 iDue = 1'000'000 + 7'000 + (int64)i * 1'337;
        CAPTURE(i);
        CHECK(vFiredAt[(size_t)i] > iDue);          // Not early...
        CHECK(vFiredAt[(size_t)i] <= iDue + 7);     // ...and on the first call after it expired.
    }
}

TEST_CASE("CTickWheel jumps, time going back, rejected entries, clear")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 50'000;
    CHECK(Collect(wheel, iNow).empty());

    TestObj a, b, c;
    a.id = 1; b.id = 2; c.id = 3;
    wheel.Insert(a.hook, &a, iNow + 100);
    wheel.Insert(b.hook, &b, iNow + 1'000'000);     // Overflow.
    wheel.Insert(c.hook, &c, iNow + 150);

    // Time going back: nothing expires, nothing is lost.
    CHECK(Collect(wheel, iNow - 10'000).empty());
    CHECK(wheel.size() == 3);

    // Rejected entries stay registered.
    a.fSleeping = true;
    CHECK(Collect(wheel, iNow + 200) == std::vector<int>{3});
    CHECK(Wheel::IsLinked(a.hook));
    CHECK(Collect(wheel, iNow + 300).empty());
    a.fSleeping = false;
    CHECK(Collect(wheel, iNow + 301) == std::vector<int>{1});

    // Jump of many revolutions.
    CHECK(Collect(wheel, iNow + 10'000'000) == std::vector<int>{2});
    CHECK(wheel.size() == 0);

    // Clear unregisters everything.
    wheel.Insert(a.hook, &a, iNow + 10'000'100);
    wheel.Insert(b.hook, &b, iNow + 90'000'000);
    int iCleared = 0;
    wheel.Clear([&iCleared](TestObj*) { ++iCleared; });
    CHECK(iCleared == 2);
    CHECK(wheel.size() == 0);
    CHECK(wheel.overflow_size() == 0);
    CHECK(!Wheel::IsLinked(a.hook));
    CHECK(!Wheel::IsLinked(b.hook));
    CHECK(Collect(wheel, iNow + 100'000'000).empty());
}

TEST_CASE("CTickWheel gives the same results of an ordered container (randomized)")
{
    for (const int64 iSlots : {256ll, 4096ll})
    {
        for (uint64 uiSeed = 1; uiSeed <= 40; ++uiSeed)
        {
            CAPTURE(iSlots);
            CAPTURE(uiSeed);
            std::mt19937_64 rng(uiSeed);
            auto rand = [&rng](int64 iMin, int64 iMax) -> int64
            {
                return std::uniform_int_distribution<int64>(iMin, iMax)(rng);
            };

            Wheel wheel;
            wheel.Init(iSlots);
            RefScheduler ref;
            const int64 iWindow = iSlots * Wheel::kSlotMs;

            std::vector<TestObj> vObjs(300);
            for (int i = 0; i < (int)vObjs.size(); ++i)
                vObjs[i].id = i;

            int64 iNow = rand(0, 3) ? rand(1'000, 5'000'000'000) : 0;
            size_t uiFired = 0;
            for (int iStep = 0; iStep < 3'000; ++iStep)
            {
                // Some operations on random objects.
                for (int iOp = (int)rand(0, 12); iOp > 0; --iOp)
                {
                    TestObj& obj = vObjs[(size_t)rand(0, (int64)vObjs.size() - 1)];
                    REQUIRE(obj.fInRef == Wheel::IsLinked(obj.hook));
                    const int64 iKind = rand(0, 99);
                    if (iKind < 8)
                    {
                        obj.fSleeping = !obj.fSleeping;
                        continue;
                    }
                    if (obj.fInRef)
                    {
                        if (iKind < 40)
                            continue;
                        wheel.Erase(obj.hook);
                        ref.Erase(obj);
                        if (iKind < 70)
                            continue;   // Removed, else rescheduled.
                    }
                    int64 iTime;
                    const int64 iWhen = rand(0, 99);
                    if (iWhen < 10)
                        iTime = iNow - rand(0, 500);                    // Already expired.
                    else if (iWhen < 50)
                        iTime = iNow + rand(0, 200);                    // This or next few ticks.
                    else if (iWhen < 80)
                        iTime = iNow + rand(0, iWindow);                // Inside the window.
                    else if (iWhen < 97)
                        iTime = iNow + rand(iWindow, iWindow * 6);      // Overflow.
                    else
                        iTime = iNow + rand(0, 3) * Wheel::kSlotMs;     // Exactly on slot boundaries.
                    wheel.Insert(obj.hook, &obj, iTime);
                    ref.Insert(obj, iTime);
                }

                // Advance the clock.
                const int64 iMove = rand(0, 999);
                if (iMove < 3)
                    iNow -= rand(1, 2'000);                     // Clock going back.
                else if (iMove < 6)
                    iNow += rand(iWindow, iWindow * 4);         // Long pause.
                else if (iMove < 30)
                    iNow += 0;                                  // Same ms.
                else
                    iNow += rand(1, 80);

                std::vector<TestObj*> vWheel, vRef;
                wheel.CollectExpired(iNow, SelectAwake, vWheel);
                ref.CollectExpired(iNow, vRef);
                REQUIRE(Ids(vWheel) == Ids(vRef));
                REQUIRE(wheel.size() == ref.m.size());
                uiFired += vWheel.size();
            }
            CHECK(uiFired > 0);

            // Everything left fires, in the same order, once the time is far enough.
            for (TestObj& obj : vObjs)
                obj.fSleeping = false;
            std::vector<TestObj*> vWheel, vRef;
            iNow += iWindow * 10;
            wheel.CollectExpired(iNow, SelectAwake, vWheel);
            ref.CollectExpired(iNow, vRef);
            REQUIRE(Ids(vWheel) == Ids(vRef));
            CHECK(wheel.size() == 0);
            CHECK(ref.m.empty());
        }
    }
}

// ---- Edge cases of the wheel itself.

TEST_CASE("CTickWheel window boundary: last slot of the window and first overflow slot")
{
    Wheel wheel;
    wheel.Init(256);
    const int64 iSlots = wheel.GetSlotCount();
    int64 iNow = 100 * Wheel::kSlotMs;   // Cursor at slot 100.
    CHECK(Collect(wheel, iNow).empty());

    TestObj lastIn, firstOut;
    lastIn.id = 1; firstOut.id = 2;
    wheel.Insert(lastIn.hook, &lastIn, (100 + iSlots - 1) * Wheel::kSlotMs);    // Last slot of the window.
    wheel.Insert(firstOut.hook, &firstOut, (100 + iSlots) * Wheel::kSlotMs);    // Same index modulo the size: overflow.
    CHECK(wheel.overflow_size() == 1);
    CHECK(lastIn.hook.iSlot != firstOut.hook.iSlot);

    std::vector<int> vFired;
    for (int iStep = 0; (iStep < 2'000) && (wheel.size() > 0); ++iStep)
    {
        iNow += 13;
        for (int id : Collect(wheel, iNow))
            vFired.emplace_back(id);
    }
    CHECK(vFired == std::vector<int>{1, 2});
}

TEST_CASE("CTickWheel time jumps around one revolution, with overflow entries")
{
    for (const int64 iJumpSlots : {255ll, 256ll, 257ll, 511ll, 512ll, 1000ll})
    {
        CAPTURE(iJumpSlots);
        Wheel wheel;
        wheel.Init(256);
        RefScheduler ref;
        int64 iNow = 1'000'000;
        CHECK(Collect(wheel, iNow).empty());

        std::vector<TestObj> vObjs(60);
        for (int i = 0; i < (int)vObjs.size(); ++i)
        {
            vObjs[i].id = i;
            const int64 iTime = iNow + (int64)i * 250;     // From now to 2.3 revolutions: slots and overflow.
            wheel.Insert(vObjs[i].hook, &vObjs[i], iTime);
            ref.Insert(vObjs[i], iTime);
        }
        for (int iStep = 0; iStep < 6; ++iStep)
        {
            iNow += iJumpSlots * Wheel::kSlotMs + (iStep * 7);
            std::vector<TestObj*> vWheel, vRef;
            wheel.CollectExpired(iNow, SelectAwake, vWheel);
            ref.CollectExpired(iNow, vRef);
            REQUIRE(Ids(vWheel) == Ids(vRef));
        }
        CHECK(wheel.size() == ref.m.size());
    }
}

TEST_CASE("CTickWheel time not moving and going back")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 77'777;
    CHECK(Collect(wheel, iNow).empty());

    TestObj a, b;
    a.id = 1; b.id = 2;
    wheel.Insert(a.hook, &a, iNow - 1);     // Expired.
    wheel.Insert(b.hook, &b, iNow);         // Expires right after now.

    // Same ms, many calls: a fires once, b never.
    CHECK(Collect(wheel, iNow) == std::vector<int>{1});
    for (int i = 0; i < 10; ++i)
        CHECK(Collect(wheel, iNow).empty());

    // Clock back by more than a revolution: nothing fires, nothing is lost.
    CHECK(Collect(wheel, iNow - 100'000).empty());
    wheel.Insert(a.hook, &a, iNow - 50'000);   // Inserted while the clock is back: in the past of the cursor too.
    CHECK(Collect(wheel, iNow - 99'000).empty());
    CHECK(Collect(wheel, iNow - 49'999) == std::vector<int>{1});
    CHECK(Collect(wheel, iNow + 1) == std::vector<int>{2});
    CHECK(wheel.size() == 0);
}

TEST_CASE("CTickWheel negative and zero clock")
{
    Wheel wheel;
    wheel.Init(256);
    CHECK(Collect(wheel, -5'000).empty());
    CHECK(Collect(wheel, 0).empty());

    TestObj a, b;
    a.id = 1; b.id = 2;
    wheel.Insert(a.hook, &a, -10);
    wheel.Insert(b.hook, &b, 10);
    CHECK(Collect(wheel, -9) == std::vector<int>{1});
    CHECK(Collect(wheel, 10).empty());
    CHECK(Collect(wheel, 11) == std::vector<int>{2});
}

TEST_CASE("CTickWheel many entries in the same slot and at the same time")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 0;
    CHECK(Collect(wheel, iNow).empty());

    std::vector<TestObj> vObjs(5'000);
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        vObjs[i].id = i;
        // Same slot, times 0..24 ms in the slot, inserted in reverse time order every other one.
        const int64 iTime = 1'000 + ((i & 1) ? (24 - (i % 25)) : (i % 25));
        wheel.Insert(vObjs[i].hook, &vObjs[i], iTime);
    }
    std::vector<TestObj*> vFired;
    wheel.CollectExpired(2'000, SelectAwake, vFired);
    REQUIRE(vFired.size() == vObjs.size());
    for (size_t i = 1; i < vFired.size(); ++i)
    {
        const TestObj* pA = vFired[i - 1];
        const TestObj* pB = vFired[i];
        CHECK(((pA->hook.iTime < pB->hook.iTime) || ((pA->hook.iTime == pB->hook.iTime) && (pA->id < pB->id))));
    }
}

TEST_CASE("CTickWheel the same object registered and removed many times")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 0;
    CHECK(Collect(wheel, iNow).empty());

    TestObj a, b;
    a.id = 1; b.id = 2;
    for (int i = 0; i < 10'000; ++i)
    {
        wheel.Insert(a.hook, &a, iNow + (i % 500) * 37);
        if (i & 1)
            wheel.Erase(a.hook);
        else
        {
            iNow += 1;
            std::vector<TestObj*> v;
            wheel.CollectExpired(iNow, SelectAwake, v);
            if (Wheel::IsLinked(a.hook))
                wheel.Erase(a.hook);
        }
        CHECK(!Wheel::IsLinked(a.hook));
    }
    CHECK(wheel.size() == 0);
    CHECK(wheel.overflow_size() == 0);

    // Still working.
    wheel.Insert(a.hook, &a, iNow + 5);
    wheel.Insert(b.hook, &b, iNow + 5);
    CHECK(Collect(wheel, iNow + 6) == std::vector<int>{1, 2});
}

TEST_CASE("CTickWheel selection changing between calls")
{
    // An expired entry rejected (sleeping, being deleted) is checked again on each call, wherever it is.
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 0;
    CHECK(Collect(wheel, iNow).empty());

    std::vector<TestObj> vObjs(30);
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        vObjs[i].id = i;
        vObjs[i].fSleeping = (i % 3) == 0;
        wheel.Insert(vObjs[i].hook, &vObjs[i], (int64)i * 300);   // Up to 8.7 s, some in the overflow.
    }
    std::vector<int> vFired;
    for (int iStep = 0; iStep < 600; ++iStep)
    {
        iNow += 20;
        if (iStep == 400)
        {
            for (TestObj& obj : vObjs)
                obj.fSleeping = false;
        }
        for (int id : Collect(wheel, iNow))
            vFired.emplace_back(id);
    }
    CHECK(vFired.size() == vObjs.size());
    CHECK(wheel.size() == 0);
}


// ---- Game-like cases. Each object is an item or a char registered in the world ticking list.

TEST_CASE("CTickWheel removed before expiring (item deleted, TIMER=-1)")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 1'000'000;
    CHECK(Collect(wheel, iNow).empty());

    TestObj inSlot, inCurrentSlot, inOverflow, dueNow;
    inSlot.id = 1; inCurrentSlot.id = 2; inOverflow.id = 3; dueNow.id = 4;
    wheel.Insert(inSlot.hook, &inSlot, iNow + 1'000);
    wheel.Insert(inCurrentSlot.hook, &inCurrentSlot, iNow + 1);
    wheel.Insert(inOverflow.hook, &inOverflow, iNow + 60'000);
    wheel.Insert(dueNow.hook, &dueNow, iNow - 5);     // Already expired, not yet collected.
    CHECK(wheel.overflow_size() == 1);

    wheel.Erase(inSlot.hook);
    wheel.Erase(inCurrentSlot.hook);
    wheel.Erase(inOverflow.hook);
    wheel.Erase(dueNow.hook);
    CHECK(wheel.size() == 0);
    CHECK(wheel.overflow_size() == 0);
    for (const TestObj* pObj : {&inSlot, &inCurrentSlot, &inOverflow, &dueNow})
        CHECK(!Wheel::IsLinked(pObj->hook));

    // Nothing fires later, even after several revolutions.
    for (int i = 0; i < 50; ++i)
    {
        iNow += 3'000;
        CHECK(Collect(wheel, iNow).empty());
    }
}

TEST_CASE("CTickWheel removed or rescheduled while the overflow scan is in progress")
{
    // The incremental scan of the overflow bucket stops at some entry and continues from there on the next call.
    //  Removing that entry (item deleted: its memory is then freed or reused) or moving it to a slot (TIMER set again)
    //  must not lose, delay or resurrect any entry, and the wheel must not touch the removed entry anymore.
    //  We don't know where the scan stops: try each entry, after a different number of calls.
    constexpr int kEntries = 100;
    for (int iVictim = 0; iVictim < kEntries; ++iVictim)
    {
        for (const int iEraseAtStep : {1, 2, 3, 5, 8, 13})
        {
            for (const bool fReschedule : {false, true})
            {
                CAPTURE(iVictim);
                CAPTURE(iEraseAtStep);
                CAPTURE(fReschedule);
                Wheel wheel;
                wheel.Init(256);    // Window 6.4 s.
                int64 iNow = 0;
                CHECK(Collect(wheel, iNow).empty());

                std::vector<TestObj> vObjs(kEntries);
                std::vector<int64> vDue(vObjs.size());
                for (int i = 0; i < kEntries; ++i)
                {
                    vObjs[i].id = i;
                    vDue[(size_t)i] = 6'500 + (int64)i * 37;   // In the overflow, entering the window soon.
                    wheel.Insert(vObjs[i].hook, &vObjs[i], vDue[(size_t)i]);
                }

                size_t uiFired = 0;
                for (int iStep = 1; (iStep < 10'000) && (wheel.size() > 0); ++iStep)
                {
                    if (iStep == iEraseAtStep)
                    {
                        TestObj& victim = vObjs[(size_t)iVictim];
                        REQUIRE(Wheel::IsLinked(victim.hook));
                        wheel.Erase(victim.hook);
                        if (fReschedule)
                        {
                            vDue[(size_t)iVictim] = iNow + 2'000;
                            wheel.Insert(victim.hook, &victim, vDue[(size_t)iVictim]);
                        }
                        else
                        {
                            std::memset((void*)&victim.hook, 0xDD, sizeof(victim.hook));   // Freed memory.
                        }
                    }
                    iNow += 5;
                    for (int id : Collect(wheel, iNow))
                    {
                        CAPTURE(id);
                        REQUIRE((fReschedule || (id != iVictim)));
                        CHECK(iNow > vDue[(size_t)id]);         // Not early...
                        CHECK(iNow <= vDue[(size_t)id] + 5);    // ...nor late.
                        ++uiFired;
                    }
                }
                CHECK(wheel.size() == 0);
                CHECK(uiFired == (fReschedule ? vObjs.size() : vObjs.size() - 1));
            }
        }
    }
}

TEST_CASE("CTickWheel rescheduled from its own tick (TIMER set in @Timer)")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 500'000;
    CHECK(Collect(wheel, iNow).empty());

    TestObj a, b, c;
    a.id = 1; b.id = 2; c.id = 3;
    wheel.Insert(a.hook, &a, iNow + 10);
    wheel.Insert(b.hook, &b, iNow + 10);
    wheel.Insert(c.hook, &c, iNow + 10);

    iNow += 11;
    std::vector<TestObj*> vFired;
    wheel.CollectExpired(iNow, SelectAwake, vFired);
    REQUIRE(Ids(vFired) == std::vector<int>{1, 2, 3});
    for (TestObj* pObj : vFired)
        CHECK(!Wheel::IsLinked(pObj->hook));

    // While ticking: a sets TIMER=0 (due now), b sets a timer in the past, c sets a long timer.
    wheel.Insert(a.hook, &a, iNow);
    wheel.Insert(b.hook, &b, iNow - 1'000);
    wheel.Insert(c.hook, &c, iNow + 600'000);

    // Same time: a is not expired yet (iTime == now), b is.
    CHECK(Collect(wheel, iNow) == std::vector<int>{2});
    CHECK(Collect(wheel, iNow + 1) == std::vector<int>{1});
    CHECK(Collect(wheel, iNow + 599'999).empty());
    CHECK(Collect(wheel, iNow + 600'001) == std::vector<int>{3});
}

TEST_CASE("CTickWheel rescheduled earlier and later (TIMER changed while pending)")
{
    Wheel wheel;
    wheel.Init(256);
    CHECK(Collect(wheel, 0).empty());

    TestObj a, b;
    a.id = 1; b.id = 2;
    wheel.Insert(a.hook, &a, 3'600'000);        // Overflow.
    wheel.Insert(b.hook, &b, 1'000);            // Slot.
    CHECK(wheel.overflow_size() == 1);

    // a: overflow -> slot, b: slot -> overflow.
    wheel.Erase(a.hook);
    wheel.Insert(a.hook, &a, 2'000);
    wheel.Erase(b.hook);
    wheel.Insert(b.hook, &b, 7'200'000);
    CHECK(wheel.overflow_size() == 1);

    CHECK(Collect(wheel, 1'001).empty());       // Old time of b.
    CHECK(Collect(wheel, 2'001) == std::vector<int>{1});
    CHECK(Collect(wheel, 3'600'001).empty());   // Old time of a.
    CHECK(Collect(wheel, 7'200'001) == std::vector<int>{2});
}

TEST_CASE("CTickWheel sleeping objects (sector going to sleep) keep their timer")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 10'000;
    CHECK(Collect(wheel, iNow).empty());

    std::vector<TestObj> vObjs(20);
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        vObjs[i].id = i;
        vObjs[i].fSleeping = true;
        wheel.Insert(vObjs[i].hook, &vObjs[i], iNow + 100 + (int64)i * 2'000);  // Some of them in the overflow.
    }

    // Asleep for many revolutions: checked again on each call, never lost nor fired.
    for (int i = 0; i < 200; ++i)
    {
        iNow += 997;
        CHECK(Collect(wheel, iNow).empty());
        CHECK(wheel.size() == vObjs.size());
    }

    // Waking up: all of them fire on the next call, in expiration order.
    for (TestObj& obj : vObjs)
        obj.fSleeping = false;
    std::vector<int> vExpected(vObjs.size());
    for (int i = 0; i < (int)vObjs.size(); ++i)
        vExpected[(size_t)i] = i;
    CHECK(Collect(wheel, iNow + 1) == vExpected);
    CHECK(wheel.size() == 0);
}

TEST_CASE("CTickWheel same expiration time from slot and overflow keeps the insertion order")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 0;
    CHECK(Collect(wheel, iNow).empty());

    const int64 iDue = 20'000;
    TestObj early, late1, late2;
    early.id = 1; late1.id = 2; late2.id = 3;
    wheel.Insert(early.hook, &early, iDue);     // Overflow now, moved to its slot later.
    CHECK(wheel.overflow_size() == 1);

    while (iNow < iDue - 1'000)
    {
        iNow += 40;
        CHECK(Collect(wheel, iNow).empty());
    }
    CHECK(wheel.overflow_size() == 0);
    wheel.Insert(late2.hook, &late2, iDue);     // Directly in the slot, inserted later.
    wheel.Insert(late1.hook, &late1, iDue);
    // Same time, so the order is the insertion one: early, late2, late1.
    CHECK(Collect(wheel, iDue + 1) == std::vector<int>{1, 3, 2});
}

TEST_CASE("CTickWheel very far timers")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 1'000;
    CHECK(Collect(wheel, iNow).empty());

    TestObj zero, negative, farFuture;
    zero.id = 1; negative.id = 2; farFuture.id = 3;
    wheel.Insert(zero.hook, &zero, 0);
    wheel.Insert(negative.hook, &negative, -50'000);
    wheel.Insert(farFuture.hook, &farFuture, INT64_MAX / 2);
    CHECK(Collect(wheel, iNow + 1) == std::vector<int>{2, 1});
    CHECK(Wheel::IsLinked(farFuture.hook));
    CHECK(Collect(wheel, INT64_MAX / 4).empty());
    CHECK(Collect(wheel, INT64_MAX / 2 + 1) == std::vector<int>{3});
}

TEST_CASE("CTickWheel cleared (shutdown, resync) and used again")
{
    Wheel wheel;
    wheel.Init(256);
    int64 iNow = 0;
    CHECK(Collect(wheel, iNow).empty());

    std::vector<TestObj> vObjs(100);
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        vObjs[i].id = i;
        wheel.Insert(vObjs[i].hook, &vObjs[i], (int64)i * 500);  // Half in the overflow.
    }
    // Partially scanned overflow, then cleared.
    iNow += 30;
    CHECK(Collect(wheel, iNow) == std::vector<int>{0});
    wheel.Clear([](TestObj*) {});
    CHECK(wheel.size() == 0);
    CHECK(wheel.overflow_size() == 0);

    // Registered again with new timers: everything works as before.
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        CHECK(!Wheel::IsLinked(vObjs[(size_t)i].hook));
        wheel.Insert(vObjs[(size_t)i].hook, &vObjs[(size_t)i], iNow + 100 + (int64)i * 500);
    }
    std::vector<int> vFired;
    for (int iStep = 0; (iStep < 10'000) && (wheel.size() > 0); ++iStep)
    {
        iNow += 25;
        for (int id : Collect(wheel, iNow))
            vFired.emplace_back(id);
    }
    CHECK(vFired.size() == vObjs.size());
    CHECK(std::is_sorted(vFired.begin(), vFired.end()));
}
