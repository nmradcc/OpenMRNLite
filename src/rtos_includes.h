/** \copyright
 * Copyright (c) 2025
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are  permitted provided that the following conditions are met:
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
 * \file rtos_includes.h
 * This file simplifies the include path for RTOS header files and provides
 * unified abstractions for FreeRTOS, ThreadX, and CMSIS-RTOS v2.
 *
 * @date 17 December 2025
 */

#ifndef _RTOS_INCLUDES_H_
#define _RTOS_INCLUDES_H_

#include "openmrn_features.h"

// Detect which RTOS is being used
#if defined(OPENMRN_FEATURE_RTOS_FREERTOS)
    #define USING_FREERTOS 1
    #include "FreeRTOS.h"
    #include "task.h"
    #include "semphr.h"
    #include "queue.h"
    // Convert nanoseconds to FreeRTOS ticks
    #define NSEC_TO_TICK(ns) ((TickType_t)(((ns) * (uint64_t)configTICK_RATE_HZ) / 1000000000ULL))

#elif defined(OPENMRN_FEATURE_RTOS_THREADX)
    #define USING_THREADX 1
    #include "tx_api.h"
    // ThreadX tick conversion (assuming 1000Hz = 1ms tick rate)
    #define NSEC_TO_TICK(ns) (((ns) * TX_TIMER_TICKS_PER_SECOND) / 1000000000ULL)

#elif defined(OPENMRN_FEATURE_RTOS_CMSIS_V2)
    #define USING_CMSIS_RTOS_V2 1
    #include "cmsis_os2.h"
    // CMSIS-RTOS v2 tick conversion
    #define NSEC_TO_TICK(ns) (((ns) * osKernelGetTickFreq()) / 1000000000ULL)

#else
    #error "No RTOS selected. Define OPENMRN_FEATURE_RTOS_FREERTOS, OPENMRN_FEATURE_RTOS_THREADX, or OPENMRN_FEATURE_RTOS_CMSIS_V2"
#endif

#endif // _RTOS_INCLUDES_H_
