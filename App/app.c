#define LOG_TAG "app"
#include "logger.h"
#include "app.h"
#include "app_logic.h"
#include "board_config.h"
#include "rtos_support.h"
#include "control.h"
#include "devices.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "main.h"

/* Two single-slot snapshots, one FIFO of one-shot operator requests. */
#define MAILBOX(name, type) \
    static StaticQueue_t name##_cb; \
    static uint8_t name##_storage[sizeof(type)] __attribute__((aligned(4))); \
    static QueueHandle_t name
MAILBOX(feedback_box, AppFeedback);
MAILBOX(command_box, AppCommand);
static StaticQueue_t request_cb;
static uint8_t request_storage[4 * sizeof(RobotRequest)] __attribute__((aligned(4)));
static QueueHandle_t requests;
#define NEW_MAILBOX(name, type) do { \
    name = xQueueCreateStatic(1, sizeof(type), name##_storage, &name##_cb); \
    configASSERT(name); \
} while (0)

static uint32_t now_ms(void) { return osKernelGetTickCount(); } /* 1 kHz */
static bool deadline_missed(uint32_t deadline)
{ return (int32_t)(now_ms() - deadline) >= 0; }
static void wait_period(uint32_t *deadline)
{
    if (osDelayUntil(*deadline) != osOK) {
        /* Skip missed periods instead of busy-looping to catch up. */
        *deadline = now_ms();
        (void)osDelay(1);
    }
}
static void control_task(void *argument)
{
    (void)argument;
    uint32_t deadline = now_ms(), latched_faults = 0, last_pose_log = now_ms();
    bool pose_was_ready = false;
    DeviceResult previous_imu_result = DEVICE_NOT_READY;
    TaskDiagnostics timing = {0}, estimator_timing = {0};
    RobotState state; RobotState_Init(&state);
    for (;;) {
        const uint32_t start = Board_DebugCycles();
        ImuData imu = {0};
        AppFeedback io = {0}; AppCommand command = {0};
        RobotRequest request = REQUEST_NONE;
        const DeviceResult imu_result = Imu_Read(&imu);
        if (imu_result != previous_imu_result) {
            if (imu_result == DEVICE_ERROR) elog_e("imu", "read failed");
            else if (imu_result == DEVICE_OK) elog_i("imu", "samples available");
            previous_imu_result = imu_result;
        }
        const uint32_t estimator_start = Board_DebugCycles();
        const bool attitude_ready = Attitude_Update(&imu);
        Debug_RecordTask(&estimator_timing, estimator_start, Board_DebugCycles(), SystemCoreClock, false);
        (void)xQueuePeek(feedback_box, &io, 0);
        uint32_t reasons = App_BlockReasons(&imu, &io, attitude_ready, Controller_Ready(), now_ms());
        /* Transport is checked for arming, but does not prevent algorithm warmup. */
        if (!(reasons & ~BLOCK_TRANSPORT) &&
            !Controller_Update(&imu, &io.wheels, &io.legs, &command.target))
            reasons |= BLOCK_UPDATE;
        (void)xQueueReceive(requests, &request, 0);
        const RobotMode previous = state.mode;
        RobotState_Step(&state, reasons == 0, (reasons & APP_DEVICE_FAULT_MASK) != 0, request);
        if (state.mode == ROBOT_FAULT && previous != ROBOT_FAULT) latched_faults = reasons;
        if (previous == ROBOT_FAULT && state.mode == ROBOT_STANDBY) latched_faults = 0;
        if (state.mode != previous) {
            if (state.mode == ROBOT_FAULT)
                log_e("mode=%u -> %u, reasons=0x%08lx", (unsigned)previous,
                      (unsigned)state.mode, (unsigned long)reasons);
            else log_i("mode=%u -> %u", (unsigned)previous, (unsigned)state.mode);
        }
        command.header = (DataHeader){true, now_ms()};
        command.run = state.mode == ROBOT_RUNNING && reasons == 0;
        if (!command.run) command.target = (ControlOutput){0};
        xQueueOverwrite(command_box, &command);

        if (attitude_ready != pose_was_ready) {
            if (attitude_ready) elog_i("imu", "attitude ready");
            else elog_w("imu", "attitude unavailable, waiting for calibration/recovery");
            pose_was_ready = attitude_ready;
        }
        if (attitude_ready && (uint32_t)(now_ms() - last_pose_log) >= 100) {
            last_pose_log = now_ms();
            elog_i("imu", "roll=%.2f pitch=%.2f yaw=%.2f deg",
                   imu.roll_rad * 57.29577951, imu.pitch_rad * 57.29577951,
                   imu.yaw_rad * 57.29577951);
        }
        deadline += APP_CONTROL_MS;
        Debug_RecordTask(&timing, start, Board_DebugCycles(), SystemCoreClock,
                         deadline_missed(deadline));
        DebugSnapshot snapshot = {
            .time_ms = now_ms(), .updates = timing.iterations, .mode = state.mode,
            .not_ready_bits = reasons, .fault_bits = latched_faults,
            .communication_errors = io.errors, .imu = imu,
            .wheels = io.wheels, .legs = io.legs, .target = command.target,
            .io_enabled = io.enabled, .control = timing, .communication = io.timing, .estimator = estimator_timing
        };
        Imu_GetDiagnostics(&snapshot.imu_status);
        if (attitude_ready) {
            snapshot.attitude_deg[0] = imu.roll_rad * 57.29577951f;
            snapshot.attitude_deg[1] = imu.pitch_rad * 57.29577951f;
            snapshot.attitude_deg[2] = imu.yaw_rad * 57.29577951f;
        }
        g_debug = snapshot; /* Single writer. Never read this back into control. */
        wait_period(&deadline);
    }
}
static void communication_task(void *argument)
{
    (void)argument;
    uint32_t deadline = now_ms();
    uint32_t previous_faults = 0;
    AppFeedback io = {0};
    for (;;) {
        const uint32_t start = Board_DebugCycles();
        io.fault_bits = 0;
        DeviceResult wheel_read = Wheels_Read(&io.wheels);
        DeviceResult leg_read = Legs_Read(&io.legs);
        if (wheel_read == DEVICE_ERROR) {
            io.fault_bits |= FAULT_WHEEL_READ; io.errors++;
        }
        if (leg_read == DEVICE_ERROR) {
            io.fault_bits |= FAULT_LEG_READ; io.errors++;
        }
        AppCommand command = {0};
        (void)xQueuePeek(command_box, &command, 0);
        ControlOutput output = App_GateOutput(&command, &io, now_ms());
        DeviceResult wr = Wheels_Write(&output.wheels);
        DeviceResult lr = Legs_Write(&output.legs);
        io.transport_ready = wr == DEVICE_OK && lr == DEVICE_OK;
        io.enabled = io.transport_ready && (output.wheels.enable || output.legs.enable);
        if (wr == DEVICE_ERROR) { io.fault_bits |= FAULT_WHEEL_WRITE; io.errors++; }
        if (lr == DEVICE_ERROR) { io.fault_bits |= FAULT_LEG_WRITE; io.errors++; }
        if (io.fault_bits != previous_faults) {
            if (io.fault_bits) elog_e("io", "faults=0x%08lx", (unsigned long)io.fault_bits);
            else elog_i("io", "faults cleared");
            previous_faults = io.fault_bits;
        }
        Logger_Poll();
        io.header = (DataHeader){true, now_ms()};
        deadline += APP_COMM_MS;
        Debug_RecordTask(&io.timing, start, Board_DebugCycles(), SystemCoreClock,
                         deadline_missed(deadline));
        xQueueOverwrite(feedback_box, &io);
        wait_period(&deadline);
    }
}
bool App_Request(RobotRequest request)
{
    if (!requests || request <= REQUEST_NONE || request > REQUEST_RESET) return false;
    return xQueueSend(requests, &request, 0) == pdPASS;
}
#define START_TASK(task_id, function, priority_value, stack_words) do { \
    static StaticTask_t task_id##_cb; \
    static StackType_t task_id##_stack[stack_words]; \
    const osThreadAttr_t attr = {.name = #task_id, .cb_mem = &task_id##_cb, \
        .cb_size = sizeof(task_id##_cb), .stack_mem = task_id##_stack, \
        .stack_size = sizeof(task_id##_stack), .priority = priority_value}; \
    if (!osThreadNew(function, NULL, &attr)) Error_Handler(); \
} while (0)
void App_Init(void)
{
    Board_DebugClockInit();
    /* Complete HAL delays before creating any FreeRTOS objects: pre-scheduler
     * critical sections can leave BASEPRI set until the scheduler starts. */
    Devices_Init(); Controller_Init();
    Logger_Init();
    log_i("boot, control=%u ms, communication=%u ms", APP_CONTROL_MS, APP_COMM_MS);
    ImuDiagnostics imu_status;
    Imu_GetDiagnostics(&imu_status);
    if (imu_status.ready)
        elog_i("imu", "BMI088 initialized, keep still for 1 s");
    else elog_e("imu", "BMI088 init failed, error=%lu", (unsigned long)imu_status.error);
    NEW_MAILBOX(feedback_box, AppFeedback);
    NEW_MAILBOX(command_box, AppCommand);
    requests = xQueueCreateStatic(4, sizeof(RobotRequest), request_storage, &request_cb);
    configASSERT(requests);
    /* Float logging reached 1988 bytes on target; retain room for nested calls. */
    START_TASK(control, control_task, osPriorityHigh, 1024);
    START_TASK(communication, communication_task, osPriorityNormal, 512);
}
