/** \copyright
 * Copyright (c) 2012, Stuart W Baker
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are  permitted provided that the following conditions are met:
 * 
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \file os.c
 * This file represents a C language abstraction of common operating
 * system calls.
 *
 * @author Stuart W. Baker
 * @date 13 August 2012
 */

#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif // _DEFAULT_SOURCE

/// Forces one definition of each inline function to be compiled.
#define OS_INLINE extern

#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

// ThreadX build: include ThreadX API
#include "tx_api.h"

#include "nmranet_config.h"

#include "utils/macros.h"
#include "os/os.h"
#include "os/os_private.h"

/** default stdin */
extern const char *STDIN_DEVICE;

/** default stdout */
extern const char *STDOUT_DEVICE;

/** default stderr */
extern const char *STDERR_DEVICE;

/** Captures point of death (line). */
int g_death_lineno;
/** Captures point of death (file). */
const char* g_death_file;

/** clock management **/
long long rtcOffset = 0;

/** This magic value is written to a task's taskList entry in order to signal
 * the idle task to pick it out of the taskList structure. */
#define DELETED_TASK_MAGIC 0xb5c5d5e5

/* This section of code is required because CodeSourcery's mips-gcc
 * distribution contains a strangely compiled NewLib (in the unhosted-libc.a
 * version) that does not forward these function calls to the implementations
 * we have. We are thus forced to override their weak definition of these
 * functions. */

/** Mutex for os_thread_once. */
static os_mutex_t onceMutex = OS_MUTEX_INITIALIZER;

/** Default hardware initializer.  This function is defined weak so that
 * a given board can stub in an intiailization specific to it.
 */
void hw_init(void) __attribute__ ((weak));
void hw_init(void)
{
}

/** Default hardware post-initializer.  This function is called from the main
 * task, after the scheduler is started, but before appl_main is invoked. This
 * function is defined weak so that a given board can stub in an intiailization
 * specific to it.
 */
void hw_postinit(void) __attribute__ ((weak));
void hw_postinit(void)
{
}

/** One time intialization routine
 * @param once one time instance
 * @param routine method to call once
 * @return 0 upon success
 */
int os_thread_once(os_thread_once_t *once, void (*routine)(void))
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
    {
        /* The scheduler has started so we should use locking */
        os_mutex_lock(&onceMutex);
        if (once->state == OS_THREAD_ONCE_NEVER)
        {
            once->state = OS_THREAD_ONCE_INPROGRESS;
            os_mutex_unlock(&onceMutex);
            routine();
            os_mutex_lock(&onceMutex);
            once->state = OS_THREAD_ONCE_DONE;
        }

        while (once->state == OS_THREAD_ONCE_INPROGRESS)
        {
            /* avoid dead lock waiting for PTHREAD_ONCE_DONE state */
            os_mutex_unlock(&onceMutex);
            usleep(MSEC_TO_USEC(10));
            os_mutex_lock(&onceMutex);
        }
        os_mutex_unlock(&onceMutex);
    }
    else
    {
        /* this is for static constructures before the scheduler is started */
        if (once->state == OS_THREAD_ONCE_NEVER)
        {
            once->state = OS_THREAD_ONCE_INPROGRESS;
            routine();
            once->state = OS_THREAD_ONCE_DONE;
        }
    }

    return 0;
}
#endif

/** Create a thread.
 * @param thread handle to the created thread
 * @param name name of thread, NULL for an auto generated name
 * @param priority priority of created thread, 0 means default,
 *        lower numbers means lower priority, higher numbers mean higher priority
 * @param stack_size size in bytes of the created thread's stack
 * @param start_routine entry point of the thread
 * @param arg entry parameter to the thread
 * @return 0 upon success or error number upon failure
 */
