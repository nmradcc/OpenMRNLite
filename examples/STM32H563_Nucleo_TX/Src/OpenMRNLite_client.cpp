/* OpenMRNLite Client Application Entry Point */

#include "OpenMRNLite_client.h"
#include "main.h"

extern "C" {

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
