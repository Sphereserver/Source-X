// this thing is somehow required to be able to initialise OLE
#define _WIN32_DCOM

#include <algorithm>
#include <atomic>
#include "../common/CException.h"
#include "../common/common.h"
#include "../common/CLog.h"
#include "../game/CServer.h"
#include "ProfileTask.h"
#include "threads.h"

#if defined(_WIN32)
	#include <process.h>
	#include <objbase.h>
#elif !defined(_BSD) && !defined(__APPLE__)
	#include <sys/prctl.h>
#endif


// number of exceptions after which we restart thread and think that the thread have gone in exceptioning loops
#define EXCEPTIONS_ALLOWED	10

// number of milliseconds to wait for a thread to close
#define THREADJOIN_TIMEOUT	60000


struct TemporaryStringThreadSafeStateHolder
{
	// Normal Buffer
	SimpleMutex g_tmpStringMutex;
	std::atomic<int> g_tmpStringIndex = 0;
	char g_tmpStrings[THREAD_TSTRING_STORAGE][THREAD_STRING_LENGTH];

	// TemporaryString Buffer
	SimpleMutex g_tmpTemporaryStringMutex;
	std::atomic<int> g_tmpTemporaryStringIndex = 0;


	struct TemporaryStringStorage
	{
		char m_buffer[THREAD_STRING_LENGTH];
		char m_state;
	} g_tmpTemporaryStringStorage[THREAD_STRING_STORAGE];


public:
	static TemporaryStringThreadSafeStateHolder* get() noexcept
	{
		static TemporaryStringThreadSafeStateHolder instance{};
		return &instance;
	}
};


#ifdef _WIN32
#pragma pack(push, 8)
typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;
	LPCSTR szName;
	DWORD dwThreadID;
	DWORD dwFlags;
} THREADNAME_INFO;
#pragma pack(pop)

static constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
#endif

void IThread::setThreadName(const char* name)
{
	// register the thread name

	// Unix uses prctl to set thread name
	// thread name must be 16 bytes, zero-padded if shorter
	char name_trimmed[m_nameMaxLength] = { '\0' };	// m_nameMaxLength = 16
    Str_CopyLimitNull(name_trimmed, name, m_nameMaxLength);

#if defined(_WIN32)
    #if defined(_MSC_VER)	// TODO: support thread naming when compiling with compilers other than Microsoft's
        // Windows uses THREADNAME_INFO structure to set thread name
        THREADNAME_INFO info;
        info.dwType = 0x1000;
        info.szName = name_trimmed;
        info.dwThreadID = (DWORD)(-1);
        info.dwFlags = 0;

        __try
        {
            RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
        }
    #endif
#elif defined(__APPLE__)	// Mac
	pthread_setname_np(name_trimmed);
#elif !defined(_BSD)		// Linux
	prctl(PR_SET_NAME, name_trimmed, 0, 0, 0);
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
	pthread_set_name_np(getCurrentThreadId(), name_trimmed);
#elif defined(__NetBSD__)
	pthread_setname_np(getCurrentThreadId(), "%s", name_trimmed);
#endif

    auto athr = static_cast<AbstractSphereThread*>(ThreadHolder::get().current());
    ASSERT(athr);
    athr->overwriteInternalThreadName(name_trimmed);
}


/**
 * ThreadHolder
**/

ThreadHolder::ThreadHolder() noexcept :
	m_threadCount(-1), m_closing(false)
{}

ThreadHolder& ThreadHolder::get() noexcept
{
	static ThreadHolder instance;
	return instance;
}

bool ThreadHolder::closing() noexcept
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_closing;
}

IThread* ThreadHolder::current()
{
    // Do not use ASSERTs here, would cause recursion.

    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (m_closing)
        return nullptr;

    const threadid_t tid = IThread::getCurrentThreadSystemId();

	if (m_spherethreadpairs_systemid_ptr.empty()) {
        auto thread = static_cast<IThread*>(DummySphereThread::getInstance());
        thread->m_threadSystemId = tid;
        lock.unlock();
        push(thread);
        return thread;
    }

    spherethreadpair_t *found = nullptr;
    for (auto &elem : m_spherethreadpairs_systemid_ptr) {
        if (elem.first == tid) {
            found = &elem;
            break;
        }
    }
    if (!found) {
        throw CSError(LOGL_FATAL, 0, "Thread handle not found in vector?");
    }

    auto thread = static_cast<IThread *>(found->second);

    ASSERT(thread->m_threadHolderId != -1);
    SphereThreadData *tdata = &(m_threads[thread->m_threadHolderId]);
    if (tdata->m_closed)
        return nullptr;

    // Uncomment it only for testing purposes, since this method is called very often and we don't need the additional overhead
    //DEBUG_ASSERT( thread->isSameThread(thread->getId()) );

	return thread;
}

