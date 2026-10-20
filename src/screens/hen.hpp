// ============================================================================
//  RAW UI - GoldHEN status.
//
//  Reports what can honestly be determined from userland: whether the payload
//  directory exists, which of its bundled servers answer, and how many cheat
//  and patch files are on disk.  No guessing about "jailbroken" states.
// ============================================================================
#pragma once

#include "app/app.hpp"
#include "app/screen.hpp"
#include "gfx/icons.hpp"

namespace rawui {

class HenScreen : public Screen {
public:
    const char* title() const override { return tr(Str::kHen); }
    uint32_t icon() const override { return icons::kSecurity; }
    std::string subtitle(App& app) override;

    uint32_t fabIcon() const override { return icons::kRefresh; }
    const char* fabLabel() const override { return tr(Str::kFabProbe); }
    void onFab(App& app) override;

    void onEnter(App& app) override;
    void draw(App& app) override;
    void onConfirm(App& app, int focusId) override;

    std::vector<ui::Hint> hints() const override {
        return {{ui::PadGlyph::Cross, tr(Str::kHintOpen)},
                {ui::PadGlyph::Triangle, tr(Str::kHintProbe)},
                {ui::PadGlyph::L1, tr(Str::kHintScreens)}};
    }

private:
    enum Id {
        kSummary = 1,
        kServerBase = 20,
        kCheatsLink = 60,
    };

    float spin_ = 0.0f;
    float lastProbe_ = -100.0f;
};

}  // namespace rawui
