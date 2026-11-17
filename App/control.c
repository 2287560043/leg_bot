#include "control.h"
void Controller_Init(void) {}
bool Controller_Ready(void) { return false; }
bool Controller_Update(const ImuSample *imu, const WheelFeedback *wheels,
                       const LegFeedback *legs, Attitude *attitude, ControlOutput *output)
{
    (void)imu; (void)wheels; (void)legs;
    *attitude = (Attitude){0};
    *output = (ControlOutput){0};
    return false; /* Add attitude estimation and control here. */
}
