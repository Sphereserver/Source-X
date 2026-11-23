#include "../common/assertion.h"
#include "CTickWheel.h"
#include <algorithm>
#include <bit>

#if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))
#   include <xmmintrin.h>
#   define TICKWHEEL_PREFETCH(p)    _mm_prefetch((const char*)(p), _MM_HINT_T0)
#elif defined(__GNUC__) || defined(__clang__)
#   define TICKWHEEL_PREFETCH(p)    __builtin_prefetch(p)
#else
#   define TICKWHEEL_PREFETCH(p)    (void)(p)
#endif


namespace
{
    // Slots: level 0, then level 1, then levels 2 to 4. Times are split in the same way: bits 0-11 select the slot of
    //  level 0, bits 12-23 the slot of level 1, bits 24-29, 30-35 and 36-41 the slots of levels 2, 3 and 4.
    constexpr int    kWideBits    = 12;                                     // Levels 0 and 1.
    constexpr int    kLnBits      = 6;                                      // Levels 2 to 4.
    constexpr int    kUpperLevels = 3;
    constexpr int    kL1Shift     = kWideBits;
    constexpr int    kLnShift     = 2 * kWideBits;                          // First bit of level 2.
    constexpr int    kTotalBits   = kLnShift + (kLnBits * kUpperLevels);    // 42 bits of ms.
    constexpr uint32 kWideSlots   = 1u << kWideBits;
    constexpr uint32 kLnSlots     = 1u << kLnBits;
    constexpr uint32 kL1First     = kWideSlots;                             // First slot of level 1...
    constexpr uint32 kLnFirst     = 2 * kWideSlots;                         // ...and of level 2.
    constexpr uint32 kSlotCount   = kLnFirst + (kLnSlots * kUpperLevels);
    constexpr uint32 kChunkBits   = 12;
    constexpr uint32 kChunkNodes  = 1u << kChunkBits;

    static_assert(kSlotCount <= 0xFFFF);
}

struct CTickWheelCore::Node
{
    int64   iTime;
    void*   pOwner;         // nullptr if the node is free.
    uint64  uiSeq  : 48;    // Insertion order, to keep the order of entries with the same time.
    uint64  uiSlot : 16;
    Handle  uiNext;
    Handle  uiPrev;
};

struct CTickWheelCore::Collected
{
    int64   iTime;
    uint64  uiSeq;
    void*   pOwner;
    Handle  h;              // Still allocated, unlinked: freed when the owners were delivered.
};


CTickWheelCore::CTickWheelCore() :
    _vSlots(kSlotCount, SlotList{ kNone, kNone }),
    _bitsL0{}, _bitsL1{}, _uiOccLn{},
    _uiPoolSize(1), _hFree(kNone), _iElapsed(0), _uiCount(0), _uiSeqNext(0)
{
    static_assert((kWideBits == ::kWideBits) && (kUpperLevels == ::kUpperLevels));
    static_assert((sizeof(void*) != 8) || (sizeof(Node) == 32));
    _vChunks.emplace_back(std::make_unique<Node[]>(kChunkNodes));
}

CTickWheelCore::~CTickWheelCore() = default;

CTickWheelCore::Node& CTickWheelCore::_N(Handle h) noexcept
{
    return _vChunks[h >> kChunkBits][h & (kChunkNodes - 1)];
}

const CTickWheelCore::Node& CTickWheelCore::_N(Handle h) const noexcept
{
    return _vChunks[h >> kChunkBits][h & (kChunkNodes - 1)];
}

bool CTickWheelCore::_IsOwnedBy(Handle h, const void* pOwner) const noexcept
{
    return (h != kNone) && (h < _uiPoolSize) && (pOwner != nullptr) && (_N(h).pOwner == pOwner);
}


// Public interface

CTickWheelCore::Handle CTickWheelCore::Insert(void* pOwner, int64 iTime)
{
    ASSERT(pOwner != nullptr);
    const Handle h = _Alloc();
    Node& node = _N(h);
    node.iTime = iTime;
    node.pOwner = pOwner;
    node.uiSeq = _uiSeqNext++;
    _Place(h);
    ++_uiCount;
    return h;
}

