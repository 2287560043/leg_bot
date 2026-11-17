# 微型轮腿机器人 STM32 主控框架

面向 DM-MC-Board02 / STM32H723VGT6，使用 C11、HAL、FreeRTOS 10.3.1 和 CMSIS-RTOS2。硬件方案见 [for_agent.md](for_agent.md)。当前交付可编译的主控应用骨架，设备驱动、姿态解算、自平衡、运动规划和 ESP32 固件后续接入。

框架启动流程、任务分工、数据流及扩展方法见 [FRAMEWORK.md](FRAMEWORK.md)。

## 构建与验证

需要 `arm-none-eabi-gcc`、CMake ≥ 3.22、Ninja；主机测试另需 `cc`。第三方源码随工程保存。

```sh
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake
cmake --build build
sh tests/run.sh
```

构建生成 `build/leg.elf`、`leg.hex`、`leg.bin`、`leg.map`。Flash 起始地址 `0x08000000`，链接脚本使用 1 MB Flash 和 AXI SRAM 的保守 128 KB 区域。未分配其余 SRAM；任务、队列和内核对象均静态分配，没有 FreeRTOS 动态堆。每个应用任务栈 2 KB，后续用实际栈水位校准。

当前验证覆盖编译链接、状态转换和占位接口默认值；尚未烧录验证任务运行、时序和硬件通信。

## 模块与任务

| 目录 | 职责 |
|---|---|
| `Core` | 启动、CubeMX 初始化、板级配置、RTOS 和计时支持 |
| `App` | 两个任务、状态/保护/调试逻辑、设备接口和算法入口 |
| `vendor` | 固定第三方依赖与版本/许可证说明 |

| 任务 | 默认周期 | 优先级 | 职责 |
|---|---|---|---|
| control | 2 ms | High | IMU 采集、算法计算、状态管理、目标及调试数据发布 |
| communication | 5 ms | Normal | 轮腿收发、发送前保护检查、通信诊断 |

任务、队列和内核对象静态分配。任务之间只有两个单槽快照队列，操作请求使用四槽 FIFO；没有独立采集、状态或调试任务。SysTick 专供 FreeRTOS，TIM6 提供 HAL 时基。详细数据流见 [FRAMEWORK.md](FRAMEWORK.md)。

## Ozone 调试

使用 J-Link 原生 SWD 打开 [leg.jdebug](leg.jdebug)，从默认 `build/leg.elf` 加载固件；直接采样 `g_debug`，初始请求采样率 200 Hz。提供姿态、轮速、轮腿目标、状态、故障原因和任务耗时/超期统计，不引入 ROS、RTT 日志或在线调参。

操作步骤、字段说明和验证范围见 [OZONE.md](OZONE.md)。当前环境无法连接 J-Link，实际下载、运行时波形和 RTOS 插件显示仍待硬件验证。

## 后续接入位置

1. 在 `App/devices.c` 实现设备驱动。当前读写均返回 `DEVICE_NOT_READY`，读出的有效标志为 false，不发生硬件传输。驱动应提供非阻塞或有界耗时的接口；无新数据时可返回最后一次成功采样的快照，保留原时间戳，不能刷新旧数据的时间戳。
2. 在 `App/control.c` 实现姿态估计与平衡控制。`Controller_Ready()` 默认 false；成功更新时填写姿态、轮部和腿部目标的有效标志、时间戳及使能标志。轮部目标单位为 V，轮角度/速度与关节角度/速度为 rad / rad/s，IMU 为 m/s² / rad/s。这些是内部接口，不代表已经确定 UART 帧格式。
3. 在板级配置中确认片选、舵机 ID、方向、机械限位和驱动限制，再实现具体限幅。未核实数值没有填写猜测默认值。USART1 当前 PB14/PB15，UART7 当前 PE8/PE7；文档中的 PA9/PA10 尚未与接线对齐。BMI088 片选尚未配置。
4. 从后续遥控或调试模块在任务上下文调用 `App_Request(REQUEST_ENABLE / REQUEST_DISABLE / REQUEST_RESET)`，返回 false 表示未接受。当前没有自动使能或遥控输入。

初始化后保持待机。所有反馈有效且未超时、控制器就绪、传输接口就绪后，显式 ENABLE 请求才可运行。运行期间失去就绪条件或出现设备故障会进入故障；恢复条件后需先 RESET，再重新 ENABLE。默认反馈超时为 100 ms，时间差支持毫秒计数回绕。通信任务对运行许可、目标时效和当前轮腿反馈再次检查，条件不足时调用设备接口发送禁用目标；当前占位接口不会真正发包。实际 ESP32 固件仍需独立实现失联禁用，避免依赖主控持续运行才能停车。

## CubeMX 重新生成

`.ioc` 已选用 FreeRTOS CMSIS V2、静态分配和 TIM6 HAL 时基；应用任务由 `App_Init()` 创建。重新生成后执行：

```sh
python3 tools/sync_cube.py
cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake
cmake --build build
```

同步脚本保留 USER CODE 接入，移除生成器重复的调度器调用和中断实现，恢复框架 CMake 入口及 FreeRTOS 配置入口。实际构建配置保存在 `cmake/firmware.cmake`，RTOS 配置保存在 `Core/Inc/rtos_config.h`，`Core/Inc/FreeRTOSConfig.h` 仅包含该配置，重新生成后由同步脚本恢复，避免手工配置被覆盖。不会编译生成器新增的 `freertos.c` 和 HAL 时基文件。修改框架构建设置请编辑该 CMake 文件。`Core` 中的系统和启动文件是本次构建使用的固定副本。

本机未安装可调用的 CubeMX，尚未验证真实重新生成过程；同步脚本已做重复执行与模拟生成内容验证，实际升级生成器后仍需检查差异并重新编译。

依赖实际版本及本地来源限制见 [vendor/README.md](vendor/README.md)。
