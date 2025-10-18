/**
 * @file threads.cpp
 *
 * Implementation details for Sphere threading:
 *  - O(1) current() via TLS fast path (zero locking).
 *  - Startup and shutdown flags are atomics (no subsystem calls in fast path).
 *  - No logging while holding ThreadHolder locks (avoid re-entrancy).
 *  - Windows thread naming fix (dwThreadID = -1).
 *  - Correct pthread equality on POSIX.
 *  - Safe temporary string pools.
 */

#ifdef _WIN32
#define _WIN32_DCOM
#include <process.h>
#include <objbase.h>
#else
#include <pthread.h>
#if !defined(_BSD) && !defined(__APPLE__)
#include <sys/prctl.h>
#endif
#include <unistd.h>
#endif

#include "../common/sphere_library/sresetevents.h"
#include "../common/sphere_library/sstringobjs.h"
//#include "../common/basic_threading.h"
#include "../common/CLog.h"
#include "../game/CServer.h"

#include "ProfileTask.h"
#include "threads.h"

#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>
//#include <system_error>

// External globals
extern CServer g_Serv;
extern CLog    g_Log;

// Thread-local pointer for fast “current thread” lookup.
thread_local AbstractSphereThread* g_tlsCurrentSphereThread = nullptr;

// Constants
#define THREAD_EXCEPTIONS_ALLOWED 10
#define THREADJOIN_TIMEOUT 60000

#define THREAD_TEMPSTRING_C_STORAGE     2048
#define THREAD_TEMPSTRING_OBJ_STORAGE   1024

/*
 * Shared pools for temporary strings used during logging/formatting.
 */
struct TemporaryStringsThreadSafeStateHolder
{
    // C-style char buffers
    SimpleMutex             g_tmpCStringMutex;
    std::atomic<uint>       g_tmpCStringIndex;
    std::unique_ptr<char[]> g_tmpCStrings;

    // TemporaryString buffers with “in-use” flags
    SimpleMutex                                                 g_tmpTemporaryStringMutex;
    std::atomic<uint>                                           g_tmpTemporaryStringIndex;
    struct TemporaryStringStorage { char m_buffer[THREAD_STRING_LENGTH]; char m_state; };
    std::unique_ptr<TemporaryStringStorage[]>                   g_tmpTemporaryStringStorage;

private:
    TemporaryStringsThreadSafeStateHolder()
        : g_tmpCStringIndex(0)
        , g_tmpTemporaryStringIndex(0)
    {
        g_tmpCStrings = std::make_unique<char[]>(THREAD_TEMPSTRING_C_STORAGE * THREAD_STRING_LENGTH);
        g_tmpTemporaryStringStorage = std::make_unique<TemporaryStringStorage[]>(THREAD_TEMPSTRING_OBJ_STORAGE);
        for (uint i = 0; i < THREAD_TEMPSTRING_OBJ_STORAGE; ++i)
            g_tmpTemporaryStringStorage[i].m_state = 0;
    }

public:
    static TemporaryStringsThreadSafeStateHolder& get() noexcept
    {
        static TemporaryStringsThreadSafeStateHolder instance;
        return instance;
    }
};

// ----- ThreadHolder (slow-path ops) -----

ThreadHolder::ThreadHolder() noexcept
    : m_threadCount(0)
{
    // Ensure pools constructed early to avoid first-use cost in weird places.
    (void)TemporaryStringsThreadSafeStateHolder::get();
}

ThreadHolder& ThreadHolder::get() noexcept
{
    static ThreadHolder instance;
    return instance;
}

