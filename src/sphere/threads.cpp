#ifdef _WIN32
    // this thing is somehow required to be able to initialise OLE
    #define _WIN32_DCOM
#endif

#include "../common/sphere_library/sresetevents.h"
#include "../common/sphere_library/sstringobjs.h"
#include "../common/basic_threading.h"
#include "../common/CException.h"
#include "../common/CLog.h"
#include "../game/CServer.h"
#include "ProfileTask.h"
#include "threads.h"

#if defined(_WIN32)
	#include <process.h>
	#include <objbase.h>
#elif !defined(_BSD) && !defined(__APPLE__)
    #include <sys/prctl.h>  // to set thread name
#endif

#include <algorithm>
#include <system_error>


// number of exceptions after which we restart thread and think that the thread have gone in exceptioning loops
#define EXCEPTIONS_ALLOWED	10

// number of milliseconds to wait for a thread to close
#define THREADJOIN_TIMEOUT	60000

// temporary string storage
#define THREAD_TEMPSTRING_C_STORAGE     2048
#define THREAD_TEMPSTRING_OBJ_STORAGE   1024


// This implementation doesn't reserve preallocated strings for each thread or attached to a specific thread,
// but it creates a pool of preallocated strings which access is guarded by a Mutex.
struct TemporaryStringsThreadSafeStateHolder
{
    // C-style string Buffer (char array)
    SimpleMutex g_tmpCStringMutex;
    std::atomic<uint> g_tmpCStringIndex;
    std::unique_ptr<char[]> g_tmpCStrings;

	// TemporaryString Buffer
	SimpleMutex g_tmpTemporaryStringMutex;
    std::atomic<uint> g_tmpTemporaryStringIndex;
	struct TemporaryStringStorage
	{
		char m_buffer[THREAD_STRING_LENGTH];
		char m_state;
    };
    std::unique_ptr<TemporaryStringStorage[]> g_tmpTemporaryStringStorage;

private:
    TemporaryStringsThreadSafeStateHolder() :
        g_tmpCStringIndex(0), g_tmpTemporaryStringIndex(0)
    {
        g_tmpCStrings = std::make_unique<char[]>(THREAD_TEMPSTRING_C_STORAGE * THREAD_STRING_LENGTH);
        g_tmpTemporaryStringStorage = std::make_unique<TemporaryStringStorage[]>(THREAD_TEMPSTRING_OBJ_STORAGE);
    }

public:
    static TemporaryStringsThreadSafeStateHolder& get() noexcept
    {
        static TemporaryStringsThreadSafeStateHolder instance;
        return instance;
    }
};


/**
 * ThreadHolder
**/

ThreadHolder::ThreadHolder() noexcept :
	m_threadCount(0), m_closingThreads(false)
{
    // While we keep this as a global state and do not decide to get a set of string buffers attached to each AbstractSphereThread,
    //  we have to ensure that we construct the global string buffer holder as soon as possible.
    TemporaryStringsThreadSafeStateHolder::get();
}

ThreadHolder& ThreadHolder::get() noexcept
{
	static ThreadHolder instance;
	return instance;
}

bool ThreadHolder::closing() noexcept
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    volatile auto ret = m_closingThreads;
    return ret;
}

