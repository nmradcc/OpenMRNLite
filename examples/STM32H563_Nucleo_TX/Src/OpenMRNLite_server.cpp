/* OpenMRNLite Server Application Entry Point */

#include "OpenMRNLite_server.h"
#include "main.h"
#include "app_azure_rtos.h"
#include "FdCanBridge.h"

#include <OpenMRNLite.h>

#include "openlcb/BroadcastTimeServer.hxx"

/// This is the OpenLCB Node ID. It must be coming from the Node ID range
/// assigned to the developer (get a range assigned to you via openlcb.org).
static constexpr uint64_t NODE_ID = UINT64_C(0x05010101181a);

static OpenMRN openmrn(NODE_ID);

namespace openlcb
{
/// These definitions tell how the Node will appear on the OpenLCB bus for a
/// network browser.
extern const SimpleNodeStaticValues SNIP_STATIC_DATA = {
    4,
    "NMRA",
    "CAN-Node STM32H563_NUCLEO server",
    "STM32H563 NUCLEO",
    "1.00"
};

/// SNIP dynamic filename (not used in this minimal example)
extern const char *const SNIP_DYNAMIC_FILENAME = nullptr;
} // namespace openlcb

extern "C" {

void OpenMRNLite_server_Entry(ULONG thread_input)
{
    // Initialize CAN bridge to connect HAL to OpenMRN
    setup_can_bridge(&openmrn);
    
    // Main time protocol server.
    openlcb::BroadcastTimeServer
        timeServer(openmrn.stack()->node(), openlcb::BroadcastTimeDefs::DEFAULT_FAST_CLOCK_ID);

    timeServer.set_time(0, 0);
    timeServer.set_date(1, 1);
    timeServer.set_year(1970);
    timeServer.set_rate_quarters(4);
    timeServer.start();

    // Start the OpenMRN stack
    openmrn.begin();
    openmrn.stack()->print_all_packets();
    openmrn.stack()->start_executor_thread("OpenMRN", 16, 2048);

    // Test: Send a CAN frame through the hub to verify TX path
    tx_thread_sleep(500); // Give stack more time to initialize
    test_send_can_frame(&openmrn);

    // Main processing loop
    while (1)
    {
        // Main OpenMRNLite processing loop
        openmrn.loop();
        BSP_LED_Toggle(LED_RED);
        tx_thread_sleep(20); // 20ms at 1000 Hz tick rate
    }
}

} // extern "C"
