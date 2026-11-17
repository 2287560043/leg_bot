#include "app_logic.h"
#include "board_config.h"

void RobotState_Init(RobotState *s) { s->mode = ROBOT_INIT; }
bool RobotSample_Fresh(DataHeader s, uint32_t now, uint32_t timeout)
{ return s.valid && (uint32_t)(now - s.time_ms) <= timeout; }
void RobotState_Step(RobotState *s, bool ready, bool fault, RobotRequest request)
{
    if (s->mode == ROBOT_INIT) { s->mode = ROBOT_STANDBY; return; }
    if (fault || (s->mode == ROBOT_RUNNING && !ready)) {
        s->mode = ROBOT_FAULT; return;
    }
    if (s->mode == ROBOT_FAULT) {
        if (ready && request == REQUEST_RESET) s->mode = ROBOT_STANDBY;
        return;
    }
    if (request == REQUEST_DISABLE) s->mode = ROBOT_STANDBY;
    else if (s->mode == ROBOT_STANDBY && ready && request == REQUEST_ENABLE)
        s->mode = ROBOT_RUNNING;
}

static bool fresh(DataHeader sample, uint32_t now)
{ return RobotSample_Fresh(sample, now, APP_FEEDBACK_TIMEOUT_MS); }
uint32_t App_BlockReasons(const ImuData *imu, const AppFeedback *io,
                          bool attitude_ready, bool controller_ready, uint32_t now)
{
    uint32_t bits = io->fault_bits;
    if (!fresh(imu->header, now)) bits |= BLOCK_IMU;
    if (!attitude_ready) bits |= BLOCK_ATTITUDE;
    if (!fresh(io->wheels.header, now)) bits |= BLOCK_WHEELS;
    if (!fresh(io->legs.header, now)) bits |= BLOCK_LEGS;
    if (!fresh(io->header, now)) bits |= BLOCK_IO;
    if (!controller_ready) bits |= BLOCK_CONTROLLER;
    if (!io->transport_ready) bits |= BLOCK_TRANSPORT;
    if (io->wheels.faults) bits |= FAULT_WHEEL_DEVICE;
    if (io->legs.faults) bits |= FAULT_LEG_DEVICE;
    return bits;
}
ControlOutput App_GateOutput(const AppCommand *command, const AppFeedback *io,
                            uint32_t now)
{
    if (!command->run || !fresh(command->header, now) ||
        !io->transport_ready || !fresh(io->header, now) ||
        !fresh(io->wheels.header, now) || !fresh(io->legs.header, now) ||
        io->fault_bits || io->wheels.faults || io->legs.faults ||
        !fresh(command->target.wheels.header, now) ||
        !fresh(command->target.legs.header, now)) return (ControlOutput){0};
    return command->target;
}

volatile DebugSnapshot g_debug;

void Debug_RecordTask(TaskDiagnostics *stats, uint32_t start, uint32_t end,
                      uint32_t clock_hz, bool overrun)
{
    /* Unsigned subtraction handles one CYCCNT wrap. Includes preemption time. */
    stats->last_us = (uint32_t)((uint64_t)(uint32_t)(end - start) * 1000000U / clock_hz);
    if (stats->last_us > stats->max_us) stats->max_us = stats->last_us;
    if (overrun) stats->overruns++;
    stats->iterations++;
}
