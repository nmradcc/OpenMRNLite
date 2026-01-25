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
 * \file os.h
 * This file represents a C language abstraction of common operating
 * system calls.
 *
 * @author Stuart W. Baker
 * @date 28 May 2012
 */

#ifndef _OS_OS_H_
#define _OS_OS_H_

#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>

#include "openmrn_features.h"

#include "utils/macros.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OS_INLINE
/// Forces one definition of each inline function to be compiled.
#define OS_INLINE extern inline __attribute__((__gnu_inline__))
#endif

// ============================================================================
// Common OS macros and constants (must be defined before impl.h includes)
// ============================================================================

#define OS_PRIO_MIN 1 /**< lowest thread priority supported by abstraction */
#define OS_PRIO_DEFAULT 0 /**< default thread priority */
#define OS_PRIO_MAX 32 /**< highest thread priority suported by abstraction */

#define OS_MQ_NONE     0 /**< error code for no error for message queues */
#define OS_MQ_TIMEDOUT 1 /**< error code for timedout for message queues */
#define OS_MQ_EMPTY    2 /**< error code for the queue being empty */
#define OS_MQ_FULL     3 /**< error code for queue being full */

#if defined LLONG_MAX
#define OPENMRN_OS_WAIT_FOREVER LLONG_MAX /**< maximum timeout period */
#else
#define OPENMRN_OS_WAIT_FOREVER __LONG_LONG_MAX__ /**< maximum timeout period */
#endif

/** @ref os_thread_once states.
 */
enum
{
    OS_THREAD_ONCE_NEVER = 0, ///< not yet executed
    OS_THREAD_ONCE_INPROGRESS, ///< execution in progress
    OS_THREAD_ONCE_DONE ///< execution complete
};
/** initial value for one time intitialization instance */
#define OS_THREAD_ONCE_INIT { OS_THREAD_ONCE_NEVER }

// ============================================================================
// Include ThreadX RTOS implementation
// ============================================================================

#include "rtos_includes.h"
#include "os/threadx_impl.h"

#if 0 // Legacy fallback code - not used in ThreadX-only version
// Fallback for single-threaded environments or when no RTOS is configured
typedef struct {
    int locked;
    uint8_t recursive;
} os_mutex_t;
typedef struct
{
    unsigned char state; /**< keep track if already executed */
} os_thread_once_t; /**< one time initialization type */
typedef struct {
    unsigned counter;
} os_sem_t;

typedef unsigned os_thread_t;
typedef void *os_mq_t; /**< message queue handle */

/** Private data structure for a queue, do not use directly */
typedef struct queue_priv
{
    os_sem_t semSend; /**< able to send semaphore */
    os_sem_t semReceive; /**< able to receive semaphore */
    char *buffer; /**< queue data */
    size_t itemSize; /**< size of each item in the queue */
    size_t bytes; /**< number of bytes that make up the queue */
    unsigned int indexSend; /**< current index for send */
    unsigned int indexReceive; /**< current index for receive */
    os_mutex_t mutex; /**< mutex to protect queue operations */
} QueuePriv;

/** Static initializer for mutexes */
#define OS_MUTEX_INITIALIZER {0, 0}
/** Static initializer for recursive mutexes */
#define OS_RECURSIVE_MUTEX_INITIALIZER {0, 1}

// Inline function implementations for fake mutex (single-threaded)
static inline os_thread_t os_thread_self(void)
{
    return 0xdeadbeef;
}

static inline int os_thread_get_priority(os_thread_t thread)
{
    return 2;
}

static inline int os_thread_get_priority_min(void)
{
    return 2;
}

static inline int os_thread_get_priority_max(void)
{
    return 2;
}

static inline int os_mutex_init(os_mutex_t *mutex)
{
    mutex->locked = 0;
    mutex->recursive = 0;
    return 0;
}

static inline int os_recursive_mutex_init(os_mutex_t *mutex)
{
    mutex->locked = 0;
    mutex->recursive = 1;
    return 0;
}

static inline int os_mutex_destroy(os_mutex_t *mutex)
{
    mutex->locked = 0;
    return 0;
}