void ThreadHolder::push(AbstractThread *thread) noexcept
{
    if (!thread)
    {
        stderrLog("ThreadHolder::push: trying to push nullptr AbstractThread* ?.\n");
        return;
    }

    // Stash debug info for post-unlock logging.
    const char* pcThreadNameForLog = thread->getName();
    int         iThreadIdForLog   = -1;
    bool        fShouldLog        = false;

    try
    {
        auto* pSphereThread = dynamic_cast<AbstractSphereThread*>(thread);
        if (!pSphereThread)
        {
            stderrLog("ThreadHolder::push: AbstractThread is not an AbstractSphereThread.\n");
            return;
        }

        {
            std::unique_lock<std::shared_mutex> lock(m_mutex);

#ifdef _DEBUG
            auto itp = std::find_if(
                m_spherethreadpairs_systemid_ptr.begin(), m_spherethreadpairs_systemid_ptr.end(),
                [pSphereThread](spherethreadpair_t const &elem) noexcept -> bool {
                    return (elem.second == pSphereThread);
                });
            DEBUG_ASSERT(itp == m_spherethreadpairs_systemid_ptr.end());
#endif

            m_threads.emplace_back(SphereThreadData{.m_ptr = thread, .m_closed = false });
            thread->m_threadHolderId = m_threadCount;

            m_spherethreadpairs_systemid_ptr.emplace_back(pSphereThread->m_threadSystemId, pSphereThread);
            ++m_threadCount;

            iThreadIdForLog = thread->m_threadHolderId;
            fShouldLog = true;
        }

#ifdef _DEBUG
        if (fShouldLog)
        {
            // Logging AFTER releasing the lock avoids re-entrancy into ThreadHolder.
            g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGF_CONSOLE_ONLY,
                "ThreadHolder: registered '%s' (ThreadHolder-ID %d).\n", pcThreadNameForLog, iThreadIdForLog);
        }
#endif
    }
    catch (const std::exception& e)
    {
        stderrLog("ThreadHolder::push: exception: '%s'.\n", e.what());
    }
    catch (...)
    {
        stderrLog("ThreadHolder::push: unknown exception.\n");
    }
}

void ThreadHolder::remove(AbstractThread *pAbstractThread) CANTHROW
{
    if (!pAbstractThread)
        throw CSError(LOGL_FATAL, 0, "ThreadHolder::remove: thread == nullptr");

    const char* pcThreadNameForLog = pAbstractThread->getName();
    int         iThreadIdForLog   = pAbstractThread->m_threadHolderId;
    bool        fShouldLog        = false;

    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        auto* pSphereThread = dynamic_cast<AbstractSphereThread *>(pAbstractThread);
        auto itp = std::find_if(
            m_spherethreadpairs_systemid_ptr.begin(), m_spherethreadpairs_systemid_ptr.end(),
            [pSphereThread](spherethreadpair_t const &elem) noexcept -> bool {
                return (elem.second == pSphereThread);
            });
        if (itp != m_spherethreadpairs_systemid_ptr.end())
            m_spherethreadpairs_systemid_ptr.erase(itp);

        auto itd = std::find_if(m_threads.begin(), m_threads.end(),
            [pAbstractThread](SphereThreadData const& elem) { return elem.m_ptr == pAbstractThread; });
        if (itd != m_threads.end())
            m_threads.erase(itd);

        if (m_threadCount > 0)
            --m_threadCount;

        pAbstractThread->m_threadHolderId = m_kiInvalidThreadID;
        fShouldLog = true;
    }

#ifdef _DEBUG
    if (fShouldLog)
    {
        g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGF_CONSOLE_ONLY,
            "ThreadHolder: removed '%s' (former ThreadHolder-ID %d).\n", pcThreadNameForLog, iThreadIdForLog);
    }
#endif
}

void ThreadHolder::markThreadsClosing() CANTHROW
{
    // Flip fast-path flag first so concurrent readers immediately see “servClosing”.
    ThreadHolder::markServservClosing();

    std::unique_lock<std::shared_mutex> lock(m_mutex);

    for (auto& thread_data : m_threads)
    {
        auto sphere_thread = static_cast<AbstractSphereThread*>(thread_data.m_ptr);
        if (!sphere_thread)
            continue;

        if (sphere_thread->m_fKeepAliveAtShutdown)
            continue;

        sphere_thread->m_uiState = AbstractThread::eRunningState::Closing;
        thread_data.m_closed = true;
    }
}

AbstractThread * ThreadHolder::getThreadAt(size_t at) noexcept
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    if (at >= getActiveThreads())
        return nullptr;
    return m_threads[at].m_ptr;
}

