/**
* @file containers.h
* @brief Sphere custom containers.
*/

#ifndef _INC_CONTAINERS_H
#define _INC_CONTAINERS_H

#include <atomic>
#include <deque>
#include <mutex>
//#include "../common/CException.h" // included in the precompiled header

// A thread-safe queue, written by one thread and read by another one (they can be different).
// Each operation takes a short lock: the previous lock-free implementation over a std::list was not safe even with a single
//  reader and a single writer, because the writer's clean() moved back the read position, so an element could be read twice.
// empty() and size() don't lock: they're polled often (e.g. CNetState::hasPendingData), so they read a counter kept by push and pop.

template<class T>
class ThreadSafeQueue
{
private:
    mutable std::mutex m_mutex;
    std::deque<T> m_queue;
    std::atomic<size_t> m_uiCount{0};

public:
    ThreadSafeQueue() = default;

    ThreadSafeQueue( const ThreadSafeQueue& copy ) = delete;
    ThreadSafeQueue& operator=( const ThreadSafeQueue& other ) = delete;

public:
    // Append an element to the end of the queue (writer)
    void push( const T& value )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.emplace_back( value );
        m_uiCount.store(m_queue.size(), std::memory_order_release);
    }

    // Release the memory of the elements already read (writer). The elements are now released when they are read.
    void clean( void ) noexcept
    {
    }

    // Retrieve the number of elements in the queue (reader/writer)
    size_t size( void ) const noexcept
    {
        return m_uiCount.load(std::memory_order_acquire);
    }

    // Determine if the queue is empty (reader/writer)
    bool empty( void ) const noexcept
    {
        return (m_uiCount.load(std::memory_order_acquire) == 0);
    }

    // Remove the first element from the queue (reader)
    void pop( void )
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if ( m_queue.empty() )
            throw CSError( LOGL_ERROR, 0, "No elements to read from queue." );
        m_queue.pop_front();
        m_uiCount.store(m_queue.size(), std::memory_order_release);
    }

    // Retrieve the first element in the queue (reader)
    T front( void ) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if ( m_queue.empty() )
            throw CSError( LOGL_ERROR, 0, "No elements to read from queue." );  // this should never happen
        return m_queue.front();
    }
};

#endif // _INC_CONTAINERS_H