static inline int os_mutex_lock(os_mutex_t *mutex)
{
    if (mutex->locked && !mutex->recursive)
    {
        DIE("Mutex deadlock.");
    }
    mutex->locked++;
    return 0;
}

static inline int os_mutex_unlock(os_mutex_t *mutex)
{
    if (mutex->locked <= 0)
    {
        DIE("Unlocking a not locked mutex");
    }
    --mutex->locked;
    return 0;
}

static inline int os_sem_init(os_sem_t *sem, unsigned int value)
{
    sem->counter = value;
    return 0;
}

static inline int os_sem_destroy(os_sem_t *sem)
{
    return 0;
}

static inline int os_sem_post(os_sem_t *sem)
{
    sem->counter++;
    return 0;
}

static inline int os_sem_wait(os_sem_t *sem)
{
    if (!sem->counter) {
        DIE("Semaphore deadlock.");
    }
    --sem->counter;
    return 0;
}

static inline os_mq_t os_mq_create(size_t length, size_t item_size)
{
    QueuePriv *q = (QueuePriv*)malloc(sizeof(QueuePriv));
    if (!q)
    {
        errno = ENOMEM;
        return NULL;
    }
    os_sem_init(&q->semSend, length);
    os_sem_init(&q->semReceive, 0);
    q->buffer = (char*)malloc(length * item_size);
    q->itemSize = item_size;
    q->bytes = length * item_size;
    q->indexSend = 0;
    q->indexReceive = 0;
    os_mutex_init(&q->mutex);
    return q;
}

static inline void os_mq_send(os_mq_t queue, const void *data)
{
    QueuePriv *q = (QueuePriv*)queue;
    
    os_sem_wait(&q->semSend);
    os_mutex_lock(&q->mutex);
    memcpy(q->buffer + q->indexSend, data, q->itemSize);
    q->indexSend += q->itemSize;
    if (q->indexSend >= q->bytes)
    {
        q->indexSend = 0;
    }
    os_mutex_unlock(&q->mutex);
    os_sem_post(&q->semReceive);
}

static inline void os_mq_receive(os_mq_t queue, void *data)
{
    QueuePriv *q = (QueuePriv*)queue;
    
    os_sem_wait(&q->semReceive);
    os_mutex_lock(&q->mutex);
    memcpy(data, q->buffer + q->indexReceive, q->itemSize);
    q->indexReceive += q->itemSize;
    if (q->indexReceive >= q->bytes)
    {
        q->indexReceive = 0;
    }
    os_mutex_unlock(&q->mutex);
    os_sem_post(&q->semSend);
}

static inline int os_mq_timedsend(os_mq_t queue, const void *data, long long timeout)
{
    DIE("unimplemented.");
    return OS_MQ_NONE;
}

static inline int os_mq_timedreceive(os_mq_t queue, void *data, long long timeout)
{
    DIE("unimplemented.");
    return OS_MQ_NONE;
}

static inline int os_mq_send_from_isr(os_mq_t queue, const void *data, int *woken)
{
    DIE("unimplemented.");
    return OS_MQ_NONE;
}

static inline int os_mq_is_full_from_isr(os_mq_t queue)
{
    DIE("unimplemented.");
    return 1;
}

static inline int os_mq_receive_from_isr(os_mq_t queue, void *data, int *woken)
{
    DIE("unimplemented.");
    return OS_MQ_NONE;
}

static inline int os_mq_num_pending(os_mq_t queue)
{
    DIE("unimplemented.");
    return 0;
}

static inline int os_mq_num_pending_from_isr(os_mq_t queue)
{
    DIE("unimplemented.");
    return 0;
}

static inline int os_mq_num_spaces(os_mq_t queue)
{
    DIE("unimplemented.");
    return 0;
}
#endif

#ifndef container_of
/** Get a pointer to the parent structure of one of its members.
 * @param _ptr original member pointer
 * @param _type parent structure type
 * @param _member name of member within structure
 * @return pointer to the parent structure
 */