AbstractThread * ThreadHolder::current() noexcept // static
{
    // TLS fast path: the absolutely hottest path in the system.
    AbstractSphereThread* tls = g_tlsCurrentSphereThread;
    if (tls != nullptr) [[likely]]
        return tls;

    /*
        In two cases, the thread-local pointer (g_tlsCurrentSphereThread) may not be set:
        - We are on a thread that is not a registered Sphere thread:
            Signal handler threads, or late after detach.
        - Some context exists but the thread object was never bound:
            Example: If we forget to attachToCurrentThread/bind a Sphere thread object (like StartupMonitorThread or g_Main in inline mode), the TLS pointer remains null.
    */

    // No TLS => not a Sphere thread or not yet attached.
    if (sm_servClosing.load(std::memory_order_relaxed)) [[unlikely]]
        return nullptr;

    if (sm_inStartup.load(std::memory_order_relaxed)) [[unlikely]]
        return DummySphereThread::getInstance(); // may be null extremely early

    // Runtime (Run mode) and not Closing: missing context => nullptr (by policy).
    return nullptr;
}

// Record that thread has started.
void ThreadHolder::markThreadStarted(AbstractThread* pThr) CANTHROW
{
    const int id = pThr->m_threadHolderId;
    SphereThreadData& threadData = m_threads.at(id);
    ASSERT(threadData.m_closed == false);
    threadData.m_closed = false;
}

// Record that the server entered Run mode (disable startup fallback in fast path).
void ThreadHolder::markServEnteredRunMode() noexcept // static
{
    sm_inStartup.store(false, std::memory_order_relaxed);
}

// Record that threads are servClosing (fast-path observable).
void ThreadHolder::markServservClosing() noexcept // static
{
    sm_servClosing.store(true, std::memory_order_relaxed);
}

bool ThreadHolder::isServClosing() noexcept { // static
    return sm_servClosing.load(std::memory_order_relaxed);
}

// ----- AbstractThread -----

int AbstractThread::m_threadsAvailable = 0;

AbstractThread::AbstractThread(const char *name, ThreadPriority priority)
{
    if (AbstractThread::m_threadsAvailable == 0)
    {
#ifdef _WIN32
        if (CoInitializeEx(nullptr, COINIT_MULTITHREADED) != S_OK)
            throw CSError(LOGL_FATAL, 0, "OLE init failed, threading unavailable");
#endif
    }
    ++AbstractThread::m_threadsAvailable;

    Str_CopyLimitNull(m_name, name, sizeof(m_name));
    m_fThread_selfTerminateAfterThisTick = true;
    m_fTerminateRequested = true;

    setPriority(priority);

    m_sleepEvent     = std::make_unique<AutoResetEvent>();
    m_terminateEvent = std::make_unique<ManualResetEvent>();
}

AbstractThread::~AbstractThread()
{
#ifdef _DEBUG
    lpctstr ptcState;
    if (m_threadHolderId == ThreadHolder::m_kiInvalidThreadID) {
        if (m_uiState == eRunningState::Closing)
            ptcState = "[previously registered]";
        else if (m_uiState == eRunningState::NeverStarted)
            ptcState = "[unregistered, never started]";
        else
            stderrLog("~AbstractThread: Thread never registered in ThreadHolder and still running at this stage?\n");
    }
    else
    {
        if (m_uiState == eRunningState::NeverStarted)
            stderrLog("~AbstractThread: Thread registered in ThreadHolder and never started?\n");
        else if (m_uiState == eRunningState::Running)
            stderrLog("~AbstractThread: Thread registered in ThreadHolder and still running at this stage?\n");
    }
    fprintf(stdout, "DEBUG: Destroying thread '%s' (ThreadHolder-ID %d)%s, sys-id %" PRIu64 ".\n",
        getName(), m_threadHolderId,
        ptcState,
        (uint64)getId());
    fflush(stdout);
#endif

    terminate(true);

    --AbstractThread::m_threadsAvailable;
    if (AbstractThread::m_threadsAvailable == 0)
    {
#ifdef _WIN32
        CoUninitialize();
#endif
    }
}

void AbstractThread::overwriteInternalThreadName(const char* name) noexcept
{
    Str_CopyLimitNull(m_name, name, sizeof(m_name));
}

void AbstractThread::start()
{
    g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGF_CONSOLE_ONLY,
        "Spawning new thread '%s' (0x% " PRIxSIZE_T ")...\n",
        getName(), reinterpret_cast<void*>(this));

#ifdef _WIN32
    m_handle = reinterpret_cast<spherethread_t>(_beginthreadex(nullptr, 0, &runner, this, 0, nullptr));
