/**
* @file threads.h
*
*/

#ifndef _INC_THREADS_H
#define _INC_THREADS_H

#include "../common/common.h"
#include "../common/sphere_library/smutex.h"
#include "../common/sphere_library/sresetevents.h"
#include "../common/sphere_library/sstringobjs.h"
#include "../sphere/ProfileData.h"
#include "../sphere_library/CSTime.h"
#include <exception>
#include <list>

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
#else
	typedef pthread_t spherethread_t;
	typedef pthread_t threadid_t;
	#define SPHERE_THREADENTRY_RETNTYPE void *
	#define SPHERE_THREADENTRY_CALLTYPE
#endif


// Interface for threads. Almost always should be used instead of any implementing classes
class IThread
{
public:
	enum Priority
	{
		Idle,			// tick 1000ms
		Low,			// tick 200ms
		Normal,			// tick 100ms
		High,			// tick 50ms
		Highest,		// tick 5ms
		RealTime,		// tick almost instantly
		Disabled = 0xFF	// tick never
	};

	virtual threadid_t getId() const = 0;
	virtual const char *getName() const = 0;

	virtual bool isActive() const = 0;
	virtual bool checkStuck() = 0;

	virtual void start() = 0;
	virtual void terminate(bool ended) = 0;
	virtual void waitForClose() = 0;

	virtual void setPriority(Priority) = 0;
	virtual Priority getPriority() const = 0;

	static inline threadid_t getCurrentThreadId() noexcept
	{
#ifdef _WIN32
		return ::GetCurrentThreadId();
#else
		return pthread_self();
#endif
	}
	static inline bool isSameThreadId(threadid_t firstId, threadid_t secondId) noexcept
	{
#ifdef _WIN32
		return (firstId == secondId);
#else
		return pthread_equal(firstId,secondId);
#endif
	}

	inline bool isSameThread(threadid_t otherThreadId) const noexcept
	{
		return isSameThreadId(getCurrentThreadId(), otherThreadId);
	}

	static const uint m_nameMaxLength = 16;	// Unix support a max 16 bytes thread name.
	static void setThreadName(const char* name);

protected:
	virtual bool shouldExit() = 0;

public:
	virtual ~IThread() { };
};

typedef std::list<IThread *> spherethreadlist_t;
template<class T>
class TlsValue;

// Singleton utility class for working with threads. Holds all running threads inside
class ThreadHolder
{
public:
	static constexpr lpctstr m_sClassName = "ThreadHolder";

	// returns current working thread or DummySphereThread * if no IThread threads are running
	static IThread *current() noexcept;
	// records a thread to the list. Sould NOT be called, internal usage
	static void push(IThread *thread);
	// removes a thread from the list. Sould NOT be called, internal usage
	static void pop(IThread *thread);
	// returns thread at i pos
	static IThread * getThreadAt(size_t at);

	// returns number of running threads. Sould NOT be called, unit tests usage
	static inline size_t getActiveThreads() { return m_threadCount; }

private:
	static void init();

private:
	static spherethreadlist_t m_threads;
	static size_t m_threadCount;
	static bool m_inited;
	static SimpleMutex m_mutex;

public:
	static TlsValue<IThread *> m_currentThread;

private:
	ThreadHolder() = delete;
};

// Thread implementation. See IThread for list of available methods.
class AbstractThread : public IThread
{
protected:
    bool _thread_selfTerminateAfterThisTick;

private:
	threadid_t m_id;
	const char *m_name;
	static int m_threadsAvailable;
	spherethread_t m_handle;
	uint m_hangCheck;
	Priority m_priority;
	uint m_tickPeriod;
	AutoResetEvent m_sleepEvent;

	bool m_terminateRequested;
	ManualResetEvent m_terminateEvent;

public:
	AbstractThread(const char *name, Priority priority = IThread::Normal);
	virtual ~AbstractThread();

private:
	AbstractThread(const AbstractThread& copy);
	AbstractThread& operator=(const AbstractThread& other);

public:
	threadid_t getId() const { return m_id; }
	const char *getName() const { return m_name; }
	void overwriteInternalThreadName(const char* name) {	// Use it only if you know what you are doing!
		m_name = name;										//  This doesn't actually do the change of the thread name!
	}

