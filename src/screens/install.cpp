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
    if (installing_) return trf(Str::kInstallingName, installName_.c_str());
    if (pkgs_.empty()) return scanned_ ? tr(Str::kNoPackages) : tr(Str::kScanning);
    const int n = static_cast<int>(pkgs_.size());
    return std::to_string(n) + " " +
           trn(Str::kUnitPackageOne, Str::kUnitPackageFew, Str::kUnitPackageMany, n);
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
    const int found = static_cast<int>(pkgs_.size());
    app.toast(found == 0 ? std::string(tr(Str::kNoPkgOnVolumes))
                         : std::to_string(found) + " " +
                               trn(Str::kUnitPackageOne, Str::kUnitPackageFew,
                                   Str::kUnitPackageMany, found));
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
            app.toast(std::string(tr(Str::kInstallRejected)) + ": " + hex32(rc), true);
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
        app.toast(trf(Str::kInstallingName, name.c_str()), true);
    };

    if (!titleId.empty() && pkg::appExists(titleId)) {
        // Built before showConfirm() so no trf() call can clobber the shared
        // buffer while the strings are still being read.
        const std::string body = std::string(trf(Str::kTitleIsInstalled, titleId.c_str())) + " " +
                                 tr(Str::kWillOverwrite);
        app.showConfirm(tr(Str::kAlreadyInstalled), body, tr(Str::kOverwrite), run, true);
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
        app.toast(trf(Str::kInstalled, installName_.c_str()), true);
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
    ui::drawText(ctx, tr(Str::kSearching), ctx.type().bodyMedium,
                 Rect(toolbar.x + 4.0f * s, toolbar.y, toolbar.w - 220.0f * s, toolbar.h),
                 c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);

    const Rect refresh(toolbar.right() - 150.0f * s, toolbar.y, 150.0f * s, 48.0f * s);
    const float refreshProx = ui::focusProximity(ctx, refresh, 90.0f * s);
    ui::button(ctx, refresh, tr(Str::kHintRescan), icons::kRefresh, ui::ButtonStyle::Tonal,
               refreshProx, false, true);
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
        const std::string msg = scanned_ ? tr(Str::kNoPkgFiles) : tr(Str::kScanningVolumes);
        ui::drawText(ctx, msg, ctx.type().bodyLarge,
                     Rect(list.x, list.y + 40.0f * s, list.w, 60.0f * s), c.onSurfaceVariant,
                     ui::Align::Left, ui::VAlign::Middle);
        ui::drawText(ctx, tr(Str::kCopyPkgHint),
                     ctx.type().bodyMedium,
                     Rect(list.x, list.y + 90.0f * s, list.w, 40.0f * s),
                     c.onSurfaceVariant.withAlphaF(0.7f), ui::Align::Left, ui::VAlign::Middle);
    }

    for (int i = 0; i < count; ++i) {
        const float y = list.y + static_cast<float>(i) * rowH - scroll_.offset;
        if (y + rowH < list.y - 1.0f || y > list.bottom() + 1.0f) continue;

        const PkgCandidate& p = pkgs_[static_cast<size_t>(i)];
        const Rect row(list.x, y, list.w, rowH - 6.0f * s);

        std::string supporting = p.contentId.empty() ? tr(Str::kContentIdMissing)
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
    if (installFailed_) return tr(Str::kStatusFailed);
    if (progress_.finished) return tr(Str::kFinishing);
    if (progress_.indeterminate) return tr(Str::kStatusNoData);
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

    std::string title = installFailed_ ? tr(Str::kInstallFailedTitle) : tr(Str::kInstalling);
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
        detail = tr(Str::kInstallRefusedDetail);
    } else if (progress_.totalBytes > 0) {
        detail = trf(Str::kBytesOf, humanSize(progress_.progressBytes).c_str(),
                     humanSize(progress_.totalBytes).c_str());
        if (progress_.remainingSec > 0) {
            // Cast for the "%d" conversion: remainingSec is unsigned, and
            // printf's promotion rules would otherwise be undefined behaviour.
            detail += trf(Str::kSecondsLeft, static_cast<int>(progress_.remainingSec));
        }
    } else {
        detail = tr(Str::kWaitingInstaller);
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
    app.toast(p.name + "  ·  " + (titleId.empty() ? tr(Str::kUnknownTitle) : titleId), true);
}

}  // namespace rawui
