// this thing is somehow required to be able to initialise OLE
#define _WIN32_DCOM

#include <algorithm>
#include "../common/CException.h"
#include "../common/common.h"
#include "../common/CLog.h"
#include "../game/CServer.h"
#include "ProfileTask.h"
#include "threads.h"

#if defined(_WIN32)
	#include <objbase.h>
#elif !defined(_BSD)
	#include <sys/prctl.h>
#endif

// number of exceptions after which we restart thread and think that the thread have gone in exceptioning loops
#define EXCEPTIONS_ALLOWED	10

// number of milliseconds to wait for a thread to close
#define THREADJOIN_TIMEOUT	60000


// Normal Buffer
SimpleMutex g_tmpStringMutex;
volatile int g_tmpStringIndex = 0;
char g_tmpStrings[THREAD_TSTRING_STORAGE][THREAD_STRING_LENGTH];

// TemporaryString Buffer
SimpleMutex g_tmpTemporaryStringMutex;
volatile int g_tmpTemporaryStringIndex = 0;

struct TemporaryStringStorage
{
	char m_buffer[THREAD_STRING_LENGTH];
	char m_state;
} g_tmpTemporaryStringStorage[THREAD_STRING_STORAGE];



threadid_t IThread::getCurrentThreadId()
{
#ifdef _WIN32
	return ::GetCurrentThreadId();
#else
	return pthread_self();
#endif
}

bool IThread::isSameThreadId(threadid_t firstId, threadid_t secondId)
{
#ifdef _WIN32
	return (firstId == secondId);
#else
	return pthread_equal(firstId,secondId);
#endif
}

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

const dword MS_VC_EXCEPTION = 0x406D1388;
#endif

void IThread::setThreadName(const char* name)
{
	// register the thread name

	// Unix uses prctl to set thread name
	// thread name must be 16 bytes, zero-padded if shorter
	char name_trimmed[m_nameMaxLength] = { '\0' };	// m_nameMaxLength = 16
	strcpylen(name_trimmed, name, m_nameMaxLength);

#if defined(_WIN32)
#if defined(_MSC_VER)	// TODO: support thread naming when compiling with compilers other than Microsoft
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
}


/**
 * ThreadHolder
**/
spherethreadlist_t ThreadHolder::m_threads;
size_t ThreadHolder::m_threadCount = 0;
bool ThreadHolder::m_inited = false;
SimpleMutex ThreadHolder::m_mutex;
TlsValue<IThread *> ThreadHolder::m_currentThread;

extern CLog g_Log;

IThread *ThreadHolder::current()
{
	init();

	IThread * thread = m_currentThread;
	if (thread == NULL)
		return DummySphereThread::getInstance();

	ASSERT( thread->isSameThread(thread->getId()) );

	return thread;
}

void ThreadHolder::push(IThread *thread)
{
	init();

	SimpleThreadLock lock(m_mutex);
	m_threads.push_back(thread);
	m_threadCount++;
}

void ThreadHolder::pop(IThread *thread)
{
	init();
	if( m_threadCount <= 0 )
		throw CSError(LOGL_ERROR, 0, "Trying to dequeue thread while no threads are active");

	SimpleThreadLock lock(m_mutex);
	spherethreadlist_t::iterator it = std::find(m_threads.begin(), m_threads.end(), thread);
	if (it != m_threads.end())
	{
		m_threadCount--;
		m_threads.erase(it);
		return;
	}

	throw CSError(LOGL_ERROR, 0, "Unable to dequeue a thread (not registered)");
}

IThread * ThreadHolder::getThreadAt(size_t at)
{
	if ( at > getActiveThreads() )
		return NULL;

	SimpleThreadLock lock(m_mutex);
	for ( spherethreadlist_t::const_iterator it = m_threads.begin(), end = m_threads.end(); it != end; ++it )
	{
		if ( at == 0 )
			return *it;

		--at;
	}

	return NULL;
}

void ThreadHolder::init()
{
	if( !m_inited )
	{
		memset(g_tmpStrings, 0, sizeof(g_tmpStrings));
		memset(g_tmpTemporaryStringStorage, 0, sizeof(g_tmpTemporaryStringStorage));

		m_inited = true;
	}
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
		if( CoInitializeEx(NULL, COINIT_MULTITHREADED) != S_OK )
		{
			throw CSError(LOGL_FATAL, 0, "OLE is not available, threading model unimplementable");
		}
#endif
		AbstractThread::m_threadsAvailable++;
	}
	m_id = 0;
	m_name = name;
	m_handle = 0;
	m_hangCheck = 0;
	m_terminateRequested = true;
	setPriority(priority);
}

