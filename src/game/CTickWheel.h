/**
* @file CTickWheel.h
* @brief Timing wheel scheduler of the world timers (CWorldTicker). Its size is set by TimerWheelSlots in sphere.ini.
*/

#ifndef _INC_CTICKWHEEL_H
#define _INC_CTICKWHEEL_H

#include "../common/assertion.h"
#include "../common/datatypes.h"
#include <algorithm>
#include <vector>


// Intrusive link of an object in a CTickWheel: lives inside the scheduled object, so the wheel never allocates.
template <typename TOwner>
struct TickWheelHook
{
    TickWheelHook* pPrev  = nullptr;
    TickWheelHook* pNext  = nullptr;
    TOwner*        pOwner = nullptr;
    int64          iTime  = 0;      // When the entry expires (it's processed when iTime < now).
    uint64         uiSeq  = 0;      // Insertion order, to process entries with the same iTime in the order they were added.
    int32          iSlot  = -2;     // CTickWheel::kSlotNone, kSlotOverflow, or the slot index.
};

/*
* Timing wheel with an overflow bucket.
* - The wheel has N slots of kSlotMs each: it covers a window of N * kSlotMs from the current slot
*   (default 131072 slots, ~55 minutes).
* - Entries expiring inside the window are linked to their slot, later ones to the overflow bucket. Both are O(1),
*   insertion and removal never allocate (the link lives inside the object).
* - Each call to CollectExpired processes only the slots elapsed since the previous call, plus the current one.
* - The overflow bucket is scanned incrementally, so that it's fully visited at least every quarter of a wheel revolution:
*   entries entering the window are moved to their slot, way before they expire.
* - The entries collected by a call are returned ordered by expiration time, then by insertion order,
*   which is the same order given by an ordered container.
*/
template <typename TOwner>
class CTickWheel
{
public:
    using Hook = TickWheelHook<TOwner>;

    static constexpr int64 kSlotMs       = 25;
    static constexpr int64 kDefaultSlots = 1ll << 17;
    static constexpr int64 kMinSlots     = 1ll << 8;
    static constexpr int64 kMaxSlots     = 1ll << 22;
    static constexpr int32 kSlotNone     = -2;
    static constexpr int32 kSlotOverflow = -1;

    CTickWheel() :
        _pOverflowHead(nullptr), _pOverflowScanCursor(nullptr),
        _iSlots(0), _iSlotMask(0), _iCursor(0),
        _uiCount(0), _uiOverflowCount(0), _uiSeqNext(0)
    {
    }

    CTickWheel(const CTickWheel&) = delete;
    CTickWheel& operator=(const CTickWheel&) = delete;

    // Clamp to [kMinSlots, kMaxSlots] and round up to a power of 2.
    static int64 NormalizeSlotCount(int64 iSlots) noexcept
    {
        if (iSlots < kMinSlots)
            iSlots = kMinSlots;
        else if (iSlots > kMaxSlots)
            iSlots = kMaxSlots;
        int64 iPow2 = kMinSlots;
        while (iPow2 < iSlots)
            iPow2 <<= 1;
        return iPow2;
    }

    // Allocate the slots. Must be called once, before inserting anything.
    void Init(int64 iSlots)
    {
        ASSERT(_uiCount == 0);
        _iSlots = NormalizeSlotCount(iSlots);
        _iSlotMask = _iSlots - 1;
        _vSlots.assign((size_t)_iSlots, nullptr);
    }

    bool IsInitialized() const noexcept
    {
        return !_vSlots.empty();
    }
    int64 GetSlotCount() const noexcept
    {
        return _iSlots;
    }

    static bool IsLinked(const Hook& hook) noexcept
    {
        return hook.iSlot != kSlotNone;
    }

    size_t size() const noexcept
    {
        return _uiCount;
    }
    size_t overflow_size() const noexcept
    {
        return _uiOverflowCount;
    }

