// ============================================================================
//  RAW UI - application shell implementation.
// ============================================================================
#include "app/app.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>

#include <SDL2/SDL.h>

#include "core/log.hpp"
#include "core/math.hpp"
#include "gfx/icons.hpp"
#include "md3/motion.hpp"
#include "platform/config.hpp"
#include "platform/fs.hpp"
#include "platform/goldhen.hpp"
#include "platform/notify.hpp"
#include "platform/pkg.hpp"

#include "screens/cheats.hpp"
#include "screens/dashboard.hpp"
#include "screens/files.hpp"
#include "screens/hen.hpp"
#include "screens/install.hpp"
#include "screens/settings.hpp"

namespace rawui {
namespace {

// Navigation rail destinations are registered before any screen content, so a
// geometric fallback lands on the rail rather than in the middle of a list.
constexpr int kRailIdBase = 1000;

constexpr float kTransitionMs = 240.0f;
constexpr float kSnackLife = 4.0f;

inline int railId(int index) { return kRailIdBase + index; }

std::string clockText() {
    const time_t now = time(nullptr);
    if (now <= 0) return {};

    struct tm parts;
    if (localtime_r(&now, &parts) == nullptr) return {};

    char buffer[8];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d", parts.tm_hour, parts.tm_min);
    return buffer;
}

std::string fitText(ui::Ctx& ctx, const std::string& text, const TextStyle& style, float maxW) {
    if (ui::textWidth(ctx, text, style) <= maxW) return text;

    std::string out = text;
    while (!out.empty() && ui::textWidth(ctx, out + "…", style) > maxW) {
        out.pop_back();
    }
    return out + "…";
}

}  // namespace

App::App() = default;
App::~App() { shutdown(); }

// ---------------------------------------------------------------------------
bool App::init() {
    setvbuf(stdout, nullptr, _IONBF, 0);
    setvbuf(stderr, nullptr, _IONBF, 0);

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0) {
        RU_LOG("[app] SDL_Init failed: %s", SDL_GetError());
        if (SDL_Init(SDL_INIT_VIDEO) != 0) return false;
    }

    window_ = SDL_CreateWindow("RAW UI", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1920,
                              1080, 0);
    if (!window_) {
        RU_LOG("[app] SDL_CreateWindow failed: %s", SDL_GetError());
        return false;
    }

    SDL_Surface* ws = SDL_GetWindowSurface(static_cast<SDL_Window*>(window_));
    if (!ws || !ws->pixels) {
        RU_LOG("[app] SDL_GetWindowSurface failed: %s", SDL_GetError());
        return false;
    }
    windowSurface_ = ws;

    screenW_ = ws->w;
    screenH_ = ws->h;
    if (screenW_ <= 0 || screenH_ <= 0) {
        screenW_ = 1920;
        screenH_ = 1080;
    }

    layoutScale_ = static_cast<float>(screenH_) / 1080.0f;

    // Prefer drawing straight into the window surface; fall back to a scratch
    // surface when SDL hands us a format the rasteriser does not write.
    const SDL_PixelFormat* fmt = ws->format;
    const bool direct = fmt && fmt->BytesPerPixel == 4 && fmt->Rmask == 0x00FF0000u &&
                        fmt->Gmask == 0x0000FF00u && fmt->Bmask == 0x000000FFu;

    if (direct) {
        scratch_ = nullptr;
        canvas_.reset(reinterpret_cast<uint32_t*>(ws->pixels), screenW_, screenH_,
                      static_cast<int>(ws->pitch / 4));
    } else {
        SDL_Surface* scratch = SDL_CreateRGBSurfaceWithFormat(0, screenW_, screenH_, 32,
                                                             SDL_PIXELFORMAT_ARGB8888);
        if (!scratch) {
            RU_LOG("[app] SDL_CreateRGBSurfaceWithFormat failed: %s", SDL_GetError());
            return false;
        }
        scratch_ = scratch;
        canvas_.reset(reinterpret_cast<uint32_t*>(scratch->pixels), screenW_, screenH_,
                      static_cast<int>(scratch->pitch / 4));
    }
    RU_LOG("[app] window %dx%d direct=%d", screenW_, screenH_, direct ? 1 : 0);