#else
    pthread_attr_t threadAttr;
    pthread_attr_init(&threadAttr);
    pthread_attr_setdetachstate(&threadAttr, PTHREAD_CREATE_DETACHED);
    spherethread_t threadHandle{};
    const int result = pthread_create(&threadHandle, &threadAttr, &runner, this);
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
    if (!isActive())
        return;

    const bool wasCurrentThread = isCurrentThread();

    if (ended == false)
    {
        if (!wasCurrentThread)
        {
            if (!m_handle.has_value())
            {
                stderrLog("AbstractThread::terminate: no handle available.\n");
            }
            else
            {
#ifdef _WIN32
                TerminateThread(m_handle.value(), 0);
                CloseHandle(m_handle.value());
#else
                pthread_cancel(m_handle.value());
#endif
            }
        }
    }

    ThreadHolder::get().remove(this);
    m_threadSystemId = 0;
    m_handle = std::nullopt;

    m_terminateEvent->set();

    if (ended == false && wasCurrentThread)
    {
#ifdef _WIN32
        _endthreadex(EXIT_SUCCESS);
#else
        pthread_exit(nullptr);
#endif
    }
}

void AbstractThread::run()
{
    onStart();

    int exceptions = 0;
    bool lastWasException = false;
    m_fThread_selfTerminateAfterThisTick = false;
    m_fTerminateRequested = false;

    setThreadName(getName());
    m_uiState = eRunningState::Running;

    for (;;)
    {
        if (shouldExit())
            break;

        bool gotException = false;

        if (m_uiHangCheck != 0)
            m_uiHangCheck = 0;

        try
        {
            tick();
            if (shouldExit())
                break;

            GetCurrentProfileData().Start(PROFILE_IDLE);
        }
        catch (const CSError& e)
        {
            gotException = true;
            g_Log.CatchEvent(&e, "[TR] CSError in %s::tick", getName());
            GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
        }
        catch (const std::exception& e)
        {
            gotException = true;
            g_Log.CatchStdException(&e, "[TR] std::exception in %s::tick", getName());
            GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
        }
        catch (...)
        {
            gotException = true;
            g_Log.CatchEvent(nullptr, "[TR] Unknown exception in %s::tick", getName());
            GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
        }

        if (gotException)
        {
            if (lastWasException)
                ++exceptions;
            else
            {
                lastWasException = true;
                exceptions = 1;
            }

            if (exceptions >= THREAD_EXCEPTIONS_ALLOWED)
            {
                g_Log.Event(LOGL_CRIT, "'%s' raised too many exceptions, soft-restarting...\n", getName());
                onStart();
                lastWasException = false;
            }
        }
        else
        {
            lastWasException = false;
            exceptions = 0;
        }

        if (shouldExit())
            break;

        m_sleepEvent->wait(m_uiTickPeriod);
    }

    m_uiState = eRunningState::Closing;
}

SPHERE_THREADENTRY_RETNTYPE SPHERE_THREADENTRY_CALLTYPE AbstractThread::runner(void *callerThread)
{
    auto* caller = reinterpret_cast<AbstractThread*>(callerThread);
    if (caller)
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
    const bool fRet = m_handle.has_value();
    return fRet;
}

bool AbstractThread::isClosing() const
{
    return (m_uiState == eRunningState::Closing);
}

bool AbstractThread::checkStuck()
{
    if (!isActive())
        return false;

    if (m_uiHangCheck == 0)
        m_uiHangCheck = 0xDEAD;
    else if (m_uiHangCheck == 0xDEAD)
        m_uiHangCheck = 0xDEADDEAD;
    else
    {
        g_Log.Event(LOGL_CRIT, "'%s' hang detected, restarting thread...\n", m_name);

        m_fTerminateRequested = true;
        awaken();
        waitForClose();

        start();
        return true;
    }
    return false;
}

void AbstractThread::waitForClose()
{
    if (!isActive())
        return;

    if (!isCurrentThread())
    {
        m_fTerminateRequested = true;
        awaken();

        m_terminateEvent->wait(THREADJOIN_TIMEOUT);
        terminate(false);
    }
}

void AbstractThread::awaken()
{
    m_sleepEvent->signal();
}

bool AbstractThread::isCurrentThread() const noexcept
{
    return isSameThreadId(getId(), getCurrentThreadSystemId());
}

