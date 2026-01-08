/** \copyright
 * Copyright (c) 2025
 * All rights reserved.
 *
 * \file freertos_impl.c
 * FreeRTOS implementation for OpenMRN OS abstraction layer.
 *
 * @date 20 December 2025
 */

#if defined(OPENMRN_FEATURE_RTOS_FREERTOS) || defined(USING_FREERTOS) || defined(__FreeRTOS__)

#include "os/os.h"
#include "rtos_includes.h"
#include <stdlib.h>

/** Captures point of death (line). */
int g_death_lineno;
/** Captures point of death (file). */
const char* g_death_file;

// Thread entry wrapper structure
typedef struct {
    void *(*user_entry)(void *);
    void *user_arg;
} thread_wrapper_t;

// Thread entry wrapper function
static void thread_entry_wrapper(void *arg)
{
    thread_wrapper_t *wrapper = (thread_wrapper_t *)arg;
    void *(*entry)(void *) = wrapper->user_entry;
    void *user_arg = wrapper->user_arg;
    
    // Free wrapper before calling user entry
    free(wrapper);
    
    // Call user entry point
    entry(user_arg);
    
    // Thread completed, delete self
    vTaskDelete(NULL);
}

int os_thread_create_freertos(os_thread_t *thread, const char *name, int priority,
                              size_t stack_size, void *(*entry)(void *), void *arg)
{
    // Create wrapper for entry point
    thread_wrapper_t *wrapper = (thread_wrapper_t *)malloc(sizeof(thread_wrapper_t));
    if (!wrapper) {
        return -1;
    }
    wrapper->user_entry = entry;
    wrapper->user_arg = arg;
    
    // Convert priority (FreeRTOS uses higher number = higher priority)
    UBaseType_t freertos_priority = (priority <= 0) ? (configMAX_PRIORITIES / 2) : priority;
    if (freertos_priority >= configMAX_PRIORITIES) {
        freertos_priority = configMAX_PRIORITIES - 1;
    }
    
    // Create FreeRTOS task
    BaseType_t result = xTaskCreate(thread_entry_wrapper, name, 
                                    stack_size / sizeof(StackType_t),
                                    wrapper, freertos_priority, thread);
    
    if (result != pdPASS) {
        free(wrapper);
        return -1;
    }
    
    return 0;
}

int os_mutex_init_freertos(os_mutex_t *mutex)
{
    mutex->recursive = 0;
    mutex->sem = xSemaphoreCreateMutex();
    return 0;
}

int os_recursive_mutex_init_freertos(os_mutex_t *mutex)
{
    mutex->recursive = 1;
    mutex->sem = xSemaphoreCreateRecursiveMutex();
    return 0;
}

int os_mutex_destroy_freertos(os_mutex_t *mutex)
{
    vSemaphoreDelete(mutex->sem);
    return 0;
}

int os_mutex_lock_freertos(os_mutex_t *mutex)
{
    if (mutex->sem == NULL)
    {
        int task_scheduler_running =
            (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING);
        if (task_scheduler_running)
        {
            vTaskSuspendAll();
        }
        if (mutex->sem == NULL)
        {
            if (mutex->recursive)
            {
                mutex->sem = xSemaphoreCreateRecursiveMutex();
            }
            else
            {
                mutex->sem = xSemaphoreCreateMutex();
            }
        }
        if (task_scheduler_running)
        {
            xTaskResumeAll();
        }
    }

    if (mutex->recursive)
    {
        xSemaphoreTakeRecursive(mutex->sem, portMAX_DELAY);
    }
    else
    {
        xSemaphoreTake(mutex->sem, portMAX_DELAY);
    }
    return 0;
}

int os_mutex_unlock_freertos(os_mutex_t *mutex)
{
    if (mutex->recursive)
    {
        xSemaphoreGiveRecursive(mutex->sem);
    }
    else
    {
        xSemaphoreGive(mutex->sem);
    }
    return 0;
}

