/**
* @file CTickWheel.h
* @brief Hierarchical timing wheel scheduling the world timers (CWorldTicker).
*/

#ifndef _INC_CTICKWHEEL_H
#define _INC_CTICKWHEEL_H

#include "../common/datatypes.h"
#include <memory>
#include <vector>


/*
* Hierarchical timing wheel with 1 ms resolution. The implementation is in CTickWheel.cpp.
*
* Layout
* - Level 0: 4096 slots of 1 ms (one revolution is ~4.1 s). Level 1: 4096 slots of 4096 ms (one revolution is ~4.66 h).
*   Levels 2 to 4: 64 slots each, every level 64 times coarser than the previous one. Together they cover 2^42 ms
*   (~139 years); farther entries are parked at the end of that range and placed again from there.
* - Level 1 is as wide as level 0 so that the timers of minutes or hours (decay, spawns) are cascaded only once, and
*   in small groups: every 4096 ms, the entries of the next 4096 ms. With 64 slots, level 2 would start at ~4.4 minutes
*   instead of ~4.66 hours: every ~4.4 minutes, the entries of the next ~4.4 minutes would be moved all at once.
* - An entry goes to the lowest level whose revolution is longer than the time left: level 0 if it expires in less than
*   4096 ms, level 1 if in less than ~4.66 hours, and so on. Its slot is given by its time: every level is a ring,
*   the slots after the current one are of the current revolution, the ones before it of the next one.
* - An entry of an upper level is moved down ("cascaded") when the current time reaches the start of its slot:
*   a timer shorter than 4096 ms is never cascaded, one shorter than ~4.66 hours at most once. The entries cascaded
*   together are the ones due in the same slot, whatever the current time: no time boundary moves all the entries due
*   after it at once (as when the level is chosen by the highest bit where the time differs from the current one).
* - One bit per slot tells if the slot holds entries: the next occupied slot is found with a few bit scans,
*   whatever the elapsed time.
*
* Entries
* - The entries are 32 byte nodes in a pool made of blocks of 4096 nodes: the blocks are never moved,
*   freed nodes are reused. The scheduled object only stores the handle of its node (0 = not registered).
* - Insert, Erase and Reschedule take constant time. Insert allocates only when the number of pending entries
*   reaches a new maximum (one block).
* - A handle is valid until its entry is erased, collected or cleared: the node is then reused by other entries.
*   Erase, Reschedule and GetTime check that the node still belongs to the given owner, so that a stale handle can't
*   change the timer of another object.
* - The wheel never reads the owners: they are only stored and returned.
*
* Collecting
* - CollectExpired removes every expired entry (time < now) and returns them ordered by expiration time,
*   then by insertion order (rescheduling counts as a new insertion): the same order of an ordered container.
* - In every slot, the entries of the same time are in insertion order: the ones set directly are appended, the
*   cascaded ones were set before them and go in front. Every level 0 slot is of a single time (but the current one):
*   taking the slots in time order gives that order already, only the entries of the current slot may need sorting.
* - If the time didn't move or went back, nothing is cascaded: only the current level 0 slot is checked.
* - If it throws (out of memory), no entry is removed: their nodes are freed only after the owners were delivered.
*
* Not thread safe: the caller must lock it.
*
* CTickWheelCore works with untyped owners; CTickWheel<TOwner> is the typed interface to use.
*/
class CTickWheelCore
{
public:
    using Handle = uint32;
    static constexpr Handle kNone = 0;

    using VisitFn  = void (*)(void* pContext, void* pOwner);
    using OutputFn = void (*)(void* pContext, void* const* ppOwners, size_t uiCount);

    CTickWheelCore();
    ~CTickWheelCore();

    CTickWheelCore(const CTickWheelCore&) = delete;
    CTickWheelCore& operator=(const CTickWheelCore&) = delete;

    size_t size() const noexcept
    {
        return _uiCount;
    }

    // Register pOwner, expiring at iTime. Store the returned handle in the owner.
    [[nodiscard]]
    Handle Insert(void* pOwner, int64 iTime);

    // Unregister. The handle is invalid afterwards. Returns false (doing nothing) if the handle isn't of pOwner.
    bool Erase(Handle h, const void* pOwner) noexcept;

    // Change the expiration time, keeping the handle. Returns false (doing nothing) if the handle isn't of pOwner.
    bool Reschedule(Handle h, const void* pOwner, int64 iTime) noexcept;

    // Expiration time of the entry, or 0 if the handle isn't of pOwner.
    int64 GetTime(Handle h, const void* pOwner) const noexcept;

    // Remove the expired entries (iTime < iNow) and give their owners to fnOutput(pContext, ...), ordered by iTime,
    //  then by insertion order (not called if none expired). Their handles are invalid afterwards.
    // If anything throws (fnOutput too), no entry is removed: the wheel can be used again normally.
    //  fnOutput must not use the wheel.
    void CollectExpired(int64 iNow, OutputFn fnOutput, void* pContext);

