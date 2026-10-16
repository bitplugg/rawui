#include "platform/input.hpp"

#include <SDL2/SDL.h>

#include "core/log.hpp"
#include "core/math.hpp"

namespace rawui {
namespace {

constexpr float kDeadZone = 0.42f;
constexpr float kRepeatDelayMs = 380.0f;
constexpr float kRepeatRateMs = 95.0f;

constexpr int kBtnCount = static_cast<int>(Btn::Count);

}  // namespace

bool Input::init() {
    if (SDL_WasInit(SDL_INIT_GAMECONTROLLER) == 0) {
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK);
    }

    // SDL maps the DualShock 4 through its game controller database, so the
    // button names below are the portable ones (A/B/X/Y, dpad, shoulders).
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            controller_ = SDL_GameControllerOpen(i);
            if (controller_) break;
        }
    }

    if (!controller_ && SDL_NumJoysticks() > 0) {
        joystick_ = SDL_JoystickOpen(0);
    }

    RU_LOG("[input] controllers=%d opened=%s", SDL_NumJoysticks(),
           controller_ ? "yes" : (joystick_ ? "joystick" : "no"));
    return controller_ != nullptr || joystick_ != nullptr;
}

void Input::shutdown() {
    if (controller_) {
        SDL_GameControllerClose(static_cast<SDL_GameController*>(controller_));
        controller_ = nullptr;
    }
    if (joystick_) {
        SDL_JoystickClose(static_cast<SDL_Joystick*>(joystick_));
        joystick_ = nullptr;
    }
}

void Input::poll(float dt) {
    for (int i = 0; i < kBtnCount; ++i) previous_[i] = current_[i];

    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_CONTROLLERDEVICEADDED || ev.type == SDL_JOYDEVICEADDED) {
            if (!controller_ && !joystick_) init();
        }
    }

    bool raw[kBtnCount] = {};

    SDL_GameController* gc = static_cast<SDL_GameController*>(controller_);
    SDL_Joystick* js = static_cast<SDL_Joystick*>(joystick_);

    if (gc) {
        raw[static_cast<int>(Btn::Confirm)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_A);
        raw[static_cast<int>(Btn::Back)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_B);
        raw[static_cast<int>(Btn::Alt)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_X);
        raw[static_cast<int>(Btn::Fab)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_Y);
        raw[static_cast<int>(Btn::Options)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_START);
        raw[static_cast<int>(Btn::Prev)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        raw[static_cast<int>(Btn::Next)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

        raw[static_cast<int>(Btn::Up)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_UP);
        raw[static_cast<int>(Btn::Down)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        raw[static_cast<int>(Btn::Left)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        raw[static_cast<int>(Btn::Right)] =
            SDL_GameControllerGetButton(gc, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

        stickX_ = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f;
        stickY_ = SDL_GameControllerGetAxis(gc, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f;
    } else if (js) {
        // Raw DualShock 4 layout, which is what the bundled SDL exposes when no
        // game-controller mapping is available.  Square/Cross/Circle/Triangle
        // are buttons 0..3, L1/R1 are 4/5, Options is 9, and the d-pad is a hat.
        const auto btn = [js](int i) { return SDL_JoystickGetButton(js, i) != 0; };

        raw[static_cast<int>(Btn::Confirm)] = btn(1);
        raw[static_cast<int>(Btn::Back)] = btn(2);
        raw[static_cast<int>(Btn::Alt)] = btn(0);
        raw[static_cast<int>(Btn::Fab)] = btn(3);
        raw[static_cast<int>(Btn::Options)] = btn(9);

        // Triggers also act as page up/down, which is handy in long lists.
        raw[static_cast<int>(Btn::Prev)] = btn(4) || SDL_JoystickGetAxis(js, 4) > 16000;
        raw[static_cast<int>(Btn::Next)] = btn(5) || SDL_JoystickGetAxis(js, 5) > 16000;

        const Uint8 hat = SDL_JoystickGetHat(js, 0);
        raw[static_cast<int>(Btn::Up)] = (hat & SDL_HAT_UP) != 0;
        raw[static_cast<int>(Btn::Down)] = (hat & SDL_HAT_DOWN) != 0;
        raw[static_cast<int>(Btn::Left)] = (hat & SDL_HAT_LEFT) != 0;
        raw[static_cast<int>(Btn::Right)] = (hat & SDL_HAT_RIGHT) != 0;

        stickX_ = SDL_JoystickGetAxis(js, 0) / 32767.0f;
        stickY_ = SDL_JoystickGetAxis(js, 1) / 32767.0f;
    } else {
        stickX_ = stickY_ = 0.0f;
    }

    // Analogue stick feeds the same four directional buttons.
    if (stickY_ < -kDeadZone) raw[static_cast<int>(Btn::Up)] = true;
    if (stickY_ > kDeadZone) raw[static_cast<int>(Btn::Down)] = true;
    if (stickX_ < -kDeadZone) raw[static_cast<int>(Btn::Left)] = true;
    if (stickX_ > kDeadZone) raw[static_cast<int>(Btn::Right)] = true;

    const float dtMs = dt * 1000.0f;
    bool anyInput = false;

    for (int i = 0; i < kBtnCount; ++i) {
        current_[i] = raw[i];
        pressedEdge_[i] = raw[i] && !previous_[i];
        repeatEdge_[i] = false;
        consumed_[i] = false;

        if (current_[i]) {
            anyInput = true;
            holdTime_[i] += dtMs;
            if (pressedEdge_[i]) {
                nextRepeat_[i] = kRepeatDelayMs;
            } else if (holdTime_[i] >= nextRepeat_[i]) {
                repeatEdge_[i] = true;
                nextRepeat_[i] = holdTime_[i] + kRepeatRateMs;
            }
        } else {
            holdTime_[i] = 0.0f;
            nextRepeat_[i] = 0.0f;
        }
    }

    idleSeconds_ = anyInput ? 0.0f : idleSeconds_ + dt;
}

void Input::consumeAll() {
    for (int i = 0; i < kBtnCount; ++i) consumed_[i] = true;
}

bool Input::anyNavPressed() const {
    return repeat(Btn::Up) || repeat(Btn::Down) || repeat(Btn::Left) || repeat(Btn::Right);
}

}  // namespace rawui