    ctx_.canvas = &canvas_;
    ctx_.fonts = &fonts_;
    ctx_.theme = &theme_;
    ctx_.focus = &focus_;
    ctx_.screenW = screenW_;
    ctx_.screenH = screenH_;

    if (!fonts_.init("/app0/assets/fonts")) {
        assetsMissing_ = fonts_.errors();
        RU_LOG("[app] fonts: %s", assetsMissing_.c_str());
    }

    notify::init();
    input_.init();

    config_ = configLoad();
    applyConfig(false);

    buildScreens();

    henAutoTimer_ = 0.5f;
    spaceTimer_ = 0.5f;
    dataSpace_ = fs::space("/data");

    time_ = 0.0f;
    running_ = true;
    return true;
}

void App::shutdown() {
    if (!running_ && screens_.empty() && !window_) return;
    running_ = false;

    saveConfig();
    input_.shutdown();
    fonts_.shutdown();

    canvas_.reset(nullptr, 0, 0, 0);

    if (scratch_) {
        SDL_FreeSurface(static_cast<SDL_Surface*>(scratch_));
        scratch_ = nullptr;
    }
    windowSurface_ = nullptr;
    if (window_) {
        SDL_DestroyWindow(static_cast<SDL_Window*>(window_));
        window_ = nullptr;
    }

    SDL_Quit();
}

// ---------------------------------------------------------------------------
void App::applyConfig(bool animate) {
    ThemeConfig tc;
    tc.seed = Color::fromHex(config_.seedRgb);
    tc.dark = config_.dark;
    tc.vibrant = config_.vibrant;
    tc.contrast = clampf(config_.contrast, -0.3f, 0.3f);
    tc.typeScale = clampf(config_.typeScale, 0.8f, 1.4f);
    tc.motionScale = clampf(config_.motionScale, 0.0f, 2.0f);

    // Geometry follows the display scale (and the user's scale preference);
    // text additionally follows the theme's type scale.
    scale_ = displayScaleDefault() * layoutScale_ * tc.typeScale;

    if (animate && theme_.motionScale > 0.0f) {
        theme_.animateTo(tc, scale_ / maxf(tc.typeScale, 0.01f));
    } else {
        theme_.apply(tc, scale_ / maxf(tc.typeScale, 0.01f));
    }

    ctx_.displayScale = scale_;
}

void App::saveConfig() { configSave(config_); }

void App::refreshHen() {
    hen_ = probeHen();
    henProbePending_ = false;
}

// ---------------------------------------------------------------------------
void App::buildScreens() {
    screens_.clear();
    screens_.push_back(std::make_unique<DashboardScreen>());
    screens_.push_back(std::make_unique<InstallScreen>());
    screens_.push_back(std::make_unique<FilesScreen>());
    screens_.push_back(std::make_unique<HenScreen>());
    screens_.push_back(std::make_unique<CheatsScreen>());
    screens_.push_back(std::make_unique<SettingsScreen>());

    currentScreen_ = clampi(config_.lastScreen, 0, screenCount() - 1);
    pendingScreen_ = -1;
    transitioning_ = false;
    swapped_ = false;

    focus_.clear();
    focus_.setPreferred(-1);
    screens_[static_cast<size_t>(currentScreen_)]->onEnter(*this);
}

Screen& App::screen(int index) {
    return *screens_[static_cast<size_t>(clampi(index, 0, screenCount() - 1))];
}

