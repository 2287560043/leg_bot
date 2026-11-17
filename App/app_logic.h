#pragma once
#include "app.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_DEVICE_FAULT_MASK 0xFFFF0000U
enum {
    BLOCK_IMU = 1U << 0, BLOCK_WHEELS = 1U << 1, BLOCK_LEGS = 1U << 2,
    BLOCK_IO = 1U << 3, BLOCK_CONTROLLER = 1U << 4,
    BLOCK_TRANSPORT = 1U << 5, BLOCK_UPDATE = 1U << 6, BLOCK_ATTITUDE = 1U << 7,
    FAULT_WHEEL_DEVICE = 1U << 16, FAULT_LEG_DEVICE = 1U << 17,
    FAULT_WHEEL_READ = 1U << 18, FAULT_LEG_READ = 1U << 19,
    FAULT_WHEEL_WRITE = 1U << 20, FAULT_LEG_WRITE = 1U << 21
};
typedef struct {
    DataHeader header;
    WheelFeedback wheels;
    LegFeedback legs;
    bool transport_ready, enabled;
    uint32_t fault_bits, errors;
    TaskDiagnostics timing;
} AppFeedback;
typedef struct {
    DataHeader header;
    bool run;
    ControlOutput target;
} AppCommand;
uint32_t App_BlockReasons(const ImuData *imu, const AppFeedback *io,
                          bool attitude_ready, bool controller_ready, uint32_t now);
/* Returns zero/disabled targets unless every gate passes. */
ControlOutput App_GateOutput(const AppCommand *command, const AppFeedback *io,
                            uint32_t now);

typedef struct { RobotMode mode; } RobotState;
void RobotState_Init(RobotState *state);
/* One-shot requests. RESET only clears a fault after readiness is restored. */
void RobotState_Step(RobotState *state, bool ready, bool fault, RobotRequest request);
bool RobotSample_Fresh(DataHeader sample, uint32_t now_ms, uint32_t timeout_ms);

void Debug_RecordTask(TaskDiagnostics *stats, uint32_t start_cycles,
                      uint32_t end_cycles, uint32_t clock_hz, bool overrun);

#ifdef __cplusplus
}
#endif
