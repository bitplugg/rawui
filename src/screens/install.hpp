// ============================================================================
//  RAW UI - package installation.
//
//  Scans the mounted volumes for .pkg files and hands them to
//  libSceAppInstUtil.  Progress is polled from the installer service; when the
//  file name does not carry a content id we fall back to watching for the title
//  to appear, because AppInstUtil needs the content id to report a percentage.
// ============================================================================
#pragma once

#include <string>
#include <vector>

#include "app/app.hpp"
#include "app/screen.hpp"
#include "gfx/icons.hpp"
#include "platform/pkg.hpp"
#include "ui/scroll.hpp"

namespace rawui {

class InstallScreen : public Screen {
public:
    const char* title() const override { return "Install packages"; }
    uint32_t icon() const override { return icons::kInstall; }
    std::string subtitle(App& app) override;

    uint32_t fabIcon() const override { return icons::kRefresh; }
    const char* fabLabel() const override { return "Rescan"; }
    void onFab(App& app) override { scan(app); }

    void onEnter(App& app) override;
    void update(App& app, float dt) override;
    void draw(App& app) override;
    void onConfirm(App& app, int focusId) override;
    void onAlt(App& app, int focusId) override;

    std::vector<ui::Hint> hints() const override {
        return {{ui::PadGlyph::Cross, "Install"},
                {ui::PadGlyph::Square, "Details"},
                {ui::PadGlyph::Circle, "Back"},
                {ui::PadGlyph::L1, "Screens"}};
    }

private:
    enum Id {
        kRefresh = 1,
        kFirstRow = 100,
    };

    void scan(App& app);
    void beginInstall(App& app, int index);
    void drawProgress(App& app, const Rect& area);
    std::string stateText() const;

    std::vector<PkgCandidate> pkgs_;
    bool scanned_ = false;

    int focusIndex_ = 0;
    ListScroll scroll_;

    // Active installation.
    bool installing_ = false;
    bool installFailed_ = false;
    std::string installName_;
    std::string installTitleId_;
    std::string installContentId_;
    InstallProgress progress_;
    float pollTimer_ = 0.0f;
    float elapsed_ = 0.0f;
};

}  // namespace rawui
