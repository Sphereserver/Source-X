
#include "../common/CException.h"
#include "../common/common.h"
#include "containers.h"


template<class T>
ThreadSafeQueue<T>::ThreadSafeQueue()
{
	m_list.push_back(T()); // at least one element must be in the queue
	m_head = m_list.begin();
	m_tail = m_list.end();
}

template<class T> 
void ThreadSafeQueue<T>::push(const T& value)
{
	m_list.push_back(value);
	m_tail = m_list.end();
	clean();
}

// Erase elements from before reader head (writer)
template<class T> 
void ThreadSafeQueue<T>::clean(void)
{
	m_list.erase(m_list.begin(), m_head);
}

template<class T> 
size_t ThreadSafeQueue<T>::size(void) const
{
	if (empty())
		return 0;

	size_t toSkip = 1;
	for (const_iterator it = m_list.begin(); it != m_head && it != m_list.end(); ++it)
	{
		if (it == m_list.end())
			break;

		toSkip++;
	}

	return m_list.size() - toSkip;
}

template<class T> 
bool ThreadSafeQueue<T>::empty(void) const
{
	iterator next = m_head;
	++next;

	return (next == m_tail);
}

template<class T> 
void ThreadSafeQueue<T>::pop(void)
{
	if (empty())
		throw CException(LOGL_ERROR, 0, "No elements to read from queue.");

	iterator next = m_head;
	++next;

	if (next != m_tail)
		m_head = next;
}

template<class T> 
T ThreadSafeQueue<T>::front(void) const
{
	if (empty() == false)
	{
		iterator next = m_head;
		++next;

		if (next != m_tail)
			return *next;
	}

	// this should never happen
	throw CException(LOGL_ERROR, 0, "No elements to read from queue.");
}