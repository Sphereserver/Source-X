#ifndef _INC_SOBJPOOL_H
#define _INC_SOBJPOOL_H

#include <array>
#include <cstddef>
#include <mutex>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <optional>
#include "sstacks.h"

namespace sl
{

// ObjectPool pre-constructs a fixed number of T objects stored in an internal array.
// It provides two interfaces: acquireUnique() returns a std::unique_ptr and
// acquireShared() returns a std::shared_ptr. When no free objects are available,
// the behavior depends on the AllowFallback template parameter:
// - If AllowFallback is true, a new, dynamically allocated object is returned.
// - Otherwise, a std::runtime_error is thrown.

template <typename PooledObject_t, size_t tp_pool_size, bool tp_allow_fallback = false>
// tp: template parameter
class ObjectPool
{
public:
    using index_t = uint32_t;   // do we really need size_t elements?
    static_assert(tp_pool_size <= std::numeric_limits<index_t>::max());

    static constexpr index_t sm_pool_size      = static_cast<index_t>(tp_pool_size);
    static constexpr bool    sm_allow_fallback = tp_allow_fallback;

    // Ensure that PooledObject_t is default constructible and destructible.
    static_assert(std::is_default_constructible_v<PooledObject_t>,
        "ObjectPool requires the type T to be default constructible");
    static_assert(std::is_destructible_v<PooledObject_t>,
        "ObjectPool requires the type T to be destructible");

    ObjectPool()
        : m_fallbackObjsCount(0)
    {
        // Initialize m_freeIndices: each index corresponds to an available object.
        for (index_t i = 0; i < sm_pool_size; ++i) {
            m_freeIndices.push(i);
        }
    }

    // Disable copy and move.
    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    // Custom deleter used for both pooled and fallback allocations.
    // The optional index indicates whether the object came from the pool.
    // When index is engaged, the object is one managed by the pool, and release() is called.
    // When not engaged, the deleter deletes the dynamically allocated object.
    struct PoolDeleter
    {
        ObjectPool* m_pool;
        std::optional<index_t> m_index; // Engaged if pooled

        void operator()(PooledObject_t* ptr) const
        {
            if (m_pool)
            {
                if (m_index.has_value())
                {
                    // Object is from the pool; return it.
                    m_pool->release(m_index.value());
                }
                else
                {
                    // Fallback allocation: decrement the fallback counter.
                    m_pool->fallbackReleased();
                    delete ptr;
                }
            }
            else
            {
                // Should not typically happen if pool pointer is always set.
                delete ptr;
            }
        }
    };

    // Type alias for unique_ptr using PoolDeleter.
    using UniquePtr_t = std::unique_ptr<PooledObject_t, PoolDeleter>;

    // Acquires an object and returns it as a unique_ptr.
    // If no pooled objects are available, either returns a fallback allocation or throws.
    [[nodiscard]]
    UniquePtr_t acquireUnique()
    {
        if (!m_freeIndices.empty()) [[likely]]
        {
            const auto idx = m_freeIndices.top();
            m_freeIndices.pop();
            PooledObject_t* ptr = &m_objects[idx];
            return UniquePtr_t(ptr, PoolDeleter {this, idx});
        }
        else [[unlikely]] if constexpr (sm_allow_fallback)
        {
            // Return a fallback (dynamically allocated) object.
            PooledObject_t* ptr = new PooledObject_t();
            m_fallbackObjsCount += 1;
            return UniquePtr_t(ptr, PoolDeleter {this, std::nullopt});
        }
        else
        {
            throw std::runtime_error("No free object available in pool");
        }
    }

    // Type alias for shared_ptr. The deleter goes in the constructor, unlike unique_ptr.
    using SharedPtr_t = std::shared_ptr<PooledObject_t>;

