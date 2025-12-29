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

#ifdef USING_FREERTOS

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

// ============================================================================
// FreeRTOS-specific feature definitions
// ============================================================================

/// Compile os_sem_timedwait functions.
#define OPENMRN_FEATURE_SEM_TIMEDWAIT 1

/// Enables use of Notifiable::notify_from_isr and OSSem::post_from_isr.
#define OPENMRN_FEATURE_RTOS_FROM_ISR 1

/// Use FreeRTOS implementation for os_thread_create and keeping a list of live threads.
#define OPENMRN_FEATURE_THREAD_FREERTOS 1

/// Compiles the FreeRTOS event group based ::select() implementation.
#define OPENMRN_FEATURE_DEVICE_SELECT 0 // Disabled for now

/// Adds implementations for ::read ::write etc, with fd table.
#define OPENMRN_FEATURE_DEVTAB 0 // Disabled for now

/// Adds struct reent pointer to the FreeRTOS Task Priv structure and swaps it
/// in when the tasks are swapped in.
#define OPENMRN_FEATURE_REENT 1

/// Adds support for FD based CAN interfaces.
#define OPENMRN_FEATURE_FD_CAN_DEVICE 1

/// Compiles support for BSD sockets API.
#define OPENMRN_FEATURE_BSD_SOCKETS 1

/// Compiles support for calling getsockname when binding a socket to a port
/// when listening for incoming connections.
#define OPENMRN_HAVE_BSD_SOCKETS_GETSOCKNAME 1

/// Enables the code using ::fstat to confirm if the file handle is a socket.
#define OPENMRN_HAVE_SOCKET_FSTAT 1

#if defined(OPENMRN_FEATURE_DEVTAB)
/// Enables the code using ::open ::close ::read ::write for non-volatile
/// storage, FileMemorySpace for the configuration space, and
/// SNIP_DYNAMIC_FILE_NAME for node names.
#define OPENMRN_HAVE_POSIX_FD 1
#endif

#if defined(OPENMRN_FEATURE_DEVICE_SELECT)
#define OPENMRN_FEATURE_EXECUTOR_SELECT 1
#endif

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

// Static initializer macros
/** Static initializer for mutexes */
#define OS_MUTEX_INITIALIZER {NULL, 0}
/** Static initializer for recursive mutexes */
#define OS_RECURSIVE_MUTEX_INITIALIZER {NULL, 1}

// Inline function implementations
static inline os_thread_t os_thread_self(void)
{
    return os_thread_self_freertos();
}

static inline int os_thread_get_priority(os_thread_t thread)
{
    return os_thread_get_priority_freertos(thread);
}

static inline int os_thread_get_priority_min(void)
{
    return os_thread_get_priority_min_freertos();
}

static inline int os_thread_get_priority_max(void)
{
    return os_thread_get_priority_max_freertos();
}

static inline int os_mutex_init(os_mutex_t *mutex)
{
    return os_mutex_init_freertos(mutex);
}

static inline int os_recursive_mutex_init(os_mutex_t *mutex)
{
    return os_recursive_mutex_init_freertos(mutex);
}

static inline int os_mutex_destroy(os_mutex_t *mutex)
{
    return os_mutex_destroy_freertos(mutex);
}

static inline int os_mutex_lock(os_mutex_t *mutex)
{
    return os_mutex_lock_freertos(mutex);
}

static inline int os_mutex_unlock(os_mutex_t *mutex)
{
    return os_mutex_unlock_freertos(mutex);
}

static inline int os_sem_init(os_sem_t *sem, unsigned int value)
{
    return os_sem_init_freertos(sem, value);
}

static inline int os_sem_destroy(os_sem_t *sem)
{
    return os_sem_destroy_freertos(sem);
}

static inline int os_sem_post(os_sem_t *sem)
{
    return os_sem_post_freertos(sem);
}

#if OPENMRN_FEATURE_RTOS_FROM_ISR
static inline int os_sem_post_from_isr(os_sem_t *sem, int *woken)
{
    return os_sem_post_from_isr_freertos(sem, woken);
}
#endif

static inline int os_sem_wait(os_sem_t *sem)
{
    return os_sem_wait_freertos(sem);
}

#if OPENMRN_FEATURE_SEM_TIMEDWAIT
static inline int os_sem_timedwait(os_sem_t *sem, long long timeout)
{
    if (timeout == OPENMRN_OS_WAIT_FOREVER)
    {
        return os_sem_wait(sem);
    }
    return os_sem_timedwait_freertos(sem, timeout);
}
#endif

static inline os_mq_t os_mq_create(size_t length, size_t item_size)
{
    return os_mq_create_freertos(length, item_size);
}

static inline void os_mq_send(os_mq_t queue, const void *data)
{
    os_mq_send_freertos(queue, data);
}

static inline int os_mq_timedsend(os_mq_t queue, const void *data, long long timeout)
{
    return os_mq_timedsend_freertos(queue, data, timeout);
}

static inline void os_mq_receive(os_mq_t queue, void *data)
{
    os_mq_receive_freertos(queue, data);
}

static inline int os_mq_timedreceive(os_mq_t queue, void *data, long long timeout)
{
    return os_mq_timedreceive_freertos(queue, data, timeout);
}

static inline int os_mq_send_from_isr(os_mq_t queue, const void *data, int *woken)
{
    return os_mq_send_from_isr_freertos(queue, data, woken);
}

static inline int os_mq_is_full_from_isr(os_mq_t queue)
{
    return os_mq_is_full_from_isr_freertos(queue);
}

static inline int os_mq_receive_from_isr(os_mq_t queue, void *data, int *woken)
{
    return os_mq_receive_from_isr_freertos(queue, data, woken);
}

static inline int os_mq_num_pending(os_mq_t queue)
{
    return os_mq_num_pending_freertos(queue);
}

static inline int os_mq_num_pending_from_isr(os_mq_t queue)
{
    return os_mq_num_pending_from_isr_freertos(queue);
}

static inline int os_mq_num_spaces(os_mq_t queue)
{
    return os_mq_num_spaces_freertos(queue);
}

#ifdef __cplusplus
}
#endif

#endif // __FreeRTOS__

#endif // _OS_FREERTOS_IMPL_H_
