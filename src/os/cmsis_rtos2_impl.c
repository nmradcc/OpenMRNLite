/** \copyright
 * Copyright (c) 2025
 * All rights reserved.
 *
 * \file cmsis_rtos2_impl.c
 * CMSIS-RTOS v2 implementation for OpenMRN OS abstraction layer.
 *
 * @date 17 December 2025
 */

#ifdef USING_CMSIS_RTOS_V2

#include "os/os.h"
#include "rtos_includes.h"
#include <stdlib.h>

// Thread entry wrapper structure
typedef struct {
    void *(*user_entry)(void *);
    void *user_arg;
} thread_wrapper_t;

// Thread entry wrapper function
static void thread_entry_wrapper(void *argument)
{
    thread_wrapper_t *wrapper = (thread_wrapper_t *)argument;
    void *(*entry)(void *) = wrapper->user_entry;
    void *arg = wrapper->user_arg;
    
    // Call user entry point
    entry(arg);
    
    // Thread completed
    free(wrapper);
}

int os_thread_create_cmsis(os_thread_t *thread, const char *name, int priority,
                           size_t stack_size, void *(*entry)(void *), void *arg)
{
    // Create wrapper for entry point
    thread_wrapper_t *wrapper = (thread_wrapper_t *)malloc(sizeof(thread_wrapper_t));
    if (!wrapper) {
        return -1;
    }
    wrapper->user_entry = entry;
    wrapper->user_arg = arg;
    
    // Convert priority (CMSIS-RTOS v2 priorities: osPriorityLow to osPriorityRealtime)
    osPriority_t os_priority;
    if (priority <= 0) {
        os_priority = osPriorityNormal;
    } else if (priority < 8) {
        os_priority = osPriorityBelowNormal;
    } else if (priority < 16) {
        os_priority = osPriorityNormal;
    } else if (priority < 24) {
        os_priority = osPriorityAboveNormal;
    } else {
        os_priority = osPriorityHigh;
    }
    
    // Configure thread attributes
    osThreadAttr_t attr = {
        .name = name,
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0,
        .stack_mem = NULL,
        .stack_size = stack_size,
        .priority = os_priority,
        .tz_module = 0,
        .reserved = 0
    };
    
    // Create CMSIS-RTOS v2 thread
    osThreadId_t thread_id = osThreadNew(thread_entry_wrapper, wrapper, &attr);
    
    if (thread_id == NULL) {
        free(wrapper);
        return -1;
    }
    
    *thread = thread_id;
    return 0;
}

int os_mutex_init_cmsis(os_mutex_t *mutex)
{
    const osMutexAttr_t attr = {
        .name = "openmrn_mutex",
        .attr_bits = osMutexRecursive | osMutexPrioInherit,
        .cb_mem = NULL,
        .cb_size = 0
    };
    
    mutex->mutex = osMutexNew(&attr);
    mutex->recursive = 1;
    return (mutex->mutex != NULL) ? 0 : -1;
}

int os_mutex_lock_cmsis(os_mutex_t *mutex)
{
    osStatus_t status = osMutexAcquire(mutex->mutex, osWaitForever);
    return (status == osOK) ? 0 : -1;
}

int os_mutex_unlock_cmsis(os_mutex_t *mutex)
{
    osStatus_t status = osMutexRelease(mutex->mutex);
    return (status == osOK) ? 0 : -1;
}

int os_sem_init_cmsis(os_sem_t *sem, unsigned int value)
{
    const osSemaphoreAttr_t attr = {
        .name = "openmrn_sem",
        .attr_bits = 0,
        .cb_mem = NULL,
        .cb_size = 0
    };
    
    sem->sem = osSemaphoreNew(UINT32_MAX, value, &attr);
    return (sem->sem != NULL) ? 0 : -1;
}

int os_sem_wait_cmsis(os_sem_t *sem)
{
    osStatus_t status = osSemaphoreAcquire(sem->sem, osWaitForever);
    return (status == osOK) ? 0 : -1;
}

int os_sem_post_cmsis(os_sem_t *sem)
{
    osStatus_t status = osSemaphoreRelease(sem->sem);
    return (status == osOK) ? 0 : -1;
}

int os_sem_timedwait_cmsis(os_sem_t *sem, long long timeout_nsec)
{
    uint32_t ticks = NSEC_TO_TICK(timeout_nsec);
    osStatus_t status = osSemaphoreAcquire(sem->sem, ticks);
    return (status == osOK) ? 0 : -1;
}

os_thread_t os_thread_self_cmsis(void)
{
    return osThreadGetId();
}

void os_thread_sleep_cmsis(long long nsec)
{
    uint32_t ticks = NSEC_TO_TICK(nsec);
    if (ticks == 0) ticks = 1;
    osDelay(ticks);
}

#endif // USING_CMSIS_RTOS_V2