    // Acquires an object and returns it as a shared_ptr.
    // If no pooled objects are available, either returns a fallback allocation or throws.
    [[nodiscard]]
    SharedPtr_t acquireShared()
    {
        if (!m_freeIndices.empty()) [[likely]]
        {
            const auto idx = m_freeIndices.top();
            m_freeIndices.pop();
            PooledObject_t* ptr = &m_objects[idx];
            return std::shared_ptr<PooledObject_t>(ptr, PoolDeleter {this, idx});
        }
        else [[unlikely]] if constexpr (sm_allow_fallback)
        {
            // Return a fallback (dynamically allocated) object.
            PooledObject_t* ptr = new PooledObject_t();
            m_fallbackObjsCount += 1;
            return std::shared_ptr<PooledObject_t>(ptr, PoolDeleter {this, std::nullopt});
        }
        else
        {
            throw std::runtime_error("No free object available in pool");
        }
    }

    // Returns the current number of outstanding fallback allocations.
    [[nodiscard]]
    index_t getFallbackCount() const noexcept
    {
        return m_fallbackObjsCount;
    }

    // Static helper method to check whether an object acquired as a unique_ptr was from the pool.
    // Hides the internal PoolDeleter implementation details.
    [[nodiscard]]
    static bool isFromPool(const UniquePtr_t &ptr) noexcept
    {
        return ptr.get_deleter().m_index.has_value();
    }

    // Static helper method for shared_ptr.
    [[nodiscard]]
    static bool isFromPool(const std::shared_ptr<PooledObject_t> &ptr)
    {
        // Does std::get_deleter work only with RTTI enabled?
        if (auto deleterPtr = std::get_deleter<PoolDeleter>(ptr))
            return deleterPtr->m_index.has_value();
        return false;
    }

private:
    // Called by the custom deleter to return a pooled object to the m_freeIndices stack.
    void release(index_t index)
    {
        m_freeIndices.push(index);
    }

    // Called by the custom deleter when a fallback allocation is released.
    void fallbackReleased() noexcept
    {
        m_fallbackObjsCount -= 1;
    }

    // Pre-constructed objects stored in an array.
    std::array<PooledObject_t, sm_pool_size> m_objects{};

    // Stack tracked indices of available objects.
    sl::fixed_comptime_stack<index_t, sm_pool_size> m_freeIndices;

    // Track outstanding fallback objects.
    index_t m_fallbackObjsCount;
};


template <typename PooledObject_t, size_t tp_pool_size, bool tp_allow_fallback = false>
// tp: template parameter
// ts: thread safe
class TSObjectPool
{
public:
    using index_t = uint32_t;   // do we really need size_t elements?
    static constexpr index_t sm_pool_size      = static_cast<index_t>(tp_pool_size);
    static constexpr bool    sm_allow_fallback = tp_allow_fallback;

    // Ensure that PooledObject_t is default constructible and destructible.
    static_assert(std::is_default_constructible_v<PooledObject_t>,
        "ObjectPool requires the type T to be default constructible");
    static_assert(std::is_destructible_v<PooledObject_t>,
        "ObjectPool requires the type T to be destructible");

    TSObjectPool()
        : m_fallbackObjsCount(0)
    {
        // Initialize m_freeIndices: each index corresponds to an available object.
        for (index_t i = 0; i < sm_pool_size; ++i) {
            m_freeIndices.push(i);
        }
    }

    // Disable copy and move.
    TSObjectPool(const TSObjectPool&) = delete;
    TSObjectPool& operator=(const TSObjectPool&) = delete;

    // Custom deleter used for both pooled and fallback allocations.
    // The optional index indicates whether the object came from the pool.
    // When index is engaged, the object is one managed by the pool, and release() is called.
    // When not engaged, the deleter deletes the dynamically allocated object.
    struct PoolDeleter
    {
        TSObjectPool* m_pool;
        std::optional<index_t> m_index; // Engaged if pooled

        void operator()(PooledObject_t* ptr) const
        {
            if (m_pool)
            {
                if (m_index.has_value())
                {
                    // Object is from the pool; return it.
                    m_pool->release(m_index.value());
                }
                else
                {
                    // Fallback allocation: decrement the fallback counter.
                    m_pool->fallbackReleased();
                    delete ptr;
                }
            }
            else
            {
                // Should not typically happen if pool pointer is always set.
                delete ptr;
            }
        }
    };