int os_sem_init_freertos(os_sem_t *sem, unsigned int value)
{
    *sem = xSemaphoreCreateCounting(LONG_MAX, value);
    if (!*sem) {
        abort();
    }
    return 0;
}

int os_sem_destroy_freertos(os_sem_t *sem)
{
    vSemaphoreDelete(*sem);
    return 0;
}

int os_sem_post_freertos(os_sem_t *sem)
{
    xSemaphoreGive(*sem);
    return 0;
}

#if OPENMRN_FEATURE_RTOS_FROM_ISR
int os_sem_post_from_isr_freertos(os_sem_t *sem, int *woken)
{
    portBASE_TYPE local_woken = 0;
    xSemaphoreGiveFromISR(*sem, &local_woken);
    *woken |= local_woken;
    return 0;
}
#endif

int os_sem_wait_freertos(os_sem_t *sem)
{
    xSemaphoreTake(*sem, portMAX_DELAY);
    return 0;
}

int os_sem_timedwait_freertos(os_sem_t *sem, long long timeout_nsec)
{
    if (xSemaphoreTake(*sem, NSEC_TO_TICK(timeout_nsec)) == pdTRUE)
    {
        return 0;
    }
    else
    {
        errno = ETIMEDOUT;
        return -1;
    }
}

os_thread_t os_thread_self_freertos(void)
{
    return xTaskGetCurrentTaskHandle();
}

int os_thread_get_priority_freertos(os_thread_t thread)
{
    return uxTaskPriorityGet(thread);
}

int os_thread_get_priority_min_freertos(void)
{
    return 1;
}

int os_thread_get_priority_max_freertos(void)
{
    return configMAX_PRIORITIES - 1;
}

os_mq_t os_mq_create_freertos(size_t length, size_t item_size)
{
    return xQueueCreate(length, item_size);
}

void os_mq_send_freertos(os_mq_t queue, const void *data)
{
    xQueueSend(queue, data, portMAX_DELAY);
}

int os_mq_timedsend_freertos(os_mq_t queue, const void *data, long long timeout)
{
    TickType_t ticks = NSEC_TO_TICK(timeout);
    
    if (xQueueSend(queue, data, ticks) != pdTRUE)
    {
        return OS_MQ_TIMEDOUT;
    }
    return OS_MQ_NONE;
}

void os_mq_receive_freertos(os_mq_t queue, void *data)
{
    xQueueReceive(queue, data, portMAX_DELAY);
}

int os_mq_timedreceive_freertos(os_mq_t queue, void *data, long long timeout)
{
    TickType_t ticks = NSEC_TO_TICK(timeout);

    if (xQueueReceive(queue, data, ticks) != pdTRUE)
    {
        return OS_MQ_TIMEDOUT;
    }
    return OS_MQ_NONE;
}

int os_mq_send_from_isr_freertos(os_mq_t queue, const void *data, int *woken)
{
    BaseType_t local_woken = pdFALSE;
    if (xQueueSendFromISR(queue, data, &local_woken) != pdTRUE)
    {
        return OS_MQ_FULL;
    }
    *woken |= local_woken;
    return OS_MQ_NONE;
}

int os_mq_is_full_from_isr_freertos(os_mq_t queue)
{
    return xQueueIsQueueFullFromISR(queue);
}

int os_mq_receive_from_isr_freertos(os_mq_t queue, void *data, int *woken)
{
    BaseType_t local_woken = pdFALSE;
    if (xQueueReceiveFromISR(queue, data, &local_woken) != pdTRUE)
    {
        return OS_MQ_EMPTY;
    }
    *woken |= local_woken;
    return OS_MQ_NONE;
}

int os_mq_num_pending_freertos(os_mq_t queue)
{
    return uxQueueMessagesWaiting(queue);
}

int os_mq_num_pending_from_isr_freertos(os_mq_t queue)
{
    return uxQueueMessagesWaitingFromISR(queue);
}

int os_mq_num_spaces_freertos(os_mq_t queue)
{
    return uxQueueSpacesAvailable(queue);
}

#endif // __FreeRTOS__