#define container_of(_ptr, _type, _member)                  \
({                                                       \
    const typeof( ((_type *)0)->_member ) *__mptr = (_ptr); \
    (_type *)( (char *)__mptr - offsetof(_type,_member) );  \
})
#endif

/** Get the monotonic time since the system started.
 * @return time in nanoseconds since system start
 */
extern long long os_get_time_monotonic(void);

/** Get the fake time for a unit test.
 * @return time in nanoseconds, <= 0 if there is no fake clock.
 */
extern long long os_get_fake_time(void);

/** One time intialization routine
 * @param once one time instance
 * @param routine method to call once
 * @return 0 upon success
 */
int os_thread_once(os_thread_once_t *once, void (*routine)(void));

/** Entry point to application.
 * @param argc number of arguments
 * @param argv list of arguments
 * @return 0 upon success.
 */
int appl_main(int argc, char *argv[]);

/// @return the available heap or -1 if this operation is not supported.
ssize_t os_get_free_heap(void);

// Legacy FreeRTOS-specific declarations removed

#ifndef container_of
/** Get a pointer to the parent structure of one of its members.
 * @param _ptr original member pointer
 * @param _type parent structure type
 * @param _member name of member within structure
 * @return pointer to the parent structure
 */
#define container_of(_ptr, _type, _member)                  \
({                                                       \
    const typeof( ((_type *)0)->_member ) *__mptr = (_ptr); \
    (_type *)( (char *)__mptr - offsetof(_type,_member) );  \
})
#endif

/** Convert a nanosecond value to a microsecond value.
 * @param _nsec nanosecond value to convert
 * @return microsecond value
 */
#define NSEC_TO_USEC(_nsec) (((long long)_nsec) / 1000LL)

/** Convert a nanosecond value to a millisecond value.
 * @param _nsec nanosecond value to convert
 * @return milliosecond value
 */
#define NSEC_TO_MSEC(_nsec) (((long long)_nsec) / 1000000LL)

/** Convert a nanosecond value to a second value.
 * @param _nsec nanosecond value to convert
 * @return second value
 */
#define NSEC_TO_SEC(_nsec) (((long long)_nsec) / 1000000000LL)

/** Convert a nanosecond value to minutes.
 * @param _nsec nanosecond value to convert
 * @return minutes value
 */
#define NSEC_TO_MIN(_nsec) (((long long)_nsec) / 60000000000LL)

/** Convert a microsecond value to a nanosecond value.
 * @param _usec microsecond value to convert
 * @return nanosecond value
 */
#define USEC_TO_NSEC(_usec) (((long long)_usec) * 1000LL)

/** Convert a microsecond value to a millisecond value.
 * @param _usec microsecond value to convert
 * @return millisecond value
 */
#define USEC_TO_MSEC(_usec) (((long long)_usec) / 1000LL)

/** Convert a microsecond value to a second value.
 * @param _usec microsecond value to convert
 * @return second value
 */
#define USEC_TO_SEC(_usec) (((long long)_usec) / 1000000LL)

/** Convert a millisecond value to a nanosecond value.
 * @param _msec millisecond value to convert
 * @return nanosecond value
 */
#define MSEC_TO_NSEC(_msec) (((long long)_msec) * 1000000LL)

/** Convert a millisecond value to a microsecond value.
 * @param _msec millisecond value to convert
 * @return microsecond value
 */
#define MSEC_TO_USEC(_msec) (((long long)_msec) * 1000LL)

/** Convert a millisecond value to a second value.
 * @param _msec millisecond value to convert
 * @return second value
 */
#define MSEC_TO_SEC(_msec) (((long long)_msec) / 1000LL)

/** Convert a second value to a nanosecond value.
 * @param _sec second value to convert
 * @return nanosecond value
 */
#define SEC_TO_NSEC(_sec) (((long long)_sec) * 1000000000LL)

/** Convert a second value to a microsecond value.
 * @param _sec second value to convert
 * @return microsecond value
 */
#define SEC_TO_USEC(_sec) (((long long)_sec) * 1000000LL)

