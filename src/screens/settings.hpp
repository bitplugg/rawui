// ============================================================================
//  RAW UI - settings screen.
//
//  Long single-column list: appearance first (what most people came for), then
//  the theme presets, tuning sliders, external-storage round-trips and the
//  about block.  Focus ids are a plain enum because the pad only ever needs
//  stable, comparable values.
// ============================================================================

#pragma once

#include <string>
#include <vector>

#include "app/app.hpp"
#include "app/screen.hpp"
#include "gfx/icons.hpp"
#include "platform/fs.hpp"
#include "platform/themeio.hpp"
#include "ui/scroll.hpp"

namespace rawui {

class SettingsScreen final : public Screen {
public:
    const char* title() const override { return tr(Str::kSettings); }
    uint32_t icon() const override { return icons::kSettings; }
    std::string subtitle(App& app) override;

    void onEnter(App& app) override;
    void draw(App& app) override;
    bool onAdjust(App& app, int focusId, int delta) override;
    void onConfirm(App& app, int focusId) override;

    std::vector<ui::Hint> hints() const override {
        return {{ui::PadGlyph::Cross, tr(Str::kHintToggle)},
                {ui::PadGlyph::Dpad, tr(Str::kHintAdjust)},
                {ui::PadGlyph::L1, tr(Str::kHintScreens)}};
    }

private:
    enum Ids {
        kToggleDark = 1,
        kToggleVibrant = 2,
        kToggleLang = 3,
        kToggleNight = 4,
        kSeedBase = 20,
        kSeedMax = 26,
        kSliderContrast = 50,
        kSliderType = 51,
        kSliderMotion = 52,
        kSliderNightStart = 53,
        kSliderNightEnd = 54,
        kReset = 80,
        kBackup = 81,
        kRestore = 82,
        kExportTheme = 83,
        kScanThemes = 84,
        kThemeBase = 100,
        kThemeMax = kThemeBase + 16,
        // Imported theme files; capped so a stick full of ini files cannot
        // push the reset button off the end of the id space.
        kUsbThemeBase = 200,
        kUsbThemeMax = kUsbThemeBase + 24,
    };

    void drawThemePreset(App& app, const Rect& list, float& y, float itemH, int index);
    void drawToggleRow(App& app, const Rect& list, float& y, float itemH, int id,
                       const std::string& label, const std::string& sub, bool on);
    void drawActionButton(App& app, const Rect& list, float& y, int id, const std::string& label,
                          const std::string& sub, uint32_t icon);

    void applyThemePreset(App& app, int index);
    void apply(App& app, bool animate);
    void reset(App& app);

    std::vector<Color> seedSwatches() const;

    ListScroll scroll_;

    // Theme files are discovered by walking the USB volumes, which is far too
    // slow for the per-frame path, so the list is taken once per visit.
    std::vector<ThemeFile> themeFiles_;
};

}  // namespace rawui
