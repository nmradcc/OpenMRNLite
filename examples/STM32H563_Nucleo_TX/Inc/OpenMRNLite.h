/* OpenMRNLite Application Header */

#ifndef __OPENMRNLITE_H
#define __OPENMRNLITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "tx_api.h"

/**
  * @brief  OpenMRNLite task entry function
  * @param  thread_input: Thread input parameter
  * @retval None
  */
void OpenMRNLite_Entry(ULONG thread_input);

#ifdef __cplusplus
}
#endif

#endif /* __OPENMRNLITE_H */
