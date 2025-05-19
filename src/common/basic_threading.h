#ifndef _INC_BASIC_THREADING_H
#define _INC_BASIC_THREADING_H

/*
 * Defines for easy C++17 thread-locking for class methods.
**/

#define MT_ENGINES  0   // are we using/working on true multithreaded network/scripting engines?


#include <mutex>        // As per standard, std::mutex has a noexcept constructor.
#include <shared_mutex> // As per standard, std::shared_mutex has NOT a noexcept contructor, nevertheless its implementations should actually not throw.


// -- Macros for direct mutex lock/unlock and mutex-guarded value returning.

#define MT_DEFAULT_CMUTEX_TYPE       std::shared_mutex

// Name of the Class Mutex
#define MT_CMUTEX                   _classMutex
// Use this in the class definition to add the class mutex.
#define MT_CMUTEX_DEF               mutable MT_DEFAULT_CMUTEX_TYPE MT_CMUTEX

// Read-Only: multiple threads can read the same resource
#define MT_SHARED_LOCK_SET(pClass)  std::shared_lock<MT_DEFAULT_CMUTEX_TYPE> _shared_lock((pClass)->MT_CMUTEX)
// Locking is already done by MT_SHARED_LOCK_SET. Use it only if you know what you are doing!
#define MT_SHARED_LOCK              _shared_lock.lock()
// Unlocking is done automatically then the function ends (when the lock created with MT_SHARED_LOCK_SET goes out of scope). Use it only if you know what you are doing!
#define MT_SHARED_UNLOCK            _shared_lock.unlock()

// Read/Write: exclusive access to a thread at a time
#define MT_UNIQUE_LOCK_SET(pClass)  std::unique_lock<MT_DEFAULT_CMUTEX_TYPE> _unique_lock((pClass)->MT_CMUTEX)
// Locking is already done by MT_UNIQUE_LOCK_SET. Use it only if you know what you are doing!
#define MT_UNIQUE_LOCK              _unique_lock.lock()
// Unlocking is done automatically then the function ends (when the lock created with MT_UNIQUE_LOCK_SET goes out of scope). Use it only if you know what you are doing!
#define MT_UNIQUE_UNLOCK            _unique_lock.unlock()

// Thread-safe return macro. If we directly do return without using a temporary storage variable, the return value will be stored in the returned register
//	after the thread lock is destroyed, so after the mutex is unlocked. In this way, we have a variable holding the value of the return expression, but before the mutex unlocking.
// Do not use to return references! (It will return the address of the local-scoped, temporary variable _MT_RETURN_val)
#define MT_RETURN(x) \
		{volatile auto _MT_RETURN_val = (x); \
		return _MT_RETURN_val;}

// A single macro to lock the class mutex and return the value in a thread-safe way.
// Do not use to return references! (It will return the address of the local-scoped, temporary variable _MT_RETURN_val)
#define MT_SHARED_LOCK_RETURN(pClass, val) \
        /* Creating and locking a mutex might THROW std::system_error ! */ \
        {MT_SHARED_LOCK_SET(pClass); \
        MT_RETURN(val);}

#define MT_UNIQUE_LOCK_RETURN(pClass, val) \
        /* Creating and locking a mutex might THROW std::system_error ! */ \
        {MT_UNIQUE_LOCK_SET(pClass); \
        MT_RETURN(val);}


#if MT_ENGINES == 1  //_DEBUG

    #define MT_ENGINE_SHARED_LOCK_SET(pClass)   MT_SHARED_LOCK_SET
    #define MT_ENGINE_SHARED_LOCK               MT_SHARED_LOCK
    #define MT_ENGINE_SHARED_UNLOCK             MT_SHARED_UNLOCK
    #define MT_ENGINE_UNIQUE_LOCK_SET(pClass)   MT_UNIQUE_LOCK_SET
    #define MT_ENGINE_UNIQUE_UNLOCK             MT_UNIQUE_UNLOCK

    #define MT_ENGINE_RETURN(x)                 MT_RETURN(x)
    #define MT_ENGINE_SHARED_LOCK_RETURN(x)     MT_SHARED_LOCK_RETURN(x)
    #define MT_ENGINE_UNIQUE_LOCK_RETURN(x)     MT_UNIQUE_LOCK_RETURN(x)

#else
    #define MT_ENGINE_SHARED_LOCK_SET(pClass) (void)0
	#define MT_ENGINE_SHARED_LOCK		      (void)0
	#define MT_ENGINE_SHARED_UNLOCK	          (void)0
    #define MT_ENGINE_UNIQUE_LOCK_SET(pClass) (void)0
	#define MT_ENGINE_UNIQUE_LOCK		      (void)0
	#define MT_ENGINE_UNIQUE_UNLOCK	          (void)0
    #define MT_ENGINE_RETURN(x)               return (x)
    #define MT_ENGINE_SHARED_LOCK_RETURN(x)   return (x)
    #define MT_ENGINE_UNIQUE_LOCK_RETURN(x)   return (x)
#endif


// ---
// Try acquiring a lock (try multiple times), to be used only for CRITICAL mutexes, that you pray will never fail (for an OS error) to be locked (not because they are already locked). If successiful, call func_, usually a lambda. If fails, program explodes (std::abort).