bool CTickWheelCore::Erase(Handle h, const void* pOwner) noexcept
{
    if (!_IsOwnedBy(h, pOwner))
        return false;   // No ASSERT here: it throws, and this is noexcept. The callers check the result.
    _Unlink(h);
    _Free(h);
    --_uiCount;
    return true;
}

bool CTickWheelCore::Reschedule(Handle h, const void* pOwner, int64 iTime) noexcept
{
    if (!_IsOwnedBy(h, pOwner))
        return false;   // No ASSERT here: it throws, and this is noexcept. The callers check the result.
    _Unlink(h);
    Node& node = _N(h);
    node.iTime = iTime;
    node.uiSeq = _uiSeqNext++;
    _Place(h);
    return true;
}

int64 CTickWheelCore::GetTime(Handle h, const void* pOwner) const noexcept
{
    return _IsOwnedBy(h, pOwner) ? _N(h).iTime : 0;
}

void CTickWheelCore::CollectExpired(const int64 iNow, OutputFn fnOutput, void* pContext)
{
    if (_uiCount == 0)
    {
        if (iNow > _iElapsed)
            _iElapsed = iNow;
        return;
    }

    // The expired entries are unlinked, but their nodes are freed only after the owners were delivered:
    //  if anything throws until then, they are placed again (_Restore).
    _vCollectBuffer.clear();
    _vKeepBuffer.clear();
    size_t uiFromCurrentSlot = 0;   // The first entries collected come from the current slot.
    try
    {
        if (iNow <= _iElapsed)
        {
            // Time didn't move or went back: only the current slot can hold expired entries
            //  (entries already expired when placed go to the current slot).
            _ScanInPlace((uint32)(_iElapsed & (kWideSlots - 1)), iNow);
            uiFromCurrentSlot = _vCollectBuffer.size();
        }
        else
        {
            const int64 iCurrentSlotStart = _iElapsed;
            uint32 uiSlot;
            int64  iSlotStart;
            while (_FindNextSlot(&uiSlot, &iSlotStart))
            {
                if (uiSlot < kL1First)
                {
                    if (iSlotStart >= iNow)
                        break;
                    _TakeL0(uiSlot, iNow);
                    if (iSlotStart == iCurrentSlotStart)
                        uiFromCurrentSlot = _vCollectBuffer.size();
                }
                else
                {
                    if (iSlotStart > iNow)
                        break;
                    _iElapsed = iSlotStart;
                    _Cascade(uiSlot);
                }
            }
            _iElapsed = iNow;
            for (const Handle h : _vKeepBuffer)
                _Place(h);
            _vKeepBuffer.clear();
        }

        if (_vCollectBuffer.empty())
            return;

        // Ordered by time, then by insertion order. The slots are lists in insertion order (_Link), and every level 0
        //  slot is of a single time, except the current one, that also gets the entries already expired when placed.
        //  So, the slots being collected in time order, only the entries of the current slot need sorting.
        //  The rare entries placed again out of their insertion order (parked far ones, the ones restored after
        //  an exception) are found by the check of the whole batch.
        auto fnLess = [](const Collected& a, const Collected& b) noexcept
        {
            return (a.iTime != b.iTime) ? (a.iTime < b.iTime) : (a.uiSeq < b.uiSeq);
        };
        const auto itBegin = _vCollectBuffer.begin(), itEnd = _vCollectBuffer.end();
        const auto itCurrentEnd = itBegin + (ptrdiff_t)uiFromCurrentSlot;
        if (!std::is_sorted(itBegin, itCurrentEnd, fnLess))
            std::sort(itBegin, itCurrentEnd, fnLess);
        if (!std::is_sorted(itBegin, itEnd, fnLess))
            std::sort(itBegin, itEnd, fnLess);
        _vOwners.clear();
        _vOwners.reserve(_vCollectBuffer.size());
        for (const Collected& entry : _vCollectBuffer)
            _vOwners.emplace_back(entry.pOwner);
        fnOutput(pContext, _vOwners.data(), _vOwners.size());
    }
    catch (...)
    {
        _Restore();
        throw;
    }

    // Delivered: nothing can fail from here.
    for (const Collected& entry : _vCollectBuffer)
        _Free(entry.h);
    _uiCount -= _vCollectBuffer.size();
    _vCollectBuffer.clear();
    _vOwners.clear();
}

