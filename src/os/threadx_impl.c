/** \copyright
 * Copyright (c) 2025
 * All rights reserved.
 *
 * \file threadx_impl.c
 * ThreadX RTOS implementation for OpenMRN OS abstraction layer.
 *
 * @date 17 December 2025
 */

#ifdef USING_THREADX

#include "os/os.h"
#include "rtos_includes.h"
#include <stdlib.h>

// Thread entry wrapper structure
typedef struct {
    void *(*user_entry)(void *);
    void *user_arg;
} thread_wrapper_t;

// Thread entry wrapper function
static void thread_entry_wrapper(ULONG thread_input)
{
    thread_wrapper_t *wrapper = (thread_wrapper_t *)thread_input;
    void *(*entry)(void *) = wrapper->user_entry;
    void *arg = wrapper->user_arg;
    
    // Call user entry point
    entry(arg);
    
    // Thread completed
    free(wrapper);
}

int os_thread_create_threadx(os_thread_t *thread, const char *name, int priority,
                             size_t stack_size, void *(*entry)(void *), void *arg)
{
    // Allocate thread control block
    TX_THREAD *tx_thread = (TX_THREAD *)malloc(sizeof(TX_THREAD));
    if (!tx_thread) {
        return -1;
    }
    
    // Allocate stack
    void *stack = malloc(stack_size);
    if (!stack) {
        free(tx_thread);
        return -1;
    }
    
    // Create wrapper for entry point
    thread_wrapper_t *wrapper = (thread_wrapper_t *)malloc(sizeof(thread_wrapper_t));
    if (!wrapper) {
        free(stack);
        free(tx_thread);
        return -1;
    }
    wrapper->user_entry = entry;
    wrapper->user_arg = arg;
    
    // Convert priority (ThreadX priorities are 0-31, lower number = higher priority)
    UINT tx_priority = (priority <= 0) ? 16 : priority;
    if (tx_priority > 31) tx_priority = 31;
    
    // Create ThreadX thread
    UINT status = tx_thread_create(tx_thread, (CHAR *)name, thread_entry_wrapper,
                                   (ULONG)wrapper, stack, stack_size,
                                   tx_priority, tx_priority, TX_NO_TIME_SLICE,
                                   TX_AUTO_START);
    
    if (status != TX_SUCCESS) {
        free(wrapper);
        free(stack);
        free(tx_thread);
        return -1;
    }
    
    *thread = tx_thread;
    return 0;
}

int os_mutex_init_threadx(os_mutex_t *mutex)
{
    UINT status = tx_mutex_create(&mutex->mutex, "openmrn_mutex", TX_NO_INHERIT);
    mutex->recursive = 1; // ThreadX mutexes are inherently recursive
    return (status == TX_SUCCESS) ? 0 : -1;
}

int os_mutex_lock_threadx(os_mutex_t *mutex)
{
    UINT status = tx_mutex_get(&mutex->mutex, TX_WAIT_FOREVER);
    return (status == TX_SUCCESS) ? 0 : -1;
}

int os_mutex_unlock_threadx(os_mutex_t *mutex)
{
    UINT status = tx_mutex_put(&mutex->mutex);
    return (status == TX_SUCCESS) ? 0 : -1;
}

int os_sem_init_threadx(os_sem_t *sem, unsigned int value)
{
    UINT status = tx_semaphore_create(&sem->sem, "openmrn_sem", value);
    return (status == TX_SUCCESS) ? 0 : -1;
}

int os_sem_wait_threadx(os_sem_t *sem)
{
    UINT status = tx_semaphore_get(&sem->sem, TX_WAIT_FOREVER);
    return (status == TX_SUCCESS) ? 0 : -1;
}

int os_sem_post_threadx(os_sem_t *sem)
{
    UINT status = tx_semaphore_put(&sem->sem);
    return (status == TX_SUCCESS) ? 0 : -1;
}

int os_sem_timedwait_threadx(os_sem_t *sem, long long timeout_nsec)
{
    ULONG ticks = NSEC_TO_TICK(timeout_nsec);
    UINT status = tx_semaphore_get(&sem->sem, ticks);
    return (status == TX_SUCCESS) ? 0 : -1;
}

os_thread_t os_thread_self_threadx(void)
{
    return tx_thread_identify();
}

void os_thread_sleep_threadx(long long nsec)
{
    ULONG ticks = NSEC_TO_TICK(nsec);
    if (ticks == 0) ticks = 1;
    tx_thread_sleep(ticks);
}

#endif // USING_THREADX
