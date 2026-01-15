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

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration - FdCanBridge is only used in C++ context
class FdCanBridge;

/// Setup function to initialize the CAN bridge
/// Call this before starting the OpenMRN stack
void setup_can_bridge(openmrn_arduino::OpenMRN *openmrn);

/// Get the bridge instance for direct access if needed
FdCanBridge* get_can_bridge();

#ifdef __cplusplus
}
#endif

#endif // _FDCAN_BRIDGE_H_
