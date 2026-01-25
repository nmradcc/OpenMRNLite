# Migration Guide: ThreadX-Only Version

This document describes the changes made to create the ThreadX-exclusive version of OpenMRNLite and how to migrate existing code.

## Major Changes

### 1. Removed Multi-RTOS Support

**Before:**
- Supported FreeRTOS, ThreadX, and CMSIS-RTOS v2
- RTOS selection via CMake options
- Abstraction layer in `src/os/` with multiple implementations

**After:**
- ThreadX only
- No RTOS selection - ThreadX is always used
- Simplified OS layer with only `threadx_impl.c` and `threadx_impl.h`

### 2. Removed FreeRTOS Drivers

**Before:**
- Arduino-compatible API
- FreeRTOS-specific drivers in `src/freertos_drivers/`
- `CanBridge` and `SerialBridge` classes for Arduino

**After:**
- STM32 HAL + ThreadX native API
- `FdCanBridge` for direct STM32 FDCAN integration
- No Arduino dependencies

### 3. Simplified OpenMRN API

**Before:**
```cpp
OpenMRN openmrn(NODE_ID);
openmrn.begin();
openmrn.add_can_port(&canPort);  // Arduino-style CAN
openmrn.start_executor_thread();  // Default parameters
```

**After:**
```cpp
OpenMRN openmrn(NODE_ID);
setup_can_bridge(&openmrn, &hfdcan1, Error_Handler);  // STM32 HAL
openmrn.begin();
openmrn.start_executor_thread("OpenMRN", 16, 2048);  // ThreadX params
```

### 4. CMakeLists.txt Changes

**Before:**
```cmake
set(OPENMRN_RTOS "ThreadX" CACHE STRING "Select RTOS")
```

**After:**
```cmake
# ThreadX is the only supported RTOS
# No RTOS selection option
```

### 5. Header File Changes

**rtos_includes.h - Before:**
```cpp
#if defined(OPENMRN_FEATURE_RTOS_FREERTOS)
    #include "FreeRTOS.h"
    // ...
#elif defined(OPENMRN_FEATURE_RTOS_THREADX)
    #include "tx_api.h"
    // ...
#elif defined(OPENMRN_FEATURE_RTOS_CMSIS_V2)
    // ...
#endif
```

**rtos_includes.h - After:**
```cpp
#define USING_THREADX 1
#include "tx_api.h"
```

## Migration Steps

### For Existing ThreadX Users

1. **Update CMakeLists.txt**
   - Remove `OPENMRN_RTOS` variable references
   - Library automatically configures for ThreadX

2. **Replace CAN Bridge**
   ```cpp
   // Old (if using custom bridge)
   openmrn.add_can_port(&myCanPort);
   
   // New
   #include "FdCanBridge.h"
   setup_can_bridge(&openmrn, &hfdcan1, Error_Handler);
   ```

3. **Update Thread Creation**
   ```cpp
   // Old
   openmrn.start_executor_thread();
   
   // New (specify ThreadX parameters)
   openmrn.start_executor_thread("OpenMRN", 16, 2048);
   ```

### For FreeRTOS/CMSIS Users

**This version does not support FreeRTOS or CMSIS-RTOS.** If you need those RTOSes, use the original multi-RTOS version of OpenMRNLite.

## API Reference Changes

### OpenMRN Class

| Method | Before | After |
|--------|--------|-------|
| Constructor | `OpenMRN()` or `OpenMRN(node_id)` | `OpenMRN(node_id)` only |
| Start executor | `start_executor_thread()` | `start_executor_thread(name, priority, stack)` |
| Add CAN port | `add_can_port(Can*)` | Use `setup_can_bridge()` from FdCanBridge.h |
| Add serial | `add_gridconnect_port(&Serial)` | Not supported (STM32 HAL only) |

### Removed Classes

- `SerialBridge<T>` - Arduino serial support removed
- `CanBridge` - Replaced by `FdCanBridge`
- All Arduino GPIO wrappers

### New Components

- `FdCanBridge` - STM32 HAL FDCAN bridge
  - `setup_can_bridge(openmrn, hfdcan, error_handler)`
  - Handles RX/TX via HAL interrupts
  - Automatic registration with CAN hub

## Example Project Structure

```
MyProject/
├── CMakeLists.txt           # Links OpenMRNLite library
├── main.c                   # STM32 HAL initialization
├── app_threadx.c            # ThreadX initialization
└── openmrn_node.cpp         # OpenMRN stack setup
    ├── #include <OpenMRNLite.h>
    ├── #include "FdCanBridge.h"
    └── setup_can_bridge(&openmrn, &hfdcan1, Error_Handler)
```

## Benefits of ThreadX-Only Version

1. **Smaller Code Size** - No multi-RTOS abstraction overhead
2. **Simpler Debugging** - Direct ThreadX API calls, no indirection
3. **Better Integration** - Native ThreadX primitives throughout
4. **Clearer Code** - No `#ifdef` blocks for RTOS selection
5. **Easier Maintenance** - Single RTOS target

## Optional Modules (BLE/DCC)

See [MIGRATION_MODULES.md](MIGRATION_MODULES.md) for information about the BLE and DCC optional modules.

## Questions?

See the example project at `examples/STM32H563_Nucleo_TX` for a complete working implementation.
