/**
 * @file threads.cpp
 */

#ifdef _WIN32
#   define _WIN32_DCOM
#   include <process.h>
#   include <objbase.h>
#else
#   include <pthread.h>
#   if !defined(_BSD) && !defined(__APPLE__)
#       include <sys/prctl.h>
#   endif
#   include <unistd.h>
#endif

#if defined(__linux__)
#   include <sys/syscall.h>
#   include <unistd.h>
#elif defined(__FreeBSD__)
#   include <pthread_np.h>
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
static thread_local AbstractSphereThread* sg_tlsCurrentSphereThread = nullptr;
// Avoid trying to get thread context while binding it.
static thread_local bool sg_tlsBindingInProgress = false;

// Global state flags
static std::atomic_bool sg_inStartup{true};
static std::atomic_bool sg_servClosing{false};

// Constants
#define THREAD_EXCEPTIONS_ALLOWED 10
#define THREAD_JOIN_TIMEOUT 60000

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
    SimpleMutex                                 g_tmpTemporaryStringMutex;
    std::atomic<uint>                           g_tmpTemporaryStringIndex;
    struct TemporaryStringStorage
    {
        char m_buffer[THREAD_STRING_LENGTH];
        char m_state;
    };
    std::unique_ptr<TemporaryStringStorage[]>   g_tmpTemporaryStringStorage;

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

// ----- ThreadHolder  -----

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

bool ThreadHolder::isSystemIdRegistered(threadid_t sysId, AbstractSphereThread** outExisting) const noexcept
{
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    auto it = std::find_if(
        m_spherethreadpairs_systemid_ptr.begin(),
        m_spherethreadpairs_systemid_ptr.end(),
        [sysId](const spherethreadpair_t& elem) noexcept {
            return elem.first == sysId;
        }
        );
    if (it == m_spherethreadpairs_systemid_ptr.end())
        return false;
    if (outExisting)
        *outExisting = it->second;
    return true;
}

void ThreadHolder::push(AbstractThread* thread) noexcept
{
    if (!thread)
    {
        stderrLog("ThreadHolder::push: nullptr thread.\n");
        return;
    }

    auto* pSphereThread = dynamic_cast<AbstractSphereThread*>(thread);
    if (!pSphereThread)
    {
        stderrLog("ThreadHolder::push: not an AbstractSphereThread.\n");
        return;
    }

    const char* nameForLog = thread->getName();
    int idForLog = -1;
    bool shouldLog = false;

    try
    {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        // 1) Idempotency: same pointer already registered.
        auto itExistingPtr = std::find_if(
            m_spherethreadpairs_systemid_ptr.begin(),
            m_spherethreadpairs_systemid_ptr.end(),
            [pSphereThread](const spherethreadpair_t& elem) noexcept {
                return elem.second == pSphereThread;
            }
            );
        if (itExistingPtr != m_spherethreadpairs_systemid_ptr.end())
        {
            // Refresh holder id from current slot if present.
            auto itSlot = std::find_if(
                m_threads.begin(),
                m_threads.end(),
                [thread](const SphereThreadData& s) noexcept { return s.m_ptr == thread; }
                );
            if (itSlot != m_threads.end())
                thread->m_threadHolderId = static_cast<int>(std::distance(m_threads.begin(), itSlot));

#ifdef _DEBUG
            idForLog = thread->m_threadHolderId;
            shouldLog = true;
#endif
            lock.unlock();
#ifdef _DEBUG
            if (shouldLog)
                g_Log.Event(LOGM_DEBUG | LOGL_EVENT | LOGF_CONSOLE_ONLY,
                    "ThreadHolder already registered: %s (ThreadHolder-ID %d).\n",
                    nameForLog, idForLog);
#endif
            return;
        }

        // 2) System-id uniqueness: refuse a second Sphere context on the same OS thread.
        auto itExistingSys = std::find_if(
            m_spherethreadpairs_systemid_ptr.begin(),
            m_spherethreadpairs_systemid_ptr.end(),
            [pSphereThread](const spherethreadpair_t& elem) noexcept {
                // Correct equality for pthread_t vs DWORD handled by threadid_t typedef.
                return elem.first == pSphereThread->m_threadSystemId;
            }
            );
        if (itExistingSys != m_spherethreadpairs_systemid_ptr.end())
        {
            // Another object is already attached to this OS thread; refuse the second.
            auto* other = itExistingSys->second;
            lock.unlock();
            g_Log.Event(LOGM_DEBUG | LOGL_EVENT | LOGF_CONSOLE_ONLY,
                "ThreadHolder: refusing to register %s on OS thread already owned by %s.\n",
                nameForLog, other ? other->getName() : "<unknown>");
            EXC_NOTIFY_DEBUGGER;
            return;
        }

        // 3) New registration.
        m_threads.emplace_back(SphereThreadData{ thread, false });
        thread->m_threadHolderId = m_threadCount;
        m_spherethreadpairs_systemid_ptr.emplace_back(pSphereThread->m_threadSystemId, pSphereThread);
        ++m_threadCount;

#ifdef _DEBUG
        idForLog = thread->m_threadHolderId;
        shouldLog = true;
#endif
        lock.unlock();

#ifdef _DEBUG
        if (shouldLog)
            g_Log.Event(LOGM_DEBUG | LOGL_EVENT | LOGF_CONSOLE_ONLY,
                "ThreadHolder registered %s (ThreadHolder-ID %d).\n",
                nameForLog, idForLog);
#endif
    }
    catch (const std::exception& e)
    {
        stderrLog("ThreadHolder::push: exception: %s.\n", e.what());
    }
    catch (...)
    {
        stderrLog("ThreadHolder::push: unknown exception.\n");
    }
}


