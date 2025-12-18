/* OpenMRNLite Application Entry Point */

#include "OpenMRNLite.h"

extern "C" {

void OpenMRNLite_Entry(ULONG thread_input)
{
    // Initialize OpenMRNLite here
    
    while (1)
    {
        // Main OpenMRNLite processing loop
        
        tx_thread_sleep(10);
    }
}

} // extern "C"