void AbstractThread::onStart()
{
    m_threadSystemId = getCurrentThreadSystemId();

    ThreadHolder& th = ThreadHolder::get();
    th.push(this);
    th.markThreadStarted(this);

#ifdef THREAD_TRACK_CALLSTACK
    g_tlsCurrentSphereThread = dynamic_cast<AbstractSphereThread*>(this);
#else
    g_tlsCurrentSphereThread = dynamic_cast<AbstractSphereThread*>(this);
#endif

#ifdef _DEBUG
    g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGF_CONSOLE_ONLY,
        "Started thread loop for '%s' (ThreadHolder-ID %d), sys-id %" PRIu64 ".\n",
        getName(), m_threadHolderId, (uint64)m_threadSystemId);
#endif
}

void AbstractThread::setPriority(ThreadPriority pri)
{
    ASSERT(((pri >= ThreadPriority::Idle) && (pri <= ThreadPriority::RealTime)) || (pri == ThreadPriority::Disabled));
    m_priority = pri;

    switch (m_priority)
    {
        case ThreadPriority::Idle:     m_uiTickPeriod = 1000; break;
        case ThreadPriority::Low:      m_uiTickPeriod = 200;  break;
        case ThreadPriority::Normal:   m_uiTickPeriod = 100;  break;
        case ThreadPriority::High:     m_uiTickPeriod = 50;   break;
        case ThreadPriority::Highest:  m_uiTickPeriod = 5;    break;
        case ThreadPriority::RealTime: m_uiTickPeriod = 0;    break;
        case ThreadPriority::Disabled: m_uiTickPeriod = AutoResetEvent::_kiInfinite; break;
    }
}

bool AbstractThread::shouldExit() noexcept
{
    return (m_uiState == eRunningState::Closing)
           || m_fTerminateRequested.load(std::memory_order_relaxed)
           || m_fThread_selfTerminateAfterThisTick;
}

void AbstractThread::setThreadName(const char* name)
{
    char name_trimmed[m_nameMaxLength] = {'\0'};
    Str_CopyLimitNull(name_trimmed, name, m_nameMaxLength);

#if defined(_WIN32)
#if defined(_MSC_VER)
#pragma pack(push, 8)
    typedef struct tagTHREADNAME_INFO {
        DWORD  dwType;     // 0x1000
        LPCSTR szName;
        DWORD  dwThreadID; // -1 => current thread
        DWORD  dwFlags;
    } THREADNAME_INFO;
#pragma pack(pop)

    static constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;
    THREADNAME_INFO info{};
    info.dwType     = 0x1000;
    info.szName     = name_trimmed;
    info.dwThreadID = static_cast<DWORD>(-1); // CRITICAL: current thread
    info.dwFlags    = 0;

    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
    }
#endif
#elif defined(__APPLE__)
    pthread_setname_np(name_trimmed);
#elif !defined(_BSD)
    prctl(PR_SET_NAME, name_trimmed, 0, 0, 0);
#elif defined(__FreeBSD__) || defined(__OpenBSD__)
    pthread_set_name_np(pthread_self(), name_trimmed);
#elif defined(__NetBSD__)
    pthread_setname_np(pthread_self(), "%s", name_trimmed);
#endif

    // Update internal name only if a context exists
    if (auto* cur = ThreadHolder::current())
        if (auto* athr = dynamic_cast<AbstractSphereThread*>(cur))
            athr->overwriteInternalThreadName(name_trimmed);
}

void AbstractThread::attachToCurrentThread(const char* osThreadName) noexcept
{
    lpctstr ptcThreadName = (osThreadName && osThreadName[0]) ? osThreadName : getName();
    g_Log.Event(LOGM_DEBUG|LOGL_EVENT|LOGF_CONSOLE_ONLY,
        "Binding current context to thread '%s'...\n", ptcThreadName);
    onStart();
    setThreadName(ptcThreadName);
}

void AbstractThread::detachFromCurrentThread() noexcept
{
    if (auto* s = dynamic_cast<AbstractSphereThread*>(this))
        if (g_tlsCurrentSphereThread == s)
            g_tlsCurrentSphereThread = nullptr;

    ThreadHolder::get().remove(this);
    m_threadSystemId = 0;
}

// ----- AbstractSphereThread -----

