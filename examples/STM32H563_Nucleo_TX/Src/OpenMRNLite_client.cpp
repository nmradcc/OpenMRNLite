/* OpenMRNLite Client Application Entry Point */

#include "OpenMRNLite_client.h"
#include "main.h"

#include <OpenMRNLite.h>

/// This is the OpenLCB Node ID. It must be coming from the Node ID range
/// assigned to the developer (get a range assigned to you via openlcb.org).
static constexpr uint64_t NODE_ID = UINT64_C(0x050101011824);

OpenMRN openmrn(NODE_ID);

namespace openlcb
{
/// SNIP dynamic filename (not used in this minimal example)
extern const char *const SNIP_DYNAMIC_FILENAME = nullptr;
} // namespace openlcb

extern "C" {

// Stub for buffer_malloc (used by dynamic buffer pool)
void *buffer_malloc(size_t size)
{
    return malloc(size);
}

void OpenMRNLite_client_Entry(ULONG thread_input)
{
    // Initialize OpenMRNLite here
    
    while (1)
    {
        // Main OpenMRNLite processing loop
        
        tx_thread_sleep(10);
    }
}

} // extern "C"