AbstractThread::~AbstractThread()
{
	terminate(false);
	AbstractThread::m_threadsAvailable--;
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

void AbstractThread::start()
{
#ifdef _WIN32
	m_handle = reinterpret_cast<spherethread_t>(_beginthreadex(NULL, 0, &runner, this, 0, NULL));
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
	ThreadHolder::push(this);
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
		ThreadHolder::pop(this);
		m_id = 0;
		m_handle = 0;

		// let everyone know we have been terminated
		m_terminateEvent.set();

		// current thread can be terminated now
		if (ended == false && wasCurrentThread)
		{
#ifdef _WIN32
			_endthreadex(0);
#else
			pthread_exit(0);
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
	m_terminateRequested = false;

	for (;;)
	{
		bool gotException = false;

		//	report me being alive if I am being checked for status
		if( m_hangCheck != 0 )
		{
			m_hangCheck = 0;
		}

		try
		{
			tick();

			// ensure this is recorded as 'idle' time (ideally this should
			// be in tick() but we cannot guarantee it to be called there
			CurrentProfileData.Start(PROFILE_IDLE);
		}
		catch( const CSError& e )
		{
			gotException = true;
			g_Log.CatchEvent(&e, "%s::tick", getName());
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}
		catch( ... )
		{
			gotException = true;
			g_Log.CatchEvent(NULL, "%s::tick", getName());
			CurrentProfileData.Count(PROFILE_STAT_FAULTS, 1);
		}

		if( gotException )
		{
			if( lastWasException )
			{
				exceptions++;
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
	if (caller != NULL)
	{
		caller->run();
		caller->terminate(true);
	}

	return 0;
}

bool AbstractThread::isActive() const
{
	return m_handle != 0;
}

void AbstractThread::waitForClose()
{
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

bool AbstractThread::isCurrentThread() const
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
	m_id = getCurrentThreadId();
	ThreadHolder::m_currentThread = this;

	if (isActive())		// This thread has actually been spawned and the code is executing on a different thread
		setThreadName(getName());
}

void AbstractThread::setPriority(IThread::Priority pri)
{
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
			m_tickPeriod = AutoResetEvent::_infinite;
			break;
		default:
			throw CSError(LOGL_FATAL, 0, "Unable to determine thread priority");
	}
}

bool AbstractThread::shouldExit()
{
	return m_terminateRequested;
}

/*
 * AbstractSphereThread
*/
AbstractSphereThread::AbstractSphereThread(const char *name, Priority priority)
	: AbstractThread(name, priority)
{
#ifdef THREAD_TRACK_CALLSTACK
	m_stackPos = 0;
	memset(m_stackInfo, 0, sizeof(m_stackInfo));
	m_freezeCallStack = false;
#endif

	// profiles that apply to every thread
	m_profile.EnableProfile(PROFILE_IDLE);
	m_profile.EnableProfile(PROFILE_OVERHEAD);
	m_profile.EnableProfile(PROFILE_STAT_FAULTS);
}

// IMHO we need a lock on allocateBuffer and allocateStringBuffer

char *AbstractSphereThread::allocateBuffer()
{
	SimpleThreadLock stlBuffer(g_tmpStringMutex);

	char * buffer = NULL;
	g_tmpStringIndex++;

	if( g_tmpStringIndex >= THREAD_TSTRING_STORAGE )
	{
		g_tmpStringIndex %= THREAD_TSTRING_STORAGE;
	}

	buffer = g_tmpStrings[g_tmpStringIndex];
	*buffer = '\0';

	return buffer;
}

TemporaryStringStorage *AbstractSphereThread::allocateStringBuffer()
{
	int initialPosition = g_tmpTemporaryStringIndex;
	int index;
	for (;;)
	{
		index = ++g_tmpTemporaryStringIndex;
		if( g_tmpTemporaryStringIndex >= THREAD_STRING_STORAGE )
		{
			index = g_tmpTemporaryStringIndex %= THREAD_STRING_STORAGE;
		}

		if( g_tmpTemporaryStringStorage[index].m_state == 0 )
		{
			TemporaryStringStorage * store = &g_tmpTemporaryStringStorage[index];
			*store->m_buffer = '\0';
			return store;
		}

		// a protection against deadlock. All string buffers are marked as being used somewhere, so we
		// have few possibilities (the case shows that we have a bug and temporary strings used not such):
		// a) return NULL and wait for exceptions in the program
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

void AbstractSphereThread::allocateString(TemporaryString &string)
{
	SimpleThreadLock stlBuffer(g_tmpTemporaryStringMutex);

	TemporaryStringStorage * store = allocateStringBuffer();
	string.init(store->m_buffer, &store->m_state);
}

bool AbstractSphereThread::shouldExit()
{
	if ( g_Serv.m_iModeCode == SERVMODE_Exiting )
		return true;

	return AbstractThread::shouldExit();
}

#ifdef THREAD_TRACK_CALLSTACK
void AbstractSphereThread::printStackTrace()
{
	// don't allow call stack to be modified whilst we're printing it
	freezeCallStack(true);

	llong startTime = m_stackInfo[0].startTime;
	int timedelta;
	uint threadId = getId();

	g_Log.EventDebug("Printing STACK TRACE for debugging.\n");
	g_Log.EventDebug("__ thread (%u) _ |  # | _____________ function _____________ | ticks passed from previous function start\n", threadId);
	for ( size_t i = 0; i < 0x1000; i++ )
	{
		if( m_stackInfo[i].startTime == 0 )
			break;

		timedelta = (int)(m_stackInfo[i].startTime - startTime);
		g_Log.EventDebug(">>         %u    | %2d | %36.36s | +%d %s\n",
			threadId, i, m_stackInfo[i].functionName, timedelta,
				( i == (m_stackPos - 1) ) ?
				"<-- exception catch point (below is guessed and could be incorrect!)" :
				"");
		startTime = m_stackInfo[i].startTime;
	}

	freezeCallStack(false);
}
#endif

/*
 * DummySphereThread
*/
DummySphereThread *DummySphereThread::instance = NULL;

DummySphereThread::DummySphereThread()
	: AbstractSphereThread("dummy", IThread::Normal)
{
}

DummySphereThread *DummySphereThread::getInstance()
{
	if( instance == NULL )
	{
		instance = new DummySphereThread();
	}
	return instance;
}

void DummySphereThread::tick()
{
}