AbstractSphereThread::AbstractSphereThread(const char *name, ThreadPriority priority)
    : AbstractThread(name, priority)
    , m_pExpr{std::make_unique<CExpression>()}
{
    m_profile.EnableProfile(PROFILE_IDLE);
    m_profile.EnableProfile(PROFILE_OVERHEAD);
    m_profile.EnableProfile(PROFILE_STAT_FAULTS);
}

AbstractSphereThread::~AbstractSphereThread()
{
    m_uiState = eRunningState::Closing;

    if (g_tlsCurrentSphereThread == this)
        g_tlsCurrentSphereThread = nullptr;
}

#ifdef THREAD_TRACK_CALLSTACK
void AbstractSphereThread::signalExceptionCaught() noexcept
{
    if (m_iStackPos < 0)
        return;

    m_iCaughtExceptionStackPos = std::max(m_iStackPos, m_iCaughtExceptionStackPos);
    printStackTrace();

    std::memset(m_stackInfoCopy, 0, sizeof(m_stackInfoCopy));
    m_iCaughtExceptionStackPos     = -1;
    m_iStackUnwindingStackPos      = -1;
}

void AbstractSphereThread::signalExceptionStackUnwinding() noexcept
{
    if (m_iStackPos < 0)
        return;

    m_iStackUnwindingStackPos = std::max(m_iStackPos, m_iStackUnwindingStackPos);
    std::memcpy(m_stackInfoCopy, m_stackInfo, sizeof(m_stackInfo));
}

void AbstractSphereThread::pushStackCall(const char *name) noexcept
{
    if (m_fFreezeCallStack)
        return;

#ifdef _DEBUG
    if (m_iStackPos < -1)
        RaiseImmediateAbort(16);
    if (m_iStackPos >= (ssize_t)ARRAY_COUNT(m_stackInfo) - 1)
        RaiseImmediateAbort(17);
#endif

    ++m_iStackPos;
    m_stackInfo[m_iStackPos].functionName = name;
}

void AbstractSphereThread::popStackCall() NOEXCEPT_NODEBUG
{
    if (m_fFreezeCallStack)
        return;

#ifdef _DEBUG
    ASSERT(m_iStackPos >= 0);
#endif
    if (m_iStackPos >= 0)
    {
        m_stackInfo[m_iStackPos].functionName = nullptr;
        --m_iStackPos;
    }
}

void AbstractSphereThread::printStackTrace() noexcept
{
    freezeCallStack(true);

    uint64_t threadId =
#if defined(_WIN32) || defined(__APPLE__)
        static_cast<uint64_t>(getId() ? getId() : getCurrentThreadSystemId());
#else
        reinterpret_cast<uint64_t>(getId() ? getId() : getCurrentThreadSystemId());
#endif

    const lpctstr threadName = getName();
    auto& stackInfo = (m_stackInfoCopy[0].functionName != nullptr) ? m_stackInfoCopy : m_stackInfo;

    g_Log.EventDebug("Printing STACK TRACE for debugging purposes.\n");
    g_Log.EventDebug(" ______ thread (id) name _____ |   # | _____________ function _____________ |\n");

    for (ssize_t i = 0; i < (ssize_t)ARRAY_COUNT(m_stackInfoCopy); ++i)
    {
        if (stackInfo[i].functionName == nullptr)
            break;

        lpctstr extra = "";
        if (i == m_iStackUnwindingStackPos)
            extra = "<-- last tracked function call (stack unwinding detected here)";
        else if (i == m_iCaughtExceptionStackPos)
            extra = (m_iStackUnwindingStackPos == -1)
                        ? "<-- exception catch point (below is guessed and could be incorrect!)"
                        : "<-- exception catch point";

        g_Log.EventDebug("(%" PRIx64 ") %15.15s | %3u | %36.36s | %s\n",
            threadId, threadName, (uint)i, stackInfo[i].functionName, extra);

        if (i == m_iStackUnwindingStackPos)
            break;
    }

    freezeCallStack(false);
}
#endif

bool AbstractSphereThread::shouldExit() noexcept
{
    if (g_Serv.GetExitFlag() != 0)
        return true;
    return AbstractThread::shouldExit();
}

// ----- DummySphereThread -----

DummySphereThread::DummySphereThread()
    : AbstractSphereThread("dummy", ThreadPriority::Normal)
{
}

void DummySphereThread::createInstance()
{
    ASSERT(_instance == nullptr);
    _instance = new DummySphereThread();
}