AbstractThread* ThreadHolder::current() noexcept
{
    // Do not use ASSERTs here, would cause recursion.

    // RETRY_SHARED_LOCK_FOR_TASK is used to try to not make mutex lock fail and, if needed,
    //  handle failure while allowing this function to be noexcept.

    AbstractThread* retval = nullptr;
    RETRY_SHARED_LOCK_FOR_TASK(m_mutex, lock, retval,
        ([this, &lock]() -> AbstractThread*
        {
            const threadid_t tid = AbstractThread::getCurrentThreadSystemId();

            if (m_spherethreadpairs_systemid_ptr.empty())
            [[unlikely]]
            {
                if (m_closingThreads) [[unlikely]]
                {
                    //STDERR_LOG("Closing?\n");
                    return nullptr;
                }

                auto thread = static_cast<AbstractThread*>(DummySphereThread::getInstance());
                if (!thread)
                [[unlikely]]
                {
                    // Should never happen.
                    RaiseImmediateAbort(11);
                }

                thread->m_threadSystemId = tid;
                lock.unlock();
                push(thread);
                return thread;
            }

            spherethreadpair_t *found = nullptr;
            for (auto &elem : m_spherethreadpairs_systemid_ptr)
            {
                if (elem.first == tid)
                {
                    found = &elem;
                    break;
                }
            }

            if (!found)
            [[unlikely]]
            {
                //throw CSError(LOGL_FATAL, 0, "Thread handle not found in vector?");
                //STDERR_LOG("Thread handle not found in vector?");

                // Should never happen.
                RaiseImmediateAbort(12);
            }

            auto thread = static_cast<AbstractThread *>(found->second);

            ASSERT(thread->m_threadHolderId != -1);
            SphereThreadData *tdata = &(m_threads[thread->m_threadHolderId]);
            if (tdata->m_closed)
            [[unlikely]]
            {
                //STDERR_LOG("Closed? Idx %u, Name %s.\n", thread->m_threadHolderId, thread->getName());
                return nullptr;
            }

            if (m_closingThreads) [[unlikely]]
            {
                auto spherethread = dynamic_cast<AbstractSphereThread *>(thread);
                if (!spherethread)
                {
                    // Should never happen.
                    RaiseImmediateAbort(13);
                }

                if (!spherethread->_fKeepAliveAtShutdown)
                {
                    //STDERR_LOG("Closing?\n");
                    return nullptr;
                }
            }

            // Uncomment it only for testing purposes, since this method is called very often and we don't need the additional overhead
            //DEBUG_ASSERT( thread->isSameThread(thread->getId()) );

            return thread;
    }));

    return retval;
}

void ThreadHolder::push(AbstractThread *thread) noexcept
{
    bool fExceptionThrown = false;
    try
    {
        auto sphere_thread = dynamic_cast<AbstractSphereThread*>(thread);
        if (!sphere_thread)
        {
            //throw CSError(LOGL_FATAL, 0, "AbstractThread not being an AbstractSphereThread?");
            STDERR_LOG("AbstractThread not being an AbstractSphereThread?");
            //fExceptionThrown = true;
            goto soft_throw;
        }

        std::unique_lock<std::shared_mutex> lock(m_mutex);

        ASSERT(thread->m_threadSystemId != 0);
        ASSERT(thread->m_threadHolderId == -1);

        m_threads.emplace_back( SphereThreadData{ thread, false });
        thread->m_threadHolderId = m_threadCount;

#ifdef _DEBUG
        auto it_thread = std::find_if(
            m_spherethreadpairs_systemid_ptr.begin(),
            m_spherethreadpairs_systemid_ptr.end(),
            [sphere_thread](spherethreadpair_t const &elem) noexcept -> bool { return elem.second == sphere_thread; });

        // I don't want duplicates.
        DEBUG_ASSERT(it_thread == m_spherethreadpairs_systemid_ptr.end());
#endif
        m_spherethreadpairs_systemid_ptr.emplace_back(sphere_thread->m_threadSystemId, sphere_thread);

        ++ m_threadCount;

    }
    catch (CAssert const& e)
    {
        fExceptionThrown = true;
        lptstr ptcBuf = Str_GetTemp();
        e.GetErrorMessage(ptcBuf, usize_narrow_u32(Str_TempLength()));
        STDERR_LOG("ASSERT failed.\n%s\n", ptcBuf);
    }
    catch (std::system_error const& e)
    {
        fExceptionThrown = true;
        STDERR_LOG("Mutex cannot be acquired. Err: '%s'.\n", e.what());
    }
    catch (...)
    {
        fExceptionThrown = true;
        STDERR_LOG("Unknown exception thrown.\n");
    }

    if (fExceptionThrown)
    {
soft_throw:
        // Should never happen.
        RaiseImmediateAbort(14);
    }

#ifdef _DEBUG
    if (dynamic_cast<DummySphereThread const*>(thread))
    {
        // Too early in the init process to use the console...
        printf("Registered thread '%s' with ThreadHolder ID %d.\n",
            thread->getName(), thread->m_threadHolderId);
    }
    else
    {
        g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGF_CONSOLE_ONLY,
                    "Registered thread '%s' with ThreadHolder ID %d.\n",
                    thread->getName(), thread->m_threadHolderId);
    }
#endif
}

void ThreadHolder::remove(AbstractThread *thread) CANTHROW
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

