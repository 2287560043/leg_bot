#include "control.h"
#include <cmath>
#include "attitude_filter.hpp"

#define LOG_TAG "control"
#include "logger.h"

namespace {
constexpr unsigned calibration_cnt = 500;

class AttitudeEstimator {
public:
    void reset() { *this = AttitudeEstimator{}; }

    bool update(ImuData& imu)
    {
        if (!imu.header.valid) return false;
        const uint32_t elapsed = imu.header.time_ms - prev_ms_;
        if (have_time_ && elapsed == 0) return false;
        if (have_time_ && elapsed > 100) reset();
        prev_ms_ = imu.header.time_ms;
        have_time_ = true;

        if (!calibrated_) {
            const float* a = imu.accel_m_s2;
            const float norm = std::sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
            float spin = 0;
            for (float v : imu.gyro_rad_s) spin += v*v;
            if (std::fabs(norm - G) > 0.979134f || spin > 0.0225f) { // 约8.59度的偏差
                calibration_samples_ = 0;
                for (float& v : bias_sum_) v = 0;
                return false;
            }
            if (calibration_samples_ == 0) calibration_start_ms_ = imu.header.time_ms;
            for (unsigned i = 0; i < 3; ++i) bias_sum_[i] += imu.gyro_rad_s[i];
            ++calibration_samples_;
            if (calibration_samples_ < calibration_cnt ||
                uint32_t(imu.header.time_ms - calibration_start_ms_) < 1000) return false;

            for (unsigned i = 0; i < 3; ++i) gyro_bias_[i] = bias_sum_[i] / calibration_samples_;
            const float roll = std::atan2(a[1], a[2]);
            const float pitch = std::atan2(-a[0], std::sqrt(a[1]*a[1] + a[2]*a[2]));
            filter_.reset(roll, pitch);
            calibrated_ = true;
        } else {
            float omega[3];
            for (unsigned i = 0; i < 3; ++i) omega[i] = imu.gyro_rad_s[i] - gyro_bias_[i];
            filter_.update(omega, imu.accel_m_s2, elapsed * 0.001f);
        }
        const auto flags = filter_.flags();
        if (flags.startup || flags.overrangeRecovery) return false;
        float angles[3];
        filter_.angles(angles);
        imu.roll_rad = angles[0]; imu.pitch_rad = angles[1]; imu.yaw_rad = angles[2];

        elog_i("imu", "roll=%.2f pitch=%.2f yaw=%.2f deg",
               imu.roll_rad * 57.29577951, imu.pitch_rad * 57.29577951,
               imu.yaw_rad * 57.29577951);

        return true;
    }
private:
    FusionFilter filter_;
    float bias_sum_[3]{}, gyro_bias_[3]{};
    uint32_t prev_ms_ = 0, calibration_start_ms_ = 0;
    unsigned calibration_samples_ = 0;
    bool have_time_ = false, calibrated_ = false;
};
AttitudeEstimator estimator;
}

void Controller_Init(void) { estimator.reset(); }
bool Attitude_Update(ImuData *imu) { return estimator.update(*imu); }
/* Pose observation does not arm or implement the balancing controller. */
bool Controller_Ready(void) { return false; }
bool Controller_Update(const ImuData *imu, const WheelFeedback *wheels,
                       const LegFeedback *legs, ControlOutput *output)
{
    (void)imu; (void)wheels; (void)legs;
    *output = ControlOutput{};
    return false;
}
