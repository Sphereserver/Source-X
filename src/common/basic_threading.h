#ifndef _INC_BASIC_THREADING_H
#define _INC_BASIC_THREADING_H

/*
 * Defines for easy C++17 thread-locking for class methods.
**/

#define MT_ENGINES  0   // are we using/working on true multithreaded network/scripting engines?

#include <mutex>
#include <shared_mutex>


// Name of the Class Mutex
#define MT_CMUTEX           _classMutex
// Use this in the class definition to add the class mutex.
#define MT_CMUTEX_DEF	    mutable std::shared_mutex MT_CMUTEX

// Read-Only: multiple threads can read the same resource
#define MT_SHARED_LOCK_SET  std::shared_lock<std::shared_mutex> _shared_lock(this->MT_CMUTEX)
// Locking is already done by MT_SHARED_LOCK_SET. Use it only if you know what you are doing!
#define MT_SHARED_LOCK      _shared_lock.lock()
// Unlocking is done automatically then the function ends (when the lock created with MT_SHARED_LOCK_SET goes out of scope). Use it only if you know what you are doing!
#define MT_SHARED_UNLOCK	   _shared_lock.unlock()

// Read/Write: exclusive access to a thread at a time
#define MT_UNIQUE_LOCK_SET  std::unique_lock<std::shared_mutex> _unique_lock(this->MT_CMUTEX)
// Locking is already done by MT_UNIQUE_LOCK_SET. Use it only if you know what you are doing!
#define MT_UNIQUE_LOCK      _unique_lock.lock()
// Unlocking is done automatically then the function ends (when the lock created with MT_UNIQUE_LOCK_SET goes out of scope). Use it only if you know what you are doing!
#define MT_UNIQUE_UNLOCK    _unique_lock.unlock()

// Thread-safe return macro. If we directly do return without using a temporary storage variable, the return value will be stored in the returned register
//	after the thread lock is destroyed, so after the mutex is unlocked. In this way, we have a variable holding the value of the return expression, but before the mutex unlocking.
// Do not use to return references! (It will return the address of the local-scoped, temporary variable _MT_RETURN_val)
#define MT_RETURN(x) \
		{volatile auto _MT_RETURN_val = (x); \
		return _MT_RETURN_val;}

// A single macro to lock the class mutex and return the value in a thread-safe way.
// Do not use to return references! (It will return the address of the local-scoped, temporary variable _MT_RETURN_val)
#define MT_SHARED_LOCK_RETURN(x) \
		{MT_SHARED_LOCK_SET; \
		MT_RETURN(x);}

#define MT_UNIQUE_LOCK_RETURN(x) \
		{MT_UNIQUE_LOCK_SET; \
		MT_RETURN(x);}


#if MT_ENGINES == 1  //_DEBUG

#define MT_ENGINE_SHARED_LOCK_SET       MT_SHARED_LOCK_SET
#define MT_ENGINE_SHARED_LOCK           MT_SHARED_LOCK
#define MT_ENGINE_SHARED_UNLOCK	       MT_SHARED_UNLOCK
#define MT_ENGINE_UNIQUE_LOCK_SET       MT_UNIQUE_LOCK_SET
#define MT_ENGINE_UNIQUE_UNLOCK         MT_UNIQUE_UNLOCK

#define MT_ENGINE_RETURN(x)             MT_RETURN(x)
#define MT_ENGINE_SHARED_LOCK_RETURN(x) MT_SHARED_LOCK_RETURN(x)
#define MT_ENGINE_UNIQUE_LOCK_RETURN(x) MT_UNIQUE_LOCK_RETURN(x)

#else
	#define MT_ENGINE_SHARED_LOCK_SET	      (void)0
	#define MT_ENGINE_SHARED_LOCK		      (void)0
	#define MT_ENGINE_SHARED_UNLOCK	          (void)0
	#define MT_ENGINE_UNIQUE_LOCK_SET	      (void)0
	#define MT_ENGINE_UNIQUE_LOCK		      (void)0
	#define MT_ENGINE_UNIQUE_UNLOCK	          (void)0
	#define MT_ENGINE_RETURN(x)                return (x)
	#define MT_ENGINE_SHARED_LOCK_RETURN(x)    return (x)
    #define MT_ENGINE_UNIQUE_LOCK_RETURN(x)    return (x)
#endif


#endif // _INC_BASIC_THREADING_H