void ThreadHolder::markThreadsClosing() CANTHROW
{
	g_Log.Event(LOGM_INIT|LOGM_NOCONTEXT|LOGL_EVENT, "Marking threads as closing.\n");

    std::unique_lock<std::shared_mutex> lock(m_mutex);

    m_closingThreads = true;
	for (auto& thread_data : m_threads)
	{
		auto sphere_thread = static_cast<AbstractSphereThread*>(thread_data.m_ptr);
        if (sphere_thread->_fKeepAliveAtShutdown)
            continue;

		sphere_thread->_fIsClosing = true;
		thread_data.m_closed = true;
	}
}

AbstractThread * ThreadHolder::getThreadAt(size_t at) noexcept
{
    AbstractThread* retval = nullptr;
    RETRY_SHARED_LOCK_FOR_TASK(m_mutex, lock, retval,
        ([this, at]() -> AbstractThread*
        {
// MSVC: warning C5101: use of preprocessor directive in function-like macro argument list is undefined behavior.
//#ifdef _DEBUG
            if (getActiveThreads() != m_threads.size())
            [[unlikely]]
            {
                STDERR_LOG("Active threads %" PRIuSIZE_T ", threads container size %" PRIuSIZE_T ".\n", getActiveThreads(), m_threads.size());
                RaiseImmediateAbort(15);
            }
//#endif

            if ( at > getActiveThreads() )
            [[unlikely]]
            {
                return nullptr;
            }


            /*
            for ( spherethreadlist_t::const_iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it )
            {
                if ( at == 0 )
                    return it->m_ptr;

                --at;
            }
            return nullptr;
            */
            return m_threads[at].m_ptr;
    }));

    return retval;
}


/*
 * AbstractThread
*/
int AbstractThread::m_threadsAvailable = 0;

AbstractThread::AbstractThread(const char *name, ThreadPriority priority) :
    m_threadSystemId(0), m_threadHolderId(-1),
    _fKeepAliveAtShutdown(false), _fIsClosing(false)
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
	m_hangCheck = 0;
    _thread_selfTerminateAfterThisTick = true;
	m_terminateRequested = true;
	setPriority(priority);
    m_sleepEvent = std::make_unique<AutoResetEvent>();
    m_terminateEvent = std::make_unique<ManualResetEvent>();
}

AbstractThread::~AbstractThread()
{
#ifdef _DEBUG
    fprintf(stdout, "DEBUG: Destroying thread '%s' with ThreadHolder ID %d and system ID %" PRIu64 ".\n",
        getName(), m_threadHolderId, (uint64)m_threadSystemId);
    fflush(stdout);
#endif

    terminate(true);
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
    //ThreadHolder::get().push(this);
    //printf("Starting thread '%s' with ThreadHolder ID %d.\n",
    //                 getName(), m_threadHolderId);
    //fflush(stdout);

#ifdef _WIN32
	m_handle = reinterpret_cast<spherethread_t>(_beginthreadex(nullptr, 0, &runner, this, 0, nullptr));
#else
	pthread_attr_t threadAttr;
	pthread_attr_init(&threadAttr);
	pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    spherethread_t threadHandle{};
    int result = pthread_create( &threadHandle, &threadAttr, &runner, this );
	pthread_attr_destroy(&threadAttr);

	if (result != 0)
	{
        m_handle = std::nullopt;
		throw CSError(LOGL_FATAL, 0, "Unable to spawn a new thread");
	}
    m_handle = threadHandle;
#endif

    m_terminateEvent->reset();
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
                TerminateThread(m_handle.value(), 0);
                CloseHandle(m_handle.value());
#else
                pthread_cancel(m_handle.value()); // IBM say it so
#endif
			}
		}

        // Common things
        ThreadHolder::get().remove(this);
        m_threadSystemId = 0;
        m_handle = std::nullopt;

		// let everyone know we have been terminated
        m_terminateEvent->set();

		// current thread can be terminated now
		if (ended == false && wasCurrentThread)
		{
#ifdef _WIN32
			_endthreadex(EXIT_SUCCESS);
#else
            //exit(EXIT_SUCCESS)
			pthread_exit(nullptr);
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

        m_sleepEvent->wait(m_tickPeriod);
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

#ifdef _WIN32
    return 0;
#else
	return nullptr;
#endif
}

bool AbstractThread::isActive() const
{
    return m_handle.has_value();
}

