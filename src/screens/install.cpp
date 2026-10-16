#include "screens/install.hpp"

#include <cstdio>

#include "gfx/icons.hpp"
#include "platform/fs.hpp"
#include "platform/notify.hpp"

namespace rawui {
namespace {

constexpr float kRowHeight = 84.0f;
constexpr float kPollInterval = 0.4f;

std::string hex32(int value) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%08X", static_cast<unsigned>(value));
    return buf;
}

}  // namespace

std::string InstallScreen::subtitle(App& app) {
    if (installing_) return "Installing " + installName_;
    if (pkgs_.empty()) return scanned_ ? "No packages found" : "Scanning drives...";
    return std::to_string(pkgs_.size()) + (pkgs_.size() == 1 ? " package" : " packages");
}

void InstallScreen::onEnter(App& app) {
    app.focus().setPreferred(kFirstRow);
    pkgs_.clear();
    scan(app);
}

void InstallScreen::scan(App& app) {
    const std::vector<std::string> roots = pkg::searchRoots();
    pkgs_ = pkg::scan(roots, 3, 400);
    scanned_ = true;
    scroll_.reset();
    app.toast(pkgs_.empty() ? "No .pkg files found on the mounted volumes"
                            : std::to_string(pkgs_.size()) + " package(s) found");
}

void InstallScreen::beginInstall(App& app, int index) {
    if (index < 0 || index >= static_cast<int>(pkgs_.size())) return;

    const PkgCandidate& candidate = pkgs_[static_cast<size_t>(index)];
    const std::string path = candidate.path;

    // The title id lives in the package header, so it is read on demand rather
    // than for every file during the scan.
    const std::string titleId = pkg::readTitleId(path);
    const std::string contentId = candidate.contentId;

    auto run = [this, &app, path, titleId, contentId, name = candidate.name]() mutable {
        installFailed_ = false;
        const int rc = pkg::startInstall(path);
        if (rc != 0) {
            app.toast("Install rejected: " + hex32(rc), true);
            installFailed_ = true;
            installing_ = false;
            return;
        }

        installing_ = true;
        installFailed_ = false;
        installName_ = name;
        installTitleId_ = titleId;
        installContentId_ = contentId;
        progress_ = InstallProgress{};
        progress_.titleId = titleId;
        progress_.contentId = contentId;
        progress_.indeterminate = contentId.empty();
        pollTimer_ = 0.0f;
        elapsed_ = 0.0f;
        app.toast("Installing " + name, true);
    };

    if (!titleId.empty() && pkg::appExists(titleId)) {
        app.showConfirm("Already installed",
                        "Title " + titleId +
                            " is already installed. The installer will overwrite it.",
                        "Overwrite", run, true);
        return;
    }

    run();
}

void InstallScreen::update(App& app, float dt) {
    scroll_.update(dt);

    if (!installing_) return;

    elapsed_ += dt;
    pollTimer_ -= dt;
    if (pollTimer_ > 0.0f) return;
    pollTimer_ = kPollInterval;

    progress_ = pkg::pollProgress(installContentId_, installTitleId_);

    if (progress_.finished) {
        installing_ = false;
        app.toast("Installed " + installName_, true);
        pkgs_.clear();
        scan(app);
    }
}

void InstallScreen::draw(App& app) {
    ui::Ctx& ctx = app.ctx();
    const Scheme& c = app.colors();
    const Rect area = app.contentRect();

    const float s = ctx.displayScale;
    const float rowH = kRowHeight * s;

    // --- toolbar ------------------------------------------------------------
    const Rect toolbar(area.x, area.y, area.w, 56.0f * s);
    ui::drawText(ctx, "Searching: USB drives, /data, /mnt/ext0", ctx.type().bodyMedium,
                 Rect(toolbar.x + 4.0f * s, toolbar.y, toolbar.w - 220.0f * s, toolbar.h),
                 c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);

    const Rect refresh(toolbar.right() - 150.0f * s, toolbar.y, 150.0f * s, 48.0f * s);
    const float refreshProx = ui::focusProximity(ctx, refresh, 90.0f * s);
    ui::button(ctx, refresh, "Rescan", icons::kRefresh, ui::ButtonStyle::Tonal, refreshProx,
               false, true);
    ctx.focus->add(refresh, kRefresh, true, refresh.h * 0.5f);

    // --- list ---------------------------------------------------------------
    const float listY = toolbar.bottom() + 14.0f * s;
    const float listH = area.bottom() - listY - (installing_ ? 190.0f * s : 0.0f);
    const Rect list(area.x, listY, area.w, maxf(listH, rowH));

    ctx.canvas->pushClip(list);

    const int count = static_cast<int>(pkgs_.size());
    int focused = ctx.focus->currentId() - kFirstRow;
    if (focused < 0 || focused >= count) focused = focusIndex_;

    if (count == 0) {
        const std::string msg = scanned_ ? "No .pkg files were found."
                                         : "Scanning mounted volumes...";
        ui::drawText(ctx, msg, ctx.type().bodyLarge,
                     Rect(list.x, list.y + 40.0f * s, list.w, 60.0f * s), c.onSurfaceVariant,
                     ui::Align::Left, ui::VAlign::Middle);
        ui::drawText(ctx, "Copy a package to a USB drive, then press Triangle to rescan.",
                     ctx.type().bodyMedium,
                     Rect(list.x, list.y + 90.0f * s, list.w, 40.0f * s),
                     c.onSurfaceVariant.withAlphaF(0.7f), ui::Align::Left, ui::VAlign::Middle);
    }

    for (int i = 0; i < count; ++i) {
        const float y = list.y + static_cast<float>(i) * rowH - scroll_.offset;
        if (y + rowH < list.y - 1.0f || y > list.bottom() + 1.0f) continue;

        const PkgCandidate& p = pkgs_[static_cast<size_t>(i)];
        const Rect row(list.x, y, list.w, rowH - 6.0f * s);

        std::string supporting = p.contentId.empty() ? "content id not in file name"
                                                     : p.contentId;
        ui::listItem(ctx, row, p.contentId.empty() ? icons::kArchive : icons::kDownloadDone,
                     p.name, supporting, humanSize(p.size), i == focused,
                     ui::focusProximity(ctx, row, 90.0f * s));

        if (i == focused) focusIndex_ = i;
        ctx.focus->add(row, kFirstRow + i, true, 12.0f * s);
    }

    ctx.canvas->popClip();

    // Scroll indicator.
    if (count * rowH > list.h) {
        ui::scrollIndicator(ctx, Rect(list.right() - 6.0f * s, list.y, 4.0f * s, list.h),
                            scroll_.offset, static_cast<float>(count) * rowH, list.h);
    }

    if (installing_ || installFailed_) {
        drawProgress(app, Rect(area.x, area.bottom() - 180.0f * s, area.w, 180.0f * s));
    }
}