void CTickWheelCore::Clear(VisitFn fnVisit, void* pContext)
{
    for (uint32 uiSlot = 0; uiSlot < kSlotCount; ++uiSlot)
    {
        for (Handle h = _vSlots[uiSlot].hHead; h != kNone; )
        {
            Node& node = _N(h);
            const Handle hNext = node.uiNext;
            void* pOwner = node.pOwner;
            node.pOwner = nullptr;
            fnVisit(pContext, pOwner);
            h = hNext;
        }
        _vSlots[uiSlot] = SlotList{ kNone, kNone };
    }
    _bitsL0 = {};
    _bitsL1 = {};
    std::fill(std::begin(_uiOccLn), std::end(_uiOccLn), 0ull);
    _vChunks.resize(1);
    _uiPoolSize = 1;
    _hFree = kNone;
    _uiCount = 0;
}


// Node pool

CTickWheelCore::Handle CTickWheelCore::_Alloc()
{
    if (_hFree != kNone)
    {
        const Handle h = _hFree;
        _hFree = _N(h).uiNext;
        return h;
    }
    if ((_uiPoolSize & (kChunkNodes - 1)) == 0)
    {
        // The nodes never move: no reallocation, no copy. Allocated before growing the vector, so that it's released
        //  if that throws.
        _vChunks.emplace_back(std::make_unique<Node[]>(kChunkNodes));
    }
    return _uiPoolSize++;   // Handle 0 is never given: it's the first node of the first block, never used.
}

void CTickWheelCore::_Free(Handle h) noexcept
{
    Node& node = _N(h);
    node.pOwner = nullptr;
    node.uiNext = _hFree;
    _hFree = h;
}


// Slots

void CTickWheelCore::SlotBits::Set(uint32 uiIdx) noexcept
{
    uiWords[uiIdx >> 6] |= (1ull << (uiIdx & 63));
    uiSummary |= (1ull << (uiIdx >> 6));
}

void CTickWheelCore::SlotBits::Reset(uint32 uiIdx) noexcept
{
    uiWords[uiIdx >> 6] &= ~(1ull << (uiIdx & 63));
    if (uiWords[uiIdx >> 6] == 0)
        uiSummary &= ~(1ull << (uiIdx >> 6));
}

bool CTickWheelCore::SlotBits::FindFrom(uint32 uiFrom, uint32* puiFound) const noexcept
{
    uint32 uiWord = uiFrom >> 6;
    uint64 uiBits = uiWords[uiWord] & (~0ull << (uiFrom & 63));
    if (uiBits == 0)
    {
        const uint64 uiNextWords = (uiWord == 63) ? 0 : (uiSummary & (~0ull << (uiWord + 1)));
        if (uiNextWords == 0)
            return false;
        uiWord = (uint32)std::countr_zero(uiNextWords);
        uiBits = uiWords[uiWord];
    }
    *puiFound = (uiWord << 6) + (uint32)std::countr_zero(uiBits);
    return true;
}

void CTickWheelCore::_SetOccupied(uint32 uiSlot) noexcept
{
    if (uiSlot < kL1First)
        _bitsL0.Set(uiSlot);
    else if (uiSlot < kLnFirst)
        _bitsL1.Set(uiSlot - kL1First);
    else
    {
        const uint32 uiIdx = uiSlot - kLnFirst;
        _uiOccLn[uiIdx >> kLnBits] |= (1ull << (uiIdx & (kLnSlots - 1)));
    }
}

