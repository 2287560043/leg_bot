#pragma once
#include "robot_types.h"
void Controller_Init(void);
bool Controller_Ready(void);
bool Controller_Update(const ImuSample *imu, const WheelFeedback *wheels,
                       const LegFeedback *legs, Attitude *attitude, ControlOutput *output);
