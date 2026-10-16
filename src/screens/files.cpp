#include "screens/files.hpp"

#include "gfx/icons.hpp"

namespace rawui {
namespace {

constexpr float kRowHeight = 78.0f;

}  // namespace

std::string FilesScreen::subtitle(App& app) {
    (void)app;
    if (!ok_) return "This folder could not be read";
    return cwd_;
}

void FilesScreen::onEnter(App& app) {
    app.focus().setPreferred(kRowBase);
    mounts_ = fs::mountPoints();
    if (mounts_.empty()) mounts_.push_back("/data");
    refresh(app);
}

void FilesScreen::refresh(App& app) {
    (void)app;
    ok_ = true;
    entries_ = fs::listDir(cwd_, ok_);
    scroll_.reset();
    focusIndex_ = 0;
}

void FilesScreen::enterDirectory(App& app, const std::string& path) {
    cwd_ = trimTrailingSlash(path);
    if (cwd_.empty()) cwd_ = "/";
    refresh(app);
    app.focus().setPreferred(kRowBase);
    app.toast(baseName(cwd_.empty() ? std::string("/") : cwd_));
}

bool FilesScreen::atMountRoot() const {
    for (const std::string& m : mounts_) {
        if (trimTrailingSlash(m) == cwd_) return true;
    }
    return cwd_ == "/" || cwd_ == "/data" || cwd_ == "/user";
}

bool FilesScreen::goUp(App& app) {
    if (atMountRoot()) return false;

    const std::string parent = dirName(cwd_);
    if (parent.empty() || parent == cwd_) return false;

    cwd_ = parent;
    refresh(app);
    app.focus().setPreferred(kRowBase);
    return true;
}

bool FilesScreen::onBack(App& app) { return goUp(app); }

void FilesScreen::draw(App& app) {
    ui::Ctx& ctx = app.ctx();
    const Scheme& c = app.colors();
    const Rect area = app.contentRect();
    const float s = ctx.displayScale;

    // --- path + shortcut chips ---------------------------------------------
    const Rect pathBar(area.x, area.y, area.w, 44.0f * s);
    ui::drawIcon(ctx, icons::kFolderOpen, 24.0f * s,
                 Vec2(pathBar.x + 12.0f * s, pathBar.cy()), c.primary, true);
    ui::drawText(ctx, cwd_, ctx.type().titleMedium,
                 Rect(pathBar.x + 36.0f * s, pathBar.y, area.w * 0.6f, pathBar.h),
                 c.onSurface, ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx, std::to_string(entries_.size()) + " items", ctx.type().bodySmall,
                 Rect(pathBar.right() - 200.0f * s, pathBar.y, 200.0f * s, pathBar.h),
                 c.onSurfaceVariant, ui::Align::Right, ui::VAlign::Middle);

    // Shortcut chips to each mount point.
    const float chipY = pathBar.bottom() + 8.0f * s;
    const float chipH = 40.0f * s;
    float chipX = area.x;
    for (size_t i = 0; i < mounts_.size() && i < 6; ++i) {
        const std::string label = mounts_[i];
        const float w = ui::textWidth(ctx, label, ctx.type().labelLarge) + 46.0f * s;
        if (chipX + w > area.right()) break;

        const Rect chip(chipX, chipY, w, chipH);
        const bool selected = trimTrailingSlash(label) == cwd_;
        ui::chip(ctx, chip, label, selected ? icons::kCheck : icons::kStorage, selected,
                 ui::focusProximity(ctx, chip, 90.0f * s), false);
        ctx.focus->add(chip, kChipBase + static_cast<int>(i), true, chipH * 0.5f);
        chipX += w + 10.0f * s;
    }

    // --- list ---------------------------------------------------------------
    const Rect list(area.x, chipY + chipH + 14.0f * s, area.w,
                    area.bottom() - (chipY + chipH + 14.0f * s));
    const float rowH = kRowHeight * s;

    // Column headings.
    const Rect head(list.x, list.y, list.w, 32.0f * s);
    ui::drawText(ctx, "Name", ctx.type().labelSmall,
                 Rect(head.x + 60.0f * s, head.y, 400.0f * s, head.h), c.onSurfaceVariant,
                 ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx, "Size", ctx.type().labelSmall,
                 Rect(head.right() - 320.0f * s, head.y, 140.0f * s, head.h),
                 c.onSurfaceVariant, ui::Align::Right, ui::VAlign::Middle);
    ui::drawText(ctx, "Modified", ctx.type().labelSmall,
                 Rect(head.right() - 170.0f * s, head.y, 170.0f * s, head.h),
                 c.onSurfaceVariant, ui::Align::Right, ui::VAlign::Middle);

    const Rect rows(list.x, head.bottom() + 4.0f * s, list.w,
                    list.bottom() - head.bottom() - 4.0f * s);
    ctx.canvas->pushClip(rows);

    if (!ok_) {
        ui::drawText(ctx, "Permission denied, or the folder no longer exists.",
                     ctx.type().bodyLarge, Rect(rows.x, rows.y + 30.0f * s, rows.w, 50.0f * s),
                     c.error, ui::Align::Left, ui::VAlign::Middle);
    } else if (entries_.empty()) {
        ui::drawText(ctx, "This folder is empty.", ctx.type().bodyLarge,
                     Rect(rows.x, rows.y + 30.0f * s, rows.w, 50.0f * s), c.onSurfaceVariant,
                     ui::Align::Left, ui::VAlign::Middle);
    }

    const int count = static_cast<int>(entries_.size());
    int focused = ctx.focus->currentId() - kRowBase;
    if (focused < 0 || focused >= count) focused = focusIndex_;

    for (int i = 0; i < count; ++i) {
        const float y = rows.y + static_cast<float>(i) * rowH - scroll_.offset;
        if (y + rowH < rows.y - 1.0f || y > rows.bottom() + 1.0f) continue;

        const FileEntry& e = entries_[static_cast<size_t>(i)];
        const Rect row(rows.x, y, rows.w - 10.0f * s, rowH - 6.0f * s);

        const std::string sizeText = e.isDir ? "Folder" : humanSize(e.size);
        ui::listItem(ctx, row, iconForPath(e.path, e.isDir), e.name, std::string(),
                     sizeText + "     " + humanTime(e.modified), i == focused,
                     ui::focusProximity(ctx, row, 90.0f * s), false,
                     e.isSymlink ? &c.tertiary : nullptr);

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

void FilesScreen::onConfirm(App& app, int focusId) {
    if (focusId >= kChipBase && focusId < kRowBase) {
        const int index = focusId - kChipBase;
        if (index >= 0 && index < static_cast<int>(mounts_.size())) {
            enterDirectory(app, mounts_[static_cast<size_t>(index)]);
        }
        return;
    }

    if (focusId < kRowBase) return;
    const int index = focusId - kRowBase;
    if (index < 0 || index >= static_cast<int>(entries_.size())) return;

    const FileEntry& e = entries_[static_cast<size_t>(index)];
    if (e.isDir) {
        enterDirectory(app, e.path);
        return;
    }

    app.toast(e.name + "  ·  " + humanSize(e.size) + "  ·  " + humanTime(e.modified), true);
}

void FilesScreen::onAlt(App& app, int focusId) {
    if (focusId < kRowBase) return;
    const int index = focusId - kRowBase;
    if (index < 0 || index >= static_cast<int>(entries_.size())) return;

    const FileEntry entry = entries_[static_cast<size_t>(index)];

    app.showConfirm("Delete " + entry.name + "?",
                    entry.isDir ? "The folder and everything inside it will be deleted."
                                : humanSize(entry.size) + " will be freed.",
                    "Delete",
                    [this, &app, entry] {
                        if (fs::removeAll(entry.path)) {
                            app.toast("Deleted " + entry.name, true);
                            refresh(app);
                        } else {
                            app.toast("Could not delete " + entry.name, true);
                        }
                    },
                    true);
}

}  // namespace rawui
