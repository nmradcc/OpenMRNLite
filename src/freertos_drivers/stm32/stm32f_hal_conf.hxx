#ifndef _OPENMRN_ARDUINO_STM32F_HAL_CONF_HXX
#define _OPENMRN_ARDUINO_STM32F_HAL_CONF_HXX

// Include the board-specific STM32 HAL configuration
// This file should be configured for your specific STM32 variant
#if defined(STM32H563xx) || defined(STM32H5xx)
#include <stm32h5xx_hal_conf.h>
#else
// Fallback - comment this out if you're getting "stm32yyxx_hal_conf.h not found"
// and uncomment the appropriate include above for your STM32 variant
// #include <stm32yyxx_hal_conf.h>
#endif

static inline void SetInterruptPriority(uint32_t irq, uint8_t priority)
{
    NVIC_SetPriority((IRQn_Type)irq, priority >> (8U - __NVIC_PRIO_BITS));
}

#ifndef configKERNEL_INTERRUPT_PRIORITY
#define configKERNEL_INTERRUPT_PRIORITY 0xA0
#endif

#endif // _OPENMRN_ARDUINO_STM32F_HAL_CONF_HXX
