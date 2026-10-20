// ============================================================================
//  RAW UI - file manager.
//
//  Browsing is deliberately fenced inside the writable mount points: the app is
//  a homebrew shell, and letting someone wander into /system from a game pad is
//  a good way to end up with a brick.
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

class FilesScreen : public Screen {
public:
    const char* title() const override { return tr(Str::kFiles); }
    uint32_t icon() const override { return icons::kFolder; }
    std::string subtitle(App& app) override;

    uint32_t fabIcon() const override { return icons::kArrowUp; }
    const char* fabLabel() const override { return tr(Str::kHintUp); }
    void onFab(App& app) override { goUp(app); }

    void onEnter(App& app) override;
    void draw(App& app) override;
    void onConfirm(App& app, int focusId) override;
    void onAlt(App& app, int focusId) override;
    bool onBack(App& app) override;

    std::vector<ui::Hint> hints() const override {
        return {{ui::PadGlyph::Cross, tr(Str::kHintOpen)},
                {ui::PadGlyph::Square, tr(Str::kHintDelete)},
                {ui::PadGlyph::Triangle, tr(Str::kHintUp)},
                {ui::PadGlyph::Circle, tr(Str::kHintBack)}};
    }

private:
    enum Id {
        kChipBase = 10,
        kRowBase = 100,
    };

    void refresh(App& app);
    void enterDirectory(App& app, const std::string& path);
    bool goUp(App& app);
    bool atMountRoot() const;

    std::string cwd_ = "/data";
    std::vector<FileEntry> entries_;
    std::vector<std::string> mounts_;
    bool ok_ = true;
    int focusIndex_ = 0;
    ListScroll scroll_;
};

}  // namespace rawui
