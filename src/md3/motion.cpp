#include "md3/motion.hpp"

namespace rawui {

// ---------------------------------------------------------------------------
// Cubic bezier easing.  x(t) is solved with a few Newton steps (falling back to
// bisection when the derivative misbehaves), which is plenty accurate for
// animation.
float motion::Easing::operator()(float t) const {
    if (t <= 0.0f) return 0.0f;
    if (t >= 1.0f) return 1.0f;

    const auto bezier = [](float p1, float p2, float u) {
        const float v = 1.0f - u;
        return 3.0f * v * v * u * p1 + 3.0f * v * u * u * p2 + u * u * u;
    };
    const auto bezierPrime = [](float p1, float p2, float u) {
        const float v = 1.0f - u;
        return 3.0f * v * v * p1 + 6.0f * v * u * (p2 - p1) + 3.0f * u * u * (1.0f - p2);
    };

    float u = t;
    for (int i = 0; i < 8; ++i) {
        const float x = bezier(x1, x2, u) - t;
        if (std::fabs(x) < 1e-5f) break;
        const float d = bezierPrime(x1, x2, u);
        if (std::fabs(d) < 1e-6f) break;
        u -= x / d;
        u = clampf(u, 0.0f, 1.0f);
    }

    return bezier(y1, y2, u);
}

// ---------------------------------------------------------------------------
float Spring::step(float dt, float timeScale) {
    if (dt <= 0.0f) return value_;
    dt *= timeScale;
    if (dt > 0.1f) dt = 0.1f;

    // Fixed sub-steps keep the integrator stable regardless of frame rate.
    constexpr float kSub = 1.0f / 240.0f;
    int steps = static_cast<int>(std::ceil(dt / kSub));
    steps = clampi(steps, 1, 48);
    const float h = dt / static_cast<float>(steps);

    for (int i = 0; i < steps; ++i) {
        const float accel = -spec_.stiffness * (value_ - target_) - spec_.damping * velocity_;
        velocity_ += accel * h;
        value_ += velocity_ * h;
    }

    if (atRest()) {
        value_ = target_;
        velocity_ = 0.0f;
    }
    return value_;
}

// ---------------------------------------------------------------------------
void Tween::start(float durationMs, motion::Easing easing, float timeScale) {
    duration_ = maxf(1.0f, durationMs);
    elapsed_ = 0.0f;
    value_ = 0.0f;
    easing_ = easing;
    timeScale_ = timeScale;
    running_ = true;
}

float Tween::step(float dt) {
    if (!running_) return value_;

    elapsed_ += dt * 1000.0f * timeScale_;
    const float t = clampf(elapsed_ / duration_, 0.0f, 1.0f);
    value_ = easing_(t);

    if (t >= 1.0f) {
        running_ = false;
        value_ = 1.0f;
    }
    return value_;
}

}  // namespace rawui
