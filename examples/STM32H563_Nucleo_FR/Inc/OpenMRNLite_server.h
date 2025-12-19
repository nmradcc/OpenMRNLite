/* OpenMRNLite Server Application Header */

#ifndef __OPENMRNLITE_SERVER_H
#define __OPENMRNLITE_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os.h"

/**
  * @brief  OpenMRNLite task entry function
  * @param  thread_input: Thread input parameter
  * @retval None
  */
void OpenMRNLite_server_Entry(ULONG thread_input);

#ifdef __cplusplus
}
#endif

#endif /* __OPENMRNLITE_SERVER_H */
