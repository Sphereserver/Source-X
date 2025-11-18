/**
 * @file threads.h
 */

#ifndef _INC_THREADS_H
#define _INC_THREADS_H

#include "../common/common.h"
#include "../sphere/ProfileData.h"

#include <atomic>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <vector>

#ifndef _WIN32
#include <pthread.h>
#endif

class CExpression;
class TemporaryString;
class AutoResetEvent;
class ManualResetEvent;

/*
 * Platform types for OS thread identity and entry points.
 */
#ifdef _WIN32
#include <windows.h>
typedef HANDLE  spherethread_t;
typedef DWORD   threadid_t;
#define SPHERE_THREADENTRY_RETNTYPE unsigned
#define SPHERE_THREADENTRY_CALLTYPE __stdcall
#define SPHERE_THREADT_NULL nullptr
#else
//#include <stdint.h>
#include <pthread.h>
typedef pthread_t spherethread_t;
#ifdef __APPLE__
typedef uint64_t threadid_t; // from pthread_threadid_np
#else
typedef pthread_t threadid_t;
#endif
#define SPHERE_THREADENTRY_RETNTYPE void*
#define SPHERE_THREADENTRY_CALLTYPE
#endif

/*
 * Priority → tick cadence mapping (ms).
 */
enum class ThreadPriority : int
{
    Idle,
    Low,
    Normal,
    High,
    Highest,
    RealTime,
    Disabled = 0xFF
};

class AbstractThread
{
    friend class ThreadHolder;

    static int m_threadsAvailable;

public:
    static constexpr uint m_nameMaxLength = 16; // OS limits for linux/bsd pthread names

protected:
    // TODO: move at least m_threadHolderId to AbstractSphereThread,
    //  since it should theoretically be used only by Sphere custom thread class/AbstractSphereThread?
    threadid_t m_threadSystemId{0};        // OS thread id for this object (0 => not bound)
    int        m_threadHolderId{-1};       // logical id in ThreadHolder

    bool m_fKeepAliveAtShutdown{false};
    bool m_fThreadSelfTerminateAfterThisTick{false};
    bool m_fTerminateRequested{false};

    enum class eRunningState : uchar {
        NeverStarted,
        Running,
        Closing
    };
    std::atomic<eRunningState> m_uiState{eRunningState::NeverStarted};

private:
    char m_name[30]{};                     // internal name (OS name uses trimmed variant)

    std::optional<spherethread_t> m_handle; // present if start() spawned an OS thread

    uint           m_uiHangCheck{0};
    ThreadPriority m_priority{ThreadPriority::Normal};
    uint           m_uiTickPeriod{100};

    std::unique_ptr<AutoResetEvent>   m_sleepEvent;
    std::unique_ptr<ManualResetEvent> m_terminateEvent;

public:
    AbstractThread(const char *name, ThreadPriority priority = ThreadPriority::Normal);
    virtual ~AbstractThread();

    AbstractThread(const AbstractThread&) = delete;
    AbstractThread& operator=(const AbstractThread&) = delete;

public:
    threadid_t     getId() const noexcept { return m_threadSystemId; }
    virtual const char *getName() const noexcept { return m_name; }

    bool isActive() const;
    bool checkStuck();
    bool isClosing() const;

    virtual void start();
    virtual void terminate(bool ended);
    virtual void waitForClose();
    void         awaken();

    void setPriority(ThreadPriority pri);
    ThreadPriority getPriority() const { return m_priority; }

    void overwriteInternalThreadName(const char* name) noexcept;
    bool isCurrentThread() const noexcept;

protected:
    virtual void tick() = 0;

    // Called on the OS thread that will execute tick().
    virtual void onStart();
    virtual bool shouldExit() noexcept;

private:
    void run();
    static SPHERE_THREADENTRY_RETNTYPE SPHERE_THREADENTRY_CALLTYPE runner(void *callerThread);

public:
    static void setThreadName(const char* name);

    // Inline binding helper: attach/detach without creating a new OS thread.
    class ThreadBindingScope
    {
        AbstractThread* m_pAbstractThread;
    public:
        explicit ThreadBindingScope(AbstractThread& t, const char* pcOsName = nullptr) noexcept
            : m_pAbstractThread(&t) { m_pAbstractThread->attachToCurrentThread(pcOsName); }
        ~ThreadBindingScope() noexcept { if (m_pAbstractThread) m_pAbstractThread->detachFromCurrentThread(); }
        ThreadBindingScope(const ThreadBindingScope&) = delete;
        ThreadBindingScope& operator=(const ThreadBindingScope&) = delete;
    };

    void attachToCurrentThread(const char* osThreadName = nullptr) noexcept;
    void detachFromCurrentThread() noexcept;
};

class AbstractSphereThread : public AbstractThread
{
    friend class ThreadHolder;

#ifdef THREAD_TRACK_CALLSTACK
    struct STACK_INFO_REC { const char *functionName; };

    STACK_INFO_REC m_stackInfo[0x500]{};
    STACK_INFO_REC m_stackInfoCopy[0x500]{};

