// ============================================================================
//  RAW UI - application shell.
//
//  Owns the SDL window, the software frame buffer, the theme, the fonts, the
//  navigation rail and the snackbar.  Screens only ever see the content area.
// ============================================================================
#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "app/screen.hpp"
#include "gfx/canvas.hpp"
#include "gfx/fontcache.hpp"
#include "md3/motion.hpp"
#include "md3/theme.hpp"
#include "platform/config.hpp"
#include "platform/fs.hpp"
#include "platform/goldhen.hpp"
#include "platform/input.hpp"
#include "ui/focus.hpp"

namespace rawui {

class App {
public:
    App();
    ~App();

    bool init();
    void run();
    void shutdown();

    // ------------------------------------------------------------ accessors --
    ui::Ctx& ctx() { return ctx_; }
    Canvas& canvas() { return canvas_; }
    Theme& theme() { return theme_; }
    FocusManager& focus() { return focus_; }
    Input& input() { return input_; }
    FontCache& fonts() { return fonts_; }
    const Scheme& colors() const { return theme_.scheme; }

    AppConfig& config() { return config_; }
    void applyConfig(bool animate);
    void saveConfig();

    // Seconds since the app started - drives the idle animations.
    float time() const { return time_; }

    // Live HEN status (cached; refreshed on demand).
    const HenStatus& hen() const { return hen_; }
    const fs::SpaceInfo& dataSpace() const { return dataSpace_; }
    void refreshHen();
    bool henBusy() const { return henProbePending_; }

    // ---------------------------------------------------------------- layout --
    int screenWidth() const { return screenW_; }
    int screenHeight() const { return screenH_; }
    Rect railRect() const;
    Rect headerRect() const;
    Rect contentRect() const;
    Rect hintRect() const;

    // ------------------------------------------------------------ navigation --
    void navigateTo(int index, bool animateBack = false);
    int currentScreen() const { return currentScreen_; }
    int screenCount() const { return static_cast<int>(screens_.size()); }
    Screen& screen(int index);

    // ------------------------------------------------------------ messaging --
    void toast(const std::string& message, bool systemToast = false);
    void quit() { running_ = false; }

    // Overlay used for confirmations ("Delete this file?").
    void showConfirm(const std::string& title, const std::string& body, const std::string& accept,
                     std::function<void()> onAccept, bool destructive = false);
    void closeConfirm();
    bool confirmVisible() const { return confirmActive_; }

private:
    void buildScreens();
    void handleGlobalInput();
    void render();
    void present();
    void drawRail();
    void drawHeader();
    void drawHints();
    void drawSnackbar();
    void drawConfirm();
    void tick(float dt);

    // Design geometry multiplier: display scale x terminal scale (1080p = 1.3).
    float L(float v) const { return v * scale_; }

    // SDL
    void* window_ = nullptr;
    void* windowSurface_ = nullptr;  // SDL_Surface*
    void* scratch_ = nullptr;        // SDL_Surface* when formats differ
    int screenW_ = 1920;
    int screenH_ = 1080;
    float scale_ = displayScaleDefault();

    Canvas canvas_;
    ui::Ctx ctx_;

    Theme theme_;
    FontCache fonts_;
    Input input_;
    FocusManager focus_;

    AppConfig config_;
    HenStatus hen_;
    bool henProbePending_ = false;

    std::vector<std::unique_ptr<Screen>> screens_;
    int currentScreen_ = 0;
    int pendingScreen_ = -1;

    // Screen transition (a fade mask over the content area).
    Tween transition_;
    bool transitioning_ = false;
    bool swapped_ = false;

    std::string assetsMissing_;

    // Snackbar
    struct Snack {
        std::string text;
        float life = 0.0f;
        float total = 0.0f;
    };
    std::vector<Snack> snacks_;

    // Confirm dialog
    bool confirmActive_ = false;
    std::string confirmTitle_;
    std::string confirmBody_;
    std::string confirmAccept_;
    std::function<void()> confirmAction_;
    int confirmFocus_ = 1;  // 0 = accept, 1 = cancel
    bool confirmDestructive_ = false;

    fs::SpaceInfo dataSpace_;
    float spaceTimer_ = 0.0f;
    float henAutoTimer_ = 0.0f;
    float layoutScale_ = 1.0f;

    // Cached result of nightWindowActive(); applyConfig() refreshes it and
    // tick() only wakes up when it actually flips.
    bool nightActive_ = false;
    float nightTimer_ = 0.0f;

    bool running_ = false;
    float time_ = 0.0f;
};

}  // namespace rawui
