/* OpenMRNLite Server Application Entry Point */

#include "OpenMRNLite_server.h"
#include "main.h"
#include "stm32h5xx_nucleo.h"

#include <OpenMRNLite.h>

/// This is the OpenLCB Node ID. It must be coming from the Node ID range
/// assigned to the developer (get a range assigned to you via openlcb.org).
//static constexpr uint64_t NODE_ID = UINT64_C(0x050101011825);

//OpenMRN openmrn(NODE_ID);

extern "C" {

void OpenMRNLite_server_Entry(ULONG thread_input)
{
    // Initialize OpenMRNLite here
    
    while (1)
    {
        // Main OpenMRNLite processing loop
        BSP_LED_Toggle(LED_RED);
        tx_thread_sleep(200);
    }
}

} // extern "C"
