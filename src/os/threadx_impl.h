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

#ifdef __cplusplus
}
#endif

#endif // USING_THREADX

#endif // _OS_THREADX_IMPL_H_