void App::navigateTo(int index, bool animateBack) {
    (void)animateBack;
    if (screenCount() == 0) return;

    index = clampi(index, 0, screenCount() - 1);
    if (index == currentScreen_ && !transitioning_) return;

    if (transitioning_) {
        // Finish the current hop instantly rather than queueing.
        if (currentScreen_ != index) {
            screens_[static_cast<size_t>(currentScreen_)]->onLeave(*this);
            currentScreen_ = index;
            focus_.setPreferred(-1);
            screens_[static_cast<size_t>(currentScreen_)]->onEnter(*this);
            config_.lastScreen = currentScreen_;
        }
        transitioning_ = false;
        swapped_ = false;
        pendingScreen_ = -1;
        return;
    }

    pendingScreen_ = index;
    transitioning_ = true;
    swapped_ = false;
    transition_.snapTo(0.0f);
    transition_.start(kTransitionMs, motion::kStandard, maxf(theme_.motionScale, 0.0001f));
}

// ---------------------------------------------------------------------------
//  Layout.  Everything is authored against a 1080p canvas and multiplied by the
//  display scale, which keeps the proportions intact on other resolutions.
Rect App::railRect() const { return Rect(0.0f, 0.0f, L(348.0f), static_cast<float>(screenH_)); }

Rect App::headerRect() const {
    const float x = railRect().right() + L(48.0f);
    const float w = static_cast<float>(screenW_) - x - L(48.0f);
    return Rect(x, L(40.0f), w, L(104.0f));
}

Rect App::contentRect() const {
    const Rect h = headerRect();
    const float y = h.bottom() + L(8.0f);
    const float bottom = static_cast<float>(screenH_) - L(96.0f);
    return Rect(h.x, y, h.w, maxf(L(120.0f), bottom - y));
}

Rect App::hintRect() const {
    return Rect(railRect().right(), static_cast<float>(screenH_) - L(84.0f),
                static_cast<float>(screenW_) - railRect().right(), L(84.0f));
}

// ---------------------------------------------------------------------------
void App::toast(const std::string& message, bool systemToast) {
    if (message.empty()) return;

    Snack snack;
    snack.text = message;
    snack.total = kSnackLife;
    snack.life = kSnackLife;
    snacks_.push_back(std::move(snack));
    if (snacks_.size() > 3) snacks_.erase(snacks_.begin());

    if (systemToast) notify::toast(message);
}

void App::showConfirm(const std::string& title, const std::string& body, const std::string& accept,
                      std::function<void()> onAccept, bool destructive) {
    confirmTitle_ = title;
    confirmBody_ = body;
    confirmAccept_ = accept.empty() ? "OK" : accept;
    confirmAction_ = std::move(onAccept);
    confirmDestructive_ = destructive;
    confirmFocus_ = 1;  // default to the safe option
    confirmActive_ = true;
}

void App::closeConfirm() {
    confirmActive_ = false;
    confirmAction_ = nullptr;
}

// ---------------------------------------------------------------------------
void App::tick(float dt) {
    time_ += dt;
    ctx_.time = time_;
    ctx_.dt = dt;

    theme_.update(dt);

    for (auto& snack : snacks_) snack.life -= dt;
    while (!snacks_.empty() && snacks_.front().life <= 0.0f) snacks_.erase(snacks_.begin());

    spaceTimer_ -= dt;
    if (spaceTimer_ <= 0.0f) {
        spaceTimer_ = 5.0f;
        dataSpace_ = fs::space("/data");
    }

    henAutoTimer_ -= dt;
    if (henAutoTimer_ <= 0.0f) {
        henAutoTimer_ = 4.0f;
        refreshHen();
    }

    if (transitioning_) {
        transition_.step(dt);
        const float v = transition_.value();

        if (!swapped_ && v >= 0.5f && pendingScreen_ >= 0) {
            screens_[static_cast<size_t>(currentScreen_)]->onLeave(*this);
            currentScreen_ = pendingScreen_;
            pendingScreen_ = -1;
            focus_.setPreferred(-1);
            screens_[static_cast<size_t>(currentScreen_)]->onEnter(*this);
            config_.lastScreen = currentScreen_;
            saveConfig();
            swapped_ = true;
        }

        if (transition_.done()) {
            transitioning_ = false;
            swapped_ = false;
        }
    }

    if (currentScreen_ >= 0 && currentScreen_ < screenCount()) {
        screens_[static_cast<size_t>(currentScreen_)]->update(*this, dt);
    }
}