void ThreadHolder::push(IThread *thread)
{
    auto sphere_thread = dynamic_cast<AbstractSphereThread*>(thread);
    if (!sphere_thread)
        throw CSError(LOGL_FATAL, 0, "IThread not being an AbstractSphereThread?");

	std::unique_lock<std::shared_mutex> lock(m_mutex);

    ASSERT(thread->m_threadSystemId != 0);
    ASSERT(thread->m_threadHolderId == -1);

	m_threads.emplace_back( SphereThreadData{ thread, false });
    thread->m_threadHolderId = m_threadCount + 1;

#ifdef _DEBUG
    auto it_thread = std::find_if(
        m_spherethreadpairs_systemid_ptr.begin(),
        m_spherethreadpairs_systemid_ptr.end(),
        [sphere_thread](spherethreadpair_t const &elem) noexcept -> bool { return elem.second == sphere_thread; });

    // I don't want duplicates.
    DEBUG_ASSERT(it_thread == m_spherethreadpairs_systemid_ptr.end());
#endif
    m_spherethreadpairs_systemid_ptr.emplace_back(sphere_thread->m_threadSystemId, sphere_thread);

	++m_threadCount;
}

/*
SphereThreadData* ThreadHolder::findThreadData(IThread* thread) noexcept
{
    // If checking for current thread, use another escamotage to retrieve it...

    // This should always run guarded by a MUTEX!
    SimpleThreadLock lock(m_mutex);
    for (size_t i = 0; i < m_threadCount; ++i)
    {
        if (m_threads[i].m_ptr == thread)
            return &(m_threads[i]);
    }
    return nullptr;
}
*/

void ThreadHolder::remove(IThread *thread)
{
    if (!thread)
        throw CSError(LOGL_FATAL, 0, "thread == nullptr");

    std::unique_lock<std::shared_mutex> lock(m_mutex);

    ASSERT(m_threadCount > 0);	// Trying to de-queue thread while no threads are active?

    auto sphere_thread = static_cast<AbstractSphereThread *>(thread);
    auto it_thread = std::find_if(
        m_spherethreadpairs_systemid_ptr.begin(),
        m_spherethreadpairs_systemid_ptr.end(),
        [sphere_thread](spherethreadpair_t const &elem) noexcept -> bool { return elem.second == sphere_thread; });
    ASSERT(it_thread != m_spherethreadpairs_systemid_ptr.end());

    auto it_data = std::find_if(m_threads.begin(), m_threads.end(),
        [thread](SphereThreadData const& elem) {
            return elem.m_ptr == thread;
        });
    ASSERT(it_data != m_threads.end());	// Ensure that the thread to de-queue is registered

	--m_threadCount;

	m_threads.erase(it_data);
    m_spherethreadpairs_systemid_ptr.erase(it_thread);
}

void ThreadHolder::markThreadsClosing()
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    m_closing = true;
	for (auto& thread_data : m_threads)
	{
		auto sphere_thread = static_cast<AbstractSphereThread*>(thread_data.m_ptr);
		sphere_thread->_fIsClosing = true;
		thread_data.m_closed = true;
	}
	//printf("Marking threads as closing.\n");
}

IThread * ThreadHolder::getThreadAt(size_t at)
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);

	if ( at > getActiveThreads() )
		return nullptr;

	for ( spherethreadlist_t::const_iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it )
	{
		if ( at == 0 )
			return it->m_ptr;

		--at;
	}

	return nullptr;
}


/*
 * AbstractThread
*/
int AbstractThread::m_threadsAvailable = 0;

