#include "devices.h"
#include "board_config.h"
/* Replace these functions with real drivers; no HAL transfers occur here. */
void Devices_Init(void) {}
DeviceResult Imu_Read(ImuSample *s) { *s = (ImuSample){0}; return DEVICE_NOT_READY; }
DeviceResult Wheels_Read(WheelFeedback *s) { *s = (WheelFeedback){0}; return DEVICE_NOT_READY; }
DeviceResult Legs_Read(LegFeedback *s) { *s = (LegFeedback){0}; return DEVICE_NOT_READY; }
DeviceResult Wheels_Write(const WheelTarget *s) { (void)s; return DEVICE_NOT_READY; }
DeviceResult Legs_Write(const LegTarget *s) { (void)s; return DEVICE_NOT_READY; }
