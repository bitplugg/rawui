#include "screens/settings.hpp"

#include "gfx/icons.hpp"

namespace rawui {
namespace {

constexpr float kItemHeight = 72.0f;
constexpr float kSection = 40.0f;

uint32_t seedRgbToHex(const Color& c) {
    return (static_cast<uint32_t>(c.r) << 16) | (static_cast<uint32_t>(c.g) << 8) | c.b;
}

bool approxColor(const Color& a, const Color& b) {
    return std::abs(a.r - b.r) < 10 && std::abs(a.g - b.g) < 10 && std::abs(a.b - b.b) < 10;
}

}  // namespace

std::string SettingsScreen::subtitle(App& app) {
    (void)app;
    return "Appearance, typography and motion";
}

void SettingsScreen::onEnter(App& app) {
    app.focus().setPreferred(kToggleDark);
    scroll_.reset();
}

void SettingsScreen::drawThemePreset(App& app, const Rect& list, float& y, float itemH,
                                     int index) {
    ui::Ctx& ctx = app.ctx();
    const Scheme& c = app.colors();
    const float s = ctx.displayScale;
    const AppConfig& cfg = app.config();
    const ThemePreset& p = kThemePresets[index];

    const Rect r(list.x, y, list.w, itemH);
    const bool active = approxColor(Color::fromHex(cfg.seedRgb), Color::fromHex(p.seed)) &&
                        cfg.dark == p.dark && cfg.vibrant == p.vibrant;

    ui::drawText(ctx, p.name, ctx.type().titleMedium,
                 Rect(r.x + 4.0f * s, r.y, r.w * 0.6f, r.h * 0.5f),
                 active ? c.primary : c.onSurface, ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx, p.description, ctx.type().bodySmall,
                 Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w * 0.6f, r.h * 0.5f),
                 c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);

    // A two-dot preview: the seed and what the surface it produces looks like.
    const Scheme preview = makeScheme(Color::fromHex(p.seed), p.dark,
                                      SchemeOptions{p.contrast, p.vibrant, true});
    const float dotR = 18.0f * s;
    const float dx = r.right() - (active ? 140.0f : 84.0f);
    ctx.canvas->fillCircle(dx, r.cy(), dotR, preview.primary);
    ctx.canvas->fillCircle(dx + dotR * 2.2f, r.cy(), dotR, preview.surface);
    ctx.canvas->strokeRoundRect(Rect(dx + dotR * 1.2f, r.cy() - dotR, dotR * 2.0f, dotR * 2.0f),
                                dotR, 1.5f * s, c.outlineVariant);
    if (active) {
        ui::drawText(ctx, "ACTIVE", ctx.type().labelSmall,
                     Rect(r.right() - 44.0f * s, r.y, 40.0f * s, r.h), c.primary, ui::Align::Left,
                     ui::VAlign::Middle);
    }

    ctx.focus->add(r, kThemeBase + index, true, 12.0f * s);
    y += itemH + 8.0f * s;
}

void SettingsScreen::applyThemePreset(App& app, int index) {
    AppConfig& cfg = app.config();
    const ThemePreset& p = kThemePresets[index];
    cfg.seedRgb = p.seed;
    cfg.dark = p.dark;
    cfg.vibrant = p.vibrant;
    cfg.contrast = p.contrast;
    apply(app, true);
    app.toast(std::string("Theme: ") + p.name);
}

std::vector<Color> SettingsScreen::seedSwatches() const {
    return {Color::fromHex(0x6750A4), Color::fromHex(0x006C4D), Color::fromHex(0x00497F),
            Color::fromHex(0x7A3045), Color::fromHex(0x7C5800), Color::fromHex(0x4C3B6F)};
}

void SettingsScreen::apply(App& app, bool animate) {
    app.applyConfig(animate);
    app.saveConfig();
}

void SettingsScreen::reset(App& app) {
    AppConfig cfg;
    app.config() = cfg;
    apply(app, true);
    app.toast("Restored defaults");
}

