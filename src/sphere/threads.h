/**
* @file threads.h
*
*/

#ifndef _INC_THREADS_H
#define _INC_THREADS_H

#include "../common/common.h"
#include "../common/sphere_library/sresetevents.h"
#include "../common/sphere_library/sstringobjs.h"
#include "../sphere/ProfileData.h"
#include <atomic>
#include <vector>

#ifndef _WIN32
	#include <pthread.h>
#endif


/**
 * Sphere threading system
 * Threads should be inherited from AbstractThread with overridden tick() method
 * Also useful to override onStart() in order to initialise class data variables for ticking
 *   which is triggered whenever the thread is starting/restarting
**/

// Types definition for different platforms
#ifdef _WIN32
	typedef HANDLE spherethread_t;
	typedef DWORD threadid_t;
	#define SPHERE_THREADENTRY_RETNTYPE unsigned
	#define SPHERE_THREADENTRY_CALLTYPE __stdcall
	#define SPHERE_THREADT_NULL nullptr
#else
	typedef pthread_t spherethread_t;
#   ifdef __APPLE__
	typedef uint64_t  threadid_t;
#   else
	typedef pthread_t threadid_t;
#endif

	#define SPHERE_THREADENTRY_RETNTYPE void *
	#define SPHERE_THREADENTRY_CALLTYPE
#endif

class AbstractThread;

// stores a value unique to each thread, intended to hold
// a pointer (e.g. the current AbstractThread instance)
template<class T>
class TlsValue
{
private:
#ifdef _WIN32
	dword _key;
#else
	pthread_key_t _key;
#endif
	bool _ready;

public:
	TlsValue();
	~TlsValue();

	TlsValue(const TlsValue& copy) = delete;
	TlsValue& operator=(const TlsValue& other) = delete;

public:
	// allows assignment to set the current value
	TlsValue& operator=(const T& value)
	{
		set(value);
		return *this;
	}

	// allows a cast to get current value
	operator T() const { return get(); }

public:
	void set(const T value); // set the value for the current thread
	T get() const; // get the value for the current thread
};

template<class T>
TlsValue<T>::TlsValue()
{
	// allocate thread storage
#ifdef _WIN32
	_key = TlsAlloc();
	_ready = (_key != TLS_OUT_OF_INDEXES);
#else
	_key = 0;
	_ready = (pthread_key_create(&_key, nullptr) == 0);
#endif
}

template<class T>
TlsValue<T>::~TlsValue()
{
	// free the thread storage
	if (_ready)
#ifdef _WIN32
		TlsFree(_key);
#else
		pthread_key_delete(_key);
#endif
	_ready = false;
}

template<class T>
void TlsValue<T>::set(const T value)
{
	ASSERT(_ready);
#ifdef _WIN32
	TlsSetValue(_key, value);
#else
	pthread_setspecific(_key, value);
#endif
}

template<class T>
T TlsValue<T>::get() const
{
	if (_ready == false)
		return nullptr;
#ifdef _WIN32
	return reinterpret_cast<T>(TlsGetValue(_key));
#else
	return reinterpret_cast<T>(pthread_getspecific(_key));
#endif
}

enum class ThreadPriority : int
{
    Idle,			// tick 1000ms
    Low,			// tick 200ms
    Normal,			// tick 100ms
    High,			// tick 50ms
    Highest,		// tick 5ms
    RealTime,		// tick almost instantly
    Disabled = 0xFF	// tick never
};

// Thread base implementation, without Sphere "extensions".
class AbstractThread
{
    friend class ThreadHolder;

    static int m_threadsAvailable;
public:
    static constexpr uint m_nameMaxLength = 16;	// Unix support a max 16 bytes thread name.

protected:
    threadid_t m_threadSystemId;
    int m_threadHolderId;

    bool _fKeepAliveAtShutdown;
    volatile std::atomic_bool _thread_selfTerminateAfterThisTick;
    volatile std::atomic_bool _fIsClosing;
private:
    volatile std::atomic_bool m_terminateRequested;
    char m_name[30];

    // pthread_t type is opaque (platform-defined). It can be an integer, a struct, a ptr something. Memset is the safest and more portable way.
    //spherethread_t m_handle;
    // Or, since we need here an "invalid" value, just use optional.
    std::optional<spherethread_t> m_handle;

	uint m_hangCheck;
    ThreadPriority m_priority;
	uint m_tickPeriod;
	AutoResetEvent m_sleepEvent;
	ManualResetEvent m_terminateEvent;

public:
    AbstractThread(const char *name, ThreadPriority priority = ThreadPriority::Normal);
	virtual ~AbstractThread();

	AbstractThread(const AbstractThread& copy) = delete;
	AbstractThread& operator=(const AbstractThread& other) = delete;

public:
    threadid_t getId() const noexcept { return m_threadSystemId; }
    virtual const char *getName() const noexcept { return m_name; }

    virtual bool isActive() const;
    virtual bool checkStuck();

    virtual void start();
    virtual void terminate(bool ended);
    virtual void waitForClose();
	void awaken();

    void setPriority(ThreadPriority pri);
    ThreadPriority getPriority() const { return m_priority; }

    void overwriteInternalThreadName(const char* name) noexcept;
    bool isCurrentThread() const noexcept;

  protected:
    virtual void tick() = 0;

    // NOTE: this should not be too long-lasted function, so no world loading, etc here!!!
    virtual void onStart();
    virtual bool shouldExit() noexcept;

  private:
    void run();
    static SPHERE_THREADENTRY_RETNTYPE SPHERE_THREADENTRY_CALLTYPE runner(void *callerThread);

  public:
    static void setThreadName(const char* name);

    bool closing() noexcept
    {
        return _fIsClosing;
    }