    // Type alias for unique_ptr using PoolDeleter.
    using UniquePtr_t = std::unique_ptr<PooledObject_t, PoolDeleter>;

    // Acquires an object and returns it as a unique_ptr.
    // If no pooled objects are available, either returns a fallback allocation or throws.
    [[nodiscard]]
    UniquePtr_t acquireUnique()
    {
        std::lock_guard lock(m_mtx);
        if (!m_freeIndices.empty()) [[likely]]
        {
            const auto idx = m_freeIndices.top();
            m_freeIndices.pop();
            PooledObject_t* ptr = &m_objects[idx];
            return UniquePtr_t(ptr, PoolDeleter {this, idx});
        }
        else [[unlikely]] if constexpr (sm_allow_fallback)
        {
            // Return a fallback (dynamically allocated) object.
            PooledObject_t* ptr = new PooledObject_t();
            m_fallbackObjsCount.fetch_add(1, std::memory_order_relaxed);
            return UniquePtr_t(ptr, PoolDeleter {this, std::nullopt});
        }
        else
        {
            throw std::runtime_error("No free object available in pool");
        }
    }

    // Type alias for shared_ptr. The deleter goes in the constructor, unlike unique_ptr.
    using SharedPtr_t = std::shared_ptr<PooledObject_t>;

    // Acquires an object and returns it as a shared_ptr.
    // If no pooled objects are available, either returns a fallback allocation or throws.
    [[nodiscard]]
    SharedPtr_t acquireShared()
    {
        std::lock_guard lock(m_mtx);
        if (!m_freeIndices.empty()) [[likely]]
        {
            const auto idx = m_freeIndices.top();
            m_freeIndices.pop();
            PooledObject_t* ptr = &m_objects[idx];
            return std::shared_ptr<PooledObject_t>(ptr, PoolDeleter {this, idx});
        }
        else [[unlikely]] if constexpr (sm_allow_fallback)
        {
            // Return a fallback (dynamically allocated) object.
            PooledObject_t* ptr = new PooledObject_t();
            m_fallbackObjsCount.fetch_add(1, std::memory_order_relaxed);
            return std::shared_ptr<PooledObject_t>(ptr, PoolDeleter {this, std::nullopt});
        }
        else
        {
            throw std::runtime_error("No free object available in pool");
        }
    }

    // Returns the current number of outstanding fallback allocations.
    [[nodiscard]]
    index_t getFallbackCount() const noexcept
    {
        return m_fallbackObjsCount.load(std::memory_order_relaxed);
    }

    // Static helper method to check whether an object acquired as a unique_ptr was from the pool.
    // Hides the internal PoolDeleter implementation details.
    [[nodiscard]]
    static bool isFromPool(const UniquePtr_t &ptr) noexcept
    {
        return ptr.get_deleter().m_index.has_value();
    }

    // Static helper method for shared_ptr.
    [[nodiscard]]
    static bool isFromPool(const std::shared_ptr<PooledObject_t> &ptr)
    {
        if (auto deleterPtr = std::get_deleter<PoolDeleter>(ptr))
            return deleterPtr->m_index.has_value();
        return false;
    }

private:
    // Called by the custom deleter to return a pooled object to the m_freeIndices stack.
    void release(index_t index)
    {
        std::lock_guard lock(m_mtx);
        m_freeIndices.push(index);
    }

    // Called by the custom deleter when a fallback allocation is released.
    void fallbackReleased() noexcept
    {
        m_fallbackObjsCount.fetch_sub(1, std::memory_order_relaxed);
    }

    // Pre-constructed objects stored in an array.
    std::array<PooledObject_t, sm_pool_size> m_objects{};

    // Stack tracked indices of available objects.
    sl::fixed_comptime_stack<index_t, sm_pool_size> m_freeIndices;

    // Mutex to ensure thread safety.
    mutable std::mutex m_mtx;

    // Atomic counter tracking outstanding fallback objects.
    mutable std::atomic<index_t> m_fallbackObjsCount;
};


} // namespace sl

#endif // _INC_SOBJPOOL_H
