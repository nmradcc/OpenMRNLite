/** \copyright
 * Copyright (c) 2025
 * All rights reserved.
 *
 * \file freertos_impl.h
 * FreeRTOS implementation for OpenMRN OS abstraction layer.
 *
 * @date 20 December 2025
 */

#ifndef _OS_FREERTOS_IMPL_H_
#define _OS_FREERTOS_IMPL_H_

#ifdef __FreeRTOS__

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

// FreeRTOS type mappings
typedef xTaskHandle os_thread_t;

typedef struct {
    xSemaphoreHandle sem;
    char recursive;
} os_mutex_t;

typedef xQueueHandle os_mq_t;

typedef struct {
    unsigned char state;
} os_thread_once_t;

typedef xSemaphoreHandle os_sem_t;

// FreeRTOS implementation functions
int os_thread_create_freertos(os_thread_t *thread, const char *name, int priority,
                              size_t stack_size, void *(*entry)(void *), void *arg);

int os_mutex_init_freertos(os_mutex_t *mutex);
int os_recursive_mutex_init_freertos(os_mutex_t *mutex);
int os_mutex_destroy_freertos(os_mutex_t *mutex);
int os_mutex_lock_freertos(os_mutex_t *mutex);
int os_mutex_unlock_freertos(os_mutex_t *mutex);

int os_sem_init_freertos(os_sem_t *sem, unsigned int value);
int os_sem_destroy_freertos(os_sem_t *sem);
int os_sem_wait_freertos(os_sem_t *sem);
int os_sem_post_freertos(os_sem_t *sem);
int os_sem_timedwait_freertos(os_sem_t *sem, long long timeout_nsec);

#if OPENMRN_FEATURE_RTOS_FROM_ISR
int os_sem_post_from_isr_freertos(os_sem_t *sem, int *woken);
#endif

os_thread_t os_thread_self_freertos(void);
int os_thread_get_priority_freertos(os_thread_t thread);
int os_thread_get_priority_min_freertos(void);
int os_thread_get_priority_max_freertos(void);

os_mq_t os_mq_create_freertos(size_t length, size_t item_size);
void os_mq_send_freertos(os_mq_t queue, const void *data);
int os_mq_timedsend_freertos(os_mq_t queue, const void *data, long long timeout);
void os_mq_receive_freertos(os_mq_t queue, void *data);
int os_mq_timedreceive_freertos(os_mq_t queue, void *data, long long timeout);
int os_mq_send_from_isr_freertos(os_mq_t queue, const void *data, int *woken);
int os_mq_is_full_from_isr_freertos(os_mq_t queue);
int os_mq_receive_from_isr_freertos(os_mq_t queue, void *data, int *woken);
int os_mq_num_pending_freertos(os_mq_t queue);
int os_mq_num_pending_from_isr_freertos(os_mq_t queue);
int os_mq_num_spaces_freertos(os_mq_t queue);

#ifdef __cplusplus
}
#endif

#endif // __FreeRTOS__

#endif // _OS_FREERTOS_IMPL_H_
