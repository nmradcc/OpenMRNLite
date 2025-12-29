/** \copyright
 * Copyright (c) 2025
 * All rights reserved.
 *
 * \file threadx_impl.h
 * ThreadX RTOS implementation for OpenMRN OS abstraction layer.
 *
 * @date 17 December 2025
 */

#ifndef _OS_THREADX_IMPL_H_
#define _OS_THREADX_IMPL_H_

#ifdef USING_THREADX

#include "tx_api.h"

// ============================================================================
// ThreadX-specific feature definitions
// ============================================================================

/// Compile os_sem_timedwait functions.
#define OPENMRN_FEATURE_SEM_TIMEDWAIT 1

/// Enables use of Notifiable::notify_from_isr and OSSem::post_from_isr.
#define OPENMRN_FEATURE_RTOS_FROM_ISR 1

// ThreadX support is minimal for now
// TODO: Implement DEVTAB, REENT, FD_CAN_DEVICE, BSD_SOCKETS and other features for ThreadX

#ifdef __cplusplus
extern "C" {
#endif

// ThreadX type mappings
typedef TX_THREAD* os_thread_t;

typedef struct {
    TX_MUTEX mutex;
    char recursive;
} os_mutex_t;

typedef TX_QUEUE* os_mq_t;

typedef struct {
    TX_SEMAPHORE sem;
} os_sem_t;

typedef struct
{
    unsigned char state; /**< keep track if already executed */
} os_thread_once_t; /**< one time initialization type */

// Static initializer macros
/** Static initializer for mutexes (ThreadX mutexes must be initialized at runtime) */
#define OS_MUTEX_INITIALIZER {{0}, 0}
/** Static initializer for recursive mutexes (ThreadX mutexes must be initialized at runtime) */
#define OS_RECURSIVE_MUTEX_INITIALIZER {{0}, 1}

// ThreadX implementation functions
int os_thread_create_threadx(os_thread_t *thread, const char *name, int priority,
                             size_t stack_size, void *(*entry)(void *), void *arg);

int os_mutex_init_threadx(os_mutex_t *mutex);
int os_mutex_lock_threadx(os_mutex_t *mutex);
int os_mutex_unlock_threadx(os_mutex_t *mutex);

int os_sem_init_threadx(os_sem_t *sem, unsigned int value);
int os_sem_wait_threadx(os_sem_t *sem);
int os_sem_post_threadx(os_sem_t *sem);
int os_sem_timedwait_threadx(os_sem_t *sem, long long timeout_nsec);

os_thread_t os_thread_self_threadx(void);
void os_thread_sleep_threadx(long long nsec);

TX_QUEUE* os_mq_create_threadx(size_t length, size_t item_size);
int os_mutex_destroy_threadx(os_mutex_t *mutex);

// ============================================================================
// Inline function implementations
// ============================================================================

static inline os_thread_t os_thread_self(void)
{
    return os_thread_self_threadx();
}

static inline int os_thread_get_priority(os_thread_t thread)
{
    UINT priority;
    UINT status = tx_thread_info_get(thread, NULL, NULL, NULL, &priority, NULL, NULL, NULL, NULL);
    return (status == TX_SUCCESS) ? priority : 16;
}

static inline int os_thread_get_priority_min(void)
{
    return 0; // ThreadX priority 0 is highest
}

static inline int os_thread_get_priority_max(void)
{
    return 31; // ThreadX priority 31 is lowest
}

static inline int os_mutex_init(os_mutex_t *mutex)
{
    return os_mutex_init_threadx(mutex);
}

static inline int os_recursive_mutex_init(os_mutex_t *mutex)
{
    // ThreadX mutexes are inherently recursive
    return os_mutex_init_threadx(mutex);
}

static inline int os_mutex_destroy(os_mutex_t *mutex)
{
    return os_mutex_destroy_threadx(mutex);
}

static inline int os_mutex_lock(os_mutex_t *mutex)
{
    return os_mutex_lock_threadx(mutex);
}

static inline int os_mutex_unlock(os_mutex_t *mutex)
{
    return os_mutex_unlock_threadx(mutex);
}

static inline int os_sem_init(os_sem_t *sem, unsigned int value)
{
    return os_sem_init_threadx(sem, value);
}

static inline int os_sem_destroy(os_sem_t *sem)
{
    UINT status = tx_semaphore_delete(&sem->sem);
    return (status == TX_SUCCESS) ? 0 : -1;
}

static inline int os_sem_post(os_sem_t *sem)
{
    return os_sem_post_threadx(sem);
}

#if OPENMRN_FEATURE_RTOS_FROM_ISR
static inline int os_sem_post_from_isr(os_sem_t *sem, int *woken)
{
    // ThreadX: semaphore post is safe from ISR context
    return os_sem_post_threadx(sem);
}
#endif

static inline int os_sem_wait(os_sem_t *sem)
{
    return os_sem_wait_threadx(sem);
}

#if OPENMRN_FEATURE_SEM_TIMEDWAIT
static inline int os_sem_timedwait(os_sem_t *sem, long long timeout)
{
    if (timeout == OPENMRN_OS_WAIT_FOREVER)
    {
        return os_sem_wait(sem);
    }
    int result = os_sem_timedwait_threadx(sem, timeout);
    if (result != 0) {
        errno = ETIMEDOUT;
    }
    return result;
}
#endif

static inline os_mq_t os_mq_create(size_t length, size_t item_size)
{
    return os_mq_create_threadx(length, item_size);
}

static inline void os_mq_send(os_mq_t queue, const void *data)
{
    DIE("unimplemented.");
}

static inline int os_mq_timedsend(os_mq_t queue, const void *data, long long timeout)
{
    DIE("unimplemented.");
    return OS_MQ_NONE;
}

static inline void os_mq_receive(os_mq_t queue, void *data)
{
    DIE("unimplemented.");
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

#ifdef __cplusplus
}
#endif

#endif // USING_THREADX

#endif // _OS_THREADX_IMPL_H_
