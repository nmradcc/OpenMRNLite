/* CMSIS-RTOS v2 time and sleep shims for OpenMRNLite */
#include "cmsis_os2.h"
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

long long os_get_time_monotonic(void)
{
    uint32_t tick_hz = osKernelGetTickFreq();
    uint32_t ticks = osKernelGetTickCount();
    return ((long long)ticks * 1000000000LL) / tick_hz;
}

int usleep(unsigned int usec)
{
    // Convert microseconds to ticks via milliseconds
    unsigned int ms = (usec + 999U) / 1000U;
    if (ms == 0) ms = 1U;
    osDelay(ms);
    return 0;
}

#ifdef __cplusplus
}
#endif