std::string InstallScreen::stateText() const {
    if (installFailed_) return "Failed";
    if (progress_.finished) return "Finishing up";
    if (progress_.indeterminate) return "Installing - no progress data available";
    return std::to_string(progress_.percent) + "%";
}

void InstallScreen::drawProgress(App& app, const Rect& area) {
    ui::Ctx& ctx = app.ctx();
    const Scheme& c = app.colors();
    const float s = ctx.displayScale;

    ui::paintSurface(ctx, area, 24.0f * s, 2, c.surfaceContainerHigh, c.outlineVariant);

    const float pad = 24.0f * s;
    const float ringR = 34.0f * s;
    const Vec2 ringCentre(area.x + pad + ringR, area.cy());

    const float value = progress_.indeterminate ? 0.0f
                                                : clampf(progress_.percent / 100.0f, 0.0f, 1.0f);
    const bool spin = progress_.indeterminate || (!progress_.finished && value <= 0.0f);

    ui::progressRing(ctx, ringCentre, ringR, 7.0f * s, value, c.surfaceContainerHighest,
                     installFailed_ ? c.error : c.primary, spin);

    std::string title = installFailed_ ? "Install failed" : "Installing";
    if (!progress_.finished && !installFailed_) title += " " + installName_;

    const float textX = ringCentre.x + ringR + 22.0f * s;
    const float textW = area.right() - pad - textX;

    ui::drawText(ctx, title, ctx.type().titleMedium, Rect(textX, area.y + pad, textW, 30.0f * s),
                 c.onSurface, ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx, stateText(), ctx.type().bodyMedium,
                 Rect(textX, area.y + pad + 32.0f * s, textW, 26.0f * s), c.onSurfaceVariant,
                 ui::Align::Left, ui::VAlign::Middle);

    const Rect bar(textX, area.y + pad + 74.0f * s, textW, 10.0f * s);
    ui::progressBar(ctx, bar, value, c.surfaceContainerHighest,
                    installFailed_ ? c.error : c.primary, spin);

    std::string detail;
    if (installFailed_) {
        detail = "The installer refused the package. Free space, a GoldHEN-enabled "
                 "system and a valid FPKG are all required.";
    } else if (progress_.totalBytes > 0) {
        detail = humanSize(progress_.progressBytes) + " of " + humanSize(progress_.totalBytes);
        if (progress_.remainingSec > 0) {
            detail += "  ·  about " + std::to_string(progress_.remainingSec) + "s left";
        }
    } else {
        detail = "Waiting for the installer service.";
    }

    ui::drawText(ctx, detail, ctx.type().bodySmall,
                 Rect(textX, area.y + pad + 96.0f * s, textW, 28.0f * s),
                 c.onSurfaceVariant.withAlphaF(0.85f), ui::Align::Left, ui::VAlign::Middle);
    (void)elapsed_;
}

void InstallScreen::onConfirm(App& app, int focusId) {
    if (focusId == kRefresh) {
        scan(app);
        return;
    }
    if (focusId >= kFirstRow) {
        beginInstall(app, focusId - kFirstRow);
    }
}

void InstallScreen::onAlt(App& app, int focusId) {
    if (focusId < kFirstRow) return;
    const int index = focusId - kFirstRow;
    if (index < 0 || index >= static_cast<int>(pkgs_.size())) return;

    const PkgCandidate& p = pkgs_[static_cast<size_t>(index)];
    const std::string titleId = pkg::readTitleId(p.path);
    app.toast(p.name + "  ·  " + (titleId.empty() ? "unknown title" : titleId), true);
}

}  // namespace rawui
