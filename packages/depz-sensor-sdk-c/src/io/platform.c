/* platform.c — see platform.h. */
#if defined(__APPLE__)
#  define _DARWIN_C_SOURCE /* pthread_cond_timedwait_relative_np */
#elif !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#  define _POSIX_C_SOURCE 200809L
#endif

#include "platform.h"

#include <stdlib.h>

typedef struct {
    depz_thread_fn fn;
    void *arg;
} thread_start;

#ifdef _WIN32

int  depz_mutex_init(depz_mutex *m) { InitializeCriticalSection(m); return 0; }
void depz_mutex_destroy(depz_mutex *m) { DeleteCriticalSection(m); }
void depz_mutex_lock(depz_mutex *m) { EnterCriticalSection(m); }
void depz_mutex_unlock(depz_mutex *m) { LeaveCriticalSection(m); }

int  depz_cond_init(depz_cond *c) { InitializeConditionVariable(c); return 0; }
void depz_cond_destroy(depz_cond *c) { (void)c; }
void depz_cond_broadcast(depz_cond *c) { WakeAllConditionVariable(c); }

bool depz_cond_wait_ms(depz_cond *c, depz_mutex *m, int timeout_ms)
{
    DWORD ms = timeout_ms < 0 ? INFINITE : (DWORD)timeout_ms;
    return SleepConditionVariableCS(c, m, ms) != 0;
}

static DWORD WINAPI thread_main(LPVOID p)
{
    thread_start s = *(thread_start *)p;
    free(p);
    s.fn(s.arg);
    return 0;
}

int depz_thread_start(depz_thread *t, depz_thread_fn fn, void *arg)
{
    thread_start *s = (thread_start *)malloc(sizeof *s);
    if (!s) return -1;
    s->fn = fn;
    s->arg = arg;
    *t = CreateThread(NULL, 0, thread_main, s, 0, NULL);
    if (!*t) { free(s); return -1; }
    return 0;
}

void depz_thread_join(depz_thread t)
{
    WaitForSingleObject(t, INFINITE);
    CloseHandle(t);
}

bool depz_thread_is_current(depz_thread t)
{
    return GetThreadId(t) == GetCurrentThreadId();
}

uint64_t depz_now_us(void)
{
    static LARGE_INTEGER freq;
    LARGE_INTEGER now;
    if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&now);
    return (uint64_t)((now.QuadPart / freq.QuadPart) * 1000000ULL
                      + (now.QuadPart % freq.QuadPart) * 1000000ULL / freq.QuadPart);
}

void depz_sleep_ms(int ms) { Sleep(ms < 0 ? 0 : (DWORD)ms); }

#else /* POSIX */

#include <errno.h>
#include <time.h>

int  depz_mutex_init(depz_mutex *m) { return pthread_mutex_init(m, NULL) ? -1 : 0; }
void depz_mutex_destroy(depz_mutex *m) { pthread_mutex_destroy(m); }
void depz_mutex_lock(depz_mutex *m) { pthread_mutex_lock(m); }
void depz_mutex_unlock(depz_mutex *m) { pthread_mutex_unlock(m); }

int depz_cond_init(depz_cond *c)
{
#ifdef __APPLE__
    /* macOS has no pthread_condattr_setclock; waits use the relative call. */
    return pthread_cond_init(c, NULL) ? -1 : 0;
#else
    pthread_condattr_t a;
    int rc;
    if (pthread_condattr_init(&a)) return -1;
    pthread_condattr_setclock(&a, CLOCK_MONOTONIC);
    rc = pthread_cond_init(c, &a);
    pthread_condattr_destroy(&a);
    return rc ? -1 : 0;
#endif
}

void depz_cond_destroy(depz_cond *c) { pthread_cond_destroy(c); }
void depz_cond_broadcast(depz_cond *c) { pthread_cond_broadcast(c); }

bool depz_cond_wait_ms(depz_cond *c, depz_mutex *m, int timeout_ms)
{
    struct timespec ts;
    if (timeout_ms < 0) {
        pthread_cond_wait(c, m);
        return true;
    }
#ifdef __APPLE__
    ts.tv_sec = timeout_ms / 1000;
    ts.tv_nsec = (long)(timeout_ms % 1000) * 1000000L;
    return pthread_cond_timedwait_relative_np(c, m, &ts) != ETIMEDOUT;
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (long)(timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000L;
    }
    return pthread_cond_timedwait(c, m, &ts) != ETIMEDOUT;
#endif
}

static void *thread_main(void *p)
{
    thread_start s = *(thread_start *)p;
    free(p);
    s.fn(s.arg);
    return NULL;
}

int depz_thread_start(depz_thread *t, depz_thread_fn fn, void *arg)
{
    thread_start *s = (thread_start *)malloc(sizeof *s);
    if (!s) return -1;
    s->fn = fn;
    s->arg = arg;
    if (pthread_create(t, NULL, thread_main, s)) { free(s); return -1; }
    return 0;
}

void depz_thread_join(depz_thread t) { pthread_join(t, NULL); }
bool depz_thread_is_current(depz_thread t) { return pthread_equal(t, pthread_self()) != 0; }

uint64_t depz_now_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

void depz_sleep_ms(int ms)
{
    struct timespec ts;
    if (ms <= 0) return;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    while (nanosleep(&ts, &ts) == -1 && errno == EINTR) {
    }
}

#endif

int depz_ms_until(uint64_t deadline_us)
{
    uint64_t now = depz_now_us();
    uint64_t left;
    if (now >= deadline_us) return 0;
    left = (deadline_us - now + 999) / 1000;
    return left > 0x7FFFFFFF ? 0x7FFFFFFF : (int)left;
}
