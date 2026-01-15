/* OpenMRNLite Client Application Entry Point */

#include "OpenMRNLite_client.h"
#include "main.h"
#include "app_azure_rtos.h"
#include "FdCanBridge.h"

#include <OpenMRNLite.h>

/// This is the OpenLCB Node ID. It must be coming from the Node ID range
/// assigned to the developer (get a range assigned to you via openlcb.org).
static constexpr uint64_t NODE_ID = UINT64_C(0x050101011824);

static OpenMRN openmrn(NODE_ID);

namespace openlcb
{
/// These definitions tell how the Node will appear on the OpenLCB bus for a
/// network browser.
extern const SimpleNodeStaticValues SNIP_STATIC_DATA = {
    4,
    "OpenMRN",
    "CAN-Node STM32H563_NUCLEO client",
    "STM32H563 NUCLEO",
    "1.00"
};

/// SNIP dynamic filename (not used in this minimal example)
extern const char *const SNIP_DYNAMIC_FILENAME = nullptr;
} // namespace openlcb

// External function to setup the CAN bridge
extern void setup_can_bridge(OpenMRN *openmrn);

#include "openlcb/BroadcastTimeAlarm.hxx"
#include "openlcb/BroadcastTimeClient.hxx"


// Which clock to display.
static constexpr auto CLOCK_ID =
    openlcb::BroadcastTimeDefs::DEFAULT_FAST_CLOCK_ID;

// Whether to allow remote control of the fast clock.
static constexpr auto ALLOW_CONTROL = true;

openlcb::BroadcastTimeClient time_client {
    openmrn.stack()->node(), CLOCK_ID, ALLOW_CONTROL};

/// Called when the date is changed.
void update_date(BarrierNotifiable *done)
{
    int y = time_client.year();
    int mon, day;
    time_client.date(&mon, &day);
    auto date = openlcb::BroadcastTimeDefs::date_to_string(y, mon, day);
    printf("Date: ");
    printf("%s\n", date.c_str());
    if (done)
        done->notify();
}

/// Called when the minute on the clock is changed.
void update_minute(BarrierNotifiable *done)
{
    struct tm time;
    time_client.gmtime_r(&time);
    auto str =
        openlcb::BroadcastTimeDefs::time_to_string(time.tm_hour, time.tm_min);
    printf("%s\n", str.c_str());
    if (done)
        done->notify();
}

/// Called when the current known time makes a jump.
void update(time_t old, time_t current)
{
    struct tm time;
    time_client.gmtime_r(&time);
    auto time_s =
        openlcb::BroadcastTimeDefs::time_to_string(time.tm_hour, time.tm_min);
    int y = time_client.year();
    int mon, day;
    time_client.date(&mon, &day);
    auto date_s = openlcb::BroadcastTimeDefs::date_to_string(y, mon, day);
    const char *is_run = time_client.is_running() ? "running" : "stopped";
    int rate_q = time_client.get_rate_quarters();
    string rate_s = openlcb::BroadcastTimeDefs::rate_quarters_to_string(rate_q);
    printf("Clock %s, rate %s, %s %s\n", is_run, rate_s.c_str(),
        date_s.c_str(), time_s.c_str());
}

openlcb::BroadcastTimeAlarmMinute alarm_minute {
    openmrn.stack()->node(), &time_client, &update_minute};

openlcb::BroadcastTimeAlarmDate alarm_date {
    openmrn.stack()->node(), &time_client, &update_date};

/// Called periodically in the loop. Checks whether we got connected to a
/// server.
void check_server()
{
    static bool server_shadow = false;
    bool has_srv = time_client.is_server_detected();
    if (has_srv != server_shadow)
    {
        printf("%s\n",
            has_srv ? "Found time source." : "Time source lost.");
    }
    server_shadow = has_srv;
    static bool warning = false;
    if (!has_srv && !warning)
//TODO: add timeout    if (!has_srv && !warning && millis() > 5000)
    {
        printf("No time source. Make sure that there is a clock "
                            "generator for the given clock ID.\n");
        warning = true;
    }
}



extern "C" {
void OpenMRNLite_client_Entry(ULONG thread_input)
{
    // Initialize CAN bridge to connect HAL to OpenMRN
    setup_can_bridge(&openmrn);
    
    // Start the OpenMRN stack
    openmrn.begin();
    time_client.update_subscribe_add(&update);
    
    // Main processing loop
    while (1)
    {
        // Main OpenMRNLite processing loop
        openmrn.loop();
        check_server();
        BSP_LED_Toggle(LED_YELLOW);
        tx_thread_sleep(20); // 20ms at 1000 Hz tick rate
    }
}

} // extern "C"
