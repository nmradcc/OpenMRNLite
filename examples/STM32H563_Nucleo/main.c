#include <stdio.h>
#include "OpenMRNLite.h"

// Example: Minimal OpenMRN app for STM32H563 Nucleo
// You must provide board/RTOS/HAL initialization in your project

int main(void)
{
    // Initialize hardware (HAL, clocks, peripherals, etc.)
    // HAL_Init();
    // SystemClock_Config();
    // MX_GPIO_Init();
    // MX_CAN_Init();
    // ...

    // Initialize OpenMRN stack
    openmrn_init();

    // Main loop
    while (1)
    {
        // Application code here
        // openmrn_poll();
    }
}