AbstractThread::AbstractThread(const char *name, IThread::Priority priority)
{
	if( AbstractThread::m_threadsAvailable == 0 )
	{
		// no threads were started before - initialise thread subsystem
#ifdef _WIN32
		if( CoInitializeEx(nullptr, COINIT_MULTITHREADED) != S_OK )
		{
			throw CSError(LOGL_FATAL, 0, "OLE is not available, threading model unimplementable");
		}
#endif
		++AbstractThread::m_threadsAvailable;
	}
	m_threadSystemId = 0;
    Str_CopyLimitNull(m_name, name, sizeof(m_name));
	m_handle = 0;
	m_hangCheck = 0;
    _thread_selfTerminateAfterThisTick = true;
	m_terminateRequested = true;
	setPriority(priority);
}

AbstractThread::~AbstractThread()
{
	terminate(false);
	--AbstractThread::m_threadsAvailable;
	if( AbstractThread::m_threadsAvailable == 0 )
	{
		// all running threads have gone, the thread subsystem is no longer needed
#ifdef _WIN32
		CoUninitialize();
#else
		// No pthread equivalent
#endif
	}
}

void AbstractThread::overwriteInternalThreadName(const char* name) noexcept
{
    // Use it only if you know what you are doing!
    //  This doesn't actually do the change of the thread name!
    Str_CopyLimitNull(m_name, name, sizeof(m_name));
}

void AbstractThread::start()
{
#ifdef _WIN32
	m_handle = reinterpret_cast<spherethread_t>(_beginthreadex(nullptr, 0, &runner, this, 0, nullptr));
#else
	pthread_attr_t threadAttr;
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
	int result = pthread_create( &m_handle, &threadAttr, &runner, this );
	pthread_attr_destroy(&threadAttr);

	if (result != 0)
	{
		m_handle = 0;
		throw CSError(LOGL_FATAL, 0, "Unable to spawn a new thread");
	}
#endif

	m_terminateEvent.reset();
}

void AbstractThread::terminate(bool ended)
{
	if( isActive() )
	{
		bool wasCurrentThread = isCurrentThread();
		if (ended == false)
		{
			g_Log.Event(LOGL_WARN, "Forcing thread '%s' to terminate...\n", getName());

			// if the thread is current then terminating here will prevent cleanup from occurring
			if (wasCurrentThread == false)
			{
#ifdef _WIN32
				TerminateThread(m_handle, 0);
				CloseHandle(m_handle);
#else
				pthread_cancel(m_handle); // IBM say it so
#endif
			}
		}

		// Common things
		ThreadHolder::get().remove(this);
		m_threadSystemId = 0;
		m_handle = 0;

		// let everyone know we have been terminated
		m_terminateEvent.set();

		// current thread can be terminated now
		if (ended == false && wasCurrentThread)
		{
#ifdef _WIN32
			_endthreadex(EXIT_SUCCESS);
#else
			pthread_exit(EXIT_SUCCESS);
#endif
		}
	}
}

void AbstractThread::run()
{
	// is the very first since there is a possibility of something being altered there
	onStart();

	int exceptions = 0;
	bool lastWasException = false;
    _thread_selfTerminateAfterThisTick = false;
	m_terminateRequested = false;

	for (;;)
	{
        if (shouldExit())
            break;

		bool gotException = false;

		//	report me being alive if I am being checked for status
		if (m_hangCheck != 0)
		{
			m_hangCheck = 0;
		}

		try
		{
			tick();

            if (shouldExit())
                break;

            // ensure this is recorded as 'idle' time for this thread (ideally this should
            // be in tick() but we cannot guarantee it to be called there
            GetCurrentProfileData().Start(PROFILE_IDLE);
		}
        /*
        catch( const CAssert& e )
        {
            gotException = true;
            g_Log.CatchEvent(&e, "[TR] ExcType=CAssert in %s::tick", getName());
            GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
        }
        */
		catch( const CSError& e )
		{
			gotException = true;
			g_Log.CatchEvent(&e, "[TR] ExcType=CSError in %s::tick", getName());
			GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
		}
        catch( const std::exception& e )
        {
            gotException = true;
            g_Log.CatchStdException(&e, "[TR] ExcType=std::exception in %s::tick", getName());
            GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
        }
		catch( ... )
		{
			gotException = true;
			g_Log.CatchEvent(nullptr, "[TR] ExcType=pure in %s::tick", getName());
			GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
		}

		if( gotException )
		{
			if( lastWasException )
			{
				++exceptions;
			}
			else
			{
				lastWasException = true;
				exceptions = 0;
			}
			if( exceptions >= EXCEPTIONS_ALLOWED )
			{
				// a bad thing really happened. ALL previous EXCEPTIONS_ALLOWED ticks resulted in exception
				// almost for sure we have looped somewhere and have no way to get out from this situation
				// probably a thread restart can fix the problems
				// but there is no real need to restart a thread, we will just simulate a thread restart,
				// notifying a subclass like we have been just restarted, so it will restart it's operations
				g_Log.Event(LOGL_CRIT, "'%s' thread raised too many exceptions, restarting...\n", getName());
				onStart();
				lastWasException = false;
			}
		}
		else
		{
			lastWasException = false;
		}

		if( shouldExit() )
			break;

		m_sleepEvent.wait(m_tickPeriod);
	}
}