void SettingsScreen::draw(App& app) {
    ui::Ctx& ctx = app.ctx();
    const Scheme& c = app.colors();
    const Rect area = app.contentRect();
    const float s = ctx.displayScale;
    AppConfig& cfg = app.config();

    const float contentH = area.h;
    const Rect list(area.x, area.y, area.w, contentH);
    const float itemH = kItemHeight * s;
    const float sectH = kSection * s;

    ctx.canvas->pushClip(list);

    float y = list.y - scroll_.offset;

    // --- themes -------------------------------------------------------------
    ui::drawText(ctx, "Themes", ctx.type().labelLarge,
                 Rect(list.x, y, list.w, sectH), c.onSurfaceVariant, ui::Align::Left,
                 ui::VAlign::Middle);
    y += sectH;

    for (int i = 0; i < kThemePresetCount && i < kThemeMax - kThemeBase; ++i) {
        drawThemePreset(app, list, y, itemH, i);
    }

    // --- appearance ---------------------------------------------------------
    ui::drawText(ctx, "Appearance", ctx.type().labelLarge,
                 Rect(list.x, y, list.w, sectH), c.onSurfaceVariant, ui::Align::Left,
                 ui::VAlign::Middle);
    y += sectH;

    // dark mode
    {
        const Rect r(list.x, y, list.w, itemH);
        ui::drawText(ctx, "Dark theme", ctx.type().titleMedium,
                     Rect(r.x + 4.0f * s, r.y, r.w * 0.5f, r.h * 0.5f), c.onSurface, ui::Align::Left,
                     ui::VAlign::Middle);
        ui::drawText(ctx, cfg.dark ? "On" : "Off", ctx.type().bodySmall,
                     Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w * 0.5f, r.h * 0.5f),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
        ui::switchControl(ctx, Rect(r.right() - 72.0f * s, r.cy() - 18.0f * s, 56.0f * s, 36.0f * s),
                          cfg.dark ? 1.0f : 0.0f, true);
        ctx.focus->add(r, kToggleDark, true, 12.0f * s);
        y += itemH + 8.0f * s;
    }

    // vibrant
    {
        const Rect r(list.x, y, list.w, itemH);
        ui::drawText(ctx, "Vibrant palette", ctx.type().titleMedium,
                     Rect(r.x + 4.0f * s, r.y, r.w * 0.5f, r.h * 0.5f), c.onSurface, ui::Align::Left,
                     ui::VAlign::Middle);
        ui::drawText(ctx, "Boost colour chroma", ctx.type().bodySmall,
                     Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w * 0.5f, r.h * 0.5f),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
        ui::switchControl(ctx, Rect(r.right() - 72.0f * s, r.cy() - 18.0f * s, 56.0f * s, 36.0f * s),
                          cfg.vibrant ? 1.0f : 0.0f, true);
        ctx.focus->add(r, kToggleVibrant, true, 12.0f * s);
        y += itemH + 8.0f * s;
    }

    // seed swatches
    {
        ui::drawText(ctx, "Seed colour", ctx.type().labelLarge,
                     Rect(list.x, y, list.w, sectH), c.onSurfaceVariant, ui::Align::Left,
                     ui::VAlign::Middle);
        y += sectH;

        const auto sw = seedSwatches();
        float sx = list.x + 4.0f * s;
        for (size_t i = 0; i < sw.size() && i < 6; ++i) {
            const Rect sr(sx, y + 4.0f * s, 48.0f * s, 48.0f * s);
            const Color col = sw[i];
            ctx.canvas->fillCircle(sr.cx(), sr.cy(), 20.0f * s, col);
            if (approxColor(Color::fromHex(cfg.seedRgb), col)) {
                ctx.canvas->strokeArc(sr.cx(), sr.cy(), 22.0f * s, 3.0f * s, 0.0f, 360.0f, c.primary);
            }
            ctx.focus->add(sr, kSeedBase + static_cast<int>(i), true, 24.0f * s);
            sx += 56.0f * s;
        }
        y += 64.0f * s + 12.0f * s;
    }

    // --- tuning -------------------------------------------------------------
    ui::drawText(ctx, "Tuning", ctx.type().labelLarge,
                 Rect(list.x, y, list.w, sectH), c.onSurfaceVariant, ui::Align::Left,
                 ui::VAlign::Middle);
    y += sectH;

    const auto sliderRow = [&](const std::string& label, const std::string& sub, float value,
                               int id) {
        const Rect r(list.x, y, list.w, itemH);
        ui::drawText(ctx, label, ctx.type().titleMedium,
                     Rect(r.x + 4.0f * s, r.y, r.w * 0.6f, r.h * 0.5f), c.onSurface, ui::Align::Left,
                     ui::VAlign::Middle);
        ui::drawText(ctx, sub, ctx.type().bodySmall,
                     Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w * 0.6f, r.h * 0.5f),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
        ui::sliderControl(ctx, Rect(r.x + r.w * 0.5f, r.cy() - 10.0f * s, r.w * 0.45f, 20.0f * s),
                          value, true, false);
        ctx.focus->add(r, id, true, 12.0f * s);
        y += itemH + 8.0f * s;
    };

    sliderRow("Contrast", "Colour contrast (-0.3 to +0.3)", clampf(cfg.contrast, -0.3f, 0.3f),
              kSliderContrast);
    sliderRow("Type scale", "Text size (0.9x - 1.2x)", clampf(cfg.typeScale, 0.9f, 1.2f),
              kSliderType);
    sliderRow("Motion scale", "Animation speed (0x - 1.5x)", clampf(cfg.motionScale, 0.0f, 1.5f),
              kSliderMotion);

    // --- about --------------------------------------------------------------
    ui::drawText(ctx, "About", ctx.type().labelLarge,
                 Rect(list.x, y, list.w, sectH), c.onSurfaceVariant, ui::Align::Left,
                 ui::VAlign::Middle);
    y += sectH;

    {
        const Rect r(list.x, y, list.w, itemH);
        ui::drawText(ctx, "RAW UI", ctx.type().titleMedium,
                     Rect(r.x + 4.0f * s, r.y, r.w, r.h * 0.5f), c.onSurface, ui::Align::Left,
                     ui::VAlign::Middle);
        ui::drawText(ctx, "Title ID RAWUI0001 · Homebrew shell", ctx.type().bodySmall,
                     Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w, r.h * 0.5f),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
        y += itemH;
    }

    {
        const Rect r(list.x, y, list.w, itemH);
        ui::drawText(ctx, "Storage", ctx.type().titleMedium,
                     Rect(r.x + 4.0f * s, r.y, r.w, r.h * 0.5f), c.onSurface, ui::Align::Left,
                     ui::VAlign::Middle);
        const fs::SpaceInfo sp = app.dataSpace();
        std::string st = sp.valid ? humanSize(sp.free) + " free of " + humanSize(sp.total) + " on /data"
                                  : "/data unavailable";
        ui::drawText(ctx, st, ctx.type().bodySmall,
                     Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w, r.h * 0.5f),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
        y += itemH + 8.0f * s;
    }

    {
        const Rect r(list.x, y, list.w, 64.0f * s);
        const float prox = ui::focusProximity(ctx, r, 90.0f * s);
        ui::button(ctx, r, "Restore defaults", icons::kRefresh, ui::ButtonStyle::Outlined, prox,
                   false, true);
        ctx.focus->add(r, kReset, true, 14.0f * s);
        y += 72.0f * s;
    }

    ctx.canvas->popClip();

    // Keep the focused row on screen and clamp the offset.
    const Rect fr = ctx.focus->currentRect();
    if (fr.h > 0.0f) {
        if (fr.y < list.y) {
            scroll_.target -= list.y - fr.y;
        } else if (fr.bottom() > list.bottom()) {
            scroll_.target += fr.bottom() - list.bottom();
        }
    }
    const float maxOff = maxf(0.0f, (y + scroll_.offset) - list.y - list.h);
    scroll_.target = clampf(scroll_.target, 0.0f, maxOff);
    scroll_.update(ctx.dt);
}

