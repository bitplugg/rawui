// ============================================================================
//  RAW UI - scrolling container used by the list screens.
//
//  Rows have a uniform height, so "keep the focused row visible" is a couple of
//  comparisons rather than a layout query.
// ============================================================================
#pragma once

#include "core/math.hpp"

namespace rawui {

struct ListScroll {
    float offset = 0.0f;
    float target = 0.0f;

    void reset() { offset = target = 0.0f; }

    void update(float dt, float speed = 16.0f) {
        offset = lerpf(offset, target, clampf(dt * speed, 0.0f, 1.0f));
        if (std::fabs(offset - target) < 0.5f) offset = target;
    }

    // Scrolls just enough to bring `index` fully into view.
    void ensureVisible(int index, int count, float rowH, float viewH, float padding = 0.0f) {
        if (count <= 0 || viewH <= 0.0f) return;

        const float top = static_cast<float>(index) * rowH - padding;
        const float bottom = top + rowH + padding * 2.0f;

        if (top < target) {
            target = top;
        } else if (bottom > target + viewH) {
            target = bottom - viewH;
        }

        const float maxOffset = maxf(0.0f, static_cast<float>(count) * rowH - viewH);
        target = clampf(target, 0.0f, maxOffset);
    }

    float maxOffset(int count, float rowH, float viewH) const {
        return maxf(0.0f, static_cast<float>(count) * rowH - viewH);
    }
};

}  // namespace rawui
