// ============================================================================
//  RAW UI - screen interface.
//
//  A screen owns its state and paints a content area that the shell has already
//  laid out.  Focus items are registered during draw() (see FocusManager) and
//  activate() receives back the id of whatever was selected.
// ============================================================================
#pragma once

#include <string>

#include "core/i18n.hpp"
#include "core/math.hpp"
#include "ui/ui.hpp"

namespace rawui {

class App;

class Screen {
public:
    virtual ~Screen() = default;

    // Navigation rail entry.
    virtual const char* title() const = 0;
    virtual uint32_t icon() const = 0;

    // Header line under the title.
    virtual std::string subtitle(App& app) { return {}; }

    // Extension FAB.  Returning 0 hides it.
    virtual uint32_t fabIcon() const { return 0; }
    virtual const char* fabLabel() const { return ""; }
    virtual void onFab(App& app) {}

    virtual void onEnter(App& app) {}
    virtual void onLeave(App& app) {}
    virtual void update(App& app, float dt) {}

    // Paints inside app.contentRect(); registers focus items on app.focus().
    virtual void draw(App& app) = 0;

    // A/Circle/Square/Triangle.
    virtual void onConfirm(App& app, int focusId) {}
    virtual void onAlt(App& app, int focusId) {}

    // Left/Right on the focused item.  Return true to consume the press (a
    // slider or segmented control does this so it can be adjusted in place).
    virtual bool onAdjust(App& app, int focusId, int delta) { return false; }

    // Return true when the screen consumed Circle itself.
    virtual bool onBack(App& app) { return false; }

    // Hints shown along the bottom of the shell.
    virtual std::vector<ui::Hint> hints() const {
        return {{ui::PadGlyph::Cross, tr(Str::kHintSelect)}, {ui::PadGlyph::Circle, tr(Str::kHintBack)}};
    }
};

}  // namespace rawui