void AbstractThread::waitForClose()
{
    // Another thread has requested us to close and it's waiting for us to complete the current tick,
    //  or to forcefully be forcefully terminated after a THREADJOIN_TIMEOUT, which of the two happens first.

	if (isActive())
	{
		if (isCurrentThread() == false)
		{
			// flag that we want the thread to terminate
			m_terminateRequested = true;
			awaken();

			// give the thread a chance to close on its own, and then
			// terminate anyway
            m_terminateEvent->wait(THREADJOIN_TIMEOUT);
		}

		terminate(false);
	}
}

void AbstractThread::awaken()
{
    m_sleepEvent->signal();
}

bool AbstractThread::isCurrentThread() const noexcept
{
#ifdef _WIN32
	return (getId() == ::GetCurrentThreadId());
#else
    return m_handle.has_value() && pthread_equal(m_handle.value(), pthread_self());
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
            //	TODO: really ugly static_cast...
            g_Log.Event(LOGL_CRIT, "'%s' thread hang, restarting...\n", m_name);
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

#ifdef _DEBUG
    g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGF_CONSOLE_ONLY,
                    "Started thread '%s' with ThreadHolder ID %d and system ID %" PRIu64 ".\n",
                     getName(), m_threadHolderId, (uint64)m_threadSystemId);
#endif
}

void AbstractThread::setPriority(ThreadPriority pri)
{
    ASSERT(((pri >= ThreadPriority::Idle) && (pri <= ThreadPriority::RealTime)) || (pri == ThreadPriority::Disabled));
	m_priority = pri;

	// detect a sleep period for thread depending on priority
	switch( m_priority )
	{
        case ThreadPriority::Idle:
			m_tickPeriod = 1000;
			break;
        case ThreadPriority::Low:
			m_tickPeriod = 200;
			break;
        case ThreadPriority::Normal:
			m_tickPeriod = 100;
			break;
        case ThreadPriority::High:
			m_tickPeriod = 50;
			break;
        case ThreadPriority::Highest:
			m_tickPeriod = 5;
			break;
        case ThreadPriority::RealTime:
			m_tickPeriod = 0;
			break;
        case ThreadPriority::Disabled:
			m_tickPeriod = AutoResetEvent::_kiInfinite;
			break;
    }
}

bool AbstractThread::shouldExit() noexcept
{
	return closing() || m_terminateRequested || _thread_selfTerminateAfterThisTick;
}

void AbstractThread::setThreadName(const char* name)
{
    // register the thread name

    // Unix uses prctl to set thread name
    // thread name must be 16 bytes, zero-padded if shorter
    char name_trimmed[m_nameMaxLength] = { '\0' };	// m_nameMaxLength = 16
    Str_CopyLimitNull(name_trimmed, name, m_nameMaxLength);

#if defined(_WIN32)
#if defined(MSVC_COMPILER)
    // TODO: support thread naming when compiling with compilers other than Microsoft's

    // Windows uses THREADNAME_INFO structure to set thread name
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
#endif // MSVC_COMPILER
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

#ifdef _DEBUG
    g_Log.Event(LOGF_CONSOLE_ONLY|LOGM_DEBUG|LOGL_EVENT,
        "Setting thread (ThreadHolder ID %d, internal name '%s') system name: '%s'.\n",
        athr->m_threadHolderId, athr->getName(), name_trimmed);
#endif
    athr->overwriteInternalThreadName(name_trimmed);
}


/*
 * AbstractSphereThread
*/
AbstractSphereThread::AbstractSphereThread(const char *name, ThreadPriority priority)
    : AbstractThread(name, priority)
#ifdef THREAD_TRACK_CALLSTACK
    , m_stackInfo{}, m_stackInfoCopy{}, m_iStackPos(-1),
    m_fFreezeCallStack(false),
    m_iStackUnwindingStackPos(-1), m_iCaughtExceptionStackPos(-1)
#endif
{
	// profiles that apply to every thread
	m_profile.EnableProfile(PROFILE_IDLE);
	m_profile.EnableProfile(PROFILE_OVERHEAD);
	m_profile.EnableProfile(PROFILE_STAT_FAULTS);
}

AbstractSphereThread::~AbstractSphereThread()
{
    AbstractThread::_fIsClosing = true;
}


