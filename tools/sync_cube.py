#!/usr/bin/env python3
"""Run after CubeMX generation to reconcile framework-owned RTOS integration."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]

def update(relative, transform):
    path = root / relative
    raw = path.read_bytes()
    before = raw.decode().replace('\r\n', '\n')
    after = transform(before)
    if after != before:
        path.write_bytes(after.replace('\n', '\r\n').encode() if b'\r\n' in raw else after.encode())

def main_source(text):
    # Discard generated scheduler calls; application owns task creation.
    text = re.sub(r'^\s*(?:osKernelInitialize|osKernelStart|MX_FREERTOS_Init)\(\);[^\n]*', '', text, flags=re.M)
    begin, end = '/* USER CODE BEGIN 2 */', '/* USER CODE END 2 */'
    if begin not in text or end not in text:
        raise RuntimeError('Missing main USER CODE 2 markers; inspect generation manually')
    block = text.split(begin, 1)[1].split(end, 1)[0]
    if 'App_Init();' not in block:
        text = text.replace(begin, begin + '\n  if (osKernelInitialize() != osOK) Error_Handler();\n  App_Init();\n  if (osKernelStart() != osOK) Error_Handler();')
    if '#include "app.h"' not in text:
        text = text.replace('/* USER CODE BEGIN Includes */', '/* USER CODE BEGIN Includes */\n#include "cmsis_os2.h"\n#include "app.h"')
    return text

def interrupts(text):
    for name in ('SVC_Handler', 'PendSV_Handler', 'SysTick_Handler', 'TIM6_DAC_IRQHandler'):
        text = re.sub(r'void ' + name + r'\(void\)\s*\{.*?\n\}', '', text, flags=re.S)
    return text

update('Core/Src/main.c', main_source)
update('Core/Src/stm32h7xx_it.c', interrupts)
update('Core/Src/stm32h7xx_hal_msp.c', lambda s: s.replace('HAL_NVIC_SetPriority(USART1_IRQn, 0, 0)', 'HAL_NVIC_SetPriority(USART1_IRQn, 5, 0)'))
update('Core/Inc/stm32h7xx_hal_conf.h', lambda s: s.replace('/* #define HAL_TIM_MODULE_ENABLED   */', '#define HAL_TIM_MODULE_ENABLED'))
(root / 'Core/Inc/FreeRTOSConfig.h').write_text(
    '/* CubeMX-facing entry; maintained settings live in rtos_config.h. */\n'
    '#pragma once\n#include "rtos_config.h"\n')
(root / 'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.22)\nproject(leg LANGUAGES C CXX ASM)\ninclude(cmake/firmware.cmake)\n')
print('CubeMX integration synchronized; framework owns RTOS tasks, vectors and TIM6 timebase.')
