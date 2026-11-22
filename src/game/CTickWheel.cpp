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
    constexpr int    kL0Bits      = 12;
    constexpr int    kLnBits      = 6;
    constexpr int    kUpperLevels = 5;
    constexpr int    kTotalBits   = kL0Bits + (kLnBits * kUpperLevels);  // 42 bits of ms.
    constexpr uint32 kL0Slots     = 1u << kL0Bits;
    constexpr uint32 kLnSlots     = 1u << kLnBits;
    constexpr uint32 kSlotCount   = kL0Slots + (kLnSlots * kUpperLevels);
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
    _vHeads(kSlotCount, kNone),
    _uiOccL0{}, _uiOccL0Summary(0), _uiOccLn{},
    _uiPoolSize(1), _hFree(kNone), _iElapsed(0), _uiCount(0), _uiSeqNext(0)
{
    static_assert((kL0Bits == ::kL0Bits) && (kUpperLevels == ::kUpperLevels));
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
    try
    {
        if (iNow <= _iElapsed)
        {
            // Time didn't move or went back: only the current slot can hold expired entries
            //  (entries already expired when placed go to the current slot).
            _ScanInPlace((uint32)(_iElapsed & (kL0Slots - 1)), iNow);
        }
        else
        {
            uint32 uiSlot;
            int64  iSlotStart;
            while (_FindNextSlot(&uiSlot, &iSlotStart))
            {
                if (uiSlot < kL0Slots)
                {
                    if (iSlotStart >= iNow)
                        break;
                    _TakeL0(uiSlot, iNow);
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

        std::sort(_vCollectBuffer.begin(), _vCollectBuffer.end(),
            [](const Collected& a, const Collected& b) noexcept
            {
                return (a.iTime != b.iTime) ? (a.iTime < b.iTime) : (a.uiSeq < b.uiSeq);
            });
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
        for (Handle h = _vHeads[uiSlot]; h != kNone; )
        {
            Node& node = _N(h);
            const Handle hNext = node.uiNext;
            void* pOwner = node.pOwner;
            node.pOwner = nullptr;
            fnVisit(pContext, pOwner);
            h = hNext;
        }
        _vHeads[uiSlot] = kNone;
    }
    std::fill(std::begin(_uiOccL0), std::end(_uiOccL0), 0ull);
    std::fill(std::begin(_uiOccLn), std::end(_uiOccLn), 0ull);
    _uiOccL0Summary = 0;
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

void CTickWheelCore::_SetOccupied(uint32 uiSlot) noexcept
{
    if (uiSlot < kL0Slots)
    {
        _uiOccL0[uiSlot >> 6] |= (1ull << (uiSlot & 63));
        _uiOccL0Summary |= (1ull << (uiSlot >> 6));
    }
    else
    {
        const uint32 uiIdx = uiSlot - kL0Slots;
        _uiOccLn[uiIdx >> kLnBits] |= (1ull << (uiIdx & (kLnSlots - 1)));
    }
}

void CTickWheelCore::_SetEmpty(uint32 uiSlot) noexcept
{
    if (uiSlot < kL0Slots)
    {
        _uiOccL0[uiSlot >> 6] &= ~(1ull << (uiSlot & 63));
        if (_uiOccL0[uiSlot >> 6] == 0)
            _uiOccL0Summary &= ~(1ull << (uiSlot >> 6));
    }
    else
    {
        const uint32 uiIdx = uiSlot - kL0Slots;
        _uiOccLn[uiIdx >> kLnBits] &= ~(1ull << (uiIdx & (kLnSlots - 1)));
    }
}

void CTickWheelCore::_Link(Handle h, uint32 uiSlot) noexcept
{
    Node& node = _N(h);
    const Handle hHead = _vHeads[uiSlot];
    node.uiSlot = uiSlot;
    node.uiPrev = kNone;
    node.uiNext = hHead;
    if (hHead != kNone)
        _N(hHead).uiPrev = h;
    else
        _SetOccupied(uiSlot);
    _vHeads[uiSlot] = h;
}

void CTickWheelCore::_Unlink(Handle h) noexcept
{
    Node& node = _N(h);
    if (node.uiPrev != kNone)
        _N(node.uiPrev).uiNext = node.uiNext;
    else
    {
        _vHeads[node.uiSlot] = node.uiNext;
        if (node.uiNext == kNone)
            _SetEmpty((uint32)node.uiSlot);
    }
    if (node.uiNext != kNone)
        _N(node.uiNext).uiPrev = node.uiPrev;
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
    if (uiDiff < kL0Slots)
    {
        uiSlot = (uint32)(uiTime & (kL0Slots - 1));
    }
    else
    {
        const int iMsb   = 63 - std::countl_zero(uiDiff);
        const int iLevel = (iMsb - kL0Bits) / kLnBits;
        const int iShift = kL0Bits + (iLevel * kLnBits);
        uiSlot = kL0Slots + ((uint32)iLevel * kLnSlots) + (uint32)((uiTime >> iShift) & (kLnSlots - 1));
    }
    _Link(h, uiSlot);
}

// Next occupied slot in time order, and the time when it starts. Lower levels always come first.
bool CTickWheelCore::_FindNextSlot(uint32* puiSlot, int64* piSlotStart) const noexcept
{
    const uint64 uiElapsed = (uint64)_iElapsed;

    // Level 0: from the current slot to the end of the revolution.
    const uint32 uiCurL0 = (uint32)(uiElapsed & (kL0Slots - 1));
    uint32 uiWord = uiCurL0 >> 6;
    uint64 uiBits = _uiOccL0[uiWord] & (~0ull << (uiCurL0 & 63));
    if (uiBits == 0)
    {
        const uint64 uiSummary = (uiWord == 63) ? 0 : (_uiOccL0Summary & (~0ull << (uiWord + 1)));
        if (uiSummary != 0)
        {
            uiWord = (uint32)std::countr_zero(uiSummary);
            uiBits = _uiOccL0[uiWord];
        }
    }
    if (uiBits != 0)
    {
        *puiSlot = (uiWord << 6) + (uint32)std::countr_zero(uiBits);
        *piSlotStart = (int64)((uiElapsed & ~(uint64)(kL0Slots - 1)) | *puiSlot);
        return true;
    }

    // Upper levels: the slots after the current one of each level.
    for (int iLevel = 0; iLevel < kUpperLevels; ++iLevel)
    {
        const int iShift = kL0Bits + (iLevel * kLnBits);
        const uint32 uiCur = (uint32)((uiElapsed >> iShift) & (kLnSlots - 1));
        const uint64 uiLevelBits = (uiCur == kLnSlots - 1) ? 0 : (_uiOccLn[iLevel] & (~0ull << (uiCur + 1)));
        if (uiLevelBits != 0)
        {
            const uint32 uiIdx = (uint32)std::countr_zero(uiLevelBits);
            *puiSlot = kL0Slots + ((uint32)iLevel * kLnSlots) + uiIdx;
            const uint64 uiHighMask = ~((1ull << (iShift + kLnBits)) - 1);
            *piSlotStart = (int64)((uiElapsed & uiHighMask) | ((uint64)uiIdx << iShift));
            return true;
        }
    }
    return false;
}

CTickWheelCore::Handle CTickWheelCore::_Detach(uint32 uiSlot) noexcept
{
    const Handle h = _vHeads[uiSlot];
    _vHeads[uiSlot] = kNone;
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
    for (Handle h = _vHeads[uiSlot]; h != kNone; h = _vHeads[uiSlot])
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
    for (Handle h = _vHeads[uiSlot]; h != kNone; )
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