bool SettingsScreen::onAdjust(App& app, int focusId, int delta) {
    AppConfig& cfg = app.config();
    if (focusId == kSliderContrast) {
        cfg.contrast = clampf(cfg.contrast + delta * 0.05f, -0.3f, 0.3f);
        apply(app, true);
        return true;
    }
    if (focusId == kSliderType) {
        cfg.typeScale = clampf(cfg.typeScale + delta * 0.02f, 0.9f, 1.2f);
        apply(app, true);
        return true;
    }
    if (focusId == kSliderMotion) {
        cfg.motionScale = clampf(cfg.motionScale + delta * 0.05f, 0.0f, 1.5f);
        apply(app, true);
        return true;
    }
    return false;
}

void SettingsScreen::onConfirm(App& app, int focusId) {
    AppConfig& cfg = app.config();
    if (focusId == kToggleDark) {
        cfg.dark = !cfg.dark;
        apply(app, true);
        return;
    }
    if (focusId == kToggleVibrant) {
        cfg.vibrant = !cfg.vibrant;
        apply(app, true);
        return;
    }
    if (focusId >= kThemeBase && focusId < kThemeMax) {
        const int idx = focusId - kThemeBase;
        if (idx >= 0 && idx < kThemePresetCount) applyThemePreset(app, idx);
        return;
    }
    if (focusId >= kSeedBase && focusId < kSliderContrast) {
        const int idx = focusId - kSeedBase;
        const auto sw = seedSwatches();
        if (idx >= 0 && idx < static_cast<int>(sw.size())) {
            cfg.seedRgb = seedRgbToHex(sw[static_cast<size_t>(idx)]);
            apply(app, true);
            app.toast("Seed colour updated");
        }
        return;
    }
    if (focusId == kReset) {
        reset(app);
        return;
    }
}

}  // namespace rawui
