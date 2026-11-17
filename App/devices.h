#pragma once
#include "robot_types.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum { DEVICE_OK, DEVICE_NOT_READY, DEVICE_ERROR } DeviceResult;
typedef struct {
    uint32_t ready, error, errors, samples, accel_id, gyro_id;
} ImuDiagnostics;
/* error: 0 OK, 1 SPI, 2 chip ID, 3 config readback, 4 sensor error. */
void Imu_GetDiagnostics(ImuDiagnostics *diagnostics);
void Devices_Init(void);
/* Read functions set header.valid=false on failure/not-ready. IMU raw values
 * are bounded, finite int16 conversions; callers need not validate them again. */
DeviceResult Imu_Read(ImuData *sample);
DeviceResult Wheels_Read(WheelFeedback *feedback);
DeviceResult Legs_Read(LegFeedback *feedback);
DeviceResult Wheels_Write(const WheelTarget *target);
DeviceResult Legs_Write(const LegTarget *target);

#ifdef __cplusplus
}
#endif