DummySphereThread* DummySphereThread::getInstance() noexcept
{
    return _instance;
}

void DummySphereThread::tick()
{
    // No-op
}

// ----- Temporary string pool helpers -----

static TemporaryStringsThreadSafeStateHolder::TemporaryStringStorage*
getThreadRawStringBuffer() CANTHROW
{
    auto& tsholder = TemporaryStringsThreadSafeStateHolder::get();
    SimpleThreadLock stlBuffer(tsholder.g_tmpTemporaryStringMutex);

    const int initialPosition = static_cast<int>(tsholder.g_tmpTemporaryStringIndex.load(std::memory_order_relaxed));
    int index = initialPosition;

    for (;;)
    {
        index = static_cast<int>(++tsholder.g_tmpTemporaryStringIndex);
        if (index >= (int)THREAD_TEMPSTRING_OBJ_STORAGE)
        {
            index %= THREAD_TEMPSTRING_OBJ_STORAGE;
            tsholder.g_tmpTemporaryStringIndex.store(index, std::memory_order_relaxed);
        }

        auto* store = &(tsholder.g_tmpTemporaryStringStorage[index]);
        if (store->m_state == 0)
        {
            store->m_state = 1;
            store->m_buffer[0] = '\0';
            return store;
        }

        if (index == initialPosition)
        {
            DEBUG_WARN(("Thread temporary string buffer is full.\n"));
            throw CSError(LOGL_FATAL, 0, "Thread temporary string buffer is full");
        }
    }
}

char* AbstractSphereThread::Strings::allocateBuffer() noexcept
{
    auto& tsholder = TemporaryStringsThreadSafeStateHolder::get();
    SimpleThreadLock stlBuffer(tsholder.g_tmpCStringMutex);

    uint index = ++tsholder.g_tmpCStringIndex;
    if (index >= THREAD_TEMPSTRING_C_STORAGE)
    {
        index %= THREAD_TEMPSTRING_C_STORAGE;
        tsholder.g_tmpCStringIndex.store(index, std::memory_order_relaxed);
    }

    if (!tsholder.g_tmpCStrings)
        RaiseImmediateAbort(20);

    char* buffer = &(tsholder.g_tmpCStrings[index * THREAD_STRING_LENGTH]);
    buffer[0] = '\0';
    return buffer;
}

void AbstractSphereThread::Strings::getBufferForStringObject(TemporaryString &string) CANTHROW
{
    ADDTOCALLSTACK("AbstractSphereThread::Strings::alloc");
    auto* store = getThreadRawStringBuffer(); // may throw
    string.init(store->m_buffer, &store->m_state);
}

// ----- StackDebugInformation (RAII) -----
#ifdef THREAD_TRACK_CALLSTACK
StackDebugInformation::StackDebugInformation(const char *name) noexcept
    : m_context(nullptr)
{
    // Fast path: TLS
    if (g_tlsCurrentSphereThread && !g_tlsCurrentSphereThread->isClosing())
    {
        m_context = g_tlsCurrentSphereThread;
        m_context->pushStackCall(name);
        return;
    }

    // Fallback: use current() (still lock-free and fast)
    AbstractThread *icontext = ThreadHolder::current();
    if (!icontext)
        return; // TODO: log this?

    m_context = dynamic_cast<AbstractSphereThread*>(icontext);
    if (m_context && !m_context->isClosing())
        m_context->pushStackCall(name);
}

StackDebugInformation::~StackDebugInformation() noexcept
{
    if (!m_context || m_context->isClosing())
        return;

    if (!m_context->isExceptionStackUnwinding())
    {
        if (std::uncaught_exceptions() != 0)
            m_context->signalExceptionStackUnwinding();
    }
    m_context->popStackCall();
}

void StackDebugInformation::printStackTrace() noexcept
{
    AbstractThread* pThreadState = ThreadHolder::current();
    if (pThreadState)
        if (auto* s = dynamic_cast<AbstractSphereThread*>(pThreadState))
            s->printStackTrace();
}

void StackDebugInformation::freezeCallStack(bool freeze) noexcept
{
    AbstractThread* pThreadState = ThreadHolder::current();
    if (pThreadState)
        if (auto* s = dynamic_cast<AbstractSphereThread*>(pThreadState))
            s->freezeCallStack(freeze);
}
#endif
