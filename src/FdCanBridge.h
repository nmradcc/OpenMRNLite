/** \copyright
 * Copyright (c) 2025
 * All rights reserved.
 *
 * \file FdCanBridge.h
 * Bridge between STM32 HAL FDCAN driver and OpenMRNLite CAN hub.
 *
 * @date January 2026
 */

#ifndef _FDCAN_BRIDGE_H_
#define _FDCAN_BRIDGE_H_

#include <OpenMRNLite.h>

// Forward declarations
typedef struct __FDCAN_HandleTypeDef FDCAN_HandleTypeDef;

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration - FdCanBridge is only used in C++ context
class FdCanBridge;

/// Setup function to initialize the CAN bridge
/// @param openmrn OpenMRN stack instance
/// @param hfdcan STM32 HAL FDCAN handle
/// @param error_handler Error handler function (e.g., Error_Handler from main.c)
void setup_can_bridge(openmrn_arduino::OpenMRN *openmrn, FDCAN_HandleTypeDef *hfdcan, void (*error_handler)());

/// Get the bridge instance for direct access if needed
FdCanBridge* get_can_bridge();

/// Test function to send a frame through the OpenMRN hub
void test_send_can_frame(openmrn_arduino::OpenMRN *openmrn);

#ifdef __cplusplus
}
#endif

#endif // _FDCAN_BRIDGE_H_
