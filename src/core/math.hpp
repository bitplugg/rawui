// ============================================================================
//  RAW UI - small maths / geometry / colour primitives.
// ============================================================================
#pragma once

#include <algorithm>
// <cstdlib> must come before <cmath>: the toolchain's <cmath> does
// `using ::abs;`, and math.h only declares `abs` once stdlib.h has been seen.
#include <cstdlib>
#include <cmath>
#include <cstdint>

namespace rawui {

// ------------------------------------------------------------------ scalars --
constexpr float kPi = 3.14159265358979323846f;

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline int   clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float minf(float a, float b) { return a < b ? a : b; }
inline float maxf(float a, float b) { return a > b ? a : b; }

inline float smoothstep01(float t) {
    t = clampf(t, 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

inline float moveTowards(float current, float target, float maxDelta) {
    const float d = target - current;
    if (std::fabs(d) <= maxDelta) return target;
    return current + (d > 0.0f ? maxDelta : -maxDelta);
}

// --------------------------------------------------------------------- vec2 --
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }

    float length() const { return std::sqrt(x * x + y * y); }
};

// --------------------------------------------------------------------- rect --
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    Rect() = default;
    Rect(float x_, float y_, float w_, float h_) : x(x_), y(y_), w(w_), h(h_) {}

    static Rect fromLTRB(float l, float t, float r, float b) { return {l, t, r - l, b - t}; }

    float left() const { return x; }
    float top() const { return y; }
    float right() const { return x + w; }
    float bottom() const { return y + h; }
    float cx() const { return x + w * 0.5f; }
    float cy() const { return y + h * 0.5f; }

    bool empty() const { return w <= 0.0f || h <= 0.0f; }
    bool contains(float px, float py) const { return px >= x && px < x + w && py >= y && py < y + h; }

    Rect inset(float d) const { return {x + d, y + d, w - 2 * d, h - 2 * d}; }
    Rect outset(float d) const { return inset(-d); }
    Rect translated(float dx, float dy) const { return {x + dx, y + dy, w, h}; }

    Rect intersect(const Rect& o) const {
        const float l = maxf(x, o.x), t = maxf(y, o.y);
        const float r = minf(right(), o.right()), b = minf(bottom(), o.bottom());
        return {l, t, maxf(0.0f, r - l), maxf(0.0f, b - t)};
    }

    bool overlaps(const Rect& o) const { return !intersect(o).empty(); }
};

// -------------------------------------------------------------------- color --
struct Color {
    uint8_t r = 0, g = 0, b = 0, a = 255;

    constexpr Color() = default;
    constexpr Color(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255)
        : r(r_), g(g_), b(b_), a(a_) {}

    static constexpr Color fromHex(uint32_t rgb, uint8_t alpha = 255) {
        return Color(static_cast<uint8_t>((rgb >> 16) & 0xFF),
                     static_cast<uint8_t>((rgb >> 8) & 0xFF),
                     static_cast<uint8_t>(rgb & 0xFF), alpha);
    }

    uint32_t argb() const {
        return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(r) << 16) |
               (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(b);
    }

    Color withAlpha(uint8_t alpha) const { return Color(r, g, b, alpha); }
    Color withAlphaF(float alpha) const {
        return Color(r, g, b, static_cast<uint8_t>(clampf(alpha, 0.0f, 1.0f) * 255.0f + 0.5f));
    }
    Color scaled(float f) const {
        return Color(static_cast<uint8_t>(clampf(r * f, 0.0f, 255.0f)),
                     static_cast<uint8_t>(clampf(g * f, 0.0f, 255.0f)),
                     static_cast<uint8_t>(clampf(b * f, 0.0f, 255.0f)), a);
    }
};

inline Color lerpColor(const Color& a, const Color& b, float t) {
    const auto mix = [t](uint8_t x, uint8_t y) {
        return static_cast<uint8_t>(clampf(lerpf(x, y, t), 0.0f, 255.0f) + 0.5f);
    };
    return Color(mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a));
}

}  // namespace rawui