SPHERE_THREADENTRY_RETNTYPE AbstractThread::runner(void *callerThread)
{
	// If the caller thread is an AbstractThread, call run, which starts it and make it enter in its main loop
	AbstractThread * caller = reinterpret_cast<AbstractThread*>(callerThread);
	if (caller != nullptr)
	{
		caller->run();
		caller->terminate(true);
	}

	return 0;
}

bool AbstractThread::isActive() const
{
	return (m_handle != 0);
}

void AbstractThread::waitForClose()
{
    // Another thread has requested us to close and it's waiting for us to complete the current tick,
    //  or to forcefully be forcefully terminated after a THREADJOIN_TIMEOUT, which of the two happens first.

    // TODO? add a mutex here to protect at least the changes to m_terminateRequested?
	if (isActive())
	{
		if (isCurrentThread() == false)
		{
			// flag that we want the thread to terminate
			m_terminateRequested = true;
			awaken();

			// give the thread a chance to close on its own, and then
			// terminate anyway
			m_terminateEvent.wait(THREADJOIN_TIMEOUT);
		}

		terminate(false);
	}
}

void AbstractThread::awaken()
{
	m_sleepEvent.signal();
}

bool AbstractThread::isCurrentThread() const noexcept
{
#ifdef _WIN32
	return (getId() == ::GetCurrentThreadId());
#else
	return pthread_equal(m_handle,pthread_self());
#endif
}

bool AbstractThread::checkStuck()
{
	if( isActive() )
	{
		if( m_hangCheck == 0 )
		{
			//	initiate hang check
			m_hangCheck = 0xDEAD;
		}
		else if( m_hangCheck == 0xDEAD )
		{
			//	one time period was not answered, wait a bit more
			m_hangCheck = 0xDEADDEADl;
			//	TODO:
			//g_Log.Event(LOGL_CRIT, "'%s' thread seems being hang (frozen) at '%s'?\n", m_name, m_action);
		}
		else if( m_hangCheck == 0xDEADDEADl )
		{
			//	TODO:
			//g_Log.Event(LOGL_CRIT, "'%s' thread hang, restarting...\n", m_name);
			#ifdef THREAD_TRACK_CALLSTACK
				static_cast<AbstractSphereThread*>(this)->printStackTrace();
			#endif
			terminate(false);
			run();
			start();
			return true;
		}
	}

	return false;
}

void AbstractThread::onStart()
{
	// start-up actions for each thread
	// when implemented in derived classes this method must always be called too, preferably before
	// the custom implementation

	// we set the id here to ensure it is available before the first tick, otherwise there's
	// a small delay when setting it from AbstractThread::start and it's possible for the id
	// to not be set fast enough (particular when using pthreads)
	m_threadSystemId = getCurrentThreadSystemId();

    ThreadHolder::get().push(this);

	if (isActive())		// This thread has actually been spawned and the code is executing on a different thread
		setThreadName(getName());
}

void AbstractThread::setPriority(IThread::Priority pri)
{
	ASSERT(((pri >= IThread::Idle) && (pri <= IThread::RealTime)) || (pri == IThread::Disabled));
	m_priority = pri;

	// detect a sleep period for thread depending on priority
	switch( m_priority )
	{
		case IThread::Idle:
			m_tickPeriod = 1000;
			break;
		case IThread::Low:
			m_tickPeriod = 200;
			break;
		case IThread::Normal:
			m_tickPeriod = 100;
			break;
		case IThread::High:
			m_tickPeriod = 50;
			break;
		case IThread::Highest:
			m_tickPeriod = 5;
			break;
		case IThread::RealTime:
			m_tickPeriod = 0;
			break;
		case IThread::Disabled:
			m_tickPeriod = AutoResetEvent::_kiInfinite;
			break;
	}\
}

