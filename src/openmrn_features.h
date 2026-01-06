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
 * This file defines global compilation-time configuration options for OpenMRN.
 * RTOS-specific features are defined in their respective impl.h files.
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

// ============================================================================
// Global Features (platform-independent)
// ============================================================================

/// Compiles support for calling reboot() in ConfigUpdateFlow.hxx and
/// MemoryConfig.cxx.
#define OPENMRN_FEATURE_REBOOT 1

/// When defined to 1, includes BLE (Bluetooth Low Energy) support.
/// Set via CMake option OPENMRN_ENABLE_BLE (default: ON).
#ifndef OPENMRN_FEATURE_BLE
#define OPENMRN_FEATURE_BLE 0
#endif

/// When defined to 1, includes DCC (Digital Command Control) support.
/// Set via CMake option OPENMRN_ENABLE_DCC (default: ON).
#ifndef OPENMRN_FEATURE_DCC
#define OPENMRN_FEATURE_DCC 0
#endif

// ============================================================================
// RTOS-specific features are defined in os/*_impl.h files:
// - freertos_impl.h: FreeRTOS features
// - threadx_impl.h: ThreadX features  
// - cmsis_rtos2_impl.h: CMSIS-RTOS v2 features
// ============================================================================

#if OPENMRN_FEATURE_SINGLE_THREADED
/// Add a fake implementation for os_mutex_lock that crashes if there is a
/// conflict.
#define OPENMRN_FEATURE_MUTEX_FAKE 1
#endif

#endif // _INCLUDE_OPENMRN_FEATURES_
