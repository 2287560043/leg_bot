#include "logger.h"
#include "SEGGER_RTT.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "main.h"
#include <stdio.h>

static StaticSemaphore_t mutex_storage;
static SemaphoreHandle_t mutex;
volatile uint32_t g_log_dropped;

void elog_port_output(const char *log, size_t size);
void elog_port_output_lock(void);
void elog_port_output_unlock(void);

void Logger_Init(void)
{
    if (elog_init() != ELOG_NO_ERR) Error_Handler();
    for (uint8_t level = 0; level < ELOG_LVL_TOTAL_NUM; ++level)
        elog_set_fmt(level, ELOG_FMT_LVL | ELOG_FMT_TAG | ELOG_FMT_TIME);
    elog_set_filter_lvl(ELOG_LVL_INFO);
    elog_start();
}

void Logger_Poll(void)
{
    char line[ELOG_LINE_BUF_SIZE];
    const size_t size = elog_async_get_line_log(line, sizeof(line));
    if (size) elog_port_output(line, size);
}

ElogErrCode elog_port_init(void)
{
    mutex = xSemaphoreCreateMutexStatic(&mutex_storage);
    configASSERT(mutex);
    SEGGER_RTT_Init();
    return ELOG_NO_ERR;
}

void elog_port_deinit(void) {}

void elog_port_output(const char *log, size_t size)
{
    if (SEGGER_RTT_WriteSkipNoLock(0, log, (unsigned)size) != size) {
        elog_port_output_lock();
        ++g_log_dropped;
        elog_port_output_unlock();
    }
}

void elog_port_output_lock(void)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        (void)xSemaphoreTake(mutex, portMAX_DELAY);
}

void elog_port_output_unlock(void)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
        (void)xSemaphoreGive(mutex);
}

const char *elog_port_get_time(void)
{
    static char time[11];
    (void)snprintf(time, sizeof(time), "%lu", (unsigned long)HAL_GetTick());
    return time;
}

const char *elog_port_get_p_info(void) { return ""; }
const char *elog_port_get_t_info(void) { return ""; }
void elog_async_output_notice(void) {} /* Communication task polls every 5 ms. */
void elog_async_output_dropped(void) { ++g_log_dropped; } /* Already locked. */