    // Call fnVisit(pContext, pOwner) for every entry, then unregister all of them: every handle is invalid afterwards.
    void Clear(VisitFn fnVisit, void* pContext);

private:
    struct Node;
    struct Collected;

    static constexpr int    kWideBits    = 12;      // Levels 0 and 1: 4096 slots each.
    static constexpr int    kUpperLevels = 3;       // Levels 2 to 4: 64 slots each.

    // One bit per slot of a level of 4096 slots, and one bit for each non zero word.
    struct SlotBits
    {
        uint64 uiWords[(1u << kWideBits) / 64];
        uint64 uiSummary;

        void Set(uint32 uiIdx) noexcept;
        void Reset(uint32 uiIdx) noexcept;
        // The first occupied slot from uiFrom to the end of the level.
        bool FindFrom(uint32 uiFrom, uint32* puiFound) const noexcept;
    };

    Node& _N(Handle h) noexcept;
    const Node& _N(Handle h) const noexcept;
    bool _IsOwnedBy(Handle h, const void* pOwner) const noexcept;
    Handle _Alloc();
    void _Free(Handle h) noexcept;
    void _SetOccupied(uint32 uiSlot) noexcept;
    void _SetEmpty(uint32 uiSlot) noexcept;
    void _Link(Handle h, uint32 uiSlot) noexcept;
    void _LinkFront(Handle h, uint32 uiSlot) noexcept;
    void _Unlink(Handle h) noexcept;
    uint32 _SlotFor(int64 iTime) const noexcept;
    void _Place(Handle h) noexcept;
    bool _FindNextSlot(uint32* puiSlot, int64* piSlotStart) const noexcept;
    void _TakeL0(uint32 uiSlot, int64 iNow);
    void _Cascade(uint32 uiSlot) noexcept;
    void _ScanInPlace(uint32 uiSlot, int64 iNow);
    void _Collect(Handle h, const Node& node);
    void _Restore() noexcept;

    struct SlotList
    {
        Handle hHead;                               // Oldest entry...
        Handle hTail;                               // ...and newest one.
    };

    std::vector<SlotList> _vSlots;
    SlotBits _bitsL0;                               // Occupied slots of level 0...
    SlotBits _bitsL1;                               // ...of level 1...
    uint64 _uiOccLn[kUpperLevels];                  // ...and of levels 2 to 4.
    std::vector<std::unique_ptr<Node[]>> _vChunks;  // Node pool, in blocks. Handle 0 is never used.
    std::vector<Handle>    _vKeepBuffer;
    std::vector<Collected> _vCollectBuffer;
    std::vector<void*>     _vOwners;
    uint32 _uiPoolSize;
    Handle _hFree;
    int64  _iElapsed;                               // Current time of the wheel.
    size_t _uiCount;
    uint64 _uiSeqNext;
};


// Typed interface of CTickWheelCore.
template <typename TOwner>
class CTickWheel
{
public:
    using Handle = CTickWheelCore::Handle;
    static constexpr Handle kNone = CTickWheelCore::kNone;

    size_t size() const noexcept
    {
        return _core.size();
    }

    [[nodiscard]]
    Handle Insert(TOwner* pOwner, int64 iTime)
    {
        return _core.Insert(pOwner, iTime);
    }

    bool Erase(Handle h, const TOwner* pOwner) noexcept
    {
        return _core.Erase(h, pOwner);
    }

    bool Reschedule(Handle h, const TOwner* pOwner, int64 iTime) noexcept
    {
        return _core.Reschedule(h, pOwner, iTime);
    }

    int64 GetTime(Handle h, const TOwner* pOwner) const noexcept
    {
        return _core.GetTime(h, pOwner);
    }

    // Append the owners of the expired entries to vOut. If it throws (out of memory), nothing changed, vOut neither.
    void CollectExpired(int64 iNow, std::vector<TOwner*>& vOut)
    {
        _core.CollectExpired(iNow,
            [](void* pContext, void* const* ppOwners, size_t uiCount)
            {
                auto& vDest = *static_cast<std::vector<TOwner*>*>(pContext);
                vDest.reserve(vDest.size() + uiCount);  // The only step that can throw.
                for (size_t i = 0; i < uiCount; ++i)
                    vDest.emplace_back(static_cast<TOwner*>(ppOwners[i]));
            },
            &vOut);
    }

    // fnVisit: void(TOwner*).
    template <typename TFunc>
    void Clear(TFunc&& fnVisit)
    {
        _core.Clear(
            [](void* pContext, void* pOwner)
            {
                (*static_cast<std::remove_reference_t<TFunc>*>(pContext))(static_cast<TOwner*>(pOwner));
            },
            (void*)&fnVisit);
    }

private:
    CTickWheelCore _core;
};

#endif // _INC_CTICKWHEEL_H
