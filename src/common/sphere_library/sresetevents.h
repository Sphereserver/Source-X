/**
* @file sresetevents.h
*
*/

#ifndef _INC_SRESETEVENTS_H
#define _INC_SRESETEVENTS_H

#include "../../common/common.h"
#ifndef _WIN32
	#include <pthread.h>
#endif


/**
* @brief Waits until the specified object is in the signaled state, or the time-out interval elapses.
*/
class AutoResetEvent
{
public:
#ifdef _WIN32
	static constexpr uint _kiInfinite = INFINITE;   // Default wait time.
#else
	static constexpr uint _kiInfinite = 0xffffffff;   // Default wait time.
#endif

	/** @name Constructors, Destructor, Asign operator:
	*/
	///@{
	AutoResetEvent();
	~AutoResetEvent();
private:
	AutoResetEvent(const AutoResetEvent& copy);
	AutoResetEvent& operator=(const AutoResetEvent& other);
	///@}

public:
	/** @name Interaction:
	*/
	///@{
	/**
	* @brief Wait until the timeout interval elapses or this object is signaled.
	* @param timeout max elapsed time to wait.
	*/
	void wait(uint timeout = _kiInfinite);
	/**
	* @brief Signal this object to stop the wait.
	*/
	void signal();
	///@}

private:
#ifdef _WIN32
	HANDLE m_handle;   // Windows API Handler to handle the event.
#else
	pthread_mutex_t m_criticalSection;    // Unix API mutex.
	pthread_mutexattr_t m_criticalSectionAttr;    // Unix API mutex attr.
	pthread_condattr_t m_conditionAttr;    // Unix API condition attr.
	pthread_cond_t m_condition;    // Unix API condition.
#endif
};


/**
* @brief Waits until the specified object is in the signaled state, or the time-out interval elapses.
*/
class ManualResetEvent
{
public:
#ifdef _WIN32
	static constexpr uint _kiInfinite = INFINITE;   // Default wait time.
#else
	static constexpr uint _kiInfinite = 0xffffffff;   // Default wait time.
#endif

	/** @name Constructors, Destructor, Asign operator:
	*/
	///@{
	ManualResetEvent();
	~ManualResetEvent();
private:
	ManualResetEvent(const ManualResetEvent& copy);
	ManualResetEvent& operator=(const ManualResetEvent& other);
	///@}

public:
	/** @name Interaction:
	*/
	///@{
	/**
	* @brief Waits until event is set, checking it each time interval.
	* @param timeout time interval to check.
	*/
	void wait(uint timeout = _kiInfinite);
	/**
	* @brief resets the event.
	*/
	void reset();
	/**
	* @brief sets the event.
	*/
	void set();
	///@}

private:
#ifdef _WIN32
	HANDLE m_handle;   // Windows API Handler to handle the event.
#else
	bool m_value;
	pthread_mutex_t m_criticalSection;    // Unix API mutex.
	pthread_mutexattr_t m_criticalSectionAttr;    // Unix API mutex attr.
	pthread_condattr_t m_conditionAttr;    // Unix API condition attr.
	pthread_cond_t m_condition;    // Unix API condition.
#endif
};


#endif //_INC_SRESETEVENTS_H