bool AbstractThread::shouldExit() noexcept
{
	return m_terminateRequested || _thread_selfTerminateAfterThisTick;
}

/*
 * AbstractSphereThread
*/
AbstractSphereThread::AbstractSphereThread(const char *name, Priority priority)
	: AbstractThread(name, priority), _fIsClosing(false)
{
#ifdef THREAD_TRACK_CALLSTACK
	m_stackPos = -1;
	memset(m_stackInfo, 0, sizeof(m_stackInfo));
	m_freezeCallStack = false;
    m_exceptionStackUnwinding = false;
#endif

	// profiles that apply to every thread
	m_profile.EnableProfile(PROFILE_IDLE);
	m_profile.EnableProfile(PROFILE_OVERHEAD);
	m_profile.EnableProfile(PROFILE_STAT_FAULTS);
}

AbstractSphereThread::~AbstractSphereThread()
{
	_fIsClosing = true;
}


char *AbstractSphereThread::allocateBuffer() noexcept
{
	auto* tsholder = TemporaryStringThreadSafeStateHolder::get();
	SimpleThreadLock stlBuffer(tsholder->g_tmpStringMutex);

	char * buffer = nullptr;
	tsholder->g_tmpStringIndex += 1;

	if (tsholder->g_tmpStringIndex >= THREAD_TSTRING_STORAGE )
	{
		tsholder->g_tmpStringIndex = tsholder->g_tmpStringIndex % THREAD_TSTRING_STORAGE;
	}

	buffer = tsholder->g_tmpStrings[tsholder->g_tmpStringIndex];
	*buffer = '\0';

	return buffer;
}

TemporaryStringThreadSafeStateHolder::TemporaryStringStorage *
getThreadRawStringBuffer()
{
	auto* tsholder = TemporaryStringThreadSafeStateHolder::get();
	SimpleThreadLock stlBuffer(tsholder->g_tmpStringMutex);

	int initialPosition = tsholder->g_tmpTemporaryStringIndex;
	int index;
	for (;;)
	{
		index = tsholder->g_tmpTemporaryStringIndex += 1;
		if(tsholder->g_tmpTemporaryStringIndex >= THREAD_STRING_STORAGE )
		{
			const int inc = tsholder->g_tmpTemporaryStringIndex % THREAD_STRING_STORAGE;
			tsholder->g_tmpTemporaryStringIndex = inc;
			index = inc;
		}

		if(tsholder->g_tmpTemporaryStringStorage[index].m_state == 0 )
		{
			auto* store = &tsholder->g_tmpTemporaryStringStorage[index];
			*store->m_buffer = '\0';
			return store;
		}

		// a protection against deadlock. All string buffers are marked as being used somewhere, so we
		// have few possibilities (the case shows that we have a bug and temporary strings used not such):
		// a) return nullptr and wait for exceptions in the program
		// b) allocate a string from a heap
		if( initialPosition == index )
		{
			// but the best is to throw an exception to give better formed information for end users
			// rather than access violations
			DEBUG_WARN(( "Thread temporary string buffer is full.\n" ));
			throw CSError(LOGL_FATAL, 0, "Thread temporary string buffer is full");
		}
	}
}

void AbstractSphereThread::getStringBuffer(TemporaryString &string) noexcept
{
	ADDTOCALLSTACK("alloc");
	auto* tsholder = TemporaryStringThreadSafeStateHolder::get();
	SimpleThreadLock stlBuffer(tsholder->g_tmpTemporaryStringMutex);

	auto* store = getThreadRawStringBuffer();
	string.init(store->m_buffer, &store->m_state);
}

bool AbstractSphereThread::shouldExit() noexcept
{
	if ( g_Serv.GetExitFlag() != 0 )
		return true;

	return AbstractThread::shouldExit();
}

void AbstractSphereThread::exceptionCaught()
{
#ifdef THREAD_TRACK_CALLSTACK
    if (m_exceptionStackUnwinding == false)
        printStackTrace();
    else
        m_exceptionStackUnwinding = false;
#endif
}

#include <iostream>
#include "../game/CServerConfig.h"

