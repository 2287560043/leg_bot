# 固定的第三方依赖

这些源文件已随工程保存，构建无需访问网络或引用其他本地项目。

| 组件 | 实际版本/来源标识 | 来源 |
|---|---|---|
| STM32H7 HAL | 1.11.3（`stm32h7xx_hal.c` 版本宏） | 本机已有 ST 官方驱动副本 |
| STM32H7 CMSIS Device / CMSIS Core | 随本机 H723 驱动副本固定，见 SHA256 清单 | ST / Arm 源码 |
| FreeRTOS Kernel | 10.3.1 | 本机已有 CubeMX 工程中的 ST 配套副本 |
| CMSIS-RTOS2 wrapper | 随 FreeRTOS 副本固定，见 SHA256 清单 | Arm / ST 源码 |

`.ioc` 声明的包版本是 STM32Cube H7 V1.12.1；本机没有完整安装包，因此无法核实这些本地副本与该包是否逐文件一致。这里记录实际版本，不将其宣称为已核验的完整 V1.12.1 包。后续升级请整套更换匹配的依赖并重新验证。

保留了源码版权头与各组件许可证。FreeRTOS 的 `ARM_CM4F` 浮点端口用于当前 Cortex-M7 配置；编译器目标始终为 Cortex-M7。仅保留当前所需的 GCC 端口，不使用动态堆实现。

`SHA256SUMS` 固定当前所有第三方文件的内容（不包含本说明和清单自身），可在工程根目录运行 `sha256sum -c vendor/SHA256SUMS` 核对。
