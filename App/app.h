#pragma once
#include "robot_types.h"
#include "devices.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { ROBOT_INIT, ROBOT_STANDBY, ROBOT_RUNNING, ROBOT_FAULT } RobotMode;
typedef enum { REQUEST_NONE, REQUEST_ENABLE, REQUEST_DISABLE, REQUEST_RESET } RobotRequest;

typedef struct {
    uint32_t last_us, max_us, overruns, iterations;
} TaskDiagnostics;

typedef struct {
    uint32_t time_ms, updates, mode;
    uint32_t not_ready_bits, fault_bits, communication_errors;
    ImuData imu;
    float attitude_deg[3]; /* roll, pitch, relative yaw */
    ImuDiagnostics imu_status;
    WheelFeedback wheels;
    LegFeedback legs;
    ControlOutput target; /* Published target; check io_enabled for the last IO gate. */
    uint32_t io_enabled;
    TaskDiagnostics control, communication, estimator;
} DebugSnapshot;

/* Observations only. One writer (control task); debugger reads are not atomic. */
extern volatile DebugSnapshot g_debug;

void App_Init(void);
/* Task context only; nonblocking. False means request queue is full. */
bool App_Request(RobotRequest request);

#ifdef __cplusplus
}
#endif
