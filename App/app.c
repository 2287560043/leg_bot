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
    uint32_t deadline = now_ms(), latched_faults = 0;
    TaskDiagnostics timing = {0};
    RobotState state; RobotState_Init(&state);
    for (;;) {
        const uint32_t start = Board_DebugCycles();
        ImuSample imu = {0}; Attitude attitude = {0};
        AppFeedback io = {0}; AppCommand command = {0};
        RobotRequest request = REQUEST_NONE;
        if (Imu_Read(&imu) != DEVICE_OK) imu.meta.valid = false;
        (void)xQueuePeek(feedback_box, &io, 0);
        uint32_t reasons = App_BlockReasons(&imu, &io, Controller_Ready(), now_ms());
        /* Transport is checked for arming, but does not prevent algorithm warmup. */
        if (!(reasons & ~BLOCK_TRANSPORT) &&
            !Controller_Update(&imu, &io.wheels, &io.legs, &attitude, &command.target))
            reasons |= BLOCK_UPDATE;
        (void)xQueueReceive(requests, &request, 0);
        const RobotMode previous = state.mode;
        RobotState_Step(&state, reasons == 0, (reasons & APP_DEVICE_FAULT_MASK) != 0, request);
        if (state.mode == ROBOT_FAULT && previous != ROBOT_FAULT) latched_faults = reasons;
        if (previous == ROBOT_FAULT && state.mode == ROBOT_STANDBY) latched_faults = 0;
        command.meta = (SampleMeta){true, now_ms()};
        command.run = state.mode == ROBOT_RUNNING && reasons == 0;
        if (!command.run) command.target = (ControlOutput){0};
        xQueueOverwrite(command_box, &command);

        deadline += APP_CONTROL_MS;
        Debug_RecordTask(&timing, start, Board_DebugCycles(), SystemCoreClock,
                         deadline_missed(deadline));
        DebugSnapshot snapshot = {
            .time_ms = now_ms(), .updates = timing.iterations, .mode = state.mode,
            .not_ready_bits = reasons, .fault_bits = latched_faults,
            .communication_errors = io.errors, .imu = imu, .attitude = attitude,
            .wheels = io.wheels, .legs = io.legs, .target = command.target,
            .io_enabled = io.enabled, .control = timing, .communication = io.timing
        };
        g_debug = snapshot; /* Single writer. Never read this back into control. */
        wait_period(&deadline);
    }
}
static void communication_task(void *argument)
{
    (void)argument;
    uint32_t deadline = now_ms();
    AppFeedback io = {0};
    for (;;) {
        const uint32_t start = Board_DebugCycles();
        io.fault_bits = 0;
        DeviceResult wheel_read = Wheels_Read(&io.wheels);
        DeviceResult leg_read = Legs_Read(&io.legs);
        if (wheel_read != DEVICE_OK) io.wheels.meta.valid = false;
        if (leg_read != DEVICE_OK) io.legs.meta.valid = false;
        if (wheel_read == DEVICE_ERROR) {
            io.fault_bits |= FAULT_WHEEL_READ; io.errors++;
            io.wheels.meta.valid = false;
        }
        if (leg_read == DEVICE_ERROR) {
            io.fault_bits |= FAULT_LEG_READ; io.errors++;
            io.legs.meta.valid = false;
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
        io.meta = (SampleMeta){true, now_ms()};
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
#define START_TASK(task_id, function, priority_value) do { \
    static StaticTask_t task_id##_cb; \
    static StackType_t task_id##_stack[512]; \
    const osThreadAttr_t attr = {.name = #task_id, .cb_mem = &task_id##_cb, \
        .cb_size = sizeof(task_id##_cb), .stack_mem = task_id##_stack, \
        .stack_size = sizeof(task_id##_stack), .priority = priority_value}; \
    if (!osThreadNew(function, NULL, &attr)) Error_Handler(); \
} while (0)
void App_Init(void)
{
    Board_DebugClockInit();
    Devices_Init(); Controller_Init();
    NEW_MAILBOX(feedback_box, AppFeedback);
    NEW_MAILBOX(command_box, AppCommand);
    requests = xQueueCreateStatic(4, sizeof(RobotRequest), request_storage, &request_cb);
    configASSERT(requests);
    START_TASK(control, control_task, osPriorityHigh);
    START_TASK(communication, communication_task, osPriorityNormal);
}
