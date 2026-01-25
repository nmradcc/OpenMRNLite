# OpenMRNLite - ThreadX Edition

This library implements network protocols for model railroading, specifically the OpenLCB protocol suite (Open Layout Control Bus), which has been adopted by the NMRA and referenced as LCC (Layout Command Control).

**This is a simplified, ThreadX-exclusive version of OpenMRNLite** designed for STM32 microcontrollers running Azure ThreadX RTOS. The OS abstraction layer has been removed in favor of direct ThreadX primitives for improved simplicity and performance.

## Key Features

- **ThreadX-Only**: Simplified codebase with native ThreadX integration
- **FdCAN Bridge**: Direct STM32 HAL FDCAN driver integration for CAN communication
- **Simplified API**: No Arduino dependencies, pure STM32 HAL + ThreadX
- **OpenLCB/LCC Protocol**: Full implementation of the Layout Command Control protocol

## Documentation

- [Build Instructions](BUILD.md) - How to build the library and examples
- [Migration Guide](MIGRATION.md) - Migrating from multi-RTOS version
- [Optional Modules](OPTIONAL_MODULES.md) - How to enable/disable BLE and DCC modules

## Supported Platforms

- **STM32 with ThreadX RTOS** (primary target: STM32H5 series)
- STM32CubeMX-generated HAL projects
- Azure RTOS ThreadX

FreeRTOS and CMSIS-RTOS support has been removed. For those RTOSes, use the original OpenMRNLite.

## Quick Start

1. Add OpenMRNLite to your STM32 project's CMakeLists.txt
2. Initialize the CAN bridge using FdCanBridge
3. Create an OpenMRN instance with your node ID
4. Start the executor thread
5. Use the OpenMRN loop for processing

See `examples/STM32H563_Nucleo_TX` for a complete working example.
