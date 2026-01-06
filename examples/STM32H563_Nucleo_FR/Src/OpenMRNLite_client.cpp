/* OpenMRNLite Client Application Entry Point */

#include "OpenMRNLite_client.h"
#include "main.h"
//#include "tx_api.h"

#include <OpenMRNLite.h>

/// This is the OpenLCB Node ID. It must be coming from the Node ID range
/// assigned to the developer (get a range assigned to you via openlcb.org).
static constexpr uint64_t NODE_ID = UINT64_C(0x050101011824);

//OpenMRN openmrn(NODE_ID);


extern "C" {

void OpenMRNLite_client_Entry(unsigned long thread_input)
{
    // Initialize OpenMRNLite here
    
    while (1)
    {
        // Main OpenMRNLite processing loop
        BSP_LED_Toggle(LED_YELLOW);
        osDelay(200);
    }
}

} // extern "C"
