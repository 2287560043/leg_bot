#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct { bool valid; uint32_t time_ms; } SampleMeta;
typedef struct { SampleMeta meta; float accel_m_s2[3], gyro_rad_s[3]; } ImuSample;
typedef struct { SampleMeta meta; float roll_rad, pitch_rad, yaw_rad; } Attitude;
typedef struct { SampleMeta meta; float angle_rad[2], speed_rad_s[2]; uint32_t faults; } WheelFeedback;
typedef struct { SampleMeta meta; float position_rad[2]; uint32_t faults; } LegFeedback;
/* Voltage-mode FOC setpoint, not measured torque/current. */
typedef struct { SampleMeta meta; bool enable; float voltage_v[2]; } WheelTarget;
typedef struct { SampleMeta meta; bool enable; float position_rad[2], speed_rad_s[2]; } LegTarget;
typedef struct { WheelTarget wheels; LegTarget legs; } ControlOutput;