static auto getThreadRawStringBuffer() -> TemporaryStringsThreadSafeStateHolder::TemporaryStringStorage *
{
    auto& tsholder = TemporaryStringsThreadSafeStateHolder::get();
    SimpleThreadLock stlBuffer(tsholder.g_tmpCStringMutex);

    int initialPosition = tsholder.g_tmpTemporaryStringIndex;
    int index;
    for (;;)
    {
        index = tsholder.g_tmpTemporaryStringIndex += 1;
        if(tsholder.g_tmpTemporaryStringIndex >= THREAD_TEMPSTRING_OBJ_STORAGE )
        {
            const int inc = tsholder.g_tmpTemporaryStringIndex % THREAD_TEMPSTRING_OBJ_STORAGE;
            tsholder.g_tmpTemporaryStringIndex = inc;
            index = inc;
        }

        if(tsholder.g_tmpTemporaryStringStorage[index].m_state == 0 )
        {
            auto* store = &(tsholder.g_tmpTemporaryStringStorage[index]);
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

char *AbstractSphereThread::Strings::allocateBuffer() noexcept
{
    auto& tsholder = TemporaryStringsThreadSafeStateHolder::get();
    SimpleThreadLock stlBuffer(tsholder.g_tmpCStringMutex);

	char * buffer = nullptr;
    tsholder.g_tmpCStringIndex += 1;

    if (tsholder.g_tmpCStringIndex >= THREAD_TEMPSTRING_C_STORAGE )
	{
        tsholder.g_tmpCStringIndex = tsholder.g_tmpCStringIndex % THREAD_TEMPSTRING_C_STORAGE;
	}

    //buffer = tsholder->g_tmpStrings.get() + (tsholder->g_tmpStringIndex * THREAD_STRING_LENGTH);
    if (!tsholder.g_tmpCStrings) {
        // I shouldn't even try to do this. Maybe i'm at the end of server shutdown.
        RaiseImmediateAbort(20);
        //return nullptr;
    }

    buffer = &(tsholder.g_tmpCStrings[tsholder.g_tmpCStringIndex * THREAD_TEMPSTRING_C_STORAGE]);
	*buffer = '\0';

	return buffer;
}

void AbstractSphereThread::Strings::getBufferForStringObject(TemporaryString &string) noexcept
{
	ADDTOCALLSTACK("alloc");
    auto& tsholder = TemporaryStringsThreadSafeStateHolder::get();
    SimpleThreadLock stlBuffer(tsholder.g_tmpTemporaryStringMutex);

	auto* store = getThreadRawStringBuffer();
	string.init(store->m_buffer, &store->m_state);
}


bool AbstractSphereThread::shouldExit() noexcept
{
	if ( g_Serv.GetExitFlag() != 0 )
		return true;

	return AbstractThread::shouldExit();
}

#ifdef THREAD_TRACK_CALLSTACK
void AbstractSphereThread::signalExceptionCaught() noexcept
{
    if (m_iStackPos < 0 || (m_iStackPos >= (ssize_t)ARRAY_COUNT(m_stackInfo)))
        return;
    m_iCaughtExceptionStackPos = std::max(m_iStackPos, m_iCaughtExceptionStackPos);

    printStackTrace();

    memset(m_stackInfoCopy, 0, sizeof(m_stackInfo));
    m_iCaughtExceptionStackPos = -1;
    m_iStackUnwindingStackPos = -1;
}

void AbstractSphereThread::signalExceptionStackUnwinding() noexcept
{
    //ASSERT(isCurrentThread());
    if (m_iStackPos < 0 || (m_iStackPos >= (ssize_t)ARRAY_COUNT(m_stackInfo)))
        return;
    m_iStackUnwindingStackPos = std::max(m_iStackPos, m_iStackUnwindingStackPos);
    memcpy(m_stackInfoCopy, m_stackInfo, sizeof(m_stackInfo));
}

static thread_local ssize_t _stackpos = -1;
void AbstractSphereThread::pushStackCall(const char *name) noexcept
{
    if (m_fFreezeCallStack == true) [[unlikely]] {
        return;
    }

#ifdef _DEBUG
    if (m_iStackPos < -1) [[unlikely]] {
        RaiseImmediateAbort(16);
    }
    if (m_iStackPos >= (ssize_t)ARRAY_COUNT(m_stackInfo)) [[unlikely]] {
        RaiseImmediateAbort(17);
    }
#endif

    ++m_iStackPos;
    _stackpos = m_iStackPos;
    m_stackInfo[m_iStackPos].functionName = name;
}

void AbstractSphereThread::popStackCall() NOEXCEPT_NODEBUG
{
    if (m_fFreezeCallStack == true)
        return;

    --m_iStackPos;
    _stackpos = m_iStackPos;
    DEBUG_ASSERT(m_iStackPos >= -1);

}

void AbstractSphereThread::printStackTrace() noexcept
{
	// don't allow call stack to be modified whilst we're printing it
	freezeCallStack(true);

    const uint64_t threadId = static_cast<uint64_t>(getId());
    const lpctstr threadName = getName();

    //EXC_NOTIFY_DEBUGGER;

    auto& stackInfo = (m_stackInfoCopy[0].functionName != nullptr) ? m_stackInfoCopy : m_stackInfo;

	g_Log.EventDebug("Printing STACK TRACE for debugging purposes.\n");
    g_Log.EventDebug(" ______ thread (id) name _____ |   # | _____________ function _____________ |\n");
    for ( ssize_t i = 0; i < (ssize_t)ARRAY_COUNT(m_stackInfoCopy); ++i )
	{
        if( stackInfo[i].functionName == nullptr )
			break;

        lpctstr extra = "";
        if (i == m_iStackUnwindingStackPos) {
            extra = "<-- last function call (stack unwinding detected here)";
        }
        else if (i == m_iCaughtExceptionStackPos) {
            if (m_iStackUnwindingStackPos == -1)
                extra = "<-- exception catch point (below is guessed and could be incorrect!)";
            else
                extra = "<-- exception catch point";
        }
        /*
        const bool origin = (i == (m_iStackPos - 1));
        if (origin)
        {
            if (m_uisignalExceptionStackUnwinding)
                extra = "<-- last function call (stack unwinding began here)";
            else
                extra = "<-- exception catch point (below is guessed and could be incorrect!)";
        }
        */

        g_Log.EventDebug("(%" PRIx64 ") %15.15s | %3u | %36.36s | %s\n",
            threadId, threadName, (uint)i, stackInfo[i].functionName, extra);

        if (i == m_iStackUnwindingStackPos)
        {
            // Stop logging/writing functions called after the exception throw...
            break;
        }
	}

	freezeCallStack(false);
}
#endif

/*
 * DummySphereThread
*/
DummySphereThread *DummySphereThread::_instance = nullptr;

DummySphereThread::DummySphereThread()
    : AbstractSphereThread("dummy", ThreadPriority::Normal)
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
    STATIC_ASSERT_NOEXCEPT_CONSTRUCTOR(StackDebugInformation, const char*);
    STATIC_ASSERT_NOEXCEPT_MEMBER_FUNCTION(AbstractSphereThread, pushStackCall, const char*);
    STATIC_ASSERT_NOEXCEPT_FREE_FUNCTION(ThreadHolder::get);
    STATIC_ASSERT_NOEXCEPT_MEMBER_FUNCTION(ThreadHolder, current);

    auto& th = ThreadHolder::get();
    if (th.closing()) [[unlikely]]
        return;

    AbstractThread *icontext = th.current();
    if (icontext == nullptr)
    [[unlikely]]
	{
		// Thread was deleted, manually or by app closing signal.
		return;
	}

	m_context = static_cast<AbstractSphereThread*>(icontext);
    if (m_context != nullptr && !m_context->closing())
	{
        [[unlikely]]
		m_context->pushStackCall(name);
	}
}

StackDebugInformation::~StackDebugInformation() noexcept
{
	if (!m_context || m_context->closing()) [[unlikely]]
		return;

    if (!m_context->isExceptionStackUnwinding())
    {
        if (std::uncaught_exceptions() != 0) [[unlikely]]
        {
            // Exception was thrown and stack unwinding is in progress.
            m_context->signalExceptionStackUnwinding();
        }
    }
    m_context->popStackCall();
}

void StackDebugInformation::printStackTrace() noexcept // static
{
    AbstractThread* pThreadState = ThreadHolder::get().current();
	if (pThreadState)
		static_cast<AbstractSphereThread*>(pThreadState)->printStackTrace();
}

void StackDebugInformation::freezeCallStack(bool freeze) noexcept // static
{
    AbstractThread* pThreadState = ThreadHolder::get().current();
	if (pThreadState)
		static_cast<AbstractSphereThread*>(pThreadState)->freezeCallStack(freeze);
}

#endif // THREAD_TRACK_CALLSTACK
