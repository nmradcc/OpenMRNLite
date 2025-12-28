/** \copyright
 * Copyright (c) 2019, Balazs Racz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  - Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \file openmrn_features.h
 *
 * This file defines compilation-time configuration options for OpenMRN, which
 * are exclusively in the form of C-compatible macros. These control
 * conditional compilation on different operating systems.
 *
 * @author Balazs Racz
 * @date 24 February 2019
 */

#ifndef _INCLUDE_OPENMRN_FEATURES_
#define _INCLUDE_OPENMRN_FEATURES_

#ifdef ESP_PLATFORM
// Pull in ESP-IDF version macros when building as an ESP-IDF component.
#include <esp_idf_version.h>
#endif

#ifndef ESP_IDF_VERSION
#define ESP_IDF_VERSION 0
#endif

#ifndef ESP_IDF_VERSION_VAL
#define ESP_IDF_VERSION_VAL(a,b,c) 1
#endif

#if defined(__FreeRTOS__)
/// Compiles the FreeRTOS event group based ::select() implementation.
#define OPENMRN_FEATURE_DEVICE_SELECT 0 // Disabled for now
/// Adds implementations for ::read ::write etc, with fd table.
#define OPENMRN_FEATURE_DEVTAB 0 // Disabled for now
/// Adds struct reent pointer to the FreeRTOS Task Priv structure and swaps it
/// in when the tasks are swapped in.
#define OPENMRN_FEATURE_REENT 1
#endif

// ThreadX and CMSIS-RTOS v2 support is minimal for now
// TODO: Implement DEVTAB, REENT, and other features for ThreadX/CMSIS-RTOS v2

#if defined(__FreeRTOS__)
// Note: this is not using OPENMRN_FEATURE_DEVICE_SELECT due to other usages
// of that macro.
/// Adds support for FD based CAN interfaces.
#define OPENMRN_FEATURE_FD_CAN_DEVICE 1
#endif

#if defined(OPENMRN_FEATURE_DEVTAB)
/// Enables the code using ::open ::close ::read ::write for non-volatile
/// storage, FileMemorySpace for the configuration space, and
/// SNIP_DYNAMIC_FILE_NAME for node names.
#define OPENMRN_HAVE_POSIX_FD 1
#endif

/// Enables the code using ::fstat to confirm if the file handle is a socket.
#define OPENMRN_HAVE_SOCKET_FSTAT 1

#if !defined(__FreeRTOS__) && !defined(OPENMRN_FEATURE_RTOS_THREADX) && !defined(OPENMRN_FEATURE_RTOS_CMSIS_V2)
/// Uses ::pselect in the Executor for sleep and pkill for waking up.
#define OPENMRN_HAVE_PSELECT 1
#endif

#if defined(OPENMRN_HAVE_PSELECT) || defined(OPENMRN_FEATURE_DEVICE_SELECT)
#define OPENMRN_FEATURE_EXECUTOR_SELECT 1
#endif

#if defined(__FreeRTOS__)
/// Use os_mutex_... implementation based on FreeRTOS mutex and semaphores.
#define OPENMRN_FEATURE_MUTEX_FREERTOS 1

/// Enables use of Notifiable::notify_from_isr and OSSem::post_from_isr.
#define OPENMRN_FEATURE_RTOS_FROM_ISR 1
#elif defined(OPENMRN_FEATURE_RTOS_THREADX)
/// Use ThreadX native mutex and semaphore implementation.
#define OPENMRN_FEATURE_MUTEX_THREADX 1

/// Compile os_sem_timedwait functions.
#define OPENMRN_FEATURE_SEM_TIMEDWAIT 1

/// Enables use of Notifiable::notify_from_isr and OSSem::post_from_isr.
#define OPENMRN_FEATURE_RTOS_FROM_ISR 1
#elif defined(OPENMRN_FEATURE_RTOS_CMSIS_V2)
/// Use CMSIS-RTOS v2 native mutex and semaphore implementation.
#define OPENMRN_FEATURE_MUTEX_CMSIS_V2 1

/// Compile os_sem_timedwait functions.
#define OPENMRN_FEATURE_SEM_TIMEDWAIT 1

/// Enables use of Notifiable::notify_from_isr and OSSem::post_from_isr.
#define OPENMRN_FEATURE_RTOS_FROM_ISR 1
#elif OPENMRN_FEATURE_SINGLE_THREADED
/// Add a fake implementation for os_mutex_lock that crashes if there is a
/// conflict.
#define OPENMRN_FEATURE_MUTEX_FAKE 1
#else
/// Use pthread_mutex for os_mutex implementation.
#define OPENMRN_FEATURE_MUTEX_PTHREAD 1
#endif

#if OPENMRN_FEATURE_MUTEX_FREERTOS || OPENMRN_FEATURE_MUTEX_PTHREAD
/// Compile os_sem_timedwait functions.
#define OPENMRN_FEATURE_SEM_TIMEDWAIT 1
#endif

#if defined(__FreeRTOS__)
/// Use FreeRTOS implementation for os_thread_create and keeping a list of live
/// threads.
#define OPENMRN_FEATURE_THREAD_FREERTOS 1
#elif OPENMRN_FEATURE_SINGLE_THREADED
#else
/// Use pthread for os_thread_create.
#define OPENMRN_FEATURE_THREAD_PTHREAD 1
/// Use pthread_setname for setting the newly created thread's name.
#define OPENMRN_HAVE_PTHREAD_SETNAME 1
/// Use pthread_attr for setting the stack size of newly created threads.
#define OPENMRN_FEATURE_PTHREAD_SETSTACK 1
#endif

#if defined(__FreeRTOS__)
/// Compiles support for BSD sockets API.
#define OPENMRN_FEATURE_BSD_SOCKETS 1

/// Compiles support for calling getsockname when binding a socket to a port
/// when listening for incoming connections.
#define OPENMRN_HAVE_BSD_SOCKETS_GETSOCKNAME 1

#endif

/// Compiles support for calling reboot() in ConfigUpdateFlow.hxx and
/// MemoryConfig.cxx.
#define OPENMRN_FEATURE_REBOOT 1


#endif // _INCLUDE_OPENMRN_FEATURES_