    void Insert(Hook& hook, TOwner* pOwner, int64 iTime)
    {
        ASSERT(IsInitialized());
        ASSERT(!IsLinked(hook));
        hook.pOwner = pOwner;
        hook.iTime  = iTime;
        hook.uiSeq  = _uiSeqNext++;
        _Place(hook);
        ++_uiCount;
    }

    void Erase(Hook& hook)
    {
        ASSERT(IsLinked(hook));
        _Unlink(hook);
        --_uiCount;
    }

    // Remove the expired entries (iTime < iNow) accepted by fnSelect(TOwner*) and append them to vOut,
    //  ordered by iTime, then by insertion order.
    // Rejected expired entries stay registered and will be checked again on the next call.
    template <typename TFunc>
    void CollectExpired(const int64 iNow, TFunc&& fnSelect, std::vector<TOwner*>& vOut)
    {
        if (!IsInitialized())
            return;

        _vCollectBuffer.clear();
        const int64 iNowSlot = _SlotOf(iNow);
        int64 iElapsedSlots = iNowSlot - _iCursor;
        bool fFullScan = false;
        if (iElapsedSlots < 0)
            iElapsedSlots = 0;          // Time went back: don't move the cursor, the current slot is still checked entry by entry.
        else if (iElapsedSlots >= _iSlots)
        {
            iElapsedSlots = _iSlots;    // More than a revolution elapsed (first tick, long pause): visit every slot once.
            fFullScan = true;
        }

        // Slots before the current one: everything in there is expired.
        for (int64 i = 0; i < iElapsedSlots; ++i)
        {
            Hook*& pHead = _vSlots[(size_t)((_iCursor + i) & _iSlotMask)];
            for (Hook* pHook = pHead; pHook != nullptr; )
            {
                Hook* pNext = pHook->pNext;
                _Unlink(*pHook);
                if ((pHook->iTime < iNow) && fnSelect(pHook->pOwner))
                {
                    --_uiCount;
                    _vCollectBuffer.emplace_back(pHook);
                }
                else
                {
                    _vKeepBuffer.emplace_back(pHook);   // Placed again after moving the cursor.
                }
                pHook = pNext;
            }
        }
        if (iNowSlot > _iCursor)
            _iCursor = iNowSlot;

        for (Hook* pHook : _vKeepBuffer)
            _Place(*pHook);
        _vKeepBuffer.clear();

        // Move the overflow entries entering the window to their slot.
        _ScanOverflow(fFullScan ? _uiOverflowCount : _GetOverflowScanQuota(iElapsedSlots));

        // Current slot: check entry by entry, it can hold entries not yet expired.
        for (Hook* pHook = _vSlots[(size_t)(_iCursor & _iSlotMask)]; pHook != nullptr; )
        {
            Hook* pNext = pHook->pNext;
            if ((pHook->iTime < iNow) && fnSelect(pHook->pOwner))
            {
                _Unlink(*pHook);
                --_uiCount;
                _vCollectBuffer.emplace_back(pHook);
            }
            pHook = pNext;
        }

        // The hooks are unlinked, but iTime and uiSeq are still valid.
        std::sort(_vCollectBuffer.begin(), _vCollectBuffer.end(),
            [](const Hook* pA, const Hook* pB) noexcept
            {
                return (pA->iTime != pB->iTime) ? (pA->iTime < pB->iTime) : (pA->uiSeq < pB->uiSeq);
            });
        vOut.reserve(vOut.size() + _vCollectBuffer.size());
        for (Hook* pHook : _vCollectBuffer)
            vOut.emplace_back(pHook->pOwner);
        _vCollectBuffer.clear();
    }

