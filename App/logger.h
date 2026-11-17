#pragma once
#include "elog.h"

#ifdef __cplusplus
extern "C" {
#endif

void Logger_Init(void); /* Once, after device HAL delays and before the scheduler. */
void Logger_Poll(void); /* Only the communication task calls this, once per period. */

extern volatile uint32_t g_log_dropped;

#ifdef __cplusplus
}
#endif
