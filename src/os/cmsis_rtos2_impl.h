/** \copyright
 * Copyright (c) 2025
 * All rights reserved.
 *
 * \file cmsis_rtos2_impl.h
 * CMSIS-RTOS v2 implementation for OpenMRN OS abstraction layer.
 *
 * @date 17 December 2025
 */

#ifndef _OS_CMSIS_RTOS2_IMPL_H_
#define _OS_CMSIS_RTOS2_IMPL_H_

#ifdef USING_CMSIS_RTOS_V2

#include "cmsis_os2.h"

// ============================================================================
// CMSIS-RTOS v2 specific feature definitions
// ============================================================================

/// Compile os_sem_timedwait functions.
#define OPENMRN_FEATURE_SEM_TIMEDWAIT 1

/// Enables use of Notifiable::notify_from_isr and OSSem::post_from_isr.
#define OPENMRN_FEATURE_RTOS_FROM_ISR 1

// CMSIS-RTOS v2 support is minimal for now
// TODO: Implement DEVTAB, REENT, FD_CAN_DEVICE, BSD_SOCKETS and other features for CMSIS-RTOS v2

#ifdef __cplusplus
extern "C" {
#endif

// CMSIS-RTOS v2 type mappings
typedef osThreadId_t os_thread_t;

typedef struct {
    osMutexId_t mutex;
    char recursive;
} os_mutex_t;

typedef osMessageQueueId_t os_mq_t;

typedef struct {
    osSemaphoreId_t sem;
} os_sem_t;

typedef struct
{
    unsigned char state; /**< keep track if already executed */
} os_thread_once_t; /**< one time initialization type */

// Static initializer macros (CMSIS-RTOS v2 requires runtime initialization)
/** Static initializer for mutexes */
#define OS_MUTEX_INITIALIZER {{0}, 0}
/** Static initializer for recursive mutexes */
#define OS_RECURSIVE_MUTEX_INITIALIZER {{0}, 1}

// CMSIS-RTOS v2 implementation functions
int os_thread_create_cmsis(os_thread_t *thread, const char *name, int priority,
                           size_t stack_size, void *(*entry)(void *), void *arg);

int os_mutex_init_cmsis(os_mutex_t *mutex);
int os_mutex_lock_cmsis(os_mutex_t *mutex);
int os_mutex_unlock_cmsis(os_mutex_t *mutex);

int os_sem_init_cmsis(os_sem_t *sem, unsigned int value);
int os_sem_wait_cmsis(os_sem_t *sem);
int os_sem_post_cmsis(os_sem_t *sem);
int os_sem_timedwait_cmsis(os_sem_t *sem, long long timeout_nsec);

os_thread_t os_thread_self_cmsis(void);
void os_thread_sleep_cmsis(long long nsec);

int os_mutex_destroy_cmsis(os_mutex_t *mutex);
int os_sem_destroy_cmsis(os_sem_t *sem);

// Inline function implementations
static inline os_thread_t os_thread_self(void)
{
    return os_thread_self_cmsis();
}

static inline int os_thread_get_priority(os_thread_t thread)
{
    osPriority_t priority = osThreadGetPriority(thread);
    return (priority == osPriorityError) ? 16 : priority;
}

static inline int os_thread_get_priority_min(void)
{
    return osPriorityIdle;
}

static inline int os_thread_get_priority_max(void)
{
    return osPriorityISR;
}

static inline int os_mutex_init(os_mutex_t *mutex)
{
    return os_mutex_init_cmsis(mutex);
}

static inline int os_recursive_mutex_init(os_mutex_t *mutex)
{
    // CMSIS-RTOS v2: handle recursive mutexes
    return os_mutex_init_cmsis(mutex);
}

static inline int os_mutex_destroy(os_mutex_t *mutex)
{
    return os_mutex_destroy_cmsis(mutex);
}

static inline int os_mutex_lock(os_mutex_t *mutex)
{
    return os_mutex_lock_cmsis(mutex);
}

static inline int os_mutex_unlock(os_mutex_t *mutex)
{
    return os_mutex_unlock_cmsis(mutex);
}

static inline int os_sem_init(os_sem_t *sem, unsigned int value)
{
    return os_sem_init_cmsis(sem, value);
}

static inline int os_sem_destroy(os_sem_t *sem)
{
    return os_sem_destroy_cmsis(sem);
}

static inline int os_sem_post(os_sem_t *sem)
{
    return os_sem_post_cmsis(sem);
}

#if OPENMRN_FEATURE_RTOS_FROM_ISR
static inline int os_sem_post_from_isr(os_sem_t *sem, int *woken)
{
    // CMSIS-RTOS v2: semaphore release is safe from ISR context
    return os_sem_post_cmsis(sem);
}
#endif

static inline int os_sem_wait(os_sem_t *sem)
{
    return os_sem_wait_cmsis(sem);
}

#if OPENMRN_FEATURE_SEM_TIMEDWAIT
static inline int os_sem_timedwait(os_sem_t *sem, long long timeout)
{
    if (timeout == OPENMRN_OS_WAIT_FOREVER)
    {
        return os_sem_wait(sem);
    }
    int result = os_sem_timedwait_cmsis(sem, timeout);
    if (result != 0) {
        errno = ETIMEDOUT;
    }
    return result;
}
#endif

static inline os_mq_t os_mq_create(size_t length, size_t item_size)
{
    DIE("unimplemented.");
    return NULL;
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

#endif // USING_CMSIS_RTOS_V2

#endif // _OS_CMSIS_RTOS2_IMPL_H_
