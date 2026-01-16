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
    printf("[THREAD] Creating thread '%s' with priority=%d, stack_size=%lu\r\n", 
           name ? name : "unnamed", priority, (unsigned long)stack_size);
    
    // Allocate thread control block
    TX_THREAD *tx_thread = (TX_THREAD *)malloc(sizeof(TX_THREAD));
    if (!tx_thread) {
        printf("[THREAD] ERROR: Failed to allocate thread control block\r\n");
        return -1;
    }
    
    // Enforce minimum stack size
    if (stack_size == 0) {
        stack_size = 2048;  // Default stack size for STM32
    }
    if (stack_size < TX_MINIMUM_STACK) {
        stack_size = TX_MINIMUM_STACK;
    }
    
    printf("[THREAD] Final stack size: %lu bytes\r\n", (unsigned long)stack_size);
    
    // Allocate stack
    void *stack = malloc(stack_size);
    if (!stack) {
        printf("[THREAD] ERROR: Failed to allocate stack (%lu bytes)\r\n", (unsigned long)stack_size);
        free(tx_thread);
        return -1;
    }
    printf("[THREAD] Stack allocated at %p\r\n", stack);
    
    // Create wrapper for entry point
    thread_wrapper_t *wrapper = (thread_wrapper_t *)malloc(sizeof(thread_wrapper_t));
    if (!wrapper) {
        printf("[THREAD] ERROR: Failed to allocate thread wrapper\r\n");
        free(stack);
        free(tx_thread);
        return -1;
    }
    wrapper->user_entry = entry;
    wrapper->user_arg = arg;
    
    // Convert priority (ThreadX priorities are 0-31, lower number = higher priority)
    UINT tx_priority = (priority <= 0) ? 16 : priority;
    if (tx_priority > 31) tx_priority = 31;
    
    printf("[THREAD] Calling tx_thread_create with priority=%d\r\n", tx_priority);
    
    // Create ThreadX thread
    UINT status = tx_thread_create(tx_thread, (CHAR *)name, thread_entry_wrapper,
                                   (ULONG)wrapper, stack, stack_size,
                                   tx_priority, tx_priority, TX_NO_TIME_SLICE,
                                   TX_AUTO_START);
    
    if (status != TX_SUCCESS) {
        // Thread creation failed - log and cleanup
        printf("[THREAD] ERROR: tx_thread_create failed with status=%d for thread '%s'\r\n",
               status, name ? name : "?");
        free(wrapper);
        free(stack);
        free(tx_thread);
        return -1;
    }
    
    printf("[THREAD] SUCCESS: Thread '%s' created with handle %p\r\n", 
           name ? name : "unnamed", (void *)tx_thread);
    
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

int os_mutex_destroy_threadx(os_mutex_t *mutex)
{
    UINT status = tx_mutex_delete(&mutex->mutex);
    return (status == TX_SUCCESS) ? 0 : -1;
}

int os_recursive_mutex_init_threadx(os_mutex_t *mutex)
{
    // ThreadX mutexes are already recursive by default
    return os_mutex_init_threadx(mutex);
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

int os_sem_destroy_threadx(os_sem_t *sem)
{
    UINT status = tx_semaphore_delete(&sem->sem);
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

TX_QUEUE* os_mq_create_threadx(size_t length, size_t item_size)
{
    TX_QUEUE *queue = (TX_QUEUE *)malloc(sizeof(TX_QUEUE));
    if (queue == NULL)
    {
        return NULL;
    }
    
    void *queue_memory = malloc(length * item_size * sizeof(ULONG));
    if (queue_memory == NULL)
    {
        free(queue);
        return NULL;
    }
    
    UINT status = tx_queue_create(queue, "openmrn_queue", item_size / sizeof(ULONG), 
                                   queue_memory, length * item_size * sizeof(ULONG));
    if (status != TX_SUCCESS)
    {
        free(queue_memory);
        free(queue);
        return NULL;
    }
    
    return queue;
}

#endif // USING_THREADX
