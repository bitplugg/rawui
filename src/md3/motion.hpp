// ============================================================================
//  RAW UI - animation.
//
//  M3 Expressive leans on springs for anything spatial, so the workhorse here
//  is a damped harmonic oscillator integrated with fixed sub-steps.  Curves are
//  provided for the effect (fade/colour) animations.
// ============================================================================
#pragma once

#include "md3/tokens.hpp"

namespace rawui {

struct SpringSpec {
    float stiffness = 220.0f;
    float damping = 24.0f;
};

namespace springs {
// Spatial springs: enter/exit, movement, size change.
constexpr SpringSpec kSpatialDefault{220.0f, 24.0f};
constexpr SpringSpec kSpatialFast{420.0f, 30.0f};
constexpr SpringSpec kSpatialSlow{120.0f, 18.0f};
// Effects: opacity, colour, elevation.
constexpr SpringSpec kEffectsDefault{1600.0f, 60.0f};
constexpr SpringSpec kEffectsFast{3800.0f, 90.0f};
}  // namespace springs

class Spring {
public:
    Spring() = default;
    explicit Spring(const SpringSpec& spec) : spec_(spec) {}

    void setSpec(const SpringSpec& spec) { spec_ = spec; }
    void snapTo(float value) {
        value_ = value;
        velocity_ = 0.0f;
    }
    void setTarget(float target) { target_ = target; }

    float value() const { return value_; }
    float target() const { return target_; }
    float velocity() const { return velocity_; }

    bool atRest() const {
        return std::fabs(value_ - target_) < 0.0015f && std::fabs(velocity_) < 0.02f;
    }

    // Advances the simulation and returns the new value.
    float step(float dt, float timeScale = 1.0f);

private:
    SpringSpec spec_{};
    float value_ = 0.0f;
    float target_ = 0.0f;
    float velocity_ = 0.0f;
};

// Eased 0..1 progress that runs towards 1 once started.
class Tween {
public:
    void start(float durationMs, motion::Easing easing = motion::kStandard,
               float timeScale = 1.0f);
    void snapTo(float value) { value_ = value; }
    float step(float dt);
    float value() const { return value_; }
    bool done() const { return value_ >= 1.0f; }
    bool running() const { return running_; }

private:
    float duration_ = 0.0f;
    float elapsed_ = 0.0f;
    float value_ = 0.0f;
    float timeScale_ = 1.0f;
    motion::Easing easing_ = motion::kStandard;
    bool running_ = false;
};

}  // namespace rawui