// ---------------------------------------------------------------------------
void App::render() {
    const Scheme& c = colors();
    canvas_.clear(c.surface);

    focus_.begin();
    drawRail();
    drawHeader();

    if (currentScreen_ >= 0 && currentScreen_ < screenCount()) {
        screens_[static_cast<size_t>(currentScreen_)]->draw(*this);
    }
    focus_.end();

    // Cross-fade mask over the content column while a transition runs.
    if (transitioning_) {
        const float v = transition_.value();
        const float alpha = (v < 0.5f) ? (v * 2.0f) : ((1.0f - v) * 2.0f);
        const float railRight = railRect().right();
        canvas_.fillRect(Rect(railRight, 0.0f, static_cast<float>(screenW_) - railRight,
                              static_cast<float>(screenH_)),
                         c.surface.withAlphaF(clampf(alpha, 0.0f, 1.0f)));
    }

    // Floating action button (Triangle).
    if (currentScreen_ >= 0 && currentScreen_ < screenCount()) {
        Screen& s = *screens_[static_cast<size_t>(currentScreen_)];
        const uint32_t fi = s.fabIcon();
        if (fi != 0) {
            const Rect content = contentRect();
            const float h = L(72.0f);
            const std::string label = s.fabLabel() ? s.fabLabel() : std::string();
            const float labelW = label.empty()
                                     ? 0.0f
                                     : ui::textWidth(ctx_, label, theme_.type.titleMedium) + L(12.0f);
            const float w = maxf(h, L(36.0f) + labelW + L(20.0f));
            const Rect r(content.right() - w, content.bottom() - h, w, h);

            const float proximity = ui::focusProximity(ctx_, r, L(140.0f));
            ui::fab(ctx_, r, fi, label, proximity, input_.pressed(Btn::Fab));
        }
    }

    if (!confirmActive_) ui::focusRing(ctx_, L(3.0f));

    drawHints();
    drawSnackbar();
    if (confirmActive_) drawConfirm();
}

void App::present() {
    if (scratch_) {
        SDL_BlitSurface(static_cast<SDL_Surface*>(scratch_), nullptr,
                        static_cast<SDL_Surface*>(windowSurface_), nullptr);
    }
    SDL_UpdateWindowSurface(static_cast<SDL_Window*>(window_));
}

void App::run() {
    uint32_t last = SDL_GetTicks();

    while (running_) {
        const uint32_t now = SDL_GetTicks();
        float dt = static_cast<float>(now - last) * 0.001f;
        last = now;
        if (dt > 0.05f) dt = 0.05f;
        if (dt < 0.0005f) dt = 0.0005f;

        input_.poll(dt);
        handleGlobalInput();

        tick(dt);
        render();
        present();

        const uint32_t spent = SDL_GetTicks() - now;
        if (spent < 16) SDL_Delay(16 - spent);
    }
}

