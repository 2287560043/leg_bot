# 第三方依赖

| 组件 | 实际版本/来源标识 | 来源 |
|---|---|---|
| STM32H7 HAL | 1.11.3（`stm32h7xx_hal.c` 版本宏） | ST 驱动源码副本 |
| STM32H7 CMSIS Device / CMSIS Core | 随本机 H723 驱动副本固定，见 SHA256 清单 | ST / Arm 源码 |
| FreeRTOS Kernel | 10.3.1 | ST CubeMX 配套源码副本 |
| x-io Fusion AHRS | `a8d7224f36a0ec82345ef49a3db50e65f8d3bab8` | [官方仓库](https://github.com/xioTechnologies/Fusion)，MIT，见 Fusion/ORIGIN.md |
| EasyLogger | `05505ca80925e99b03cccfeb7866064922bee6d9` | [官方仓库](https://github.com/armink/EasyLogger)，MIT；仅核心、异步、工具及公共头 |
| SEGGER RTT | `9106eaa39babe53e00994d7828c1afb29390cdc1` | [官方仓库](https://github.com/SEGGERMicro/RTT)，许可证见 RTT/LICENSE.md；仅 RTT 核心和头文件 |
| CMSIS-RTOS2 wrapper | 随 FreeRTOS 副本固定，见 SHA256 清单 | Arm / ST 源码 |
