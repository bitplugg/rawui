#include "screens/dashboard.hpp"

#include "gfx/icons.hpp"
#include "platform/fs.hpp"

namespace rawui {
namespace {

ui::Ctx& C(App& app) { return app.ctx(); }
const Scheme& S(App& app) { return app.colors(); }

float L(App& app, float v) { return v * app.ctx().displayScale; }

Rect scaled(const Rect& r, float s) {
    const float w = r.w * s;
    const float h = r.h * s;
    return Rect(r.cx() - w * 0.5f, r.cy() - h * 0.5f, w, h);
}

void drawHeroCard(App& app, const Rect& rect, uint32_t icon, const std::string& label,
                  const std::string& value, const Color& accent, bool highlight) {
    ui::Ctx& ctx = C(app);
    const Scheme& c = S(app);

    const float proximity = ui::focusProximity(ctx, rect, L(app, 90.0f));
    const Rect box = scaled(rect, 1.0f + 0.015f * proximity);

    ui::paintSurface(ctx, box, L(app, 24.0f), 1, c.surfaceContainerLow,
                     highlight ? c.primary : c.outlineVariant);

    const float p = L(app, 22.0f);
    const float badge = L(app, 48.0f);

    // Accent badge.
    ctx.canvas->fillRoundRect(Rect(box.x + p, box.y + p, badge, badge), L(app, 16.0f),
                              accent.withAlphaF(0.18f));
    ui::drawIcon(ctx, icon, L(app, 26.0f), Vec2(box.x + p + badge * 0.5f, box.y + p + badge * 0.5f),
                 accent, true);

    ui::drawText(ctx, label, ctx.type().labelLarge,
                 Rect(box.x + p + badge + L(app, 14.0f), box.y + p, box.w, badge),
                 c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);

    ui::drawText(ctx, value, ctx.type().headlineSmall,
                 Rect(box.x + p, box.bottom() - L(app, 56.0f) - p * 0.5f, box.w - p * 2.0f,
                      L(app, 46.0f)),
                 c.onSurface, ui::Align::Left, ui::VAlign::Middle);
}

void drawTile(App& app, const Rect& rect, uint32_t icon, const std::string& title,
              const std::string& supporting) {
    ui::Ctx& ctx = C(app);
    const Scheme& c = S(app);

    const float proximity = ui::focusProximity(ctx, rect, L(app, 90.0f));
    const Rect box = scaled(rect, 1.0f + 0.02f * proximity);

    ui::paintSurface(ctx, box, L(app, 26.0f), 1, c.surfaceContainer,
                     proximity > 0.35f ? c.primary : Color(0, 0, 0, 0));

    const float p = L(app, 22.0f);
    const float badge = L(app, 54.0f);

    ctx.canvas->fillRoundRect(Rect(box.x + p, box.y + p, badge, badge), L(app, 18.0f),
                              c.secondaryContainer);
    ui::drawIcon(ctx, icon, L(app, 30.0f),
                 Vec2(box.x + p + badge * 0.5f, box.y + p + badge * 0.5f),
                 c.onSecondaryContainer, proximity > 0.5f);

    ui::drawText(ctx, title, ctx.type().titleMedium,
                 Rect(box.x + p, box.y + L(app, 92.0f), box.w - p * 2.0f, L(app, 32.0f)),
                 c.onSurface, ui::Align::Left, ui::VAlign::Middle);

    if (!supporting.empty()) {
        ui::drawText(ctx, supporting, ctx.type().bodySmall,
                     Rect(box.x + p, box.y + L(app, 126.0f), box.w - p * 2.0f, L(app, 28.0f)),
                     c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
    }
}

}  // namespace

const DashboardScreen::Tile* DashboardScreen::tiles(int& count) {
    static const Tile kTiles[] = {
        {"Install packages", "Scan drives for .pkg files", icons::kInstall, 1},
        {"File manager", "Browse and clean up /data", icons::kFolder, 2},
        {"GoldHEN", "Payload status and servers", icons::kSecurity, 3},
        {"Cheats", "Cheat files and patches", icons::kExtension, 4},
        {"Settings", "Theme, text and storage", icons::kSettings, 5},
    };
    count = static_cast<int>(sizeof(kTiles) / sizeof(kTiles[0]));
    return kTiles;
}

std::string DashboardScreen::subtitle(App& app) {
    const HenStatus& h = app.hen();
    std::string s = h.running ? "GoldHEN active" : "GoldHEN not detected";
    s += h.running && h.portCount > 0 ? "  ·  " + std::to_string(h.portCount) + " servers"
                                      : "";
    return s;
}

void DashboardScreen::onEnter(App& app) {
    app.focus().setPreferred(kTileBase);
    app.refreshHen();
}

void DashboardScreen::draw(App& app) {
    ui::Ctx& ctx = C(app);
    const Scheme& c = S(app);
    const Rect area = app.contentRect();

    const float gap = L(app, 22.0f);
    const float heroH = L(app, 150.0f);
    const float colW = (area.w - gap * 2.0f) / 3.0f;

    // --- status cards -------------------------------------------------------
    const HenStatus& h = app.hen();
    const fs::SpaceInfo& sp = app.dataSpace();

    const std::string henValue = h.running ? "Active" : "Offline";
    const Color henAccent = h.running ? c.success : c.warning;

    std::string storageValue = "--";
    if (sp.valid) storageValue = humanSize(sp.free) + " free";

    const std::string cheatValue =
        h.cheatCount > 0 ? std::to_string(h.cheatCount) + " files" : "None found";

    const uint32_t cardIcons[3] = {icons::kSecurity, icons::kStorage, icons::kExtension};
    const std::string cardLabels[3] = {"GoldHEN", "Internal storage", "Cheat files"};
    const std::string cardValues[3] = {henValue, storageValue, cheatValue};
    const Color cardAccents[3] = {henAccent, c.tertiary, c.primary};
    const int cardIds[3] = {kCardHen, kCardStorage, kCardCheats};

    for (int i = 0; i < 3; ++i) {
        const Rect r(area.x + static_cast<float>(i) * (colW + gap), area.y, colW, heroH);
        drawHeroCard(app, r, cardIcons[i], cardLabels[i], cardValues[i], cardAccents[i],
                     i == 0 && h.running);
        ctx.focus->add(r, cardIds[i], true, L(app, 24.0f));
    }

    // --- quick actions ------------------------------------------------------
    int tileCount = 0;
    const Tile* list = tiles(tileCount);

    const float sectionY = area.y + heroH + L(app, 30.0f);
    ui::drawText(ctx, "Quick actions", ctx.type().labelLarge,
                 Rect(area.x, sectionY, area.w, L(app, 30.0f)), c.onSurfaceVariant,
                 ui::Align::Left, ui::VAlign::Middle);

    const float gridY = sectionY + L(app, 40.0f);
    const float tileH = L(app, 168.0f);

    for (int i = 0; i < tileCount; ++i) {
        const int col = i % 3;
        const int row = i / 3;
        const Rect r(area.x + static_cast<float>(col) * (colW + gap),
                     gridY + static_cast<float>(row) * (tileH + gap), colW, tileH);
        if (r.bottom() > area.bottom()) break;

        drawTile(app, r, list[i].icon, list[i].label, list[i].supporting);
        ctx.focus->add(r, kTileBase + i, true, L(app, 26.0f));
    }
}

void DashboardScreen::onConfirm(App& app, int focusId) {
    if (focusId == kCardHen) {
        app.navigateTo(3);
        return;
    }
    if (focusId == kCardStorage) {
        app.navigateTo(2);
        return;
    }
    if (focusId == kCardCheats) {
        app.navigateTo(4);
        return;
    }

    int tileCount = 0;
    const Tile* list = tiles(tileCount);
    const int index = focusId - kTileBase;
    if (index >= 0 && index < tileCount) {
        app.navigateTo(list[index].screenIndex);
    }
}

}  // namespace rawui
