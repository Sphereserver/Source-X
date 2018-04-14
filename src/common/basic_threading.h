#ifndef _INC_BASIC_THREADING
#define _INC_BASIC_THREADING

/*
 * Defines for easy C++17 thread-locking for class methods.
**/

//#ifdef _DEBUG
#include <shared_mutex>
#define THREAD_CLASS_MUTEX_DEF	std::shared_mutex _classMutex									// Use this in the class definition to add the class mutex.
#define THREAD_SHARED_LOCK_SET	std::shared_lock<std::shared_mutex> _shared_lock(_classMutex)	// Read-Only: multiple threads can read the same resource
#define THREAD_SHARED_LOCK		_shared_lock.lock()												// Locking is already done by THREAD_SHARED_LOCK_SET. Use it only if you know what you are doing!
#define THREAD_SHARED_UNLOCK	_shared_lock.unlock()											// Unlocking is done automatically then the function ends (when the lock created with THREAD_SHARED_LOCK_SET goes out of scope). Use it only if you know what you are doing!
#define THREAD_UNIQUE_LOCK_SET	std::unique_lock<std::shared_mutex> _unique_lock(_classMutex)	// Read/Write: exclusive access to a thread at a time
#define THREAD_UNIQUE_LOCK		_unique_lock.lock()												// Locking is already done by THREAD_UNIQUE_LOCK_SET. Use it only if you know what you are doing!
#define THREAD_UNIQUE_UNLOCK	_unique_lock.unlock()											// Unlocking is done automatically then the function ends (when the lock created with THREAD_UNIQUE_LOCK_SET goes out of scope). Use it only if you know what you are doing!

// Thread-safe return macro. If we directly do return without using a temporary storage variable, the return value will be stored in the returned register
//	after the thread lock is destroyed, so after the mutex is unlocked. In this way, we have a variable holding the value of the return expression, but before the mutex unlocking.
#define TS_RETURN(x) \
		auto _ts_return_val = (x); \
		return _ts_return_val

// A single macro to lock the class mutex and return the value in a thread-safe way.
#define THREAD_LOCK_RETURN(x) \
		THREAD_SHARED_LOCK_SET; \
		TS_RETURN(x)
/*
#else
	#define THREAD_CLASS_MUTEX_DEF	(void)0
	#define THREAD_SHARED_LOCK_SET	(void)0
	#define THREAD_SHARED_LOCK		(void)0
	#define THREAD_SHARED_UNLOCK	(void)0
	#define THREAD_UNIQUE_LOCK_SET	(void)0
	#define THREAD_UNIQUE_LOCK		(void)0
	#define THREAD_UNIQUE_UNLOCK	(void)0
	#define TS_RETURN(x)			return (x)
	#define THREAD_LOCK_RETURN(x)	return (x)
#endif
*/

#endif // _INC_BASIC_THREADING
