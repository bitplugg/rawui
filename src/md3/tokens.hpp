// ============================================================================
//  RAW UI - Material Design 3 design tokens.
//
//  Values come from the M3 spec (typography, shape, elevation, motion).  Sizes
//  are multiplied by a display scale because the UI is designed for a living
//  room: 1080p on a TV, viewed from a sofa.
// ============================================================================
#pragma once

#include <cstdint>

#include "core/math.hpp"

namespace rawui {

// ------------------------------------------------------------------- shape --
namespace shape {
constexpr float kNone = 0.0f;
constexpr float kExtraSmall = 4.0f;
constexpr float kSmall = 8.0f;
constexpr float kMedium = 12.0f;
constexpr float kLarge = 16.0f;
constexpr float kLargeIncreased = 20.0f;
constexpr float kExtraLarge = 28.0f;
constexpr float kExtraLargeIncreased = 32.0f;
constexpr float kExtraExtraLarge = 48.0f;
constexpr float kFull = 9999.0f;
}  // namespace shape

// -------------------------------------------------------------- typography --
struct TextStyle {
    float size = 14.0f;
    float lineHeight = 20.0f;
    float letterSpacing = 0.0f;
    int weight = 400;
};

struct Typescale {
    TextStyle displayLarge, displayMedium, displaySmall;
    TextStyle headlineLarge, headlineMedium, headlineSmall;
    TextStyle titleLarge, titleMedium, titleSmall;
    TextStyle bodyLarge, bodyMedium, bodySmall;
    TextStyle labelLarge, labelMedium, labelSmall;

    // Uniformly grows/shrinks the scale (TV factor, user preference).
    Typescale scaled(float factor) const;
};

// The baseline M3 type scale (Roboto Flex weights).
Typescale defaultTypescale();

// Default size multiplier for a 1080p UI seen from a sofa.
float displayScaleDefault();

// --------------------------------------------------------------- elevation --
struct Elevation {
    float shadowAlpha = 0.0f;  // 0..1
    float shadowBlur = 0.0f;   // px
    float shadowDy = 0.0f;     // px
    float tintAlpha = 0.0f;    // primary tint mixed into the surface (dark UI)
};

Elevation elevationLevel(int level);

// ------------------------------------------------------------------ motion --
namespace motion {

// Durations in milliseconds.
constexpr float kShort1 = 50.0f;
constexpr float kShort2 = 100.0f;
constexpr float kShort3 = 150.0f;
constexpr float kShort4 = 200.0f;
constexpr float kMedium1 = 250.0f;
constexpr float kMedium2 = 300.0f;
constexpr float kMedium3 = 350.0f;
constexpr float kMedium4 = 400.0f;
constexpr float kLong1 = 450.0f;
constexpr float kLong2 = 500.0f;
constexpr float kLong3 = 550.0f;
constexpr float kLong4 = 600.0f;
constexpr float kExtraLong1 = 700.0f;
constexpr float kExtraLong2 = 800.0f;

struct Easing {
    float x1, y1, x2, y2;
    float operator()(float t) const;
};

constexpr Easing kLinear{0.0f, 0.0f, 1.0f, 1.0f};
constexpr Easing kStandard{0.2f, 0.0f, 0.0f, 1.0f};
constexpr Easing kStandardDecelerate{0.0f, 0.0f, 0.0f, 1.0f};
constexpr Easing kStandardAccelerate{0.3f, 0.0f, 1.0f, 1.0f};
constexpr Easing kEmphasized{0.2f, 0.0f, 0.0f, 1.0f};
constexpr Easing kEmphasizedDecelerate{0.05f, 0.7f, 0.1f, 1.0f};
constexpr Easing kEmphasizedAccelerate{0.3f, 0.0f, 0.8f, 0.15f};

}  // namespace motion

// ------------------------------------------------------------------- state --
// M3 "state layers": overlay opacity for hover/focus/press/drag.
namespace state {
constexpr float kHover = 0.08f;
constexpr float kFocus = 0.12f;
constexpr float kPressed = 0.12f;
constexpr float kDragged = 0.16f;
constexpr float kDisabled = 0.38f;
constexpr float kDisabledContainer = 0.12f;
}  // namespace state

}  // namespace rawui
