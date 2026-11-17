#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
void vApplicationGetIdleTaskMemory(StaticTask_t **cb, StackType_t **stack, uint32_t *size)
{
    static StaticTask_t idle_cb;
    static StackType_t idle_stack[configMINIMAL_STACK_SIZE];
    *cb = &idle_cb; *stack = idle_stack; *size = configMINIMAL_STACK_SIZE;
}
void vApplicationGetTimerTaskMemory(StaticTask_t **cb, StackType_t **stack, uint32_t *size)
{
    static StaticTask_t timer_cb;
    static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH];
    *cb = &timer_cb; *stack = timer_stack; *size = configTIMER_TASK_STACK_DEPTH;
}
void vApplicationStackOverflowHook(TaskHandle_t task, char *name)
{ (void)task; (void)name; Error_Handler(); }
void vApplicationMallocFailedHook(void) { Error_Handler(); }

#include "rtos_support.h"
#include "stm32h7xx.h"
void Board_DebugClockInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    /* Unlock the Cortex-M7 DWT cycle counter. */
    DWT->LAR = 0xC5ACCE55U;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
    __ISB();
}
uint32_t Board_DebugCycles(void) { return DWT->CYCCNT; }
