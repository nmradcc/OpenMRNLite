/* FreeRTOS time and sleep shims for OpenMRNLite */
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

long long os_get_time_monotonic(void)
{
#if (configUSE_16_BIT_TICKS == 1)
    TickType_t ticks = xTaskGetTickCount();
#else
    TickType_t ticks = xTaskGetTickCount();
#endif
    // Convert ticks to nanoseconds: ns = ticks * (1e9 / tick_hz)
    return ((long long)ticks * 1000000000LL) / configTICK_RATE_HZ;
}

int usleep(unsigned int usec)
{
    // Round up to nearest millisecond
    unsigned int ms = (usec + 999U) / 1000U;
    if (ms == 0) ms = 1U;
    vTaskDelay(pdMS_TO_TICKS(ms));
    return 0;
}

#ifdef __cplusplus
}
#endif
