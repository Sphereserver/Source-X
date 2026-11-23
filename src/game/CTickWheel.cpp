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

    // Number of the first slot of 2^iShift ms that starts at or after uiTime (counted from time 0).
    constexpr uint64 FirstSlotFrom(uint64 uiTime, int iShift) noexcept
    {
        return (uiTime + ((1ull << iShift) - 1)) >> iShift;
    }

    // Start of the slot uiIdx of a level of uiSlots slots of 2^iShift ms, whose ring starts from the slot number uiFirst.
    constexpr uint64 RingSlotStart(uint64 uiFirst, uint32 uiIdx, uint32 uiSlots, int iShift) noexcept
    {
        return (uiFirst + ((uiIdx - (uint32)uiFirst) & (uiSlots - 1))) << iShift;
    }
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

        // Ordered by time, then by insertion order. In every slot the entries of the same time are in insertion order
        //  (_Cascade), and every level 0 slot is of a single time, except the current one, that also gets the entries
        //  already expired when placed.
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

// In every list, the entries of the same time are in insertion order, oldest first: the entries placed directly are
//  appended, the cascaded ones put in front (see _Cascade).
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

void CTickWheelCore::_LinkFront(Handle h, uint32 uiSlot) noexcept
{
    Node& node = _N(h);
    SlotList& list = _vSlots[uiSlot];
    node.uiSlot = uiSlot;
    node.uiPrev = kNone;
    node.uiNext = list.hHead;
    if (list.hHead != kNone)
        _N(list.hHead).uiPrev = h;
    else
    {
        list.hTail = h;
        _SetOccupied(uiSlot);
    }
    list.hHead = h;
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

// The level is the lowest one whose revolution is longer than the time left (less than 4096 ms: level 0, less than
//  2^24 ms: level 1, ...); the slot is given by the bits of the time for that level. A slot before the current one of
//  its level is of the next revolution (see _FindNextSlot).
uint32 CTickWheelCore::_SlotFor(int64 iTime) const noexcept
{
    // _iElapsed is never negative: it starts from 0 and only grows.
    const uint64 uiElapsed = (uint64)_iElapsed;
    if (iTime <= _iElapsed)
        return (uint32)(uiElapsed & (kWideSlots - 1));     // Already expired: current slot.

    uint64 uiTime = (uint64)iTime;
    uint64 uiLeft = uiTime - uiElapsed;
    if (uiLeft >= (1ull << kTotalBits))
    {
        // Farther than the whole range: parked at its end, placed again from there.
        uiLeft = (1ull << kTotalBits) - 1;
        uiTime = uiElapsed + uiLeft;
    }
    if (uiLeft < (1ull << kL1Shift))
        return (uint32)(uiTime & (kWideSlots - 1));
    if (uiLeft < (1ull << kLnShift))
        return kL1First + (uint32)((uiTime >> kL1Shift) & (kWideSlots - 1));
    const int iMsb   = 63 - std::countl_zero(uiLeft);
    const int iLevel = (iMsb - kLnShift) / kLnBits;
    const int iShift = kLnShift + (iLevel * kLnBits);
    return kLnFirst + ((uint32)iLevel * kLnSlots) + (uint32)((uiTime >> iShift) & (kLnSlots - 1));
}

void CTickWheelCore::_Place(Handle h) noexcept
{
    _Link(h, _SlotFor(_N(h).iTime));
}

// Next occupied slot in time order, and the time when it starts.
// Every level is a ring of slots, starting from its first slot that starts at or after the current time: the current
//  slot if the current time is its start, else the next one. The slots before it are of the next revolution.
//  Level 0 starts from the current slot, that also holds the entries already expired.
// When slots of more levels start at the same time, level 1 is taken first, then level 2 and up, then level 0:
//  see _Cascade.
bool CTickWheelCore::_FindNextSlot(uint32* puiSlot, int64* piSlotStart) const noexcept
{
    const uint64 uiElapsed = (uint64)_iElapsed;
    bool   fFound = false;
    uint32 uiBestSlot = 0;
    uint64 uiBestStart = 0;
    uint32 uiIdx;

    // Level 0. A slot before the first start of a level 1 slot comes before every slot of the upper levels.
    if (_bitsL0.FindFrom((uint32)(uiElapsed & (kWideSlots - 1)), &uiIdx) || _bitsL0.FindFrom(0, &uiIdx))
    {
        fFound = true;
        uiBestSlot = uiIdx;
        uiBestStart = RingSlotStart(uiElapsed, uiIdx, kWideSlots, 0);
        if (uiBestStart < FirstSlotFrom(uiElapsed, kL1Shift) << kL1Shift)
        {
            *puiSlot = uiBestSlot;
            *piSlotStart = (int64)uiBestStart;
            return true;
        }
    }

    // The upper levels, from the highest one: on a tie, the last one considered wins.
    auto fnConsider = [&fFound, &uiBestSlot, &uiBestStart](uint32 uiSlot, uint64 uiStart) noexcept
    {
        if (!fFound || (uiStart <= uiBestStart))
        {
            fFound = true;
            uiBestSlot = uiSlot;
            uiBestStart = uiStart;
        }
    };
    for (int iLevel = kUpperLevels - 1; iLevel >= 0; --iLevel)
    {
        const uint64 uiOcc = _uiOccLn[iLevel];
        if (uiOcc == 0)
            continue;
        const int    iShift  = kLnShift + (iLevel * kLnBits);
        const uint64 uiFirst = FirstSlotFrom(uiElapsed, iShift);
        const uint64 uiAfter = uiOcc & (~0ull << (uiFirst & (kLnSlots - 1)));
        uiIdx = (uint32)std::countr_zero((uiAfter != 0) ? uiAfter : uiOcc);
        fnConsider(kLnFirst + ((uint32)iLevel * kLnSlots) + uiIdx, RingSlotStart(uiFirst, uiIdx, kLnSlots, iShift));
    }
    {
        const uint64 uiFirst = FirstSlotFrom(uiElapsed, kL1Shift);
        if (_bitsL1.FindFrom((uint32)(uiFirst & (kWideSlots - 1)), &uiIdx) || _bitsL1.FindFrom(0, &uiIdx))
            fnConsider(kL1First + uiIdx, RingSlotStart(uiFirst, uiIdx, kWideSlots, kL1Shift));
    }

    if (!fFound)
        return false;
    *puiSlot = uiBestSlot;
    *piSlotStart = (int64)uiBestStart;
    return true;
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
            _vKeepBuffer.emplace_back(h);   // Not expected (they're all expired): placed again for the new time.
        _Unlink(h);
    }
}

// The entries are placed again for the current time, the start of the slot. Of two entries due at the same time, the
//  one set first is never on a lower level: it was farther from its time when placed, and both move down when the
//  current time reaches the start of their slot, to the level of the time left then. So the entries moved were set
//  before the ones of the same time in the slots they go to, and go in front of them: taken from the last one, each
//  in front of its new slot, their order is kept. For the same reason, when slots of more levels start at the same
//  time, the lowest level is cascaded first (_FindNextSlot): the entries of an upper one go in front of the ones it
//  moved.
void CTickWheelCore::_Cascade(uint32 uiSlot) noexcept
{
    Handle h = _vSlots[uiSlot].hTail;
    _vSlots[uiSlot] = SlotList{ kNone, kNone };
    _SetEmpty(uiSlot);
    while (h != kNone)
    {
        const Handle hPrev = _N(h).uiPrev;
        _LinkFront(h, _SlotFor(_N(h).iTime));
        h = hPrev;
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