	bool isActive() const;
	bool isCurrentThread() const;
	bool checkStuck();

	virtual void start();
	virtual void terminate(bool ended);
	virtual void waitForClose();
	virtual void awaken();

	void setPriority(Priority pri);
	Priority getPriority() const { return m_priority; }


protected:
	virtual void tick() = 0;
	// NOTE: this should not be too long-lasted function, so no world loading, etc here!!!
	virtual void onStart();
	virtual bool shouldExit();

private:
	void run();
	static SPHERE_THREADENTRY_RETNTYPE SPHERE_THREADENTRY_CALLTYPE runner(void *callerThread);
};

struct TemporaryStringStorage;

// Sphere thread. Have some sphere-specific
class AbstractSphereThread : public AbstractThread
{
private:
#ifdef THREAD_TRACK_CALLSTACK
	struct STACK_INFO_REC
	{
		const char *functionName;
	};

	STACK_INFO_REC m_stackInfo[0x1000];
	size_t m_stackPos;
	bool m_freezeCallStack;
    bool m_exceptionStackUnwinding;
#endif

public:
	AbstractSphereThread(const char *name, Priority priority = IThread::Normal);
	virtual ~AbstractSphereThread() = default;

private:
	AbstractSphereThread(const AbstractSphereThread& copy);
	AbstractSphereThread& operator=(const AbstractSphereThread& other);

public:
	// allocates a char* with size of THREAD_MAX_LINE_LENGTH characters from the thread local storage
	char *allocateBuffer();
	TemporaryStringStorage *allocateStringBuffer();

	// allocates a manageable String from the thread local storage
	void allocateString(TemporaryString &string);

    void exceptionCaught();

#ifdef THREAD_TRACK_CALLSTACK
	inline void freezeCallStack(bool freeze) noexcept
	{
		m_freezeCallStack = freeze;
	}

	void pushStackCall(const char *name) noexcept;
	inline void popStackCall(void) noexcept
	{
		if (m_freezeCallStack == false)
			--m_stackPos;
	}

    void exceptionNotifyStackUnwinding(void);
	void printStackTrace();
#endif

	ProfileData m_profile;	// the current active statistical profile.

protected:
	virtual bool shouldExit();
};

// Dummy thread for context when no thread really exists
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

// stores a value unique to each thread, intended to hold
// a pointer (e.g. the current IThread instance)
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

private:
	TlsValue(const TlsValue& copy);
	TlsValue& operator=(const TlsValue& other);

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


// used to hold debug information for stack
#ifdef THREAD_TRACK_CALLSTACK
class StackDebugInformation
{
private:
	AbstractSphereThread* m_context;

public:
	StackDebugInformation(const char *name) noexcept;
	~StackDebugInformation();

private:
	StackDebugInformation(const StackDebugInformation& copy);
	StackDebugInformation& operator=(const StackDebugInformation& other);

public:
	static void printStackTrace()
	{
		static_cast<AbstractSphereThread *>(ThreadHolder::current())->printStackTrace();
	}
    static void freezeCallStack(bool freeze)
    {
        static_cast<AbstractSphereThread *>(ThreadHolder::current())->freezeCallStack(freeze);
    }
};

// Remember, call stack is disabled on Release builds!
#define ADDTOCALLSTACK(_function_)	const StackDebugInformation debugStack(_function_)

// Add to the call stack these functions only in debug mode, to have the most precise call stack
//  even if these functions are thought to be very safe and (nearly) exception-free.
#ifdef _DEBUG
	#define ADDTOCALLSTACK_INTENSIVE(_function_)	ADDTOCALLSTACK(_function_)
#else
	#define ADDTOCALLSTACK_INTENSIVE(_function_)    (void)0
#endif


#else // THREAD_TRACK_CALLSTACK

#define ADDTOCALLSTACK(_function_)                  (void)0
#define ADDTOCALLSTACK_INTENSIVE(_function_)        (void)0

#endif // THREAD_TRACK_CALLSTACK


#endif // _INC_THREADS_H
