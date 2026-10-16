#include "screens/hen.hpp"

#include <cmath>
#include <cstdio>

#include "gfx/icons.hpp"

namespace rawui {
namespace {

constexpr float kRowHeight = 78.0f;

Color stateColor(const Scheme& c, bool ok) { return ok ? c.success : c.onSurfaceVariant; }

}  // namespace

std::string HenScreen::subtitle(App& app) {
    const HenStatus& h = app.hen();
    if (h.running) {
        return "Payload detected" +
               (h.version.empty() ? std::string() : "  ·  " + h.version);
    }
    return h.payloadDirPresent ? "Payload folder found, no servers answering"
                               : "Payload not found";
}

void HenScreen::onEnter(App& app) {
    app.focus().setPreferred(kSummary);
    app.refreshHen();
    lastProbe_ = app.time();
}

void HenScreen::onFab(App& app) {
    app.refreshHen();
    lastProbe_ = app.time();
    app.toast("Probing GoldHEN ports...");
}

void HenScreen::draw(App& app) {
    ui::Ctx& ctx = app.ctx();
    const Scheme& c = app.colors();
    const Rect area = app.contentRect();
    const float s = ctx.displayScale;

    const HenStatus& h = app.hen();

    // Animation for the status dot.
    spin_ += ctx.dt;

    // --- summary card -------------------------------------------------------
    const Rect card(area.x, area.y, area.w, 150.0f * s);
    const Color accent = stateColor(c, h.running);
    ui::paintSurface(ctx, card, 24.0f * s, 1, c.surfaceContainerLow,
                     h.running ? c.primary : c.outlineVariant);
    ctx.focus->add(Rect(card.x, card.y, card.w, card.h), kSummary, false, 24.0f * s);

    const Color dot = h.running ? c.success : c.warning;
    const float pulse = h.running ? 0.5f + 0.5f * std::sin(spin_ * 3.0f) : 1.0f;

    ctx.canvas->fillCircle(card.x + 40.0f * s, card.cy(), (h.running ? 11.0f : 8.0f) * s,
                           dot.withAlphaF(h.running ? (0.55f + 0.45f * pulse) : 1.0f));

    ui::drawText(ctx, h.running ? "GoldHEN is running" : "GoldHEN is not answering",
                 ctx.type().headlineSmall,
                 Rect(card.x + 70.0f * s, card.y + 26.0f * s, card.w - 90.0f * s, 40.0f * s),
                 c.onSurface, ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx, h.payloadPath.empty() ? "No payload directory at /data/GoldHEN"
                                            : h.payloadPath,
                 ctx.type().bodyMedium,
                 Rect(card.x + 70.0f * s, card.y + 70.0f * s, card.w - 90.0f * s, 28.0f * s),
                 c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx, std::to_string(h.portCount) + " of " +
                          std::to_string(h.servers.size()) + " services listening",
                 ctx.type().bodySmall,
                 Rect(card.x + 70.0f * s, card.y + 100.0f * s, card.w - 90.0f * s, 26.0f * s),
                 accent, ui::Align::Left, ui::VAlign::Middle);

    // --- servers ------------------------------------------------------------
    float y = card.bottom() + 24.0f * s;
    ui::drawText(ctx, "Services", ctx.type().labelLarge,
                 Rect(area.x, y, area.w, 28.0f * s), c.onSurfaceVariant, ui::Align::Left,
                 ui::VAlign::Middle);
    y += 36.0f * s;

    for (size_t i = 0; i < h.servers.size(); ++i) {
        const HenServer& srv = h.servers[i];
        const Rect row(area.x, y, area.w, kRowHeight * s);
        const Color dotColor = stateColor(c, srv.listening);

        ui::paintSurface(ctx, row, 14.0f * s, 0, c.surfaceContainerLowest, Color(0, 0, 0, 0));
        ctx.canvas->fillCircle(row.x + 24.0f * s, row.cy(), 7.0f * s, dotColor);

        ui::drawText(ctx, srv.label, ctx.type().titleSmall,
                     Rect(row.x + 46.0f * s, row.y + 10.0f * s, row.w * 0.4f, 26.0f * s),
                     c.onSurface, ui::Align::Left, ui::VAlign::Middle);
        ui::drawText(ctx, srv.description, ctx.type().bodySmall,
                     Rect(row.x + 46.0f * s, row.y + 38.0f * s, row.w * 0.5f, 26.0f * s),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);

        char addr[64];
        std::snprintf(addr, sizeof(addr), "%s:%d", srv.host, srv.port);
        ui::drawText(ctx, srv.listening ? "listening" : "closed", ctx.type().bodySmall,
                     Rect(row.right() - 260.0f * s, row.cy() - 13.0f * s, 240.0f * s,
                          26.0f * s),
                     dotColor, ui::Align::Right, ui::VAlign::Middle);
        ui::drawText(ctx, addr, ctx.type().bodySmall,
                     Rect(row.right() - 260.0f * s, row.cy() + 13.0f * s, 240.0f * s,
                          26.0f * s),
                     c.onSurfaceVariant, ui::Align::Right, ui::VAlign::Middle);

        ctx.focus->add(row, kServerBase + static_cast<int>(i), false, 14.0f * s);
        y += kRowHeight * s + 8.0f * s;
    }

    // --- cheats shortcut ----------------------------------------------------
    y += 8.0f * s;
    const Rect link(area.x, y, area.w, 84.0f * s);
    ui::paintSurface(ctx, link, 20.0f * s, 0, c.surfaceContainerLow, Color(0, 0, 0, 0));
    ui::drawIcon(ctx, icons::kExtension, 30.0f * s,
                 Vec2(link.x + 40.0f * s, link.cy()), c.primary, true);
    ui::drawText(ctx, "Cheats and patches", ctx.type().titleMedium,
                 Rect(link.x + 76.0f * s, link.y + 16.0f * s, link.w * 0.6f, 28.0f * s),
                 c.onSurface, ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx, std::to_string(h.cheatCount) + " cheat files  ·  " +
                          std::to_string(h.patchCount) + " patch files",
                 ctx.type().bodySmall,
                 Rect(link.x + 76.0f * s, link.y + 46.0f * s, link.w * 0.6f, 26.0f * s),
                 c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
    ui::drawIcon(ctx, icons::kArrowForward, 26.0f * s,
                 Vec2(link.right() - 34.0f * s, link.cy()), c.onSurfaceVariant, false);
    ctx.focus->add(link, kCheatsLink, true, 20.0f * s);
}

void HenScreen::onConfirm(App& app, int focusId) {
    if (focusId == kCheatsLink) {
        app.navigateTo(4);
    }
}

}  // namespace rawui