// ---------------------------------------------------------------------------
void App::handleGlobalInput() {
    // Modal dialogs swallow everything.
    if (confirmActive_) {
        if (input_.pressed(Btn::Left) || input_.pressed(Btn::Right)) {
            confirmFocus_ = 1 - confirmFocus_;
        }
        if (input_.pressed(Btn::Back)) {
            closeConfirm();
        } else if (input_.pressed(Btn::Confirm)) {
            if (confirmFocus_ == 0) {
                std::function<void()> action = std::move(confirmAction_);
                closeConfirm();
                if (action) action();
            } else {
                closeConfirm();
            }
        }
        input_.consumeAll();
        return;
    }

    if (transitioning_) {
        input_.consumeAll();
        return;
    }

    if (input_.pressed(Btn::Options)) {
        config_.dark = !config_.dark;
        applyConfig(true);
        saveConfig();
        toast(config_.dark ? "Dark theme" : "Light theme");
    }

    if (input_.pressed(Btn::Prev) && screenCount() > 1) {
        navigateTo((currentScreen_ + screenCount() - 1) % screenCount());
        return;
    }
    if (input_.pressed(Btn::Next) && screenCount() > 1) {
        navigateTo((currentScreen_ + 1) % screenCount());
        return;
    }

    if (currentScreen_ < 0 || currentScreen_ >= screenCount()) return;
    Screen& s = *screens_[static_cast<size_t>(currentScreen_)];

    // Left/Right first go to the focused control, so sliders and segmented
    // buttons can be adjusted in place; anything else moves the highlight.
    if (input_.repeat(Btn::Left)) {
        if (!s.onAdjust(*this, focus_.currentId(), -1)) focus_.navigate(-1, 0);
    }
    if (input_.repeat(Btn::Right)) {
        if (!s.onAdjust(*this, focus_.currentId(), +1)) focus_.navigate(1, 0);
    }
    if (input_.repeat(Btn::Up)) focus_.navigate(0, -1);
    if (input_.repeat(Btn::Down)) focus_.navigate(0, 1);

    if (input_.pressed(Btn::Confirm)) {
        const int id = focus_.currentId();
        if (id >= kRailIdBase) {
            const int index = id - kRailIdBase;
            if (index >= 0 && index < screenCount() && index != currentScreen_) {
                navigateTo(index);
            }
        } else if (id >= 0) {
            s.onConfirm(*this, id);
        }
    }

    if (input_.pressed(Btn::Alt)) {
        const int id = focus_.currentId();
        if (id >= 0 && id < kRailIdBase) s.onAlt(*this, id);
    }

    if (input_.pressed(Btn::Fab)) s.onFab(*this);

    if (input_.pressed(Btn::Back)) {
        if (!s.onBack(*this)) {
            if (currentScreen_ != 0) {
                navigateTo(0);
            } else {
                showConfirm("Exit RAW UI", "Return to the PS4 menu?", "Exit", [this] { quit(); },
                            true);
            }
        }
    }
}

