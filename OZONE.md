# Ozone 波形与运行诊断

本工程通过 **J-Link 原生 USB/SWD** 连接 Ozone。固件暴露 `g_debug` 全局结构，Ozone 从 ELF 获取成员地址并后台采样，无需 ROS、RTT 日志或专门的调试任务。

本次完成固件构建、符号及主机逻辑测试；当前环境无法连接 J-Link，**尚未验证实际下载、运行波形、采样速度和 RTOS 插件显示**。工程脚本按本机 Ozone 3.52 手册编写，实际连接后需完成下述验收。

## 1. 连接与打开工程

1. 按开发板接口定义连接 J-Link 的 SWDIO、SWCLK、GND 和目标参考电压；按需要连接 NRST。确认板卡正常供电。
2. 先构建默认目录的固件：

   ```sh
   cmake -S . -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-gcc.cmake
   cmake --build build
   ```

3. 在 Ozone 打开根目录 [leg.jdebug](leg.jdebug)。它使用相对于工程文件的 `build/leg.elf`，配置 STM32H723VG、USB J-Link、SWD、1 MHz，未绑定序列号。
4. 多个 J-Link 同时连接时，在 Ozone 中选择实际探针。执行 Download & Reset Program，再 Resume。

第一版仍是占位固件，预期 `g_debug.mode` 为 STANDBY，测量有效标志为 false，任务计数持续增长。不要把全零姿态误认为真实测量成功。

## 2. 先看最简单的运行波形

在 Watched Data 添加 `g_debug`，展开查看成员。打开 View 中的 Data Sampling（旧版本可能显示 Data Graph），先添加：

```text
g_debug.updates
g_debug.control.last_us
g_debug.communication.last_us
```

也可在变量的右键菜单选择 Graph，将成员加入采样。工程将请求采样率设为 200 Hz；在 Data Sampling 的采样频率设置中可调整。实际频率受探针、通道数和 SWD 配置影响，不等于控制任务的 500 Hz，也不保证记录每轮控制数据。

运行时应看到 updates 增长、两个任务耗时有数值。计数曲线能验证读数链路，但不是模拟传感器波形。若不能采样，先检查固件确实运行、ELF 匹配，以及连接使用原生 J-Link 而不是 OpenOCD/GDB Server。

## 3. 接入驱动后看什么

| 表达式 | 含义 | 有效条件 |
|---|---|---|
| `g_debug.imu.accel_m_s2[0]` | 加速度 X | `imu.meta.valid` 且时间未过期 |
| `g_debug.imu.gyro_rad_s[1]` | 角速度 Y | 同上 |
| `g_debug.attitude.pitch_rad` | 俯仰角 | `attitude.meta.valid` 且时间未过期 |
| `g_debug.wheels.speed_rad_s[0]` / `[1]` | 两侧轮速 | `wheels.meta.valid` 且时间未过期 |
| `g_debug.wheels.angle_rad[0]` / `[1]` | 两侧轮角度 | 同上 |
| `g_debug.legs.position_rad[0]` / `[1]` | 两侧关节位置 | `legs.meta.valid` 且时间未过期 |
| `g_debug.target.wheels.voltage_v[0]` / `[1]` | 发布的轮部电压目标 | RUNNING、目标有效且使能 |
| `g_debug.target.legs.position_rad[0]` / `[1]` | 发布的腿部目标 | 同上 |
| `g_debug.io_enabled` | 上轮通信结果为 OK，且至少一类输出带使能 | 是软件发送状态，不是远端实际动作证明 |

`target` 是控制任务发布的目标，通信任务仍可能再次禁用；它不是驱动板实际电压或舵机实际目标的回读值。

## 4. 如何定位“为什么不能运行”

`g_debug.mode`：0 = INIT，1 = STANDBY，2 = RUNNING，3 = FAULT。

`g_debug.not_ready_bits` 为当前原因位，可以同时出现多个；`g_debug.fault_bits` 锁存本次进入 FAULT 的原因，直到恢复条件满足并成功 RESET。位定义以 `App/app_logic.h` 为准：

| 掩码 | 含义 |
|---|---|
| `0x000001` | IMU 无效或过期 |
| `0x000002` | 轮部反馈无效或过期 |
| `0x000004` | 腿部反馈无效或过期 |
| `0x000008` | 通信任务快照无效或过期 |
| `0x000010` | 控制器未就绪 |
| `0x000020` | 上轮轮部或腿部写接口未返回 OK |
| `0x000040` | 算法更新失败 |
| `0x010000` / `0x020000` | 轮部 / 腿部上报故障 |
| `0x040000` / `0x080000` | 轮部 / 腿部读接口返回 ERROR |
| `0x100000` / `0x200000` | 轮部 / 腿部写接口返回 ERROR |

NOT_READY 表示尚未接入或暂无可用结果，不计为通信错误；`communication_errors` 累计读写接口返回 DEVICE_ERROR 的次数，一轮内可增加多次。低层 CRC 错误和丢包计数需真实驱动后续补充。

## 5. 如何看任务运行情况

两个任务均提供：

- `last_us`：最近一次业务段的墙上时间，微秒，包括抢占时间，不含主动等待。
- `max_us`：上电以来最大值。
- `overruns`：业务段完成时已经达到下一计划时刻的次数，每轮最多加一。
- `iterations`：完成的轮数。

控制任务统计不包含末尾调试快照复制；通信任务统计不包含末尾反馈队列复制。它们用于发现耗时异常，不能直接当作 CPU 使用率。暂停、断点和单步后的耗时不用于性能评价。DWT 32 位周期差支持一次回绕，单次测量必须短于回绕周期；当前 192 MHz 下约 22 秒。

脚本加载 Ozone 自带的 `FreeRTOSPlugin_Cortex-M`。在 View → RTOS 中查看任务、状态和栈信息；应包含 control、communication，以及内核的 Idle 和软件定时器任务。这里的 RTOS awareness 不包含 SystemView 调度事件追踪。

## 6. 采样一致性与导出

`g_debug` 为 volatile，只有控制任务写入，但 Ozone 对多个字段的后台采样不保证来自同一轮更新。需要关联时同时观察 time_ms / updates；严格同步录波需要额外的采样缓冲，目前没有实现。

当前工程没有启用 D-cache。以后启用缓存时，需要保证调试变量所在区域对外部访问可见，例如配置为非缓存区域；仅加 volatile 不会解决缓存一致性。

在 Data Sampling 窗口右键使用 Export 导出 CSV。也可在 Ozone Console 执行：

```text
Export.DataGraphs("build/debug.csv");
```

本阶段不支持通过 g_debug 在线调参，不增加串口日志流。真实驱动和算法未就绪时，波形中的数值只能按对应有效标志解释。

参考：本机 Ozone 3.52 手册 `/opt/SEGGER/Ozone_V352/Doc/UM08025_Ozone.pdf`；[SEGGER HSS 说明](https://kb.segger.com/HSS)。
