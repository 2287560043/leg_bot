# leg_bot

基于达妙 DM-MC-Board02（STM32H723）的微型轮腿机器人主控固件，使用 C11 / C++17 和 FreeRTOS。

目前已实现 BMI088 数据采集、基于 x-io Fusion 的三轴姿态估计，以及 EasyLogger / RTT 日志输出。轮腿驱动、平衡控制和运动规划仍在开发中。

## 构建

需要 ARM GNU Toolchain（`arm-none-eabi-gcc/g++`）、CMake 3.22 及以上版本和 Ninja。

```sh
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake
cmake --build build
```

生成文件位于 `build/`，包括 ELF、HEX 和 BIN。
Ozone 调试工程为根目录的 `leg.jdebug`。

## 目录

- `App/`：应用任务、设备接口、姿态算法和日志。
- `Core/`：启动、硬件初始化及 RTOS 配置。
- `vendor/`：第三方源码，版本和许可证见 [依赖说明](vendor/README.md)。
- `tools/`：CubeMX 生成代码同步工具。