void ThreadHolder::remove(AbstractThread* pAbstractThread) CANTHROW
{
    if (!pAbstractThread)
        throw CSError(LOGL_FATAL, 0, "ThreadHolder::remove: thread == nullptr");

    AbstractSphereThread* pSphereThread = dynamic_cast<AbstractSphereThread*>(pAbstractThread);
    threadid_t sysId = pSphereThread ? pSphereThread->m_threadSystemId : threadid_t{};
    const char* ptcName = pAbstractThread->getName();

    std::unique_lock lock(m_mutex);

    // Remove from main list.
    auto it = std::find_if(m_threads.begin(), m_threads.end(),
        [pAbstractThread](const SphereThreadData& s) noexcept { return s.m_ptr == pAbstractThread; });
    if (it != m_threads.end())
        m_threads.erase(it);

    // Remove from sys-id map.
    for (auto it2 = m_spherethreadpairs_systemid_ptr.begin(); it2 != m_spherethreadpairs_systemid_ptr.end(); )
    {
        if (it2->second == pSphereThread)
            it2 = m_spherethreadpairs_systemid_ptr.erase(it2);
        else
            ++it2;
    }

    // Mark as no longer registered to aid destructor diagnostics.
    pAbstractThread->m_threadHolderId = ThreadHolder::m_kiInvalidThreadID;

    lock.unlock();

#ifdef _DEBUG
    if (!isServClosing())
    {
        g_Log.Event(LOGM_DEBUG | LOGL_EVENT | LOGF_CONSOLE_ONLY,
            "ThreadHolder removed %s with sys-id %" PRIu64 ".\n",
            ptcName, (uint64_t)sysId);
    }
    else
    {
        // Logger may be shutting down; print directly to stdout to avoid loss.
        fprintf(stdout, "DEBUG: ThreadHolder removed %s with sys-id %" PRIu64 ".\n",
            ptcName, (uint64_t)sysId);
        fflush(stdout);
    }
#endif
}



void ThreadHolder::markThreadsClosing() CANTHROW
{
    // Flip fast-path flag first so concurrent readers immediately see “server is Closing”.
    ThreadHolder::markServClosing();

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
    AbstractSphereThread* tls = sg_tlsCurrentSphereThread;
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
    if (sg_servClosing.load(std::memory_order_relaxed) == true) [[unlikely]]
        return nullptr;

    if (sg_inStartup.load(std::memory_order_relaxed) == true) [[unlikely]]
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
    sg_inStartup.store(false, std::memory_order_relaxed);
}

// Record that threads are servClosing (fast-path observable).
void ThreadHolder::markServClosing() noexcept // static
{
    sg_servClosing.store(true, std::memory_order_relaxed);
}

bool ThreadHolder::isServClosing() noexcept { // static
    return sg_servClosing.load(std::memory_order_relaxed);
}

// ----- AbstractThread -----

int AbstractThread::m_threadsAvailable = 0;

static inline bool is_same_thread_id(threadid_t firstId, threadid_t secondId) noexcept
{
#if defined(_WIN32) || defined(__APPLE__)
    return (firstId == secondId);
#else
    return pthread_equal(firstId, secondId);
#endif
}

