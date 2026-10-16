// ============================================================================
//  RAW UI - controller input.
//
//  A DualShock 4 is the only input device, so the mapping is fixed and the
//  analogue stick is folded into the same directional model as the d-pad.  Held
//  directions auto-repeat, which is what TV UIs need for long lists.
// ============================================================================
#pragma once

namespace rawui {

enum class Btn {
    Up,
    Down,
    Left,
    Right,
    Confirm,  // Cross
    Back,     // Circle
    Alt,      // Square - secondary action on the focused item
    Fab,      // Triangle - the screen's primary action
    Options,  // Start
    Prev,     // L1
    Next,     // R1
    Count
};

class Input {
public:
    bool init();
    void shutdown();

    // Pumps SDL events and recomputes the button state.  Call once per frame.
    void poll(float dt);

    bool down(Btn b) const { return current_[static_cast<int>(b)]; }
    bool pressed(Btn b) const { return pressedEdge_[static_cast<int>(b)]; }
    bool repeat(Btn b) const { return repeatEdge_[static_cast<int>(b)]; }
    bool consumed(Btn b) const { return consumed_[static_cast<int>(b)]; }

    // Marks a button press as handled so it does not bubble to the nav rail.
    void consume(Btn b) { consumed_[static_cast<int>(b)] = true; }
    void consumeAll();

    bool connected() const { return controller_ != nullptr; }
    bool anyNavPressed() const;

    // Raw stick values (-1..1) after dead zone, for parallax effects.
    float stickX() const { return stickX_; }
    float stickY() const { return stickY_; }

    // Milliseconds since the last input of any kind (drives idle animations).
    float idleSeconds() const { return idleSeconds_; }

private:
    bool current_[static_cast<int>(Btn::Count)] = {};
    bool previous_[static_cast<int>(Btn::Count)] = {};
    bool pressedEdge_[static_cast<int>(Btn::Count)] = {};
    bool repeatEdge_[static_cast<int>(Btn::Count)] = {};
    bool consumed_[static_cast<int>(Btn::Count)] = {};
    float holdTime_[static_cast<int>(Btn::Count)] = {};
    float nextRepeat_[static_cast<int>(Btn::Count)] = {};

    void* controller_ = nullptr;  // SDL_GameController*
    void* joystick_ = nullptr;    // SDL_Joystick*

    float stickX_ = 0.0f;
    float stickY_ = 0.0f;
    float idleSeconds_ = 0.0f;
};

}  // namespace rawui