#ifdef THREAD_TRACK_CALLSTACK
static thread_local ssize_t _stackpos = -1;
void AbstractSphereThread::pushStackCall(const char *name) NOEXCEPT_NODEBUG
{
    if (m_freezeCallStack == true)
        return;

    DEBUG_ASSERT(m_stackPos >= -1);
    DEBUG_ASSERT(m_stackPos < (ssize_t)sizeof(m_stackInfo));
    ++m_stackPos;
    _stackpos = m_stackPos;
    m_stackInfo[m_stackPos].functionName = name;
}

void AbstractSphereThread::popStackCall() NOEXCEPT_NODEBUG
{
    if (m_freezeCallStack == true)
        return;

    --m_stackPos;
    _stackpos = m_stackPos;
    ASSERT(m_stackPos >= -1);

}

void AbstractSphereThread::exceptionNotifyStackUnwinding() noexcept
{
    //ASSERT(isCurrentThread());
    if (m_exceptionStackUnwinding == false)
    {
        m_exceptionStackUnwinding = true;
        printStackTrace();
    }
}

void AbstractSphereThread::printStackTrace() noexcept
{
	// don't allow call stack to be modified whilst we're printing it
	freezeCallStack(true);

    const uint64_t threadId = static_cast<uint64_t>(getId());
    const lpctstr threadName = getName();

    //EXC_NOTIFY_DEBUGGER;

	g_Log.EventDebug("Printing STACK TRACE for debugging purposes.\n");
	g_Log.EventDebug(" _______ thread (id) name _______ |  # | _____________ function _____________ |\n");
	for ( ssize_t i = 0; i < (ssize_t)ARRAY_COUNT(m_stackInfo); ++i )
	{
		if( m_stackInfo[i].functionName == nullptr )
			break;

        const bool origin = (i == (m_stackPos - 1));
        lpctstr extra = " ";
        if (origin)
        {
            if (m_exceptionStackUnwinding)
                extra = "<-- last function call (stack unwinding began here)";
            else
                extra = "<-- exception catch point (below is guessed and could be incorrect!)";
        }

		g_Log.EventDebug("(%" PRIx64 ") %16.16s | %2u | %36.36s | %s\n",
			threadId, threadName, (uint)i, m_stackInfo[i].functionName, extra);
	}

	freezeCallStack(false);
}
#endif

/*
 * DummySphereThread
*/
DummySphereThread *DummySphereThread::_instance = nullptr;

DummySphereThread::DummySphereThread()
	: AbstractSphereThread("dummy", IThread::Normal)
{
}

void DummySphereThread::createInstance() // static
{
	// This dummy thread is created at the very beginning of the application startup.
	//  Before the server becomes operational, this won't be used anymore, since fully functional threads will be created.

	// Create this only once, it has to be one of the first operations to be done when the application starts.
	ASSERT(_instance == nullptr);
	_instance = new DummySphereThread();
}

DummySphereThread *DummySphereThread::getInstance() noexcept // static
{
	return _instance;
}

void DummySphereThread::tick()
{
}


/*
* StackDebugInformation
*/

#ifdef THREAD_TRACK_CALLSTACK

StackDebugInformation::StackDebugInformation(const char *name) noexcept
	: m_context(nullptr)
{
    auto& th = ThreadHolder::get();
    if (th.closing())
        return;

    IThread *icontext = th.current();
	if (icontext == nullptr)
	{
		// Thread was deleted, manually or by app closing signal.
		return;
	}

	m_context = static_cast<AbstractSphereThread*>(icontext);
	if (m_context != nullptr && !m_context->closing())
	{
		m_context->pushStackCall(name);
        //printf("-  -  -  -  Pushing stack call: %s.\n", name);
	}
}

StackDebugInformation::~StackDebugInformation() noexcept
{
	if (!m_context || m_context->closing())
		return;

    if (std::uncaught_exceptions() != 0)
    {
        // Exception was thrown and stack unwinding is in progress.
        m_context->exceptionNotifyStackUnwinding();
    }
    m_context->popStackCall();
}

void StackDebugInformation::printStackTrace() noexcept // static
{
	IThread* pThreadState = ThreadHolder::get().current();
	if (pThreadState)
		static_cast<AbstractSphereThread*>(pThreadState)->printStackTrace();
}

void StackDebugInformation::freezeCallStack(bool freeze) noexcept // static
{
	IThread* pThreadState = ThreadHolder::get().current();
	if (pThreadState)
		static_cast<AbstractSphereThread*>(pThreadState)->freezeCallStack(freeze);
}

#endif // THREAD_TRACK_CALLSTACK