void CTickWheelCore::_SetEmpty(uint32 uiSlot) noexcept
{
    if (uiSlot < kL1First)
        _bitsL0.Reset(uiSlot);
    else if (uiSlot < kLnFirst)
        _bitsL1.Reset(uiSlot - kL1First);
    else
    {
        const uint32 uiIdx = uiSlot - kLnFirst;
        _uiOccLn[uiIdx >> kLnBits] &= ~(1ull << (uiIdx & (kLnSlots - 1)));
    }
}

// Appended: every list is in insertion order, oldest first. The entries cascaded to a slot were all placed before
//  the ones placed there directly (they reach it when the slot starts being in range of its level), and they are
//  moved in their order: so the order of a list doesn't depend on the levels its entries went through.
void CTickWheelCore::_Link(Handle h, uint32 uiSlot) noexcept
{
    Node& node = _N(h);
    SlotList& list = _vSlots[uiSlot];
    node.uiSlot = uiSlot;
    node.uiPrev = list.hTail;
    node.uiNext = kNone;
    if (list.hTail != kNone)
        _N(list.hTail).uiNext = h;
    else
    {
        list.hHead = h;
        _SetOccupied(uiSlot);
    }
    list.hTail = h;
}

void CTickWheelCore::_Unlink(Handle h) noexcept
{
    // The slot is read only if the entry is at one end of its list.
    const Node& node = _N(h);
    if (node.uiPrev != kNone)
        _N(node.uiPrev).uiNext = node.uiNext;
    else
        _vSlots[node.uiSlot].hHead = node.uiNext;
    if (node.uiNext != kNone)
        _N(node.uiNext).uiPrev = node.uiPrev;
    else
        _vSlots[node.uiSlot].hTail = node.uiPrev;
    if ((node.uiPrev == kNone) && (node.uiNext == kNone))
        _SetEmpty((uint32)node.uiSlot);
}

void CTickWheelCore::_Place(Handle h) noexcept
{
    const Node& node = _N(h);
    // _iElapsed is never negative: it starts from 0 and only grows.
    const uint64 uiElapsed = (uint64)_iElapsed;
    uint64 uiTime = (node.iTime > _iElapsed) ? (uint64)node.iTime : uiElapsed;   // Already expired: current slot.
    const uint64 uiRangeEnd = uiElapsed | ((1ull << kTotalBits) - 1);
    if (uiTime > uiRangeEnd)
        uiTime = uiRangeEnd;    // Farther than the whole range: parked at its end, placed again from there.

    const uint64 uiDiff = uiTime ^ uiElapsed;
    uint32 uiSlot;
    if (uiDiff < (1ull << kL1Shift))
    {
        uiSlot = (uint32)(uiTime & (kWideSlots - 1));
    }
    else if (uiDiff < (1ull << kLnShift))
    {
        uiSlot = kL1First + (uint32)((uiTime >> kL1Shift) & (kWideSlots - 1));
    }
    else
    {
        const int iMsb   = 63 - std::countl_zero(uiDiff);
        const int iLevel = (iMsb - kLnShift) / kLnBits;
        const int iShift = kLnShift + (iLevel * kLnBits);
        uiSlot = kLnFirst + ((uint32)iLevel * kLnSlots) + (uint32)((uiTime >> iShift) & (kLnSlots - 1));
    }
    _Link(h, uiSlot);
}

