#include "stm32h7xx_hal.h"
/* TIM6 supplies the HAL millisecond clock; SysTick belongs to FreeRTOS. */
static TIM_HandleTypeDef tick_timer;
HAL_StatusTypeDef HAL_InitTick(uint32_t priority)
{
    RCC_ClkInitTypeDef clocks;
    uint32_t latency;
    if (priority >= (1UL << __NVIC_PRIO_BITS)) return HAL_ERROR;
    __HAL_RCC_TIM6_CLK_ENABLE();
    HAL_RCC_GetClockConfig(&clocks, &latency);
    uint32_t hz = HAL_RCC_GetPCLK1Freq();
    if (clocks.APB1CLKDivider != RCC_APB1_DIV1) hz *= 2U;
    tick_timer.Instance = TIM6;
    tick_timer.Init.Prescaler = hz / 1000000U - 1U;
    tick_timer.Init.Period = 999U;
    tick_timer.Init.CounterMode = TIM_COUNTERMODE_UP;
    tick_timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_NVIC_SetPriority(TIM6_DAC_IRQn, priority, 0);
    HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
    uwTickPrio = priority;
    if (HAL_TIM_Base_Init(&tick_timer) != HAL_OK) return HAL_ERROR;
    __HAL_TIM_CLEAR_FLAG(&tick_timer, TIM_FLAG_UPDATE);
    return HAL_TIM_Base_Start_IT(&tick_timer);
}
void TIM6_DAC_IRQHandler(void) { HAL_TIM_IRQHandler(&tick_timer); }
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *timer)
{ if (timer->Instance == TIM6) HAL_IncTick(); }
void HAL_SuspendTick(void) { __HAL_TIM_DISABLE_IT(&tick_timer, TIM_IT_UPDATE); }
void HAL_ResumeTick(void) { __HAL_TIM_ENABLE_IT(&tick_timer, TIM_IT_UPDATE); }
