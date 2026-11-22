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
* - Level 0: 4096 slots of 1 ms (one revolution is ~4.1 s). Levels 1 to 5: 64 slots each, every level 64 times coarser
*   than the previous one. Together they cover 2^42 ms (~139 years); farther entries are parked at the end of that range
*   and placed again from there.
* - An entry goes to the level of the highest group of bits where its time differs from the current time of the wheel.
*   So every entry of a level expires after every entry of the lower levels, and an entry of an upper level is moved
*   down ("cascaded") when the current time reaches the start of its slot: at most once per level in its life,
*   never if it expires within the current level 0 revolution.
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
* - If the time didn't move or went back, nothing is cascaded: only the current level 0 slot is checked.
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

    using VisitFn = void (*)(void* pContext, void* pOwner);

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

    // Remove the expired entries (iTime < iNow) and append their owners to vOut, ordered by iTime,
    //  then by insertion order. Their handles are invalid afterwards.
    void CollectExpired(int64 iNow, std::vector<void*>& vOut);

    // Call fnVisit(pContext, pOwner) for every entry, then unregister all of them: every handle is invalid afterwards.
    void Clear(VisitFn fnVisit, void* pContext);

private:
    struct Node;
    struct Collected;

    static constexpr int    kL0Bits      = 12;
    static constexpr int    kUpperLevels = 5;
    static constexpr uint32 kL0Slots     = 1u << kL0Bits;

    Node& _N(Handle h) noexcept;
    const Node& _N(Handle h) const noexcept;
    bool _IsOwnedBy(Handle h, const void* pOwner) const noexcept;
    Handle _Alloc();
    void _Free(Handle h) noexcept;
    void _SetOccupied(uint32 uiSlot) noexcept;
    void _SetEmpty(uint32 uiSlot) noexcept;
    void _Link(Handle h, uint32 uiSlot) noexcept;
    void _Unlink(Handle h) noexcept;
    void _Place(Handle h) noexcept;
    bool _FindNextSlot(uint32* puiSlot, int64* piSlotStart) const noexcept;
    Handle _Detach(uint32 uiSlot) noexcept;
    void _TakeL0(uint32 uiSlot, int64 iNow);
    void _Cascade(uint32 uiSlot) noexcept;
    void _ScanInPlace(uint32 uiSlot, int64 iNow);
    void _Collect(const Node& node);

    std::vector<Handle> _vHeads;                    // First entry of each slot.
    uint64 _uiOccL0[kL0Slots / 64];                 // Occupied level 0 slots, one bit each...
    uint64 _uiOccL0Summary;                         // ...and one bit for each non zero word of _uiOccL0.
    uint64 _uiOccLn[kUpperLevels];                  // Occupied slots of the upper levels.
    std::vector<std::unique_ptr<Node[]>> _vChunks;  // Node pool, in blocks. Handle 0 is never used.
    std::vector<Handle>    _vKeepBuffer;
    std::vector<Collected> _vCollectBuffer;
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

    void CollectExpired(int64 iNow, std::vector<TOwner*>& vOut)
    {
        _vCollected.clear();
        _core.CollectExpired(iNow, _vCollected);
        vOut.reserve(vOut.size() + _vCollected.size());
        for (void* pOwner : _vCollected)
            vOut.emplace_back(static_cast<TOwner*>(pOwner));
        _vCollected.clear();
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
    std::vector<void*> _vCollected;
};

#endif // _INC_CTICKWHEEL_H
