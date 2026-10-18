#include "screens/cheats.hpp"

#include "gfx/icons.hpp"

namespace rawui {
namespace {

constexpr float kRowHeight = 76.0f;
constexpr float kSegHeight = 52.0f;

bool isJson(const FileEntry& e) {
    const std::string ext = fileExtension(e.path);
    return ext == "json";
}

bool isPatch(const FileEntry& e) {
    const std::string ext = fileExtension(e.path);
    return ext == "txt" || ext == "patch" || ext == "cht" || ext == "shn";
}

std::string stripDisabled(const std::string& name) {
    if (name.size() > 9 && name.substr(name.size() - 9) == ".disabled") {
        return name.substr(0, name.size() - 9);
    }
    return name;
}

}  // namespace

std::string CheatsScreen::subtitle(App& app) {
    const HenStatus& h = app.hen();
    if (mode_ == kCheats) {
        return "Cheats · " + std::to_string(h.cheatCount) + " files";
    }
    return "Patches · " + std::to_string(h.patchCount) + " files";
}

void CheatsScreen::onEnter(App& app) {
    app.focus().setPreferred(kSegBase);
    mode_ = kCheats;
    refresh(app);
}

bool CheatsScreen::isDisabled(const std::string& name) const {
    return name.size() > 9 && name.substr(name.size() - 9) == ".disabled";
}

std::string CheatsScreen::enabledPath(const std::string& path) const {
    const std::string name = baseName(path);
    if (isDisabled(name)) return dirName(path) + "/" + stripDisabled(name);
    return path;
}

std::string CheatsScreen::disabledPath(const std::string& path) const {
    const std::string name = baseName(path);
    if (isDisabled(name)) return path;
    return path + ".disabled";
}

void CheatsScreen::refresh(App& app) {
    (void)app;
    if (mode_ == kCheats) {
        sources_ = cheatDirectories();
    } else {
        sources_ = patchDirectories();
    }

    entries_.clear();
    for (const std::string& dir : sources_) {
        bool okDir = false;
        std::vector<FileEntry> list = fs::listDir(dir, okDir);
        if (!okDir) continue;
        for (const auto& e : list) {
            if (e.isDir) continue;
            if (mode_ == kCheats) {
                if (!isJson(e)) continue;
            } else {
                if (!isPatch(e) && fileExtension(e.path) != "ini" && fileExtension(e.path) != "json") {
                    // Still show txt/patch/ini but try to be sensible.
                    if (!isPatch(e)) {
                        // Relax - GoldHEN reads various patch files; include all non-dir files.
                    }
                }
            }
            // For patches, accept any file in patch dir (common names vary).
            if (mode_ == kPatches) {
                if (fileExtension(e.path) == "bak") continue;
            }
            entries_.push_back(e);
        }
    }

    scroll_.reset();
    focusIndex_ = 0;
}

void CheatsScreen::draw(App& app) {
    ui::Ctx& ctx = app.ctx();
    const Scheme& c = app.colors();
    const Rect area = app.contentRect();
    const float s = ctx.displayScale;

    // Segmented control.
    const std::vector<std::string> seg = {"Cheats", "Patches"};
    const float segH = kSegHeight * s;
    const Rect segR(area.x, area.y, minf(area.w, 420.0f * s), segH);
    ui::segmented(ctx, segR, seg, mode_ == kCheats ? 0 : 1);
    ctx.focus->add(segR, kSegBase, false, segH * 0.5f);

    // Header row.
    const float listY = segR.bottom() + 16.0f * s;
    const Rect head(area.x, listY, area.w, 32.0f * s);
    ui::drawText(ctx, "File", ctx.type().labelSmall,
                 Rect(head.x + 60.0f * s, head.y, 400.0f * s, head.h), c.onSurfaceVariant,
                 ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx, "Status", ctx.type().labelSmall,
                 Rect(head.right() - 240.0f * s, head.y, 220.0f * s, head.h), c.onSurfaceVariant,
                 ui::Align::Right, ui::VAlign::Middle);
    ui::drawText(ctx, "Modified", ctx.type().labelSmall,
                 Rect(head.right() - 60.0f * s, head.y, 200.0f * s, head.h), c.onSurfaceVariant,
                 ui::Align::Right, ui::VAlign::Middle);

    const Rect rows(area.x, head.bottom() + 4.0f * s, area.w,
                    area.bottom() - head.bottom() - 4.0f * s);
    ctx.canvas->pushClip(rows);

    if (entries_.empty()) {
        ui::drawText(ctx,
                     mode_ == kCheats ? "No cheat JSON files found in GoldHEN/cheats"
                                      : "No patch files found in GoldHEN/patches",
                     ctx.type().bodyLarge, Rect(rows.x, rows.y + 30.0f * s, rows.w, 50.0f * s),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
        if (!sources_.empty()) {
            ui::drawText(ctx, "Looked in: " + sources_[0],
                         ctx.type().bodySmall, Rect(rows.x, rows.y + 80.0f * s, rows.w, 40.0f * s),
                         c.onSurfaceVariant.withAlphaF(0.7f), ui::Align::Left, ui::VAlign::Middle);
        }
    }

    const float rowH = kRowHeight * s;
    const int count = static_cast<int>(entries_.size());
    int focused = ctx.focus->currentId() - kRowBase;
    if (focused < 0 || focused >= count) focused = focusIndex_;

    for (int i = 0; i < count; ++i) {
        const float y = rows.y + static_cast<float>(i) * rowH - scroll_.offset;
        if (y + rowH < rows.y - 1.0f || y > rows.bottom() + 1.0f) continue;

        const FileEntry& e = entries_[static_cast<size_t>(i)];
        const bool disabled = isDisabled(e.name);
        const Rect row(rows.x, y, rows.w - 8.0f * s, rowH - 6.0f * s);

        ui::listItem(ctx, row, icons::kExtension, stripDisabled(e.name),
                     disabled ? "Disabled" : "Enabled", humanTime(e.modified), i == focused,
                     ui::focusProximity(ctx, row, 90.0f * s), false,
                     disabled ? &c.onSurfaceVariant : nullptr);

        if (i == focused) focusIndex_ = i;
        ctx.focus->add(row, kRowBase + i, true, 12.0f * s);
    }

    ctx.canvas->popClip();

    if (count * rowH > rows.h) {
        ui::scrollIndicator(ctx, Rect(rows.right() - 4.0f * s, rows.y, 4.0f * s, rows.h),
                            scroll_.offset, static_cast<float>(count) * rowH, rows.h);
    }

    scroll_.ensureVisible(focused, count, rowH, rows.h, 6.0f * s);
}

bool CheatsScreen::onAdjust(App& app, int focusId, int delta) {
    (void)app;
    if (focusId == kSegBase) {
        mode_ = (mode_ == kCheats && delta > 0) ? kPatches : (mode_ == kPatches && delta < 0) ? kCheats : mode_;
        refresh(app);
        app.focus().setPreferred(kRowBase);
        return true;
    }
    return false;
}

void CheatsScreen::onConfirm(App& app, int focusId) {
    if (focusId < kRowBase) return;
    const int index = focusId - kRowBase;
    if (index < 0 || index >= static_cast<int>(entries_.size())) return;

    const FileEntry e = entries_[static_cast<size_t>(index)];
    const bool disabled = isDisabled(e.name);
    const std::string to = disabled ? enabledPath(e.path) : disabledPath(e.path);

    if (fs::rename(e.path, to)) {
        app.toast(stripDisabled(e.name) + (disabled ? " enabled" : " disabled"), true);
        refresh(app);
    } else {
        app.toast("Failed to toggle " + e.name, true);
    }
}

void CheatsScreen::onAlt(App& app, int focusId) {
    if (focusId < kRowBase) return;
    const int index = focusId - kRowBase;
    if (index < 0 || index >= static_cast<int>(entries_.size())) return;

    const FileEntry e = entries_[static_cast<size_t>(index)];
    app.showConfirm("Delete " + stripDisabled(e.name) + "?", "This file will be removed.",
                    "Delete",
                    [this, &app, e] {
                        if (fs::removeAll(e.path)) {
                            app.toast("Deleted " + stripDisabled(e.name), true);
                            refresh(app);
                        } else {
                            app.toast("Could not delete " + e.name, true);
                        }
                    },
                    true);
}

}  // namespace rawui
