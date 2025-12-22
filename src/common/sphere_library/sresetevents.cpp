#include "sresetevents.h"
#include <time.h>

#ifdef _WIN32
#include <errno.h>
#else
// Helper: convert “now + ms” to an absolute timespec
static void make_timespec(struct timespec& ts, uint32_t ms) noexcept
{
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec  += ms / 1000;
    ts.tv_nsec += (ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L)  //static_cast<decltype(time.tv_nsec)>(1.0e9);
    {
        ts.tv_sec  += 1;
        ts.tv_nsec -= 1000000000L;
    }
}
#endif


AutoResetEvent::AutoResetEvent() noexcept
{
#ifdef _WIN32
    m_handle = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);          // auto-reset
#else
    pthread_mutex_init(&m_criticalSection, nullptr);
    pthread_cond_init (&m_condition , nullptr);
    m_signaled = false;
#endif
}

AutoResetEvent::~AutoResetEvent() noexcept
{
#ifdef _WIN32
    ::CloseHandle(m_handle);
#else
    pthread_cond_destroy (&m_condition);
    pthread_mutex_destroy(&m_criticalSection);
#endif
}

void AutoResetEvent::wait(uint32 timeout) noexcept
{
#ifdef _WIN32
    if (timeout == 0)
    {
        // If timeout is 0 then the thread's timeslice may not be given up as with normal
        // sleep methods - so we will check for this condition ourselves and use SleepEx
        // instead
        ::SleepEx(0, TRUE);

        // or, if we want a non-blocking probe
        //::WaitForSingleObjectEx(m_handle, 0, FALSE);
        return;
    }

    const DWORD start = ::GetTickCount();
    DWORD remaining  = timeout;

    while (true)
    {
        DWORD rc = ::WaitForSingleObjectEx(m_handle, remaining, TRUE);
        if (rc == WAIT_OBJECT_0 || rc == WAIT_TIMEOUT)
            return;                // signalled or timed out

        // WAIT_IO_COMPLETION – recalc remaining time
        DWORD now = ::GetTickCount();
        if (now - start >= timeout)
            return;
        remaining = timeout - (now - start);
    }
#else   // POSIX
    pthread_mutex_lock(&m_criticalSection);

    if (timeout == 0 && !m_signaled)
    {   // immediate return
        pthread_mutex_unlock(&m_criticalSection);
        return;
    }

    struct timespec ts;
    if (timeout != _kiInfinite)
        make_timespec(ts, timeout);

    // Check m_signaled to protect outselves from spurious wakeups, like above, but leverage POSIX threads
    int res = 0;
    while (!m_signaled && res == 0)
    {
        if (timeout == _kiInfinite)
            res = pthread_cond_wait(&m_condition, &m_criticalSection);
        else
            res = pthread_cond_timedwait(&m_condition, &m_criticalSection, &ts);
    }

    if (m_signaled)
        m_signaled = false;                 // auto-reset

    pthread_mutex_unlock(&m_criticalSection);
#endif
}

void AutoResetEvent::signal() noexcept
{
#ifdef _WIN32
    ::SetEvent(m_handle);                   // kernel handles auto-reset
#else
    pthread_mutex_lock(&m_criticalSection);
    m_signaled = true;
    pthread_cond_signal(&m_condition);           // wake exactly one waiter
    pthread_mutex_unlock(&m_criticalSection);
#endif
}

/*────────────────────────────  Manual-reset event  ─────────────────────────*/

ManualResetEvent::ManualResetEvent() noexcept
{
#ifdef _WIN32
    m_handle = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);           // manual-reset
#else
    pthread_mutex_init(&m_criticalSection, nullptr);
    pthread_cond_init (&m_condition , nullptr);
    m_signaled = false;
#endif
}

ManualResetEvent::~ManualResetEvent() noexcept
{
#ifdef _WIN32
    ::CloseHandle(m_handle);
#else
    pthread_cond_destroy(&m_condition);
    pthread_mutex_destroy(&m_criticalSection);
#endif
}

void ManualResetEvent::wait(uint32 timeout) noexcept
{
#ifdef _WIN32
    if (timeout == 0)
    {
        ::WaitForSingleObjectEx(m_handle, 0, FALSE);
        return;
    }

    const DWORD start = ::GetTickCount();
    DWORD remaining  = timeout;

    while (true)
    {
        DWORD rc = ::WaitForSingleObjectEx(m_handle, remaining, TRUE);
        if (rc == WAIT_OBJECT_0 || rc == WAIT_TIMEOUT)
            return;

        DWORD now = ::GetTickCount();
        if (now - start >= timeout)
            return;
        remaining = timeout - (now - start);
    }

#else   // POSIX
    pthread_mutex_lock(&m_criticalSection);

    if (timeout == 0 && !m_signaled)
    {
        pthread_mutex_unlock(&m_criticalSection);
        return;
    }

    struct timespec ts;
    if (timeout != _kiInfinite)
        make_timespec(ts, timeout);

    int res = 0;
    while (!m_signaled && res == 0)
    {
        if (timeout == _kiInfinite)
            res = pthread_cond_wait(&m_condition, &m_criticalSection);
        else
            res = pthread_cond_timedwait(&m_condition, &m_criticalSection, &ts);
    }

    pthread_mutex_unlock(&m_criticalSection);
#endif
}

void ManualResetEvent::set() noexcept
{
#ifdef _WIN32
    ::SetEvent(m_handle);
#else
    pthread_mutex_lock(&m_criticalSection);
    m_signaled = true;
    pthread_cond_broadcast(&m_condition);        // wake *all* waiters
    pthread_mutex_unlock(&m_criticalSection);
#endif
}

void ManualResetEvent::reset() noexcept
{
#ifdef _WIN32
    ::ResetEvent(m_handle);
#else
    pthread_mutex_lock(&m_criticalSection);
    m_signaled = false;
    pthread_mutex_unlock(&m_criticalSection);
#endif
}