// Next occupied slot in time order, and the time when it starts. Lower levels always come first.
bool CTickWheelCore::_FindNextSlot(uint32* puiSlot, int64* piSlotStart) const noexcept
{
    const uint64 uiElapsed = (uint64)_iElapsed;
    uint32 uiIdx;

    // Level 0: from the current slot to the end of the revolution.
    if (_bitsL0.FindFrom((uint32)(uiElapsed & (kWideSlots - 1)), &uiIdx))
    {
        *puiSlot = uiIdx;
        *piSlotStart = (int64)((uiElapsed & ~(uint64)(kWideSlots - 1)) | uiIdx);
        return true;
    }

    // Level 1: the slots after the current one (the entries of the current one are in level 0).
    const uint32 uiCurL1 = (uint32)((uiElapsed >> kL1Shift) & (kWideSlots - 1));
    if ((uiCurL1 != kWideSlots - 1) && _bitsL1.FindFrom(uiCurL1 + 1, &uiIdx))
    {
        *puiSlot = kL1First + uiIdx;
        *piSlotStart = (int64)((uiElapsed & ~((1ull << kLnShift) - 1)) | ((uint64)uiIdx << kL1Shift));
        return true;
    }

    // Levels 2 to 4: the slots after the current one of each level.
    for (int iLevel = 0; iLevel < kUpperLevels; ++iLevel)
    {
        const int iShift = kLnShift + (iLevel * kLnBits);
        const uint32 uiCur = (uint32)((uiElapsed >> iShift) & (kLnSlots - 1));
        const uint64 uiLevelBits = (uiCur == kLnSlots - 1) ? 0 : (_uiOccLn[iLevel] & (~0ull << (uiCur + 1)));
        if (uiLevelBits != 0)
        {
            uiIdx = (uint32)std::countr_zero(uiLevelBits);
            *puiSlot = kLnFirst + ((uint32)iLevel * kLnSlots) + uiIdx;
            const uint64 uiHighMask = ~((1ull << (iShift + kLnBits)) - 1);
            *piSlotStart = (int64)((uiElapsed & uiHighMask) | ((uint64)uiIdx << iShift));
            return true;
        }
    }
    return false;
}

CTickWheelCore::Handle CTickWheelCore::_Detach(uint32 uiSlot) noexcept
{
    const Handle h = _vSlots[uiSlot].hHead;
    _vSlots[uiSlot] = SlotList{ kNone, kNone };
    _SetEmpty(uiSlot);
    return h;
}


// Collecting

// Every step that can throw (adding to a buffer) is done before changing the wheel: if it throws, the entry is still
//  linked, and the ones already taken are in the buffers, for _Restore.

void CTickWheelCore::_Collect(Handle h, const Node& node)
{
    // The caller is going to read the owner soon: start loading it while the other entries are collected.
    TICKWHEEL_PREFETCH(node.pOwner);
    _vCollectBuffer.push_back({ node.iTime, node.uiSeq, node.pOwner, h });
}

void CTickWheelCore::_TakeL0(uint32 uiSlot, int64 iNow)
{
    for (Handle h = _vSlots[uiSlot].hHead; h != kNone; h = _vSlots[uiSlot].hHead)
    {
        const Node& node = _N(h);
        if (node.iTime < iNow)
            _Collect(h, node);
        else
            _vKeepBuffer.emplace_back(h);   // Parked far entry: placed again once the current time is updated.
        _Unlink(h);
    }
}

void CTickWheelCore::_Cascade(uint32 uiSlot) noexcept
{
    for (Handle h = _Detach(uiSlot); h != kNone; )
    {
        const Handle hNext = _N(h).uiNext;
        _Place(h);
        h = hNext;
    }
}

void CTickWheelCore::_ScanInPlace(uint32 uiSlot, int64 iNow)
{
    for (Handle h = _vSlots[uiSlot].hHead; h != kNone; )
    {
        const Node& node = _N(h);
        const Handle hNext = node.uiNext;
        if (node.iTime < iNow)
        {
            _Collect(h, node);
            _Unlink(h);
        }
        h = hNext;
    }
}

// The entries taken by a CollectExpired that didn't complete go back in the wheel, placed for its current time
//  (which may have moved forward): nothing is lost, the next call finds them.
void CTickWheelCore::_Restore() noexcept
{
    for (const Handle h : _vKeepBuffer)
        _Place(h);
    for (const Collected& entry : _vCollectBuffer)
        _Place(entry.h);
    _vKeepBuffer.clear();
    _vCollectBuffer.clear();
    _vOwners.clear();
}