    ssize_t m_iStackPos{-1};
    ssize_t m_iStackUnwindingStackPos{-1};
    ssize_t m_iCaughtExceptionStackPos{-1};
    bool    m_fFreezeCallStack{false};
#endif

public:
    std::unique_ptr<CExpression> m_pExpr;

public:
    AbstractSphereThread(const char *name, ThreadPriority priority = ThreadPriority::Normal);
    virtual ~AbstractSphereThread();

    AbstractSphereThread(const AbstractSphereThread&) = delete;
    AbstractSphereThread& operator=(const AbstractSphereThread&) = delete;

    class Strings
    {
        friend class TemporaryString;
        friend tchar* Str_GetTemp() noexcept;

        static char *allocateBuffer() noexcept;
        static void getBufferForStringObject(TemporaryString &string) CANTHROW;
    };

public:
#ifdef THREAD_TRACK_CALLSTACK
    void signalExceptionCaught() noexcept;
    void signalExceptionStackUnwinding() noexcept;

    inline bool isExceptionCaught() const noexcept { return (m_iCaughtExceptionStackPos >= 0); }
    inline bool isExceptionStackUnwinding() const noexcept { return (m_iStackUnwindingStackPos >= 0); }

    inline void freezeCallStack(bool freeze) noexcept { m_fFreezeCallStack = freeze; }

    void pushStackCall(const char *name) noexcept;
    void popStackCall() NOEXCEPT_NODEBUG;

    void printStackTrace() noexcept;
#endif

    ProfileData m_profile;

protected:
    virtual bool shouldExit() noexcept;
};

/*
 * DummySphereThread:
 *  - Exists at startup so code has a safe expression/call-stack context.
 *  - After entering Run, missing contexts return nullptr by policy.
 */
class DummySphereThread : public AbstractSphereThread
{
private:
    friend struct GlobalInitializer;
    static DummySphereThread *_instance;

public:
    static void createInstance();
    static DummySphereThread *getInstance() noexcept;

protected:
    DummySphereThread();
    virtual void tick();
};

/*
 * ThreadHolder: registry and state flags. The “current()” hot path is fully inline and lock-free.
 *
 * Fast-path policy:
 *  - If TLS is set (g_tlsCurrentSphereThread != nullptr) → return it. O(1).
 *  - Else, if servClosing → return nullptr.
 *  - Else, if still in startup → return Dummy (if exists).
 *  - Else (Run mode) → return nullptr.
 *
 * Slow-path operations (push/remove/getThreadAt) use a mutex; NEVER log while holding it.
 */
class ThreadHolder
{
    friend class AbstractThread;

    struct SphereThreadData
    {
        AbstractThread *    m_ptr;
        bool m_closed;
    };
    using spherethreadlist_t = std::vector<SphereThreadData>;
    spherethreadlist_t m_threads;

    using spherethreadpair_t = std::pair<threadid_t, AbstractSphereThread *>;
    std::vector<spherethreadpair_t> m_spherethreadpairs_systemid_ptr;

    int                         m_threadCount;
    mutable std::shared_mutex   m_mutex;

    ThreadHolder() noexcept;
    ~ThreadHolder() noexcept = default;

public:
    static constexpr lpctstr m_sClassName = "ThreadHolder";
    static constexpr int m_kiInvalidThreadID = -1;

    // Singleton instance for slow-path ops
    static ThreadHolder& get() noexcept;

    // High-performance fast path for “current” thread lookup.
    static AbstractThread *current() noexcept;

    // Record that the server entered Run mode (disable startup fallback in fast path).
    static void markServEnteredRunMode() noexcept;

    // Record that threads are servClosing (fast-path observable).
    static void markServClosing() noexcept;

    static bool isServClosing() noexcept;

    // Slow-path registry ops
    void push(AbstractThread *pAbstractThread) noexcept;
    void remove(AbstractThread *thread) CANTHROW;

    AbstractThread * getThreadAt(size_t at) noexcept;
    inline size_t getActiveThreads() noexcept { return static_cast<size_t>(m_threadCount); }

    void markThreadStarted(AbstractThread* pThr) CANTHROW;

    // Helper to mark servClosing and set flags.
    void markThreadsClosing() CANTHROW;

private:
    bool isSystemIdRegistered(threadid_t sysId, AbstractSphereThread** outExisting) const noexcept;

};

#ifdef THREAD_TRACK_CALLSTACK
class StackDebugInformation
{
private:
    AbstractSphereThread* m_context;

public:
    StackDebugInformation(const char *name) noexcept;
    ~StackDebugInformation() noexcept;

    StackDebugInformation(const StackDebugInformation&) = delete;
    StackDebugInformation& operator=(const StackDebugInformation&) = delete;

public:
    static void printStackTrace() noexcept;
    static void freezeCallStack(bool freeze) noexcept;
};

#define ADDTOCALLSTACK(_function_)    const StackDebugInformation debugStack(_function_)

#ifdef _DEBUG
#define ADDTOCALLSTACK_DEBUG(_function_)  ADDTOCALLSTACK(_function_)
#else
#define ADDTOCALLSTACK_DEBUG(_function_)  (void)0
#endif

#else // ! THREAD_TRACK_CALLSTACK

#define ADDTOCALLSTACK(_function_)           (void)0
#define ADDTOCALLSTACK_DEBUG(_function_)     (void)0

#endif

#endif // _INC_THREADS_H
