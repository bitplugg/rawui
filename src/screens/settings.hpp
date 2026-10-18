// ============================================================================
//  RAW UI - settings.
//
//  A compact set of preferences: appearance (dark mode, seed colour, vibrance,
//  contrast), typography (type scale), motion and a small "about" block.  All
//  changes are applied live with a spring animation and written out straight
//  after the user changes them.
// ============================================================================
#pragma once

#include <string>

#include "app/app.hpp"
#include "app/screen.hpp"
#include "gfx/icons.hpp"
#include "platform/fs.hpp"
#include "ui/scroll.hpp"

namespace rawui {

class SettingsScreen : public Screen {
public:
    const char* title() const override { return "Settings"; }
    uint32_t icon() const override { return icons::kSettings; }
    std::string subtitle(App& app) override;

    void onEnter(App& app) override;
    void draw(App& app) override;
    bool onAdjust(App& app, int focusId, int delta) override;
    void onConfirm(App& app, int focusId) override;

    std::vector<ui::Hint> hints() const override {
        return {{ui::PadGlyph::Cross, "Toggle"},
                {ui::PadGlyph::Dpad, "Navigate/Adjust"},
                {ui::PadGlyph::L1, "Screens"}};
    }

private:
    enum Id {
        kToggleDark = 1,
        kToggleVibrant = 2,
        kSeedBase = 20,
        kSliderContrast = 50,
        kSliderType = 51,
        kSliderMotion = 52,
        kReset = 80,
        kThemeBase = 100,
        kThemeMax = kThemeBase + 16,
    };

    std::vector<Color> seedSwatches() const;
    void apply(App& app, bool animate);
    void reset(App& app);
    void drawThemePreset(App& app, const Rect& list, float& y, float itemH, int index);
    void applyThemePreset(App& app, int index);

    ListScroll scroll_;
};

}  // namespace rawui