    static inline threadid_t getCurrentThreadSystemId() noexcept
    {
#if defined(_WIN32)
        return ::GetCurrentThreadId();
#elif defined(__APPLE__)
        // On OSX, 'threadid_t' is not an integer but a '_opaque_pthread_t *'), so we need to resort to another method.
        uint64_t threadid = 0;
        pthread_threadid_np(pthread_self(), &threadid);
        return threadid;
#else
        return pthread_self();
#endif
    }
    static inline bool isSameThreadId(threadid_t firstId, threadid_t secondId) noexcept
    {
#if defined(_WIN32) || defined(__APPLE__)
        return (firstId == secondId);
#else
        return pthread_equal(firstId,secondId);
#endif
    }

    inline bool isSameThread(threadid_t otherThreadId) const noexcept
    {
        return isSameThreadId(getCurrentThreadSystemId(), otherThreadId);
    }
};


// Sphere thread. Have some sphere-specific
class AbstractSphereThread : public AbstractThread
{
	friend class ThreadHolder;

#ifdef THREAD_TRACK_CALLSTACK
	struct STACK_INFO_REC
	{
		const char *functionName;
	};

	STACK_INFO_REC m_stackInfo[0x1000];
	ssize_t m_stackPos;
	bool m_freezeCallStack;
    bool m_exceptionStackUnwinding;
#endif

public:
    AbstractSphereThread(const char *name, ThreadPriority priority = ThreadPriority::Normal);
	virtual ~AbstractSphereThread();

	AbstractSphereThread(const AbstractSphereThread& copy) = delete;
	AbstractSphereThread& operator=(const AbstractSphereThread& other) = delete;

public:
	// allocates a char* with size of THREAD_MAX_LINE_LENGTH characters from the thread local storage
	char *allocateBuffer() noexcept;

	// allocates a manageable String from the thread local storage
	void getStringBuffer(TemporaryString &string) noexcept;

    void exceptionCaught();

#ifdef THREAD_TRACK_CALLSTACK
	inline void freezeCallStack(bool freeze) noexcept
	{
		m_freezeCallStack = freeze;
	}

	void pushStackCall(const char *name) noexcept;
	void popStackCall() NOEXCEPT_NODEBUG;

    void exceptionNotifyStackUnwinding() noexcept;
	void printStackTrace() noexcept;
#endif

	ProfileData m_profile;	// the current active statistical profile.

protected:
	virtual bool shouldExit() noexcept;
};

// Dummy thread for context when no thread really exists. To be called only once, at startup.
class DummySphereThread : public AbstractSphereThread
{
private:
	static DummySphereThread *_instance;

public:
	static void createInstance();
	static DummySphereThread *getInstance() noexcept;

protected:
	DummySphereThread();
	virtual void tick();
};


// Singleton utility class for working with threads. Holds all running threads inside.
class ThreadHolder
{
    friend class AbstractThread;

    struct SphereThreadData
    {
        AbstractThread *m_ptr;
        bool m_closed;
    };
    using spherethreadlist_t = std::vector<SphereThreadData>;
    spherethreadlist_t m_threads;

    using spherethreadpair_t = std::pair<threadid_t, AbstractSphereThread *>;
    std::vector<spherethreadpair_t> m_spherethreadpairs_systemid_ptr;

	int m_threadCount;
    volatile std::atomic_bool m_closingThreads;
	mutable std::shared_mutex m_mutex;

	ThreadHolder() noexcept;
    ~ThreadHolder() noexcept = default;

	friend void atexit_handler(void);
    friend void Sphere_ExitServer(void);
	void markThreadsClosing() CANTHROW;

    //SphereThreadData* findThreadData(AbstractThread* thread) noexcept;

public:
	static constexpr lpctstr m_sClassName = "ThreadHolder";

	static ThreadHolder& get() noexcept;

    bool closing() noexcept;
	// returns current working thread or DummySphereThread * if no AbstractThread threads are running
	AbstractThread *current() noexcept;
	// records a thread to the list. Sould NOT be called, internal usage
	void push(AbstractThread *thread) noexcept;
	// removes a thread from the list. Sould NOT be called, internal usage
	void remove(AbstractThread *thread) CANTHROW;
	// returns thread at i pos
	AbstractThread * getThreadAt(size_t at) noexcept;

	// returns number of running threads. Sould NOT be called, unit tests usage
	inline size_t getActiveThreads() noexcept { return m_threadCount; }
};


// used to hold debug information for the function call stack
#ifdef THREAD_TRACK_CALLSTACK
class StackDebugInformation
{
private:
	AbstractSphereThread* m_context;

public:
	StackDebugInformation(const char *name) noexcept;
	~StackDebugInformation() noexcept;

	StackDebugInformation(const StackDebugInformation& copy) = delete;
	StackDebugInformation& operator=(const StackDebugInformation& other) = delete;

public:
	static void printStackTrace() noexcept;
	static void freezeCallStack(bool freeze) noexcept;
};

// Remember, call stack is disabled on Release builds!
#define ADDTOCALLSTACK(_function_)	const StackDebugInformation debugStack(_function_)

// Add to the call stack these functions only in debug mode, to have the most precise call stack
//  even if these functions are thought to be very safe and (nearly) exception-free.
#ifdef _DEBUG
	#define ADDTOCALLSTACK_DEBUG(_function_)	ADDTOCALLSTACK(_function_)
#else
	#define ADDTOCALLSTACK_DEBUG(_function_)    (void)0
#endif


#else // THREAD_TRACK_CALLSTACK

#define ADDTOCALLSTACK(_function_)                  (void)0
#define ADDTOCALLSTACK_DEBUG(_function_)        (void)0

#endif // THREAD_TRACK_CALLSTACK


#endif // _INC_THREADS_H
