#pragma once

#define SEGGER_RTT_MAX_NUM_UP_BUFFERS 1
#define SEGGER_RTT_MAX_NUM_DOWN_BUFFERS 1
#define BUFFER_SIZE_UP 2048
#define BUFFER_SIZE_DOWN 16
#define SEGGER_RTT_MODE_DEFAULT SEGGER_RTT_MODE_NO_BLOCK_SKIP
#define RTT_USE_ASM 0
/* Current startup leaves the CPU data cache disabled. */