// ---------------------------------------------------------------------------
void App::drawRail() {
    const Scheme& c = colors();
    const Rect rail = railRect();
    const float pad = L(24.0f);

    canvas_.fillRect(rail, c.surfaceContainerLow);
    canvas_.fillRect(Rect(rail.right() - 1.0f, 0.0f, 1.0f, rail.h), c.outlineVariant);

    // Brand block.
    ui::drawText(ctx_, "RAW UI", theme_.type.titleLarge,
                 Rect(pad, L(36.0f), rail.w - pad * 2.0f, L(44.0f)), c.primary, ui::Align::Left,
                 ui::VAlign::Middle);
    ui::drawText(ctx_, "PlayStation 4 shell", theme_.type.labelMedium,
                 Rect(pad, L(84.0f), rail.w - pad * 2.0f, L(28.0f)), c.onSurfaceVariant,
                 ui::Align::Left, ui::VAlign::Middle);

    // Destinations.
    const float itemH = L(72.0f);
    const float gap = L(10.0f);
    float y = L(150.0f);

    for (int i = 0; i < screenCount(); ++i) {
        const Rect item(pad, y, rail.w - pad * 2.0f, itemH);
        if (item.bottom() > rail.bottom() - L(220.0f)) break;

        const int id = railId(i);
        const bool selected = (i == currentScreen_);
        const float proximity =
            (focus_.currentId() == id) ? ui::focusProximity(ctx_, item, L(80.0f)) : 0.0f;

        Screen& s = *screens_[static_cast<size_t>(i)];
        ui::navRailItem(ctx_, item, s.icon(), s.title(), selected, proximity);
        focus_.add(item, id, true, L(28.0f));

        y += itemH + gap;
    }

    // Footer status card.
    const float cardH = L(132.0f);
    const Rect card(pad, rail.bottom() - cardH - L(28.0f), rail.w - pad * 2.0f, cardH);
    ui::paintSurface(ctx_, card, L(20.0f), 1, c.surfaceContainer, c.outlineVariant);

    const float inner = L(18.0f);
    const bool henOk = hen_.running;
    const Color accent = henOk ? c.success : c.warning;

    ctx_.canvas->fillCircle(card.x + inner + L(9.0f), card.y + inner + L(11.0f), L(9.0f),
                            accent);
    ui::drawText(ctx_, "GoldHEN",
                 theme_.type.titleSmall,
                 Rect(card.x + inner + L(26.0f), card.y + inner, card.w, L(24.0f)), c.onSurface,
                 ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx_, henOk ? "Running" : "Not detected", theme_.type.bodySmall,
                 Rect(card.x + inner + L(26.0f), card.y + inner + L(24.0f), card.w, L(22.0f)),
                 accent, ui::Align::Left, ui::VAlign::Middle);

    ui::divider(ctx_, Rect(card.x + inner, card.y + L(70.0f), card.w - inner * 2.0f, 1.0f),
                c.outlineVariant);

    std::string storage = "Storage unavailable";
    if (dataSpace_.valid) storage = humanSize(dataSpace_.free) + " free";
    ui::drawText(ctx_, storage, theme_.type.bodySmall,
                 Rect(card.x + inner, card.y + L(78.0f), card.w - inner * 2.0f, L(22.0f)),
                 c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
    ui::drawText(ctx_, clockText(), theme_.type.bodySmall,
                 Rect(card.x + inner, card.y + L(100.0f), card.w - inner * 2.0f, L(22.0f)),
                 c.onSurfaceVariant, ui::Align::Left, ui::VAlign::Middle);
}

void App::drawHeader() {
    const Scheme& c = colors();
    const Rect head = headerRect();
    if (currentScreen_ < 0 || currentScreen_ >= screenCount()) return;

    Screen& s = *screens_[static_cast<size_t>(currentScreen_)];
    const float chipZone = L(360.0f);
    const float textW = head.w - chipZone;

    const std::string title = fitText(ctx_, s.title(), theme_.type.headlineMedium, textW);
    ui::drawText(ctx_, title, theme_.type.headlineMedium,
                 Rect(head.x, head.y, textW, L(58.0f)), c.onSurface, ui::Align::Left,
                 ui::VAlign::Bottom);

    const std::string sub = fitText(ctx_, s.subtitle(*this), theme_.type.bodyMedium, textW);
    if (!sub.empty()) {
        ui::drawText(ctx_, sub, theme_.type.bodyMedium,
                     Rect(head.x, head.y + L(58.0f), textW, L(34.0f)), c.onSurfaceVariant,
                     ui::Align::Left, ui::VAlign::Top);
    }

    // Status chips, laid out from the right edge.
    const float chipH = L(44.0f);
    const float chipY = head.y + (head.h - chipH) * 0.5f;
    float x = head.right();

    const auto chip = [&](uint32_t icon, const std::string& text, const Color& fg,
                          const Color& bg) {
        const float w = L(24.0f) + ui::textWidth(ctx_, text, theme_.type.labelLarge) + L(34.0f);
        x -= w;
        if (x < head.x + textW) return;
        ui::statusChip(ctx_, Rect(x, chipY, w, chipH), icon, text, fg, bg);
        x -= L(12.0f);
    };

    chip(icons::kStorage, dataSpace_.valid ? humanSize(dataSpace_.free) + " free" : "—",
         c.onSurfaceVariant, c.surfaceContainerHigh);

    if (hen_.running) {
        chip(icons::kSecurity, "HEN " + (hen_.version.empty() ? std::string("on") : hen_.version),
             c.onSuccessContainer, c.successContainer);
    } else {
        chip(icons::kSecurity, "No HEN", c.onSurfaceVariant, c.surfaceContainerHighest);
    }
}

void App::drawHints() {
    const Scheme& c = colors();
    const Rect hint = hintRect();
    canvas_.fillRect(Rect(hint.x, hint.y, hint.w, 1.0f), c.outlineVariant);

    if (currentScreen_ >= 0 && currentScreen_ < screenCount()) {
        std::vector<ui::Hint> hints = screens_[static_cast<size_t>(currentScreen_)]->hints();
        ui::hintBar(ctx_, hint, hints);
    }
}

void App::drawSnackbar() {
    if (snacks_.empty()) return;

    const Scheme& c = colors();
    const Rect content = contentRect();
    const float h = L(64.0f);
    const float gap = L(12.0f);

    float bottom = hintRect().y - L(14.0f);

    for (size_t i = 0; i < snacks_.size(); ++i) {
        const Snack& snack = snacks_[snacks_.size() - 1 - i];

        // Fade in quickly, fade out over the last 0.6s.
        float alpha = 1.0f;
        const float elapsed = snack.total - snack.life;
        if (elapsed < 0.16f) alpha = elapsed / 0.16f;
        if (snack.life < 0.6f) alpha = minf(alpha, snack.life / 0.6f);
        alpha = clampf(alpha, 0.0f, 1.0f);

        const std::string text =
            fitText(ctx_, snack.text, theme_.type.bodyMedium, content.w - L(64.0f));
        const float w = ui::textWidth(ctx_, text, theme_.type.bodyMedium) + L(64.0f);
        const Rect r(content.x + (content.w - w) * 0.5f, bottom - h, w, h);

        ui::paintSurface(ctx_, r.translated(0.0f, (1.0f - alpha) * L(10.0f)), L(16.0f), 3,
                         c.inverseSurface.withAlphaF(alpha * 0.96f));
        ui::drawText(ctx_, text, theme_.type.bodyMedium, r, c.inverseOnSurface.withAlphaF(alpha),
                     ui::Align::Center, ui::VAlign::Middle);

        bottom -= h + gap;
    }
}

void App::drawConfirm() {
    const Scheme& c = colors();
    const float s = displayScaleDefault() * layoutScale_;

    canvas_.fillRect(Rect(0.0f, 0.0f, static_cast<float>(screenW_),
                          static_cast<float>(screenH_)),
                     Color(0, 0, 0, 150));

    const float cardW = minf(L(640.0f), static_cast<float>(screenW_) - L(160.0f));
    const float pad = L(32.0f);
    const float cardH = L(96.0f) + L(60.0f) + L(56.0f) + pad * 2.0f;
    const Rect card((static_cast<float>(screenW_) - cardW) * 0.5f,
                    (static_cast<float>(screenH_) - cardH) * 0.5f, cardW, cardH);

    ui::paintSurface(ctx_, card, L(28.0f), 4, c.surfaceContainerHigh);

    ui::drawText(ctx_, confirmTitle_, theme_.type.headlineSmall,
                 Rect(card.x + pad, card.y + pad, card.w - pad * 2.0f, L(40.0f)), c.onSurface,
                 ui::Align::Left, ui::VAlign::Middle);
    ui::drawParagraph(ctx_, confirmBody_, theme_.type.bodyMedium,
                      Rect(card.x + pad, card.y + pad + L(52.0f), card.w - pad * 2.0f, L(72.0f)),
                      c.onSurfaceVariant, L(26.0f));

    const float buttonH = L(56.0f);
    const float buttonW = (card.w - pad * 3.0f) * 0.5f;
    const float buttonY = card.bottom() - pad - buttonH;

    const Rect cancel(card.x + pad, buttonY, buttonW, buttonH);
    const Rect accept(card.right() - pad - buttonW, buttonY, buttonW, buttonH);

    const float ring = L(3.0f);
    if (confirmFocus_ == 1) {
        ctx_.canvas->strokeRoundRect(cancel.outset(ring * 0.5f), cancel.h * 0.5f, ring,
                                     c.primary);
    } else {
        ctx_.canvas->strokeRoundRect(accept.outset(ring * 0.5f), accept.h * 0.5f, ring,
                                     c.primary);
    }

    ui::button(ctx_, cancel, "Cancel", 0, ui::ButtonStyle::Text, 0.0f, false);
    ui::button(ctx_, accept, confirmAccept_, 0,
               confirmDestructive_ ? ui::ButtonStyle::Filled : ui::ButtonStyle::Filled,
               confirmDestructive_ ? 0.0f : 0.0f, false);
    (void)s;
}

}  // namespace rawui
