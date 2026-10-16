// ============================================================================
//  RAW UI - focus management.
//
//  Focus is computed geometrically instead of being wired up by hand: every
//  frame a screen registers the rectangles of its interactive items, and a
//  directional press picks the best candidate in that direction.  That means
//  moving a card in a layout automatically fixes the navigation.
//
//  A single animated rectangle follows the focused item, which is what produces
//  the sliding highlight M3 Expressive UIs are known for.
// ============================================================================
#pragma once

#include <vector>

#include "core/math.hpp"
#include "md3/motion.hpp"

namespace rawui {

class FocusManager {
public:
    // --- frame lifecycle ----------------------------------------------------
    // Two lists are kept: `items_` holds the geometry the previous frame used
    // (so drawing can read the current selection while the new list is being
    // built), and `add()` appends to a scratch list that `end()` promotes.
    void begin() {
        build_.clear();
    }

    // `id` is whatever the screen wants back from currentId().
    void add(const Rect& rect, int id, bool enabled = true, float radius = 28.0f) {
        build_.push_back({rect, id, enabled, radius});
    }

    // Called after the last add(); resolves the selection when the item set
    // changed (screen switch, list refresh, item removed...).
    void end();

    // Preferred item to fall back to when the current selection disappears
    // (i.e. after switching screens).  -1 disables the preference.
    void setPreferred(int id) { preferred_ = id; }

    // --- queries ------------------------------------------------------------
    bool empty() const { return items_.empty(); }
    int count() const { return static_cast<int>(items_.size()); }
    int currentIndex() const { return current_; }
    int currentId() const;

    Rect currentRect() const;
    Rect itemRect(int index) const;
    bool itemEnabled(int index) const;

    // The highlight that slides between items.
    Rect animatedRect() const { return animated_; }
    float highlightAlpha() const;
    float currentRadius() const;
    float animatedRadius() const { return radiusSpring_.value(); }

    // --- control ------------------------------------------------------------
    // Returns true when the selection actually moved.
    bool navigate(int dx, int dy);
    void selectById(int id);
    void selectIndex(int index);

    // Picks `preferredId` if it is present, otherwise the first enabled item.
    void resetTo(int preferredId);

    // Forgets the selection and the animation state (used when the whole
    // screen set is rebuilt).
    void clear() {
        items_.clear();
        build_.clear();
        current_ = -1;
        preferred_ = -1;
        animInitialised_ = false;
    }

    void update(float dt, float timeScale = 1.0f);

private:
    struct Item {
        Rect rect;
        int id = -1;
        bool enabled = true;
        float radius = 28.0f;
    };

    int indexOfId(int id) const;

    std::vector<Item> items_;
    std::vector<Item> build_;
    int current_ = -1;
    int preferred_ = -1;

    Rect animated_;
    bool animInitialised_ = false;
    Spring sx_{springs::kSpatialFast};
    Spring sy_{springs::kSpatialFast};
    Spring sw_{springs::kSpatialFast};
    Spring sh_{springs::kSpatialFast};
    Spring radiusSpring_{springs::kSpatialFast};
};

}  // namespace rawui
