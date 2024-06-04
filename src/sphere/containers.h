/**
* @file containers.h
* @brief Sphere custom containers.
*/

#ifndef _INC_CONTAINERS_H
#define _INC_CONTAINERS_H

#include <list>
#include "../common/CException.h"
// a thread-safe implementation of a queue container that doesn't use any locks
// this only works as long as there is only a single reader thread and writer thread (can be different)


template<class T>
class ThreadSafeQueue
{
public:
	typedef std::list<T> list;
	typedef typename std::list<T>::iterator iterator;
	typedef typename std::list<T>::const_iterator const_iterator;

private:
	list m_list;
	iterator m_head;
	iterator m_tail;

public:
	ThreadSafeQueue() noexcept
	{
        m_list.emplace_back(T{}); // at least one element must be in the queue
		m_head = m_list.begin();
		m_tail = m_list.end();
	}

	ThreadSafeQueue( const ThreadSafeQueue& copy ) = delete;
	ThreadSafeQueue& operator=( const ThreadSafeQueue& other ) = delete;

public:
	// Append an element to the end of the queue (writer)
	void push( const T& value ) noexcept
	{
		m_list.emplace_back( value );
		m_tail = m_list.end();
		clean();
	}

	// Erase elements from before reader head (writer)
	void clean( void )
	{
        m_head = m_list.erase( m_list.begin(), m_head );
	}

	// Retrieve the number of elements in the queue (reader/writer)
	size_t size( void ) const
	{
		if ( empty() )
			return 0;

		size_t toSkip = 1;
		for ( const_iterator it = m_list.cbegin(), end = m_list.cend(); (it != m_head) && (it != end); ++it )
			++toSkip;

		return m_list.size() - toSkip;
	}

	// Determine if the queue is empty (reader/writer)
	bool empty( void ) const
	{
		iterator next = m_head;
		++next;

		return ( next == m_tail );
	}

	// Remove the first element from the queue (reader)
	void pop( void )
	{
		if ( empty() )
			throw CSError( LOGL_ERROR, 0, "No elements to read from queue." );

		iterator next = m_head;
		++next;

		if ( next != m_tail )
			m_head = next;
	}

	// Retrieve the first element in the queue (reader)
	T front( void ) const
	{
		if ( empty() == false )
		{
			iterator next = m_head;
			++next;

			if ( next != m_tail )
				return *next;
		}

		// this should never happen
		throw CSError( LOGL_ERROR, 0, "No elements to read from queue." );
	}
};

#endif // _INC_CONTAINERS_H
