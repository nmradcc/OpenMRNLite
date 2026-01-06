# RTOS Support in OpenMRNLite

OpenMRNLite now supports multiple RTOS options for embedded targets:

## Supported RTOS Options

1. **FreeRTOS** - The most widely used embedded RTOS
2. **ThreadX** - Azure RTOS ThreadX
3. **CMSIS-RTOS v2** - ARM CMSIS-RTOS v2 API
4. **None** - No OS (single-threaded)

## Selecting an RTOS

When configuring your CMake build, specify the RTOS using the `-DOPENMRN_RTOS` option:

### FreeRTOS
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DOPENMRN_RTOS=FreeRTOS
```

### ThreadX
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DOPENMRN_RTOS=ThreadX
```

### CMSIS-RTOS v2
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake -DOPENMRN_RTOS=CMSIS_RTOS_V2
```

### None
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake

```


## RTOS-Specific Requirements

### FreeRTOS
- Include FreeRTOS header paths in your toolchain file
- Link against FreeRTOS library
- Ensure FreeRTOS kernel is available

Example toolchain additions:
```cmake
# FreeRTOS paths
include_directories(/path/to/FreeRTOS/Source/include)
include_directories(/path/to/FreeRTOS/Source/portable/GCC/ARM_CM4F)
```

### ThreadX
- Include ThreadX header paths in your toolchain file
- Link against ThreadX library (tx.a or equivalent)
- Ensure ThreadX kernel is initialized in your main()

Example toolchain additions:
```cmake
# ThreadX paths
include_directories(/path/to/threadx/common/inc)
include_directories(/path/to/threadx/ports/cortex_m4/gnu/inc)
link_libraries(tx)
```

ThreadX initialization in your application:
```c
#include "tx_api.h"

int main(void)
{
    // Initialize hardware
    hw_init();
    
    // Enter ThreadX kernel
    tx_kernel_enter();
    
    return 0; // Should never reach here
}
```

### CMSIS-RTOS v2
- Include CMSIS-RTOS v2 header paths
- Link against CMSIS-RTOS v2 implementation (often FreeRTOS-based)
- Ensure RTOS kernel is initialized

Example toolchain additions:
```cmake
# CMSIS-RTOS v2 paths
include_directories(/path/to/CMSIS_5/CMSIS/RTOS2/Include)
include_directories(/path/to/CMSIS_5/CMSIS/RTOS2/FreeRTOS/Include)
```

## RTOS Abstraction Layer

The OS abstraction layer (`src/os/`) provides a unified API that works across all supported RTOS options:

### Thread Management
- `os_thread_create()` - Create a new thread
- `os_thread_self()` - Get current thread handle
- Thread priorities and stack sizes are automatically converted

### Synchronization Primitives
- **Mutexes**: `os_mutex_init()`, `os_mutex_lock()`, `os_mutex_unlock()`
- **Semaphores**: `os_sem_init()`, `os_sem_wait()`, `os_sem_post()`, `os_sem_timedwait()`
- **Message Queues**: `os_mq_create()`, `os_mq_send()`, `os_mq_receive()`

### Timing Functions
- `os_thread_sleep()` - Sleep for specified nanoseconds
- `NSEC_TO_TICK()` - Convert nanoseconds to RTOS ticks

## Implementation Notes

### Priority Mapping
Each RTOS has different priority schemes. OpenMRNLite automatically converts:

- **FreeRTOS**: 0 (lowest) to configMAX_PRIORITIES-1 (highest)
- **ThreadX**: 0 (highest) to 31 (lowest) - inverted mapping applied
- **CMSIS-RTOS v2**: osPriorityLow to osPriorityRealtime - mapped based on value ranges

### Tick Rate Configuration
Each RTOS may have different tick rates. The `NSEC_TO_TICK()` macro handles conversions:

- **FreeRTOS**: Based on `NSEC_TO_TICK_SHIFT` (typically 1ms ticks)
- **ThreadX**: Based on `TX_TIMER_TICKS_PER_SECOND` (typically 100Hz)
- **CMSIS-RTOS v2**: Based on `osKernelGetTickFreq()` (runtime query)

### Memory Allocation
- ThreadX and CMSIS-RTOS v2 implementations use `malloc()` for thread control blocks
- Ensure sufficient heap is available for RTOS objects
- FreeRTOS typically uses static allocation or its own heap

## Porting Guidelines

If you're porting existing FreeRTOS code:

1. Use `os_*` functions instead of direct RTOS calls where possible
2. Test thoroughly as timing and priority behavior may differ between RTOS implementations

## Troubleshooting

### Build Errors
- **Missing RTOS headers**: Ensure your toolchain file includes correct header paths
- **Undefined references**: Link against the appropriate RTOS library
- **Wrong RTOS detected**: Explicitly set `-DOPENMRN_RTOS=<YourRTOS>`

### Runtime Issues
- **Thread creation failures**: Check available heap space and stack sizes
- **Priority inversions**: Verify priority mappings are appropriate for your application
- **Timing issues**: Adjust tick rate conversions if using non-standard RTOS configurations

## Performance Considerations

Different RTOS implementations have different characteristics:

- **FreeRTOS**: Lightweight, well-tested in OpenMRN ecosystem
- **ThreadX**: Industrial-grade, certified for safety-critical applications
- **CMSIS-RTOS v2**: Portable across ARM Cortex-M devices, often FreeRTOS-based underneath

Choose based on your project requirements, certification needs, and existing infrastructure.