    // Call fn(TOwner*) for every entry, then unregister all of them.
    template <typename TFunc>
    void Clear(TFunc&& fn)
    {
        auto clearList = [&fn](Hook*& pHead)
        {
            for (Hook* pHook = pHead; pHook != nullptr; )
            {
                Hook* pNext = pHook->pNext;
                pHook->pPrev = pHook->pNext = nullptr;
                pHook->iSlot = kSlotNone;
                fn(pHook->pOwner);
                pHook = pNext;
            }
            pHead = nullptr;
        };
        for (Hook*& pHead : _vSlots)
            clearList(pHead);
        clearList(_pOverflowHead);
        _pOverflowScanCursor = nullptr;
        _uiCount = _uiOverflowCount = 0;
    }

private:
    static int64 _SlotOf(int64 iTime) noexcept
    {
        return (iTime < 0) ? 0 : (iTime / kSlotMs);
    }

    size_t _GetOverflowScanQuota(int64 iElapsedSlots) const noexcept
    {
        // Visit the whole overflow bucket at least every quarter revolution: an entry put there expires at least
        //  one full revolution later, so it reaches its slot well before expiring.
        const int64 iPassSlots = _iSlots / 4;
        const int64 iSlots = (iElapsedSlots > 0) ? iElapsedSlots : 1;
        return (size_t)(((int64)_uiOverflowCount * iSlots + iPassSlots - 1) / iPassSlots) + 8;
    }

    void _LinkHead(Hook& hook, Hook*& pHead, int32 iSlot) noexcept
    {
        hook.iSlot = iSlot;
        hook.pPrev = nullptr;
        hook.pNext = pHead;
        if (pHead != nullptr)
            pHead->pPrev = &hook;
        pHead = &hook;
    }

    void _Place(Hook& hook) noexcept
    {
        int64 iAbsSlot = _SlotOf(hook.iTime);
        if (iAbsSlot < _iCursor)
            iAbsSlot = _iCursor;    // Already expired: current slot, processed on the next call.

        if (iAbsSlot < _iCursor + _iSlots)
        {
            const int32 iSlot = (int32)(iAbsSlot & _iSlotMask);
            _LinkHead(hook, _vSlots[(size_t)iSlot], iSlot);
        }
        else
        {
            _LinkHead(hook, _pOverflowHead, kSlotOverflow);
            ++_uiOverflowCount;
        }
    }

    void _Unlink(Hook& hook) noexcept
    {
        Hook*& pHead = (hook.iSlot == kSlotOverflow) ? _pOverflowHead : _vSlots[(size_t)hook.iSlot];
        if (hook.iSlot == kSlotOverflow)
        {
            if (_pOverflowScanCursor == &hook)
                _pOverflowScanCursor = hook.pNext;
            --_uiOverflowCount;
        }
        if (hook.pPrev != nullptr)
            hook.pPrev->pNext = hook.pNext;
        else
            pHead = hook.pNext;
        if (hook.pNext != nullptr)
            hook.pNext->pPrev = hook.pPrev;
        hook.pPrev = hook.pNext = nullptr;
        hook.iSlot = kSlotNone;
    }

    void _ScanOverflow(size_t uiQuota) noexcept
    {
        const int64 iWindowEnd = _iCursor + _iSlots;
        while ((uiQuota-- > 0) && (_pOverflowHead != nullptr))
        {
            if (_pOverflowScanCursor == nullptr)
                _pOverflowScanCursor = _pOverflowHead;    // Start a new pass.

            Hook* pHook = _pOverflowScanCursor;
            _pOverflowScanCursor = pHook->pNext;
            if (_SlotOf(pHook->iTime) < iWindowEnd)
            {
                _Unlink(*pHook);
                _Place(*pHook);
            }
        }
    }

    std::vector<Hook*> _vSlots;         // Head of the list of each slot.
    std::vector<Hook*> _vKeepBuffer;
    std::vector<Hook*> _vCollectBuffer;
    Hook*  _pOverflowHead;
    Hook*  _pOverflowScanCursor;
    int64  _iSlots;
    int64  _iSlotMask;
    int64  _iCursor;                    // Absolute index (time / kSlotMs) of the current slot.
    size_t _uiCount;
    size_t _uiOverflowCount;
    uint64 _uiSeqNext;
};

#endif // _INC_CTICKWHEEL_H
