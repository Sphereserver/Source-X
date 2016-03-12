#ifndef _INC_CONTAINERS_H_
#define _INC_CONTAINERS_H_
#pragma once

#include <list>
// a thread-safe implementation of a queue container that doesn't use any locks
// this only works as long as there is only a single reader thread and writer thread

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
	ThreadSafeQueue();

private:
	ThreadSafeQueue(const ThreadSafeQueue& copy);
	ThreadSafeQueue& operator=(const ThreadSafeQueue& other);

public:
	// Append an element to the end of the queue (writer)
	void push(const T& value);

	// Erase elements from before reader head (writer)
	void clean(void);

	// Retrieve the number of elements in the queue (reader/writer)
	size_t size(void) const;

	// Determine if the queue is empty (reader/writer)
	bool empty(void) const;

	// Remove the first element from the queue (reader)
	void pop(void);

	// Retrieve the first element in the queue (reader)
	T front(void) const;
};

#endif // _INC_CONTAINERS_H_
