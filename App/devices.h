#pragma once
#include "robot_types.h"
typedef enum { DEVICE_OK, DEVICE_NOT_READY, DEVICE_ERROR } DeviceResult;
void Devices_Init(void);
DeviceResult Imu_Read(ImuSample *sample);
DeviceResult Wheels_Read(WheelFeedback *feedback);
DeviceResult Legs_Read(LegFeedback *feedback);
DeviceResult Wheels_Write(const WheelTarget *target);
DeviceResult Legs_Write(const LegTarget *target);
