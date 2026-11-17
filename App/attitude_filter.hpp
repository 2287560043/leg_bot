#pragma once
#include <cmath>
extern "C" {
#include "FusionAhrs.h"
}

constexpr float G = 9.79134f; // local gravity, m/s^2

class FusionFilter {
public:
    void reset(float roll = 0, float pitch = 0) {
        FusionAhrsInitialise(&ahrs_);
        FusionAhrsSettings settings = fusionAhrsDefaultSettings;
        settings.sampleRate = 500;
        settings.convention = FusionConventionNwu;
        settings.gain = 2.0f;
        settings.gyroscopeRange = 2000;
        settings.accelerationRejection = 10;
        settings.magneticRejection = 0;
        settings.rejectionTimeout = 5;
        FusionAhrsSetSettings(&ahrs_, &settings);
        const float cr = std::cos(roll/2), sr = std::sin(roll/2);
        const float cp = std::cos(pitch/2), sp = std::sin(pitch/2);
        FusionQuaternion q{};
        q.element.w = cr*cp; q.element.x = sr*cp;
        q.element.y = cr*sp; q.element.z = -sr*sp;
        FusionAhrsSetQuaternion(&ahrs_, q);
        FusionAhrsSkipStartup(&ahrs_);
    }
    void update(const float* gyro, const float* accel, float dt) {
        FusionVector g{}, a{};
        for (unsigned i = 0; i < 3; ++i) {
            g.array[i] = gyro[i] * 57.295779513f;
            a.array[i] = accel[i] / G;
        }
        FusionAhrsSetSamplePeriod(&ahrs_, dt);
        FusionAhrsUpdateNoMagnetometer(&ahrs_, g, a);
    }
    FusionQuaternion quaternion() const { return FusionAhrsGetQuaternion(&ahrs_); }
    void angles(float* out) const {
        const FusionEuler euler = FusionQuaternionToEuler(quaternion());
        for (unsigned i = 0; i < 3; ++i) out[i] = euler.array[i] * 0.01745329252f;
    }
    FusionAhrsInternalStates internal() const { return FusionAhrsGetInternalStates(&ahrs_); }
    FusionAhrsFlags flags() const { return FusionAhrsGetFlags(&ahrs_); }
private:
    FusionAhrs ahrs_{};
};
