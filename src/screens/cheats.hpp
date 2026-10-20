// ============================================================================
//  RAW UI - cheats and patches.
//
//  GoldHEN reads `.json` cheat files from its cheat directory and applies patch
//  files from the patch directory.  Many tools rename cheats to
//  `*.json.disabled` to toggle them - the screen mirrors that convention.
// ============================================================================
#pragma once

#include <string>
#include <vector>

#include "app/app.hpp"
#include "app/screen.hpp"
#include "gfx/icons.hpp"
#include "platform/fs.hpp"
#include "ui/scroll.hpp"

namespace rawui {

class CheatsScreen : public Screen {
public:
    const char* title() const override { return tr(Str::kCheats); }
    uint32_t icon() const override { return icons::kExtension; }
    std::string subtitle(App& app) override;

    uint32_t fabIcon() const override { return icons::kRefresh; }
    const char* fabLabel() const override { return tr(Str::kHintRefresh); }
    void onFab(App& app) override { refresh(app); }

    void onEnter(App& app) override;
    void draw(App& app) override;
    void onConfirm(App& app, int focusId) override;
    void onAlt(App& app, int focusId) override;
    bool onAdjust(App& app, int focusId, int delta) override;

    std::vector<ui::Hint> hints() const override {
        return {{ui::PadGlyph::Cross, tr(Str::kHintToggle)},
                {ui::PadGlyph::Square, tr(Str::kHintDelete)},
                {ui::PadGlyph::Triangle, tr(Str::kHintRefresh)},
                {ui::PadGlyph::L1, tr(Str::kHintScreens)}};
    }

private:
    enum Id {
        kSegBase = 10,
        kRowBase = 100,
    };

    enum Mode { kCheats, kPatches };
    Mode mode_ = kCheats;

    void refresh(App& app);
    bool isDisabled(const std::string& name) const;
    std::string enabledPath(const std::string& path) const;
    std::string disabledPath(const std::string& path) const;

    std::vector<FileEntry> entries_;
    std::vector<std::string> sources_;
    ListScroll scroll_;
    int focusIndex_ = 0;
};

}  // namespace rawui