static uint64_t os_current_tid() noexcept    // Equivalent to old getCurrentThreadSystemId()
{
#if defined(_WIN32)
    return static_cast<uint64_t>(::GetCurrentThreadId()); // Ret type: DWORD.
#elif defined(__APPLE__)
    uint64_t tid = 0;
    (void)pthread_threadid_np(nullptr, &tid);
    return tid;
#elif defined(__linux__)
    // gettid is the kernel TID seen by system tools
    return static_cast<uint64_t>(::syscall(SYS_gettid));
#elif defined(__FreeBSD__)  // TODO: does it work also for other BSD systems?
    return static_cast<uint64_t>(::pthread_getthreadid_np());
#else
    // Last-resort fallback: make a printable token from pthread_self()
    return static_cast<uint64_t>(reinterpret_cast<uintptr_t>(::pthread_self()));
#endif
}

static void os_set_thread_name_portable(const char* name_trimmed) noexcept
{
#if defined(_WIN32)
    // Prefer SetThreadDescription (Windows 10+) if available
    using SetThreadDescription_t = HRESULT (WINAPI *)(HANDLE, PCWSTR);
    static SetThreadDescription_t pSetThreadDescription =
        reinterpret_cast<SetThreadDescription_t>(
            GetProcAddress(GetModuleHandleW(L"Kernel32.dll"), "SetThreadDescription"));
    if (pSetThreadDescription)
    {
        wchar_t wname[64];
        ::MultiByteToWideChar(CP_UTF8, 0, name_trimmed, -1, wname, static_cast<int>(std::size(wname)));
        pSetThreadDescription(GetCurrentThread(), wname);
    }
    else
    {
#   if defined(_MSC_VER)
#   pragma pack(push, 8)
        typedef struct tagTHREADNAME_INFO {
            DWORD  dwType;     // 0x1000
            LPCSTR szName;
            DWORD  dwThreadID; // -1 => current thread
            DWORD  dwFlags;
        } THREADNAME_INFO;
#   pragma pack(pop)

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
#   else
        stderrLog("WARN: no available implementation to set the thread name.\n");
#   endif
    }
#elif defined(__APPLE__)
    // macOS: pthread_setname_np only sets the current thread, 64-char limit
    (void)pthread_setname_np(name_trimmed);
#elif defined(__linux__)
//  Linux: prctl(PR_SET_NAME) sets current thread name, 16-byte limit incl. NUL
#   if !defined(_BSD) && !defined(__APPLE__)
        ::prctl(PR_SET_NAME, name_trimmed, 0, 0, 0);
#   endif
//  Also attempt pthread_setname_np where available for consistency
#   if defined(__GLIBC__) || defined(__ANDROID__)
        (void)pthread_setname_np(pthread_self(), name_trimmed);
#   endif
#elif defined(__FreeBSD__)
    (void)pthread_set_name_np(pthread_self(), name_trimmed);
#else
    // TODO: support other BSD systems
    // No-op on unknown platforms
    (void)name_trimmed;
    stderrLog("WARN: no available implementation to set the thread name for the current platform (unknown/unsupported).\n");
#endif
}

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
    m_fThreadSelfTerminateAfterThisTick = true;
    m_fTerminateRequested = true;

    setPriority(priority);

    m_sleepEvent     = std::make_unique<AutoResetEvent>();
    m_terminateEvent = std::make_unique<ManualResetEvent>();
}

// threads.cpp — robust, flag-free destructor classification
AbstractThread::~AbstractThread()
{
#ifdef _DEBUG
    const char* name = getName();
    if (!name || !name[0]) name = "(unnamed)";

    const bool stillRegistered = (m_threadHolderId != ThreadHolder::m_kiInvalidThreadID);
    const bool everBound       = (m_threadSystemId != 0);
    const bool everRanLoop     = (m_uiState != eRunningState::NeverStarted);

    const char* state =
        stillRegistered               ? "[registered, closing]" :
        (everBound && everRanLoop)    ? "[detached, closed]" :
        (everBound && !everRanLoop)   ? "[attached-only, closed]" :
                                        "[not started]";

    if (everBound)
        fprintf(stdout, "DEBUG: Destroying thread '%s' (ThreadHolder-ID %d) %s, sys-id %" PRIu64 ".\n",
            name, m_threadHolderId, state, (uint64_t)m_threadSystemId);
    else
        fprintf(stdout, "DEBUG: Destroying thread '%s' (ThreadHolder-ID %d) %s, sys-id n/a.\n",
            name, m_threadHolderId, state);
    fflush(stdout);
#endif

    terminate(true);
    --AbstractThread::m_threadsAvailable;
#ifdef _WIN32
    if (AbstractThread::m_threadsAvailable == 0)
        CoUninitialize();
#endif
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

    detachFromCurrentThread();
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
    m_fThreadSelfTerminateAfterThisTick = false;
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

        m_terminateEvent->wait(THREAD_JOIN_TIMEOUT);
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
    static_assert(sizeof(threadid_t) == sizeof(DWORD));
#endif
    return is_same_thread_id(getId(), static_cast<threadid_t>(os_current_tid()));
}

