#include <doctest/doctest.h>
#include "../../src/game/CTickWheel.h"
#include <algorithm>
#include <climits>
#include <map>
#include <new>
#include <random>
#include <vector>

// Most of these cases were written by nightNR for the single level timing wheel of PR #1672, and adapted to the
//  hierarchical one: the overflow list cases became cases of the level boundaries and of the cascades.

namespace
{
    struct TestObj;
    using RefMap = std::multimap<int64, TestObj*>;
    using Wheel = CTickWheel<TestObj>;

    struct TestObj
    {
        int id = 0;
        Wheel::Handle hWheel = Wheel::kNone;
        bool fInRef = false;
        RefMap::iterator itRef;
    };

    // Register, or reschedule if already registered, as CWorldTicker does.
    void Set(Wheel& wheel, TestObj& obj, int64 iTime)
    {
        if (obj.hWheel == Wheel::kNone)
            obj.hWheel = wheel.Insert(&obj, iTime);
        else
            REQUIRE(wheel.Reschedule(obj.hWheel, &obj, iTime));
    }

    void Unset(Wheel& wheel, TestObj& obj)
    {
        REQUIRE(wheel.Erase(obj.hWheel, &obj));
        obj.hWheel = Wheel::kNone;
    }

    // Collect, and forget the handles of the collected objects, as CWorldTicker does.
    void CollectInto(Wheel& wheel, int64 iNow, std::vector<TestObj*>& vOut)
    {
        const size_t uiFirst = vOut.size();
        wheel.CollectExpired(iNow, vOut);
        for (size_t i = uiFirst; i < vOut.size(); ++i)
            vOut[i]->hWheel = Wheel::kNone;
    }

    // Reference: an ordered container, as the sorted lists used before the wheel.
    //  Entries with key < now are processed in key order, then insertion order.
    struct RefScheduler
    {
        RefMap m;

