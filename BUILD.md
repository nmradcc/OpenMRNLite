# Building OpenMRNLite with CMake

This document describes how to build OpenMRNLite using CMake for embedded targets.

## Requirements

- CMake 3.12 or higher
- Cross-compilation toolchain for your target (ARM GCC, AVR GCC, etc.)
- C++14 compatible compiler
- C11 compatible compiler
- Toolchain file configured for your embedded target

## Important Note

**OpenMRNLite only supports cross-compilation for embedded targets.** You must use a toolchain file with `CMAKE_SYSTEM_NAME=Generic`.

## Quick Start

### Building for ARM Cortex-M (Example)

```bash
# Create build directory
mkdir build && cd build

# Configure with toolchain file (REQUIRED)
cmake .. -DCMAKE_TOOLCHAIN_FILE=../toolchain-arm.cmake

# Build
cmake --build .

# Install (optional)
cmake --install . --prefix=/path/to/install
```

## Build Options

You can customize the build using the following options:

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake  # REQUIRED: Specify toolchain
cmake .. -DOPENMRN_RTOS=FreeRTOS                 # Select RTOS: FreeRTOS (default), ThreadX, CMSIS_RTOS_V2, or None
cmake .. -DBUILD_EXAMPLES=ON                     # Build example applications (default: OFF)
cmake .. -DBUILD_SHARED_LIBS=ON                  # Build shared libraries (default: OFF)
cmake .. -DCMAKE_BUILD_TYPE=Debug                # Set build type: Debug, Release, etc.
```

## RTOS Selection

OpenMRNLite supports multiple RTOS options:
- **FreeRTOS** (default) - Most widely used
- **ThreadX** - Azure RTOS
- **CMSIS-RTOS v2** - ARM standard API
- **None** - Bare-metal/single-threaded operation (no RTOS)

See [RTOS_SUPPORT.md](RTOS_SUPPORT.md) for detailed information about RTOS selection and configuration.

## Creating a Toolchain File

You must create a toolchain file for your embedded target.

### Example toolchain file for ARM Cortex-M:
```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```

## Using OpenMRNLite in Your CMake Project

After installation, you can use OpenMRNLite in your project:

```cmake
find_package(OpenMRNLite REQUIRED)
target_link_libraries(your_target PRIVATE OpenMRNLite::OpenMRNLite)
```

Or without installation, add as subdirectory:

```cmake
add_subdirectory(path/to/OpenMRNLite)
target_link_libraries(your_target PRIVATE OpenMRNLite)
```

## Build Types

- **Debug**: No optimization, debug symbols included
- **Release**: Full optimization, no debug symbols
- **RelWithDebInfo**: Optimization with debug symbols
- **MinSizeRel**: Optimize for size

## Troubleshooting

### "Only supports embedded targets" error
This error occurs when you try to build without a toolchain file or with CMAKE_SYSTEM_NAME not set to "Generic". Make sure you're using a proper toolchain file:
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/toolchain.cmake
```

### Toolchain not found
Ensure your cross-compiler is installed and in your PATH:
```bash
# For ARM
arm-none-eabi-gcc --version

# For AVR
avr-gcc --version
```

### Compiler warnings
Some platform-specific code may generate warnings when compiling for different targets. This is expected and doesn't affect functionality for your target platform.
