/* OpenMRNLite Server Application Entry Point */

#include "OpenMRNLite_server.h"
#include "main.h"
#include "app_azure_rtos.h"
#include "FdCanBridge.h"

#include <OpenMRNLite.h>

#include <cstdio>

#include "openlcb/BroadcastTimeServer.hxx"
#include "openlcb/BroadcastTimeClient.hxx"

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
    
    // Start the OpenMRN stack FIRST (must happen before creating BroadcastTimeServer)
    openmrn.begin();
    openmrn.stack()->print_all_packets();
    openmrn.stack()->start_executor_thread("OpenMRN", 16, 2048);
    
    // Give executor thread time to fully initialize before scheduling work
    tx_thread_sleep(200);
    
    // Main time protocol server (created AFTER executor is running and initialized)
    openlcb::BroadcastTimeServer
        timeServer(openmrn.stack()->node(), openlcb::BroadcastTimeDefs::DEFAULT_FAST_CLOCK_ID);

    printf("[MAIN] BroadcastTimeServer created\r\n");
    fflush(stdout);
    
    timeServer.set_time(0, 0);
    timeServer.set_date(1, 1);
    timeServer.set_year(1970);
    // Set clock rate to 80 quarters (20x speed) = 1 virtual minute every 3 real seconds
    timeServer.set_rate_quarters(80);
    
    printf("[MAIN] BroadcastTimeServer configured with rate=20x (80 quarters)\r\n");
    printf("[MAIN] Virtual time advances 1 minute every 3 real seconds\r\n");
    fflush(stdout);
    
    timeServer.start();
    
    printf("[MAIN] BroadcastTimeServer started\r\n");
    fflush(stdout);
    
    // Manually subscribe to time events every minute to drive the alarm queue.
    printf("[MAIN] Subscribing minutes (every 1) to drive broadcasts\r\n");
    fflush(stdout);
    for (int hour = 0; hour < 24; ++hour)
    {
        for (int min = 0; min < 60; ++min)
        {
            timeServer.subscribe_minute(hour, min);
        }
    }
    printf("[MAIN] Subscriptions added; timers should populate now\r\n");
    fflush(stdout);

    // Give time for initial scheduling to take effect
    tx_thread_sleep(200);

    // Main processing loop
    uint32_t loop_count = 0;
    uint32_t last_seq = 0;
    while (1)
    {
        // Main OpenMRNLite processing loop
        openmrn.loop();
        BSP_LED_Toggle(LED_RED);
        
        // Check executor sequence frequently to see if it's processing
        uint32_t curr_seq = openmrn.stack()->executor()->sequence();
        if (curr_seq != last_seq)
        {
            printf("[MAIN] Executor sequence advanced: %lu -> %lu\r\n", last_seq, curr_seq);
            last_seq = curr_seq;
        }
        
        // Diagnostic output every 1 second (50 iterations at 20ms)
        if (++loop_count % 50 == 0)
        {
            printf("[MAIN] Loop: %lu iterations. Executor sequence=%lu\r\n",
                   (unsigned long)loop_count,
                   curr_seq);
        }
        
        tx_thread_sleep(20); // 20ms at 1000 Hz tick rate
    }
}

} // extern "C"
