#pragma once
#include "robot_types.h"

#ifdef __cplusplus
extern "C" {
#endif
bool Attitude_Update(ImuData *imu);

void Controller_Init(void);
bool Controller_Ready(void);
bool Controller_Update(const ImuData *imu, const WheelFeedback *wheels,
                       const LegFeedback *legs, ControlOutput *output);

#ifdef __cplusplus
}
#endif
