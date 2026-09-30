/*
 * platform.h — the few OS services the I/O layer needs, behind one small API:
 * a thread, a mutex, a condition variable with a relative timeout, a
 * monotonic clock. POSIX (Linux, macOS) and Win32. Internal, not installed.
 */
#ifndef DEPZ_IO_PLATFORM_H
#define DEPZ_IO_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
typedef CRITICAL_SECTION   depz_mutex;
typedef CONDITION_VARIABLE depz_cond;
typedef HANDLE             depz_thread;
#else
#  include <pthread.h>
typedef pthread_mutex_t depz_mutex;
typedef pthread_cond_t  depz_cond;
typedef pthread_t       depz_thread;
#endif

typedef void (*depz_thread_fn)(void *arg);

int  depz_mutex_init(depz_mutex *m);
void depz_mutex_destroy(depz_mutex *m);
void depz_mutex_lock(depz_mutex *m);
void depz_mutex_unlock(depz_mutex *m);

int  depz_cond_init(depz_cond *c);
void depz_cond_destroy(depz_cond *c);
/* Wait at most `timeout_ms` (< 0 = forever). Returns false on timeout. Spurious
 * wake-ups are possible: callers loop on their predicate with a deadline. */
bool depz_cond_wait_ms(depz_cond *c, depz_mutex *m, int timeout_ms);
void depz_cond_broadcast(depz_cond *c);

int  depz_thread_start(depz_thread *t, depz_thread_fn fn, void *arg);
void depz_thread_join(depz_thread t);
bool depz_thread_is_current(depz_thread t);

/* Monotonic clock, µs since an arbitrary origin. */
uint64_t depz_now_us(void);
void     depz_sleep_ms(int ms);

/* Milliseconds left until `deadline_us` (0 when past), clamped to int. */
int depz_ms_until(uint64_t deadline_us);

#endif /* DEPZ_IO_PLATFORM_H */