        void Set(TestObj& obj, int64 iTime)
        {
            if (obj.fInRef)
                m.erase(obj.itRef);
            obj.itRef = m.emplace(iTime, &obj);
            obj.fInRef = true;
        }
        void Unset(TestObj& obj)
        {
            REQUIRE(obj.fInRef);
            m.erase(obj.itRef);
            obj.fInRef = false;
        }
        void CollectExpired(int64 iNow, std::vector<TestObj*>& vOut)
        {
            for (auto it = m.begin(); (it != m.end()) && (it->first < iNow); )
            {
                it->second->fInRef = false;
                vOut.emplace_back(it->second);
                it = m.erase(it);
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
        CollectInto(wheel, iNow, v);
        return Ids(v);
    }

    // Run the wheel and the reference side by side with random operations, comparing every batch.
    void CompareWithReference(uint64 uiSeed, int64 iStart, int iSteps)
    {
        std::mt19937_64 rng(uiSeed);
        auto rand = [&rng](int64 iMin, int64 iMax) -> int64
        {
            return std::uniform_int_distribution<int64>(iMin, iMax)(rng);
        };

        Wheel wheel;
        RefScheduler ref;
        std::vector<TestObj> vObjs(300);
        for (int i = 0; i < (int)vObjs.size(); ++i)
            vObjs[(size_t)i].id = i;

        int64 iNow = iStart;
        size_t uiFired = 0;
        for (int iStep = 0; iStep < iSteps; ++iStep)
        {
            for (int iOp = (int)rand(0, 12); iOp > 0; --iOp)
            {
                TestObj& obj = vObjs[(size_t)rand(0, (int64)vObjs.size() - 1)];
                REQUIRE(obj.fInRef == (obj.hWheel != Wheel::kNone));
                const int64 iKind = rand(0, 99);
                if (obj.fInRef && (iKind < 40))
                {
                    Unset(wheel, obj);
                    ref.Unset(obj);
                    continue;
                }
                int64 iTime;
                const int64 iWhen = rand(0, 99);
                if (iWhen < 10)
                    iTime = iNow - rand(0, 500);                            // Already expired.
                else if (iWhen < 40)
                    iTime = iNow + rand(0, 200);                            // This or the next few ticks.
                else if (iWhen < 55)
                    iTime = iNow + rand(3'900, 4'300);                      // Around the end of the level 0 range.
                else if (iWhen < 75)
                    iTime = iNow + rand(0, 300'000);                        // Levels 1 and 2.
                else if (iWhen < 90)
                    iTime = iNow + rand(0, 20'000'000);                     // Up to level 3.
                else if (iWhen < 95)
                    iTime = iNow + rand(0, 1ll << 44);                      // Beyond the whole range.
                else
                    iTime = iNow + rand(0, 3) * 100;                        // Many equal times.
                Set(wheel, obj, iTime);
                ref.Set(obj, iTime);
            }

            const int64 iMove = rand(0, 999);
            if (iMove < 10)
                iNow -= rand(1, 2'000);                     // Clock going back.
            else if (iMove < 20)
                iNow += rand(4'000, 13'000);                // More than a level 0 revolution.
            else if (iMove < 25)
                iNow += rand(0, 30'000'000);                // Long pause.
            else if (iMove < 60)
                iNow += 0;                                  // Same ms.
            else
                iNow += rand(1, 80);

            std::vector<TestObj*> vWheel, vRef;
            CollectInto(wheel, iNow, vWheel);
            ref.CollectExpired(iNow, vRef);
            REQUIRE(Ids(vWheel) == Ids(vRef));
            REQUIRE(wheel.size() == ref.m.size());
            uiFired += vWheel.size();
        }
        CHECK(uiFired > 0);

        // Everything left fires, in the same order, once the time is far enough.
        std::vector<TestObj*> vWheel, vRef;
        iNow += (1ll << 46);
        CollectInto(wheel, iNow, vWheel);
        ref.CollectExpired(iNow, vRef);
        REQUIRE(Ids(vWheel) == Ids(vRef));
        CHECK(wheel.size() == 0);
        CHECK(ref.m.empty());
    }
}

TEST_CASE("CTickWheel handles")
{
    Wheel wheel;
    TestObj a, b;
    a.id = 1; b.id = 2;

    a.hWheel = wheel.Insert(&a, 100);
    CHECK(a.hWheel != Wheel::kNone);
    CHECK(wheel.GetTime(a.hWheel, &a) == 100);
    CHECK(wheel.GetTime(a.hWheel, &b) == 0);    // Not the owner.

    // Rescheduling keeps the handle.
    const Wheel::Handle hOld = a.hWheel;
    Set(wheel, a, 200);
    CHECK(a.hWheel == hOld);
    CHECK(wheel.GetTime(a.hWheel, &a) == 200);
    CHECK(wheel.size() == 1);

    // A stale handle, now used by another object, is refused: it can't change the timer of the other object.
    const Wheel::Handle hStale = a.hWheel;
    Unset(wheel, a);
    b.hWheel = wheel.Insert(&b, 300);
    CHECK(b.hWheel == hStale);      // Node reused.
    CHECK(wheel.GetTime(hStale, &a) == 0);
    CHECK(!wheel.Erase(hStale, &a));
    CHECK(!wheel.Reschedule(hStale, &a, 5));
    CHECK(!wheel.Erase(Wheel::kNone, &a));
    CHECK(!wheel.Erase(0xFFFF'FFF0u, &a));     // Beyond the pool.
    CHECK(wheel.GetTime(b.hWheel, &b) == 300);
    CHECK(wheel.size() == 1);
    CHECK(Collect(wheel, 301) == std::vector<int>{2});
}

TEST_CASE("CTickWheel expiration boundary and order")
{
    Wheel wheel;
    const int64 iBase = 10'000'000'000;
    CHECK(Collect(wheel, iBase).empty());

    TestObj a, b, c, d;
    a.id = 1; b.id = 2; c.id = 3; d.id = 4;
    Set(wheel, c, iBase + 500);
    Set(wheel, a, iBase + 500);
    Set(wheel, b, iBase + 499);
    Set(wheel, d, iBase + 510);
    CHECK(wheel.size() == 4);

    CHECK(Collect(wheel, iBase + 499).empty());                    // Expires when iTime < now.
    CHECK(Collect(wheel, iBase + 500) == std::vector<int>{2});
    CHECK(Collect(wheel, iBase + 501) == std::vector<int>{3, 1});  // Same time: insertion order.
    CHECK(Collect(wheel, iBase + 510).empty());
    CHECK(Collect(wheel, iBase + 2000) == std::vector<int>{4});
    CHECK(wheel.size() == 0);
    CHECK(a.hWheel == Wheel::kNone);
}

TEST_CASE("CTickWheel entries of the upper levels fire on time")
{
    for (const int64 iSpacing : {1'337ll, 97'531ll, 3'999'999ll})
    {
        CAPTURE(iSpacing);
        Wheel wheel;
        int64 iNow = 1'000'000;
        CHECK(Collect(wheel, iNow).empty());

        std::vector<TestObj> vObjs(50);
        std::vector<int64> vDue(vObjs.size());
        for (int i = 0; i < (int)vObjs.size(); ++i)
        {
            vObjs[(size_t)i].id = i;
            vDue[(size_t)i] = iNow + 7'000 + (int64)i * iSpacing;
            Set(wheel, vObjs[(size_t)i], vDue[(size_t)i]);
        }

        // Step of 7 ms, or of a random-ish size when the timers are far: each one must fire on the first call after it expired.
        std::vector<int64> vFiredAt(vObjs.size(), -1);
        int64 iPrev = iNow;
        for (int iStep = 0; (iStep < 2'000'000) && (wheel.size() > 0); ++iStep)
        {
            iPrev = iNow;
            iNow += (iSpacing > 100'000) ? (6'007 + (iStep % 5) * 1'000) : 7;
            std::vector<TestObj*> v;
            CollectInto(wheel, iNow, v);
            for (TestObj* pObj : v)
            {
                CAPTURE(pObj->id);
                CHECK(iNow > vDue[(size_t)pObj->id]);      // Not early...
                CHECK(iPrev <= vDue[(size_t)pObj->id]);    // ...and not in a later call.
                vFiredAt[(size_t)pObj->id] = iNow;
            }
        }
        CHECK(wheel.size() == 0);
        CHECK(std::find(vFiredAt.begin(), vFiredAt.end(), -1) == vFiredAt.end());
    }
}

TEST_CASE("CTickWheel jumps, time going back, clear")
{
    Wheel wheel;
    int64 iNow = 50'000;
    CHECK(Collect(wheel, iNow).empty());

    TestObj a, b, c;
    a.id = 1; b.id = 2; c.id = 3;
    Set(wheel, a, iNow + 100);
    Set(wheel, b, iNow + 1'000'000);     // Upper level.
    Set(wheel, c, iNow + 150);

    // Time going back: nothing expires, nothing is lost.
    CHECK(Collect(wheel, iNow - 10'000).empty());
    CHECK(wheel.size() == 3);

    CHECK(Collect(wheel, iNow + 200) == std::vector<int>{1, 3});

    // Jump of many revolutions.
    CHECK(Collect(wheel, iNow + 10'000'000) == std::vector<int>{2});
    CHECK(wheel.size() == 0);

    // Clear unregisters everything.
    Set(wheel, a, iNow + 10'000'100);
    Set(wheel, b, iNow + 90'000'000);
    int iCleared = 0;
    wheel.Clear([&iCleared](TestObj* pObj) { pObj->hWheel = Wheel::kNone; ++iCleared; });
    CHECK(iCleared == 2);
    CHECK(wheel.size() == 0);
    CHECK(Collect(wheel, iNow + 100'000'000).empty());
}

TEST_CASE("CTickWheel gives the same results of an ordered container (randomized)")
{
    // Start from 0, from a typical world clock, or just before the boundaries of the levels.
    const int64 iStarts[] = { 0, 5, 10'000'000'000ll, (1ll << 12) - 3, (1ll << 18) - 3, (1ll << 24) - 5,
                              (1ll << 30) - 2, (1ll << 36) - 4, (1ll << 42) - 7, -50 };
    for (uint64 uiSeed = 1; uiSeed <= 40; ++uiSeed)
    {
        CAPTURE(uiSeed);
        CompareWithReference(uiSeed, iStarts[uiSeed % std::size(iStarts)], 3'000);
    }
}

// ---- Edge cases of the wheel itself.

TEST_CASE("CTickWheel level boundaries")
{
    // Entries right before and after the end of each level range, with the clock just before it,
    //  must fire in time order and on the first call after they expired.
    for (const int iBits : {12, 18, 24, 30, 36, 42})
    {
        CAPTURE(iBits);
        Wheel wheel;
        RefScheduler ref;
        const int64 iBoundary = (1ll << iBits);
        int64 iNow = iBoundary - 3;
        CHECK(Collect(wheel, iNow).empty());

        std::vector<TestObj> vObjs(12);
        const int64 iOffsets[] = { -2, -1, 0, 1, 2, 4'095, 4'096, 4'097, 262'143, 262'144, 262'145, 3 };
        for (int i = 0; i < (int)vObjs.size(); ++i)
        {
            vObjs[(size_t)i].id = i;
            Set(wheel, vObjs[(size_t)i], iBoundary + iOffsets[i]);
            ref.Set(vObjs[(size_t)i], iBoundary + iOffsets[i]);
        }
        for (int iStep = 0; (iStep < 300'000) && (wheel.size() > 0); ++iStep)
        {
            iNow += 1 + (iStep % 3);
            std::vector<TestObj*> vWheel, vRef;
            CollectInto(wheel, iNow, vWheel);
            ref.CollectExpired(iNow, vRef);
            REQUIRE(Ids(vWheel) == Ids(vRef));
        }
        CHECK(wheel.size() == 0);
    }
}

TEST_CASE("CTickWheel time jumps around one revolution of each level")
{
    for (const int64 iJump : {4'095ll, 4'096ll, 4'097ll, 8'191ll, 262'143ll, 262'144ll, 262'145ll, 16'777'216ll})
    {
        CAPTURE(iJump);
        Wheel wheel;
        RefScheduler ref;
        int64 iNow = 1'000'000;
        CHECK(Collect(wheel, iNow).empty());

        std::vector<TestObj> vObjs(60);
        for (int i = 0; i < (int)vObjs.size(); ++i)
        {
            vObjs[(size_t)i].id = i;
            const int64 iTime = iNow + (int64)i * (iJump / 20 + 1);
            Set(wheel, vObjs[(size_t)i], iTime);
            ref.Set(vObjs[(size_t)i], iTime);
        }
        for (int iStep = 0; iStep < 6; ++iStep)
        {
            iNow += iJump + (iStep * 7);
            std::vector<TestObj*> vWheel, vRef;
            CollectInto(wheel, iNow, vWheel);
            ref.CollectExpired(iNow, vRef);
            REQUIRE(Ids(vWheel) == Ids(vRef));
        }
        CHECK(wheel.size() == ref.m.size());
    }
}

TEST_CASE("CTickWheel time not moving and going back")
{
    Wheel wheel;
    int64 iNow = 77'777;
    CHECK(Collect(wheel, iNow).empty());

    TestObj a, b;
    a.id = 1; b.id = 2;
    Set(wheel, a, iNow - 1);     // Expired.
    Set(wheel, b, iNow);         // Expires right after now.

    // Same ms, many calls: a fires once, b never.
    CHECK(Collect(wheel, iNow) == std::vector<int>{1});
    for (int i = 0; i < 10; ++i)
        CHECK(Collect(wheel, iNow).empty());

    // Clock back by more than a revolution: nothing fires, nothing is lost.
    CHECK(Collect(wheel, iNow - 100'000).empty());
    Set(wheel, a, iNow - 50'000);   // Inserted while the clock is back: in the past of the wheel time too.
    CHECK(Collect(wheel, iNow - 99'000).empty());
    CHECK(Collect(wheel, iNow - 49'999) == std::vector<int>{1});
    CHECK(Collect(wheel, iNow + 1) == std::vector<int>{2});
    CHECK(wheel.size() == 0);
}

TEST_CASE("CTickWheel negative and zero clock")
{
    Wheel wheel;
    CHECK(Collect(wheel, -5'000).empty());
    CHECK(Collect(wheel, 0).empty());

    TestObj a, b;
    a.id = 1; b.id = 2;
    Set(wheel, a, -10);
    Set(wheel, b, 10);
    CHECK(Collect(wheel, -9) == std::vector<int>{1});
    CHECK(Collect(wheel, 10).empty());
    CHECK(Collect(wheel, 11) == std::vector<int>{2});
}

TEST_CASE("CTickWheel many entries at close and equal times")
{
    Wheel wheel;
    CHECK(Collect(wheel, 0).empty());

    std::vector<TestObj> vObjs(5'000);
    std::vector<int64> vTime(vObjs.size());
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        vObjs[(size_t)i].id = i;
        // Times from 1000 to 1024 ms, inserted in reverse time order every other one; some of them in level 1.
        vTime[(size_t)i] = ((i % 7) ? 1'000 : 5'000) + ((i & 1) ? (24 - (i % 25)) : (i % 25));
        Set(wheel, vObjs[(size_t)i], vTime[(size_t)i]);
    }
    std::vector<TestObj*> vFired;
    CollectInto(wheel, 6'000, vFired);
    REQUIRE(vFired.size() == vObjs.size());
    for (size_t i = 1; i < vFired.size(); ++i)
    {
        const int64 iA = vTime[(size_t)vFired[i - 1]->id];
        const int64 iB = vTime[(size_t)vFired[i]->id];
        CHECK(((iA < iB) || ((iA == iB) && (vFired[i - 1]->id < vFired[i]->id))));
    }
}

TEST_CASE("CTickWheel the same object registered and removed many times")
{
    Wheel wheel;
    int64 iNow = 0;
    CHECK(Collect(wheel, iNow).empty());

    TestObj a, b;
    a.id = 1; b.id = 2;
    for (int i = 0; i < 10'000; ++i)
    {
        Set(wheel, a, iNow + (i % 500) * 37);
        if (i & 1)
            Unset(wheel, a);
        else
        {
            iNow += 1;
            std::vector<TestObj*> v;
            CollectInto(wheel, iNow, v);
            if (a.hWheel != Wheel::kNone)
                Unset(wheel, a);
        }
        CHECK(a.hWheel == Wheel::kNone);
    }
    CHECK(wheel.size() == 0);

    // Still working.
    Set(wheel, a, iNow + 5);
    Set(wheel, b, iNow + 5);
    CHECK(Collect(wheel, iNow + 6) == std::vector<int>{1, 2});
}


TEST_CASE("CTickWheel expired entries inserted again later")
{
    // The ticker drops the collected objects that are sleeping, and registers them again with their old, expired,
    //  timer when they wake up: they must fire on the next call, in expiration order, wherever the wheel time is.
    Wheel wheel;
    int64 iNow = 0;
    CHECK(Collect(wheel, iNow).empty());

    std::vector<TestObj> vObjs(30);
    std::vector<int64> vTime(vObjs.size());
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        vObjs[(size_t)i].id = i;
        vTime[(size_t)i] = (int64)i * 300;     // Up to 8.7 s, some in level 1.
        Set(wheel, vObjs[(size_t)i], vTime[(size_t)i]);
    }
    std::vector<int> vFired, vDropped;
    for (int iStep = 0; iStep < 600; ++iStep)
    {
        iNow += 20;
        if (iStep == 400)
        {
            // Woken up: registered again, in reverse order.
            for (auto it = vDropped.rbegin(); it != vDropped.rend(); ++it)
                Set(wheel, vObjs[(size_t)*it], vTime[(size_t)*it]);
            const std::vector<int> vWoken = Collect(wheel, iNow);
            CHECK(vWoken.size() == vDropped.size());
            CHECK(std::is_sorted(vWoken.begin(), vWoken.end()));
            vFired.insert(vFired.end(), vWoken.begin(), vWoken.end());
            vDropped.clear();
            continue;
        }
        for (int id : Collect(wheel, iNow))
        {
            if ((iStep < 400) && ((id % 3) == 0))
                vDropped.emplace_back(id);     // Sleeping.
            else
                vFired.emplace_back(id);
        }
    }
    CHECK(vFired.size() == vObjs.size());
    CHECK(wheel.size() == 0);
}


// ---- Game-like cases. Each object is an item or a char registered in the world ticking list.

TEST_CASE("CTickWheel removed before expiring (item deleted, TIMER=-1)")
{
    Wheel wheel;
    int64 iNow = 1'000'000;
    CHECK(Collect(wheel, iNow).empty());

    TestObj inL0, inCurrentSlot, inUpper, dueNow;
    inL0.id = 1; inCurrentSlot.id = 2; inUpper.id = 3; dueNow.id = 4;
    Set(wheel, inL0, iNow + 1'000);
    Set(wheel, inCurrentSlot, iNow + 1);
    Set(wheel, inUpper, iNow + 60'000);
    Set(wheel, dueNow, iNow - 5);     // Already expired, not yet collected.

    Unset(wheel, inL0);
    Unset(wheel, inCurrentSlot);
    Unset(wheel, inUpper);
    Unset(wheel, dueNow);
    CHECK(wheel.size() == 0);

    // Nothing fires later, even after several revolutions.
    for (int i = 0; i < 50; ++i)
    {
        iNow += 3'000;
        CHECK(Collect(wheel, iNow).empty());
    }
}

TEST_CASE("CTickWheel removed or rescheduled while waiting in an upper level")
{
    // An entry removed or moved while it waits for its cascade must not be lost, delayed or resurrected,
    //  and must not disturb the other entries of its slot.
    constexpr int kEntries = 60;
    for (int iVictim = 0; iVictim < kEntries; iVictim += 7)
    {
        for (const int iActAtStep : {1, 2, 3, 5, 8, 13})
        {
            for (const bool fReschedule : {false, true})
            {
                CAPTURE(iVictim);
                CAPTURE(iActAtStep);
                CAPTURE(fReschedule);
                Wheel wheel;
                int64 iNow = 0;
                CHECK(Collect(wheel, iNow).empty());

                std::vector<TestObj> vObjs(kEntries);
                std::vector<int64> vDue(vObjs.size());
                for (int i = 0; i < kEntries; ++i)
                {
                    vObjs[(size_t)i].id = i;
                    vDue[(size_t)i] = 4'200 + (int64)i * 37;   // Level 1, cascaded soon.
                    Set(wheel, vObjs[(size_t)i], vDue[(size_t)i]);
                }

                size_t uiFired = 0;
                for (int iStep = 1; (iStep < 10'000) && (wheel.size() > 0); ++iStep)
                {
                    if (iStep == iActAtStep)
                    {
                        TestObj& victim = vObjs[(size_t)iVictim];
                        REQUIRE(victim.hWheel != Wheel::kNone);
                        if (fReschedule)
                        {
                            vDue[(size_t)iVictim] = iNow + 2'000;
                            Set(wheel, victim, vDue[(size_t)iVictim]);
                        }
                        else
                        {
                            Unset(wheel, victim);
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
    int64 iNow = 500'000;
    CHECK(Collect(wheel, iNow).empty());

    TestObj a, b, c;
    a.id = 1; b.id = 2; c.id = 3;
    Set(wheel, a, iNow + 10);
    Set(wheel, b, iNow + 10);
    Set(wheel, c, iNow + 10);

    iNow += 11;
    std::vector<TestObj*> vFired;
    CollectInto(wheel, iNow, vFired);
    REQUIRE(Ids(vFired) == std::vector<int>{1, 2, 3});

    // While ticking: a sets TIMER=0 (due now), b sets a timer in the past, c sets a long timer.
    Set(wheel, a, iNow);
    Set(wheel, b, iNow - 1'000);
    Set(wheel, c, iNow + 600'000);

    // Same time: a is not expired yet (iTime == now), b is.
    CHECK(Collect(wheel, iNow) == std::vector<int>{2});
    CHECK(Collect(wheel, iNow + 1) == std::vector<int>{1});
    CHECK(Collect(wheel, iNow + 599'999).empty());
    CHECK(Collect(wheel, iNow + 600'001) == std::vector<int>{3});
}

TEST_CASE("CTickWheel rescheduled earlier and later (TIMER changed while pending)")
{
    Wheel wheel;
    CHECK(Collect(wheel, 0).empty());

    TestObj a, b;
    a.id = 1; b.id = 2;
    Set(wheel, a, 3'600'000);       // Upper level.
    Set(wheel, b, 1'000);           // Level 0.

    // a: upper level -> level 0, b: level 0 -> upper level.
    Set(wheel, a, 2'000);
    Set(wheel, b, 7'200'000);

    CHECK(Collect(wheel, 1'001).empty());       // Old time of b.
    CHECK(Collect(wheel, 2'001) == std::vector<int>{1});
    CHECK(Collect(wheel, 3'600'001).empty());   // Old time of a.
    CHECK(Collect(wheel, 7'200'001) == std::vector<int>{2});
}

TEST_CASE("CTickWheel same expiration time from an upper level and from level 0 keeps the insertion order")
{
    Wheel wheel;
    int64 iNow = 0;
    CHECK(Collect(wheel, iNow).empty());

    const int64 iDue = 20'000;
    TestObj early, late1, late2;
    early.id = 1; late1.id = 2; late2.id = 3;
    Set(wheel, early, iDue);        // Upper level now, cascaded later.

    while (iNow < iDue - 1'000)
    {
        iNow += 40;
        CHECK(Collect(wheel, iNow).empty());
    }
    Set(wheel, late2, iDue);        // Directly in level 0, inserted later.
    Set(wheel, late1, iDue);
    // Same time, so the order is the insertion one: early, late2, late1.
    CHECK(Collect(wheel, iDue + 1) == std::vector<int>{1, 3, 2});

    // Same, but all of them are in an upper level, cascaded together: the slots are lists in reverse insertion order.
    Set(wheel, early, iDue + 100'000);
    Set(wheel, late2, iDue + 100'000);
    Set(wheel, late1, iDue + 100'000);
    CHECK(Collect(wheel, iDue + 100'001) == std::vector<int>{1, 3, 2});
}

TEST_CASE("CTickWheel very far timers")
{
    Wheel wheel;
    int64 iNow = 1'000;
    CHECK(Collect(wheel, iNow).empty());

    TestObj zero, negative, farFuture;
    zero.id = 1; negative.id = 2; farFuture.id = 3;
    Set(wheel, zero, 0);
    Set(wheel, negative, -50'000);
    Set(wheel, farFuture, INT64_MAX / 2);   // Beyond the range of the wheel.
    CHECK(Collect(wheel, iNow + 1) == std::vector<int>{2, 1});
    CHECK(farFuture.hWheel != Wheel::kNone);
    CHECK(Collect(wheel, INT64_MAX / 4).empty());
    CHECK(Collect(wheel, INT64_MAX / 2 + 1) == std::vector<int>{3});
}

TEST_CASE("CTickWheel cleared (shutdown, resync) and used again")
{
    Wheel wheel;
    int64 iNow = 0;
    CHECK(Collect(wheel, iNow).empty());

    // More entries than a block of the pool, to free blocks on Clear.
    std::vector<TestObj> vObjs(10'000);
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        vObjs[(size_t)i].id = i;
        Set(wheel, vObjs[(size_t)i], (int64)i * 5);
    }
    iNow += 3;
    CHECK(Collect(wheel, iNow) == std::vector<int>{0});
    wheel.Clear([](TestObj* pObj) { pObj->hWheel = Wheel::kNone; });
    CHECK(wheel.size() == 0);

    // Registered again with new timers: everything works as before.
    for (int i = 0; i < (int)vObjs.size(); ++i)
    {
        CHECK(vObjs[(size_t)i].hWheel == Wheel::kNone);
        Set(wheel, vObjs[(size_t)i], iNow + 100 + (int64)i * 5);
    }
    std::vector<int> vFired;
    for (int iStep = 0; (iStep < 100'000) && (wheel.size() > 0); ++iStep)
    {
        iNow += 25;
        for (int id : Collect(wheel, iNow))
            vFired.emplace_back(id);
    }
    CHECK(vFired.size() == vObjs.size());
    CHECK(std::is_sorted(vFired.begin(), vFired.end()));
}

TEST_CASE("CTickWheel a collection that throws removes nothing")
{
    // The owners are given to the caller before the entries are removed: if that throws (out of memory), the wheel
    //  keeps them and the next call gives them again, in the same order. Two wheels get the same operations; the
    //  collections of the first one throw once, then are done again.
    struct Output
    {
        bool fThrow = false;
        std::vector<int> vIds;
    };
    auto fnOutput = [](void* pContext, void* const* ppOwners, size_t uiCount)
    {
        auto& out = *static_cast<Output*>(pContext);
        if (out.fThrow)
            throw std::bad_alloc();
        for (size_t i = 0; i < uiCount; ++i)
            out.vIds.emplace_back(static_cast<const TestObj*>(ppOwners[i])->id);
    };

    std::mt19937_64 rng(4242);
    auto rand = [&rng](int64 iMin, int64 iMax) -> int64
    {
        return std::uniform_int_distribution<int64>(iMin, iMax)(rng);
    };

    CTickWheelCore failing, plain;
    std::vector<TestObj> vObjs(400);
    std::vector<CTickWheelCore::Handle> vhFailing(vObjs.size(), CTickWheelCore::kNone), vhPlain(vObjs.size(), CTickWheelCore::kNone);
    std::vector<int64> viTime(vObjs.size(), 0);
    for (int i = 0; i < (int)vObjs.size(); ++i)
        vObjs[(size_t)i].id = i;

    int64 iNow = 0;
    int iThrown = 0;
    for (int iStep = 0; iStep < 400; ++iStep)
    {
        for (int iOp = (int)rand(0, 10); iOp > 0; --iOp)
        {
            const size_t i = (size_t)rand(0, (int64)vObjs.size() - 1);
            const int64 iTime = iNow + rand(-5, (int64)1 << rand(0, 30));    // Every level, and expired ones.
            viTime[i] = iTime;
            if (vhFailing[i] == CTickWheelCore::kNone)
            {
                vhFailing[i] = failing.Insert(&vObjs[i], iTime);
                vhPlain[i] = plain.Insert(&vObjs[i], iTime);
            }
            else
            {
                REQUIRE(failing.Reschedule(vhFailing[i], &vObjs[i], iTime));
                REQUIRE(plain.Reschedule(vhPlain[i], &vObjs[i], iTime));
            }
        }
        iNow += rand(0, (int64)1 << rand(0, 26));

        Output outFailing, outPlain;
        outFailing.fThrow = true;
        const size_t uiSize = failing.size();
        bool fThrown = false;
        try
        {
            failing.CollectExpired(iNow, fnOutput, &outFailing);
        }
        catch (const std::bad_alloc&)
        {
            fThrown = true;
        }
        // Not thrown: nothing expired, the output wasn't called.
        REQUIRE(failing.size() == uiSize);
        for (size_t i = 0; i < vObjs.size(); ++i)
        {
            if (vhFailing[i] != CTickWheelCore::kNone)
                REQUIRE(failing.GetTime(vhFailing[i], &vObjs[i]) == viTime[i]);
        }
        if (fThrown)
        {
            ++iThrown;
            outFailing.fThrow = false;
            failing.CollectExpired(iNow, fnOutput, &outFailing);
        }
        plain.CollectExpired(iNow, fnOutput, &outPlain);
        REQUIRE(outFailing.vIds == outPlain.vIds);
        CHECK(failing.size() == plain.size());
        for (int id : outPlain.vIds)
        {
            vhFailing[(size_t)id] = CTickWheelCore::kNone;
            vhPlain[(size_t)id] = CTickWheelCore::kNone;
        }
    }
    CHECK(iThrown > 100);
}
