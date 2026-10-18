// ============================================================================
//  RAW UI - the theme bundle: colour scheme + typography + motion preference,
//  with animated transitions between colour schemes.
// ============================================================================
#pragma once

#include "md3/color.hpp"
#include "md3/motion.hpp"
#include "md3/tokens.hpp"

namespace rawui {

struct ThemeConfig {
    Color seed = Color::fromHex(0x6750A4);
    bool dark = true;
    bool vibrant = false;
    float contrast = 0.0f;
    float typeScale = 1.0f;    // user preference, multiplies the TV scale
    float motionScale = 1.0f;  // 0 disables animation
};

// A complete named look: seed + mode + chroma treatment.  Picking one in
// Settings applies all four values at once (the theme "stylesheets").
struct ThemePreset {
    const char* name;
    const char* description;
    uint32_t seed;
    bool dark;
    bool vibrant;
    float contrast;
};
extern const ThemePreset kThemePresets[];
extern const int kThemePresetCount;

struct Theme {
    // Live values (what widgets read).
    Scheme scheme;
    Typescale type;

    bool dark = true;
    float motionScale = 1.0f;
    ThemeConfig config;

    // Rebuilds the scheme immediately.
    void apply(const ThemeConfig& cfg, float displayScale);
    // Animates towards a new configuration.
    void animateTo(const ThemeConfig& cfg, float displayScale);

    // Call once per frame; advances the scheme transition.
    void update(float dt);

    bool transitioning() const { return !transition_.atRest(); }

private:
    Scheme from_;
    Scheme to_;
    bool animating_ = false;
    Spring transition_{springs::kEffectsFast};
    float displayScale_ = 1.0f;

    void buildCurrent();
};

}  // namespace rawui