void AbstractThread::onStart()
{
    // Mark registration window so diagnostics don’t warn while TLS is not yet published.
    sg_tlsBindingInProgress = true;

    // Capture OS thread id first.
    m_threadSystemId = static_cast<threadid_t>(os_current_tid());

    // Try to register in the holder.
    ThreadHolder& th = ThreadHolder::get();
    th.push(this);

    // If push was refused (e.g., this OS thread already owned), leave clean.
    if (m_threadHolderId == ThreadHolder::m_kiInvalidThreadID)
    {
        m_threadSystemId = 0;
        sg_tlsBindingInProgress = false;
        EXC_NOTIFY_DEBUGGER;
        return;
    }

    // Mark started and publish TLS fast-path.
    th.markThreadStarted(this);
    sg_tlsCurrentSphereThread = dynamic_cast<AbstractSphereThread*>(this);

#ifdef _DEBUG
    g_Log.Event(LOGM_DEBUG | LOGL_EVENT | LOGF_CONSOLE_ONLY,
        "Started thread loop for '%s' (ThreadHolder-ID %d), sys-id %" PRIu64 ".\n",
        getName(), m_threadHolderId, (uint64)m_threadSystemId);
#endif

    // End of the small registration window.
    sg_tlsBindingInProgress = false;
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
           || m_fTerminateRequested
           || m_fThreadSelfTerminateAfterThisTick;
}

void AbstractThread::setThreadName(const char* name)
{
    char name_trimmed[m_nameMaxLength] = {'\0'};
    Str_CopyLimitNull(name_trimmed, name, m_nameMaxLength);

    os_set_thread_name_portable(name_trimmed);

    // Update internal name only if a context exists
    if (auto* cur = ThreadHolder::current())
        if (auto* athr = dynamic_cast<AbstractSphereThread*>(cur))
            athr->overwriteInternalThreadName(name_trimmed);
}

void AbstractThread::attachToCurrentThread(const char* osThreadName) noexcept
{
    lpctstr ptcThreadName = (osThreadName && osThreadName[0]) ? osThreadName : getName();

    // Refuse if another Sphere context already owns this OS thread.
    if (sg_tlsCurrentSphereThread && sg_tlsCurrentSphereThread != this)
    {
        g_Log.Event(LOGM_DEBUG | LOGL_EVENT | LOGF_CONSOLE_ONLY,
            "attachToCurrentThread refused: current OS thread already bound to '%s'; wanted '%s'.\n",
            sg_tlsCurrentSphereThread->getName(), ptcThreadName);
        return;
    }

    g_Log.Event(LOGM_DEBUG | LOGL_EVENT | LOGF_CONSOLE_ONLY,
        "Binding current context to thread '%s'...\n", ptcThreadName);

    // Attempt to bind; onStart will self-guard and leave this object clean on refusal.
    onStart();

    // Only set the OS-visible name if registration succeeded.
    if (m_threadHolderId != ThreadHolder::m_kiInvalidThreadID)
        setThreadName(ptcThreadName);
}


void AbstractThread::detachFromCurrentThread() noexcept
{
    if (auto* s = dynamic_cast<AbstractSphereThread*>(this))
        if (sg_tlsCurrentSphereThread == s)
            sg_tlsCurrentSphereThread = nullptr;

    // Keep m_threadSystemId as historical OS TID; invalidation is done via m_threadHolderId.
    ThreadHolder::get().remove(this);
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

    if (sg_tlsCurrentSphereThread == this)
        sg_tlsCurrentSphereThread = nullptr;
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

    uint64_t threadId = //static_cast<uint64_t>(getId());
#if defined(_WIN32) || defined(__APPLE__)
        static_cast<uint64_t>(getId() ? getId() : os_current_tid());
#else
        reinterpret_cast<uint64_t>(getId() ? getId() : os_current_tid());
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
    if (sg_tlsCurrentSphereThread && !sg_tlsCurrentSphereThread->isClosing())
    {
        m_context = sg_tlsCurrentSphereThread;
        m_context->pushStackCall(name);
        return;
    }

    // If a thread is in the middle of binding, don’t warn about missing context.
    if (sg_tlsBindingInProgress)
        return;

    // Fallback: use current()
    AbstractThread *icontext = ThreadHolder::current();
    if (!icontext)
        return;

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