#define RETRY_LOCK_ATTEMPTS_MAX 20
#define RETRY_LOCK_FOR_TASK_IMPL_(lock_type_, mutex_name_, lock_name_, max_retries_, retval_, func_) \
    { \
        bool success_acquire_ = false; \
        for (uint spin_attempt_ = 0; spin_attempt_ < max_retries_; ++spin_attempt_) { \
            try { \
                lock_type_ <std::remove_reference_t<decltype(mutex_name_)>> lock_name_ ((mutex_name_)); \
                retval_ = func_(); \
                success_acquire_ = true; \
                break; \
            } catch (const std::system_error& e_) { \
                STDERR_LOG("[File '%s', line %d, function '%s']. Failed to acquire lock on attempt %u. Exc msg: '%s'.\n", \
                           __FILE__, __LINE__, __func__, spin_attempt_ + 1, e_.what()); \
            } \
        } \
        if (!success_acquire_) { \
            /*throw std::runtime_error("Failed to acquire lock after " #max_retries_ " attempts."); */ \
            /* That should never happen. */ \
            RaiseImmediateAbort(10); \
        } \
    }

#define RETRY_SHARED_LOCK_FOR_TASK(mutex_name_, lock_name_, retval_, func_) \
    RETRY_LOCK_FOR_TASK_IMPL_(std::shared_lock, (mutex_name_), (lock_name_), RETRY_LOCK_ATTEMPTS_MAX, (retval_), (func_))

#define RETRY_UNIQUE_LOCK_FOR_TASK(mutex_name_, lock_name_, retval_, func_) \
    RETRY_LOCK_FOR_TASK_IMPL_(std::unique_lock, (mutex_name_), (lock_name_), RETRY_LOCK_ATTEMPTS_MAX, (retval_), (func_))


// ---

namespace sl
{

template<typename T>
class GuardedAccess
{
public:    
    using Mutex      = MT_DEFAULT_CMUTEX_TYPE;
    using SharedLock = std::shared_lock<Mutex>;
    using UniqueLock = std::unique_lock<Mutex>;

    struct NoOpLock
    {
        NoOpLock(Mutex&) noexcept {}
        ~NoOpLock() noexcept = default;
    };
    using OptLock =
#if MT_ENGINES
        SharedLock
#else
        NoOpLock
#endif
          ;

    // ---- Construct underlying T in-place
    template<typename... Args>
    explicit GuardedAccess(Args&&... args)
        : mutex_(), data_(std::forward<Args>(args)...)
    {}

    // ----- Reader proxies -----
    class LockedReader
    {
    public:
        LockedReader(const GuardedAccess& owner)
            : lock_(owner->pmutex_), ptr_(&owner.data_)
        {}
        RETURNS_NOTNULL
        const T* operator->() const { return ptr_; }
        const T& operator* () const { return *ptr_; }
    private:
        SharedLock  lock_;
        const T*    ptr_;
    };

    class MTEngineLockedReader
    {
    public:
        MTEngineLockedReader(const GuardedAccess& owner)
            :
#if MT_ENGINES
            locked_(owner)
#else
            owner_(owner)
#endif
        {}
#if MT_ENGINES
        LockedReader operator-> () const { return locked_.operator->; }
        LockedReader operator*  () const { return locked_.operator*;  }
#else
        RETURNS_NOTNULL
        const T* operator->() const { return &owner_.data_; }
        const T& operator* () const { return owner_.data_; }
#endif
    private:
#if MT_ENGINES
        LockedReader   locked_;
#else
        const GuardedAccess&  owner_;
#endif
        };

    // ----- Writer proxies -----
    class LockedWriter
    {
    public:
        LockedWriter(GuardedAccess& owner)
            : lock_(owner->pmutex_), ptr_(&owner.data_)
        {}
        RETURNS_NOTNULL
        T*       operator->()       { return ptr_; }
        T&       operator* ()       { return *ptr_; }
    private:
        UniqueLock  lock_;
        T*          ptr_;
    };

    class MTEngineLockedWriter
    {
    public:
        MTEngineLockedWriter(GuardedAccess& owner)
            :
#if MT_ENGINES
            locked_(owner)
#else
            owner_(owner)
#endif
        {}
#if MT_ENGINES
        LockedWriter operator-> () { return locked_.operator->; }
        LockedWriter operator*  () { return locked_.operator*;  }
#else
        RETURNS_NOTNULL
        T* operator->() { return &owner_.data_; }
        T& operator* () { return owner_.data_; }
#endif
    private:
#if MT_ENGINES
        LockedWriter   locked_;
#else
        GuardedAccess&  owner_;
#endif
    };

    // ---- Unsafe/raw accessors ----
    // No lock taken!  You must lock manually.
    T&       unsafeWriter()       { return data_; }
    const T& unsafeReader() const { return data_; }

    // ---- Lock grabbing ----
    auto getLockShared() const { return SharedLock(mutex_); }
    auto getLockUnique()       { return UniqueLock(mutex_); }

    auto mtEngineGetLockShared() const   { return OptLock(mutex_); }
    auto mtEngineGetLockUnique()         { return OptLock(mutex_); }

    // ---- Accessors ----
    auto lockedReader() const   { return LockedReader(*this); }
    auto lockedWriter()         { return LockedWriter(*this); }

    auto mtEngineLockedReader() const   { return MTEngineLockedReader(*this); }
    auto mtEngineLockedWriter()         { return MTEngineLockedWriter(*this); }

private:
    mutable Mutex  mutex_;
    T              data_;
};


}   // namespace sl

#endif // _INC_BASIC_THREADING_H
