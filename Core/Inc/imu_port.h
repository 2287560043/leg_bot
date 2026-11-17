#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bounded, synchronous transfer. CS is restored high even on failure. */
bool ImuPort_Transfer(bool accelerometer, const uint8_t *tx, uint8_t *rx, uint16_t length);
void ImuPort_DelayMs(uint32_t ms); /* Initialization only, before scheduler start. */
uint32_t ImuPort_TimeMs(void);    /* RTOS epoch, for runtime samples. */

#ifdef __cplusplus
}
#endif
