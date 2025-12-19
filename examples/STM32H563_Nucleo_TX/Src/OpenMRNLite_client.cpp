/* OpenMRNLite Client Application Entry Point */

#include "OpenMRNLite_client.h"
#include "main.h"
#include "app_azure_rtos.h"

#include <OpenMRNLite.h>

/// This is the OpenLCB Node ID. It must be coming from the Node ID range
/// assigned to the developer (get a range assigned to you via openlcb.org).
static constexpr uint64_t NODE_ID = UINT64_C(0x050101011824);

//OpenMRN openmrn(NODE_ID);

namespace openlcb
{
/// These definitions tell how the Node will appear on the OpenLCB bus for a
/// network browser.
extern const SimpleNodeStaticValues SNIP_STATIC_DATA = {
    4,
    "OpenMRN",
    "CAN-Node STM32H563_NUCLEO",
    "STM32H563 NUCLEO",
    "1.00"
};

/// SNIP dynamic filename (not used in this minimal example)
extern const char *const SNIP_DYNAMIC_FILENAME = nullptr;
} // namespace openlcb

extern "C" {

// buffer_malloc - allocates memory from ThreadX byte pool
void *buffer_malloc(size_t size)
{
    VOID *memory_ptr = NULL;
    TX_BYTE_POOL *pool = get_app_byte_pool();
    
    if (pool == NULL)
    {
        return NULL;
    }
    
    UINT status = tx_byte_allocate(pool, &memory_ptr, size, TX_NO_WAIT);
    
    if (status != TX_SUCCESS)
    {
        return NULL;
    }
    
    return memory_ptr;
}

void OpenMRNLite_client_Entry(ULONG thread_input)
{
    // Initialize OpenMRNLite here
    OpenMRN openmrn(NODE_ID);    
    while (1)
    {
        openmrn.loop();
        // Main OpenMRNLite processing loop
        BSP_LED_Toggle(LED_YELLOW);
        tx_thread_sleep(200);
    }
}

} // extern "C"
