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

#ifdef __cplusplus
}
#endif

#endif // USING_CMSIS_RTOS_V2

#endif // _OS_CMSIS_RTOS2_IMPL_H_