/** Convert a second value to a millisecond value.
 * @param _sec second value to convert
 * @return millisecond value
 */
#define SEC_TO_MSEC(_sec) (((long long)_sec) * 1000LL)

/** Convert a nanosecond value to a microsecond value.
 * @param _nsec nanosecond value to convert
 * @return microsecond value
 */
#define NSEC_TO_USEC_ROUNDED(_nsec) \
    ((((long long)_nsec) + 500LL) / 1000LL)

/** Convert a nanosecond value to a millisecond value.
 * @param _nsec nanosecond value to convert
 * @return milliosecond value
 */
#define NSEC_TO_MSEC_ROUNDED(_nsec) \
    ((((long long)_nsec) + 500000LL) / 1000000LL)

/** Convert a nanosecond value to a second value.
 * @param _nsec nanosecond value to convert
 * @return second value
 */
#define NSEC_TO_SEC_ROUNDED(_nsec) \
    ((((long long)_nsec) + 500000000LL) / 1000000000LL)

/** Convert a nanosecond value to minutes.
 * @param _nsec nanosecond value to convert
 * @return minutes value
 */
#define NSEC_TO_MIN_ROUNDED(_nsec) \
    ((((long long)_nsec) + 30000000000LL) / 60000000000LL)

/** Convert a microsecond value to a millisecond value.
 * @param _usec microsecond value to convert
 * @return millisecond value
 */
#define USEC_TO_MSEC_ROUNDED(_usec) \
    ((((long long)_usec) + 500LL) / 1000LL)

/** Convert a microsecond value to a second value.
 * @param _usec microsecond value to convert
 * @return second value
 */
#define USEC_TO_SEC_ROUNDED(_usec) \
    ((((long long)_usec) +500000LL) / 1000000LL)

/** Convert a millisecond value to a second value.
 * @param _msec millisecond value to convert
 * @return second value
 */
#define MSEC_TO_SEC_ROUNDED(_msec) \
    ((((long long)_msec) + 500LL) / 1000LL)

/** Creates a thread.
 * @param thread handle to the created thread
 * @param name name of thread, NULL for an auto generated name
 * @param priority priority of created thread, 0 means default
 * @param stack_size size in bytes of the created thread's stack
 * @param start_routine entry point of the thread
 * @param arg entry parameter to the thread
 * @return 0 upon success or error number upon failure
 */
int os_thread_create(os_thread_t *thread, const char *name, int priority,
                     size_t stack_size,
                     void *(*start_routine) (void *), void *arg);

/** Destroy a thread.
 * @param thread handle to the created thread
 */
void os_thread_cancel(os_thread_t thread);

// ============================================================================
// Inline function implementations for thread priority, mutex, semaphore, and
// message queue operations are provided by the RTOS-specific impl.h files
// (freertos_impl.h, threadx_impl.h, cmsis_rtos2_impl.h) which are included
// above based on the OPENMRN_FEATURE_RTOS_* macros.
//
// The impl.h files define these static inline functions:
//   Thread:     os_thread_self(), os_thread_get_priority(),
//               os_thread_get_priority_min(), os_thread_get_priority_max()
//   Mutex:      os_mutex_init(), os_recursive_mutex_init(), os_mutex_destroy(),
//               os_mutex_lock(), os_mutex_unlock()
//   Semaphore:  os_sem_init(), os_sem_destroy(), os_sem_post(), os_sem_wait()
//               os_sem_post_from_isr(), os_sem_timedwait() (if supported)
//   Msg Queue:  os_mq_create(), os_mq_send(), os_mq_timedsend(),
//               os_mq_receive(), os_mq_timedreceive(), os_mq_send_from_isr(),
//               os_mq_is_full_from_isr(), os_mq_receive_from_isr(),
//               os_mq_num_pending(), os_mq_num_pending_from_isr(), os_mq_num_spaces()
// ============================================================================




























/** Get the monotonic time since the system started.
 * @return time in nanoseconds since system start
 */
extern long long os_get_time_monotonic(void);


#ifdef __cplusplus
}
#endif

#endif /* _OS_OS_H_ */
