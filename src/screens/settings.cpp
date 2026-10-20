#include "screens/settings.hpp"

#include <cstdio>
#include <ctime>
#include <string>

#include "gfx/icons.hpp"

// Supplied by the Makefile (-DRAWUI_TITLE_ID / -DRAWUI_VERSION); the fallbacks
// keep a stray host-side compile from failing on an undefined macro.
#ifndef RAWUI_TITLE_ID
#define RAWUI_TITLE_ID "RAWU00001"
#endif
#ifndef RAWUI_VERSION
#define RAWUI_VERSION "dev"
#endif

namespace rawui {
namespace {

constexpr float kItemHeight = 72.0f;
constexpr float kSection = 40.0f;
constexpr int kMaxMinute = 23 * 60 + 59;

uint32_t seedRgbToHex(const Color& c) {
    return (static_cast<uint32_t>(c.r) << 16) | (static_cast<uint32_t>(c.g) << 8) | c.b;
}

bool approxColor(const Color& a, const Color& b) {
    return std::abs(a.r - b.r) < 10 && std::abs(a.g - b.g) < 10 && std::abs(a.b - b.b) < 10;
}

// "22:00" - the schedule rows are minutes-since-midnight and are always shown
// as a clock time, in both languages.
std::string hhmm(int minutes) {
    const int clamped = clampi(minutes, 0, kMaxMinute);
    char buf[8];
    std::snprintf(buf, sizeof(buf), "%02d:%02d", clamped / 60, clamped % 60);
    return buf;
}

}  // namespace

std::string SettingsScreen::subtitle(App& app) {
    (void)app;
    return tr(Str::kSettingsSubtitle);
}

void SettingsScreen::onEnter(App& app) {
    app.focus().setPreferred(kToggleDark);
    scroll_.reset();

    // Walking the USB volumes is far too slow for the per-frame path, so the
    // theme files are discovered once per visit (and on demand via "Import").
    themeFiles_ = scanThemeFiles();
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
    const float dx = r.right() - (active ? 170.0f : 84.0f);
    ctx.canvas->fillCircle(dx, r.cy(), dotR, preview.primary);
    ctx.canvas->fillCircle(dx + dotR * 2.2f, r.cy(), dotR, preview.surface);
    ctx.canvas->strokeRoundRect(Rect(dx + dotR * 1.2f, r.cy() - dotR, dotR * 2.0f, dotR * 2.0f),
                                dotR, 1.5f * s, c.outlineVariant);
    if (active) {
        // Right-aligned and generously sized: "АКТИВНО" is longer than
        // "ACTIVE" and must not collide with the preview dots.
        ui::drawText(ctx, tr(Str::kActive), ctx.type().labelSmall,
                     Rect(r.right() - 84.0f * s, r.y, 80.0f * s, r.h), c.primary, ui::Align::Right,
                     ui::VAlign::Middle);
    }

    ctx.focus->add(r, kThemeBase + index, true, 12.0f * s);
    y += itemH + 8.0f * s;
}

void SettingsScreen::drawToggleRow(App& app, const Rect& list, float& y, float itemH, int id,
                                   const std::string& label, const std::string& sub, bool on) {
    ui::Ctx& ctx = app.ctx();
    const Scheme& c = app.colors();
    const float s = ctx.displayScale;

    const Rect r(list.x, y, list.w, itemH);
    ui::drawText(ctx, label, ctx.type().titleMedium,
                 Rect(r.x + 4.0f * s, r.y, r.w * 0.5f, r.h * 0.5f), c.onSurface, ui::Align::Left,
                 ui::VAlign::Middle);
    ui::drawText(ctx, sub, ctx.type().bodySmall,
                 Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w * 0.5f, r.h * 0.5f),
                 c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
    ui::switchControl(ctx, Rect(r.right() - 72.0f * s, r.cy() - 18.0f * s, 56.0f * s, 36.0f * s),
                      on ? 1.0f : 0.0f, true);
    ctx.focus->add(r, id, true, 12.0f * s);
    y += itemH + 8.0f * s;
}

void SettingsScreen::drawActionButton(App& app, const Rect& list, float& y, int id,
                                      const std::string& label, const std::string& sub,
                                      uint32_t icon) {
    ui::Ctx& ctx = app.ctx();
    const Scheme& c = app.colors();
    const float s = ctx.displayScale;

    const float rowH = kItemHeight * s;
    const Rect r(list.x, y, list.w, rowH);
    const float prox = ui::focusProximity(ctx, r, 90.0f * s);
    ui::button(ctx, Rect(r.x, r.y, r.w, 44.0f * s), label, icon, ui::ButtonStyle::Tonal, prox,
               false, true);

    // The supporting line sits below the button rather than beside it, so the
    // button keeps a comfortable target size whatever the label length does to
    // it in another language.
    ui::drawText(ctx, sub, ctx.type().bodySmall,
                 Rect(r.x + 4.0f * s, r.y + 48.0f * s, r.w, 20.0f * s), c.onSurfaceVariant,
                 ui::Align::Left, ui::VAlign::Middle);

    ctx.focus->add(r, id, true, 14.0f * s);
    y += rowH + 12.0f * s;
}

void SettingsScreen::applyThemePreset(App& app, int index) {
    AppConfig& cfg = app.config();
    const ThemePreset& p = kThemePresets[index];
    cfg.seedRgb = p.seed;
    cfg.dark = p.dark;
    cfg.vibrant = p.vibrant;
    cfg.contrast = p.contrast;
    apply(app, true);
    app.toast(std::string(tr(Str::kThemeApplied)) + p.name);
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
    // Two fields survive a reset on purpose:
    //  * path  - without it the next saveConfig() has nowhere to write, and the
    //             settings silently stop persisting for the rest of the
    //             session (the original reset hit exactly that).
    //  * lang  - dropping someone back into English in the middle of the screen
    //            they were resetting is a trap, not a default.
    // lastScreen comes along too so a reset does not teleport you to the
    // dashboard on the next launch.
    const AppConfig& live = app.config();
    const std::string keepPath = live.path;
    const Lang keepLang = live.lang;
    const int keepScreen = live.lastScreen;

    AppConfig cfg;
    cfg.path = keepPath;
    cfg.lang = keepLang;
    cfg.lastScreen = keepScreen;
    app.config() = cfg;
    apply(app, true);
    app.toast(tr(Str::kDefaultsRestored));
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

    float y = list.y - scroll_.offset;

    const auto section = [&](const char* title) {
        ui::drawText(ctx, title, ctx.type().labelLarge, Rect(list.x, y, list.w, sectH),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
        y += sectH;
    };

    ctx.canvas->pushClip(list);

    // --- themes -------------------------------------------------------------
    section(tr(Str::kThemes));

    for (int i = 0; i < kThemePresetCount && i < kThemeMax - kThemeBase; ++i) {
        drawThemePreset(app, list, y, itemH, i);
    }

    // --- appearance ---------------------------------------------------------
    section(tr(Str::kAppearance));

    drawToggleRow(app, list, y, itemH, kToggleDark, tr(Str::kDark),
                  tr(cfg.dark ? Str::kOn : Str::kOff), cfg.dark);
    drawToggleRow(app, list, y, itemH, kToggleVibrant, tr(Str::kVibrant), tr(Str::kChromaBoost),
                  cfg.vibrant);

    // seed swatches
    {
        ui::drawText(ctx, tr(Str::kSeedColour), ctx.type().labelLarge,
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

    // --- general ------------------------------------------------------------
    section(tr(Str::kGeneral));

    drawToggleRow(app, list, y, itemH, kToggleLang, tr(Str::kLanguage),
                  langName(cfg.lang == Lang::English ? Lang::Russian : Lang::English),
                  cfg.lang == Lang::Russian);

    // Night mode: a toggle plus the two ends of the window.  Left/right moves
    // in half-hour steps, which is about as fine as a schedule row can be
    // driven with a d-pad.
    //
    // The supporting line changes with the state so it always says something
    // the two slider rows below do not already say: with the feature off you
    // want to see the window it would use, with it on you want to know whether
    // it is in force right now.
    {
        const bool activeNow = nightWindowActive(cfg, std::time(nullptr));
        const std::string window =
            std::string(tr(Str::kNightFrom)) + " " + hhmm(cfg.nightStart) + " " +
            tr(Str::kNightTo) + " " + hhmm(cfg.nightEnd);
        const std::string sub = !cfg.nightMode    ? window
                                : activeNow       ? tr(Str::kNightNow)
                                                  : tr(Str::kNightHint);
        drawToggleRow(app, list, y, itemH, kToggleNight, tr(Str::kNight), sub, cfg.nightMode);
    }

    if (cfg.nightMode) {
        const auto timeRow = [&](int id, const char* label, int minutes) {
            const Rect r(list.x, y, list.w, itemH);
            ui::drawText(ctx, label, ctx.type().titleMedium,
                         Rect(r.x + 4.0f * s, r.y, r.w * 0.6f, r.h * 0.5f), c.onSurface,
                         ui::Align::Left, ui::VAlign::Middle);
            ui::drawText(ctx, hhmm(minutes), ctx.type().bodySmall,
                         Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w * 0.6f, r.h * 0.5f),
                         c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
            ui::sliderControl(
                ctx, Rect(r.x + r.w * 0.5f, r.cy() - 10.0f * s, r.w * 0.45f, 20.0f * s),
                static_cast<float>(clampi(minutes, 0, kMaxMinute)) / static_cast<float>(kMaxMinute),
                true, false);
            ctx.focus->add(r, id, true, 12.0f * s);
            y += itemH + 8.0f * s;
        };
        timeRow(kSliderNightStart, tr(Str::kNightFrom), cfg.nightStart);
        timeRow(kSliderNightEnd, tr(Str::kNightTo), cfg.nightEnd);
    }

    // --- tuning -------------------------------------------------------------
    section(tr(Str::kTuning));

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

    sliderRow(tr(Str::kContrast), tr(Str::kContrastHint), clampf(cfg.contrast, -0.3f, 0.3f),
              kSliderContrast);
    sliderRow(tr(Str::kTypeScale), tr(Str::kTypeScaleHint), clampf(cfg.typeScale, 0.9f, 1.2f),
              kSliderType);
    sliderRow(tr(Str::kMotionScale), tr(Str::kMotionScaleHint), clampf(cfg.motionScale, 0.0f, 1.5f),
              kSliderMotion);

    // --- external storage ---------------------------------------------------
    section(tr(Str::kThemesUsb));

    drawActionButton(app, list, y, kExportTheme, tr(Str::kExportTheme), tr(Str::kExportThemeHint),
                     icons::kSave);
    drawActionButton(app, list, y, kScanThemes, tr(Str::kImportThemes), tr(Str::kImportThemesHint),
                     icons::kRefresh);

    if (themeFiles_.empty()) {
        // Not focusable: there is nothing to pick, and the two rows above say
        // where to put the files.
        ui::drawText(ctx, tr(Str::kNothingToImport), ctx.type().bodySmall,
                     Rect(list.x + 4.0f * s, y, list.w, itemH * 0.5f), c.onSurfaceVariant,
                     ui::Align::Left, ui::VAlign::Middle);
        y += itemH * 0.5f + 8.0f * s;
    } else {
        for (size_t i = 0; i < themeFiles_.size() && i < kUsbThemeMax - kUsbThemeBase; ++i) {
            const ThemeFile& f = themeFiles_[i];
            const Rect r(list.x, y, list.w, itemH * 0.75f);
            ui::listItem(ctx, r, icons::kPalette, f.name, dirName(f.path), "", false,
                         ui::focusProximity(ctx, r, 90.0f * s));
            ctx.focus->add(r, kUsbThemeBase + static_cast<int>(i), true, 12.0f * s);
            y += itemH * 0.75f + 8.0f * s;
        }
    }

    section(tr(Str::kBackup));

    drawActionButton(app, list, y, kBackup, tr(Str::kBackupToUsb), tr(Str::kBackupToUsbHint),
                     icons::kSave);
    drawActionButton(app, list, y, kRestore, tr(Str::kRestoreFromUsb), tr(Str::kRestoreFromUsbHint),
                     icons::kUpload);

    // --- about --------------------------------------------------------------
    section(tr(Str::kAbout));

    {
        const Rect r(list.x, y, list.w, itemH);
        ui::drawText(ctx, "RAW UI", ctx.type().titleMedium,
                     Rect(r.x + 4.0f * s, r.y, r.w, r.h * 0.5f), c.onSurface, ui::Align::Left,
                     ui::VAlign::Middle);
        ui::drawText(ctx, trf(Str::kAboutLine, RAWUI_TITLE_ID, RAWUI_VERSION, tr(Str::kHomebrewShell)),
                     ctx.type().bodySmall,
                     Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w, r.h * 0.5f),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
        y += itemH;
    }

    {
        const Rect r(list.x, y, list.w, itemH);
        ui::drawText(ctx, tr(Str::kStorage), ctx.type().titleMedium,
                     Rect(r.x + 4.0f * s, r.y, r.w, r.h * 0.5f), c.onSurface, ui::Align::Left,
                     ui::VAlign::Middle);
        const fs::SpaceInfo sp = app.dataSpace();
        std::string st = sp.valid ? humanSize(sp.free) + tr(Str::kFreeOf) + humanSize(sp.total) +
                                        tr(Str::kOnData)
                                  : tr(Str::kDataUnavailable);
        ui::drawText(ctx, st, ctx.type().bodySmall,
                     Rect(r.x + 4.0f * s, r.y + r.h * 0.5f - 6.0f * s, r.w, r.h * 0.5f),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
        y += itemH + 8.0f * s;
    }

    drawActionButton(app, list, y, kReset, tr(Str::kRestoreDefaults), "", icons::kRefresh);

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
    if (focusId == kSliderNightStart) {
        cfg.nightStart = clampi(cfg.nightStart + delta * 30, 0, kMaxMinute);
        apply(app, false);  // no palette change, so no transition to animate
        return true;
    }
    if (focusId == kSliderNightEnd) {
        cfg.nightEnd = clampi(cfg.nightEnd + delta * 30, 0, kMaxMinute);
        apply(app, false);
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
    if (focusId == kToggleLang) {
        cfg.lang = cfg.lang == Lang::English ? Lang::Russian : Lang::English;
        // Language is applied by applyConfig(), so the whole shell - rail,
        // hints, other screens - switches in one go rather than only this one.
        apply(app, false);
        return;
    }
    if (focusId == kToggleNight) {
        cfg.nightMode = !cfg.nightMode;
        apply(app, true);
        return;
    }
    if (focusId >= kThemeBase && focusId < kThemeMax) {
        const int idx = focusId - kThemeBase;
        if (idx >= 0 && idx < kThemePresetCount) applyThemePreset(app, idx);
        return;
    }
    if (focusId >= kSeedBase && focusId < kSeedMax) {
        const int idx = focusId - kSeedBase;
        const auto sw = seedSwatches();
        if (idx >= 0 && idx < static_cast<int>(sw.size())) {
            cfg.seedRgb = seedRgbToHex(sw[static_cast<size_t>(idx)]);
            apply(app, true);
            app.toast(tr(Str::kSeedUpdated));
        }
        return;
    }
    if (focusId >= kUsbThemeBase && focusId < kUsbThemeMax) {
        const int idx = focusId - kUsbThemeBase;
        if (idx < 0 || idx >= static_cast<int>(themeFiles_.size())) return;

        const ThemeFile& file = themeFiles_[static_cast<size_t>(idx)];
        if (importThemeFile(file.path, cfg)) {
            apply(app, true);
            app.toast(std::string(tr(Str::kImportOk)) + file.name);
        } else {
            app.toast(tr(Str::kImportFail), true);
        }
        return;
    }
    if (focusId == kScanThemes) {
        themeFiles_ = scanThemeFiles();
        app.toast(themeFiles_.empty()
                      ? std::string(tr(Str::kNothingToImport))
                      : trf(Str::kThemesFound, static_cast<int>(themeFiles_.size())));
        return;
    }
    if (focusId == kExportTheme) {
        std::string path;
        app.toast(exportTheme(cfg, &path) ? tr(Str::kExportOk) : tr(Str::kExportFail));
        return;
    }
    if (focusId == kBackup) {
        std::string path;
        app.toast(configBackupUsb(cfg, &path) ? tr(Str::kBackupOk) : tr(Str::kBackupFail));
        return;
    }
    if (focusId == kRestore) {
        std::string path;
        if (configRestoreUsb(cfg, &path)) {
            apply(app, true);
            app.toast(tr(Str::kRestoreOk));
        } else {
            app.toast(tr(Str::kRestoreFail), true);
        }
        return;
    }
    if (focusId == kReset) {
        reset(app);
        return;
    }
}

}  // namespace rawui
