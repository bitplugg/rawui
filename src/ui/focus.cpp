#include "ui/focus.hpp"

namespace rawui {
namespace {

// Smallest sensible separation before a candidate is considered "in that
// direction" - keeps a row of cards from stealing focus from the row below it.
constexpr float kAxisEpsilon = 2.0f;

}  // namespace

void FocusManager::end() {
    // Remember what was selected before the list is replaced - indices are not
    // stable between frames, ids are.
    int keepId = -1;
    if (current_ >= 0 && current_ < static_cast<int>(items_.size())) {
        keepId = items_[static_cast<size_t>(current_)].id;
    }

    items_.swap(build_);

    const auto indexOfEnabledId = [this](int id) -> int {
        if (id < 0) return -1;
        for (size_t i = 0; i < items_.size(); ++i) {
            if (items_[i].id == id && items_[i].enabled) return static_cast<int>(i);
        }
        return -1;
    };

    const int kept = indexOfEnabledId(keepId);
    if (kept >= 0) {
        current_ = kept;
        return;
    }

    // The previous item is gone (screen switched, list refreshed...).  Prefer
    // the entry point the screen asked for, then the first enabled item - which
    // is a navigation rail entry, since those are registered first.
    const int preferred = indexOfEnabledId(preferred_);
    if (preferred >= 0) {
        current_ = preferred;
        return;
    }

    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].enabled) {
            current_ = static_cast<int>(i);
            return;
        }
    }
    current_ = -1;
}

int FocusManager::indexOfId(int id) const {
    for (size_t i = 0; i < items_.size(); ++i) {
        if (items_[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

int FocusManager::currentId() const {
    if (current_ < 0 || current_ >= static_cast<int>(items_.size())) return -1;
    return items_[static_cast<size_t>(current_)].id;
}

Rect FocusManager::currentRect() const {
    if (current_ < 0 || current_ >= static_cast<int>(items_.size())) return Rect();
    return items_[static_cast<size_t>(current_)].rect;
}

Rect FocusManager::itemRect(int index) const {
    if (index < 0 || index >= static_cast<int>(items_.size())) return Rect();
    return items_[static_cast<size_t>(index)].rect;
}

bool FocusManager::itemEnabled(int index) const {
    if (index < 0 || index >= static_cast<int>(items_.size())) return false;
    return items_[static_cast<size_t>(index)].enabled;
}

void FocusManager::selectById(int id) {
    const int index = indexOfId(id);
    if (index >= 0 && items_[static_cast<size_t>(index)].enabled) current_ = index;
}

void FocusManager::selectIndex(int index) {
    if (index >= 0 && index < static_cast<int>(items_.size()) &&
        items_[static_cast<size_t>(index)].enabled) {
        current_ = index;
    }
}

void FocusManager::resetTo(int preferredId) {
    const int index = indexOfId(preferredId);
    if (index >= 0 && items_[static_cast<size_t>(index)].enabled) {
        current_ = index;
        animInitialised_ = false;
        return;
    }
    end();
    animInitialised_ = false;
}

bool FocusManager::navigate(int dx, int dy) {
    if (items_.empty() || current_ < 0) {
        end();
        return false;
    }

    const Rect from = items_[static_cast<size_t>(current_)].rect;
    const float fx = from.cx();
    const float fy = from.cy();

    int best = -1;
    float bestScore = 0.0f;

    for (size_t i = 0; i < items_.size(); ++i) {
        const Item& it = items_[i];
        if (!it.enabled || static_cast<int>(i) == current_) continue;

        const float tx = it.rect.cx();
        const float ty = it.rect.cy();
        const float ddx = tx - fx;
        const float ddy = ty - fy;

        // Distance along the requested axis must be meaningful, and the
        // candidate must actually lie in that direction.
        const float along = ddx * static_cast<float>(dx) + ddy * static_cast<float>(dy);
        if (along <= kAxisEpsilon) continue;

        const float alongX = ddx * static_cast<float>(dx == 0 ? 1 : dx);
        (void)alongX;

        // Perpendicular offset punishes candidates that are far off axis.
        const float perp = (dx != 0) ? std::fabs(ddy) : std::fabs(ddx);

        // Prefer items whose extent overlaps the current one on the other axis
        // (feels like moving within a row/column).
        const bool overlap = (dx != 0) ? (it.rect.bottom() > from.top() + 1.0f &&
                                         it.rect.top() < from.bottom() - 1.0f)
                                       : (it.rect.right() > from.left() + 1.0f &&
                                          it.rect.left() < from.right() - 1.0f);

        float score = along + perp * 2.5f;
        if (overlap) score *= 0.55f;
        if (it.rect.w * it.rect.h <= 0.0f) score += 10000.0f;

        if (best < 0 || score < bestScore) {
            best = static_cast<int>(i);
            bestScore = score;
        }
    }

    if (best < 0) return false;
    current_ = best;
    return true;
}

void FocusManager::update(float dt, float timeScale) {
    const Rect target = currentRect();

    if (!animInitialised_ && !target.empty()) {
        animated_ = target;
        sx_.snapTo(target.x);
        sy_.snapTo(target.y);
        sw_.snapTo(target.w);
        sh_.snapTo(target.h);
        radiusSpring_.snapTo(currentRadius());
        animInitialised_ = true;
        return;
    }

    if (target.empty()) {
        animInitialised_ = false;
        return;
    }

    sx_.setTarget(target.x);
    sy_.setTarget(target.y);
    sw_.setTarget(target.w);
    sh_.setTarget(target.h);
    radiusSpring_.setTarget(currentRadius());

    animated_.x = sx_.step(dt, timeScale);
    animated_.y = sy_.step(dt, timeScale);
    animated_.w = sw_.step(dt, timeScale);
    animated_.h = sh_.step(dt, timeScale);
    radiusSpring_.step(dt, timeScale);
}

float FocusManager::currentRadius() const {
    if (current_ < 0 || current_ >= static_cast<int>(items_.size())) return 28.0f;
    return items_[static_cast<size_t>(current_)].radius;
}

float FocusManager::highlightAlpha() const {
    if (!animInitialised_) return 0.0f;

    // Fades in as the springs settle so the highlight does not pop on entry.
    const float dx = std::fabs(animated_.x - currentRect().x);
    const float dy = std::fabs(animated_.y - currentRect().y);
    const float d = maxf(dx, dy);
    return clampf(1.0f - d / 120.0f, 0.35f, 1.0f);
}

}  // namespace rawui
