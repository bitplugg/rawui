#include "md3/theme.hpp"

namespace rawui {

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