int os_thread_create(os_thread_t *thread, const char *name, int priority,
                     size_t stack_size, void *(*start_routine) (void *),
                     void *arg)
{
    static unsigned int count = 0;
    char auto_name[10];

    if (name == NULL)
    {
        strcpy(auto_name, "thread.");
        auto_name[9] = '\0';
        auto_name[8] = '0' + (count % 10);
        auto_name[7] = '0' + (count / 10);
        count++;
        name = auto_name;
    }

#if OPENMRN_FEATURE_THREAD_PTHREAD    
    pthread_attr_t attr;

    int result = pthread_attr_init(&attr);
    if (result != 0)
    {
        return result;
    }
    result = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (result != 0)
    {
        return result;
    }

#if OPENMRN_FEATURE_PTHREAD_SETSTACK
    struct sched_param sched_param;
    result = pthread_attr_setstacksize(&attr, stack_size);
    if (result != 0)
    {
        return result;
    }

    sched_param.sched_priority = 0; /* default priority */
    result = pthread_attr_setschedparam(&attr, &sched_param);
    if (result != 0)
    {
        return result;
    }

    result = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    if (result != 0)
    {
        return result;
    }

    result = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    if (result != 0)
    {
        return result;
    }
#endif // no stack size set needed
    pthread_t local_thread_handle;
    if (!thread)
    {
        thread = &local_thread_handle;
    }
    result = pthread_create(thread, &attr, start_routine, arg);

#if OPENMRN_HAVE_PTHREAD_SETNAME
    if (!result)
    {
        pthread_setname_np(*thread, name);
    }
#endif // OPENMRN_HAVE_PTHREAD_SETNAME

    return result;
#endif // OPENMRN_FEATURE_THREAD_PTHREAD
}
#endif // !OPENMRN_FEATURE_RTOS_THREADX

/// Implement this function to read timing more accurately than 1 msec in
/// FreeRTOS.
extern long long hw_get_partial_tick_time_nsec(void);
/// Default implementation does not provide more accuracy.
long long __attribute__((weak)) hw_get_partial_tick_time_nsec() { return 0; }

long long os_get_time_monotonic(void)
{
    static long long last = 0;
    long long time;
    
    // ThreadX uses TX_TIMER_TICKS_PER_SECOND for tick frequency
    // Get current tick count
    extern unsigned long tx_time_get(void);
    unsigned long tick = tx_time_get();
    // Convert ticks to nanoseconds
    // Assuming TX_TIMER_TICKS_PER_SECOND is defined (typically 100 for ThreadX)
    #ifndef TX_TIMER_TICKS_PER_SECOND
    #define TX_TIMER_TICKS_PER_SECOND 100
    #endif // TX_TIMER_TICKS_PER_SECOND
    time = ((long long)tick * 1000000000LL) / TX_TIMER_TICKS_PER_SECOND;

    /* This logic ensures that every successive call is one value larger
     * than the last.  Each call returns a unique value.
     */
    os_atomic_lock();
    if (time <= last)
    {
        last++;
        time = last;
    }
    else
    {
        last = time;
    }
    os_atomic_unlock();
    
    return time;
}

// ThreadX-specific standard library implementations

/** Implementation of standard sleep().
 * @param seconds number of seconds to sleep
 */
unsigned sleep(unsigned seconds)
{
    extern unsigned int tx_thread_sleep(unsigned long ticks);
    tx_thread_sleep(seconds * TX_TIMER_TICKS_PER_SECOND);
    return 0;
}

/** Implementation of standard usleep().
 * @param usec number of microseconds to sleep
 */
int usleep(useconds_t usec)
{
    // Convert microseconds to ThreadX ticks
    extern unsigned int tx_thread_sleep(unsigned long ticks);
    unsigned long ticks = (usec * TX_TIMER_TICKS_PER_SECOND) / 1000000;
    if (ticks == 0 && usec > 0) ticks = 1;
    tx_thread_sleep(ticks);
    return 0;
}

void abort(void)
{
    for (;;)
    {
    }
}

// ThreadX implementation - no special heap management needed
ssize_t __attribute__((weak)) os_get_free_heap()
{
    return -1;
}

/** This function does nothing. It can be used to alias other symbols to it via
 * linker flags, such as atexit(). @return 0. */
int ignore_fn(void)
{
    return 0;
}

int main(int argc, char *argv[]) __attribute__ ((weak));

/** Entry point to program.
 * @param argc number of command line arguments
 * @param argv array of command line aguments
 * @return 0, should never return
 */
int main(int argc, char *argv[])
{
    // ThreadX: initialization handled by ThreadX application
    return appl_main(argc, argv);
}
