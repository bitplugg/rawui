#include "md3/theme.hpp"

namespace rawui {

// ---------------------------------------------------------------------------
// Theme presets.  The seeds reuse the hand-picked Material You palette; each
// entry bundles the toggles that go with it so Settings (and the web theme
// generator) can offer one-click looks.
const ThemePreset kThemePresets[] = {
    {"Amethyst Dark", "Material baseline, dark", 0x6750A4, true, false, 0.0f},
    {"Amethyst Light", "Material baseline, light", 0x6750A4, false, false, 0.0f},
    {"Ocean", "Deep blue, dark, calm", 0x00497F, true, false, 0.0f},
    {"Meadow", "Green, light, airy", 0x006C4D, false, false, 0.0f},
    {"Sunset", "Warm red, dark, vivid", 0x7A3045, true, true, 0.1f},
    {"Honey", "Amber, light, golden", 0x7C5800, false, false, 0.0f},
    {"Violet Night", "Purple, dark, high contrast", 0x4C3B6F, true, false, 0.2f},
    {"Graphite", "Neutral grey, dark, focused", 0x5A5A66, true, false, -0.1f},
    {"Sky", "Bright blue, light, airy", 0x0B57D0, false, false, 0.0f},
    {"Cyber", "Electric teal, dark, vibrant", 0x00639B, true, true, 0.1f},
};
const int kThemePresetCount = static_cast<int>(sizeof(kThemePresets) / sizeof(kThemePresets[0]));

void Theme::apply(const ThemeConfig& cfg, float displayScale) {
    config = cfg;
    displayScale_ = displayScale;
    dark = cfg.dark;
    motionScale = cfg.motionScale;

    const SchemeOptions opts{cfg.contrast, cfg.vibrant, true};
    to_ = makeScheme(cfg.seed, cfg.dark, opts);
    from_ = to_;
    scheme = to_;
    animating_ = false;

    type = defaultTypescale().scaled(displayScale * cfg.typeScale);
}

void Theme::animateTo(const ThemeConfig& cfg, float displayScale) {
    displayScale_ = displayScale;
    config = cfg;
    dark = cfg.dark;
    motionScale = cfg.motionScale;

    if (motionScale <= 0.0f) {
        apply(cfg, displayScale);
        return;
    }

    const SchemeOptions opts{cfg.contrast, cfg.vibrant, true};
    const Scheme next = makeScheme(cfg.seed, cfg.dark, opts);

    from_ = scheme;
    to_ = next;
    animating_ = true;
    transition_.snapTo(0.0f);
    transition_.setTarget(1.0f);

    type = defaultTypescale().scaled(displayScale * cfg.typeScale);
}

void Theme::buildCurrent() {
    scheme = mixScheme(from_, to_, clampf(transition_.value(), 0.0f, 1.0f));
}

void Theme::update(float dt) {
    // Typography changes are instant, but the type scale can be animated by the
    // caller if it wants to.
    if (!animating_) return;

    transition_.step(dt, motionScale);
    buildCurrent();

    if (transition_.atRest()) {
        animating_ = false;
        scheme = to_;
    }
}

}  // namespace rawui
