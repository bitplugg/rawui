// ============================================================================
//  RAW UI - dashboard.
//
//  The landing screen: three status cards that double as shortcuts, plus the
//  launcher grid for the rest of the app.
// ============================================================================
#pragma once

#include "app/app.hpp"
#include "app/screen.hpp"
#include "gfx/icons.hpp"

namespace rawui {

class DashboardScreen : public Screen {
public:
    const char* title() const override { return "RAW UI"; }
    uint32_t icon() const override { return icons::kDashboard; }
    std::string subtitle(App& app) override;

    void onEnter(App& app) override;
    void draw(App& app) override;
    void onConfirm(App& app, int focusId) override;

    std::vector<ui::Hint> hints() const override {
        return {{ui::PadGlyph::Cross, "Open"},{ui::PadGlyph::Dpad, "Navigate"},
                {ui::PadGlyph::L1, "Screens"}};
    }

private:
    // Focus ids.
    enum Id {
        kCardHen = 0,
        kCardStorage = 1,
        kCardCheats = 2,
        kTileBase = 10,
    };

    struct Tile {
        const char* label;
        const char* supporting;
        uint32_t icon;
        int screenIndex;
    };

    static const Tile* tiles(int& count);
};

}  // namespace rawui
